#include "nv2_chart.h"

#include <wx/dcmemory.h>
#include <wx/filename.h>

wxIMPLEMENT_DYNAMIC_CLASS(EstibordoNv2Chart, PlugInChartBase);

EstibordoNv2Chart::EstibordoNv2Chart() {
  m_ChartType = PI_CHART_TYPE_PLUGIN;
  m_ChartFamily = PI_CHART_FAMILY_VECTOR;
  m_projection = PI_PROJECTION_MERCATOR;
  m_Chart_Scale = 100000;
  m_Chart_Skew = 0.0;
  m_DepthUnits = "Meters";
  m_datum_str = "WGS84";
  m_bReadyToRender = false;
}

wxString EstibordoNv2Chart::GetFileSearchMask() { return "*.nv2"; }

int EstibordoNv2Chart::Init(const wxString& full_path, int init_flags) {
  m_FullPath = full_path;
  open_result_ = backend_manager_.Open(std::filesystem::path(full_path.ToStdWstring()));
  if (!open_result_.ok) return PI_INIT_FAIL_REMOVE;

  m_Name = wxString::FromUTF8(open_result_.document.metadata.title);
  m_Description = "Navionics NV2 / Estibordo interoperability adapter";
  m_ID = wxString::FromUTF8(open_result_.document.metadata.chart_id);
  m_SE = wxString::FromUTF8(open_result_.document.header.edition_stamp);

  // Header-only initialization is safe and allows OpenCPN to index the chart.
  // Full rendering intentionally remains fail-closed until either a validated
  // legacy feature profile or a validated NavSDK ABI renderer is active.
  if (init_flags != PI_FULL_INIT) {
    m_bReadyToRender = false;
    return PI_INIT_OK;
  }

  m_bReadyToRender = false;
  return PI_INIT_FAIL_RETRY;
}

bool EstibordoNv2Chart::GetChartExtent(ExtentPI* pext) {
  if (!pext || !open_result_.ok) return false;
  pext->WLON = open_result_.extent.west;
  pext->SLAT = open_result_.extent.south;
  pext->ELON = open_result_.extent.east;
  pext->NLAT = open_result_.extent.north;
  return true;
}

wxBitmap& EstibordoNv2Chart::RenderRegionView(const PlugIn_ViewPort& vp,
                                               const wxRegion&) {
  const int width = vp.pix_width > 0 ? vp.pix_width : 1;
  const int height = vp.pix_height > 0 ? vp.pix_height : 1;
  if (!bitmap_.IsOk() || bitmap_.GetWidth() != width || bitmap_.GetHeight() != height)
    bitmap_ = wxBitmap(width, height, 32);

  wxMemoryDC dc(bitmap_);
  dc.SetBackground(*wxBLACK_BRUSH);
  dc.Clear();
  dc.SetTextForeground(*wxWHITE);
  dc.DrawText("NV2 decoder: rendering gated until validation passes", 12, 12);
  dc.SelectObject(wxNullBitmap);
  return bitmap_;
}

void EstibordoNv2Chart::GetValidCanvasRegion(const PlugIn_ViewPort& vp,
                                              wxRegion* valid_region) {
  if (valid_region) *valid_region = wxRegion(0, 0, vp.pix_width, vp.pix_height);
}
