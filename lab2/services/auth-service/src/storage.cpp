#include "storage.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

#include <openssl/sha.h>

#include <userver/utils/uuid4.hpp>

namespace {

std::string HashPassword(const std::string &password) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char *>(password.data()),
         password.size(), hash);
  std::ostringstream oss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }
  return oss.str();
}

} // namespace

namespace auth {

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context) {}

std::optional<User> StorageComponent::AddUser(const std::string &login,
                                              const std::string &password,
                                              const std::string &first_name,
                                              const std::string &last_name,
                                              const std::string &email) {
  std::lock_guard lock(mutex_);
  if (login_to_id_.count(login)) {
    return std::nullopt;
  }
  User user;
  user.id = userver::utils::generators::GenerateUuid();
  user.login = login;
  user.password = HashPassword(password);
  user.first_name = first_name;
  user.last_name = last_name;
  user.email = email;
  users_[user.id] = user;
  login_to_id_[login] = user.id;
  return user;
}

std::optional<User>
StorageComponent::FindUserByLogin(const std::string &login) const {
  std::lock_guard lock(mutex_);
  auto it = login_to_id_.find(login);
  if (it == login_to_id_.end())
    return std::nullopt;
  auto uit = users_.find(it->second);
  if (uit == users_.end())
    return std::nullopt;
  return uit->second;
}

std::vector<User>
StorageComponent::SearchUsersByName(const std::string &mask) const {
  std::lock_guard lock(mutex_);
  std::string lower_mask = mask;
  std::transform(lower_mask.begin(), lower_mask.end(), lower_mask.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  std::vector<User> result;
  for (const auto &[id, user] : users_) {
    std::string full_name = user.first_name + " " + user.last_name;
    std::transform(full_name.begin(), full_name.end(), full_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (full_name.find(lower_mask) != std::string::npos) {
      result.push_back(user);
    }
  }
  return result;
}

std::optional<std::string>
StorageComponent::ValidateCredentials(const std::string &login,
                                      const std::string &password) const {
  std::lock_guard lock(mutex_);
  auto it = login_to_id_.find(login);
  if (it == login_to_id_.end())
    return std::nullopt;
  auto uit = users_.find(it->second);
  if (uit == users_.end())
    return std::nullopt;
  if (uit->second.password != HashPassword(password))
    return std::nullopt;
  return uit->second.id;
}

std::string StorageComponent::StoreRefreshToken(const std::string &user_id) {
  auto token = userver::utils::generators::GenerateUuid();
  std::lock_guard lock(mutex_);
  refresh_tokens_[token] = user_id;
  return token;
}

std::optional<std::string>
StorageComponent::ValidateRefreshToken(const std::string &token) const {
  std::lock_guard lock(mutex_);
  auto it = refresh_tokens_.find(token);
  if (it == refresh_tokens_.end())
    return std::nullopt;
  return it->second;
}

bool StorageComponent::RevokeRefreshToken(const std::string &token) {
  std::lock_guard lock(mutex_);
  return refresh_tokens_.erase(token) > 0;
}

} // namespace auth
