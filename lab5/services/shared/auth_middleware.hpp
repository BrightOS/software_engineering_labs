#pragma once

#include "jwt_utils.hpp"

#include <optional>

#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

namespace hotel_booking {

inline const std::string kJwtSecret = "hotel-booking-secret-key-lab2";
static constexpr std::string_view kUserIdKey = "user_id";

inline const std::string &
RequireAuth(const userver::server::http::HttpRequest &request,
            userver::server::request::RequestContext &context) {
  const auto &auth = request.GetHeader("Authorization");
  std::optional<std::string> user_id;
  if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ") {
    try {
      user_id = VerifyJwt(auth.substr(7), kJwtSecret);
    } catch (...) {
    }
  }
  if (!user_id) {
    throw userver::server::handlers::ExceptionWithCode<
        userver::server::handlers::HandlerErrorCode::kUnauthorized>(
        userver::server::handlers::InternalMessage{"Authentication required"});
  }
  context.SetData(std::string{kUserIdKey}, std::move(*user_id));
  return context.GetData<std::string>(kUserIdKey);
}

} // namespace hotel_booking
