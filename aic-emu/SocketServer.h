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

#ifndef UNIX_STREAM_SOCKET_SERVER_H
#define UNIX_STREAM_SOCKET_SERVER_H

#include <iostream>
#include <memory>
#include <string>
extern "C"
{
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
}

using IOResult = std::tuple<ssize_t, std::string>;

#define MAX_CONNECTIONS 1

namespace aic_emu {
namespace server {

class UnixServerSocket
{
public:
    UnixServerSocket(const std::string& server_path);
    ~UnixServerSocket();

    bool             AwaitConnection();
    bool             Connected();
    IOResult         Send(const uint8_t* data, size_t size);
    IOResult         Recv(uint8_t* data, size_t size);
    IOResult         SendMsg(int* data, size_t sizeInBytes);
    void             Close() ;

private:
    int  fd_ = -1;
    int  server_fd_ = -1;
    bool connected_ = false;

    struct sockaddr_un server_;
    std::string        server_socket_path_;
};
} // namespace server
} // namespace aic_emu

#endif /* UNIX_STREAM_SOCKET_CLIENT_H */
