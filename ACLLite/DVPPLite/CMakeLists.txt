# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.

# CMake lowest version requirement
cmake_minimum_required(VERSION 3.5.1)

# project information
project(dvpp_interface)

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

add_library(acllite_dvpp_lite SHARED
    ./src/ImageProc.cpp
    ./src/VdecHelper.cpp
    ./src/VideoRead.cpp
    ./src/VideoWrite.cpp
    ./src/VencHelper.cpp
)
include_directories(acllite_dvpp_lite
    ./include
    ${INC_PATH}/runtime/include/
    ${INC_PATH}/include/
)

add_compile_options(acllite_dvpp_lite
    -O2
    -Wall
    -fpic
)

target_link_libraries(acllite_dvpp_lite
    ascendcl
    acl_dvpp
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

INSTALL(TARGETS acllite_dvpp_lite LIBRARY DESTINATION lib)
INSTALL(FILES ./include/ImageProc.h DESTINATION include/acllite_dvpp_lite)
INSTALL(FILES ./include/VdecHelper.h DESTINATION include/acllite_dvpp_lite)
INSTALL(FILES ./include/VideoRead.h DESTINATION include/acllite_dvpp_lite)
INSTALL(FILES ./include/VencHelper.h DESTINATION include/acllite_dvpp_lite)
INSTALL(FILES ./include/VideoWrite.h DESTINATION include/acllite_dvpp_lite)
