#include <tuple>
namespace xigua::coro {
template <typename T> class when_all_ready_awaitable;

template <> class when_all_ready_awaitable<std::tuple<>> {
public:
  constexpr when_all_ready_awaitable(std::tuple<>) noexcept {}
};

} // namespace xigua::coro