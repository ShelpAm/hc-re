#include <boost/uuid.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include <fstream>
#include <libhc/config.h>
#include <libhc/debug.h>
#include <libhc/server.h>
#include <libhc/submit-request.h>
#include <print>

std::unordered_map<std::string, Student>
load_students(sqlpp::postgresql::connection &db)
{
    constexpr auto s = schema::Student{};
    auto res = db(sqlpp::select(s.student_id, s.name).from(s));
    std::unordered_map<std::string, Student> students;
    students.reserve(static_cast<std::size_t>(res.size()));
    for (auto const &r : res) {
        students.insert({std::string{r.student_id},
                         Student{.student_id{r.student_id}, .name{r.name}}});
    }
    return students;
}

std::unordered_map<std::string, Assignment>
load_assignments(sqlpp::postgresql::connection &db)
{
    constexpr auto a = schema::Assignment{};
    constexpr auto s = schema::Submission{};
    auto res =
        db(sqlpp::select(a.name, a.start_time, a.end_time, s.student_id,
                         s.submission_time, s.filepath, s.original_filename)
               .from(a.left_outer_join(s).on(a.name == s.assignment_name)));

    std::unordered_map<std::string, Assignment> assignments;

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

Server::Server(sqlpp::postgresql::connection_config const &config) : db_(config)
{
    if (!db_.is_connected()) {
        throw std::runtime_error{"Failed to connect to server"};
    }

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

    server_.set_exception_handler([](httplib::Request const &_,
                                     httplib::Response &w,
                                     std::exception_ptr ep) {
        try {
            std::rethrow_exception(std::move(ep));
        }
        catch (std::exception &e) {
            spdlog::error("Server interal error: {}", e.what());
            w.status = httplib::StatusCode::InternalServerError_500;
            w.set_content(e.what(), "text/plain");
        }
    });

    server_.Get("/hi", [](httplib::Request const &_, httplib::Response &w) {
        w.set_content("Hello World!", "text/plain");
    });

    server_.Get("/api/assignments", [this](httplib::Request const &_,
                                           httplib::Response &w) {
        spdlog::info("Assignment List Request");
        auto const j = nlohmann::json(assignments_ | std::views::values |
                                      std::ranges::to<std::vector>());
        w.set_content(j.dump(), "application/json");
        spdlog::debug("Responded: assignments: {}", j.dump());
    });

    server_.Post("/api/assignments/add", [this](httplib::Request const &r,
                                                httplib::Response &w) {
        auto const j = nlohmann::json::parse(r.body);
        spdlog::info("Assignment Add Request: {}", j.dump());
        auto a = j.get<Assignment>();
        spdlog::debug("Time parsed as from {} to {}", a.start_time, a.end_time);
        if (assignments_.contains(a.name)) {
            auto const err = std::format(
                "Assignment '{}' already exists. Ignoring request.", a.name);
            w.status = httplib::StatusCode::BadRequest_400;
            w.set_content(err, "text/plain");
            spdlog::error(err);
            return;
        }
        assignments_.insert({a.name, a});
        constexpr auto ta = schema::Assignment{};
        auto insert = sqlpp::insert_into(ta).set(ta.name = a.name,
                                                 ta.start_time = a.start_time,
                                                 ta.end_time = a.end_time);
        spdlog::debug("Running {}", to_string(db_, insert));
        db_(insert);
    });

    server_.Post("/api/assignments/submit", [this](httplib::Request const &r,
                                                   httplib::Response &w) {
        auto const j = nlohmann::json::parse(r.body);
        spdlog::info("Assignment Submit Request: {}", j.dump());

        try {
            auto sr = j.get<SubmitRequest>();

            if (!students_.contains(sr.student_id) ||
                students_.at(sr.student_id).name != sr.student_name) {
                auto const err = std::format("Student '{} {}' doesn't exist.",
                                             sr.student_id, sr.student_name);
                w.status = httplib::StatusCode::BadRequest_400;
                w.set_content(err, "text/plain");
                spdlog::error("{} Ignoring request.", err);
                return;
            }

            if (!assignments_.contains(sr.assignment_name)) {
                auto const err = std::format(
                    "Assignment '{}' doesn't exist. Ignoring request.",
                    sr.assignment_name);
                w.status = httplib::StatusCode::BadRequest_400;
                w.set_content(err, "text/plain");
                spdlog::error(err);
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
            w.status = httplib::StatusCode::InternalServerError_500;
            w.set_content("Bad request format", "text/plain");
        }
        catch (...) {
            throw;
        }
    });

    server_.Get("/api/students", [this](httplib::Request const &_,
                                        httplib::Response &w) {
        spdlog::info("Student List Request");

        // 转换学生数据为 JSON 格式
        auto const j = nlohmann::json(students_ | std::views::values |
                                      std::ranges::to<std::vector>());

        // 返回 JSON 数据
        w.set_content(j.dump(), "application/json");
        spdlog::debug("Responded: students: {}", j.dump());
    });

    server_.Post("/api/students/add", [this](httplib::Request const &r,
                                             httplib::Response &w) {
        auto const j = nlohmann::json::parse(r.body);
        spdlog::info("Student Add Request: {}", j.dump());
        auto const s = j.get<Student>();

        if (students_.contains(s.student_id)) {
            w.status = httplib::StatusCode::BadRequest_400;
            spdlog::error("Student '{}' already exists. Ignoring request.",
                          s.name);
            return;
        }

        students_.insert({s.student_id, s});
        constexpr auto ts = schema::Student{};
        db_(sqlpp::insert_into(ts).set(ts.student_id = s.student_id,
                                       ts.name = s.name));
    });
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
        server_.bind_to_port(host, port);
        server_.listen_after_bind();
        spdlog::info("Server stopped.");
    });
    // Waits for server started
    while (!server_.is_running()) {
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
