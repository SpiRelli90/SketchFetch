#pragma once
#include <exception>
#include <stdexcept>
#include <string_view>

namespace SketchFetch
{
// File error
class file_error : public std::exception
{
public:
  file_error(std::string_view what_)
      : e(what_.data())
  {
  }
  [[nodiscard]] const char* what() const noexcept override { return e.what(); }

private:
  std::runtime_error e;
};

// Network Error

// Authentiction Error
class authentication_error : public std::exception
{
public:
  authentication_error(std::string_view what_)
      : e(what_.data())
  {
  }
  [[nodiscard]] const char* what() const noexcept override { return e.what(); }

private:
  std::runtime_error e;
};

}  // namespace SketchFetch