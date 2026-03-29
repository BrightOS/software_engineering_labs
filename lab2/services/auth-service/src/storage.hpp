#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

namespace auth {

struct User {
  std::string id;
  std::string login;
  std::string password;
  std::string first_name;
  std::string last_name;
  std::string email;
};

class StorageComponent final : public userver::components::ComponentBase {
public:
  static constexpr std::string_view kName = "storage-component";

  StorageComponent(const userver::components::ComponentConfig &config,
                   const userver::components::ComponentContext &context);

  std::optional<User> AddUser(const std::string &login,
                              const std::string &password,
                              const std::string &first_name,
                              const std::string &last_name,
                              const std::string &email);

  std::optional<User> FindUserByLogin(const std::string &login) const;
  std::vector<User> SearchUsersByName(const std::string &mask) const;

  std::optional<std::string>
  ValidateCredentials(const std::string &login,
                      const std::string &password) const;

  std::string StoreRefreshToken(const std::string &user_id);

  std::optional<std::string>
  ValidateRefreshToken(const std::string &token) const;

  bool RevokeRefreshToken(const std::string &token);

private:
  mutable std::mutex mutex_;
  std::map<std::string, User> users_;
  std::map<std::string, std::string> login_to_id_;
  std::map<std::string, std::string> refresh_tokens_;
};

} // namespace auth
