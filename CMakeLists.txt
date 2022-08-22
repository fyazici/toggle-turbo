cmake_minimum_required(VERSION 3.9)
project(toggle-turbo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRCS 
    src/main.cpp
    main.rc
)

add_executable(toggle-turbo ${SRCS})
target_compile_features(toggle-turbo PRIVATE cxx_std_17)
target_link_options(toggle-turbo PRIVATE -static-libgcc -static-libstdc++)
