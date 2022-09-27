#pragma once

#include <filesystem>
#include <string>

#include <fmt/core.h>
#include <libzippp/libzippp.h>

#include "detail/Connection.hpp"
#include "detail/ModelSearch.hpp"

namespace SketchFetch
{
enum class SketchFlag : bool
{
  CreateFolder = true,
  DontCreateFolder = false
};

class SketchFetch
{
public:
  using ModelSearchResults = std::optional<std::vector<ModelSearchResult>>;
  using download_t = std::vector<uint8_t>;

  SketchFetch();
  SketchFetch(std::filesystem::path, std::filesystem::path, std::filesystem::path);
  SketchFetch(std::string_view, std::string_view, SketchFlag);

  auto setAccess(std::string_view, std::string_view) -> void;
  auto authenticate() -> bool;
  auto getAuthenticated() -> bool;
  auto setThumbnailFolder(std::filesystem::path const&, bool) -> void;
  auto setModelFolder(std::filesystem::path const&, bool) -> void;
  auto setWorkingFolder(std::filesystem::path const&) -> void;

  auto getWorkingFolder() -> std::filesystem::path;
  auto getModelFolder() -> std::filesystem::path;
  auto getThumbnailFolder() -> std::filesystem::path;

  [[nodiscard]] auto search(ModelSearchQuery const&) const -> std::optional<std::vector<ModelSearchResult>>;
  [[nodiscard]] auto searchJSON(ModelSearchQuery const&) const -> std::optional<json>;

  // auto getThumbnails(ModelSearchResult const&) const
  //     -> std::unordered_map<std::string, download_t>;

  auto getThumbnail(ModelSearchResult const&) const -> std::optional<download_t>;
  auto downloadThumbnail(ModelSearchResult const&) const -> void;

  auto getThumbnail(std::string_view) const -> std::optional<download_t>;
  auto downloadThumbnail(std::string_view, std::string_view) const -> void;

  [[nodiscard]] auto getModel(ModelSearchResult const&) const -> std::optional<download_t>;
  auto downloadModel(std::string_view) const -> void;
  auto downloadModel(ModelSearchResult const&) const -> void;

  auto print() -> void;

private:
  std::filesystem::path working_folder = std::filesystem::current_path();
  std::filesystem::path thumbnail_folder = std::filesystem::current_path().append("thumbnails");
  std::filesystem::path model_folder = std::filesystem::current_path().append("models");
  Connection conn;

  auto storeToDisk(const download_t&, std::filesystem::path) const -> void;
  auto createOrCheckFolder(std::filesystem::path const&, bool) const -> void;
};

inline auto SketchFetch::print() -> void
{
  fmt::print(
      "Authenticated: {}\nWorking Folder: {}\nModel Folder: {}\nThumbnail "
      "Folder: {}\n",
      conn.getAuthenticated(),
      working_folder.string(),
      model_folder.string(),
      thumbnail_folder.string());
}
inline SketchFetch::SketchFetch()
{
  setModelFolder(model_folder, true);
  setThumbnailFolder(thumbnail_folder, true);
}

inline SketchFetch::SketchFetch(std::string_view username_,
                                std::string_view password_,
                                SketchFlag create = SketchFlag::DontCreateFolder)
    : conn(username_, password_)
{
  setModelFolder(model_folder, create == SketchFlag::CreateFolder);
  setThumbnailFolder(thumbnail_folder, create == SketchFlag::CreateFolder);
}

inline auto SketchFetch::getThumbnail(std::string_view url) const -> std::optional<download_t>
{
  return conn.download(url);
}
inline auto SketchFetch::downloadThumbnail(std::string_view url, std::string_view name) const -> void
{
  auto thumbnail = getThumbnail(url);
  auto file_name = thumbnail_folder;
  file_name /= name;
  file_name += ".jpg";
  storeToDisk(*thumbnail, file_name);
}

inline auto SketchFetch::getThumbnail(ModelSearchResult const& result) const -> std::optional<download_t>
{
  return conn.download(result.thumbnail);
}

inline auto SketchFetch::downloadThumbnail(ModelSearchResult const& result) const -> void
{
  auto thumbnail = getThumbnail(result);
  auto file_name = thumbnail_folder;
  file_name /= result.uid + ".jpg";  // TODO: get thumbnail UID
  storeToDisk(*thumbnail, file_name);
}

inline auto SketchFetch::storeToDisk(const download_t& data, std::filesystem::path path) const -> void
{
  if (std::ofstream os {path, std::ios::binary}; os.is_open()) {
    os.write(reinterpret_cast<const char*>(data.data()), data.size());
  } else {
    throw file_error(fmt::format("Can't write File: {}", path.string()));
  }
}

inline auto SketchFetch::createOrCheckFolder(std::filesystem::path const& path, bool create) const -> void
{
  using namespace std::filesystem;
  if (create) {
    create_directory(path, working_folder);
  }
  if (!exists(path)) {
    throw file_error("Folder doesn't exist");
  }
}

[[nodiscard]] inline auto SketchFetch::search(ModelSearchQuery const& query) const
    -> std::optional<std::vector<ModelSearchResult>>
{
  if (const auto temp_result = conn.get(query); temp_result && !(*temp_result)["results"].empty()) {
    std::vector<ModelSearchResult> model_search_results;
    for (auto const& result : (*temp_result)["results"]) {
      model_search_results.push_back(
          ModelSearchResult {.uri = result["uri"].get<std::string>(),
                             .uid = result["uid"].get<std::string>(),
                             .name = result["name"].get<std::string>(),
                             .description = result["description"].get<std::string>(),
                             .thumbnail = result["thumbnails"]["images"][0]["url"].get<std::string>(),
                             .views = result["viewCount"].get<std::size_t>(),
                             .likes = result["likeCount"].get<std::size_t>(),
                             .isDownloadable = result["isDownloadable"].get<bool>()});
    }
    return std::make_optional(model_search_results);
  }
  return std::nullopt;
}

[[nodiscard]] inline auto SketchFetch::searchJSON(ModelSearchQuery const& query) const -> std::optional<json>
{
  return conn.get(query);
}

inline auto SketchFetch::setAccess(std::string_view username_, std::string_view password_) -> void
{
  conn.setAccess(username_, password_);
}

inline auto SketchFetch::authenticate() -> bool
{
  return conn.getAuthenticated();
}

inline auto SketchFetch::getAuthenticated() -> bool
{
  return conn.getAuthenticated();
}

inline auto SketchFetch::setThumbnailFolder(std::filesystem::path const& thumbnail_folder_, bool create = false) -> void
{
  createOrCheckFolder(thumbnail_folder_, create);
  thumbnail_folder = thumbnail_folder_;
}

inline auto SketchFetch::setModelFolder(std::filesystem::path const& model_folder_, bool create = false) -> void
{
  createOrCheckFolder(model_folder_, create);
  model_folder = model_folder_;
}

inline auto SketchFetch::setWorkingFolder(std::filesystem::path const& working_folder_) -> void
{
  createOrCheckFolder(working_folder_, false);
  working_folder = working_folder_;
}

inline auto SketchFetch::getWorkingFolder() -> std::filesystem::path
{
  return working_folder;
}
inline auto SketchFetch::getModelFolder() -> std::filesystem::path
{
  return model_folder;
}
inline auto SketchFetch::getThumbnailFolder() -> std::filesystem::path
{
  return thumbnail_folder;
}

[[nodiscard]] auto SketchFetch::getModel(ModelSearchResult const& result) const -> std::optional<download_t>
{
  std::string url;
  try {
    url = std::get<std::string>(conn.getModelDownloadURI(result.uid));
  } catch (const std::exception& e) {
    fmt::print("getModel(): {}\n", e.what());
    return std::nullopt;
  }

  return conn.download(url);
}
auto SketchFetch::downloadModel(std::string_view url) const -> void {}
auto SketchFetch::downloadModel(ModelSearchResult const& result) const -> void
{
  if (auto data = getModel(result)) {
    auto file_name = model_folder;
    file_name /= result.uid;
    auto folder = file_name;
    create_directory(file_name, model_folder);
    file_name /= result.uid;
    file_name += ".zip";
    storeToDisk(*data, file_name);
    std::ofstream namefile {folder / "name.txt", std::ios::binary};
    namefile.write(result.name.c_str(), result.name.size());
    namefile.close();
    auto zip = libzippp::ZipArchive(file_name.string());
    zip.open(libzippp::ZipArchive::ReadOnly);
    for (auto&& entry : zip.getEntries()) {
      if (entry.isDirectory()) {
        create_directory(folder / entry.getName(), folder);
      }
      std::ofstream os {folder / entry.getName(), std::ios::binary};
      os.write(static_cast<const char*>(entry.readAsBinary()), entry.getSize());
    }
    zip.close();
  } else
    throw std::runtime_error {"Can't download"};
}

}  // namespace SketchFetch