#pragma once
#include <concepts>
#include <coroutine>
#include <type_traits>
// template <typename T> struct is_coroutine_handle : std::false_type {};
// template <typename Promise>
// struct is_coroutine_handle<std::coroutine_handle<Promise>> : std::true_type
// {};

// template <typename T>
// struct is_valid_await_suspend_return_value
//     : std::disjunction<std::is_void<T>, std::is_same<T, bool>,
//                        is_coroutine_handle<T>> {};
// std::disjunction 编译期逻辑或

// 监测是否是std::coroutine_handle 类型
template <typename T>
concept IsCoroutineHandle = requires {
  std::is_same_v<T, std::coroutine_handle<>> ||
      std::is_base_of_v<std::coroutine_handle<>, T>;
};

// 监测await_suspend 的返回值是否有效
template <typename T>
concept IsValidAwaitSuspendReturnValue =
    std::is_void_v<T> || std::is_same_v<T, bool> || IsCoroutineHandle<T>;

// 监测是否是一个有效的awaiter
template <typename T>
concept Awaiter = requires(T t, std::coroutine_handle<> h) {
  // 返回值可以转换成bool类型
  { t.await_ready() } -> std::convertible_to<bool>;
  { t.await_suspend(h) } -> IsValidAwaitSuspendReturnValue;
  { t.await_resume() };
};

// 检测是否存在 operator co_await(obj)
template <typename T>
concept HasMemberCoAwait = requires(T t) {
  { t.operator co_await() };
};

template <typename T>
concept HasFreeCoAwait = requires(T t) {
  { operator co_await(t) };
};

// 检测是否可以转换成 Awaiter（即能 `co_await`）
template <typename T>
concept Awaitable = requires(T t) {
  requires Awaiter<T> || Awaiter<decltype(t.operator co_await())> ||
               Awaiter<decltype(operator co_await(t))>;
};