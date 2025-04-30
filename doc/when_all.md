# when_all 的实现
#### when_all的使用案例
```cpp
auto [result1, result2, result3] = co_await cppcoro::when_all(task1, task2, task3);
```
* when_all 这个函数本质上就是接收Awaitable对象，包装成一个when_all_task对象返回

#### std::add_pointer_t
```cpp
#include <type_traits>
static_assert(std::is_same_v<std::add_pointer_t<int>, int*>);
static_assert(std::is_same_v<std::add_pointer_t<int&>, int*>);
static_assert(std::is_same_v<std::add_pointer_t<void(int)>, void(*)(int)>);
```
1. **如果原来是引用类型**，它会先去掉引用，再加*
2. **如果原来是函数类型**，它会自动转成函数指针，void(int)->void(*)(int)

#### ADL(Argument-Dependent Lookup)实参依赖查找
```cpp
namespace MathLib {
    struct Vec {};

    void print(const Vec&) {
        std::cout << "print(Vec)\n";
    }
}

int main() {
    MathLib::Vec v;
    print(v); // 没有 using namespace MathLib，但能正常调用
}

```

#### fmap
`fmap`的作用是：**将一个Awaitable类型，绑定一个回调函数**形成一个新的`Awaitable类型`
```cpp
coro::task<std::string> get() { co_return "代拍"; }
coro::task<void> test() {
  auto t2 = get() | coro::fmap([](const std::string &str) {
              std::cout << "lambda 执行\n";
              std::cout << str << std::endl;
            });
  auto t3 = coro::fmap([](std::string) {}, get());
  co_await t2;
}
```