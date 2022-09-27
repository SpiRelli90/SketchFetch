#pragma once
#include <charconv>
#include <fstream>

#include "Exception.hpp"
#include "SketchFabAPI.hpp"
#include "Util.hpp"
#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "fmt/os.h"
#include "httplib.h"

namespace SketchFetch::Authentication
{
class Auth
{
public:
  Auth() = default;
  Auth(std::string_view, std::string_view);
  Auth(std::string_view, std::string_view, std::string_view);

  [[nodiscard]] auto getAccessToken() const -> std::string_view;
  auto authenticate(std::string_view, std::string_view) -> void;
  auto authenticate() -> void;
  auto getAuthenticated() const -> bool;

private:
  std::string username, password, client_id;  // client_id unused
  std::string access_token, refresh_token;
  std::string auth_token {SketchFabAPI::auth_token};
  long long token_valid_until;
  bool authenticated {false};  // True if authentication was successful

  auto getToken() -> void;
  auto refreshToken() -> void;
  auto storeToken() const -> void;
  auto loadToken() -> bool;

  auto sendAndParse(httplib::Params const&) -> void;
};

/**
 * @brief Provides access token for SketchFab API access
 * @pre RestClient has to be initialized
 */
inline Auth::Auth(std::string_view username_, std::string_view password_)
    : username(username_)
    , password(password_)
    , auth_token(SketchFabAPI::auth_token)
{
  authenticate();
};

inline Auth::Auth(std::string_view username_, std::string_view password_, std::string_view client_id_)
    : Auth(username_, password_)
{
  client_id = client_id_;
}

inline auto Auth::authenticate() -> void
{
  using namespace std::chrono;
  const bool loaded = loadToken();
  if (loaded && time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count() < token_valid_until) {
    authenticated = true;
    return;
  } else if (loaded) {
    refreshToken();
  } else {
    getToken();
  }
  authenticated = true;
  storeToken();
}

inline auto Auth::authenticate(std::string_view username_, std::string_view password_) -> void
{
  username = username_;
  password = password_;
  authenticate();
}

inline auto Auth::getAuthenticated() const -> bool
{
  return authenticated;
}

inline auto Auth::getAccessToken() const -> std::string_view
{
  if (!authenticated)
    throw std::runtime_error("Missing Username and Password");
  return std::string_view(access_token);
};

inline auto Auth::getToken() -> void
{
  httplib::Params param;
  param.emplace("grant_type", "password");
  param.emplace("client_id", auth_token);
  param.emplace("username", username);
  param.emplace("password", password);

  sendAndParse(param);
}

inline auto Auth::refreshToken() -> void
{
  httplib::Params param;
  param.emplace("grant_type", "refresh_token");
  param.emplace("refresh_token", refresh_token);
  param.emplace("client_id", auth_token);
  param.emplace("username", username);
  param.emplace("password", password);

  sendAndParse(param);
}

inline auto Auth::storeToken() const -> void
{
  auto file = fmt::output_file("sketchfab.auth");

  file.print("{};{};{}", access_token, refresh_token, token_valid_until);
}

inline auto Auth::loadToken() -> bool
{
  std::ifstream file {"sketchfab.auth", std::ios::in};
  std::tm t {};
  if (file.good()) {
    std::getline(file, access_token, ';');
    std::getline(file, refresh_token, ';');
    std::string time_string;
    std::getline(file, time_string, ';');
    std::from_chars(time_string.c_str(), time_string.c_str() + time_string.size(), token_valid_until);
  } else {
    return false;
  }
  return true;
}

inline auto Auth::sendAndParse(httplib::Params const& params) -> void
{
  httplib::Client cli(SketchFabAPI::auth_base.data());
  if (const auto response = cli.Post(SketchFabAPI::auth_query.data(), params)) {
    if (response->status == 200) {
      auto json_response = json::parse(response->body);
      access_token = json_response.at("access_token");
      refresh_token = json_response.at("refresh_token");
      const int expires_in = json_response.at("expires_in");
      token_valid_until = (time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())
                           + std::chrono::seconds {expires_in})
                              .time_since_epoch()
                              .count();
    } else {
      throw authentication_error(
          fmt::format("Authentication Error:\tCode: {}\n{}\n", response->status, response->body));
    }
  } else {
    throw std::runtime_error(std::format("Httplib Error: {}", to_string(response.error())));
  }
}
}  // namespace SketchFetch::Authentication
