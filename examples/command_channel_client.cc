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
#include "command_channel_interface.h"
#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using namespace std::chrono_literals;
using namespace std;
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

    TcpConnectionInfo conn_info = { ip_addr };
    CommandChannelInterface command_channel_interface(conn_info,
      [&](const CommandChannelMessage& command_channel_msg) {
        switch (command_channel_msg.msg_type) {
            case MsgType::kActivityMonitor: {
                string msg((char*)command_channel_msg.data, command_channel_msg.data_size);
                cout << "Recevied msg: " << msg << endl;
                break;
            }
            case MsgType::kAicCommand: {
                string msg((char*)command_channel_msg.data, command_channel_msg.data_size);
                cout << "Recevied msg: " << msg << endl;
                break;
            }
            default:
                cout << "Unknown MstType received, exiting with failure\n";
                exit(1);
        }
    });

    cout << "Waiting Command Channel callback..\n";

    this_thread::sleep_for(1s);
    MsgType message_type;
    string cmd;
    IOResult ior;
    int sent;

    cout << "start com.android.gallery3d\n";
    message_type = MsgType::kActivityMonitor;
    cmd = "0:com.android.gallery3d:0";
    ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }
    this_thread::sleep_for(3s);

    //Test input
    cout << "input keyevent 4\n";
    message_type = MsgType::kAicCommand;
    cmd = "input keyevent 4";
    ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }
    this_thread::sleep_for(3s);

    //Test pm
    cout << "pm list packages\n";
    message_type = MsgType::kAicCommand;
    cmd = "pm list packages";
    ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }
    this_thread::sleep_for(3s);

    //Test setprop
    cout << "setprop command_channel_test 3\n";
    message_type = MsgType::kAicCommand;
    cmd = "setprop command_channel_test 3";
    ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }
    this_thread::sleep_for(3s);

    //Test am
    cout << "am start com.android.settings\n";
    message_type = MsgType::kAicCommand;
    cmd = "am start com.android.settings";
    ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }
    this_thread::sleep_for(3s);

    //Test dumpsys
    cout << "dumpsys SurfaceFlinger\n";
    message_type = MsgType::kAicCommand;
    cmd = "dumpsys SurfaceFlinger";
    ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }
    this_thread::sleep_for(3s);

    //Test monkey
    cout << "monkey -p com.android.gallery3d -s 10 10000\n";
    message_type = MsgType::kAicCommand;
    cmd = "monkey -p com.android.gallery3d -s 10 10000";
    ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }

    this_thread::sleep_for(3s);
    return 0;
}
