#include "navsdk_bridge.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <array>
#include <sstream>

namespace estibordo::nv2 {

namespace {
constexpr std::array<const char*, 8> kRequiredExports = {
    "NavSDK_InitDll", "NavSDK_CreateController", "NavSDK_CloseController",
    "NavSDK_MountChart", "NavSDK_UnmountChart", "NavSDK_SetGeoPos",
    "NavSDK_PositionToPix", "NavSDK_PixToPosition"};

constexpr std::array<const char*, 8> kQueryExports = {
    "NavSDK_QueryObjects", "NavSDK_GetObjectURI", "NavSDK_GetObjectByURI",
    "NavSDK_GetObjectAttributes", "NavSDK_ReleaseObjectAttributes",
    "NavSDK_ReleaseObjects", "NavSDK_QuickSearchPixTxt",
    "NavSDK_QuickSearchPosTxt"};

constexpr std::array<const char*, 4> kTideCurrentExports = {
    "NavSDK_GetTideInfo", "NavSDK_GetTidesPeriod", "NavSDK_GetCurrentsInfo",
    "NavSDK_GetCurrentsPeriod"};
}  // namespace

NavSdkBridge::NavSdkBridge() = default;
NavSdkBridge::~NavSdkBridge() { Unload(); }

bool NavSdkBridge::Load(const std::filesystem::path& dll_path) {
  Unload();
#ifdef _WIN32
  HMODULE mod = LoadLibraryW(dll_path.wstring().c_str());
  if (!mod) {
    capabilities_.diagnostic = "LoadLibraryW failed";
    return false;
  }
  module_ = reinterpret_cast<void*>(mod);

  auto resolve = [&](const char* name) -> void* {
    FARPROC p = GetProcAddress(mod, name);
    if (p) exports_[name] = reinterpret_cast<void*>(p);
    return reinterpret_cast<void*>(p);
  };

  for (const auto* name : kRequiredExports) resolve(name);
  for (const auto* name : kQueryExports) resolve(name);
  for (const auto* name : kTideCurrentExports) resolve(name);
  resolve("NavSDK_GetMountedCharts");
  resolve("NavSDK_ReleaseMountInfo");
  resolve("NavSDK_SetGeoRect");
  resolve("NavSDK_ResizeView");
  resolve("NavSDK_Zoom");
  resolve("NavSDK_Scroll");
  resolve("NavSDK_SetOnDrawCallBack");
  resolve("NavSDK_SetOnNotificationCallBack");
  resolve("NavSDK_GetDepthUnit");
  resolve("NavSDK_OpenSettingsDial");

  capabilities_.loaded = true;
  capabilities_.has_init = exports_.count("NavSDK_InitDll") != 0;
  capabilities_.has_controller =
      exports_.count("NavSDK_CreateController") &&
      exports_.count("NavSDK_CloseController");
  capabilities_.has_mount = exports_.count("NavSDK_MountChart") &&
                            exports_.count("NavSDK_UnmountChart");
  capabilities_.has_projection =
      exports_.count("NavSDK_PositionToPix") &&
      exports_.count("NavSDK_PixToPosition") && exports_.count("NavSDK_SetGeoPos");

  capabilities_.has_query = true;
  for (const auto* name : kQueryExports)
    capabilities_.has_query &= exports_.count(name) != 0;

  capabilities_.has_draw_callback =
      exports_.count("NavSDK_SetOnDrawCallBack") != 0;

  capabilities_.has_tides_currents = true;
  for (const auto* name : kTideCurrentExports)
    capabilities_.has_tides_currents &= exports_.count(name) != 0;

  const auto missing = MissingRequiredExports();
  if (!missing.empty()) {
    std::ostringstream os;
    os << "NavSDK runtime loaded, but required exports are missing:";
    for (const auto& name : missing) os << ' ' << name;
    capabilities_.diagnostic = os.str();
    return false;
  }

  capabilities_.diagnostic =
      "Compatible NavSDK export surface detected. ABI invocation remains gated "
      "until parameter contracts are validated against licensed runtime behavior.";
  return true;
#else
  (void)dll_path;
  capabilities_.diagnostic = "NavSDK bridge is Windows-only";
  return false;
#endif
}

void NavSdkBridge::Unload() {
#ifdef _WIN32
  if (module_) FreeLibrary(reinterpret_cast<HMODULE>(module_));
#endif
  module_ = nullptr;
  exports_.clear();
  capabilities_ = {};
}

bool NavSdkBridge::IsLoaded() const { return capabilities_.loaded; }

const NavSdkCapabilities& NavSdkBridge::Capabilities() const {
  return capabilities_;
}

std::vector<std::string> NavSdkBridge::MissingRequiredExports() const {
  std::vector<std::string> missing;
  for (const auto* name : kRequiredExports) {
    if (!exports_.count(name)) missing.emplace_back(name);
  }
  return missing;
}

}  // namespace estibordo::nv2
