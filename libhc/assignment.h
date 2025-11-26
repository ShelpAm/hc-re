#pragma once
#include "submission.h"
#include "time-defs.h"
#include <nlohmann/json.hpp>
#include <string>

struct Assignment {
  public:
    std::string name;
    TimePoint start_time;
    TimePoint end_time;
    std::vector<Submission> submissions;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Assignment, name
                                   // , start_time_, end_time_
                                   // , submissions_
    );
};
