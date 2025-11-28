#pragma once
#include "assignment.h"
#include "schema/Assignment.h"
#include "schema/Student.h"
#include "schema/Submission.h"
#include "student.h"
#include "submission.h"
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <sqlpp23/postgresql/postgresql.h>

// StudentID -> Student
std::unordered_map<std::string, Student>
load_students(sqlpp::postgresql::connection &db);

// AssignmentName -> Assignment
std::unordered_map<std::string, Assignment>
load_assignments(sqlpp::postgresql::connection &db);

class Server {
  public:
    Server(Server const &) = delete;
    Server(Server &&) = delete;
    Server &operator=(Server const &) = delete;
    Server &operator=(Server &&) = delete;
    Server(sqlpp::postgresql::connection_config const &config);

    ~Server();

    void start(std::string const &host, std::uint16_t port);

    void stop();

  private:
    httplib::Server server_;
    std::unique_ptr<std::jthread>
        server_thread_; // If not nullptr, then server is running
    sqlpp::postgresql::connection db_;
    std::unordered_map<std::string, Student> students_;
    std::unordered_map<std::string, Assignment> assignments_;
};
