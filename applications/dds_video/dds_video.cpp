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
#include <holoscan/operators/holoviz/holoviz.hpp>
#include <holoscan/operators/v4l2_video_capture/v4l2_video_capture.hpp>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include "dds/key_value/subscriber.hpp"
#include "dds/video/publisher.hpp"
#include "dds/video/subscriber.hpp"

#include <getopt.h>

namespace holoscan::ops {

class DDSKeyValueRenderer : public Operator {
 public:
  HOLOSCAN_OPERATOR_FORWARD_ARGS(DDSKeyValueRenderer)

  DDSKeyValueRenderer() = default;

  void setup(OperatorSpec& spec) override {
    // Inputs from DDSKeyValueSubscriber
    spec.input<std::vector<holoscan::dds::KeyValue>>("input");

    // Outputs to Holoviz
    spec.output<gxf::Entity>("outputs");
    spec.output<std::vector<HolovizOp::InputSpec>>("output_specs");
  }

  void initialize() override {
    allocator_ = fragment()->make_resource<UnboundedAllocator>("pool");
    add_arg(allocator_);

    Operator::initialize();
  }

  void compute(InputContext& op_input, OutputContext& op_output,
               ExecutionContext& context) override {
    auto entity = gxf::Entity::New(&context);

    // Generate the string to display
    std::stringstream ss;
    auto key_values = op_input.receive<std::vector<holoscan::dds::KeyValue>>("input").value();
    for (auto& kv : key_values) {
      ss << kv.key << ": " << kv.value << std::endl;
    }

    // Output the text coordinates (to 'outputs')
    auto tensor = static_cast<nvidia::gxf::Entity&>(entity)
        .add<nvidia::gxf::Tensor>("text").value();
    auto allocator = nvidia::gxf::Handle<nvidia::gxf::Allocator>::Create(
        context.context(), allocator_->gxf_cid());
    tensor->reshape<float>(nvidia::gxf::Shape({1, 2}),
                           nvidia::gxf::MemoryStorageType::kHost, allocator.value());
    std::memcpy(tensor->pointer(), coords_, sizeof(coords_));
    op_output.emit(entity, "outputs");

    // Output the text to render (to 'output_specs')
    auto specs = std::vector<HolovizOp::InputSpec>();
    HolovizOp::InputSpec spec;
    spec.tensor_name_ = "text";
    spec.type_ = HolovizOp::InputType::TEXT;
    spec.color_ = {0.0f, 0.0f, 0.0f, 1.0f};
    spec.text_.push_back(ss.str());
    specs.push_back(spec);
    op_output.emit(specs, "output_specs");
  }

 private:
  float coords_[2] = {0.05f, 0.05f};
  std::shared_ptr<UnboundedAllocator> allocator_;
};

}  // namespace holoscan::ops

class V4L2ToDDS : public holoscan::Application {
 public:
  explicit V4L2ToDDS(DDS::DomainParticipant_var participant)
      : participant_(participant) {}

  void compose() override {
    using namespace holoscan;

    auto v4l2 = make_operator<ops::V4L2VideoCaptureOp>("v4l2",
        Arg("allocator") = make_resource<UnboundedAllocator>("pool"),
        Arg("width", 640u), Arg("height", 480u));
    auto dds = make_operator<ops::DDSVideoPublisherOp>("dds");
    dds->set_domain_participant(participant_);

    add_flow(v4l2, dds, {{"signal", "input"}});
  }

 private:
  DDS::DomainParticipant_var participant_;
};

class DDSToHoloviz : public holoscan::Application {
 public:
  explicit DDSToHoloviz(DDS::DomainParticipant_var participant, uint32_t client_id)
      : participant_(participant), client_id_(client_id) {}

  void compose() override {
    using namespace holoscan;

    //  DDS Video Subscriber
    auto video_subscriber = make_operator<ops::DDSVideoSubscriberOp>("video_subscriber");
    video_subscriber->set_domain_participant(participant_);

    // DDS Key Value Subscriber
    auto key_value_subscriber = make_operator<ops::DDSKeyValueSubscriberOp>("key_value_subscriber");
    key_value_subscriber->set_domain_participant(participant_);
    key_value_subscriber->set_client_id(client_id_);

    // DDS Key Value Renderer
    auto key_value_renderer = make_operator<ops::DDSKeyValueRenderer>("key_value_renderer");

    // Holoviz
    std::vector<ops::HolovizOp::InputSpec> input_spec;
    auto& video_spec = input_spec.emplace_back(
        ops::HolovizOp::InputSpec("", ops::HolovizOp::InputType::COLOR));
    auto holoviz = make_operator<ops::HolovizOp>("holoviz",
        Arg("width", 640u), Arg("height", 480u), Arg("tensors", input_spec));

    add_flow(video_subscriber, holoviz, {{"output", "receivers"}});
    add_flow(key_value_subscriber, key_value_renderer, {{"output", "input"}});
    add_flow(key_value_renderer, holoviz, {{"outputs", "receivers"}});
    add_flow(key_value_renderer, holoviz, {{"output_specs", "input_specs"}});
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

  struct option long_options[] = {
      {"sender", no_argument, 0, 's'},
      {"receiver", no_argument, 0, 'r'},
      {"client", required_argument, 0, 'c'},
      {"domain", required_argument, 0, 'd'},
      {0, 0, 0, 0}};

  while (true) {
    int option_index = 0;

    const int c = getopt_long(argc, argv, "src:d:", long_options, &option_index);
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
    auto app = holoscan::make_application<V4L2ToDDS>(participant);
    app->run();
  } else if (receiver) {
    auto app = holoscan::make_application<DDSToHoloviz>(participant, client_id);
    app->run();
  }

  participant->delete_contained_entities();
  participant_factory->delete_participant(participant.in());

  TheServiceParticipant->shutdown();

  return 0;
}
