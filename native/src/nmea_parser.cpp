#include "nmea_parser.h"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
std::vector<std::string_view> Split(std::string_view s, char delimiter) {
  std::vector<std::string_view> out;
  size_t start = 0;
  while (start <= s.size()) {
    size_t pos = s.find(delimiter, start);
    if (pos == std::string_view::npos) pos = s.size();
    out.push_back(s.substr(start, pos - start));
    if (pos == s.size()) break;
    start = pos + 1;
  }
  return out;
}

double ParseDouble(std::string_view s) {
  if (s.empty()) return 0.0;
  std::string tmp(s);
  return std::strtod(tmp.c_str(), nullptr);
}

int ParseInt(std::string_view s) {
  if (s.empty()) return 0;
  int value = 0;
  std::from_chars(s.data(), s.data() + s.size(), value);
  return value;
}

double NmeaCoord(std::string_view raw, std::string_view hemi) {
  const double v = ParseDouble(raw);
  const double deg = std::floor(v / 100.0);
  const double minutes = v - deg * 100.0;
  double decimal = deg + minutes / 60.0;
  if (hemi == "S" || hemi == "W") decimal = -decimal;
  return decimal;
}

std::string_view StripChecksum(std::string_view s) {
  const size_t p = s.find('*');
  return p == std::string_view::npos ? s : s.substr(0, p);
}
}  // namespace

bool NmeaChecksumValid(std::string_view sentence) {
  if (sentence.empty() || (sentence.front() != '
  const size_t star = sentence.find('*');
  if (star == std::string_view::npos || star + 2 >= sentence.size()) return true;

  unsigned char sum = 0;
  for (size_t i = 1; i < star; ++i) sum ^= static_cast<unsigned char>(sentence[i]);

  const auto hex = sentence.substr(star + 1, 2);
  unsigned int expected = 0;
  for (char c : hex) {
    expected <<= 4;
    if (c >= '0' && c <= '9') expected |= static_cast<unsigned>(c - '0');
    else if (c >= 'A' && c <= 'F') expected |= static_cast<unsigned>(c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') expected |= static_cast<unsigned>(c - 'a' + 10);
    else return false;
  }
  return sum == expected;
}

std::optional<RmcMessage> ParseRmc(std::string_view sentence) {
  if (!NmeaChecksumValid(sentence)) return std::nullopt;
  const auto fields = Split(StripChecksum(sentence), ',');
  if (fields.size() < 9) return std::nullopt;
  if (fields[0].find("RMC") == std::string_view::npos) return std::nullopt;

  RmcMessage m;
  m.valid = fields[2] == "A";
  if (!m.valid) return m;
  m.lat = NmeaCoord(fields[3], fields[4]);
  m.lon = NmeaCoord(fields[5], fields[6]);
  m.sog_kn = ParseDouble(fields[7]);
  m.cog_deg = ParseDouble(fields[8]);
  return m;
}

std::optional<GgaMessage> ParseGga(std::string_view sentence) {
  if (!NmeaChecksumValid(sentence)) return std::nullopt;
  const auto fields = Split(StripChecksum(sentence), ',');
  if (fields.size() < 8) return std::nullopt;
  if (fields[0].find("GGA") == std::string_view::npos) return std::nullopt;

  GgaMessage m;
  m.valid = ParseInt(fields[6]) > 0;
  if (!m.valid) return m;
  m.lat = NmeaCoord(fields[2], fields[3]);
  m.lon = NmeaCoord(fields[4], fields[5]);
  m.satellites = ParseInt(fields[7]);
  return m;
}
 && sentence.front() != '!')) return false;
  const size_t star = sentence.find('*');
  if (star == std::string_view::npos || star + 2 >= sentence.size()) return true;

  unsigned char sum = 0;
  for (size_t i = 1; i < star; ++i) sum ^= static_cast<unsigned char>(sentence[i]);

  const auto hex = sentence.substr(star + 1, 2);
  unsigned int expected = 0;
  for (char c : hex) {
    expected <<= 4;
    if (c >= '0' && c <= '9') expected |= static_cast<unsigned>(c - '0');
    else if (c >= 'A' && c <= 'F') expected |= static_cast<unsigned>(c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') expected |= static_cast<unsigned>(c - 'a' + 10);
    else return false;
  }
  return sum == expected;
}

std::optional<RmcMessage> ParseRmc(std::string_view sentence) {
  if (!NmeaChecksumValid(sentence)) return std::nullopt;
  const auto fields = Split(StripChecksum(sentence), ',');
  if (fields.size() < 9) return std::nullopt;
  if (fields[0].find("RMC") == std::string_view::npos) return std::nullopt;

  RmcMessage m;
  m.valid = fields[2] == "A";
  if (!m.valid) return m;
  m.lat = NmeaCoord(fields[3], fields[4]);
  m.lon = NmeaCoord(fields[5], fields[6]);
  m.sog_kn = ParseDouble(fields[7]);
  m.cog_deg = ParseDouble(fields[8]);
  return m;
}

std::optional<GgaMessage> ParseGga(std::string_view sentence) {
  if (!NmeaChecksumValid(sentence)) return std::nullopt;
  const auto fields = Split(StripChecksum(sentence), ',');
  if (fields.size() < 8) return std::nullopt;
  if (fields[0].find("GGA") == std::string_view::npos) return std::nullopt;

  GgaMessage m;
  m.valid = ParseInt(fields[6]) > 0;
  if (!m.valid) return m;
  m.lat = NmeaCoord(fields[2], fields[3]);
  m.lon = NmeaCoord(fields[4], fields[5]);
  m.satellites = ParseInt(fields[7]);
  return m;
}
