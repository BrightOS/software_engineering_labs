#include "booking_handler.hpp"
#include "auth_middleware.hpp"
#include "rate_limiter.hpp"
#include "storage.hpp"

#include <chrono>
#include <regex>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace booking {

namespace {

bool IsValidUuid(const std::string& s) {
    static const std::regex kUuidRe(
        R"([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})");
    return std::regex_match(s, kUuidRe);
}

bool IsValidDate(const std::string& s) {
    static const std::regex kDateRe(R"(\d{4}-\d{2}-\d{2})");
    if (!std::regex_match(s, kDateRe))
        return false;
    int y = std::stoi(s.substr(0, 4));
    int m = std::stoi(s.substr(5, 2));
    int d = std::stoi(s.substr(8, 2));
    if (m < 1 || m > 12 || d < 1)
        return false;
    const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30,
                                  31};
    int max_day = days_in_month[m - 1];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        max_day = 29;
    return d <= max_day;
}

userver::formats::json::Value BookingToJson(const Booking& booking) {
    userver::formats::json::ValueBuilder b;
    b["id"] = booking.id;
    b["user_id"] = booking.user_id;
    b["hotel_id"] = booking.hotel_id;
    b["check_in"] = booking.check_in;
    b["check_out"] = booking.check_out;
    b["status"] = booking.status;
    return b.ExtractValue();
}

bool HotelExists(userver::clients::http::Client& http_client,
                 const std::string& hotel_id) {
    try {
        auto response =
            http_client.CreateRequest()
                .get("http://inventory-service:8082/api/internal/hotels/" +
                     hotel_id)
                .timeout(std::chrono::seconds{5})
                .perform();
        return response->status_code() == 200;
    } catch (const std::exception&) {
    }
    return false;
}

class BookingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-bookings";

    BookingHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          storage_(context.FindComponent<StorageComponent>()),
          http_client_(
              context.FindComponent<userver::components::HttpClient>()
                  .GetHttpClient()),
          rate_limiter_(context.FindComponent<RateLimiterComponent>()) {}

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override {
        request.GetHttpResponse().SetContentType(
            userver::http::ContentType{"application/json"});

        const auto& user_id = hotel_booking::RequireAuth(request, context);

        if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
            return HandleGet(request, user_id);
        }
        return HandlePost(request, user_id);
    }

private:
    std::string HandleGet(const userver::server::http::HttpRequest& request,
                          const std::string& caller_id) const {
        const auto& query_user_id = request.GetArg("userId");
        const auto& target_id =
            query_user_id.empty() ? caller_id : query_user_id;

        auto bookings = storage_.GetUserBookings(target_id);

        userver::formats::json::ValueBuilder arr(
            userver::formats::json::Type::kArray);
        for (const auto& b : bookings) {
            arr.PushBack(BookingToJson(b));
        }
        return userver::formats::json::ToString(arr.ExtractValue());
    }

    std::string HandlePost(const userver::server::http::HttpRequest& request,
                           const std::string& user_id) const {
        auto& response = request.GetHttpResponse();

        auto rl = rate_limiter_.CheckLimit(user_id);
        response.SetHeader(std::string_view{"X-RateLimit-Limit"}, std::to_string(rl.limit));
        response.SetHeader(std::string_view{"X-RateLimit-Remaining"}, std::to_string(rl.remaining));
        response.SetHeader(std::string_view{"X-RateLimit-Reset"}, std::to_string(rl.reset_seconds));

        if (!rl.allowed) {
            response.SetStatus(
                userver::server::http::HttpStatus::kTooManyRequests);
            return R"({"error":"Too many requests. Please try again later."})";
        }

        userver::formats::json::Value body;
        try {
            body = userver::formats::json::FromString(request.RequestBody());
        } catch (const std::exception&) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"Invalid JSON"})";
        }

        auto hotel_id = body["hotel_id"].As<std::string>("");
        auto check_in = body["check_in"].As<std::string>("");
        auto check_out = body["check_out"].As<std::string>("");

        if (hotel_id.empty() || check_in.empty() || check_out.empty()) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"hotel_id, check_in and check_out are required"})";
        }

        if (!IsValidUuid(hotel_id)) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"hotel_id must be a valid UUID"})";
        }

        if (!IsValidDate(check_in) || !IsValidDate(check_out)) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"check_in and check_out must be valid dates in YYYY-MM-DD format"})";
        }

        if (check_out <= check_in) {
            response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"check_out must be after check_in"})";
        }

        if (!HotelExists(http_client_, hotel_id)) {
            response.SetStatus(userver::server::http::HttpStatus::kNotFound);
            return R"({"error":"Hotel not found"})";
        }

        auto b = storage_.AddBooking(user_id, hotel_id, check_in, check_out);
        response.SetStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(BookingToJson(b));
    }

    StorageComponent& storage_;
    userver::clients::http::Client& http_client_;
    RateLimiterComponent& rate_limiter_;
};

class BookingCancelHandler final
    : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-bookings-cancel";

    BookingCancelHandler(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          storage_(context.FindComponent<StorageComponent>()) {}

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override {
        request.GetHttpResponse().SetContentType(
            userver::http::ContentType{"application/json"});

        const auto& user_id = hotel_booking::RequireAuth(request, context);

        const auto& booking_id = request.GetPathArg("id");
        if (!IsValidUuid(booking_id)) {
            request.GetHttpResponse().SetStatus(
                userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"booking id must be a valid UUID"})";
        }
        auto booking = storage_.FindBooking(booking_id);
        if (!booking) {
            request.GetHttpResponse().SetStatus(
                userver::server::http::HttpStatus::kNotFound);
            return R"({"error":"Booking not found"})";
        }
        if (booking->user_id != user_id) {
            request.GetHttpResponse().SetStatus(
                userver::server::http::HttpStatus::kForbidden);
            return R"({"error":"Access denied"})";
        }
        if (!storage_.CancelBooking(booking_id)) {
            request.GetHttpResponse().SetStatus(
                userver::server::http::HttpStatus::kConflict);
            return R"({"error":"Booking already cancelled"})";
        }

        return R"({"message":"Booking cancelled successfully"})";
    }

private:
    StorageComponent& storage_;
};

} // namespace

void AppendBookingHandlers(userver::components::ComponentList& component_list) {
    component_list.Append<BookingHandler>();
    component_list.Append<BookingCancelHandler>();
}

} // namespace booking
