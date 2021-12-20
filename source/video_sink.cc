/**
 * @file video_sink.cc
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 * @brief
 * @version 1.0
 * @date 2021-04-27
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
#include "video_sink.h"
#include "video_sink_impl.h"
#include "unix_stream_socket_client.h"
#include "vsock_stream_socket_client.h"
#include <functional>
#include <memory>
#include <string>
#include <cstring>
#include <sys/types.h>

#define CAMERA_UNIX_SOCKET "/camera-socket"

namespace vhal {
namespace client {

VideoSink::VideoSink(UnixConnectionInfo unix_conn_info, CameraCallback callback)
{
    auto sockPath = unix_conn_info.socket_dir;
    if (sockPath.length() == 0) {
        throw std::invalid_argument("Please set a valid socket_dir");
    } else {
        sockPath += CAMERA_UNIX_SOCKET;
        if (unix_conn_info.android_instance_id >= 0) {
            sockPath += std::to_string(unix_conn_info.android_instance_id);
        }
    }

    //Creating interface to communicate to VHAL via libvhal
    auto unix_sock_client =
      std::make_unique<UnixStreamSocketClient>(std::move(sockPath));
    impl_ = std::make_unique<Impl>(std::move(unix_sock_client), callback);
}

VideoSink::VideoSink(VsockConnectionInfo vsock_conn_info, CameraCallback callback)
{

    if (vsock_conn_info.android_vm_cid == -1) {
        throw std::invalid_argument("Please set a valid socket_dir");
    }
    //Creating interface to communicate to VHAL via libvhal
    auto vsock_sock_client =
      std::make_unique<VsockStreamSocketClient>(std::move(vsock_conn_info.android_vm_cid));
    impl_ = std::make_unique<Impl>(std::move(vsock_sock_client), callback);
}

VideoSink::~VideoSink() {}

bool
VideoSink::IsConnected()
{
    return impl_->IsConnected();
}

IOResult VideoSink::SendDataPacket(const uint8_t* packet, size_t size)
{
    return impl_->SendDataPacket(packet, size);
}

IOResult VideoSink::SendRawPacket(const uint8_t* packet, size_t size)
{
    return impl_->SendRawPacket(packet, size);
}

std::shared_ptr<VideoSink::camera_capability_t>
VideoSink::GetCameraCapabilty()
{
    return impl_->GetCameraCapabilty();
}

bool
VideoSink::SetCameraCapabilty(camera_info_t *camera_info)
{
    return impl_->SetCameraCapabilty(camera_info);
}
}; // namespace client
} // namespace vhal
