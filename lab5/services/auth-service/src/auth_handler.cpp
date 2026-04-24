#include "auth_handler.hpp"
#include "storage.hpp"
#include "user_utils.hpp"

#include <chrono>

#include <jwt-cpp/jwt.h>

#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace auth {

namespace {

class AuthRegisterHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-auth-register";

  AuthRegisterHandler(const userver::components::ComponentConfig &config,
                      const userver::components::ComponentContext &context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<StorageComponent>()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::ContentType{"application/json"});

    userver::formats::json::Value body;
    try {
      body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception &) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Invalid JSON"})";
    }

    auto login = body["login"].As<std::string>("");
    auto password = body["password"].As<std::string>("");
    auto first_name = body["first_name"].As<std::string>("");
    auto last_name = body["last_name"].As<std::string>("");
    auto email = body["email"].As<std::string>("");

    if (login.empty() || password.empty() || first_name.empty() ||
        last_name.empty()) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"login, password, first_name and last_name are required"})";
    }

    auto user = storage_.AddUser(login, password, first_name, last_name, email);
    if (!user) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kConflict);
      return R"({"error":"Login already taken"})";
    }

    request.GetHttpResponse().SetStatus(
        userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(UserToJson(*user));
  }

private:
  StorageComponent &storage_;
};

class AuthLoginHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-auth-login";

  AuthLoginHandler(const userver::components::ComponentConfig &config,
                   const userver::components::ComponentContext &context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<StorageComponent>()),
        jwt_secret_(config["jwt-secret"].As<std::string>()) {}

  static userver::yaml_config::Schema GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: AuthLoginHandler config
additionalProperties: false
properties:
    jwt-secret:
        type: string
        description: JWT signing secret
)");
  }

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::ContentType{"application/json"});

    userver::formats::json::Value body;
    try {
      body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception &) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Invalid JSON"})";
    }

    auto login = body["login"].As<std::string>("");
    auto password = body["password"].As<std::string>("");

    if (login.empty() || password.empty()) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"login and password are required"})";
    }

    auto user_id = storage_.ValidateCredentials(login, password);
    if (!user_id) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      return R"({"error":"Invalid login or password"})";
    }

    const auto now = std::chrono::system_clock::now();
    auto token = jwt::create()
                     .set_issuer("hotel-booking")
                     .set_subject(*user_id)
                     .set_issued_at(now)
                     .set_expires_at(now + std::chrono::hours{24})
                     .sign(jwt::algorithm::hs256{jwt_secret_});

    auto refresh_token = storage_.StoreRefreshToken(*user_id);

    userver::formats::json::ValueBuilder resp;
    resp["token"] = token;
    resp["refresh_token"] = refresh_token;
    resp["user_id"] = *user_id;
    return userver::formats::json::ToString(resp.ExtractValue());
  }

private:
  StorageComponent &storage_;
  std::string jwt_secret_;
};

class AuthRefreshHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-auth-refresh";

  AuthRefreshHandler(const userver::components::ComponentConfig &config,
                     const userver::components::ComponentContext &context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<StorageComponent>()),
        jwt_secret_(config["jwt-secret"].As<std::string>()) {}

  static userver::yaml_config::Schema GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: AuthRefreshHandler config
additionalProperties: false
properties:
    jwt-secret:
        type: string
        description: JWT signing secret
)");
  }

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::ContentType{"application/json"});

    userver::formats::json::Value body;
    try {
      body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception &) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Invalid JSON"})";
    }

    auto refresh_token = body["refresh_token"].As<std::string>("");
    if (refresh_token.empty()) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"refresh_token is required"})";
    }

    auto user_id = storage_.ValidateRefreshToken(refresh_token);
    if (!user_id) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      return R"({"error":"Invalid or expired refresh token"})";
    }

    storage_.RevokeRefreshToken(refresh_token);
    auto new_refresh_token = storage_.StoreRefreshToken(*user_id);

    const auto now = std::chrono::system_clock::now();
    auto token = jwt::create()
                     .set_issuer("hotel-booking")
                     .set_subject(*user_id)
                     .set_issued_at(now)
                     .set_expires_at(now + std::chrono::hours{24})
                     .sign(jwt::algorithm::hs256{jwt_secret_});

    userver::formats::json::ValueBuilder resp;
    resp["token"] = token;
    resp["refresh_token"] = new_refresh_token;
    resp["user_id"] = *user_id;
    return userver::formats::json::ToString(resp.ExtractValue());
  }

private:
  StorageComponent &storage_;
  std::string jwt_secret_;
};

class AuthLogoutHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-auth-logout";

  AuthLogoutHandler(const userver::components::ComponentConfig &config,
                    const userver::components::ComponentContext &context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<StorageComponent>()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::ContentType{"application/json"});

    userver::formats::json::Value body;
    try {
      body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception &) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Invalid JSON"})";
    }

    auto refresh_token = body["refresh_token"].As<std::string>("");
    if (refresh_token.empty()) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"refresh_token is required"})";
    }

    if (!storage_.RevokeRefreshToken(refresh_token)) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kNotFound);
      return R"({"error":"Refresh token not found"})";
    }

    return R"({"message":"Logged out successfully"})";
  }

private:
  StorageComponent &storage_;
};

} // namespace

void AppendAuthHandlers(userver::components::ComponentList &component_list) {
  component_list.Append<AuthRegisterHandler>();
  component_list.Append<AuthLoginHandler>();
  component_list.Append<AuthRefreshHandler>();
  component_list.Append<AuthLogoutHandler>();
}

} // namespace auth
