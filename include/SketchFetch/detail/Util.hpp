#pragma once
#include <chrono>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace SketchFetch::Util
{
auto static printResponse(json response) -> void
{
  for (auto&& i : response["results"]) {
    fmt::print("\"{}\"{}\n\tViews: {}, Likes: {}\n\tDescription:{:.80}\n",
               i["name"].get<std::string>(),
               i["uid"].get<std::string>(),
               i["viewCount"].get<int>(),
               i["likeCount"].get<int>(),
               i["description"].get<std::string>());
  }
};

auto static percentEncode(std::string_view str) -> std::string
{
  std::string encoded;
  for (auto&& i : str) {
    // if (isalnum(i) || i == '-' || i == '_' || i == '.' || i == '~')
    if (isalnum(i)) {
      encoded += i;
    } else {
      encoded += fmt::format("%{:02X}", i);
    }
  }
  return encoded;
}

class Timer
{
  std::chrono::time_point<std::chrono::steady_clock> start;
  std::chrono::time_point<std::chrono::steady_clock> end;
  std::chrono::duration<double> elapsed {};
  std::source_location location;

public:
  Timer(const std::source_location a_location = std::source_location::current())
      : location(a_location)
  {
    start = std::chrono::steady_clock::now();
  }

  ~Timer()
  {
    end = std::chrono::steady_clock::now();
    elapsed = end - start;
    fmt::print("{}() Elapsed time: {} s\n", location.function_name(), elapsed.count());
  }
};
}  // namespace SketchFetch::Util
