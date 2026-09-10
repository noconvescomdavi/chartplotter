#pragma once
#include <atomic>
#include <thread>

class NavigationState;
class AisStore;

class UdpNmeaReceiver {
 public:
  UdpNmeaReceiver(NavigationState& state, AisStore& ais);
  ~UdpNmeaReceiver();

  bool Start(unsigned short port = 10110);
  void Stop();

 private:
  void Run(unsigned short port);

  NavigationState& state_;
  AisStore& ais_;
  std::atomic_bool running_{false};
  std::thread thread_;
};
