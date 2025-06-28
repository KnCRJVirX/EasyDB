cmake_minimum_required(VERSION 3.10)
project(SimpleLeakDetector LANGUAGES C)

set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/source-charset:utf-8)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_EXE_LINKER_FLAGS "-mwindows")
endif()

# EasyDB
add_library(easydb STATIC easydb.c)

# EasyDBExplorer.exe
add_executable(EasyDBExplorer EasyDB_Explorer_wingui.c)

# 链接静态库
target_link_libraries(EasyDBExplorer PRIVATE kernel32 user32 gdi32 comctl32 uxTheme easydb)