#pragma once

#include "backend_manager.h"
#include "ocpn_plugin.h"

#include <wx/bitmap.h>
#include <wx/object.h>

class EstibordoNv2Chart : public PlugInChartBase {
 public:
  wxDECLARE_DYNAMIC_CLASS(EstibordoNv2Chart);

  EstibordoNv2Chart();
  ~EstibordoNv2Chart() override = default;

  int Init(const wxString& full_path, int init_flags) override;
  wxString GetFileSearchMask() override;
  bool GetChartExtent(ExtentPI* pext) override;
  wxBitmap& RenderRegionView(const PlugIn_ViewPort& vp,
                             const wxRegion& region) override;
  void GetValidCanvasRegion(const PlugIn_ViewPort& vp,
                            wxRegion* valid_region) override;

 private:
  estibordo::nv2::BackendManager backend_manager_;
  estibordo::nv2::OpenResult open_result_;
  wxBitmap bitmap_;
};
