#include "kap_chart.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <string_view>

namespace {
std::string Trim(std::string s) {
  while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
  size_t i = 0;
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
  return s.substr(i);
}

std::vector<std::string> Split(const std::string& s, char c) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start <= s.size()) {
    size_t p = s.find(c, start);
    if (p == std::string::npos) p = s.size();
    out.push_back(s.substr(start, p - start));
    if (p == s.size()) break;
    start = p + 1;
  }
  return out;
}

bool ParseInt(const std::string& s, int& out) {
  auto first = s.data(), last = s.data() + s.size();
  return std::from_chars(first, last, out).ec == std::errc{};
}

double ToDouble(const std::string& s) {
  char* e = nullptr;
  return std::strtod(s.c_str(), &e);
}

std::wstring Widen(const std::string& s) {
  if (s.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
  if (n <= 0) return std::wstring(s.begin(), s.end());
  std::wstring w(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
  return w;
}

bool ExtractKeyPair(const std::string& text, const char* key, int& a, int& b) {
  const std::string needle = std::string(key) + "=";
  const size_t p = text.find(needle);
  if (p == std::string::npos) return false;
  const size_t begin = p + needle.size();
  const size_t comma = text.find(',', begin);
  if (comma == std::string::npos) return false;
  size_t end = comma + 1;
  while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) || text[end] == '-')) ++end;
  return ParseInt(text.substr(begin, comma - begin), a) &&
         ParseInt(text.substr(comma + 1, end - comma - 1), b);
}

std::string ExtractName(const std::string& header) {
  const size_t p = header.find("NA=");
  if (p == std::string::npos) return "Untitled BSB/KAP";
  size_t e = header.find(',', p + 3);
  size_t nl = header.find_first_of("\r\n", p + 3);
  if (e == std::string::npos || (nl != std::string::npos && nl < e)) e = nl;
  if (e == std::string::npos) e = header.size();
  return Trim(header.substr(p + 3, e - (p + 3)));
}

bool DecodeVarInt(const std::vector<std::uint8_t>& b, size_t& pos, std::uint32_t& value) {
  value = 0;
  int guard = 0;
  while (pos < b.size() && guard++ < 6) {
    const std::uint8_t c = b[pos++];
    value = (value << 7) | (c & 0x7Fu);
    if ((c & 0x80u) == 0) return true;
  }
  return false;
}
} // namespace

std::unique_ptr<KapChart> KapChart::Open(const std::filesystem::path& path,
                                         std::wstring& error) {
  auto chart = std::unique_ptr<KapChart>(new KapChart());
  if (!chart->Load(path, error)) return {};
  return chart;
}

bool KapChart::Load(const std::filesystem::path& path, std::wstring& error) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) { error = L"Nao foi possivel abrir o arquivo KAP."; return false; }
  const auto size = f.tellg();
  if (size <= 0) { error = L"Arquivo KAP vazio."; return false; }
  std::vector<std::uint8_t> bytes(static_cast<size_t>(size));
  f.seekg(0);
  f.read(reinterpret_cast<char*>(bytes.data()), size);
  if (!f) { error = L"Falha ao ler o KAP."; return false; }

  size_t terminator = std::string::npos;
  for (size_t i = 0; i + 1 < bytes.size(); ++i) {
    if (bytes[i] == 0x1A && bytes[i + 1] == 0x00) { terminator = i; break; }
  }
  if (terminator == std::string::npos) {
    error = L"Cabecalho BSB/KAP sem terminador 0x1A00.";
    return false;
  }

  const std::string header(reinterpret_cast<const char*>(bytes.data()), terminator);
  title_ = Widen(ExtractName(header));
  if (!ExtractKeyPair(header, "RA", width_, height_) || width_ <= 0 || height_ <= 0) {
    error = L"Dimensoes RA= invalidas no cabecalho KAP.";
    return false;
  }

  int ifm = 0;
  {
    const size_t p = header.find("IFM/");
    if (p != std::string::npos) {
      size_t e = header.find_first_of("\r\n", p);
      ifm = std::atoi(header.substr(p + 4, e - p - 4).c_str());
    }
  }
  if (ifm < 1 || ifm > 7) {
    error = L"Profundidade IFM nao suportada.";
    return false;
  }

  for (auto& c : palette_) { c.rgbRed = 30; c.rgbGreen = 55; c.rgbBlue = 70; c.rgbReserved = 0; }

  std::istringstream lines(header);
  std::string line;
  while (std::getline(lines, line)) {
    line = Trim(line);
    if (line.rfind("RGB/", 0) == 0) {
      auto parts = Split(line.substr(4), ',');
      if (parts.size() >= 4) {
        int idx = std::atoi(parts[0].c_str());
        if (idx >= 0 && idx < static_cast<int>(palette_.size())) {
          palette_[idx].rgbRed = static_cast<BYTE>(std::clamp(std::atoi(parts[1].c_str()), 0, 255));
          palette_[idx].rgbGreen = static_cast<BYTE>(std::clamp(std::atoi(parts[2].c_str()), 0, 255));
          palette_[idx].rgbBlue = static_cast<BYTE>(std::clamp(std::atoi(parts[3].c_str()), 0, 255));
        }
      }
    } else if (line.rfind("REF/", 0) == 0) {
      auto p = Split(line.substr(4), ',');
      if (p.size() >= 5) {
        refs_.push_back({std::atoi(p[0].c_str()), ToDouble(p[1]), ToDouble(p[2]), ToDouble(p[3]), ToDouble(p[4])});
      }
    } else if (line.rfind("PLY/", 0) == 0) {
      auto p = Split(line.substr(4), ',');
      if (p.size() >= 3) polygon_.push_back({ToDouble(p[1]), ToDouble(p[2])});
    }
  }

  ComputeExtent();
  if (!extent_.valid()) {
    error = L"KAP sem REF/PLY geograficamente valido.";
    return false;
  }

  if (!DecodeRaster(bytes, terminator + 2, ifm, error)) return false;

  bmi_storage_.resize(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
  bitmap_info_ = reinterpret_cast<BITMAPINFO*>(bmi_storage_.data());
  bitmap_info_->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info_->bmiHeader.biWidth = width_;
  bitmap_info_->bmiHeader.biHeight = -height_; // top-down
  bitmap_info_->bmiHeader.biPlanes = 1;
  bitmap_info_->bmiHeader.biBitCount = 8;
  bitmap_info_->bmiHeader.biCompression = BI_RGB;
  bitmap_info_->bmiHeader.biSizeImage = static_cast<DWORD>(pixels_.size());
  bitmap_info_->bmiHeader.biClrUsed = 128;
  bitmap_info_->bmiHeader.biClrImportant = 128;
  for (size_t i = 0; i < palette_.size(); ++i) bitmap_info_->bmiColors[i] = palette_[i];

  return true;
}

void KapChart::ComputeExtent() {
  extent_.north = -90; extent_.south = 90; extent_.east = -180; extent_.west = 180;
  auto accept = [this](double lat, double lon) {
    if (!std::isfinite(lat) || !std::isfinite(lon)) return;
    extent_.north = std::max(extent_.north, lat);
    extent_.south = std::min(extent_.south, lat);
    extent_.east = std::max(extent_.east, lon);
    extent_.west = std::min(extent_.west, lon);
  };
  if (!polygon_.empty()) for (const auto& [lat, lon] : polygon_) accept(lat, lon);
  else for (const auto& r : refs_) accept(r.lat, r.lon);
}

bool KapChart::DecodeRaster(const std::vector<std::uint8_t>& bytes,
                            size_t binary_offset, int ifm, std::wstring& error) {
  if (binary_offset >= bytes.size()) { error = L"Secao raster KAP ausente."; return false; }

  const int depth = bytes[binary_offset++];
  if (depth != ifm && depth >= 1 && depth <= 7) ifm = depth;

  const size_t stride = static_cast<size_t>((width_ + 3) & ~3);
  pixels_.assign(stride * static_cast<size_t>(height_), 0);

  const std::uint8_t color_mask = static_cast<std::uint8_t>((1u << ifm) - 1u);
  const int count_bits = 7 - ifm;
  const std::uint8_t count_mask = static_cast<std::uint8_t>((1u << count_bits) - 1u);

  size_t pos = binary_offset;
  int rows_decoded = 0;
  while (pos < bytes.size() && rows_decoded < height_) {
    std::uint32_t row_number = 0;
    if (!DecodeVarInt(bytes, pos, row_number)) break;
    if (row_number == 0 || row_number > static_cast<std::uint32_t>(height_ + 1)) break;

    int x = 0;
    while (pos < bytes.size()) {
      std::uint8_t c = bytes[pos++];
      if (c == 0) break;

      const std::uint8_t color = static_cast<std::uint8_t>((c >> count_bits) & color_mask);
      std::uint32_t run = c & count_mask;
      while ((c & 0x80u) != 0) {
        if (pos >= bytes.size()) { error = L"KAP RLE truncado."; return false; }
        c = bytes[pos++];
        run = (run << 7) | (c & 0x7Fu);
      }
      ++run; // BSB stores run length minus one.

      if (x + static_cast<int>(run) > width_) run = static_cast<std::uint32_t>(std::max(0, width_ - x));
      const int row = static_cast<int>(row_number) - 1;
      if (row >= 0 && row < height_) {
        auto* dst = pixels_.data() + static_cast<size_t>(row) * stride + static_cast<size_t>(x);
        std::fill_n(dst, run, color);
      }
      x += static_cast<int>(run);
      if (x >= width_) {
        while (pos < bytes.size() && bytes[pos] != 0) ++pos;
        if (pos < bytes.size() && bytes[pos] == 0) ++pos;
        break;
      }
    }
    ++rows_decoded;
  }

  if (rows_decoded < std::min(height_, 2)) {
    error = L"Nao foi possivel decodificar as linhas raster BSB/KAP.";
    return false;
  }
  return true;
}

bool KapChart::Render(HDC dc, const RECT& chart_rect,
                      double center_lat, double center_lon, double zoom) const {
  if (!bitmap_info_ || pixels_.empty() || !extent_.valid()) return false;

  const double center_cos = std::cos(center_lat * 3.14159265358979323846 / 180.0);
  auto to_x = [&](double lon) {
    return static_cast<int>((chart_rect.left + chart_rect.right) / 2.0 +
      (lon - center_lon) * center_cos * zoom);
  };
  auto to_y = [&](double lat) {
    return static_cast<int>((chart_rect.top + chart_rect.bottom) / 2.0 -
      (lat - center_lat) * zoom);
  };

  const int left = to_x(extent_.west);
  const int right = to_x(extent_.east);
  const int top = to_y(extent_.north);
  const int bottom = to_y(extent_.south);
  if (right <= chart_rect.left || left >= chart_rect.right ||
      bottom <= chart_rect.top || top >= chart_rect.bottom) return true;

  const int result = StretchDIBits(dc, left, top, right - left, bottom - top,
                                   0, 0, width_, height_, pixels_.data(),
                                   bitmap_info_, DIB_RGB_COLORS, SRCCOPY);
  return result != GDI_ERROR;
}
