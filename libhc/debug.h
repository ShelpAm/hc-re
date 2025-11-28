#pragma once
#include <sqlpp23/postgresql/postgresql.h>
#include <string>

inline std::string to_string(auto &&db, auto &&expr)
{
    using sqlpp::to_sql_string;
    return to_sql_string(db, expr);
}
