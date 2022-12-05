/**
 * @file unix_stream_socket_client.cc
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 * @brief
 * @version 1.0
 * @date 2021-04-27
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

#include "unix_stream_socket_client.h"
#include "unix_stream_socket_client_impl.h"

namespace vhal {
namespace client {
UnixStreamSocketClient::UnixStreamSocketClient(
  const std::string& remote_server_path)
  : impl_{ std::make_unique<Impl>(remote_server_path) }
{}

UnixStreamSocketClient::~UnixStreamSocketClient() = default;

ConnectionResult
UnixStreamSocketClient::Connect()
{
    return impl_->Connect();
}

bool
UnixStreamSocketClient::Connected() const
{
    return impl_->Connected();
}

int
UnixStreamSocketClient::GetNativeSocketFd() const
{
    return impl_->GetNativeSocketFd();
}

IOResult
UnixStreamSocketClient::Send(const uint8_t* data, size_t size)
{
    return impl_->Send(data, size);
}

IOResult
UnixStreamSocketClient::Recv(uint8_t* data, size_t size, uint8_t flag)
{
    return impl_->Recv(data, size);
}

void
UnixStreamSocketClient::Close()
{
    impl_->Close();
}

} // namespace client
} // namespace vhal
