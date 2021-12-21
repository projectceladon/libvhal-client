/**
 * @file camera_client.cc
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

#include "unix_stream_socket_client.h"
#include "video_sink.h"
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using namespace std::chrono_literals;
using namespace vhal::client;
using namespace std;

static void
usage(string program_name)
{
    cout << "\tUsage:   " << program_name
         << " <filename> <vhal-sock-path>\n"
            "\tExample: "
         << program_name
         << " test.h265 /ipc/camdec-sock-0"
            "frames\n";
    return;
}

int
main(int argc, char** argv)
{
    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    string        socket_path(argv[2]);
    string        filename = argv[1];
    CameraBackend camera_backend;

    auto xx_sock_client = make_unique<XXStreamSocketClient>(move(socket_path));

    VideoSink video_sink(move(xx_sock_client));
      [&](const VideoSink::CtrlMessage& ctrl_msg) {
        switch (ctrl_msg.cmd) {
            case VideoSink::Command::kOpen:
                cout << "Received Open command from Camera VHal\n";
                auto video_params = ctrl_msg.video_params;
                auto codec_type   = video_params.codec_type;
                auto frame_res    = video_params.resolution;
                // Request Backend to share camera data
                // with above video parames: codec type and frame resolution.

                camera_backend.RegisterCallback(
                  codec_type,
                  frame_res,
                  [&video_sink]() {
                      if (codec_type != VideoSink::VideoCodecType::I420) {
                          // Write payload size
                          if (auto [sent, error_msg] =
                                video_sink.WritePacket((uint8_t*)&inbuf_size,
                                                       sizeof(inbuf_size));
                              sent < 0) {
                              cout << "Error in writing payload size to "
                                      "Camera VHal: "
                                   << error_msg << "\n";
                              exit(1);
                          }
                      }

                      // Write payload
                      if (auto [sent, error_msg] =
                            video_sink.WritePacket(inbuf.data(), inbuf_size);
                          sent < 0) {
                          cout << "Error in writing payload to Camera VHal: "
                               << error_msg << "\n";
                          exit(1);
                      }
                      cout << "[rate=30fps] Sent " << istrm.gcount()
                           << " bytes to Camera VHal.\n";
                  });
                break;
            case VideoSink::Command::kClose:
                cout << "Received Close command from Camera VHal\n";
                camera_backend(nullptr);
                exit(0);
            default:
                cout << "Unknown Command received, exiting with failure\n";
                exit(1);
        }
    });

    cout << "Waiting Camera Open callback..\n";

    // we need to be alive :)
    while (true) {
        this_thread::sleep_for(5ms);
    }

    return 0;
}
