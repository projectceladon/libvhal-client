/**
 * @file vsock_stream_socket_client_impl.h
 * @author Shiva kumara R (shiva.kumara.rudrappa@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-05-06
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

#ifndef VSOCK_STREAM_SOCKET_CLIENT_IMPL_H
#define VSOCK_STREAM_SOCKET_CLIENT_IMPL_H

#include "vsock_stream_socket_client.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <system_error>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/vm_sockets.h>
extern "C"
{
    #include <sys/socket.h>
    #include<netdb.h>	//hostent
    #include<arpa/inet.h>
    #include <sys/types.h>
    #include <sys/types.h>
    #include <sys/un.h>
    #include <unistd.h>
}
#define DEFAULT_PORT_CAMERA 1982

namespace vhal {
namespace client {

class VsockStreamSocketClient::Impl
{
public:
    Impl(const int android_vm_cid)
    {
        server_ = {};
        server_.svm_cid = android_vm_cid;
        server_.svm_family = AF_VSOCK;
        server_.svm_port = DEFAULT_PORT_CAMERA;
    }
    ~Impl() { Close(); }

    ConnectionResult Connect()
    {
        std::string error_msg = "";
        std::cout << "cid " << server_.svm_cid << " port " << server_.svm_port;
        if (fd_ >= 0) {
            Close();
        }
        fd_ = ::socket(AF_VSOCK, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category());
        }
        connected_ = ::connect(fd_, (struct sockaddr*)&server_, sizeof(server_)) == 0;
        if (!connected_) {
            std::cout << "Connect() failed args: fd: " << fd_
                      << ", remote server path: ";
            error_msg = std::strerror(errno);
        }
        return { connected_, error_msg };
    }

    bool Connected() const { return connected_; }

    int GetNativeSocketFd() const { return fd_; }

    IOResult Send(const uint8_t* data, size_t size)
    {
        std::string error_msg = "";

        ssize_t sent = ::send(fd_, data, size, 0);
        if (sent  == -1) {
            std::cout << ". Send() args: fd: " << fd_ << ", data: " << data
                      << ", size: " << size << "\n";
            error_msg = std::strerror(errno);
        }
        return { sent, error_msg };
    }

    IOResult Recv(uint8_t* data, size_t size, uint8_t flag)
    {
        std::string error_msg = "";
        ssize_t received = ::recv(fd_, data, size, flag);
        if (received  == -1) {
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
    struct sockaddr_vm server_;
};

} // namespace client
} // namespace vhal

#endif /* VSOCK_STREAM_SOCKET_CLIENT_IMPL_H */
