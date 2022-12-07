/**
 * @file vsock_stream_socket_client.cc
 * @author Shiva Kumara R (shiva.kumara.rudrappa@intel.com)
 * @brief
 * @version 1.0
 * @date 2021-05-06
 *
 * Copyright (C) 2021-2022 Intel Corporation
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

#include "vsock_stream_socket_client.h"
#include "vsock_stream_socket_client_impl.h"

namespace vhal {
namespace client {
VsockStreamSocketClient::VsockStreamSocketClient(
  const int android_vm_cid)
  : impl_{ std::make_unique<Impl>(android_vm_cid) }
{}

VsockStreamSocketClient::~VsockStreamSocketClient() = default;

ConnectionResult
VsockStreamSocketClient::Connect()
{
    return impl_->Connect();
}

bool
VsockStreamSocketClient::Connected() const
{
    return impl_->Connected();
}

int
VsockStreamSocketClient::GetNativeSocketFd() const
{
    return impl_->GetNativeSocketFd();
}

IOResult
VsockStreamSocketClient::Send(const uint8_t* data, size_t size)
{

    return impl_->Send(data, size);
}

IOResult
VsockStreamSocketClient::Recv(uint8_t* data, size_t size, uint8_t flag)
{
    return impl_->Recv(data, size, flag);
}

void
VsockStreamSocketClient::Close()
{
    impl_->Close();
}

} // namespace client
} // namespace vhal
