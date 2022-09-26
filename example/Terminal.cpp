#include <SketchFetch/SketchFetch.hpp>

int main(int, char const**)
{
  SketchFetch::SketchFetch fetchy {};
  // fetchy.print();
  auto result = fetchy.search({.q = "bunny",
                               .tags = {{}},
                               .date = SketchFetch::date_t::month,
                               .downloadable = true,
                               .sort_by = SketchFetch::sort_by_t::nviewCount,
                               .count = 24});
  // fmt::print("{}\n", (*result)[0]);
  try {
    fetchy.setAccess("", "");
  } catch (const std::exception& e) {
    fmt::print("{}\n", e.what());
  }

  fetchy.downloadModel((*result)[2]);
}
