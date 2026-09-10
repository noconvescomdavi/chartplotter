#pragma once

#include "navsdk_bridge.h"
#include "nv2_reader.h"

#include <filesystem>
#include <optional>
#include <string>

namespace estibordo::nv2 {

enum class BackendKind {
  None,
  NavSdk,
  LegacyStructural
};

struct GeographicExtent {
  double west{};
  double south{};
  double east{};
  double north{};
};

struct OpenResult {
  bool ok{false};
  BackendKind backend{BackendKind::None};
  Document document;
  GeographicExtent extent;
  std::string diagnostic;
};

class BackendManager {
 public:
  BackendManager() = default;

  void SetNavSdkRuntime(const std::filesystem::path& dll_path);
  OpenResult Open(const std::filesystem::path& nv2_path);

  const NavSdkBridge& NavSdk() const noexcept { return navsdk_; }

 private:
  std::optional<std::filesystem::path> navsdk_path_;
  NavSdkBridge navsdk_;
};

const char* ToString(BackendKind kind) noexcept;

}  // namespace estibordo::nv2
