/**
 * @file command_channel_interface.h
 * @author Kai Liu (kai1.liu@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-07-22
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

#ifndef COMMAND_CHANNEL_INTERFACE
#define COMMAND_CHANNEL_INTERFACE

#include "istream_socket_client.h"
#include "libvhal_common.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#include <tuple>

namespace vhal {
namespace client {

/**
 * @brief Comamnd Channel message type.
 *
 */
enum MsgType
{
    kNone  = 0,
    // Represents message or command that interacts with ActivityMonitorService.
    kActivityMonitor = 1,
    // Represents message or command that interacts with AicCommandService.
    kAicCommand = 2,
    // Represents message or command that interacts with FileTransferService.
    kFileTransfer = 3
};

/**
 * @brief Command Channel message sent by Command Channel server to client.
 *
 */
struct CommandChannelMessage
{
    MsgType msg_type = MsgType::kNone;
    uint8_t* data = nullptr;
    uint32_t data_size = 0;
};

/**
 * @brief Type of the Command Channel callback which Command Channel Server trigger for
 * Activity switch notification and Command execution result notification cases.
 *
 */
using CommandChannelCallback = std::function<void(const CommandChannelMessage& command_channel_msg)>;

/**
 * @brief Class that acts as a pipe between command channel client and server.
 * Server writes result to the pipe and
 * command channel client writes command data to the pipe.
 *
 */
class CommandChannelInterface
{
public:
    /**
     * @brief Constructs a new CommandChannelInterface object with the ip
     *        address of android instance
     * @param callback Command Channel callback function object or lambda
     * function pointer.
     *
     * @param tcp_conn_info Information needed to connect to the tcp server socket.
     *
     */
    CommandChannelInterface(TcpConnectionInfo tcp_conn_info, CommandChannelCallback callback);

    /**
     * @brief Destroy the CommandChannelInterface object
     *
     */
    ~CommandChannelInterface();

    /**
     * @brief Sends command packet to server.
     *
     * @param message type indicates which service the message will be sent to
     * @param message Message data.
     * @param size Size of message.
     *
     * @return IOResult tuple<ssize_t, std::string>.
     *         ssize_t No of bytes sent and -1 incase of failure
     *         string is the status message.
     */
    IOResult SendDataPacket(MsgType msg_type, const uint8_t* message, size_t size);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace client
} // namespace vhal
#endif /* COMMAND_CHANNEL_INTERFACE */
