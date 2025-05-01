#include <atomic>
#include <cassert>
#include <cerrno>
#include <climits>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
namespace xigua::coro {
#ifdef __linux__
class lightweight_manual_reset_event {
public:
  void wait() {
    int old_value = value_.load(std::memory_order_acquire);
    while (old_value == 0) {
      int ret = syscall(SYS_futex, &value_, FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
                        old_value, nullptr, nullptr, 0);
      if (ret == -1) {
        if (errno == EAGAIN)
          return;
      }
      old_value = value_.load(std::memory_order_acquire);
    } // 可能出现以外唤醒，所以需要while循环判断
  }
  void set() {
    value_.store(1, std::memory_order_release);
    syscall(SYS_futex, &value_, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT_MAX,
            nullptr, nullptr, 0);
  }
  void reset() { value_.store(0, std::memory_order_relaxed); }

private:
  std::atomic<int> value_{0};
};
#else
class lightweight_manlightweight_manual_reset_event {};
#endif

} // namespace xigua::coro
