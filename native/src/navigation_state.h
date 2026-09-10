#pragma once
#include <mutex>
#include <vector>

struct NavPoint {
  double lat{};
  double lon{};
};

struct NavSnapshot {
  bool gps_valid{};
  double lat{};
  double lon{};
  double sog_kn{};
  double cog_deg{};
  int satellites{};
};

class NavigationState {
 public:
  void UpdateGps(double lat, double lon, double sog_kn, double cog_deg);
  void UpdateSatellites(int satellites);
  NavSnapshot Snapshot() const;

  void AddWaypoint(double lat, double lon);
  void ClearRoute();
  std::vector<NavPoint> Route() const;

 private:
  mutable std::mutex mutex_;
  NavSnapshot nav_{};
  std::vector<NavPoint> route_;
};
