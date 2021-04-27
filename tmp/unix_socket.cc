
/**
 * @file unix_socket.cc
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
#include "unix_socket_impl.h"

namespace vhal {
namespace client {

UnixSocket::UnixSocket() : impl_{ std::make_unique<Impl>() } {}

UnixSocket::~UnixSocket() = default;

SocketFamily
UnixSocket::Family() const
{
    return impl_->Family();
}

bool
UnixSocket::Valid() const
{
    return impl_->Valid();
}

bool
UnixSocket::Connect(const std::string& remote_server_socket_path)
{
    return impl_->Connect(remote_server_socket_path);
}

size_t
UnixSocket::Send(const uint8_t* data, size_t size)
{
    return impl_->Send(data, size);
}

size_t
UnixSocket::Recv(uint8_t* data, size_t size)
{
    return impl_->Recv(data, size);
}

void
UnixSocket::Close()
{
    impl_->Close();
}

} // namespace client
} // namespace vhal
