cmake_minimum_required(VERSION 3.28.0)
project(coro)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)
aux_source_directory(${CMAKE_SOURCE_DIR}/lib LIB_SRCS)


# 添加gtest子目录
add_subdirectory(third_party/googletest)
add_subdirectory(tests)

# 添加libcoro 
# add_subdirectory(third_party/libcoro)





add_executable(coro main.cpp ${LIB_SRCS})
target_include_directories(coro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(coro PRIVATE ${HEADERS})  
target_include_directories(coro PRIVATE ${stdexec_SOURCE_DIR}/include)
# Link the libcoro cmake target to your project(s).
# target_link_libraries(coro PUBLIC libcoro)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

