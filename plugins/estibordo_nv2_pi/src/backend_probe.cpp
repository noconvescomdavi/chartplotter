#include "backend_manager.h"

#include <iostream>

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    std::cerr << "usage: nv2backendprobe <chart.nv2> [NavSDKDll.dll]\n";
    return 2;
  }

  estibordo::nv2::BackendManager manager;
  if (argc == 3) manager.SetNavSdkRuntime(argv[2]);

  const auto result = manager.Open(argv[1]);
  std::cout << "ok=" << (result.ok ? "true" : "false") << "\n"
            << "backend=" << estibordo::nv2::ToString(result.backend) << "\n"
            << "confidence="
            << estibordo::nv2::ToString(result.document.validation.confidence)
            << "\n"
            << "chart_id=" << result.document.metadata.chart_id << "\n"
            << "title=" << result.document.metadata.title << "\n"
            << "bbox=" << result.extent.west << ',' << result.extent.south << ','
            << result.extent.east << ',' << result.extent.north << "\n"
            << "diagnostic=" << result.diagnostic << "\n";
  return result.ok ? 0 : 1;
}
