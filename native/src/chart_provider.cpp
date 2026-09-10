#include "chart_provider.h"
#include "kap_chart.h"

#include <algorithm>

std::unique_ptr<IChartProvider> OpenChartFile(const std::filesystem::path& path,
                                              std::wstring& error) {
  auto ext = path.extension().wstring();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
  if (ext == L".kap") {
    return KapChart::Open(path, error);
  }
  error = L"Formato ainda nao suportado nesta build: " + ext;
  return {};
}
