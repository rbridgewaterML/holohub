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

#include "subscriber.hpp"

namespace holoscan::ops {

void DDSVideoSubscriberOp::set_domain_participant(DDS::DomainParticipant_var participant) {
  participant_ = participant;
}

void DDSVideoSubscriberOp::setup(OperatorSpec& spec) {
  spec.output<gxf::Entity>("output");
}

void DDSVideoSubscriberOp::initialize() {
  Operator::initialize();

  subscriber_ = holoscan::dds::VideoStream::create_subscriber(participant_);
}

void DDSVideoSubscriberOp::compute(InputContext& op_input,
                                   OutputContext& op_output,
                                   ExecutionContext& context) {
  auto frame = subscriber_->read();

  nvidia::gxf::VideoTypeTraits<nvidia::gxf::VideoFormat::GXF_VIDEO_FORMAT_RGBA> video_type;
  nvidia::gxf::VideoFormatSize<nvidia::gxf::VideoFormat::GXF_VIDEO_FORMAT_RGBA> color_format;
  auto color_planes = color_format.getDefaultColorPlanes(frame->width(), frame->height());
  auto layout = nvidia::gxf::SurfaceLayout::GXF_SURFACE_LAYOUT_PITCH_LINEAR;
  nvidia::gxf::VideoBufferInfo buffer_info{frame->width(),
                                           frame->height(),
                                           video_type.value,
                                           color_planes,
                                           layout};

  auto output = nvidia::gxf::Entity::New(context.context());
  if (!output) {
    throw std::runtime_error("Failed to allocate output");
  }

  auto video_buffer = output.value().add<nvidia::gxf::VideoBuffer>();
  if (!video_buffer) {
    throw std::runtime_error("Failed to allocate video buffer");
  }

  video_buffer.value()->wrapMemory(buffer_info, frame->width() * frame->height() * 4,
                                   nvidia::gxf::MemoryStorageType::kHost,
                                   const_cast<uint8_t*>(frame->data()), nullptr);

  auto result = gxf::Entity(std::move(output.value()));
  op_output.emit(result, "output");
}

}  // namespace holoscan::ops
