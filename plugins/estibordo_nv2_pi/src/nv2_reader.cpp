#include "nv2_reader.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace estibordo::nv2 {
namespace {
constexpr double kEarthRadiusM = 6378137.0;

std::uint16_t U16(const std::vector<std::uint8_t>& b, std::size_t o) {
  if (o + 2 > b.size()) throw std::runtime_error("NV2 truncated u16");
  return static_cast<std::uint16_t>(b[o]) |
         (static_cast<std::uint16_t>(b[o + 1]) << 8);
}
std::uint32_t U32(const std::vector<std::uint8_t>& b, std::size_t o) {
  if (o + 4 > b.size()) throw std::runtime_error("NV2 truncated u32");
  return static_cast<std::uint32_t>(b[o]) |
         (static_cast<std::uint32_t>(b[o + 1]) << 8) |
         (static_cast<std::uint32_t>(b[o + 2]) << 16) |
         (static_cast<std::uint32_t>(b[o + 3]) << 24);
}
std::int32_t I32(const std::vector<std::uint8_t>& b, std::size_t o) {
  return static_cast<std::int32_t>(U32(b, o));
}
bool IsPrintable(std::uint8_t c) {
  return c == 9 || c == 10 || c == 13 || (c >= 32 && c < 127);
}
bool Digits16(const std::string& s) {
  return s.size() == 16 && std::all_of(s.begin(), s.end(), [](unsigned char c) { return c >= '0' && c <= '9'; });
}
std::optional<std::size_t> MetadataStart(const std::vector<std::uint8_t>& b) {
  static constexpr char marker[] = "Marine e-chart";
  const auto limit = std::min<std::size_t>(b.size(), 16384);
  const auto begin = b.begin();
  auto it = std::search(begin, begin + static_cast<std::ptrdiff_t>(limit), std::begin(marker), std::end(marker) - 1);
  if (it == begin + static_cast<std::ptrdiff_t>(limit)) return std::nullopt;
  const auto p = static_cast<std::size_t>(std::distance(begin, it));
  if (p < 4) return std::nullopt;
  if (U16(b, p - 4) != 9 || U16(b, p - 2) != 14) return std::nullopt;
  return p - 4;
}
void AssignMetadata(std::uint16_t tag, const std::string& value, Metadata& m) {
  switch (tag) {
    case 9: if (m.format.empty()) m.format = value; break;
    case 10: if (m.chart_id.empty()) m.chart_id = value; break;
    case 11: if (m.title.empty()) m.title = value; break;
    case 12: if (m.vendor.empty()) m.vendor = value; break;
    case 13: if (m.attribution.empty()) m.attribution = value; break;
    default: break;
  }
}
Metadata ParseStableMetadata(const std::vector<std::uint8_t>& b) {
  Metadata m;
  auto start = MetadataStart(b);
  if (!start) return m;
  std::size_t o = *start;
  // Tags 9..13 are consecutive and structurally stable in every chart in the
  // current corpus. Stop at the first unknown/control record rather than guess.
  for (std::uint16_t expected = 9; expected <= 13; ++expected) {
    if (o + 4 > b.size()) break;
    const auto tag = U16(b, o);
    const auto len = U16(b, o + 2);
    if (tag != expected || len == 0 || len > 4096 || o + 4u + len > b.size()) break;
    if (!std::all_of(b.begin() + static_cast<std::ptrdiff_t>(o + 4), b.begin() + static_cast<std::ptrdiff_t>(o + 4 + len), IsPrintable)) break;
    std::string value(reinterpret_cast<const char*>(b.data() + o + 4), len);
    AssignMetadata(tag, value, m);
    o += 4u + len;
  }
  return m;
}
}  // namespace

bool MercatorExtent::valid() const noexcept {
  const auto world = static_cast<std::int32_t>(std::ceil(M_PI * kEarthRadiusM));
  return min_x < max_x && min_y < max_y &&
         min_x >= -world && max_x <= world && min_y >= -world && max_y <= world;
}

std::pair<double, double> MercatorToLonLat(std::int32_t x, std::int32_t y) {
  constexpr double kPi = 3.14159265358979323846;
  const double lon = (static_cast<double>(x) / kEarthRadiusM) * 180.0 / kPi;
  const double lat = (2.0 * std::atan(std::exp(static_cast<double>(y) / kEarthRadiusM)) - kPi / 2.0) * 180.0 / kPi;
  return {lon, lat};
}

Document Reader::ParseFile(const std::filesystem::path& path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) throw std::runtime_error("Cannot open NV2 file");
  const auto end = f.tellg();
  if (end < 0) throw std::runtime_error("Cannot size NV2 file");
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
  f.seekg(0);
  if (!bytes.empty() && !f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
    throw std::runtime_error("Cannot read NV2 file");
  return Parse(bytes);
}

Document Reader::Parse(const std::vector<std::uint8_t>& b) {
  if (b.size() < 61) throw std::runtime_error("NV2 file shorter than confirmed header");
  Document d;
  d.header.magic = U32(b, 0);
  d.header.format_signature = U32(b, 4);
  d.header.declared_size = U32(b, 11);
  d.header.variant_byte = b[19];
  d.header.edition_stamp.assign(reinterpret_cast<const char*>(b.data() + 29), 16);
  d.header.extent = {I32(b, 45), I32(b, 49), I32(b, 53), I32(b, 57)};
  d.metadata = ParseStableMetadata(b);

  auto& v = d.validation;
  v.magic_ok = d.header.magic == kMagic;
  v.signature_ok = d.header.format_signature == kFormatSignature;
  v.declared_size_ok = d.header.declared_size == b.size();
  v.edition_stamp_ok = Digits16(d.header.edition_stamp);
  v.extent_ok = d.header.extent.valid();
  v.metadata_ok = d.metadata.format == "Marine e-chart" && !d.metadata.chart_id.empty() &&
                  !d.metadata.title.empty() && d.metadata.vendor == "Navionics";
  if (!v.magic_ok) v.warnings.emplace_back("magic mismatch");
  if (!v.signature_ok) v.warnings.emplace_back("format signature mismatch");
  if (!v.declared_size_ok) v.warnings.emplace_back("declared file size mismatch");
  if (!v.edition_stamp_ok) v.warnings.emplace_back("invalid edition stamp");
  if (!v.extent_ok) v.warnings.emplace_back("invalid Web Mercator extent");
  if (!v.metadata_ok) v.warnings.emplace_back("stable metadata validation failed");

  const bool all = v.magic_ok && v.signature_ok && v.declared_size_ok && v.edition_stamp_ok && v.extent_ok && v.metadata_ok;
  v.confidence = all ? Confidence::StructuralConfirmed :
      ((v.magic_ok && v.signature_ok) ? Confidence::Partial : Confidence::Unsupported);
  return d;
}

const char* ToString(Confidence c) noexcept {
  switch (c) {
    case Confidence::StructuralConfirmed: return "STRUCTURAL_CONFIRMED";
    case Confidence::Partial: return "PARTIAL";
    default: return "UNSUPPORTED";
  }
}

}  // namespace estibordo::nv2
