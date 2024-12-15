cmake_minimum_required(VERSION 3.5.0)
project(tem VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_GENERATOR_PLATFORM Win32)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -pthread -lwinpthread")

set(MINGW_INCLUDE_PATH /usr/i686-w64-mingw32/include)
set(MINGW_LIBRARY_PATH /usr/i686-w64-mingw32/lib)

file(GLOB_RECURSE LIB_IMGUI "lib/imgui/*.cpp")
file(GLOB_RECURSE LIB_MINHOOK "lib/minhook/*.c")

add_library(tem.dll
    SHARED
    ${LIB_IMGUI}
    ${LIB_MINHOOK}
    dllmain.cpp
    Dumper.cpp
    GFWL.cpp
    Memory.cpp
    SDK.cpp
    SpotChecks.cpp
    TEM.cpp
    UI.cpp
)

set_target_properties(tem.dll PROPERTIES PREFIX "")
set_target_properties(tem.dll PROPERTIES SUFFIX "")

target_compile_definitions(tem.dll
    PRIVATE
    UNICODE
    _UNICODE
)

target_compile_options(tem.dll
    PRIVATE
    -Wall
    -Wextra
    -Wno-expansion-to-defined
    -Wno-unknown-pragmas
    -Wno-deprecated-declarations
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-cast-function-type
)

target_link_libraries(tem.dll
    ${MINGW_LIBRARY_PATH}/libd3d9.a
    ${MINGW_LIBRARY_PATH}/libdwmapi.a
    ${MINGW_LIBRARY_PATH}/libgdi32.a
)

target_include_directories(tem.dll
    PUBLIC
    ${MINGW_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
