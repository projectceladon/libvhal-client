/**
 * Copyright (C) 2022 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SocketServer.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <system_error>

namespace aic_emu {
namespace server {

    UnixServerSocket::UnixServerSocket(const std::string& server_socket_path)
    {
        connected_ = false;
        server_.sun_family = AF_UNIX;
        strncpy(server_.sun_path, server_socket_path.c_str(), sizeof(server_.sun_path) -1);
        server_socket_path_ = server_socket_path;
    }

    UnixServerSocket::~UnixServerSocket()
    {
        Close();
    }

    bool UnixServerSocket::AwaitConnection()
    {
        int status = 0;
        if (server_fd_ >= 0 || fd_ >= 0)
            Close();

        //Create Server Socket to accept connections
        server_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd_ < 0)
        {
            std::cout << "Socket create Failed. Error " << errno << ": " << std::strerror(errno) << std::endl;
            throw std::system_error(errno, std::system_category());
        }

        //Unlink existing files. Else will run into bind errors
        status = unlink(server_socket_path_.c_str());
        if (status !=  0)
            std::cout << "Unlink Failed. Error " << errno << ": " << std::strerror(errno) << std::endl;

        //Bind
        status = bind(server_fd_, (struct sockaddr*)&server_, SUN_LEN(&server_));
        if (status < 0)
        {
            std::cout << "Bind Failed. Error " << errno << ": " << std::strerror(errno) << std::endl;
            throw std::system_error(errno, std::system_category());
        }

        //Make socket writable and accessible to all users
        status = chmod(server_socket_path_.c_str(),
                       S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
        if(status < 0)
        {
            std::cout << "Socket chmod Failed. Error " << errno << ": " << std::strerror(errno) << std::endl;
            throw std::system_error(errno, std::system_category());
        }

        //Configure Listen
        status = listen(server_fd_, MAX_CONNECTIONS);
        if (status < 0)
        {
            std::cout << "Listen Failed. Error " << errno << ": " << std::strerror(errno) << std::endl;
            throw std::system_error(errno, std::system_category());
        }

        //Block for client connection
        std::cout << "Awaiting Client connection: Path: " << std::string(server_.sun_path) << std::endl;
        fd_ = accept(server_fd_, NULL, NULL);
        if (fd_ < 0)
        {
            std::cout << "Accept Failed. Error " << errno << ": " << std::strerror(errno) << std::endl;
            throw std::system_error(errno, std::system_category());
        }

        connected_ = true;

        return connected_;
    }

    bool UnixServerSocket::Connected() { return connected_; }

    IOResult UnixServerSocket::Send(const uint8_t* data, size_t size)
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

    IOResult UnixServerSocket::Recv(uint8_t* data, size_t size)
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

    IOResult UnixServerSocket::SendMsg(int* data, size_t sizeInBytes)
    {
        std::string error_msg = "";
        if (!data) {
            error_msg =  std::string("Null Pointer for data");
            return { -1, error_msg };
        }

        if (sizeInBytes <= 0) {
            error_msg = std::string("Invalid size specified: ") + std::to_string(sizeInBytes);
            return { -1, error_msg };
        }

        //Prepare message
        int sdata[4] = {0x88};

        struct msghdr msg = {0};
        struct cmsghdr *cmsg = nullptr;
        struct iovec vec;

        char buf[CMSG_SPACE(sizeInBytes)]; //ancillary data buffer

        msg.msg_control = buf;
        msg.msg_controllen = sizeof(buf);
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeInBytes);

        // Init payload
        int* payload  = (int *) CMSG_DATA(cmsg);
        memcpy(payload, data, sizeInBytes);

        // Program msg
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &vec;
        msg.msg_iovlen = 1;
        msg.msg_flags = 0;

        vec.iov_base = &sdata;
        vec.iov_len = 16;

        ssize_t sent;
        if ((sent = ::sendmsg(fd_, &msg, MSG_NOSIGNAL)) < 0)
        {
            std::cout << ". sendmsg() args: fd: " << fd_ << ", data: " << data
                      << ", sizeInBytes: " << sizeInBytes << std::endl;
            std::cout << ". msg. msg_controllen: " << msg.msg_controllen << std::endl;
            error_msg = std::strerror(errno);
        }

        std::cout << std::string(__FUNCTION__) << ": Sent "<< sent << " bytes ... "
                  << "payload fd: " << *data << std::endl;
        return { sent, error_msg };
    }


    void UnixServerSocket::Close()
    {
        connected_ = false;
        if (fd_ >= 0)
        {
            shutdown(fd_, SHUT_RDWR);
            close(fd_);
            fd_ = -1;
        }
        if (server_fd_ >= 0)
        {
            shutdown(server_fd_, SHUT_RDWR);
            close(server_fd_);
            server_fd_ = -1;
        }
    }

} // namespace server
} // namespace vhal

