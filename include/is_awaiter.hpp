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
concept is_coroutine_handle = requires {
  std::is_same_v<T, std::coroutine_handle<>> ||
      std::is_base_of_v<std::coroutine_handle<>, T>;
};

// 监测await_suspend 的返回值是否有效
template <typename T>
concept is_valid_await_suspend_return_value =
    std::is_void_v<T> || std::is_same_v<T, bool> || is_coroutine_handle<T>;

// 监测是否是一个有效的awaiter
template <typename T>
concept is_awaiter = requires(T t, std::coroutine_handle<> h) {
  // 返回值可以转换成bool类型
  { t.await_ready() } -> std::convertible_to<bool>;
  { t.await_suspend(h) } -> is_valid_await_suspend_return_value;
  { t.await_resume() };
};