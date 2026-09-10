#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace estibordo::nv2 {

constexpr std::uint32_t kMagic = 0x00FE8050u;
constexpr std::uint32_t kFormatSignature = 0x081273ABu;

struct MercatorExtent {
  std::int32_t min_x{};
  std::int32_t min_y{};
  std::int32_t max_x{};
  std::int32_t max_y{};
  bool valid() const noexcept;
};

struct Header {
  std::uint32_t magic{};
  std::uint32_t format_signature{};
  std::uint32_t declared_size{};
  std::uint8_t variant_byte{};
  std::string edition_stamp;
  MercatorExtent extent;
};

struct Metadata {
  std::string format;
  std::string chart_id;
  std::string title;
  std::string vendor;
  std::string attribution;
};

enum class Confidence {
  Unsupported,
  Partial,
  StructuralConfirmed
};

struct Validation {
  Confidence confidence{Confidence::Unsupported};
  bool magic_ok{};
  bool signature_ok{};
  bool declared_size_ok{};
  bool edition_stamp_ok{};
  bool extent_ok{};
  bool metadata_ok{};
  std::vector<std::string> warnings;
};

struct Document {
  Header header;
  Metadata metadata;
  Validation validation;
};

class Reader {
 public:
  static Document ParseFile(const std::filesystem::path& path);
  static Document Parse(const std::vector<std::uint8_t>& bytes);
};

std::pair<double, double> MercatorToLonLat(std::int32_t x, std::int32_t y);
const char* ToString(Confidence confidence) noexcept;

}  // namespace estibordo::nv2
