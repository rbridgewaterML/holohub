/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "publisher.hpp"

namespace holoscan::ops {

void DDSVideoPublisherOp::set_domain_participant(DDS::DomainParticipant_var participant) {
  participant_ = participant;
}

void DDSVideoPublisherOp::setup(OperatorSpec& spec) {
  spec.input<gxf::Entity>("input");
}

void DDSVideoPublisherOp::initialize() {
  Operator::initialize();

  publisher_ = holoscan::dds::VideoStream::create_publisher(participant_);
}

void DDSVideoPublisherOp::compute(InputContext& op_input,
                                  OutputContext& op_output,
                                  ExecutionContext& context) {
  auto input = op_input.receive<gxf::Entity>("input").value();
  if (!input) {
    throw std::runtime_error("No input available");
  }

  const auto& buffer = static_cast<nvidia::gxf::Entity>(input).get<nvidia::gxf::VideoBuffer>();
  if (!buffer) {
    throw std::runtime_error("No video buffer attached to input");
  }

  if (buffer.value()->storage_type() != nvidia::gxf::MemoryStorageType::kHost) {
    throw std::runtime_error("Only host buffers are supported");
  }

  const auto& info = buffer.value()->video_frame_info();
  if (info.color_format != nvidia::gxf::VideoFormat::GXF_VIDEO_FORMAT_RGBA) {
    throw std::runtime_error("Invalid buffer format; Only RGBA is supported");
  }

  publisher_->write(info.width, info.height, buffer.value()->pointer());
}

}  // namespace holoscan::ops
