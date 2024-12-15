cmake_minimum_required(VERSION 3.5.0)
project(tem-proxy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_GENERATOR_PLATFORM Win32)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)

set(MINGW_INCLUDE_PATH /usr/i686-w64-mingw32/include)
set(MINGW_LIBRARY_PATH /usr/i686-w64-mingw32/lib)

add_library(dinput8.dll
    SHARED
    dllmain.cpp
    pch.cpp
)

set_target_properties(dinput8.dll PROPERTIES PREFIX "")
set_target_properties(dinput8.dll PROPERTIES SUFFIX "")

target_compile_options(dinput8.dll
    PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

#set_target_properties(dinput8.dll
#    PROPERTIES
#    LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/dinput8.def"
#)
