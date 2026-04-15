cmake_minimum_required(VERSION 3.18)
project(gizmo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(pybind11 CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVCODEC  REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVUTIL   REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWRESAMPLE REQUIRED IMPORTED_TARGET libswresample)

find_library(VOSK_LIBRARY NAMES vosk PATHS /usr/lib /usr/local/lib)
find_path(VOSK_INCLUDE_DIR NAMES vosk_api.h PATHS /usr/include /usr/local/include)

if(NOT VOSK_LIBRARY OR NOT VOSK_INCLUDE_DIR)
    message(FATAL_ERROR "Vosk library not found. Install libvosk or set VOSK_LIBRARY and VOSK_INCLUDE_DIR.")
endif()

set(GIZMO_SOURCES
    gizmo/extractor.cpp
    gizmo/correlator.cpp
    gizmo/synchro/synchronizer.cpp
    gizmo/media/demux.cpp
    gizmo/media/stream.cpp
    gizmo/media/audiodec.cpp
    gizmo/media/resampler.cpp
    gizmo/media/subdec.cpp
    gizmo/media/speechrec.cpp
    gizmo/text/translator.cpp
    gizmo/text/dictionary.cpp
    gizmo/text/words.cpp
    gizmo/text/ssa.cpp
    gizmo/text/ngrams.cpp
    gizmo/text/utf8.cpp
    gizmo/text/wordsqueue.cpp
    gizmo/math/point.cpp
    gizmo/math/line.cpp
    gizmo/math/linefinder.cpp
    gizmo/general/exception.cpp
    gizmo/general/logger.cpp
    gizmo/general/platform.cpp
    gizmo/python/wrapper.cpp
    gizmo/python/general.cpp
    gizmo/python/extractor.cpp
    gizmo/python/media.cpp
    gizmo/python/stream.cpp
    gizmo/python/correlator.cpp
    gizmo/python/translator.cpp
)

pybind11_add_module(gizmo ${GIZMO_SOURCES})

target_include_directories(gizmo PRIVATE
    ${CMAKE_SOURCE_DIR}/gizmo
    ${VOSK_INCLUDE_DIR}
)

target_link_libraries(gizmo PRIVATE
    PkgConfig::AVFORMAT
    PkgConfig::AVCODEC
    PkgConfig::AVUTIL
    PkgConfig::SWRESAMPLE
    ${VOSK_LIBRARY}
)

target_compile_definitions(gizmo PRIVATE
    NDEBUG
    USE_PYBIND11
    USE_VOSK
)

if(MSVC)
    target_compile_options(gizmo PRIVATE /EHsc /O2)
else()
    target_compile_options(gizmo PRIVATE
        -O3
        -fvisibility=hidden
        -fomit-frame-pointer
    )
endif()
