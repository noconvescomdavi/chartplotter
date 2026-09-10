#pragma once
#include "chart_provider.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct KapRefPoint {
  int id{};
  double pixel_x{}, pixel_y{};
  double lat{}, lon{};
};

class KapChart final : public IChartProvider {
 public:
  static std::unique_ptr<KapChart> Open(const std::filesystem::path& path,
                                        std::wstring& error);

  const wchar_t* FormatName() const override { return L"BSB/KAP"; }
  const std::wstring& Title() const override { return title_; }
  ChartExtent Extent() const override { return extent_; }
  bool Render(HDC dc, const RECT& chart_rect,
              double center_lat, double center_lon, double zoom) const override;

  int Width() const { return width_; }
  int Height() const { return height_; }

 private:
  bool Load(const std::filesystem::path& path, std::wstring& error);
  bool DecodeRaster(const std::vector<std::uint8_t>& bytes,
                    size_t binary_offset, int ifm, std::wstring& error);
  void ComputeExtent();

  std::wstring title_;
  int width_{};
  int height_{};
  std::array<RGBQUAD, 128> palette_{};
  std::vector<KapRefPoint> refs_;
  std::vector<std::pair<double,double>> polygon_;
  ChartExtent extent_{};
  std::vector<std::uint8_t> pixels_; // top-down 8-bit palette indexes
  BITMAPINFO* bitmap_info_{};
  std::vector<std::uint8_t> bmi_storage_;
};
