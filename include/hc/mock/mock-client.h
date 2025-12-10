#pragma once
#include <httplib.h>

namespace hc::mock {

void successfully_hi(httplib::Client &client);

void hi_unreachable(httplib::Client &client);

void successfully_add_assignment_testassignmentinfinite(
    httplib::Client &client);

void successfully_add_student_ljf(httplib::Client &client);

void ljf_successfully_submit_to_testassignmentinfinite(httplib::Client &client);

} // namespace hc::mock
