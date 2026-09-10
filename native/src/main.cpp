#include <windows.h>
#include <windowsx.h>\n#include <commdlg.h>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "chart_provider.h"\n#include "navigation_state.h"
#include "udp_nmea_receiver.h"

namespace {
constexpr wchar_t kClassName[] = L"EstibordoNavigatorNativeWindow";
constexpr double kPi = 3.14159265358979323846;
constexpr int kSidebar = 112;
constexpr int kTopbar = 54;
constexpr int kBottom = 52;

NavigationState g_nav;
std::unique_ptr<UdpNmeaReceiver> g_receiver;\nstd::unique_ptr<IChartProvider> g_chart;\nstd::wstring g_chart_error;

double g_center_lat = -22.90;
double g_center_lon = -43.16;
double g_zoom = 5500.0;
bool g_follow = false;
bool g_route_mode = false;
bool g_dragging = false;
POINT g_drag_start{};
double g_drag_lat{}, g_drag_lon{};

double ClampLat(double lat) {
  if (lat > 85.0) return 85.0;
  if (lat < -85.0) return -85.0;
  return lat;
}

POINT GeoToScreen(HWND hwnd, double lat, double lon) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  const int left = kSidebar;
  const int top = kTopbar;
  const int right = rc.right;
  const int bottom = rc.bottom - kBottom;
  const double width = static_cast<double>(right - left);
  const double height = static_cast<double>(bottom - top);

  const double x = (lon - g_center_lon) * std::cos(g_center_lat * kPi / 180.0) * g_zoom;
  const double y = (lat - g_center_lat) * -g_zoom;
  return {
      left + static_cast<LONG>(width / 2.0 + x),
      top + static_cast<LONG>(height / 2.0 + y)};
}

GeoPoint ScreenToGeo(HWND hwnd, int x, int y) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  const int left = kSidebar;
  const int top = kTopbar;
  const int right = rc.right;
  const int bottom = rc.bottom - kBottom;
  const double width = static_cast<double>(right - left);
  const double height = static_cast<double>(bottom - top);

  const double dx = x - (left + width / 2.0);
  const double dy = y - (top + height / 2.0);
  const double c = std::cos(g_center_lat * kPi / 180.0);
  return {
      ClampLat(g_center_lat - dy / g_zoom),
      g_center_lon + dx / (g_zoom * (std::abs(c) < 1e-6 ? 1.0 : c))};
}

void Fill(HDC dc, RECT r, COLORREF color) {
  HBRUSH b = CreateSolidBrush(color);
  FillRect(dc, &r, b);
  DeleteObject(b);
}

void DrawTextAt(HDC dc, int x, int y, const std::wstring& text, COLORREF color, int size = 18, bool bold = false) {
  LOGFONTW lf{};
  lf.lfHeight = -size;
  lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
  wcscpy_s(lf.lfFaceName, L"Segoe UI");
  HFONT f = CreateFontIndirectW(&lf);
  HFONT old = static_cast<HFONT>(SelectObject(dc, f));
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, color);
  TextOutW(dc, x, y, text.c_str(), static_cast<int>(text.size()));
  SelectObject(dc, old);
  DeleteObject(f);
}

void DrawButton(HDC dc, RECT r, const wchar_t* text, bool active = false) {
  Fill(dc, r, active ? RGB(36, 99, 160) : RGB(20, 31, 45));
  FrameRect(dc, &r, static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
  DrawTextAt(dc, r.left + 12, r.top + 13, text, RGB(235, 241, 247), 17, active);
}

void FitChartToViewport(HWND hwnd) {
  if (!g_chart) return;
  RECT rc{};
  GetClientRect(hwnd, &rc);
  const auto e = g_chart->Extent();
  g_center_lat = (e.north + e.south) * 0.5;
  g_center_lon = (e.east + e.west) * 0.5;
  const double lat_span = std::max(0.0001, e.north - e.south);
  const double lon_span = std::max(0.0001, (e.east - e.west) * std::cos(g_center_lat * kPi / 180.0));
  const double usable_w = std::max(100, rc.right - kSidebar - 40);
  const double usable_h = std::max(100, rc.bottom - kTopbar - kBottom - 40);
  g_zoom = std::min(usable_w / lon_span, usable_h / lat_span);
  g_zoom = std::clamp(g_zoom, 200.0, 500000.0);
}

void OpenChartDialog(HWND hwnd) {
  wchar_t file[MAX_PATH] = {};
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = file;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrFilter = L"BSB/KAP Nautical Charts (*.kap)\0*.kap\0All files (*.*)\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = L"kap";

  if (!GetOpenFileNameW(&ofn)) return;

  std::wstring error;
  auto chart = OpenChartFile(file, error);
  if (!chart) {
    g_chart_error = error;
    MessageBoxW(hwnd, error.c_str(), L"Estibordo Navigator - Chart Error", MB_OK | MB_ICONERROR);
    return;
  }

  g_chart = std::move(chart);
  g_chart_error.clear();
  g_follow = false;
  FitChartToViewport(hwnd);
  InvalidateRect(hwnd, nullptr, TRUE);
}

void DrawGrid(HWND hwnd, HDC dc, RECT chart) {
  HPEN pen = CreatePen(PS_SOLID, 1, RGB(41, 58, 72));
  HPEN old = static_cast<HPEN>(SelectObject(dc, pen));
  for (int x = chart.left; x < chart.right; x += 80) {
    MoveToEx(dc, x, chart.top, nullptr);
    LineTo(dc, x, chart.bottom);
  }
  for (int y = chart.top; y < chart.bottom; y += 80) {
    MoveToEx(dc, chart.left, y, nullptr);
    LineTo(dc, chart.right, y);
  }
  SelectObject(dc, old);
  DeleteObject(pen);

  auto route = g_nav.Route();
  if (route.size() > 1) {
    HPEN routePen = CreatePen(PS_SOLID, 3, RGB(255, 190, 55));
    old = static_cast<HPEN>(SelectObject(dc, routePen));
    POINT p = GeoToScreen(hwnd, route.front().lat, route.front().lon);
    MoveToEx(dc, p.x, p.y, nullptr);
    for (size_t i = 1; i < route.size(); ++i) {
      p = GeoToScreen(hwnd, route[i].lat, route[i].lon);
      LineTo(dc, p.x, p.y);
    }
    SelectObject(dc, old);
    DeleteObject(routePen);
  }

  for (size_t i = 0; i < route.size(); ++i) {
    POINT p = GeoToScreen(hwnd, route[i].lat, route[i].lon);
    HBRUSH b = CreateSolidBrush(RGB(255, 190, 55));
    HBRUSH oldb = static_cast<HBRUSH>(SelectObject(dc, b));
    Ellipse(dc, p.x - 6, p.y - 6, p.x + 6, p.y + 6);
    SelectObject(dc, oldb);
    DeleteObject(b);
    DrawTextAt(dc, p.x + 9, p.y - 10, L"WP" + std::to_wstring(i + 1), RGB(255, 220, 140), 14);
  }

  const auto nav = g_nav.Snapshot();
  if (nav.gps_valid) {
    POINT p = GeoToScreen(hwnd, nav.lat, nav.lon);
    HPEN shipPen = CreatePen(PS_SOLID, 3, RGB(50, 220, 120));
    old = static_cast<HPEN>(SelectObject(dc, shipPen));
    MoveToEx(dc, p.x, p.y - 13, nullptr);
    LineTo(dc, p.x - 9, p.y + 10);
    LineTo(dc, p.x, p.y + 6);
    LineTo(dc, p.x + 9, p.y + 10);
    LineTo(dc, p.x, p.y - 13);
    SelectObject(dc, old);
    DeleteObject(shipPen);
  }
}

void Paint(HWND hwnd) {
  PAINTSTRUCT ps{};
  HDC dc = BeginPaint(hwnd, &ps);
  RECT rc{};
  GetClientRect(hwnd, &rc);

  Fill(dc, rc, RGB(9, 17, 26));

  RECT top{0, 0, rc.right, kTopbar};
  Fill(dc, top, RGB(10, 23, 34));
  DrawTextAt(dc, 18, 15, L"ESTIBORDO NAVIGATOR", RGB(240, 247, 252), 22, true);

  const auto nav = g_nav.Snapshot();
  const std::wstring gps = nav.gps_valid ? L"GPS  ●" : L"GPS  ○";
  DrawTextAt(dc, rc.right - 310, 16, gps, nav.gps_valid ? RGB(80, 230, 130) : RGB(230, 100, 100), 18, true);
  DrawTextAt(dc, rc.right - 190, 16, L"NMEA UDP 10110", RGB(165, 185, 201), 15);

  RECT sidebar{0, kTopbar, kSidebar, rc.bottom - kBottom};
  Fill(dc, sidebar, RGB(12, 24, 35));
  int by = kTopbar + 10;
  RECT r1{8, by, kSidebar - 8, by + 48}; DrawButton(dc, r1, L"CHARTS"); by += 56;
  RECT r2{8, by, kSidebar - 8, by + 48}; DrawButton(dc, r2, L"ROUTE", g_route_mode); by += 56;
  RECT r3{8, by, kSidebar - 8, by + 48}; DrawButton(dc, r3, L"FOLLOW", g_follow); by += 56;
  RECT r4{8, by, kSidebar - 8, by + 48}; DrawButton(dc, r4, L"CLEAR"); by += 56;
  RECT r5{8, by, kSidebar - 8, by + 48}; DrawButton(dc, r5, L"+ ZOOM"); by += 56;
  RECT r6{8, by, kSidebar - 8, by + 48}; DrawButton(dc, r6, L"- ZOOM");

  RECT chart{kSidebar, kTopbar, rc.right, rc.bottom - kBottom};
  Fill(dc, chart, RGB(13, 28, 39));
  if (g_chart) {
    g_chart->Render(dc, chart, g_center_lat, g_center_lon, g_zoom);
  }
  DrawGrid(hwnd, dc, chart);

  if (g_chart) {
    DrawTextAt(dc, chart.left + 18, chart.top + 14,
               std::wstring(L"CHART: ") + g_chart->Title() + L"  [" + g_chart->FormatName() + L"]",
               RGB(240, 247, 252), 15, true);
  } else {
    DrawTextAt(dc, chart.left + 18, chart.top + 14,
               L"Native chart canvas — clique CHARTS para abrir um .KAP",
               RGB(152, 176, 194), 15);
  }

  RECT bottom{0, rc.bottom - kBottom, rc.right, rc.bottom};
  Fill(dc, bottom, RGB(10, 23, 34));
  wchar_t status[512];
  if (nav.gps_valid) {
    swprintf_s(status, L"LAT %.5f   LON %.5f   SOG %.1f kn   COG %.1f°   SAT %d   ZOOM %.0f",
               nav.lat, nav.lon, nav.sog_kn, nav.cog_deg, nav.satellites, g_zoom);
  } else {
    swprintf_s(status, L"GPS aguardando NMEA   |   Centro %.5f, %.5f   |   ZOOM %.0f",
               g_center_lat, g_center_lon, g_zoom);
  }
  DrawTextAt(dc, 18, bottom.top + 15, status, RGB(220, 231, 239), 17);

  EndPaint(hwnd, &ps);
}

bool Hit(int x, int y, int index) {
  const int top = kTopbar + 10 + index * 56;
  return x >= 8 && x <= kSidebar - 8 && y >= top && y <= top + 48;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_CREATE:
      SetTimer(hwnd, 1, 500, nullptr);
      return 0;

    case WM_TIMER: {
      if (g_follow) {
        const auto nav = g_nav.Snapshot();
        if (nav.gps_valid) {
          g_center_lat = nav.lat;
          g_center_lon = nav.lon;
        }
      }
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    }

    case WM_MOUSEWHEEL:
      g_zoom *= GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1.20 : 0.833333333;
      if (g_zoom < 200.0) g_zoom = 200.0;
      if (g_zoom > 500000.0) g_zoom = 500000.0;
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;

    case WM_LBUTTONDOWN: {
      const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
      if (x < kSidebar) {
        if (Hit(x, y, 0)) { OpenChartDialog(hwnd); return 0; }
        if (Hit(x, y, 1)) g_route_mode = !g_route_mode;
        else if (Hit(x, y, 2)) g_follow = !g_follow;
        else if (Hit(x, y, 3)) g_nav.ClearRoute();
        else if (Hit(x, y, 4)) g_zoom *= 1.25;
        else if (Hit(x, y, 5)) g_zoom /= 1.25;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
      }

      RECT rc{}; GetClientRect(hwnd, &rc);
      if (y >= kTopbar && y < rc.bottom - kBottom) {
        if (g_route_mode) {
          const GeoPoint p = ScreenToGeo(hwnd, x, y);
          g_nav.AddWaypoint(p.lat, p.lon);
        } else {
          g_dragging = true;
          g_drag_start = {x, y};
          g_drag_lat = g_center_lat;
          g_drag_lon = g_center_lon;
          SetCapture(hwnd);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_MOUSEMOVE:
      if (g_dragging && (wp & MK_LBUTTON)) {
        const int dx = GET_X_LPARAM(lp) - g_drag_start.x;
        const int dy = GET_Y_LPARAM(lp) - g_drag_start.y;
        const double c = std::cos(g_drag_lat * kPi / 180.0);
        g_center_lat = ClampLat(g_drag_lat + dy / g_zoom);
        g_center_lon = g_drag_lon - dx / (g_zoom * (std::abs(c) < 1e-6 ? 1.0 : c));
        g_follow = false;
        InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;

    case WM_LBUTTONUP:
      if (g_dragging) {
        g_dragging = false;
        ReleaseCapture();
      }
      return 0;

    case WM_PAINT:
      Paint(hwnd);
      return 0;

    case WM_DESTROY:
      KillTimer(hwnd, 1);
      if (g_receiver) g_receiver->Stop();
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
  g_receiver = std::make_unique<UdpNmeaReceiver>(g_nav);
  g_receiver->Start(10110);

  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = WndProc;
  wc.hInstance = instance;
  wc.lpszClassName = kClassName;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  if (!RegisterClassExW(&wc)) return 1;

  HWND hwnd = CreateWindowExW(
      0, kClassName, L"Estibordo Navigator — Native Chartplotter",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 820,
      nullptr, nullptr, instance, nullptr);
  if (!hwnd) return 2;

  ShowWindow(hwnd, show);
  UpdateWindow(hwnd);

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return static_cast<int>(msg.wParam);
}
