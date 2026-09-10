#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "navigation_state.h"

bool SaveRouteGpx(const std::filesystem::path& path,
                  const std::vector<NavPoint>& points,
                  std::wstring& error);

bool LoadRouteGpx(const std::filesystem::path& path,
                  std::vector<NavPoint>& points,
                  std::wstring& error);
