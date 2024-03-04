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
#include "dds_video_frameTypeSupportImpl.h"

namespace holoscan::dds {

/**
 * Provides functionality to publish or subscribe to a stream of DDS VideoFrames.
 */
class VideoStream {
 public:
  /**
   * Object to wrap a publisher for a Holoscan-to-DDS video stream.
   *
   * Created by VideoStream::create_publisher.
   */
  class Publisher {
   public:
    /**
     * @brief Creates a Publisher object that wraps a VideoFrameDataWriter.
     *
     * Called by VideoStream::create_publisher.
     */
    explicit Publisher(VideoFrameDataWriter_var writer);

    /**
     * @brief Write an RGBA8888 video frame using the publisher.
     *
     * @param width Width of the video frame.
     * @param height Height of the video frame.
     * @param ptr Pointer to the video frame data.
     */
    void write(uint32_t width, uint32_t height, uint8_t* ptr);

   private:
    VideoFrameDataWriter_var writer_;
  };

  /**
   * Object to wrap a subscriber for a DDS-to-Holoscan video stream.
   *
   * Created by VideoStream::create_subscriber.
   */
  class Subscriber {
   public:
    /**
     * @brief Creates a Subscriber object that wraps a VideoFrameDataWriter.
     *
     * Called by VideoStream::create_subscriber.
     */
    explicit Subscriber(VideoFrameDataReader_var reader);

    /**
     * Helper object to hold a reference to a zero-copied video frame read by a
     * VideoStream::Subscriber.
     */
    class ReadFrame {
     public:
      /**
       * @brief Constructs a ReadFrame object.
       *
       * Called by VideoStream::Subscriber::read methods.
       */
      ReadFrame(VideoFrameDataReader_var reader,
                VideoFrameSeq frames,
                DDS::SampleInfoSeq info);

      ~ReadFrame();

      const uint32_t width() { return frames_[0].width; }
      const uint32_t height() { return frames_[0].height; }
      const uint8_t* data() { return frames_[0].data.get_buffer(); }

     private:
      VideoFrameDataReader_var reader_;
      VideoFrameSeq frames_;
      DDS::SampleInfoSeq info_;
    };

    /**
     * Reads an RGBA8888 video frame from the subscriber.
     */
    std::shared_ptr<ReadFrame> read();

   private:
    VideoFrameDataReader_var reader_;
    DDS::WaitSet_var wait_set_;
  };

  /**
   * @brief Factory method to create a VideoStream::Publisher
   */
  static std::shared_ptr<Publisher> create_publisher(DDS::DomainParticipant_var& participant);

  /**
   * @brief Factory method to create a VideoStream::Subscriber
   */
  static std::shared_ptr<Subscriber> create_subscriber(DDS::DomainParticipant_var& participant);

 private:
  // Shared instance of the registered VideoStream topic.
  static DDS::Topic_var topic_;

  /**
   * @brief Registers the VideoStream topic with the DDS::DomainParticipant.
   */
  static void register_topic(DDS::DomainParticipant_var& participant);
};

}  // namespace holoscan::dds
