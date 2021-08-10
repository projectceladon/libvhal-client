#ifndef VIDEO_SINK_IMPL_H
#define VIDEO_SINK_IMPL_H
/**
 * @file video_sink_impl.h
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
#include "istream_socket_client.h"
#include "video_sink.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <system_error>
extern "C"
{
#include <sys/poll.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
}
#include <thread>
#include <tuple>
#include <mutex>
#include <condition_variable>

using namespace std;
using namespace chrono_literals;

namespace vhal {
namespace client {

class VideoSink::Impl
{
public:
    Impl(unique_ptr<IStreamSocketClient> socket_client)
      : socket_client_{ move(socket_client) }
    {
        vhal_talker_thread_ = thread([this]() {
            while (should_continue_) {
                if (not socket_client_->Connected()) {
                    if (auto [connected, error_msg] = socket_client_->Connect();
                        !connected) {
                        cout << "VideoSink Failed to connect to VHal: "
                             << error_msg
                             << ". Retry after 3ms...\n";
                        this_thread::sleep_for(3ms);
                        continue;
                    }
                    cout << "Connected to Camera VHal!\n";
                }

                cmd_capability_ = new camera_capability_t();
                // connected ...
                do {
                    cout << "Camera VHal has some message for us!\n";

                    size_t header_size = sizeof(camera_header_t);
                    camera_header_t cmd_header;
                    std::tuple<ssize_t, std::string> response;
  
                    response = socket_client_->Recv(
                        reinterpret_cast<uint8_t*>(&cmd_header),
                        header_size, MSG_WAITALL);
                    if (get<0>(response) != header_size) {
                        cout << "Failed to read message from VideoSink: "
                        << get<1>(response)
                        << ", going to disconnect and reconnect.\n";
                        socket_client_->Close();
                        // FIXME: What to do ?? Exit ?
                        continue;
                    }
                    switch(cmd_header.type) {
                        case camera_packet_type_t::CAPABILITY:
                            cout <<"received capability" <<"\n";
                            if(!handle_capability())
                                continue;
                            break;

                        case camera_packet_type_t::ACK:
                            cout <<"received ack" <<"\n";
                            if(!handle_ack())
                                continue;
                            break;

                        case camera_packet_type_t::CAMERA_CONFIG:
                            cout <<"received config" <<"\n";
                            if(!handle_cmd())
                                continue;
                            break;
                        default :
                            cout <<"invalid header type received "<<"\n";
                            break;
                    }
                } while (should_continue_);
            }
        });
    }

    ~Impl()
    {
        should_continue_ = false;
        vhal_talker_thread_.join();
    }

    bool RegisterCallback(CameraCallback callback)
    {
        callback_ = move(callback);
        return true;
    }

    IOResult SendDataPacket(const uint8_t* packet, size_t size)
    {
        std::string result_error_msg = "";

        std::tuple<ssize_t, std::string> response;
        // Write payload size
        response = socket_client_->Send((uint8_t*)&size, sizeof(size));
        if (get<0>(response) == -1) {
                get<1>(response) = "Error in writing payload size to Camera VHal: "
                  + get<1>(response);
                return response;
            }
        // Write payload
        response = socket_client_->Send(packet, size);
        if (get<0>(response) == -1) {
                get<1>(response) = "Error in writing payload to Camera VHal: "
                  + get<1>(response);
                return response;
            }

        // success
        return response;
    }

    IOResult SendRawPacket(const uint8_t* packet, size_t size)
    {
      	std::tuple<ssize_t, std::string> response;

        // Write payload
        response = socket_client_->Send(packet, size);
        if (get<0>(response) == -1) {
                get<1>(response) = "Error in writing payload to Camera VHal: "
                  + get<1>(response);
                return response;
        }

        // success
        return response;
    }

    camera_capability_t* GetCameraCapabilty()
    {
        std::tuple<ssize_t, std::string> response;
        camera_header_t header_packet;

        header_packet.type = VideoSink::camera_packet_type_t::REQUST_CAPABILITY;
        response = SendRawPacket((unsigned char*)&header_packet, sizeof(camera_header_t));
        if (get<0>(response) == -1) {
            get<1>(response) = "Error in sending request capability header to Camera VHal: "
              + get<1>(response);
            return NULL;
        }

        std::unique_lock<std::mutex> lck(mutex_);
        wait_api_data.wait(lck);

        cout << " returning GetCameraCapabilty result" << "\n";
        return cmd_capability_;
    }

    bool SetCameraCapabilty(camera_capability_t *camera_config)
    {
        std::tuple<ssize_t, std::string> response;
        
        camera_header_t header_packet;
        header_packet.type = camera_packet_type_t::CAMERA_CONFIG;
        header_packet.size = sizeof(camera_capability_t);

        response = SendRawPacket((unsigned char*)&header_packet, sizeof(camera_header_t));
        if (get<0>(response) == -1) {
            get<1>(response) = "Error in sending config header to Camera VHal: "
              + get<1>(response);
            return false;
        }

        response = SendRawPacket((unsigned char*)camera_config, sizeof(camera_capability_t));
        if (get<0>(response) == -1) {
            get<1>(response) = "Error in sending config to Camera VHal: "
              + get<1>(response);
            return false;
        }

        std::unique_lock<std::mutex> lck(mutex_);
        wait_api_data.wait(lck);
        cout << " returning SetCameraCapabilty result" << "\n";

        return true;
    }

    bool handle_ack()
    {
        size_t ack_pkt_size = sizeof(CameraAck);
        std::tuple<ssize_t, std::string> response;

        CameraAck ack_pkt;
        response = socket_client_->Recv(
        reinterpret_cast<uint8_t*>(&ack_pkt),
        ack_pkt_size, MSG_WAITALL);
        if(get<0>(response) != ack_pkt_size) {
            cout << "Failed to read message from VideoSink: "
            << get<1>(response)
            << ", going to disconnect and reconnect.\n";
            socket_client_->Close();
            return false;
        }
        wait_api_data.notify_one();
        return true;
    }
    bool handle_capability()
    {
        size_t capability_pkt_size = sizeof(camera_capability_t);
        std::tuple<ssize_t, std::string> response;

        response = socket_client_->Recv(
        reinterpret_cast<uint8_t*>(cmd_capability_),
        capability_pkt_size, MSG_WAITALL);
        if (get<0>(response) != capability_pkt_size) {
            cout << "Failed to read message from VideoSink: "
            << get<1>(response)
            << ", going to disconnect and reconnect.\n";
            socket_client_->Close();
            return false;
            // FIXME: What to do ?? Exit ?
        }
        cout <<"params codec type "<<cmd_capability_->codec_type <<"resolution"<<cmd_capability_->resolution<<"\n";
        wait_api_data.notify_one();

        return true;
    }
    bool handle_cmd()
    {
        size_t cmd_pkt_size = sizeof(camera_config_cmd_t);
        std::tuple<ssize_t, std::string> response;

        camera_config_cmd_t cmd_pkt;
        response = socket_client_->Recv(
        reinterpret_cast<uint8_t*>(&cmd_pkt),
        cmd_pkt_size, MSG_WAITALL);
        if(get<0>(response) != cmd_pkt_size) {
            cout << "Failed to read message from VideoSink: "
            << get<1>(response)
            << ", going to disconnect and reconnect.\n";
            socket_client_->Close();
            return false;
            // FIXME: What to do ?? Exit ?
        }

        cout << "camera cmd received "<< (int)cmd_pkt.cmd <<"\n";
        callback_(cref(cmd_pkt));
        return true;
    }


private:
    CameraCallback                  callback_ = nullptr;
    unique_ptr<IStreamSocketClient> socket_client_;
    thread                          vhal_talker_thread_;
    atomic<bool>                    should_continue_ = true;

    camera_capability_t *cmd_capability_;
    std::mutex mutex_;
    std::condition_variable wait_api_data;
};

} // namespace client
} // namespace vhal

#endif /* VHAL_VIDEO_SINK_IMPL_H */
