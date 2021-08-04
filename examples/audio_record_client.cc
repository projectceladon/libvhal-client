#include "tcp_stream_socket_client.h"
#include "audio_sink.h"
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
         << " <filename> <android_instance_ip>\n"
         << "\tExample: "
         << program_name
         << " s_48K_16Bit_Sunshine.wav 172.100.0.2\n";
    return;
}

int
main(int argc, char** argv)
{
    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    string       ip_addr(argv[2]);
    string       filename = argv[1];
    atomic<bool> stop     = false;
    thread       file_src_thread;

    TcpConnectionInfo conn_info = { ip_addr };
    AudioSink audio_sink(conn_info);
    cout << "Waiting Audio Open callback..\n";

    audio_sink.RegisterCallback([&](const CtrlMessage& ctrl_msg) {
        switch (ctrl_msg.cmd) {
            case Command::kOpen: {
                cout << "Received Open command from Audio VHal\n";
                auto sample_rate = ctrl_msg.asci.sample_rate;
                auto channel_count   = ctrl_msg.asci.channel_count;
                auto bufferSizeInBytes = ctrl_msg.asci.frame_count *
                                         channel_count *
                                         audio_bytes_per_sample(ctrl_msg.asci.format);
                // Start thread that is going to push audio input
                file_src_thread = thread([&stop, &audio_sink, &filename]() {
                    // open file for reading
                    fstream istrm(filename, istrm.binary | istrm.in);
                    if (!istrm.is_open()) {
                        cout << "Failed to open " << filename << '\n';
                        exit(1);
                    }
                    cout << "Will start reading from file: " << filename
                         << '\n';
                    const size_t inbuf_size                   = 1920;
                    array<uint8_t, inbuf_size> inbuf;
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

                        // Write payload
                        if (auto [sent, error_msg] =
                              audio_sink.SendDataPacket(inbuf.data(), inbuf_size);
                            sent < 0) {
                            cout << "Error in writing payload to Audio VHal: "
                                 << error_msg << "\n";
                            exit(1);
                        }
                        cout << "Sent " << istrm.gcount()
                             << " bytes to Audio VHal.\n";
                        this_thread::sleep_for(10ms);
                    }
                });
                break;
            }
            case Command::kClose:
                cout << "Received Close command from VHal\n";
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

