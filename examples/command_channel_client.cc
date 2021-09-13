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
    CommandChannelInterface command_channel_interface(conn_info);
    cout << "Waiting Command Channel callback..\n";

    command_channel_interface.RegisterCallback([&](const CommandChannelMessage& command_channel_msg) {
        switch (command_channel_msg.msg_type) {
            case MsgType::kActivityMonitor: {
                string msg((char*)command_channel_msg.data, command_channel_msg.data_size);
                cout << "Recevied msg: " << msg << endl;
                if (command_channel_msg.data != nullptr)
                    delete [] command_channel_msg.data;
                break;
            }
            case MsgType::kAicCommand: {
                string msg((char*)command_channel_msg.data, command_channel_msg.data_size);
                cout << "Recevied msg: " << msg << endl;
                if (command_channel_msg.data != nullptr)
                    delete [] command_channel_msg.data;
                break;
            }
            default:
                cout << "Unknown MstType received, exiting with failure\n";
                exit(1);
        }
    });

    this_thread::sleep_for(1s);
    cout << "start com.android.settings\n";
    MsgType message_type = MsgType::kActivityMonitor;
    string cmd = "0:com.android.settings";
    IOResult ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    int sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }
    this_thread::sleep_for(1s);

    cout << "start com.android.gallery3d\n";
    message_type = MsgType::kActivityMonitor;
    cmd = "0:com.android.gallery3d";
    ior = command_channel_interface.SendDataPacket(message_type,
        reinterpret_cast<const uint8_t*>(cmd.c_str()), cmd.length());
    sent = std::get<0>(ior);
    if (sent < 0) {
        cout << "Error in writing payload to Command Channel server: "
             << std::get<1>(ior) << "\n";
        exit(1);
    }
    this_thread::sleep_for(1s);

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
    this_thread::sleep_for(1s);

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

    cout << "start com.android.launcher3\n";
    message_type = MsgType::kActivityMonitor;
    cmd = "0:com.android.launcher3";
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
