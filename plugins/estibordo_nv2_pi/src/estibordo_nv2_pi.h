#pragma once

#include "ocpn_plugin.h"

class EstibordoNv2Pi : public opencpn_plugin_122 {
 public:
  explicit EstibordoNv2Pi(void* pmgr);
  ~EstibordoNv2Pi() override = default;

  int Init() override;
  bool DeInit() override;
  int GetAPIVersionMajor() override { return 1; }
  int GetAPIVersionMinor() override { return 22; }
  int GetPlugInVersionMajor() override { return 0; }
  int GetPlugInVersionMinor() override { return 1; }
  int GetPlugInVersionPatch() override { return 0; }
  wxString GetCommonName() override { return "Estibordo NV2"; }
  wxString GetShortDescription() override {
    return "Validated Navionics NV2 interoperability backend";
  }
  wxString GetLongDescription() override {
    return "Indexes structurally validated NV2 charts and exposes a fail-closed chart adapter. Rendering is enabled only by validated decoder profiles/backends.";
  }
  wxBitmap* GetPlugInBitmap() override { return nullptr; }
  wxArrayString GetDynamicChartClassNameArray() override;
};

extern "C" DECL_EXP opencpn_plugin* create_pi(void* ppimgr);
extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p);
