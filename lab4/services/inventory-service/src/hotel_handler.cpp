#include "hotel_handler.hpp"
#include "auth_middleware.hpp"
#include "storage.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace inventory {

namespace {

userver::formats::json::Value HotelToJson(const Hotel &hotel) {
  userver::formats::json::ValueBuilder b;
  b["id"] = hotel.id;
  b["name"] = hotel.name;
  b["city"] = hotel.city;
  b["address"] = hotel.address;
  b["description"] = hotel.description;
  b["stars"] = hotel.stars;
  return b.ExtractValue();
}

class HotelHandler final : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-hotels";

  HotelHandler(const userver::components::ComponentConfig &config,
               const userver::components::ComponentContext &context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<StorageComponent>()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &context) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::ContentType{"application/json"});

    if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
      return HandleGet(request);
    }
    return HandlePost(request, context);
  }

private:
  std::string
  HandleGet(const userver::server::http::HttpRequest &request) const {
    const auto &city = request.GetArg("city");
    auto hotels = storage_.GetHotels(city);

    userver::formats::json::ValueBuilder arr(
        userver::formats::json::Type::kArray);
    for (const auto &hotel : hotels) {
      arr.PushBack(HotelToJson(hotel));
    }
    return userver::formats::json::ToString(arr.ExtractValue());
  }

  std::string
  HandlePost(const userver::server::http::HttpRequest &request,
             userver::server::request::RequestContext &context) const {
    hotel_booking::RequireAuth(request, context);

    userver::formats::json::Value body;
    try {
      body = userver::formats::json::FromString(request.RequestBody());
    } catch (const std::exception &) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Invalid JSON"})";
    }

    auto name = body["name"].As<std::string>("");
    auto city = body["city"].As<std::string>("");
    auto address = body["address"].As<std::string>("");
    auto description = body["description"].As<std::string>("");
    auto stars = body["stars"].As<int>(0);

    if (name.empty() || city.empty() || address.empty()) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"name, city and address are required"})";
    }

    auto hotel = storage_.AddHotel(name, city, address, description, stars);
    request.GetHttpResponse().SetStatus(
        userver::server::http::HttpStatus::kCreated);
    return userver::formats::json::ToString(HotelToJson(hotel));
  }

  StorageComponent &storage_;
};

class HotelInternalHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-internal-hotels";

  HotelInternalHandler(const userver::components::ComponentConfig &config,
                       const userver::components::ComponentContext &context)
      : HttpHandlerBase(config, context),
        storage_(context.FindComponent<StorageComponent>()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::ContentType{"application/json"});

    const auto &hotel_id = request.GetPathArg("id");
    auto hotel = storage_.FindHotel(hotel_id);
    if (!hotel) {
      request.GetHttpResponse().SetStatus(
          userver::server::http::HttpStatus::kNotFound);
      return R"({"error":"Hotel not found"})";
    }
    return userver::formats::json::ToString(HotelToJson(*hotel));
  }

private:
  StorageComponent &storage_;
};

} // namespace

void AppendHotelHandlers(userver::components::ComponentList &component_list) {
  component_list.Append<HotelHandler>();
  component_list.Append<HotelInternalHandler>();
}

} // namespace inventory
