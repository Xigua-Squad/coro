#pragma once
#include <atomic>
class lightweight_manual_reset_event {
public:
  lightweight_manual_reset_event(bool initiallySet = false);
  ~lightweight_manual_reset_event();
  void set() noexcept;
  void reset() noexcept;
  void wait() noexcept;

private:
  std::atomic<int> value_;
};