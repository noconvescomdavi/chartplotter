#pragma once
#include <optional>
#include <string_view>

struct RmcMessage {
  bool valid{};
  double lat{};
  double lon{};
  double sog_kn{};
  double cog_deg{};
};

struct GgaMessage {
  bool valid{};
  double lat{};
  double lon{};
  int satellites{};
};

bool NmeaChecksumValid(std::string_view sentence);
std::optional<RmcMessage> ParseRmc(std::string_view sentence);
std::optional<GgaMessage> ParseGga(std::string_view sentence);
