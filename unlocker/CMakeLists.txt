cmake_minimum_required(VERSION 3.5.0)
project(tem-unlocker VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_GENERATOR_PLATFORM Win32)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static -pthread -lwinpthread")

set(MINGW_INCLUDE_PATH /usr/i686-w64-mingw32/include)
set(MINGW_LIBRARY_PATH /usr/i686-w64-mingw32/lib)
set(OPENSSL_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/openssl)

file(GLOB_RECURSE LIB_BIG_DIGITS "lib/BigDigits/b*.c")

add_executable(unlocker.exe
    main.cpp
    ${LIB_BIG_DIGITS}
)

target_compile_definitions(unlocker.exe
    PRIVATE
    UNICODE
    _UNICODE
)

target_compile_options(unlocker.exe
    PRIVATE
    -Wall
    -Wextra
    -Wno-expansion-to-defined
    -Wno-unknown-pragmas
    -Wno-deprecated-declarations
    -Wno-cast-function-type
)

target_include_directories(unlocker.exe
    PUBLIC
    ${MINGW_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(unlocker.exe
    ${OPENSSL_LIB_PATH}/libcrypto.a
    ${OPENSSL_LIB_PATH}/libssl.a
)

# tests

add_executable(test.exe
    main.cpp
    Tests.cpp
    ${LIB_BIG_DIGITS}
)

target_compile_definitions(test.exe
    PRIVATE
    UNICODE
    _UNICODE
    BUILD_TESTS
)

target_compile_options(test.exe
    PRIVATE
    -Wall
    -Wextra
    -Wno-expansion-to-defined
    -Wno-unknown-pragmas
    -Wno-deprecated-declarations
    -Wno-unused-parameter
    -Wno-cast-function-type
)

target_link_libraries(test.exe
    ${OPENSSL_LIB_PATH}/libcrypto.a
    ${OPENSSL_LIB_PATH}/libssl.a
)

target_include_directories(test.exe
    PUBLIC
    ${MINGW_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
