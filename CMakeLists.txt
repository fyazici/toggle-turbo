cmake_minimum_required(VERSION 3.0)
project(toggle-turbo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRCS 
    src/main.cpp
    main.rc
)

add_executable(toggle-turbo ${SRCS})
target_link_options(toggle-turbo PRIVATE -static-libgcc -static-libstdc++)
