#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

#include "ais.h"
#include "gpx_route.h"
#include "nmea_parser.h"

namespace {
int failures = 0;
void Check(bool ok, const char* name) {
  if (!ok) { std::cerr << "FAIL: " << name << "\n"; ++failures; }
  else { std::cout << "PASS: " << name << "\n"; }
}
}

int main() {
  auto rmc = ParseRmc("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A");
  Check(rmc.has_value() && rmc->valid, "RMC valid");
  if (rmc) {
    Check(std::abs(rmc->lat - 48.1173) < 1e-4, "RMC latitude");
    Check(std::abs(rmc->lon - 11.5166667) < 1e-4, "RMC longitude");
    Check(std::abs(rmc->sog_kn - 22.4) < 1e-6, "RMC speed");
  }
  Check(!NmeaChecksumValid("$GPRMC,1,2,3*00"), "Checksum rejection");

  auto ais = ParseAivdmPosition("!AIVDM,1,1,,A,15Muq?001oJr>tpE`E>4?wvl0<0u,0*5C");
  Check(ais.has_value(), "AIS decode");

  std::vector<NavPoint> route{{-22.90,-43.16},{-22.91,-43.15},{-22.92,-43.14}};
  const auto temp = std::filesystem::temp_directory_path() / "estibordo-core-test.gpx";
  std::wstring error;
  Check(SaveRouteGpx(temp, route, error), "GPX save");
  std::vector<NavPoint> loaded;
  Check(LoadRouteGpx(temp, loaded, error), "GPX load");
  Check(loaded.size() == route.size(), "GPX point count");
  if (loaded.size() == route.size()) Check(std::abs(loaded[1].lat - route[1].lat) < 1e-8, "GPX coordinate roundtrip");
  std::error_code ec; std::filesystem::remove(temp, ec);
  return failures == 0 ? 0 : 1;
}
