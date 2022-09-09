cmake_minimum_required(VERSION 3.0)
project(toggle-turbo)

set(TURBO_USE_POWERAPI 0 CACHE BOOL "Use newer Windows PowerAPI to modify power settings.")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRCS 
    src/main.cpp
    resources/main.rc
)

add_executable(toggle-turbo ${SRCS})
target_include_directories(toggle-turbo PRIVATE include/)
target_compile_options(toggle-turbo PRIVATE -Wall -Wextra -pedantic)
target_compile_features(toggle-turbo PRIVATE cxx_std_20)

if (TURBO_USE_POWERAPI)
    target_link_libraries(toggle-turbo powrprof)
    add_compile_definitions(TURBO_USE_POWERAPI=1)
else()
    add_compile_definitions(TURBO_USE_POWERAPI=0)
endif()

target_link_options(toggle-turbo PRIVATE -static -Wl,-strip-all)
