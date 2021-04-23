/**
 * @file camera_sink.h
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-04-23
 *
 * Copyright (c) 2021 Intel Corporation
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "unix_socket.h"
#include <functional>
#include <memory>

namespace vhal {
namespace client {

/**
 * @brief
 *
 */
class CameraSink
{
public:
    /**
     * @brief
     *
     */
    enum class VideoCodecType
    {
        kH264 = 0,
        kH265
    };

    /**
     * @brief
     *
     */
    enum class FrameResolution
    {
        k480p = 0,
        k720p,
        k1080p
    };

    /**
     * @brief
     *
     */
    struct FrameInfo
    {
        VideoCodecType  codec_type = VideoCodecType::kH265;
        FrameResolution resolution = FrameResolution::k480p;
        uint32_t        reserved[4];
    };

    /**
     * @brief
     *
     */
    enum class Command
    {
        kOpen  = 11,
        kClose = 12,
        kNone  = 13
    };

    /**
     * @brief
     *
     */
    enum class VHalVersion
    {
        kV1 = 0, // decode out of camera vhal
        kV2 = 1, // decode in camera vhal
    };

    /**
     * @brief Camera control message sent by Camera VHAL
     *
     */
    struct CtrlMessage
    {
        VHalVersion version = VHalVersion::kV2;
        Command     cmd     = Command::kNone;
        FrameInfo   frame_info;
    };

    /**
     * @brief
     *
     */
    using CameraCallback = std::function<void(const CtrlMessage& ctrl_msg)>;

    /**
     * @brief Construct a new Camera Sink object
     *
     * @param socket
     */
    CameraSink(std::unique_ptr<UnixSocket> socket, CameraCallback cb = nullptr);

    /**
     * @brief Destroy the Camera Sink object
     *
     */
    ~CameraSink();

    /**
     * @brief Set the Callback Handler object
     *
     * @param cb
     * @return true
     * @return false
     */
    bool SetCallbackHandler(CameraCallback cb);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace client
} // namespace vhal