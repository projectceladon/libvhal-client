/**
 *
 * Copyright (c) 2021-2022 Intel Corporation
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

#include "tcp_stream_socket_client.h"
#include "audio_source.h"
#include "android_audio_core.h"
#include <atomic>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using namespace std::chrono_literals;
using namespace std;
using namespace vhal::client::audio;
using namespace vhal::client;

static void
usage(string program_name)
{
    cout << "\tUsage:   " << program_name
         << " <android_instance_ip>\n"
         << "\tExample: "
         << program_name
         << " 172.100.0.2\n";
    return;
}

int
main(int argc, char** argv)
{
    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }

    string       ip_addr(argv[1]);
    atomic<bool> stop1      = false;
    atomic<bool> stop2      = false;
    const size_t inbuf_size = 1920;
    array<uint8_t, inbuf_size> inbuf;

    TcpConnectionInfo conn_info = { ip_addr };
    AudioSource audio_source(conn_info, [&](const CtrlMessage& ctrl_msg) {
        switch (ctrl_msg.cmd) {
        case Command::kOpen:{
            cout<<"Received Open command from Audio VHal\n";
            auto sampleRate = ctrl_msg.asci.sample_rate;
            auto channelNumber = ctrl_msg.asci.channel_count;
            auto bufferSizeInBytes = ctrl_msg.asci.frame_count *
                                     channelNumber *
                                     audio_bytes_per_sample(ctrl_msg.asci.format);
            break;
        }
        case Command::kData:{
            cout << "Received Data command from VHal\n";
            if (ctrl_msg.data_size > 0) {
                auto [size, error_msg] = audio_source.ReadDataPacket(inbuf.data(), ctrl_msg.data_size);
                if (size < 0) {
                    cout << "Error in writing payload to Audio VHal: "
                         << error_msg << "\n";
                    exit(1);
                }
                cout << "Recieved " << size
                     << " bytes from Audio VHal.\n";
              }
            break;
        }
        case Command::kClose:
            cout << "Received Close command \n";
            exit(0);
        default:
            cout << "Unknown Command received, exiting with failure\n";
            exit(1);
        }
    });
    cout << "Waiting Audio Open callback..\n";
    // we need to be alive :)
    while (true) {
        this_thread::sleep_for(5ms);
    }

    return 0;
}
