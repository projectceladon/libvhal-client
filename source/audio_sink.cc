/**
 * @file audio_sink.cc
 * @author Nitisha Tomar (nitisha.tomar@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-07-27
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

#include "audio_sink.h"
#include "audio_sink_impl.h"
#include "tcp_stream_socket_client.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#define LIBVHAL_AUDIO_RECORD_PORT 8767

namespace vhal {
namespace client {
namespace audio {

AudioSink::AudioSink(TcpConnectionInfo tcp_conn_info, AudioCallback callback, const int32_t user_id)
{
    auto tcp_sock_client =
      std::make_unique<TcpStreamSocketClient>(tcp_conn_info.ip_addr,
      LIBVHAL_AUDIO_RECORD_PORT);
    impl_ = std::make_unique<Impl>(std::move(tcp_sock_client), callback, user_id);
}

AudioSink::~AudioSink() {}

IOResult AudioSink::SendDataPacket(const uint8_t* packet, size_t size)
{
    return impl_->SendDataPacket(packet, size);
}

} // namespace audio
} // namespace client
} // namespace vhal
