#include "navigation_state.h"

void NavigationState::UpdateGps(double lat, double lon, double sog_kn, double cog_deg) {
  std::scoped_lock lock(mutex_);
  nav_.gps_valid = true;
  nav_.lat = lat;
  nav_.lon = lon;
  nav_.sog_kn = sog_kn;
  nav_.cog_deg = cog_deg;
}

void NavigationState::UpdateSatellites(int satellites) {
  std::scoped_lock lock(mutex_);
  nav_.satellites = satellites;
}

NavSnapshot NavigationState::Snapshot() const {
  std::scoped_lock lock(mutex_);
  return nav_;
}

void NavigationState::AddWaypoint(double lat, double lon) {
  std::scoped_lock lock(mutex_);
  route_.push_back({lat, lon});
}

void NavigationState::ClearRoute() {
  std::scoped_lock lock(mutex_);
  route_.clear();
}

std::vector<GeoPoint> NavigationState::Route() const {
  std::scoped_lock lock(mutex_);
  return route_;
}
