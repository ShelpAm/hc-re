#pragma once
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

    ~Server();

    void start(std::string const &host, std::uint16_t port);

    void stop();

    bool is_running() const;

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

    httplib::Server server_;
    std::unique_ptr<std::jthread>
        server_thread_; // If not nullptr, then server is running
    sqlpp::postgresql::connection db_;
    std::shared_mutex lock_;
    std::map<std::string, Student> students_;
    std::map<std::string, Assignment> assignments_;
    std::queue<std::pair<TimePoint, std::filesystem::path>> exported_tmp_files_;
};

struct ApiAssignmentsExportParam {
    std::string assignment_name;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ApiAssignmentsExportParam, assignment_name);
};
