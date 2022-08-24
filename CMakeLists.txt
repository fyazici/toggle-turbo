cmake_minimum_required(VERSION 3.0)
project(toggle-turbo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRCS 
    src/main.cpp
    resources/main.rc
)

add_executable(toggle-turbo ${SRCS})
target_include_directories(toggle-turbo PRIVATE include/)
target_compile_options(toggle-turbo PRIVATE -Wall -Wextra)
target_compile_features(toggle-turbo PRIVATE cxx_std_17)
target_link_options(toggle-turbo PRIVATE -static -Wl,-strip-all)
