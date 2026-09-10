#pragma once
#include <filesystem>
#include <string>
#include <vector>

enum class ChartFormat { Kap, S57, Mbtiles, Nv2, Cm93, Unknown };

struct ChartCatalogEntry {
  std::filesystem::path path;
  ChartFormat format{ChartFormat::Unknown};
  std::wstring display_name;
};

class ChartCatalog {
 public:
  void Clear();
  void Scan(const std::filesystem::path& root, bool recursive = true);
  const std::vector<ChartCatalogEntry>& Entries() const { return entries_; }

  static ChartFormat Detect(const std::filesystem::path& path);
  static const wchar_t* FormatName(ChartFormat format);

 private:
  std::vector<ChartCatalogEntry> entries_;
};
