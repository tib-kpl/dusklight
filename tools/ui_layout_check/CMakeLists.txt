cmake_minimum_required(VERSION 3.25)
project(dusklight_layout_check LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(freetype
    URL https://files.twilitrealm.dev/freetype-2.14.3.tar.gz
    URL_HASH SHA256=e61b31ab26358b946e767ed7eb7f4bb2e507da1cfefeb7a8861ace7fd5c899a1
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE)
FetchContent_MakeAvailable(freetype)
add_library(Freetype::Freetype ALIAS freetype)
set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rmlui
    URL https://github.com/encounter/RmlUi/archive/f00a0fee38391b2f927114e11bea18dc0a7dba1e.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE)
FetchContent_MakeAvailable(rmlui)
add_executable(layout-check main.cpp)
target_link_libraries(layout-check PRIVATE rmlui_core)
