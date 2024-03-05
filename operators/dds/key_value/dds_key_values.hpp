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

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include "dds_key_valueTypeSupportImpl.h"

namespace holoscan::dds {

/**
 * Provides functionality to publish or subscribe to DDS KeyValues.
 */
class KeyValues {
 public:
  /**
   * Object to wrap a publisher for Holoscan-to-DDS KeyValues.
   *
   * Created by KeyValues::create_publisher.
   */
  class Publisher {
   public:
    /**
     * @brief Creates a Publisher object that wraps a KeyValueDataWriter.
     *
     * Called by KeyValues::create_publisher.
     */
    explicit Publisher(KeyValueDataWriter_var writer);

    /**
     * Writes a KeyValue to the Publisher.
     */
    void write(const KeyValue& key_value);

    /**
     * Writes a KeyValue to the Publisher.
     */
    void write(uint32_t client_id, const std::string& key, const std::string& value);

   private:
    KeyValueDataWriter_var writer_;
  };

  /**
   * Object to wrap a subscriber for DDS-to-Holoscan KeyValues.
   *
   * Created by KeyValues::create_subscriber.
   */
  class Subscriber {
   public:
    /**
     * @brief Creates a Subscriber object that wraps a KeyValueDataReader.
     *
     * Called by KeyValues::create_subscriber.
     */
    explicit Subscriber(KeyValueDataReader_var reader);

    /**
     * @brief Reads the KeyValues available to the Subscriber.
     */
    std::vector<KeyValue> read();

   private:
    KeyValueDataReader_var reader_;
  };

  /**
   * @brief Factory method to create a KeyValue::Publisher
   */
  static std::shared_ptr<Publisher> create_publisher(DDS::DomainParticipant_var& participant);

  /**
   * @brief Factory method to create a KeyValue::Subscriber
   */
  static std::shared_ptr<Subscriber> create_subscriber(DDS::DomainParticipant_var& participant,
                                                       uint32_t client_id);

 private:
  // Shared instance of the registered KeyValue topic.
  static DDS::Topic_var topic_;

  /**
   * @brief Registers the KeyValue topic with the DDS::DomainParticipant.
   */
  static void register_topic(DDS::DomainParticipant_var& participant);
};

}  // namespace holoscan::dds
