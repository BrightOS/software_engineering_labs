#pragma once

#include <optional>
#include <string>

#include <jwt-cpp/jwt.h>

namespace hotel_booking {

inline std::string ExtractBearerToken(const std::string &auth_header) {
  if (auth_header.size() > 7 && auth_header.substr(0, 7) == "Bearer ") {
    return auth_header.substr(7);
  }
  return {};
}

inline std::optional<std::string> VerifyJwt(const std::string &token,
                                            const std::string &secret) {
  try {
    auto decoded = jwt::decode(token);
    jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{secret})
        .with_issuer("hotel-booking")
        .verify(decoded);
    return decoded.get_subject();
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

} // namespace hotel_booking
