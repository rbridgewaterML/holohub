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

void DDSKeyValueSubscriberOp::set_domain_participant(DDS::DomainParticipant_var participant) {
  participant_ = participant;
}

void DDSKeyValueSubscriberOp::set_client_id(uint32_t client_id) {
  client_id_ = client_id;
}

void DDSKeyValueSubscriberOp::setup(OperatorSpec& spec) {
  spec.output<std::vector<holoscan::dds::KeyValue>>("output");
}

void DDSKeyValueSubscriberOp::initialize() {
  Operator::initialize();

  subscriber_ = holoscan::dds::KeyValues::create_subscriber(participant_, client_id_);
}

void DDSKeyValueSubscriberOp::compute(InputContext& op_input,
                                      OutputContext& op_output,
                                      ExecutionContext& context) {
  auto key_values = subscriber_->read();

  op_output.emit(key_values, "output");
}

}  // namespace holoscan::ops
