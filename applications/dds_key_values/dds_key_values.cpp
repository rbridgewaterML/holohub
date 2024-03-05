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

#include <holoscan/holoscan.hpp>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include "dds/key_value/publisher.hpp"
#include "dds/key_value/subscriber.hpp"

#include <chrono>
#include <getopt.h>

namespace holoscan::ops {

class KeyValueGenerator : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(KeyValueGenerator)

  KeyValueGenerator() = default;

  void setup(OperatorSpec& spec) override {
    spec.param(client_id_, "client_id");
    spec.param(key_, "key");
    spec.param(value_, "value");

    spec.output<std::vector<holoscan::dds::KeyValue>>("output");
  }

  void compute(InputContext&, OutputContext& op_output, ExecutionContext&) override {
    std::vector<holoscan::dds::KeyValue> key_values;

    auto& kv = key_values.emplace_back();
    kv.client_id = client_id_;
    kv.key = key_.get().c_str();
    kv.value = value_.get().c_str();

    HOLOSCAN_LOG_INFO("Sending to client {}: {} / {}", kv.client_id, kv.key, kv.value);

    op_output.emit(key_values, "output");
  }

 private:
  Parameter<uint32_t> client_id_;
  Parameter<std::string> key_, value_;
};

class KeyValuePrinter : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(KeyValuePrinter)

  KeyValuePrinter() = default;

  void setup(OperatorSpec& spec) override {
    spec.input<std::vector<holoscan::dds::KeyValue>>("input");
  }

  void compute(InputContext& op_input, OutputContext&, ExecutionContext&) override {
    auto key_values = op_input.receive<std::vector<holoscan::dds::KeyValue>>("input").value();
    HOLOSCAN_LOG_INFO("Query {}:", count_);
    for (auto kv : key_values) {
      HOLOSCAN_LOG_INFO("  {} / {}", kv.key, kv.value);
    }
    ++count_;
  }

 private:
  uint32_t count_ = 0;
};

}  // namespace holoscan::ops

class DDSKeyValueSender : public holoscan::Application {
 public:
  explicit DDSKeyValueSender(DDS::DomainParticipant_var participant, uint32_t client_id,
                             std::string& key, std::string& value)
      : participant_(participant), client_id_(client_id), key_(key), value_(value) {}

  void compose() override {
    using namespace holoscan;

    auto generator = make_operator<ops::KeyValueGenerator>("generator",
        Arg("client_id", client_id_),
        Arg("key", key_),
        Arg("value", value_),
        make_condition<CountCondition>(1));
    auto publisher = make_operator<ops::DDSKeyValuePublisherOp>("publisher");
    publisher->set_domain_participant(participant_);

    add_flow(generator, publisher, {{"output", "input"}});
  }

 private:
  DDS::DomainParticipant_var participant_;
  uint32_t client_id_;
  std::string key_, value_;
};

class DDSKeyValueReceiver : public holoscan::Application {
 public:
  explicit DDSKeyValueReceiver(DDS::DomainParticipant_var participant, uint32_t client_id)
      : participant_(participant), client_id_(client_id) {}

  void compose() override {
    using namespace holoscan;
    using namespace std::chrono_literals;

    auto subscriber = make_operator<ops::DDSKeyValueSubscriberOp>("subscriber",
        make_condition<PeriodicCondition>(1.0s));
    subscriber->set_domain_participant(participant_);
    subscriber->set_client_id(client_id_);
    auto printer = make_operator<ops::KeyValuePrinter>("printer");

    add_flow(subscriber, printer, {{"output", "input"}});
  }

 private:
  DDS::DomainParticipant_var participant_;
  uint32_t client_id_;
};

int main(int argc, char** argv) {
  bool sender = false;
  bool receiver = false;
  uint32_t client_id = 0;
  uint32_t domain = 42;
  std::string key = "DefaultKey";
  std::string value = "DefaultValue";

  struct option long_options[] = {
      {"sender", no_argument, 0, 's'},
      {"receiver", no_argument, 0, 'r'},
      {"client", required_argument, 0, 'c'},
      {"key", required_argument, 0, 'k'},
      {"value", required_argument, 0, 'v'},
      {"domain", required_argument, 0, 'd'},
      {0, 0, 0, 0}};

  while (true) {
    int option_index = 0;

    const int c = getopt_long(argc, argv, "src:k:v:d:", long_options, &option_index);
    if (c == -1) { break; }

    const std::string argument(optarg ? optarg : "");
    switch (c) {
      case 's':
        sender = true;
        break;
      case 'r':
        receiver = true;
        break;
      case 'c':
        client_id = stoi(argument);
        break;
      case 'k':
        key = argument;
        break;
      case 'v':
        value = argument;
        break;
      case 'd':
        domain = stoi(argument);
        break;
      default:
        holoscan::log_error("Unhandled option '{}'", static_cast<char>(c));
    }
  }

  if (!sender && !receiver) {
    holoscan::log_error("Must provide either -s or -r for sender or receiver, respectively");
    return -1;
  }

  DDS::DomainParticipantFactory_var participant_factory = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant = participant_factory->create_participant(
      domain,
      PARTICIPANT_QOS_DEFAULT,
      DDS::DomainParticipantListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!participant) {
    throw std::runtime_error("Failed to create DomainParticipant");
  }

  if (sender) {
    auto app = holoscan::make_application<DDSKeyValueSender>(participant, client_id, key, value);
    app->run();
  } else if (receiver) {
    auto app = holoscan::make_application<DDSKeyValueReceiver>(participant, client_id);
    app->run();
  }

  participant->delete_contained_entities();
  participant_factory->delete_participant(participant.in());

  TheServiceParticipant->shutdown();

  return 0;
}
