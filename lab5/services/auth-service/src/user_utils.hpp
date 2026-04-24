#pragma once

#include "storage.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace auth {

inline userver::formats::json::Value UserToJson(const User &user) {
  userver::formats::json::ValueBuilder b;
  b["id"] = user.id;
  b["login"] = user.login;
  b["first_name"] = user.first_name;
  b["last_name"] = user.last_name;
  b["email"] = user.email;
  return b.ExtractValue();
}

} // namespace auth
