/**
 * @file video_sink.h
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-04-23
 *
 * Copyright (C) 2021-2022 Intel Corporation
 *
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
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VIDEO_SINK_H
#define VIDEO_SINK_H

#include "istream_socket_client.h"
#include "libvhal_common.h"
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
class VideoSink
{
public:


    /**
     * @brief protocol ack notification flags
     *
     */
    enum CameraAck {
        NACK_CONFIG =0,
        ACK_CONFIG = 1,
    };

    /**
     * @brief data transfer control cmd
     *
     */
    enum camera_packet_type_t : uint32_t {
        REQUEST_CAPABILITY = 0,
        CAPABILITY = 1,
        CAMERA_CONFIG = 2,
        CAMERA_DATA = 3,
        ACK = 4,
        CAMERA_INFO = 5,
        CAMERA_USER_ID = 6
    };

    /**
     * @brief header info to share host capabality
     * @size is sizeof(payload). Payload's size depends on the type of the data. See #camera_packet_type_t.
     *
     */
    struct camera_header_t
    {
        camera_packet_type_t type;
        uint32_t size; // number of cameras * sizeof(camera_info_t)
    };

    /**
     * @brief
     *
     */
    enum VideoCodecType : uint32_t {
        kH264 = 0x01,
        kH265 = 0x02,
        kI420 = 0x04
    };

    /**
     * @brief
     *
     */
    enum FrameResolution : uint32_t {
        k480p = 0x01,    // 640x480
        k720p = 0x02,   // 1280x720
        k1080p = 0x04  // 1920x1080
    };

    /**
     * Image sensor orientation info
     */
    enum SensorOrientation : uint32_t {
        ORIENTATION_0 = 0,
        ORIENTATION_90 = 90,
        ORIENTATION_180 = 180,
        ORIENTATION_270 = 270
    };

    /**
     * Camera facing info
     */
    enum CameraFacing : uint32_t {
        BACK_FACING = 0,
        FRONT_FACING = 1
    };

    /**
     * @brief Camera capabilities of the Android camera vHAL that
     * needs to be shared with client.
     */
    struct camera_capability_t
    {
        VideoCodecType codec_type = VideoCodecType::kH264;
        FrameResolution resolution = FrameResolution::k480p;
        uint32_t maxNumberOfCameras;
        uint32_t reserved[5];
    };

    /**
     * @brief Camera capability info of the remote camera requested to
     * camera vHAL.
     */
    struct camera_info_t {
        uint32_t cameraId;
        VideoCodecType codec_type;
        FrameResolution resolution;
        SensorOrientation sensorOrientation;
        CameraFacing facing;  // '0' for back camera and '1' for front camera
        uint32_t reserved[3];
    };

    /**
     * @brief Camera config is used to pass the camera parameters from
     * camera vHAL to client on every openCamera() call.
     */
    struct camera_config_t {
        uint32_t cameraId;
        VideoCodecType codec_type;
        FrameResolution resolution;
        uint32_t reserved[5];
    };

    /**
     * @brief encapsulated structure to exchange both data and control
     *
     */
    struct camera_packet_t {
        camera_header_t header;
        uint8_t *payload;
    };

    /**
     * @brief Camera operation commands sent by Camera VHAL to client.
     *
     */
    enum camera_cmd_t {
        CMD_OPEN  = 11,
        CMD_CLOSE = 12,
        CMD_NONE  = 15
    };

    /**
     * @brief Camera VHAL version.
     *
     */
    enum camera_version_t {
        CAMERA_VHAL_VERSION_1 = 0, // decode out of camera vhal
        CAMERA_VHAL_VERSION_2 = 1, // decode in camera vhal
    };


    /**
     * @brief Camera config cmd sent by camera vHAL to client.
     *
     */
    struct camera_config_cmd_t
    {
        camera_version_t version = camera_version_t::CAMERA_VHAL_VERSION_2;
        camera_cmd_t cmd = camera_cmd_t::CMD_NONE;
        camera_config_t camera_config;
    };

    /**
     * @brief Type of the Camera callback which Camera VHAL triggers for
     * OpenCamera and CloseCamera cases.
     *
     */
    using CameraCallback = std::function<void(const camera_config_cmd_t& ctrl_msg)>;

    /**
     * @brief Construct a default VideoSink object from the Android instance id.
     *        Throws std::invalid_argument excpetion.
     *
     * @param unix_conn_info Information needed to connect to the unix vhal socket.
     * @param callback Camera callback function object or lambda or function
     *        pointer.
     * @param user_id Optional parameter to specify the user# in multi-user config.
     *
     */
    VideoSink(UnixConnectionInfo unix_conn_info, CameraCallback callback, const int32_t user_id = -1);

    /**
     * @brief Construct a default VideoSink object from the Android vm cid.
     *        Throws std::invalid_argument excpetion.
     *
     * @param vsock_conn_info Information needed to connect to the vsock vhal socket.
     *
     */

    VideoSink(VsockConnectionInfo vsock_conn_info, CameraCallback callback);

    /**
     * @brief Destroy the VideoSink object
     *
     */
    ~VideoSink();

    /**
     * @brief Returns Camera vhal connection status.
     *
     * @return true Connected to Camera vhal.
     * @return false Not Connected to Camera vhal.
     */
    bool IsConnected();

    /**
     * @brief Send an encoded Camera packet to VHAL.
     *
     * This function sends data packet with the header containing size of the data
     * packet. It's call is equivalent to the following 2 calls of SendRawPacket:
     *
     * \code
     * SendRawPacket((uint8_t*)&size, sizeof(size));
     * SendRawPacket(packet, size);
     * \endcode
     *
     * @param packet Encoded Camera packet.
     * @param size Size of the Camera packet.
     *
     * @return ssize_t No of bytes written to VHAL, -1 if failure.
     * @return IOResult tuple<ssize_t, std::string>.
     *         ssize_t No of bytes sent and -1 incase of failure
     *         string is the status message.
     */
    IOResult SendDataPacket(const uint8_t* packet, size_t size);

    /**
     * @brief Send an raw Camera packet to VHAL for cases like I420
     *        where data is fixed always. when using this api both
     *        vhal and libvHAL-client knows packet size prior
     *
     * @param packet raw Camera packet.
     * @param size Size of the Camera packet.
     *
     * @return ssize_t No of bytes written to VHAL, -1 if failure.
     * @return IOResult tuple<ssize_t, std::string>.
     *         ssize_t No of bytes sent and -1 incase of failure
     *         string is the status message.
     */
    IOResult SendRawPacket(const uint8_t* packet, size_t size);

    /**
     * @brief GetCameraCapabilty
     *        api is called to get vhal capability
     *        client should call this api after successful connection 
     *
     * @return camera_capability_t which provides vhal capabilites
     * @return NULL on failure
     */
    std::shared_ptr<camera_capability_t> GetCameraCapabilty();

    /**
     * @brief SetCameraCapability() API is called to set client
     *        requested capability to camera vHAL
     *
     * @param camera_info_t
     *
     * @return true libvhal able to send camera info
     * @return false libvhal failed to send camera info
     */
    bool SetCameraCapabilty(std::vector<camera_info_t> camera_info);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace client
} // namespace vhal
#endif /* VIDEO_SINK_H */
