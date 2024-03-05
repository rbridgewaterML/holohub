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

#pragma once

#include <holoscan/holoscan.hpp>

#include "dds_key_values.hpp"

namespace holoscan::ops {

/**
 * @brief Operator class to publish KeyValues to DDS.
 */
class DDSKeyValuePublisherOp : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(DDSKeyValuePublisherOp)

  DDSKeyValuePublisherOp() = default;

  void set_domain_participant(DDS::DomainParticipant_var participant);

  void setup(OperatorSpec& spec) override;
  void initialize() override;
  void compute(InputContext& op_input, OutputContext& op_output,
               ExecutionContext& context) override;

 private:
  DDS::DomainParticipant_var participant_;
  std::shared_ptr<holoscan::dds::KeyValues::Publisher> publisher_;
};

}  // namespace holoscan::ops
