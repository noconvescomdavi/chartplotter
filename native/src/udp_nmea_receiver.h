#pragma once
#include <atomic>
#include <thread>

class NavigationState;

class UdpNmeaReceiver {
 public:
  explicit UdpNmeaReceiver(NavigationState& state);
  ~UdpNmeaReceiver();

  bool Start(unsigned short port = 10110);
  void Stop();

 private:
  void Run(unsigned short port);

  NavigationState& state_;
  std::atomic_bool running_{false};
  std::thread thread_;
};
