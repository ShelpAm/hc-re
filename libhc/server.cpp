#include "server.h"

#include <utility>

std::unordered_map<std::string, Student>
load_students(sqlpp::postgresql::connection &db)
{
    constexpr auto s = schema::Student{};
    auto res = db(sqlpp::select(s.student_id, s.name).from(s));
    std::unordered_map<std::string, Student> students;
    students.reserve(static_cast<std::size_t>(res.size()));
    for (auto const &r : res) {
        students.insert(
            {std::string{r.student_id},
             Student(std::string{r.student_id}, std::string{r.name})});
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
                .submissions.push_back(Submission(
                    std::string{r.student_id.value()},
                    r.submission_time.value(), std::string{r.filepath.value()},
                    std::string{r.original_filename.value()}));
        }
    }

    return assignments;
}

template <typename Result>
static Result parse_time(std::string const &fmt, std::string str)
{
    auto is = std::istringstream{std::move(str)};
    is.imbue(std::locale("en_US.utf-8"));
    Result res;
    is >> std::chrono::parse(fmt, res);
    if (is.fail()) {
        throw std::runtime_error{"Failed to parse time"};
    }
    return res;
};

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
        auto const startime = std::format("{:%Y-%m-%d %H:%M:%S}", v.start_time);
        auto const endtime = std::format("{:%Y-%m-%d %H:%M:%S}", v.end_time);
        spdlog::debug("assignment=> name: {}, start_time: {}, end_time: {}",
                      v.name, startime, endtime);
    }

    server_.set_exception_handler([](httplib::Request const &_,
                                     httplib::Response &w,
                                     std::exception_ptr ep) {
        try {
            std::rethrow_exception(std::move(ep));
        }
        catch (std::exception &e) {
            spdlog::warn("Server interal error: {}", e.what());
        }
        w.status = httplib::StatusCode::InternalServerError_500;
    });

    server_.Get("/hi", [](httplib::Request const &_, httplib::Response &w) {
        w.set_content("Hello World!", "text/plain");
    });

    server_.Get("/api/assignments", [this](httplib::Request const &_,
                                           httplib::Response &w) {
        auto const j = nlohmann::json(assignments_ | std::views::values |
                                      std::ranges::to<std::vector>());
        w.set_content(j.dump(), "application/json");
        spdlog::debug("Responded: assignments: {}", j.dump());
    });

    server_.Post("/api/assignments/add", [this](httplib::Request const &r,
                                                httplib::Response &w) {
        auto const j = nlohmann::json::parse(r.body);
        spdlog::info("Request: {}", r.body);
        auto const &s = j.at("start_time_").get_ref<std::string const &>();
        auto const res = parse_time<TimePoint>("%Y-%m-%d", s);
        auto a = j.get<Assignment>();
        spdlog::debug("Received: res: {}, assignments: {}",
                      res.time_since_epoch().count(), r.body);
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
        server_.listen(host, port);
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
