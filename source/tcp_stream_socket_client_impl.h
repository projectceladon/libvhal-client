/**
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

#ifndef TCP_STREAM_SOCKET_CLIENT_IMPL_H
#define TCP_STREAM_SOCKET_CLIENT_IMPL_H

#include "tcp_stream_socket_client.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <system_error>
extern "C"
{
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
}

namespace vhal {
namespace client {

class TcpStreamSocketClient::Impl
{
public:
    Impl(const std::string& remote_server_ip, const int port)
    {
        tcp_sock_addr_.sin_family = AF_INET;
        tcp_sock_addr_.sin_port = htons(port);
        inet_pton(AF_INET,remote_server_ip.c_str(), &tcp_sock_addr_.sin_addr);
    }
    ~Impl() { Close(); }

    ConnectionResult Connect()
    {
        std::string error_msg = "";
        if (fd_ >= 0) {
            Close();
        }
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category());
        }
        connected_ = ::connect(fd_, (struct sockaddr*)&tcp_sock_addr_, sizeof(struct sockaddr_in)) == 0;
        if (!connected_) {
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
        ssize_t left = size;
        while (left > 0 ) {
            ssize_t received = ::recv(fd_,data, left,0);
            if (received <= 0) {
                std::cout << ". Recv() args: fd: " << fd_ << ", data: " << data
                      << ", size: " << size << "\n";
                error_msg = std::strerror(errno);
                break;
            }
            else {
                data += received;
                left -= received;
            }
        }
        return { size-left, error_msg };
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
    struct sockaddr_in tcp_sock_addr_;
};

} // namespace client
} // namespace vhal

#endif /* TCP_STREAM_SOCKET_CLIENT_IMPL_H */
