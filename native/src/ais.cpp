#include "ais.h"

#include "nmea_parser.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {
std::vector<std::string_view> Split(std::string_view s, char delimiter) {
  std::vector<std::string_view> out;
  size_t start = 0;
  while (start <= s.size()) {
    size_t p = s.find(delimiter, start);
    if (p == std::string_view::npos) p = s.size();
    out.push_back(s.substr(start, p - start));
    if (p == s.size()) break;
    start = p + 1;
  }
  return out;
}

int SixBit(char c) {
  int v = static_cast<unsigned char>(c) - 48;
  if (v > 40) v -= 8;
  return (v >= 0 && v < 64) ? v : -1;
}

class BitView {
 public:
  explicit BitView(std::string_view payload) : payload_(payload) {}

  bool Has(size_t pos, size_t len) const { return pos + len <= payload_.size() * 6; }

  std::uint32_t Unsigned(size_t pos, size_t len) const {
    std::uint32_t out = 0;
    for (size_t i = 0; i < len; ++i) {
      const size_t bit = pos + i;
      const int v = SixBit(payload_[bit / 6]);
      const int shift = 5 - static_cast<int>(bit % 6);
      out = (out << 1) | static_cast<std::uint32_t>((v >> shift) & 1);
    }
    return out;
  }

  std::int32_t Signed(size_t pos, size_t len) const {
    const std::uint32_t u = Unsigned(pos, len);
    if (len == 0 || len >= 32) return static_cast<std::int32_t>(u);
    const std::uint32_t sign = 1u << (len - 1);
    if ((u & sign) == 0) return static_cast<std::int32_t>(u);
    const std::uint32_t mask = ~((1u << len) - 1u);
    return static_cast<std::int32_t>(u | mask);
  }

 private:
  std::string_view payload_;
};

bool PlausiblePosition(double lat, double lon) {
  return std::isfinite(lat) && std::isfinite(lon) &&
         lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}
}  // namespace

void AisStore::Upsert(const AisTarget& target) {
  if (!target.mmsi) return;
  std::scoped_lock lock(mutex_);
  targets_[target.mmsi] = target;
}

std::vector<AisTarget> AisStore::Snapshot() const {
  std::scoped_lock lock(mutex_);
  std::vector<AisTarget> out;
  out.reserve(targets_.size());
  for (const auto& [_, target] : targets_) out.push_back(target);
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.mmsi < b.mmsi; });
  return out;
}

void AisStore::Clear() {
  std::scoped_lock lock(mutex_);
  targets_.clear();
}

std::optional<AisTarget> ParseAivdmPosition(std::string_view sentence) {
  if (sentence.size() < 8 || sentence.front() != '!') return std::nullopt;
  if (!NmeaChecksumValid(sentence)) return std::nullopt;

  const size_t star = sentence.find('*');
  const auto body = star == std::string_view::npos ? sentence : sentence.substr(0, star);
  const auto f = Split(body, ',');
  if (f.size() < 7) return std::nullopt;
  if (f[0].find("VDM") == std::string_view::npos && f[0].find("VDO") == std::string_view::npos) return std::nullopt;
  if (f[1] != "1" || f[2] != "1") return std::nullopt; // multi-fragment static data handled later

  const std::string_view payload = f[5];
  if (payload.empty()) return std::nullopt;
  BitView bits(payload);
  if (!bits.Has(0, 38)) return std::nullopt;

  const int type = static_cast<int>(bits.Unsigned(0, 6));
  AisTarget t;
  t.message_type = type;
  t.mmsi = bits.Unsigned(8, 30);

  if (type == 1 || type == 2 || type == 3) {
    if (!bits.Has(0, 137)) return std::nullopt;
    const auto sog_raw = bits.Unsigned(50, 10);
    const auto lon_raw = bits.Signed(61, 28);
    const auto lat_raw = bits.Signed(89, 27);
    const auto cog_raw = bits.Unsigned(116, 12);
    const auto hdg_raw = bits.Unsigned(128, 9);

    t.sog_kn = sog_raw < 1023 ? sog_raw / 10.0 : 0.0;
    t.lon = lon_raw / 600000.0;
    t.lat = lat_raw / 600000.0;
    t.cog_deg = cog_raw < 3600 ? cog_raw / 10.0 : 0.0;
    t.heading = hdg_raw < 360 ? static_cast<int>(hdg_raw) : -1;
    t.position_valid = PlausiblePosition(t.lat, t.lon) &&
                       std::abs(t.lon) < 181.0 && std::abs(t.lat) < 91.0;
    return t;
  }

  if (type == 18) {
    if (!bits.Has(0, 133)) return std::nullopt;
    const auto sog_raw = bits.Unsigned(46, 10);
    const auto lon_raw = bits.Signed(57, 28);
    const auto lat_raw = bits.Signed(85, 27);
    const auto cog_raw = bits.Unsigned(112, 12);
    const auto hdg_raw = bits.Unsigned(124, 9);

    t.sog_kn = sog_raw < 1023 ? sog_raw / 10.0 : 0.0;
    t.lon = lon_raw / 600000.0;
    t.lat = lat_raw / 600000.0;
    t.cog_deg = cog_raw < 3600 ? cog_raw / 10.0 : 0.0;
    t.heading = hdg_raw < 360 ? static_cast<int>(hdg_raw) : -1;
    t.position_valid = PlausiblePosition(t.lat, t.lon) &&
                       std::abs(t.lon) < 181.0 && std::abs(t.lat) < 91.0;
    return t;
  }

  return std::nullopt;
}
