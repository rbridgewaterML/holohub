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

#include "dds_key_values.hpp"

#include <dds/DCPS/Marked_Default_Qos.h>

namespace holoscan::dds {

DDS::Topic_var KeyValues::topic_;

void KeyValues::register_topic(DDS::DomainParticipant_var& participant) {
  if (!topic_) {
    // Create KeyValue type support
    KeyValueTypeSupport_var ts = new KeyValueTypeSupportImpl();
    if (ts->register_type(participant.in(), "KeyValue") != DDS::RETCODE_OK) {
      throw std::runtime_error("Failed to register KeyValue type");
    }

    // Create KeyValue topic
    topic_ = participant->create_topic("KeyValue",
                                       "KeyValue",
                                       TOPIC_QOS_DEFAULT,
                                       DDS::TopicListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic_) {
      throw std::runtime_error("Failed to create KeyValue topic");
    }
  }
}

std::shared_ptr<KeyValues::Publisher> KeyValues::create_publisher(
    DDS::DomainParticipant_var& participant) {
  // Register the KeyValue topic.
  register_topic(participant);

  // Create publisher
  DDS::Publisher_var publisher = participant->create_publisher(
      PUBLISHER_QOS_DEFAULT,
      DDS::PublisherListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!publisher) {
    throw std::runtime_error("Failed to create KeyValue publisher");
  }

  // Create data writer
  DDS::DataWriter_var writer = publisher->create_datawriter(
      topic_.in(),
      DATAWRITER_QOS_DEFAULT,
      DDS::DataWriterListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!writer) {
    throw std::runtime_error("Failed to create KeyValue data writer");
  }

  KeyValueDataWriter_var value_writer = KeyValueDataWriter::_narrow(writer);
  if (!value_writer) {
    throw std::runtime_error("Failed to get KeyValue writer");
  }

  // Wait for subscriber
  {
    DDS::StatusCondition_var condition = writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    while (true) {
      DDS::PublicationMatchedStatus matches;
      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        throw std::runtime_error("Failed to get publication matched status");
      }

      if (matches.current_count >= 1) {
        break;
      }

      DDS::ConditionSeq conditions;
      DDS::Duration_t timeout = { 60, 0 };
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        throw std::runtime_error("Failed to wait for subscriber");
      }
    }
  }

  // Return a new KeyValue::Publisher object.
  return std::make_shared<Publisher>(value_writer);
}

std::shared_ptr<KeyValues::Subscriber> KeyValues::create_subscriber(
    DDS::DomainParticipant_var& participant, uint32_t client_id) {
  // Register the KeyValue topic.
  register_topic(participant);

  // Create subscriber
  DDS::Subscriber_var subscriber = participant->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT,
      DDS::SubscriberListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!subscriber) {
    throw std::runtime_error("Failed to create KeyValue subscriber");
  }

  // Create content filtered topic for the requested id
  auto filter = std::string("client_id = ") + std::to_string(client_id);
  DDS::ContentFilteredTopic_var cft = participant->create_contentfilteredtopic(
      "KeyValue-Filtered",
      topic_,
      filter.c_str(),
      DDS::StringSeq());
  if (!cft) {
    throw std::runtime_error("Failed to create KeyValue filtered topic");
  }

  // Create data reader for the filtered topic
  DDS::DataReader_var reader = subscriber->create_datareader(
      cft,
      DATAREADER_QOS_DEFAULT,
      DDS::DataReaderListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!reader) {
    throw std::runtime_error("Failed to create KeyValue data reader");
  }

  KeyValueDataReader_var value_reader = KeyValueDataReader::_narrow(reader);
  if (!value_reader) {
    throw std::runtime_error("Failed to get KeyValue reader");
  }

  // Return a new KeyValues::Subscriber object.
  return std::make_shared<Subscriber>(value_reader);
}

KeyValues::Publisher::Publisher(KeyValueDataWriter_var writer)
    : writer_(writer)
{}

void KeyValues::Publisher::write(const KeyValue& key_value) {
  DDS::ReturnCode_t error = writer_->write(key_value, DDS::HANDLE_NIL);
  if (error != DDS::RETCODE_OK) {
    throw std::runtime_error("Failed to write KeyValue");
  }
}

void KeyValues::Publisher::write(uint32_t client_id, const std::string& key,
                                 const std::string& value) {
  KeyValue key_value;
  key_value.client_id = client_id;
  key_value.key = strdup(key.c_str());
  key_value.value = strdup(value.c_str());
  write(key_value);
}

KeyValues::Subscriber::Subscriber(KeyValueDataReader_var reader)
    : reader_(reader)
{}

std::vector<KeyValue> KeyValues::Subscriber::read() {
  KeyValueSeq samples;
  DDS::SampleInfoSeq info;
  std::vector<KeyValue> values;

  DDS::ReturnCode_t error = reader_->read(samples, info,
                                          DDS::LENGTH_UNLIMITED,
                                          DDS::ANY_SAMPLE_STATE,
                                          DDS::ANY_VIEW_STATE,
                                          DDS::ANY_INSTANCE_STATE);
  if (error == DDS::RETCODE_OK) {
    for (size_t i = 0; i < samples.length(); i++) {
      if (info[i].valid_data) {
        values.push_back(samples[i]);
      }
    }
  }

  return values;
}

}  // namespace holoscan::dds
