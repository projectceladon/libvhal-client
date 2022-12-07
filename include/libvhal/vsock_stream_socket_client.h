/**
 * @file vsock_stream_socket.h
 *
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 *
 * @brief
 *
 * @version 1.0
 *
 * @date 2021-04-24
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

#ifndef VSOCK_STREAM_SOCKET_CLIENT_H
#define VSOCK_STREAM_SOCKET_CLIENT_H

#include "istream_socket_client.h"
#include <iostream>
#include <memory>
#include <string>

namespace vhal {
namespace client {

class VsockStreamSocketClient final : public IStreamSocketClient
{
public:
    VsockStreamSocketClient(const int android_vm_cid);
    ~VsockStreamSocketClient();

    ConnectionResult Connect() override;
    bool             Connected() const override;
    int              GetNativeSocketFd() const override;
    IOResult         Send(const uint8_t* data, size_t size) override;
    IOResult         Recv(uint8_t* data, size_t size, uint8_t flag= 0) override;

    void             Close() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace client
} // namespace vhal

#endif /* VSOCK_STREAM_SOCKET_CLIENT_H */
