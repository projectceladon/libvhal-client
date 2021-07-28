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
         << " <filename> <socket-dir>\n"
            "\tExample: "
         << program_name
         << " test.h265 /opt/workdir/ipc/\n";
    return;
}

int
main(int argc, char** argv)
{
    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    string                  socket_path(argv[2]);
    string                  filename = argv[1];
    int                     instance_id = 0;
    atomic<bool>            stop     = false;
    thread                  file_src_thread;
    shared_ptr<VideoSink>   video_sink;

    UnixConnectionInfo conn_info = { socket_path, instance_id };
    try {
        video_sink = make_shared<VideoSink>(conn_info);
    } catch (const std::exception& ex) {
        cout << "VideoSink creation error :"
             << ex.what() << endl;
        exit(1);
    }

    cout << "Waiting Camera Open callback..\n";

    video_sink->RegisterCallback([&](const VideoSink::CtrlMessage& ctrl_msg) {
        switch (ctrl_msg.cmd) {
            case VideoSink::Command::kOpen: {
                cout << "Received Open command from Camera VHal\n";
                auto video_params = ctrl_msg.video_params;
                auto codec_type   = video_params.codec_type;
                auto frame_res    = video_params.resolution;

                // Make sure to interpret codec type and frame resolution and
                // provide video input that match these params.
                // For ex: Currently supported codec type is
                // `VideoSink::VideoCodecType::kH264`. If codec type is
                // kH264, then make sure to input only h264 packets. And if
                // frame resolution is k480p, make sure to have the same
                // resolution.

                // Start thread that is going to push video input
                file_src_thread = thread([&stop, video_sink, &filename]() {
                    // open file for reading
                    fstream istrm(filename, istrm.binary | istrm.in);
                    if (!istrm.is_open()) {
                        cout << "Failed to open " << filename << '\n';
                        exit(1);
                    }
                    cout << "Will start reading from file: " << filename
                         << '\n';
                    const size_t av_input_buffer_padding_size = 64;
                    const size_t inbuf_size                   = 4 * 1024;
                    array<uint8_t, inbuf_size + av_input_buffer_padding_size>
                      inbuf;
                    while (!stop) {
                        istrm.read(reinterpret_cast<char*>(inbuf.data()),
                                   inbuf_size); // binary input
                        if (!istrm.gcount() or istrm.eof()) {
                            //   istrm.clear();
                            //   istrm.seekg(0);
                            istrm.close();
                            istrm.open(filename, istrm.binary | istrm.in);
                            if (!istrm.is_open()) {
                                cout << "Failed to open " << filename << '\n';
                                exit(1);
                            }
                            cout << "Closed and re-opened file: " << filename
                                 << "\n";
                            continue;
                        }

                        // Send payload
                        if (auto [sent, error_msg] =
                              video_sink->SendDataPacket(inbuf.data(), inbuf_size);
                            sent < 0) {
                            cout << "Error in writing payload to Camera VHal: "
                                 << error_msg << "\n";
                            exit(1);
                        }
                        cout << "[rate=30fps] Sent " << istrm.gcount()
                             << " bytes to Camera VHal.\n";
                        // sleep for 33ms to maintain 30 fps
                        this_thread::sleep_for(33ms);
                    }
                });
                break;
            }
            case VideoSink::Command::kClose:
                cout << "Received Close command from Camera VHal\n";
                stop = true;
                file_src_thread.join();
                exit(0);
            default:
                cout << "Unknown Command received, exiting with failure\n";
                exit(1);
        }
    });

    // we need to be alive :)
    while (true) {
        this_thread::sleep_for(5ms);
    }

    return 0;
}
