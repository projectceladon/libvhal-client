/**
 * @file tcp_stream_socket_client.cc
 * @author Nitisha Tomar (nitisha.tomar@intel.com)
 * @brief
 * @version 1.0
 * @date 2021-07-27
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

#include "tcp_stream_socket_client.h"
#include "tcp_stream_socket_client_impl.h"

namespace vhal {
namespace client {
TcpStreamSocketClient::TcpStreamSocketClient(
    const std::string& remote_server_ip,const int port)
    : impl_{ std::make_unique<Impl>(remote_server_ip,port) }
{}

TcpStreamSocketClient::~TcpStreamSocketClient() = default;

ConnectionResult
TcpStreamSocketClient::Connect()
{
    return impl_->Connect();
}

bool
TcpStreamSocketClient::Connected() const
{
    return impl_->Connected();
}

int
TcpStreamSocketClient::GetNativeSocketFd() const
{
    return impl_->GetNativeSocketFd();
}

IOResult
TcpStreamSocketClient::Send(const uint8_t* data, size_t size)
{
    return impl_->Send(data, size);
}

IOResult
TcpStreamSocketClient::Recv(uint8_t* data, size_t size, uint8_t flag)
{
    return impl_->Recv(data, size);
}

void
TcpStreamSocketClient::Close()
{
    impl_->Close();
}

} // namespace client
} // namespace vhal
