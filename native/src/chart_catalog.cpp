#include "chart_catalog.h"

#include <algorithm>
#include <cwctype>

namespace {
std::wstring Lower(std::wstring s) {
  std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
  return s;
}
}

void ChartCatalog::Clear() { entries_.clear(); }

ChartFormat ChartCatalog::Detect(const std::filesystem::path& path) {
  const auto ext = Lower(path.extension().wstring());
  if (ext == L".kap") return ChartFormat::Kap;
  if (ext == L".000") return ChartFormat::S57;
  if (ext == L".mbtiles") return ChartFormat::Mbtiles;
  if (ext == L".nv2") return ChartFormat::Nv2;
  return ChartFormat::Unknown;
}

const wchar_t* ChartCatalog::FormatName(ChartFormat f) {
  switch (f) {
    case ChartFormat::Kap: return L"BSB/KAP";
    case ChartFormat::S57: return L"S-57 ENC";
    case ChartFormat::Mbtiles: return L"MBTiles";
    case ChartFormat::Nv2: return L"NV2";
    case ChartFormat::Cm93: return L"CM93";
    default: return L"Unknown";
  }
}

void ChartCatalog::Scan(const std::filesystem::path& root, bool recursive) {
  entries_.clear();
  if (!std::filesystem::exists(root)) return;

  auto append = [this](const std::filesystem::directory_entry& item) {
    if (!item.is_regular_file()) return;
    const auto format = Detect(item.path());
    if (format == ChartFormat::Unknown) return;
    entries_.push_back({item.path(), format, item.path().filename().wstring()});
  };

  std::error_code ec;
  if (recursive) {
    for (std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied, ec), end;
         it != end; it.increment(ec)) {
      if (ec) { ec.clear(); continue; }
      append(*it);
    }
  } else {
    for (const auto& item : std::filesystem::directory_iterator(root, ec)) {
      if (ec) break;
      append(item);
    }
  }

  std::sort(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
    if (a.format != b.format) return static_cast<int>(a.format) < static_cast<int>(b.format);
    return a.display_name < b.display_name;
  });
}
