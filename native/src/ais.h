#pragma once
#include <cstdint>
#include <mutex>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

struct AisTarget {
  std::uint32_t mmsi{};
  int message_type{};
  bool position_valid{};
  double lat{};
  double lon{};
  double sog_kn{};
  double cog_deg{};
  int heading{-1};
};

class AisStore {
 public:
  void Upsert(const AisTarget& target);
  std::vector<AisTarget> Snapshot() const;
  void Clear();

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::uint32_t, AisTarget> targets_;
};

std::optional<AisTarget> ParseAivdmPosition(std::string_view sentence);
