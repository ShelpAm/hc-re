#pragma once

// clang-format off
// generated schema header (made to match sqlpp23 expectations)

#include <optional>

#include <sqlpp23/core/basic/table.h>
#include <sqlpp23/core/basic/table_columns.h>
#include <sqlpp23/core/name/create_name_tag.h>
#include <sqlpp23/core/type_traits.h>

namespace schema {
  struct Teacher_ {
    struct TeacherId {
      SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(teacher_id, teacher_id);
      using data_type = ::sqlpp::text;
      using has_default = std::false_type;
    };
    struct Name {
      SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(name, name);
      using data_type = ::sqlpp::text;
      using has_default = std::false_type;
    };
    struct Password {
      SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(password, password);
      using data_type = ::sqlpp::text;
      using has_default = std::false_type;
    };
    SQLPP_CREATE_NAME_TAG_FOR_SQL_AND_CPP(teacher, teacher);
    template<typename T>
    using _table_columns = sqlpp::table_columns<T,
               TeacherId,
               Name,
               Password>;
    using _required_insert_columns = sqlpp::detail::type_set<
               sqlpp::column_t<sqlpp::table_t<Teacher_>, TeacherId>,
               sqlpp::column_t<sqlpp::table_t<Teacher_>, Name>,
               sqlpp::column_t<sqlpp::table_t<Teacher_>, Password>>;
  };
  using Teacher = ::sqlpp::table_t<Teacher_>;

} // namespace schema