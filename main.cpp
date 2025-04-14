#include <iostream>
#include <type_traits>
#include "task.hpp"
#include <list>
#include <thread>
#include <chrono>
#include "shared_task.hpp"
enum class State {
	Running ,
	Completed,
	NotStarted
};

struct sh_task {
public:
	

	struct promise_type {
		std::list <std::coroutine_handle<>> waiters;
		std::atomic<State> state_ = State::NotStarted;
		sh_task get_return_object() noexcept {
			return sh_task{ std::coroutine_handle<promise_type>::from_promise(*this) };
		}

		struct final_awaiter {
			bool await_ready() noexcept { return false; }
			void await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
				// 这里的handle就是本协程
				auto& promise = handle.promise();
				promise.state_ = State::Completed;
				// 可能出现，清理过程中，其他线程出现新的等待协程
				for (auto& waiter : promise.waiters) {
					waiter.resume();
				}
				promise.waiters.clear();
			}
			void await_resume() noexcept {}
		};

		auto initial_suspend() noexcept { return std::suspend_always{}; }
		auto final_suspend() noexcept { return final_awaiter{}; }
		void unhandled_exception() { std::terminate(); }
		void return_void() {
			std::cout << "shared_task 执行完成\n";
		}
	};

	struct awaiterable {
		std::coroutine_handle<promise_type> handle_;
		awaiterable(std::coroutine_handle<promise_type> h) : handle_(h) {}
		bool await_ready() const noexcept { return false; }
		void await_suspend(std::coroutine_handle<> h) const noexcept{

			if (handle_.promise().state_ == State::NotStarted)
			{
				handle_.promise().state_ = State::Running;
				handle_.resume();
			}
			else if (handle_.promise().state_ == State::Running)
			{
				handle_.promise().waiters.push_back(h);
			}
			else if (handle_.promise().state_ == State::Completed)
			{
				h.resume();
			}
	
		}
		void await_resume()const noexcept {}
	};
	auto operator co_await() const noexcept{
		return awaiterable{ handle_ };
	}

	std::coroutine_handle<promise_type> handle_;
	sh_task(std::coroutine_handle<promise_type> h) : handle_(h) {}
	~sh_task() {
		if (handle_) {
			handle_.destroy();
		}
	}
};
using namespace std::chrono_literals;
sh_task sharedTask() {
	std::cout << "shared_task 执行中\n";
	std::this_thread::sleep_for(2s);
	co_return;
}

task<int> fun1(const sh_task & sh) {
	std::this_thread::sleep_for(1s);
	std::cout << "func1 开始等待 sh 执行\n";
	co_await sh;
	std::cout << "func1 等待结束\n";
}
task<int> fun2(const sh_task& sh) {
	std::this_thread::sleep_for(1s);
	std::cout << "func2 开始等待 sh 执行\n";
	co_await sh;
	std::cout << "func2 等待结束\n";
}

task<int> test1() {
	std::cout << "test1开始执行\n";
	for (int i = 0; i < 10000; ++i)
	{
		for (int j = 0; j < 100000; ++j)
		{
			int n = i * j;
		}
	}
	std::cout << "test1执行完毕,当前执行线程:" << std::this_thread::get_id()<<"\n";
	co_return 100;
}
task<int> test2() {
	std::cout << "test2,开始在线程:" << std::this_thread::get_id() << "\n";
	co_return 100;
}
int main() {
	std::shared_ptr<int> ptr = std::make_shared<int>(8);
	ptr = ptr;
	std::cout << ptr.use_count();
	return 0;
}
// shared_task 和shared_ptr高度相似