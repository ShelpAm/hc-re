#pragma once
#include <functional>
#include <hc/assignment.h>
#include <hc/schema/Assignment.h>
#include <hc/schema/Student.h>
#include <hc/schema/Submission.h>
#include <hc/student.h>
#include <hc/submission.h>
#include <httplib.h>
#include <map>
#include <queue>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <sqlpp23/postgresql/postgresql.h>

// StudentID -> Student
std::map<std::string, Student> load_students(sqlpp::postgresql::connection &db);

// AssignmentName -> Assignment
std::map<std::string, Assignment>
load_assignments(sqlpp::postgresql::connection &db);

class Server {
  public:
    Server(Server const &) = delete;
    Server(Server &&) = delete;
    Server &operator=(Server const &) = delete;
    Server &operator=(Server &&) = delete;

    Server(sqlpp::postgresql::connection &&db);

    ~Server() noexcept;

    /// @brief  Synchronously start the http server. i.e. it blocks until the
    /// server actually starts.
    void start(std::string const &host, std::uint16_t port);

    /// @brief  Synchronously stops the http server. i.e. it blocks until the
    /// server actually stops.
    ///
    /// DON'T call `stop()` inside any http handler, or this will form a dead
    /// lock.
    void stop() noexcept;

    bool is_running() const noexcept;

    void wait_until_started() noexcept;
    void wait_until_stopped() noexcept;

  private:
    bool verify_assignment_not_exists(std::string_view assignment_name,
                                      httplib::Response &w) noexcept;
    bool verify_assignment_exists(std::string_view assignment_name,
                                  httplib::Response &w) noexcept;
    bool verify_student_exists(std::string_view student_id,
                               std::string_view student_name,
                               httplib::Response &w) noexcept;
    bool verify_student_not_exists(std::string_view student_id,
                                   httplib::Response &w) noexcept;

    using Handler = void (Server::*)(httplib::Request const &,
                                     httplib::Response &);
    void get(std::string const &path, Handler h);
    void post(std::string const &path, Handler h);

    void hi(httplib::Request const &_, httplib::Response &w);

    void api_admin_login(httplib::Request const &r, httplib::Response &w);
    void api_admin_verify_token(httplib::Request const &r,
                                httplib::Response &w);
    void api_assignments(httplib::Request const &, httplib::Response &);
    void api_assignments_add(httplib::Request const &, httplib::Response &);
    void api_assignments_submit(httplib::Request const &, httplib::Response &);
    void api_assignments_export(httplib::Request const &, httplib::Response &);

    void api_students(httplib::Request const &, httplib::Response &);
    void api_students_add(httplib::Request const &, httplib::Response &);
    void api_stop(httplib::Request const &, httplib::Response &);

    // Clean files
    static void clean_all_files();
    void clean_expired_files();

    httplib::Server http_server_;

    // If has_value, then server is running
    std::optional<std::jthread> server_thread_;

    sqlpp::postgresql::connection db_;
    std::shared_mutex lock_; // For data
    std::mutex http_lock_;   // For http server
    std::map<std::string, Student> students_;
    std::map<std::string, Assignment> assignments_;
    std::queue<std::pair<TimePoint, std::filesystem::path>> tmp_files_;
    std::set<std::string> tokens_;
};

struct ApiAssignmentsExportParam {
    std::string assignment_name;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ApiAssignmentsExportParam, assignment_name);
};
