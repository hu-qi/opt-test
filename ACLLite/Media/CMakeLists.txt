# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.

# CMake lowest version requirement
cmake_minimum_required(VERSION 3.5.1)

# project information
project(camera_interface)

add_definitions(-DENABLE_DVPP_INTERFACE)
set(INC_PATH $ENV{DDK_PATH})
if (NOT DEFINED ENV{DDK_PATH})
    set(INC_PATH "/usr/local/Ascend/ascend-toolkit/latest/aarch64-linux")
    message(STATUS "set default INC_PATH: ${INC_PATH}")
else ()
    message(STATUS "env INC_PATH: ${INC_PATH}")
endif()

set(LIB_PATH $ENV{NPU_HOST_LIB})
if (NOT DEFINED ENV{NPU_HOST_LIB})
    set(LIB_PATH "/usr/local/Ascend/ascend-toolkit/latest/aarch64-linux/runtime/lib64/stub")
    message(STATUS "set default LIB_PATH: ${LIB_PATH}")
else ()
    message(STATUS "env LIB_PATH: ${LIB_PATH}")
endif()

link_directories(
    ${LIB_PATH}
)

add_library(acllite_media SHARED
    ./CameraRead.cpp
)
include_directories(acllite_media
    ./
    ${INC_PATH}/runtime/include/
    ${INC_PATH}/include/
)

add_compile_options(acllite_media
    -O2
    -Wall
    -fpic
)

target_link_libraries(acllite_media
    ascendcl
    stdc++
    acllite_common
    pthread
    avcodec
    avformat
    avutil
    swresample
    swscale
    avdevice
    dl
)

INSTALL(TARGETS acllite_media LIBRARY DESTINATION lib)
INSTALL(FILES ./CameraRead.h DESTINATION include/acllite_media)