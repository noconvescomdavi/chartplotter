#include "nv2_reader.h"

#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: nv2probe <chart.nv2>\n";
    return 2;
  }
  try {
    auto d = estibordo::nv2::Reader::ParseFile(argv[1]);
    const auto sw = estibordo::nv2::MercatorToLonLat(d.header.extent.min_x, d.header.extent.min_y);
    const auto ne = estibordo::nv2::MercatorToLonLat(d.header.extent.max_x, d.header.extent.max_y);
    std::cout << "confidence=" << estibordo::nv2::ToString(d.validation.confidence) << "\n"
              << "chart_id=" << d.metadata.chart_id << "\n"
              << "title=" << d.metadata.title << "\n"
              << "vendor=" << d.metadata.vendor << "\n"
              << "edition=" << d.header.edition_stamp << "\n"
              << std::fixed << std::setprecision(6)
              << "bbox=" << sw.first << "," << sw.second << "," << ne.first << "," << ne.second << "\n";
    for (const auto& w : d.validation.warnings) std::cout << "warning=" << w << "\n";
    return d.validation.confidence == estibordo::nv2::Confidence::StructuralConfirmed ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << "NV2 parse failure: " << e.what() << "\n";
    return 1;
  }
}
