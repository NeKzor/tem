cmake_minimum_required(VERSION 3.5.0)
project(tem-launcher VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_GENERATOR_PLATFORM Win32)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)

set(MINGW_INCLUDE_PATH /usr/i686-w64-mingw32/include)
set(MINGW_LIBRARY_PATH /usr/i686-w64-mingw32/lib)

add_executable(GridGameLauncher.exe
    Launcher.cpp
)

target_compile_definitions(GridGameLauncher.exe
    PRIVATE
    UNICODE
    _UNICODE
)

target_compile_options(GridGameLauncher.exe
    PRIVATE
    -Wall
    -Wextra
)
