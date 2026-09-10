#include "udp_nmea_receiver.h"

#include "navigation_state.h"
#include "nmea_parser.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <string_view>

UdpNmeaReceiver::UdpNmeaReceiver(NavigationState& state) : state_(state) {}
UdpNmeaReceiver::~UdpNmeaReceiver() { Stop(); }

bool UdpNmeaReceiver::Start(unsigned short port) {
  if (running_.exchange(true)) return true;
  thread_ = std::thread([this, port] { Run(port); });
  return true;
}

void UdpNmeaReceiver::Stop() {
  running_ = false;
  if (thread_.joinable()) thread_.join();
}

void UdpNmeaReceiver::Run(unsigned short port) {
  WSADATA data{};
  if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
    running_ = false;
    return;
  }

  SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s == INVALID_SOCKET) {
    WSACleanup();
    running_ = false;
    return;
  }

  DWORD timeout = 500;
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    closesocket(s);
    WSACleanup();
    running_ = false;
    return;
  }

  char buffer[4096];
  while (running_) {
    const int n = recvfrom(s, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
    if (n <= 0) continue;
    buffer[n] = 0;
    std::string packet(buffer, static_cast<size_t>(n));

    size_t start = 0;
    while (start < packet.size()) {
      size_t end = packet.find_first_of("\r\n", start);
      if (end == std::string::npos) end = packet.size();
      std::string_view line(packet.data() + start, end - start);
      if (!line.empty()) {
        if (auto rmc = ParseRmc(line); rmc && rmc->valid) {
          state_.UpdateGps(rmc->lat, rmc->lon, rmc->sog_kn, rmc->cog_deg);
        } else if (auto gga = ParseGga(line); gga && gga->valid) {
          state_.UpdateGps(gga->lat, gga->lon, 0.0, 0.0);
          state_.UpdateSatellites(gga->satellites);
        }
      }
      start = end + 1;
    }
  }

  closesocket(s);
  WSACleanup();
}
