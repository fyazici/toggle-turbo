# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# which compilers to use
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# adjust the default behavior of the find commands:
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# win resource compiler
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
