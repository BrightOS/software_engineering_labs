#include "storage.hpp"

#include <iomanip>
#include <sstream>

#include <openssl/sha.h>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
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

namespace {

template <typename Row>
User RowToUser(const Row &row) {
  User u;
  u.id = row["id"].template As<std::string>();
  u.login = row["login"].template As<std::string>();
  u.first_name = row["first_name"].template As<std::string>();
  u.last_name = row["last_name"].template As<std::string>();
  u.email = row["email"].template As<std::string>();
  return u;
}

} // namespace

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig &config,
    const userver::components::ComponentContext &context)
    : ComponentBase(config, context),
      pg_cluster_(
          context.FindComponent<userver::components::Postgres>("auth-database")
              .GetCluster()) {}

std::optional<User> StorageComponent::AddUser(const std::string &login,
                                              const std::string &password,
                                              const std::string &first_name,
                                              const std::string &last_name,
                                              const std::string &email) {
  auto hashed = HashPassword(password);
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO users (login, password, first_name, last_name, email) "
      "VALUES ($1, $2, $3, $4, $5) "
      "ON CONFLICT (login) DO NOTHING "
      "RETURNING id::text, login, password, first_name, last_name, "
      "COALESCE(email, '') as email",
      login, hashed, first_name, last_name, email);

  if (result.IsEmpty()) {
    return std::nullopt;
  }
  return RowToUser(result[0]);
}

std::optional<User>
StorageComponent::FindUserByLogin(const std::string &login) const {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT id::text, login, password, first_name, last_name, "
      "COALESCE(email, '') as email "
      "FROM users WHERE login = $1",
      login);

  if (result.IsEmpty()) {
    return std::nullopt;
  }
  return RowToUser(result[0]);
}

std::vector<User>
StorageComponent::SearchUsersByName(const std::string &mask) const {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT id::text, login, first_name, last_name, "
      "COALESCE(email, '') as email "
      "FROM users "
      "WHERE LOWER(first_name || ' ' || last_name) LIKE '%' || LOWER($1) || '%'",
      mask);

  std::vector<User> users;
  for (auto row : result) {
    users.push_back(RowToUser(row));
  }
  return users;
}

std::optional<std::string>
StorageComponent::ValidateCredentials(const std::string &login,
                                      const std::string &password) const {
  auto hashed = HashPassword(password);
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT id::text FROM users WHERE login = $1 AND password = $2",
      login, hashed);

  if (result.IsEmpty()) {
    return std::nullopt;
  }

  return result[0]["id"].As<std::string>();
}

std::string StorageComponent::StoreRefreshToken(const std::string &user_id) {
  auto token = userver::utils::generators::GenerateUuid();
  pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "INSERT INTO refresh_tokens (token, user_id) VALUES ($1::uuid, $2::uuid)",
      token, user_id);
  return token;
}

std::optional<std::string>
StorageComponent::ValidateRefreshToken(const std::string &token) const {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      "SELECT user_id::text FROM refresh_tokens WHERE token = $1::uuid",
      token);

  if (result.IsEmpty()) {
    return std::nullopt;
  }

  return result[0]["user_id"].As<std::string>();
}

bool StorageComponent::RevokeRefreshToken(const std::string &token) {
  auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      "DELETE FROM refresh_tokens WHERE token = $1::uuid",
      token);
  return result.RowsAffected() > 0;
}

} // namespace auth
