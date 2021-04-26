
/**
 * @file unix_socket_impl.h
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-04-23
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
#include "unix_socket.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <system_error>
extern "C"
{
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
}

namespace vhal {
namespace client {

class UnixSocket::Impl
{
public:
    Impl()
    {
        fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category());
        }
    }
    ~Impl() { Close(); }

    SocketFamily Family() const { return SocketFamily::kUnix; }

    bool Valid() const { return !(fd_ < 0); };

    bool Connect(const std::string& remote_server_socket_path)
    {
        remote_.sun_family = AF_UNIX;
        strcpy(remote_.sun_path, remote_server_socket_path.c_str());
        auto len = strlen(remote_.sun_path) + sizeof(remote_.sun_family);
        if (connect(fd_, (struct sockaddr*)&remote_, len) == -1) {
            throw std::system_error(errno, std::system_category());
        }
        return true;
    }

    size_t Send(const uint8_t* data, size_t size)
    {
        ssize_t sent;
        if ((sent = ::send(fd_, data, size, 0)) == -1) {
            throw std::system_error(errno, std::system_category());
        }
        return sent;
    }

    size_t Recv(uint8_t* data, size_t size)
    {
        ssize_t received;
        if ((received = ::recv(fd_, data, size, 0)) == -1) {
            throw std::system_error(errno, std::system_category());
        }
        return received;
    }

    void Close() { close(fd_); }

private:
    int fd_;

    struct sockaddr_un remote_;
};

} // namespace client
} // namespace vhal
