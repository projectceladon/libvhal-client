
#include "unix_stream_socket_client.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>

using namespace std::chrono_literals;
using namespace std;
using namespace vhal::client;

// This definition should be aligned with camera vHAL

struct camera_socket_configuration_info_t
{
    uint32_t codec_type;
    uint32_t resolution;
    uint32_t reserved[4];
};

enum camera_cmd_t
{
    CMD_OPEN_CAMERA  = 11,
    CMD_CLOSE_CAMERA = 12,
};

enum camera_vhal_version_t
{
    CAMERA_VHAL_VERSION_0 = 0, // decode out of camera vhal
    CAMERA_VHAL_VERSION_1 = 1, // decode in camera vhal
};

struct camera_socket_info_t
{
    camera_vhal_version_t              version;
    camera_cmd_t                       cmd;
    camera_socket_configuration_info_t config;
};

#define AV_INPUT_BUFFER_PADDING_SIZE 64

#define INBUF_SIZE (4 * 1024)

static void
usage(char** argv)
{
    fprintf(stderr,
            "\tUsage:   %s <filename> <vhal-sock-path>\n"
            "\tExample: %s test.h265 /ipc/camdec-sock-0"
            "frames\n",
            argv[0],
            argv[0]);
    return;
}

int
main(int argc, char** argv)
{
    if (argc < 3) {
        usage(argv);
        exit(1);
    }
    const char* filename = argv[1];

    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    try {
        UnixStreamSocketClient socket_client(argv[2]);

        auto [connected, conn_err_msg] = socket_client.Connect();
        if (!connected) {
            std::cout << "Connect() failed due to " << conn_err_msg << "\n";
            exit(EXIT_FAILURE);
        }

        std::cout << "Waiting for CMD_OPEN_CAMERA\n";

        camera_socket_info_t camera_sock_info;

        auto [received, recv_err_msg] =
          socket_client.Recv(reinterpret_cast<uint8_t*>(&camera_sock_info),
                             sizeof(camera_sock_info));
        if (received <= 0) {
            cout << "Recv() failed due to " << recv_err_msg << "\n";
            exit(EXIT_FAILURE);
        }
        if (camera_sock_info.cmd == CMD_OPEN_CAMERA) {
            cout << "Received CMD_OPEN_CAMERA\n";

            const size_t kSize = INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE;
            std::array<uint8_t, kSize> inbuf = { 0 };
            ssize_t                    data_size;

            while (true) {
                data_size = fread(inbuf.data(), 1, kSize, f);
                if (data_size == 0) {
                    if (feof(f)) {
                        cout << "File eof reached, restart\n";
                        rewind(f);
                        continue;
                    } else if (ferror(f)) {
                        cout << "File error occurred, exitint error "
                             << strerror(errno) << "\n";
                        break;
                    }
                }
                // send size
                if (auto [sent, send_err_msg] =
                      socket_client.Send((uint8_t*)&data_size, sizeof(size_t));
                    sent != sizeof(size_t)) {
                    cout << "Send() failed due to " << send_err_msg << "\n";
                    break;
                }
                cout << "Sent " << sizeof(size_t) << " bytes to VHal\n";
                // send data
                if (auto [sent, send_err_msg] =
                      socket_client.Send(inbuf.data(), data_size);
                    sent != data_size) {
                    cout << "Send() failed due to " << send_err_msg << "\n";
                    break;
                }
                cout << "Sent " << data_size << " bytes to VHal\n";
                cout << ">>>>>> Sending frames at 30fps...\n";
                std::this_thread::sleep_for(33ms);
            }
        } else if (camera_sock_info.cmd == CMD_CLOSE_CAMERA) {
            cout << "Received CMD_CLOSE_CAMERA, exit\n";
        }

        fclose(f);
    } catch (const std::system_error& error) {
        std::cout << "Error: " << error.code() << " - "
                  << error.code().message() << '\n';
    }

    return 0;
}
