#include "backend_manager.h"

#include <sstream>

namespace estibordo::nv2 {

void BackendManager::SetNavSdkRuntime(const std::filesystem::path& dll_path) {
  navsdk_path_ = dll_path;
  navsdk_.Unload();
}

OpenResult BackendManager::Open(const std::filesystem::path& nv2_path) {
  OpenResult result;
  try {
    result.document = Reader::ParseFile(nv2_path);
  } catch (const std::exception& e) {
    result.diagnostic = e.what();
    return result;
  }

  const auto& v = result.document.validation;
  if (v.confidence == Confidence::Unsupported) {
    result.diagnostic = "NV2 signature/structure not supported";
    return result;
  }

  auto sw = MercatorToLonLat(result.document.header.extent.min_x,
                             result.document.header.extent.min_y);
  auto ne = MercatorToLonLat(result.document.header.extent.max_x,
                             result.document.header.extent.max_y);
  result.extent = {sw.first, sw.second, ne.first, ne.second};

  if (navsdk_path_) {
    if (!navsdk_.IsLoaded()) navsdk_.Load(*navsdk_path_);
    if (navsdk_.IsLoaded() && navsdk_.MissingRequiredExports().empty()) {
      result.ok = true;
      result.backend = BackendKind::NavSdk;
      result.diagnostic =
          "NV2 structurally valid; compatible licensed NavSDK runtime detected";
      return result;
    }
  }

  if (v.confidence == Confidence::StructuralConfirmed) {
    result.ok = true;
    result.backend = BackendKind::LegacyStructural;
    result.diagnostic =
        "NV2 structurally confirmed; rendering remains gated by decoded feature profile";
    return result;
  }

  result.diagnostic = "NV2 partially recognized; diagnostics-only mode";
  return result;
}

const char* ToString(BackendKind kind) noexcept {
  switch (kind) {
    case BackendKind::NavSdk:
      return "NAVSDK";
    case BackendKind::LegacyStructural:
      return "LEGACY_STRUCTURAL";
    default:
      return "NONE";
  }
}

}  // namespace estibordo::nv2
