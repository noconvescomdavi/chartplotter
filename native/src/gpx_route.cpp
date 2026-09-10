#include "gpx_route.h"

#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

namespace {
std::string ReadAll(const std::filesystem::path& p) {
  std::ifstream f(p, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}
}

bool SaveRouteGpx(const std::filesystem::path& path,
                  const std::vector<NavPoint>& points,
                  std::wstring& error) {
  std::ofstream f(path, std::ios::binary);
  if (!f) { error=L"Nao foi possivel salvar o GPX."; return false; }

  f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  f << "<gpx version=\"1.1\" creator=\"Estibordo Navigator\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n";
  f << "  <rte><name>Estibordo Route</name>\n";
  f << std::fixed << std::setprecision(8);
  for (size_t i=0;i<points.size();++i) {
    f << "    <rtept lat=\"" << points[i].lat << "\" lon=\"" << points[i].lon << "\"><name>WP"
      << (i+1) << "</name></rtept>\n";
  }
  f << "  </rte>\n</gpx>\n";
  return static_cast<bool>(f);
}

bool LoadRouteGpx(const std::filesystem::path& path,
                  std::vector<NavPoint>& points,
                  std::wstring& error) {
  const auto xml=ReadAll(path);
  if (xml.empty()) { error=L"GPX vazio ou ilegivel."; return false; }

  static const std::regex re(R"(<(?:rtept|wpt)\b[^>]*\blat\s*=\s*["']([^"']+)["'][^>]*\blon\s*=\s*["']([^"']+)["'])",
                             std::regex::icase);
  points.clear();
  for (std::sregex_iterator it(xml.begin(),xml.end(),re),end; it!=end; ++it) {
    try {
      const double lat=std::stod((*it)[1].str());
      const double lon=std::stod((*it)[2].str());
      if (lat>=-90 && lat<=90 && lon>=-180 && lon<=180) points.push_back({lat,lon});
    } catch (...) {}
  }
  if (points.empty()) { error=L"Nenhum waypoint/route point valido encontrado no GPX."; return false; }
  return true;
}
