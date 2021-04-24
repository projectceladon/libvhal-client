
#include "unix_socket.h"
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
        UnixSocket  socket;
        std::string sock_path = argv[2];

        socket.Connect(move(sock_path));

        std::cout << "Waiting for CMD_OPEN_CAMERA\n";

        camera_socket_info_t camera_sock_info;

        size_t ret;

        if ((ret = socket.Recv(reinterpret_cast<uint8_t*>(&camera_sock_info),
                               sizeof(camera_sock_info))) &&
            camera_sock_info.cmd == CMD_OPEN_CAMERA) {

            cout << "Received CMD_OPEN_CAMERA\n";

            const size_t kSize = INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE;
            std::array<uint8_t, kSize> inbuf = { 0 };
            size_t                     data_size;

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
                if (socket.Send((uint8_t*)&data_size, sizeof(size_t)) ==
                    sizeof(size_t)) {
                    cout << "Sent " << sizeof(size_t) << " bytes to VHal\n";
                    // send data
                    if (socket.Send(inbuf.data(), data_size) == data_size) {
                        cout << "Sent " << data_size << " bytes to VHal\n";
                    }
                    cout << ">>>>>> Sending frames at 30fps...\n";
                    std::this_thread::sleep_for(33ms);
                }
            }
        } else if (camera_sock_info.cmd == CMD_CLOSE_CAMERA) {
            cout << "Received CMD_CLOSE_CAMERA\n";
        }

        fclose(f);
    } catch (const std::system_error& error) {
        std::cout << "Error: " << error.code() << " - "
                  << error.code().message() << '\n';
    }

    return 0;
}
