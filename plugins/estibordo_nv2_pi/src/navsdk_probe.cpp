#include "navsdk_bridge.h"

#include <iostream>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: navsdkprobe <path-to-NavSDKDll.dll>\n";
    return 2;
  }

  estibordo::nv2::NavSdkBridge bridge;
  const bool ok = bridge.Load(argv[1]);
  const auto& c = bridge.Capabilities();
  std::cout << "loaded=" << c.loaded << "\n"
            << "init=" << c.has_init << "\n"
            << "controller=" << c.has_controller << "\n"
            << "mount=" << c.has_mount << "\n"
            << "projection=" << c.has_projection << "\n"
            << "query=" << c.has_query << "\n"
            << "draw_callback=" << c.has_draw_callback << "\n"
            << "tides_currents=" << c.has_tides_currents << "\n"
            << "diagnostic=" << c.diagnostic << "\n";

  for (const auto& name : bridge.MissingRequiredExports())
    std::cout << "missing=" << name << "\n";
  return ok ? 0 : 1;
}
