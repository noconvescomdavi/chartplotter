#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace estibordo::nv2 {

struct NavSdkCapabilities {
  bool loaded{false};
  bool has_init{false};
  bool has_controller{false};
  bool has_mount{false};
  bool has_projection{false};
  bool has_query{false};
  bool has_draw_callback{false};
  bool has_tides_currents{false};
  std::string diagnostic;
};

// Optional interoperability bridge for a user-installed/licensed ScanNav/Navionics
// runtime. The Estibordo project does not ship or modify proprietary Navionics
// binaries and never attempts to bypass chart activation or licensing.
class NavSdkBridge {
 public:
  NavSdkBridge();
  ~NavSdkBridge();
  NavSdkBridge(const NavSdkBridge&) = delete;
  NavSdkBridge& operator=(const NavSdkBridge&) = delete;

  bool Load(const std::filesystem::path& dll_path);
  void Unload();
  bool IsLoaded() const;
  const NavSdkCapabilities& Capabilities() const;
  std::vector<std::string> MissingRequiredExports() const;

 private:
  void* module_{nullptr};
  NavSdkCapabilities capabilities_;
  std::map<std::string, void*> exports_;
};

}  // namespace estibordo::nv2
