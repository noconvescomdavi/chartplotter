#pragma once
#include <windows.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct ChartExtent {
  double north{}, south{}, east{}, west{};
  bool valid() const { return north > south && east > west; }
};

class IChartProvider {
 public:
  virtual ~IChartProvider() = default;
  virtual const wchar_t* FormatName() const = 0;
  virtual const std::wstring& Title() const = 0;
  virtual ChartExtent Extent() const = 0;
  virtual bool Render(HDC dc, const RECT& chart_rect,
                      double center_lat, double center_lon, double zoom) const = 0;
};

std::unique_ptr<IChartProvider> OpenChartFile(const std::filesystem::path& path,
                                              std::wstring& error);
