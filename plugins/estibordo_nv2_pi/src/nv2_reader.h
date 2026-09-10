#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace estibordo::nv2 {
constexpr std::uint32_t kMagic = 0x00FE8050u;
constexpr std::uint32_t kFormatSignature = 0x081273ABu;
constexpr std::uint16_t kDictionaryBlockTag = 0x8030u;

struct MercatorExtent {
  std::int32_t min_x{}, min_y{}, max_x{}, max_y{};
  bool valid() const noexcept;
};
struct Header {
  std::uint32_t magic{}, format_signature{}, declared_size{};
  std::uint8_t variant_byte{};
  std::string edition_stamp;
  MercatorExtent extent;
};
struct DictionaryEntry {
  std::uint16_t tag{};
  std::string text;
};
struct Metadata {
  std::string format, chart_id, title, vendor, attribution;
  std::vector<std::pair<std::uint16_t, std::string>> chart_refs;
  std::vector<DictionaryEntry> dictionary_entries;
};
struct BlockInfo {
  std::uint16_t tag{};
  std::uint32_t offset{};
  std::uint32_t payload_offset{};
  std::uint32_t length{};
  std::uint32_t end_offset{};
};
enum class Confidence { Unsupported, Partial, StructuralConfirmed };
struct Validation {
  Confidence confidence{Confidence::Unsupported};
  bool magic_ok{}, signature_ok{}, declared_size_ok{}, edition_stamp_ok{}, extent_ok{}, metadata_ok{}, dictionary_block_ok{}, dictionary_entries_ok{};
  std::vector<std::string> warnings;
};
struct Document {
  Header header;
  Metadata metadata;
  std::optional<BlockInfo> dictionary_block;
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
