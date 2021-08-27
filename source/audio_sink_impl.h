#ifndef AUDIO_SINK_IMPL_H
#define AUDIO_SINK_IMPL_H

/**
 * @file audio_sink_impl.h
 * @author  Nitisha Tomar (nitisha.tomar@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-07-27
 *
 * Copyright (c) 2021 Intel Corporation
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
 */

#include "istream_socket_client.h"
#include "audio_sink.h"
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

namespace vhal {
namespace client {
namespace audio {

class AudioSink::Impl
{
public:
    Impl(unique_ptr<IStreamSocketClient> socket_client)
      : socket_client_{ move(socket_client) }
    {
        vhal_talker_thread_ = thread([this]() {
            while (should_continue_) {
                if (not socket_client_->Connected()) {
                    auto [connected, error_msg] = socket_client_->Connect();
                    if (!connected) {
                        cout << "AudioSink Failed to connect to VHal: "
                             << error_msg
                             << ". Retry after 3ms...\n";
                        this_thread::sleep_for(3ms);
                        continue;
                    }
                }
                // connected ...
                cout << "Connected to Audio VHal (sink)!\n";

                struct pollfd fds[1];
                int           ret;

                // watch socket for input
                fds[0].fd     = socket_client_->GetNativeSocketFd();
                fds[0].events = POLLIN;

                do {
                    // Wait indefinitely for an event.
                    ret = poll(fds, std::size(fds), -1);
                    if (ret == -1) {
                        throw system_error(errno, system_category());
                    }
                    if (!ret) {
                        continue;
                    }
                    if (fds[0].revents & POLLIN) {
                        CtrlMessage ctrl_msg;
                        auto [received, recv_err_msg] =
                            socket_client_->Recv(
                            reinterpret_cast<uint8_t*>(&ctrl_msg),
                            sizeof(ctrl_msg));
                        if (received != sizeof(CtrlMessage)) {
                            cout << "Failed to read message from AudioSink: "
                                 << recv_err_msg
                                 << ", going to disconnect and reconnect.\n";
                            socket_client_->Close();
                            break;
                        }
                        // success, invoke client callback
                        callback_(cref(ctrl_msg));
                    } else {
                        if (fds[0].revents & (POLLERR|POLLHUP)) {
                            cout << "AudioSink Poll Fail event: "
                                << fds[0].revents
                                << ", reconnect\n";
                            socket_client_->Close();
                            break;
                        }
                        cout << "AudioSink : Poll revents " << fds[0].revents << "\n";
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

    bool RegisterCallback(AudioCallback callback)
    {
        callback_ = move(callback);
        return true;
    }

    IOResult SendDataPacket(const uint8_t* packet, size_t size)
    {
        auto [sent, error_msg] = socket_client_->Send(reinterpret_cast<const uint8_t*>(packet), size);
        if (sent == -1)
            return { sent, error_msg };
        // success
        return { size, "" };
    }

private:
    AudioCallback                   callback_ = nullptr;
    unique_ptr<IStreamSocketClient> socket_client_;
    thread                          vhal_talker_thread_;
    atomic<bool>                    should_continue_ = true;
};

} // namespace audio
} // namespace client
} // namespace vhal

#endif /* AUDIO_SINK_IMPL_H */
