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

#include "dds_video_stream.hpp"

#include <dds/DCPS/Marked_Default_Qos.h>

namespace holoscan::dds {

DDS::Topic_var VideoStream::topic_;

void VideoStream::register_topic(DDS::DomainParticipant_var& participant) {
  if (!topic_) {
    // Create VideoFrame type support
    VideoFrameTypeSupport_var ts = new VideoFrameTypeSupportImpl();
    if (ts->register_type(participant.in(), "VideoFrame") != DDS::RETCODE_OK) {
      throw std::runtime_error("Failed to register VideoFrame type");
    }

    // Create VideoStream topic for VideoFrames
    topic_ = participant->create_topic("VideoStream",
                                       "VideoFrame",
                                       TOPIC_QOS_DEFAULT,
                                       DDS::TopicListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic_) {
      throw std::runtime_error("Failed to create VideoStream topic");
    }
  }
}

std::shared_ptr<VideoStream::Publisher> VideoStream::create_publisher(
    DDS::DomainParticipant_var& participant) {
  // Register the VideoStream topic.
  register_topic(participant);

  // Create publisher
  DDS::Publisher_var publisher = participant->create_publisher(
      PUBLISHER_QOS_DEFAULT,
      DDS::PublisherListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!publisher) {
    throw std::runtime_error("Failed to create VideoStream publisher");
  }

  // Create data writer
  DDS::DataWriter_var writer = publisher->create_datawriter(
      topic_.in(),
      DATAWRITER_QOS_DEFAULT,
      DDS::DataWriterListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!writer) {
    throw std::runtime_error("Failed to create VideoStream data writer");
  }

  VideoFrameDataWriter_var frame_writer = VideoFrameDataWriter::_narrow(writer);
  if (!frame_writer) {
    throw std::runtime_error("Failed to get VideoFrame writer");
  }

  // Return a new VideoStream::Publisher object.
  return std::make_shared<Publisher>(frame_writer);
}

std::shared_ptr<VideoStream::Subscriber> VideoStream::create_subscriber(
    DDS::DomainParticipant_var& participant) {
  // Register the VideoStream topic.
  register_topic(participant);

  // Create subscriber
  DDS::Subscriber_var subscriber = participant->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT,
      DDS::SubscriberListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!subscriber) {
    throw std::runtime_error("Failed to create VideoStream subscriber");
  }

  // Create data reader
  DDS::DataReader_var reader = subscriber->create_datareader(
      topic_.in(),
      DATAREADER_QOS_DEFAULT,
      DDS::DataReaderListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!reader) {
    throw std::runtime_error("Failed to create VideoStream data reader");
  }

  VideoFrameDataReader_var frame_reader = VideoFrameDataReader::_narrow(reader);
  if (!frame_reader) {
    throw std::runtime_error("Failed to get VideoFrame reader");
  }

  // Return a new VideoStream::Subscriber object.
  return std::make_shared<Subscriber>(frame_reader);
}

VideoStream::Publisher::Publisher(VideoFrameDataWriter_var writer)
    : writer_(writer)
{}

void VideoStream::Publisher::write(uint32_t width, uint32_t height, uint8_t* ptr) {
  const auto size = width * height * 4;

  VideoFrame frame;
  frame.width = width;
  frame.height = height;
  frame.data.replace(size, ptr);

  DDS::ReturnCode_t error = writer_->write(frame, DDS::HANDLE_NIL);
  if (error != DDS::RETCODE_OK) {
    throw std::runtime_error("Failed to write frame");
  }
}

VideoStream::Subscriber::Subscriber(VideoFrameDataReader_var reader) : reader_(reader) {
  // Create condition and wait set to wait for DATA_AVAILABLE_STATUS.
  // This will be used by the read function(s).
  DDS::StatusCondition_var condition = reader_->get_statuscondition();
  condition->set_enabled_statuses(DDS::DATA_AVAILABLE_STATUS);
  wait_set_ = new DDS::WaitSet;
  wait_set_->attach_condition(condition);
}

std::shared_ptr<VideoStream::Subscriber::ReadFrame> VideoStream::Subscriber::read() {
  VideoFrameSeq frames;
  DDS::SampleInfoSeq info;

  while (true) {
    // Read a frame from the reader using zero-copy.
    DDS::ReturnCode_t error = reader_->take(frames, info, 1,
                                            DDS::ANY_SAMPLE_STATE,
                                            DDS::ANY_VIEW_STATE,
                                            DDS::ANY_INSTANCE_STATE);

    if (error == DDS::RETCODE_OK) {
      if (info[0].valid_data) {
        break;
      }
    } else if (error == DDS::RETCODE_NO_DATA) {
      // If no video frame is available, wait for the DATA_AVAILABLE condition.
      DDS::ConditionSeq conditions;
      DDS::Duration_t timeout = { 30, 0 };
      if (wait_set_->wait(conditions, timeout) != DDS::RETCODE_OK) {
        throw std::runtime_error("Failed to wait for data");
      }
    } else {
      throw std::runtime_error("Failed to read data");
    }
  }

  // Return the frame as a ReadFrame object that will hold a reference to the
  // frame and return its resources to the subscriber when destroyed.
  return std::make_shared<VideoStream::Subscriber::ReadFrame>(reader_, frames, info);
}

VideoStream::Subscriber::ReadFrame::ReadFrame(VideoFrameDataReader_var reader,
                                              VideoFrameSeq frames,
                                              DDS::SampleInfoSeq info)
    : reader_(reader), frames_(frames), info_(info)
{}

VideoStream::Subscriber::ReadFrame::~ReadFrame() {
  // Return the loan to the zero-copy frame read from DDS.
  reader_->return_loan(frames_, info_);
}

}  // namespace holoscan::dds
