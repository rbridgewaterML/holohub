# SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.24)
project(holoscan_dds_video)

find_package(holoscan 0.6 REQUIRED CONFIG
             PATHS "/opt/nvidia/holoscan" "/workspace/holoscan-sdk/install")

# Publisher operator.
option("OP_dds_video_publisher" "Build the DDS VideoPublisher operator" ${BUILD_ALL})
if(${OP_dds_video_publisher})
  add_library(holoscan_dds_video_publisher SHARED publisher.cpp publisher.hpp)
  add_library(holoscan::ops::dds_video_publisher ALIAS holoscan_dds_video_publisher)
  target_link_libraries(holoscan_dds_video_publisher PUBLIC
    holoscan::core
    holoscan_dds_video_stream
  )
  target_include_directories(holoscan_dds_video_publisher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
  set("LIB_dds_video_frame" ON CACHE BOOL "Build LIB_dds_video_frame" FORCE)
endif()

# Subscriber operator.
option("OP_dds_video_subscriber" "Build the DDS VideoSubscriber operator" ${BUILD_ALL})
if(${OP_dds_video_subscriber})
  add_library(holoscan_dds_video_subscriber SHARED subscriber.cpp subscriber.hpp)
  add_library(holoscan::ops::dds_video_subscriber ALIAS holoscan_dds_video_subscriber)
  target_link_libraries(holoscan_dds_video_subscriber PUBLIC
    holoscan::core
    holoscan_dds_video_stream
  )
  target_include_directories(holoscan_dds_video_subscriber PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
  set("LIB_dds_video_frame" ON CACHE BOOL "Build LIB_dds_video_frame" FORCE)
endif()

# DDS support libraries.
option("LIB_dds_video_frame" "Build the DDS VideoFrame libraries" ${BUILD_ALL})
if(${LIB_dds_video_frame})
  find_package(OpenDDS REQUIRED PATHS "/opt/opendds")

  # holoscan::dds::VideoFrame type generated from IDL.
  add_library(holoscan_dds_video_frame_idl SHARED)
  opendds_target_sources(holoscan_dds_video_frame_idl PUBLIC dds_video_frame.idl)
  target_link_libraries(holoscan_dds_video_frame_idl PUBLIC OpenDDS::Dcps)

  # holoscan::dds::VideoStream library used by operators.
  add_library(holoscan_dds_video_stream SHARED dds_video_stream.cpp dds_video_stream.hpp)
  target_link_libraries(holoscan_dds_video_stream PUBLIC holoscan_dds_video_frame_idl)
endif()
