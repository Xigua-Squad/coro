aux_source_directory(. tests_src)

add_executable(tests
    ${tests_src}
    ${LIB_SRCS}
)

target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(tests PRIVATE gtest_main)

# 添加测试
add_test(NAME CoroTest COMMAND tests)