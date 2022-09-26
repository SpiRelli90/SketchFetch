#pragma once
#include <fstream>
#include <future>
#include <optional>
#include <span>
#include <string>
#include <variant>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <nlohmann/json.hpp>

#include "Authentication.hpp"
#include "Util.hpp"
#include "httplib.h"
// #include "restclient-cpp/connection.h"
// #include "restclient-cpp/restclient.h"
using json = nlohmann::json;
namespace SketchFetch
{

class Connection
{
private:
  Authentication::Auth auth;

public:
  Connection(std::string_view username_, std::string_view password_);
  Connection() = default;
  // ~Connection();
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  auto get(const std::string& query_) const -> std::optional<json>;
  auto download(std::string_view url) const
      -> std::optional<std::vector<uint8_t>>;
  auto getModelDownloadURI(std::string_view model_uid) const
      -> std::variant<std::string, int>;
  auto downloadThumbnails(std::span<std::string> urls) const
      -> std::optional<std::vector<std::vector<uint8_t>>>;

  auto setAccess(std::string_view, std::string_view) -> void;
  auto getAuthenticated() -> bool;
  auto authenticate() -> bool;
};

Connection::Connection(std::string_view username_, std::string_view password_)
    : auth(username_, password_)
{
}

inline auto Connection::get(const std::string& query) const
    -> std::optional<json>
{
  Util::Timer t;
  httplib::Client conn {SketchFabAPI::api_endpoint.data()};
  if (auth.getAuthenticated()) {
    conn.set_bearer_token_auth(auth.getAccessToken().data());
  }

  std::string str = "/v3" + query;
  auto r = conn.Get(str.c_str());
  if (r->status == 200) {
    try {
      return json::parse(r->body);
    } catch (const std::exception& e) {
      fmt::print(fg(fmt::color::red), "ERROR Parsing Error: {}\n", e.what());
    }
  } else {
    fmt::print(fg(fmt::color::red), "ERROR Response Code: {}\n", r->status);
  }
  return std::nullopt;
}

auto Connection::download(std::string_view url) const
    -> std::optional<std::vector<uint8_t>>
{
  Util::Timer t;
  auto sep = url.find(".com");
  auto path {url.substr(sep + 4)};
  std::string host {url.substr(0, sep + 4)};
  fmt::print("{}\n{}\n{}\n", sep, path, host);
  httplib::Client conn {host.c_str()};
  if (auto r = conn.Get(path.data())) {
    if (r->status == 200) {
      return std::vector<uint8_t>(r->body.begin(), r->body.end());
    } else {
      fmt::print(fg(fmt::color::red), "ERROR Response Code: {}\n", r->status);
    }
  } else {
    throw std::runtime_error(
        std::format("Httplib Error: {}", to_string(r.error())));
  }
  return std::nullopt;
}

auto Connection::getModelDownloadURI(std::string_view model_uid) const
    -> std::variant<std::string, int>
{
  std::variant<std::string, int> ret;
  httplib::Client conn {SketchFabAPI::api_endpoint.data()};
  conn.set_bearer_token_auth(auth.getAccessToken().data());
  auto r = conn.Get(fmt::format("/v3/models/{}/download", model_uid).c_str());

  if (r->status == 200) {
    try {
      auto resp = json::parse(r->body);
      ret = resp.at("gltf").at("url").get<std::string>();  // get options
    } catch (const std::exception& e) {
      fmt::print(
          fg(fmt::color::red), "ERROR Json Parsing Error: {}\n", e.what());
    }
  } else {
    ret = r->status;
  }
  return ret;
}

auto Connection::downloadThumbnails(std::span<std::string> urls) const
    -> std::optional<std::vector<std::vector<uint8_t>>>
{
  std::vector<std::future<std::optional<std::vector<uint8_t>>>> futures;
  std::vector<std::vector<uint8_t>> results;
  for (auto&& url : urls) {
    futures.push_back(std::async(std::launch::async,
                                 [this, url]() { return download(url); }));
  }
  for (auto&& f : futures) {
    try {
      if (auto result = f.get()) {
        if (result) {
          results.push_back(std::move(*result));
        }
      }
    } catch (const std::exception& e) {
      fmt::print(fg(fmt::color::red), "ERROR Future Get: {}\n", e.what());
    }
  }
  return results.empty() ? std::nullopt
                         : std::make_optional(std::move(results));
}

inline auto Connection::setAccess(std::string_view username_,
                                  std::string_view password_) -> void
{
  auth.authenticate(username_, password_);
}

inline auto Connection::authenticate() -> bool
{
  auth.authenticate();
  return auth.getAuthenticated();
}

inline auto Connection::getAuthenticated() -> bool
{
  return auth.getAuthenticated();
}
}  // namespace SketchFetch
