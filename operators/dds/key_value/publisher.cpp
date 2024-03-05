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

void DDSKeyValuePublisherOp::set_domain_participant(DDS::DomainParticipant_var participant) {
  participant_ = participant;
}

void DDSKeyValuePublisherOp::setup(OperatorSpec& spec) {
  spec.input<std::vector<holoscan::dds::KeyValue>>("input");
}

void DDSKeyValuePublisherOp::initialize() {
  Operator::initialize();

  publisher_ = holoscan::dds::KeyValues::create_publisher(participant_);
}

void DDSKeyValuePublisherOp::compute(InputContext& op_input,
                                     OutputContext& op_output,
                                     ExecutionContext& context) {
  auto input = op_input.receive<std::vector<holoscan::dds::KeyValue>>("input").value();

  for (auto key_value : input) {
    publisher_->write(key_value);
  }
}

}  // namespace holoscan::ops
