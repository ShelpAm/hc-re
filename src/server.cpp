#include <hc/server.h>

#include <hc/archive.h>
#include <hc/config.h>
#include <hc/debug.h>
#include <hc/submit-request.h>

#include <archive.h>
#include <boost/uuid.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <fstream>
#include <print>

namespace fs = std::filesystem;

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
                                             r.end_time, {})}); // FIXME: this
            // shifts based on geometric position when fetched from DB!!!
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

    using httplib::Request;
    using httplib::Response;
    using httplib::StatusCode;
    server_.set_exception_handler(
        [](Request const &_, Response &w, std::exception_ptr ep) {
            try {
                std::rethrow_exception(std::move(ep));
            }
            catch (std::exception &e) {
                spdlog::error("Server interal error: {}", e.what());
                w.status = StatusCode::InternalServerError_500;
                w.set_content(e.what(), "text/plain");
            }
        });

    auto hi = [](Request const &_, Response &w) {
        w.set_content("Hello World!", "text/plain");
    };
    auto api_assignments = [this](Request const &_, Response &w) {
        spdlog::info("Assignment List Request");
        std::shared_lock guard{lock_};
        auto const j = nlohmann::json(assignments_ | std::views::values |
                                      std::ranges::to<std::vector>());
        guard.unlock();
        w.set_content(j.dump(), "application/json");
        spdlog::debug("Responded: assignments: {}", j.dump());
    };
    auto api_assignments_add = [this](Request const &r, Response &w) {
        auto const j = nlohmann::json::parse(r.body);
        spdlog::info("Assignment Add Request: {}", j.dump());
        auto a = j.get<Assignment>();
        spdlog::trace("Time parsed as from {} to {}", a.start_time, a.end_time);
        std::unique_lock guard{lock_};
        if (!verify_assignment_not_exists(a.name, w)) {
            return;
        }
        assignments_.insert({a.name, a});
        constexpr auto ta = schema::Assignment{};
        auto insert = sqlpp::insert_into(ta).set(ta.name = a.name,
                                                 ta.start_time = a.start_time,
                                                 ta.end_time = a.end_time);
        spdlog::critical("{}", to_string(db_, insert));
        db_(insert);
    };
    auto api_assignments_submit = [this](Request const &r, Response &w) {
        auto const j = nlohmann::json::parse(r.body);
        spdlog::info("Assignment Submit Request: {}", j.dump());

        try {
            auto sr = j.get<SubmitRequest>();

            std::unique_lock guard{lock_};
            if (!verify_student_exists(sr.student_id, sr.student_name, w)) {
                return;
            }

            if (!verify_assignment_exists(sr.assignment_name, w)) {
                return;
            }

            auto &a = assignments_.at(sr.assignment_name);

            constexpr auto ts = schema::Submission{};
            if (a.submissions.contains(sr.student_id)) {
                fs::remove(a.submissions.at(sr.student_id).filepath);
                db_(sqlpp::delete_from(ts).where(
                    ts.assignment_name == sr.assignment_name &&
                    ts.student_id == sr.student_id));
            }

            boost::uuids::random_generator gen;
            auto const uuid = gen();
            auto const filename = boost::uuids::to_string(uuid);
            auto const filedir = config::datahome() / "files";
            fs::create_directories(filedir);
            auto const filepath = filedir / filename;

            using base64 = cppcodec::base64_rfc4648;
            auto file = base64::decode(sr.file.content);

            std::ofstream ofs(filepath, std::ios::binary);
            // NOLINTNEXTLINE
            ofs.write(reinterpret_cast<char const *>(file.data()), file.size());

            auto const s = Submission{
                .assignment_name{sr.assignment_name},
                .student_id{sr.student_id},
                .submission_time{TimePoint{UtcClock::now().time_since_epoch()}},
                .filepath{filepath},
                .original_filename{sr.file.filename},
            };

            a.submissions[sr.student_id] = s;

            db_(sqlpp::insert_into(ts).set(
                ts.student_id = s.student_id,
                ts.submission_time = s.submission_time,
                ts.assignment_name = s.assignment_name,
                ts.original_filename = s.original_filename,
                ts.filepath = s.filepath.string()));
        }
        catch (nlohmann::json::parse_error const &e) {
            w.status = StatusCode::InternalServerError_500;
            w.set_content("Bad request format", "text/plain");
        }
        catch (...) {
            throw;
        }
    };
    auto api_assignments_export = [this](Request const &r, Response &w) {
        throw std::runtime_error{"Unimplemented"}; // TODO
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
        auto const tmpdir = fs::temp_directory_path() / "hc";
        auto const adir = tmpdir / a.name;
        fs::create_directories(adir);
        for (auto const &[_, sub] : a.submissions) {
            auto const &stu = students_.at(sub.student_id);
            auto const studir = adir / (stu.student_id + stu.name);
            fs::create_directory(studir);
            fs::copy_file(sub.filepath, studir / sub.original_filename);
        }
        boost::uuids::random_generator gen;
        auto const uuid = gen();
        auto const filepath =
            tmpdir / boost::uuids::to_string(uuid) / ".tar.zst";
        std::string err;
        if (!hc::archive::create_tar_zst(filepath, {adir}, err)) {
            throw std::runtime_error{"Failed to make archive: " + err};
        }
        w.set_file_content(filepath.string(),
                           "application/x-zstd-compressed-tar");
    };
    auto api_students = [this](Request const &_, Response &w) {
        spdlog::info("Student List Request");

        std::shared_lock guard{lock_};
        auto const j = nlohmann::json(students_ | std::views::values |
                                      std::ranges::to<std::vector>());
        guard.unlock();

        w.set_content(j.dump(), "application/json");
        spdlog::debug("Responded: students: {}", j.dump());
    };
    auto api_students_add = [this](Request const &r, Response &w) {
        auto const j = nlohmann::json::parse(r.body);
        spdlog::info("Student Add Request: {}", j.dump());
        auto const s = j.get<Student>();

        if (s.student_id.size() != 12) { // See definition of struct Assignment
            w.status = StatusCode::BadRequest_400;
            w.set_content("Bad assignment name length, should be 12",
                          "text/plain");
            spdlog::warn("Bad assignment name length, ignoring");
            return;
        }
        std::unique_lock guard{lock_};
        if (students_.contains(s.student_id)) {
            auto const err =
                std::format("Student '{}' already exists.", s.name);
            w.status = StatusCode::BadRequest_400;
            w.set_content(err, "text/plain");
            spdlog::warn("{} Ignoring request.", err);
            return;
        }

        students_.insert({s.student_id, s});
        constexpr auto ts = schema::Student{};
        db_(sqlpp::insert_into(ts).set(ts.student_id = s.student_id,
                                       ts.name = s.name));
    };

    server_.Get("/hi", hi);
    server_.Get("/api/assignments", api_assignments);
    server_.Post("/api/assignments/add", api_assignments_add);
    server_.Post("/api/assignments/submit", api_assignments_submit);
    server_.Get("/api/students", api_students);
    server_.Post("/api/students/add", api_students_add);
    server_.Post("/api/assignments/export", api_assignments_export);
}

Server::~Server()
{
    // Avoids server_thread_ blocking.
    if (server_.is_running()) {
        server_.stop();
    }
}

void Server::start(std::string const &host, std::uint16_t port)
{
    if (server_thread_) {
        throw std::runtime_error{"Server already started"};
    }

    server_thread_ = std::make_unique<std::jthread>([this, &host, port]() {
        spdlog::info("Server started. Binding to {}:{}", host, port);
        if (!server_.bind_to_port(host, port)) {
            throw std::runtime_error{"Address already in use"};
        }
        server_.listen_after_bind();
        spdlog::info("Server stopped.");
    });
    // Waits for server started
    while (!server_.is_running()) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
    }
}

void Server::stop()
{
    if (!server_thread_) {
        throw std::runtime_error{"Server not running"};
    }

    server_.stop();
    server_thread_.reset();
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
