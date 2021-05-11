#ifndef VHAL_VIDEO_SINK_H
#define VHAL_VIDEO_SINK_H
/**
 * @file vhal_video_sink.h
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
#include "istream_socket_client.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#include <tuple>

namespace vhal {
namespace client {

/**
 * @brief Class that acts as a pipe between Camera client and VHAL.
 * Camera client writes encoded video packet to the pipe and
 *
 */
class VHalVideoSink
{
public:
    using IOResult = std::tuple<ssize_t, std::string>;
    /**
     * @brief
     *
     */
    enum class VideoCodecType
    {
        kH264 = 0,
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
     * @brief Camera capabilities that needs to be supported by Remote camera.
     *
     */
    struct VideoParams
    {
        VideoCodecType  codec_type = VideoCodecType::kH264;
        FrameResolution resolution = FrameResolution::k480p;
        uint32_t        reserved[4];
    };

    /**
     * @brief Camera operation commands sent by Camera VHAL to client.
     *
     */
    enum class Command
    {
        kOpen  = 11,
        kClose = 12,
        kNone  = 13
    };

    /**
     * @brief Camera VHAL version.
     *
     */
    enum class VHalVersion
    {
        kV1 = 0, // decode out of camera vhal
        kV2 = 1, // decode in camera vhal
    };

    /**
     * @brief Camera control message sent by Camera VHAL to client.
     *
     */
    struct CtrlMessage
    {
        VHalVersion version = VHalVersion::kV2;
        Command     cmd     = Command::kNone;
        VideoParams video_params;
    };

    /**
     * @brief Type of the Camera callback which Camera VHAL triggers for
     * OpenCamera and CloseCamera cases.
     *
     */
    using CameraCallback = std::function<void(const CtrlMessage& ctrl_msg)>;

    /**
     * @brief Construct a new VHalVideoSink object
     *
     * @param socket_client Stream socket that handles VHAL transaction.
     * Currently only Stream socket types are supported.
     * @param callback Callback that is triggered by VHAL for OpenCamera and
     * CloseCamera cases.
     */
    VHalVideoSink(std::unique_ptr<IStreamSocketClient> socket_client);

    /**
     * @brief Destroy the VHalVideoSink object
     *
     */
    ~VHalVideoSink();

    /**
     * @brief Registers Camera callback.
     *
     * @param callback Camera callback function object or lambda or function
     * pointer.
     *
     * @return true Camera callback registered successfully.
     * @return false Camera callback failed to register.
     */
    bool RegisterCallback(CameraCallback callback);

    /**
     * @brief Write an encoded Camera packet to VHAL.
     *
     * @param packet Encoded Camera packet.
     * @param size Size of the Camera packet.
     *
     * @return ssize_t No of bytes written to VHAL, -1 if failure.
     */
    IOResult WritePacket(const uint8_t* packet, size_t size);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace client
} // namespace vhal
#endif /* VHAL_VIDEO_SINK_H */
