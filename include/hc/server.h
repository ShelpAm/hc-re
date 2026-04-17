#pragma once
#include <hc/assignment.h>
#include <hc/schema/Assignment.h>
#include <hc/schema/Student.h>
#include <hc/schema/Submission.h>
#include <hc/schema/Teacher.h> 
#include <hc/student.h>
#include <hc/submission.h>
#include <hc/teacher.h>
#include <httplib.h>
#include <map>
#include <queue>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <sqlpp23/postgresql/postgresql.h>
#include <optional>

// StudentID -> Student
std::map<std::string, Student> load_students(sqlpp::postgresql::connection &db);

// AssignmentName -> Assignment
std::map<std::string, Assignment> load_assignments(sqlpp::postgresql::connection &db);

// TeacherID -> Teacher
std::map<std::string, Teacher> load_teachers(sqlpp::postgresql::connection &db);

class Server {
    using DatabaseConnection = sqlpp::postgresql::connection;

  public:
    Server(Server const &) = delete;
    Server(Server &&) = delete;
    Server &operator=(Server const &) = delete;
    Server &operator=(Server &&) = delete;

    Server(DatabaseConnection &&db);

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

    /// @brief Creates a blob publicly accessible in `server/api/blob/`.
    void api_assignments_export(httplib::Request const &, httplib::Response &);

    void api_students(httplib::Request const &, httplib::Response &);
    void api_students_add(httplib::Request const &, httplib::Response &);
    void api_stop(httplib::Request const &, httplib::Response &);

    // Teacher APIs
    void api_teacher_login(httplib::Request const &r, httplib::Response &w);
    void api_teacher_add(httplib::Request const &r, httplib::Response &w);
    void api_teacher_verify_token(httplib::Request const &r, httplib::Response &w);

    // 认证：返回 token 对应的主体（"admin" 或 teacher_id），失败返回 std::nullopt
    std::optional<std::string> authenticate_request(httplib::Request const &req,
                                                    httplib::Response &w) noexcept;

    // Clean files
    static void clean_all_files();
    void clean_expired_files();

    httplib::Server http_server_;

    // If has_value, then server is running
    std::optional<std::jthread> server_thread_;

    DatabaseConnection db_;
    std::shared_mutex lock_; // For data
    std::mutex http_lock_;   // For http server
    std::map<std::string, Student> students_;
    std::map<std::string, Assignment> assignments_;
    std::map<std::string, Teacher> teachers_;
    std::queue<std::pair<TimePoint, std::filesystem::path>> tmp_files_;

    // token -> principal ("admin" or teacher_id)
    std::map<std::string, std::string> tokens_;
};

struct ApiAssignmentsExportParam {
    std::string assignment_name;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ApiAssignmentsExportParam, assignment_name);
};

struct AssignmentsExportResult {
    std::string exported_uri;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssignmentsExportResult, exported_uri);
};

// Teacher Login DTOs
struct TeacherLoginParams {
    std::string teacher_id;
    std::string password;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TeacherLoginParams, teacher_id, password);
};

struct TeacherLoginResult {
    std::string token;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TeacherLoginResult, token);
};

struct TeacherVerifyTokenParams {
    std::string token;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TeacherVerifyTokenParams, token);
};

struct TeacherVerifyTokenResult {
    bool ok;
    std::string principal; // "admin" or teacher_id
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TeacherVerifyTokenResult, ok, principal);
};
