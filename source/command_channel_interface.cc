/**
 * @file command_channel_interface.cc
 * @author Kai Liu (kai1.liukai@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-09-13
 *
 * Copyright (c) 2021 Intel Corporation
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
 */
#include "command_channel_interface.h"
#include "command_channel_interface_impl.h"
#include "tcp_stream_socket_client.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#define COMMAND_CHANNEL_ACTIVITY_MONITOR_PORT 8770
#define COMMAND_CHANNEL_AIC_COMMAND_PORT 8771

namespace vhal {
namespace client {

CommandChannelInterface::CommandChannelInterface(TcpConnectionInfo tcp_conn_info, CommandChannelCallback callback)
{
    auto tcp_sock_activity_monitor_client =
        std::make_unique<TcpStreamSocketClient>(tcp_conn_info.ip_addr,
        COMMAND_CHANNEL_ACTIVITY_MONITOR_PORT);
    auto tcp_sock_aic_command_client =
        std::make_unique<TcpStreamSocketClient>(tcp_conn_info.ip_addr,
        COMMAND_CHANNEL_AIC_COMMAND_PORT);
    impl_ = std::make_unique<Impl>(std::move(tcp_sock_activity_monitor_client),
                                   std::move(tcp_sock_aic_command_client), callback);
}

CommandChannelInterface::~CommandChannelInterface() {}

IOResult CommandChannelInterface::SendDataPacket(MsgType msg_type, const uint8_t* message, size_t size)
{
    return impl_->SendDataPacket(msg_type, message, size);
}

}; // namespace client
} // namespace vhal
