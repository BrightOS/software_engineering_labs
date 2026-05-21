#include "user_handler.hpp"
#include "storage.hpp"
#include "user_utils.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>

namespace auth {

namespace {

class UserByLoginHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-users-by-login";

  UserByLoginHandler(const userver::components::ComponentConfig &config,
                     const userver::components::ComponentContext &context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<StorageComponent>()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::ContentType{"application/json"});

    const auto &login = request.GetPathArg("login");
    auto user = storage_.FindUserByLogin(login);
    if (!user) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kNotFound);
      return R"({"error":"User not found"})";
    }
    return userver::formats::json::ToString(UserToJson(*user));
  }

private:
  StorageComponent &storage_;
};

class UserSearchHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-users-search";

  UserSearchHandler(const userver::components::ComponentConfig &config,
                    const userver::components::ComponentContext &context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<StorageComponent>()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::ContentType{"application/json"});

    const auto &query = request.GetArg("query");
    if (query.empty()) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"query parameter is required"})";
    }

    auto users = storage_.SearchUsersByName(query);

    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto &user : users) {
      arr.PushBack(UserToJson(user));
    }
    return userver::formats::json::ToString(arr.ExtractValue());
  }

private:
  StorageComponent &storage_;
};

} // namespace

void AppendUserHandlers(userver::components::ComponentList &component_list) {
  component_list.Append<UserByLoginHandler>();
  component_list.Append<UserSearchHandler>();
}

} // namespace auth
