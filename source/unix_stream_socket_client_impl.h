/**
 * @file unix_stream_socket_client_impl.h
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-04-23
 *
 * Copyright (C) 2021-2022 Intel Corporation
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

#ifndef UNIX_STREAM_SOCKET_CLIENT_IMPL_H
#define UNIX_STREAM_SOCKET_CLIENT_IMPL_H

#include "unix_stream_socket_client.h"
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

class UnixStreamSocketClient::Impl
{
public:
    Impl(const std::string& remote_server_socket_path)
    {
        remote_.sun_family = AF_UNIX;
        strncpy(remote_.sun_path, remote_server_socket_path.c_str(), sizeof(remote_.sun_path) -1);
    }
    ~Impl() { Close(); }

    ConnectionResult Connect()
    {
        std::string error_msg = "";
        auto        len = strlen(remote_.sun_path) + sizeof(remote_.sun_family);
        if (fd_ >= 0) {
            Close();
        }
        fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category());
        }
        connected_ = ::connect(fd_, (struct sockaddr*)&remote_, len) == 0;
        if (!connected_) {
            std::cout << "Connect() failed args: fd: " << fd_
                      << ", remote server path: " << remote_.sun_path << "\n";
            error_msg = std::strerror(errno);
        }
        return { connected_, error_msg };
    }

    bool Connected() const { return connected_; }

    int GetNativeSocketFd() const { return fd_; }

    IOResult Send(const uint8_t* data, size_t size)
    {
        std::string error_msg = "";

        ssize_t sent;
        if ((sent = ::send(fd_, data, size, 0)) == -1) {
            std::cout << ". Send() args: fd: " << fd_ << ", data: " << data
                      << ", size: " << size << "\n";
            error_msg = std::strerror(errno);
        }
        return { sent, error_msg };
    }

    IOResult Recv(uint8_t* data, size_t size)
    {
        std::string error_msg = "";

        ssize_t received;
        if ((received = ::recv(fd_, data, size, 0)) == -1) {
            std::cout << ". Recv() args: fd: " << fd_ << ", data: " << data
                      << ", size: " << size << "\n";
            error_msg = std::strerror(errno);
        }
        return { received, error_msg };
    }

    void Close() {
        connected_ = false;
        if (fd_ < 0) return;
        shutdown(fd_, SHUT_RDWR);
        close(fd_);
        fd_ = -1;
    }

private:
    int  fd_ = -1;
    bool connected_ = false;

    struct sockaddr_un remote_;
    std::string        remote_server_socket_path_;
};

} // namespace client
} // namespace vhal

#endif /* UNIX_STREAM_SOCKET_CLIENT_IMPL_H */
