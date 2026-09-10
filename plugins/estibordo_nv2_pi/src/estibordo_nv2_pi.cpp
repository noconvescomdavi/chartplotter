#include "estibordo_nv2_pi.h"
#include "nv2_chart.h"

EstibordoNv2Pi::EstibordoNv2Pi(void* pmgr) : opencpn_plugin_122(pmgr) {}

int EstibordoNv2Pi::Init() { return INSTALLS_PLUGIN_CHART; }
bool EstibordoNv2Pi::DeInit() { return true; }

wxArrayString EstibordoNv2Pi::GetDynamicChartClassNameArray() {
  wxArrayString classes;
  classes.Add("EstibordoNv2Chart");
  return classes;
}

extern "C" DECL_EXP opencpn_plugin* create_pi(void* ppimgr) {
  return new EstibordoNv2Pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) { delete p; }
