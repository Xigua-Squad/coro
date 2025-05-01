#include "sync_wait.hpp"
#include "task.hpp"
#include <gtest/gtest.h>
using namespace xigua;

coro::task<void> test1() {
  std::cout << "test 1" << '\n';
  co_return;
}

coro::task<int> test2() {
  std::cout << "test 2" << '\n';
  co_return 1;
}

int freeVal = 123;
coro::task<int &> test3() {
  std::cout << "test 3" << '\n';
  co_return freeVal;
}

coro::task<int> test4() {
  std::cout << "test 4" << '\n';
  co_return 1;
}
int const_val = 100;
coro::task<const int &> test5() {
  std::cout << "test 5" << '\n';
  co_return const_val;
}
TEST(SyncWaitTest, Test1) { EXPECT_NO_THROW(coro::sync_wait(test1())); }

TEST(SyncWaitTest, Test2) {
  int result = coro::sync_wait(test2());
  EXPECT_EQ(result, 1);
}

TEST(SyncWaitTest, Test3) {
  int &result = coro::sync_wait(test3());
  EXPECT_EQ(result, freeVal);
  EXPECT_EQ(result, 123);

  // Modify freeVal and ensure the reference reflects the change
  freeVal = 456;
  EXPECT_EQ(result, 456);
}

TEST(SyncWaitTest, Test4) {
  int result = coro::sync_wait(test4());
  EXPECT_EQ(result, 1);
}

TEST(SyncWaitTest, Test5) {
  decltype(auto) res = coro::sync_wait(test5());
  static_assert(std::is_same_v<decltype(res), const int &>);
  EXPECT_EQ(res, const_val);
  EXPECT_EQ(res, 100);
}