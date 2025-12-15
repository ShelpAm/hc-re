#include <hc/server.h>

#include <hc/api-admin.h>
#include <hc/archive.h>
#include <hc/assignments-submit-param.h>
#include <hc/config.h>
#include <hc/debug.h>

#include <archive.h>
#include <boost/uuid.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <fstream>
#include <print>

using namespace std::chrono_literals;

namespace fs = std::filesystem;
namespace uuid = boost::uuids;

using httplib::Request;
using httplib::Response;
using httplib::StatusCode;

std::map<std::string, Student> load_students(sqlpp::postgresql::connection &db)
{
    constexpr auto s = schema::Student{};
    auto res = db(sqlpp::select(s.student_id, s.name).from(s));
    std::map<std::string, Student> students;
    for (auto const &r : res) {
        students.insert({std::string{r.student_id},
                         Student{.student_id{r.student_id}, .name{r.name}}});
    }
    return students;
}

std::map<std::string, Assignment>
load_assignments(sqlpp::postgresql::connection &db)
{
    constexpr auto a = schema::Assignment{};
    constexpr auto s = schema::Submission{};

    // Instead of query per assignment, this increase performance by query all
    // data with one time.
    auto res =
        db(sqlpp::select(a.name, a.start_time, a.end_time, s.student_id,
                         s.submission_time, s.filepath, s.original_filename)
               .from(a.left_outer_join(s).on(a.name == s.assignment_name)));

    std::map<std::string, Assignment> assignments;

    for (auto const &r : res) {
        auto assignment_name = std::string{r.name};
        if (!assignments.contains(assignment_name)) {
            assignments.insert(
                {assignment_name, Assignment(std::string{r.name}, r.start_time,
                                             r.end_time, {})});
        }

        if (r.student_id.has_value()) {
            assignments.at(assignment_name)
                .submissions.insert(
                    {std::string{r.student_id.value()},
                     Submission{
                         .assignment_name{r.student_id.value()},
                         .student_id{r.student_id.value()},
                         .submission_time{r.submission_time.value()},
                         .filepath{r.filepath.value()},
                         .original_filename{r.original_filename.value()}}});
        }
    }

    return assignments;
}

Server::Server(sqlpp::postgresql::connection &&db) : db_(std::move(db))
{
    if (!db_.is_connected()) {
        throw std::runtime_error{"Failed to connect to server"};
    }

    // db_("SET timezone TO 'Asia/Shanghai'");
    // db_("INSERT INTO fuck (t) VALUES ('2025-12-03 17:00:00+00');");
    students_ = load_students(db_);
    assignments_ = load_assignments(db_);

    for (auto const &[_, v] : students_)
        spdlog::debug("student=> student_id: {}, name: {}", v.student_id,
                      v.name);

    for (auto const &[_, v] : assignments_) {
        spdlog::debug("assignment=> name: {}, start_time: {:%Y-%m-%d "
                      "%H:%M:%S}, end_time: {:%Y-%m-%d %H:%M:%S}",
                      v.name, v.start_time, v.end_time);
    }

    http_server_.set_logger([](Request const &req, Response const &res) {
        spdlog::info("{:4} {:25} -> {}", req.method, req.path, res.status);
    });

    http_server_.set_error_handler([](Request const &req, Response const &res) {
        if (res.status == StatusCode::NotFound_404) {
            spdlog::error("Strange access: {} {} -> {}", req.method, req.path,
                          res.status);
        }
    });

    http_server_.set_exception_handler(
        [](Request const &_, Response &w, std::exception_ptr ep) {
            try {
                std::rethrow_exception(std::move(ep));
            }
            catch (nlohmann::json::parse_error &e) {
                w.status = StatusCode::BadRequest_400;
                w.set_content("Bad request json format", "text/plain");
                spdlog::error("Bad request json format: {}", e.what());
            }
            catch (std::exception &e) {
                w.status = StatusCode::InternalServerError_500;
                spdlog::error("Server interal error: {}", e.what());
            }
        });

    // === 全局 CORS 中间件 ===
    http_server_.set_pre_routing_handler(
        [](httplib::Request const &req, httplib::Response &res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods",
                           "GET, POST, PUT, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers",
                           "Content-Type, Authorization");

            // 处理 OPTIONS 预检
            if (req.method == "OPTIONS") {
                res.status = 204; // No Content
                return httplib::Server::HandlerResponse::Handled;
            }

            return httplib::Server::HandlerResponse::Unhandled;
        });

    get("/hi", &Server::hi);

    get("/api/assignments", &Server::api_assignments);
    post("/api/assignments/add", &Server::api_assignments_add);
    post("/api/assignments/submit", &Server::api_assignments_submit);
    post("/api/assignments/export", &Server::api_assignments_export);

    get("/api/students", &Server::api_students);
    post("/api/students/add", &Server::api_students_add);

    post("/api/admin/login", &Server::api_admin_login);
    post("/api/admin/verify", &Server::api_admin_verify_token);
    post("/api/stop", &Server::api_stop);
}

Server::~Server() noexcept
{
    if (is_running()) {
        stop();
    }
}

void Server::start(std::string const &host, std::uint16_t port)
{
    std::scoped_lock guard(http_lock_);
    if (is_running()) {
        throw std::runtime_error{"Server already started"};
    }

    server_thread_.emplace([this, host, port]() {
        spdlog::info("Server binding to {}:{}", host, port);
        if (!http_server_.bind_to_port(host, port)) {
            throw std::runtime_error{"Address already in use"};
        }
        spdlog::info("Server listening to {}:{}", host, port);
        http_server_.listen_after_bind();
        spdlog::info("Server stopped.");
    });
    wait_until_started();
}

void Server::stop() noexcept
{
    std::scoped_lock guard(http_lock_);
    if (!is_running()) {
        spdlog::warn("stop() called but server is not running. This is caused "
                     "probably because of race condition");
        return;
    }

    clean_all_files();
    http_server_.stop();
    spdlog::info("Waiting for the internal server to stop gracefully");
    // Wait for the server to stop gracefully
    wait_until_stopped();
    server_thread_.reset();
}

bool Server::is_running() const noexcept
{
    return server_thread_.has_value();
}

bool Server::verify_assignment_not_exists(std::string_view assignment_name,
                                          httplib::Response &w) noexcept
{
    using httplib::StatusCode;
    if (assignments_.contains(std::string{assignment_name})) {
        auto const err =
            std::format("Assignment '{}' already exists.", assignment_name);
        w.status = StatusCode::BadRequest_400;
        w.set_content(err, "text/plain");
        spdlog::warn("{} Ignoring request.", err);
        return false;
    }
    return true;
}

bool Server::verify_assignment_exists(std::string_view assignment_name,
                                      httplib::Response &w) noexcept
{
    using httplib::StatusCode;
    if (!assignments_.contains(std::string{assignment_name})) {
        auto const err =
            std::format("Assignment '{}' doesn't exist.", assignment_name);
        w.status = StatusCode::BadRequest_400;
        w.set_content(err, "text/plain");
        spdlog::warn("{} Ignoring request.", err);
        return false;
    }
    return true;
}

bool Server::verify_student_exists(std::string_view student_id,
                                   std::string_view student_name,
                                   httplib::Response &w) noexcept
{
    using httplib::StatusCode;
    if (!students_.contains(std::string{student_id}) ||
        students_.at(std::string{student_id}).name != student_name) {
        auto const err = std::format("Student {} {} doesn't exist.", student_id,
                                     student_name);
        w.status = StatusCode::BadRequest_400;
        w.set_content(err, "text/plain");
        spdlog::warn("{} Ignoring request.", err);
        return false;
    }
    return true;
}

bool Server::verify_student_not_exists(std::string_view student_id,
                                       httplib::Response &w) noexcept
{
    using httplib::StatusCode;
    if (!students_.contains(std::string{student_id})) {
        auto const err =
            std::format("Student '{}' already exists.", student_id, student_id);
        w.status = StatusCode::BadRequest_400;
        w.set_content(err, "text/plain");
        spdlog::warn("{} Ignoring request.", err);
        return false;
    }
    return true;
}

void Server::clean_all_files()
{
    spdlog::info("Cleaning temporary directory");
    fs::remove_all(fs::temp_directory_path() / "hc");
}

void Server::clean_expired_files()
{
    spdlog::info("Cleaning expired files");
    auto const now = SystemClock::now();
    while (!tmp_files_.empty() && tmp_files_.front().first <= now) {
        fs::remove(tmp_files_.front().second);
        tmp_files_.pop();
    }
}

void Server::wait_until_started() noexcept
{
    while (!http_server_.is_running()) {
        std::this_thread::sleep_for(10ms);
    }
}

void Server::wait_until_stopped() noexcept
{
    while (http_server_.is_running()) {
        std::this_thread::sleep_for(10ms);
    }
}

void Server::get(std::string const &path, Handler h)
{
    http_server_.Get(path, std::bind_front(h, this));
}

void Server::post(std::string const &path, Handler h)
{
    http_server_.Post(path, std::bind_front(h, this));
}

void Server::api_stop(Request const & /*unused*/, Response & /*unused*/)
{
    // 在单独线程中执行真正的 stop()，避免在 server 的 handler 线程中调用
    std::thread([this]() { this->stop(); }).detach();
}
void Server::api_admin_login(Request const &r, Response &w)
{
    auto const j = nlohmann::json::parse(r.body);
    auto params = j.get<AdminLoginParams>();
    spdlog::info("Admin Login: {} {}", params.username, params.password);

    if (params.username != "xhw" || params.password != "xhw") {
        w.status = StatusCode::BadRequest_400;
        return;
    }

    boost::uuids::random_generator gen;
    auto const token = to_string(gen());
    tokens_.insert(token);
    spdlog::info("Generated token: {}", token);
    AdminLoginResult result{.token{token}};
    w.set_content(nlohmann::json(result).dump(), "application/json");
}
void Server::api_admin_verify_token(Request const &r, Response &w)
{
    auto const j = nlohmann::json::parse(r.body);
    auto params = j.get<AdminVerifyTokenParams>();

    AdminVerifyTokenResult result{.ok = tokens_.contains(params.token)};
    w.set_content(nlohmann::json(result).dump(), "application/json");
};
void Server::api_assignments(Request const &_, Response &w)
{
    std::shared_lock guard{lock_};
    auto const j = nlohmann::json(assignments_ | std::views::values |
                                  std::ranges::to<std::vector>());
    guard.unlock();
    w.set_content(j.dump(), "application/json");
    spdlog::debug("Responded: assignments: {}", j.dump());
}

void Server::api_assignments_add(Request const &r, Response &w)
{
    auto const j = nlohmann::json::parse(r.body);
    spdlog::info("Assignment Add Request: {}", j.dump());
    auto a = j.get<Assignment>();
    spdlog::debug("Time parsed as from {} to {}", a.start_time, a.end_time);
    std::unique_lock guard{lock_};
    if (!verify_assignment_not_exists(a.name, w)) {
        return;
    }
    assignments_.insert({a.name, a});
    constexpr auto ta = schema::Assignment{};
    auto insert = sqlpp::insert_into(ta).set(ta.name = a.name,
                                             ta.start_time = a.start_time,
                                             ta.end_time = a.end_time);
    db_(insert);
}

void Server::api_assignments_submit(Request const &r, Response &w)
{
    auto const j = nlohmann::json::parse(r.body);
    auto params = j.get<AssignmentsSubmitParams>();

    spdlog::info("Assignment Submit Request: assignment: {}, name: {}, "
                 "school_id: {}",
                 params.assignment_name, params.student_name,
                 params.student_id);

    std::unique_lock guard{lock_};
    if (!verify_student_exists(params.student_id, params.student_name, w)) {
        return;
    }

    if (!verify_assignment_exists(params.assignment_name, w)) {
        return;
    }

    auto &a = assignments_.at(params.assignment_name);

    constexpr auto ts = schema::Submission{};
    if (a.submissions.contains(params.student_id)) {
        fs::remove(a.submissions.at(params.student_id).filepath);
        db_(sqlpp::delete_from(ts).where(ts.assignment_name ==
                                             params.assignment_name &&
                                         ts.student_id == params.student_id));
    }

    uuid::random_generator gen;
    auto const uuid = gen();
    auto const filename = uuid::to_string(uuid);
    auto const filedir = config::datahome() / "files";
    fs::create_directories(filedir);
    auto const filepath = filedir / filename;

    using base64 = cppcodec::base64_rfc4648;
    auto file = base64::decode(params.file.content);

    std::ofstream ofs(filepath, std::ios::binary);
    // NOLINTNEXTLINE
    ofs.write(reinterpret_cast<char const *>(file.data()), file.size());

    auto const s = Submission{
        .assignment_name{params.assignment_name},
        .student_id{params.student_id},
        .submission_time{TimePoint{UtcClock::now().time_since_epoch()}},
        .filepath{filepath},
        .original_filename{params.file.filename},
    };

    a.submissions[params.student_id] = s;

    db_(sqlpp::insert_into(ts).set(ts.student_id = s.student_id,
                                   ts.submission_time = s.submission_time,
                                   ts.assignment_name = s.assignment_name,
                                   ts.original_filename = s.original_filename,
                                   ts.filepath = s.filepath.string()));
}

void Server::api_assignments_export(Request const &r, Response &w)
{
    auto const j = nlohmann::json::parse(r.body);
    auto param = j.get<ApiAssignmentsExportParam>();
    std::shared_lock guard{lock_};
    if (!verify_assignment_exists(param.assignment_name, w)) {
        return;
    }
    auto &a = assignments_.at(param.assignment_name);

    // ARCHIVE
    // assignment_name
    // |- student_id+student_name
    // |  |- filename
    // |-...
    uuid::random_generator gen;

    auto const tmpdir = fs::temp_directory_path() / "hc" / to_string(gen());
    auto const adir = tmpdir / a.name;
    fs::create_directories(adir);
    for (auto const &[_, sub] : a.submissions) {
        auto const &stu = students_.at(sub.student_id);
        auto const studir = adir / (stu.student_id + stu.name);
        fs::create_directory(studir);
        fs::copy_file(sub.filepath, studir / sub.original_filename);
    }
    auto const filepath = tmpdir / (to_string(gen()) + ".tar.zst");
    std::vector dirs{adir};
    hc::archive::create_tar_zst(filepath, dirs);
    fs::remove_all(adir);
    w.set_file_content(filepath.string(), "application/x-zstd-compressed-tar");
    w.set_header("Content-Disposition",
                 std::format("attachment; filename=\"{}\"",
                             std::string(std::from_range,
                                         filepath.filename().u8string())));

    // Cleanups
    tmp_files_.push({SystemClock::now() + 1h, filepath});
    clean_expired_files(); // Clean files on request
}

void Server::api_students(Request const &_, Response &w)
{
    spdlog::info("Student List Request");

    std::shared_lock guard{lock_};
    auto const j = nlohmann::json(students_ | std::views::values |
                                  std::ranges::to<std::vector>());
    guard.unlock();

    w.set_content(j.dump(), "application/json");
    spdlog::debug("Responded: students: {}", j.dump());
}

void Server::api_students_add(Request const &r, Response &w)
{
    auto const j = nlohmann::json::parse(r.body);
    spdlog::info("Student Add Request: {}", j.dump());
    auto const s = j.get<Student>();

    if (s.student_id.size() != 12) { // See definition of struct Assignment
        w.status = StatusCode::BadRequest_400;
        w.set_content("Bad assignment name length, should be 12", "text/plain");
        spdlog::warn("Bad assignment name length, ignoring");
        return;
    }
    std::unique_lock guard{lock_};
    if (students_.contains(s.student_id)) {
        auto const err = std::format("Student '{}' already exists.", s.name);
        w.status = StatusCode::BadRequest_400;
        w.set_content(err, "text/plain");
        spdlog::warn("{} Ignoring request.", err);
        return;
    }

    students_.insert({s.student_id, s});
    constexpr auto ts = schema::Student{};
    db_(sqlpp::insert_into(ts).set(ts.student_id = s.student_id,
                                   ts.name = s.name));
}
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Server::hi(Request const &_, Response &w)
{
    w.set_content("Hello World!", "text/plain");
};
