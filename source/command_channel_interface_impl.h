/**
 * @file command_channel_interface_impl.h
 * @author  Kai Liu (kai1.liu@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-09-13
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

#ifndef COMMAND_CHANNEL_INTERFACE_IMPL_H
#define COMMAND_CHANNEL_INTERFACE_IMPL_H

#include "istream_socket_client.h"
#include "command_channel_interface.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
extern "C"
{
#include <sys/poll.h>
#include <sys/types.h>
#include <unistd.h>
}
#include <thread>
#include <tuple>

using namespace std;
using namespace chrono_literals;

#define CMD_CHANNEL_MSG_SIZE_MAX 65536

namespace vhal {
namespace client {

class CommandChannelInterface::Impl
{
public:
    Impl(unique_ptr<IStreamSocketClient> ams_socket_client,
         unique_ptr<IStreamSocketClient> acs_socket_client,
         unique_ptr<IStreamSocketClient> ftc_socket_client,
         CommandChannelCallback callback)
      : ams_socket_client_{ move(ams_socket_client) },
        acs_socket_client_{ move(acs_socket_client) },
        ftc_socket_client_{ move(ftc_socket_client) },
        callback_{ move(callback) }
    {
        ams_talker_thread_ = thread([this]() {
            while (should_continue_) {
                if (not ams_socket_client_->Connected()) {
                    auto [connected, error_msg] = ams_socket_client_->Connect();
                    if (!connected) {
                        cout << "CommandChannelInterface Failed to connect to Activity Monitor Service: "
                             << error_msg
                             << ". Retry after 1s...\n";
                        this_thread::sleep_for(1s);
                        continue;
                    }
                }
                // connected ...
                cout << "Connected to Activity Monitor Service!\n";

                struct pollfd fds[1];
                const int     timeout_ms = 1 * 1000; // 1 sec timeout
                int           ret;

                // watch socket for input
                fds[0].fd     = ams_socket_client_->GetNativeSocketFd();
                fds[0].events = POLLIN;

                do {
                    ret = poll(fds, std::size(fds), timeout_ms);
                    if (ret == -1) {
                        throw system_error(errno, system_category());
                    }
                    if (!ret) {
                        continue;
                    }
                    if (fds[0].revents & POLLIN) {
                        CommandChannelMessage command_channel_msg;
                        int msg_length = 0;
                        IOResult ior = ams_socket_client_->Recv(
                            reinterpret_cast<uint8_t*>(&msg_length),
                            sizeof(int));
                        int received = std::get<0>(ior);
                        if (received != sizeof(int) || msg_length <= 0 || msg_length > CMD_CHANNEL_MSG_SIZE_MAX) {
                            cout << "Failed to read message from Activity Monitor Service: "
                                 << std::get<1>(ior)
                                 << ", going to disconnect and reconnect.\n";
                            ams_socket_client_->Close();
                            break;
                        }

                        if ((size_t)msg_length > ams_client_buf_.size()) {
                            ams_client_buf_.resize(msg_length);
                        }

                        ior = ams_socket_client_->Recv(
                            &ams_client_buf_[0],
                            msg_length);
                        received = std::get<0>(ior);
                        if (received != msg_length) {
                            cout << "Failed to read message from Activity Monitor Service: "
                                 << std::get<1>(ior)
                                 << ", going to disconnect and reconnect.\n";
                            ams_socket_client_->Close();
                            break;
                        }
                        // success, invoke client callback
                        command_channel_msg.msg_type = MsgType::kActivityMonitor;
                        command_channel_msg.data = &ams_client_buf_[0];
                        command_channel_msg.data_size = msg_length;
                        callback_(cref(command_channel_msg));
                    } else {
                        if (fds[0].revents & (POLLERR|POLLHUP)) {
                            cout << "CommandChannelInterface Poll Fail event: "
                                << fds[0].revents
                                << ", reconnect\n";
                            ams_socket_client_->Close();
                            break;
                        }
                        cout << "CommandChannelInterface : Poll revents " << fds[0].revents << "\n";
                    }
                } while (should_continue_);
            }
        });

        acs_talker_thread_ = thread([this]() {
            while (should_continue_) {
                if (not acs_socket_client_->Connected()) {
                    auto [connected, error_msg] = acs_socket_client_->Connect();
                    if (!connected) {
                        cout << "CommandChannelInterface Failed to connect to Aic Command Service: "
                             << error_msg
                             << ". Retry after 1s...\n";
                        this_thread::sleep_for(1s);
                        continue;
                    }
                }
                // connected ...
                cout << "Connected to Aic Command Service!\n";

                struct pollfd fds[1];
                const int     timeout_ms = 1 * 1000; // 1 sec timeout
                int           ret;

                // watch socket for input
                fds[0].fd     = acs_socket_client_->GetNativeSocketFd();
                fds[0].events = POLLIN;

                do {
                    ret = poll(fds, std::size(fds), timeout_ms);
                    if (ret == -1) {
                        throw system_error(errno, system_category());
                    }
                    if (!ret) {
                        continue;
                    }
                    if (fds[0].revents & POLLIN) {
                        CommandChannelMessage command_channel_msg;
                        int msg_length = 0;
                        IOResult ior = acs_socket_client_->Recv(
                            reinterpret_cast<uint8_t*>(&msg_length),
                            sizeof(int));
                        int received = std::get<0>(ior);
                        if (received != sizeof(int) || msg_length <= 0 || msg_length > CMD_CHANNEL_MSG_SIZE_MAX) {
                            cout << "Failed to read message from Aic Command Service: "
                                 << std::get<1>(ior)
                                 << ", going to disconnect and reconnect.\n";
                            acs_socket_client_->Close();
                            break;
                        }

                        if ((size_t)msg_length > acs_client_buf_.size()) {
                            acs_client_buf_.resize(msg_length);
                        }

                        ior = acs_socket_client_->Recv(
                            &acs_client_buf_[0],
                            msg_length);
                        received = std::get<0>(ior);
                        if (received != msg_length) {
                            cout << "Failed to read message from Aic Command Service: "
                                 << std::get<1>(ior)
                                 << ", going to disconnect and reconnect.\n";
                            acs_socket_client_->Close();
                            break;
                        }
                        // success, invoke client callback
                        command_channel_msg.msg_type = MsgType::kAicCommand;
                        command_channel_msg.data = &acs_client_buf_[0];
                        command_channel_msg.data_size = msg_length;
                        callback_(cref(command_channel_msg));
                    } else {
                        if (fds[0].revents & (POLLERR|POLLHUP)) {
                            cout << "CommandChannelInterface Poll Fail event: "
                                << fds[0].revents
                                << ", reconnect\n";
                            acs_socket_client_->Close();
                            break;
                        }
                        cout << "CommandChannelInterface : Poll revents " << fds[0].revents << "\n";
                    }
                } while (should_continue_);
            }
        });

        ftc_talker_thread_ = thread([this]() {
            while (should_continue_) {
                if (not ftc_socket_client_->Connected()) {
                    auto [connected, error_msg] = ftc_socket_client_->Connect();
                    if (!connected) {
                        cout << "CommandChannelInterface Failed to connect to "
                                "Aic Command Service: "
                             << error_msg << ". Retry after 1s...\n";
                        this_thread::sleep_for(1s);
                        continue;
                    }
                }
                // connected ...
                cout << "Connected to File Transfer Service!\n";

                struct pollfd fds[1];
                const int     timeout_ms = 1 * 1000; // 1 sec timeout
                int           ret;

                // watch socket for input
                fds[0].fd     = ftc_socket_client_->GetNativeSocketFd();
                fds[0].events = POLLIN;

                do {
                    ret = poll(fds, std::size(fds), timeout_ms);
                    if (ret == -1) {
                        throw system_error(errno, system_category());
                    }
                    if (!ret) {
                        continue;
                    }
                    if (fds[0].revents & POLLIN) {
                        CommandChannelMessage command_channel_msg;
                        int                   msg_length = 0;
                        IOResult              ior = ftc_socket_client_->Recv(
                          reinterpret_cast<uint8_t*>(&msg_length),
                          sizeof(int));
                        int received = std::get<0>(ior);
                        if (received != sizeof(int) || msg_length <= 0 || msg_length > CMD_CHANNEL_MSG_SIZE_MAX) {
                            cout << "Failed to read message from Aic Command "
                                    "Service: "
                                 << std::get<1>(ior)
                                 << ", going to disconnect and reconnect.\n";
                            ftc_socket_client_->Close();
                            break;
                        }

                        if ((long unsigned int)msg_length > acs_client_buf_.size()) {
                            acs_client_buf_.resize(msg_length);
                        }

                        ior      = ftc_socket_client_->Recv(&acs_client_buf_[0],
                                                       msg_length);
                        received = std::get<0>(ior);
                        if (received != msg_length) {
                            cout << "Failed to read message from Aic Command "
                                    "Service: "
                                 << std::get<1>(ior)
                                 << ", going to disconnect and reconnect.\n";
                            ftc_socket_client_->Close();
                            break;
                        }
                        // success, invoke client callback
                        command_channel_msg.msg_type  = MsgType::kFileTransfer;
                        command_channel_msg.data      = &acs_client_buf_[0];
                        command_channel_msg.data_size = msg_length;
                        callback_(cref(command_channel_msg));
                    } else {
                        if (fds[0].revents & (POLLERR | POLLHUP)) {
                            cout << "CommandChannelInterface Poll Fail event: "
                                 << fds[0].revents << ", reconnect\n";
                            ftc_socket_client_->Close();
                            break;
                        }
                        cout << "CommandChannelInterface : Poll revents "
                             << fds[0].revents << "\n";
                    }
                } while (should_continue_);
            }
        });
    }

    ~Impl()
    {
        should_continue_ = false;
        ams_talker_thread_.join();
        acs_talker_thread_.join();
        ftc_talker_thread_.join();
    }

    IOResult SendDataPacket(MsgType msg_type, const uint8_t* message, size_t size)
    {
        int message_length = static_cast<int>(size);
        int sent = -1;
        IOResult ior;
        if (msg_type == MsgType::kActivityMonitor) {
            ior =
                ams_socket_client_->Send(reinterpret_cast<const uint8_t*>(&message_length), sizeof(int));
            sent = std::get<0>(ior);
            if (sent == -1)
                return ior;

            ior = ams_socket_client_->Send(reinterpret_cast<const uint8_t*>(message), size);
            sent = std::get<0>(ior);
            if (sent == -1)
                return ior;
        } else if (msg_type == MsgType::kAicCommand) {
            ior =
                acs_socket_client_->Send(reinterpret_cast<const uint8_t*>(&message_length), sizeof(int));
            sent = std::get<0>(ior);
            if (sent == -1)
                return ior;

            ior = acs_socket_client_->Send(reinterpret_cast<const uint8_t*>(message), size);
            sent = std::get<0>(ior);
            if (sent == -1)
                return ior;
        } else if (msg_type == MsgType::kFileTransfer) {
            ior = ftc_socket_client_->Send(reinterpret_cast<const uint8_t*>(&message_length), sizeof(int));
            sent = std::get<0>(ior);
            if (sent == -1)
                return ior;

            ior = ftc_socket_client_->Send(reinterpret_cast<const uint8_t*>(message), size);
            sent = std::get<0>(ior);
            if (sent == -1)
                return ior;
        }
        // success
        return { size, "" };
    }

private:
    unique_ptr<IStreamSocketClient> ams_socket_client_;
    unique_ptr<IStreamSocketClient> acs_socket_client_;
    unique_ptr<IStreamSocketClient> ftc_socket_client_;
    CommandChannelCallback          callback_ = nullptr;
    thread                          ams_talker_thread_;
    thread                          acs_talker_thread_;
    thread                          ftc_talker_thread_;
    atomic<bool>                    should_continue_ = true;
    std::vector<uint8_t>            ams_client_buf_ = std::vector<uint8_t>(1024);
    std::vector<uint8_t>            acs_client_buf_ = std::vector<uint8_t>(1024);
};

} // namespace client
} // namespace vhal

#endif /* COMMAND_CHANNEL_INTERFACE_IMPL_H */
