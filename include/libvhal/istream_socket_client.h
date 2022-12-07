/**
 * @file istream_socket_client.h
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

#ifndef ISTREAM_SOCKET_CLIENT_H
#define ISTREAM_SOCKET_CLIENT_H

#include "libvhal_common.h"
#include <cstdint>
#include <sys/types.h>
#include <tuple>

namespace vhal {
namespace client {

/**
 * Below are the return types used by derived class member functions.
 */

/**
 * @brief Interface for Stream oriented (connection-oriented) sockets, which
 * covers TCP, Unix domain, vSock sockets. Stream socket clients have notion of
 * connect, send, recv, close idiom. In specialized cases such as TCP and Unix,
 * connect parameters are different. In case of TCP, remote address and port
 * number is required, while Unix sockets just need server path. Hence, the
 * interface doesn't take any parameters, details of it is handled by
 * implementation.
 */
class IStreamSocketClient
{
public:
    /**
     * @brief Destroy the IStreamSocket object
     *
     */
    virtual ~IStreamSocketClient() = default;

    /**
     * @brief Connect to remote endpoint.
     *
     * @return <true, null-error-msg> connection successful.
     * @return <false, error-msg> connection failed. Reason for failure is found
     * in error msg string.
     */
    virtual ConnectionResult Connect() = 0;

    /**
     * @brief Get socket connection status
     *
     * @return true
     * @return false
     */
    virtual bool Connected() const = 0;

    /**
     * @brief Get the Native Socket Fd object
     *
     * @return int
     */
    virtual int GetNativeSocketFd() const = 0;

    /**
     * @brief Send raw data to server
     *
     * @param data
     * @param size
     * @return IOResult
     *         <Number of bytes sent, Empty string> on Success
     *         <Error number, Error message on Failure> on Failure
     */
    virtual IOResult Send(const uint8_t* data, size_t size) = 0;

    /**
     * @brief
     *
     * @param data Recevie raw data from server
     * @param size
     * @param flag - default value is 0 which is equivalent to read
     *               please refer to recv flags and how to use them
     *               https://man7.org/linux/man-pages/man2/recv.2.html
     *
     * @return IOResult
     *         <Number of bytes received, Empty string> on Success
     *         <Error number, Error message on Failure> on Failure
     */
    virtual IOResult Recv(uint8_t* data, size_t size, uint8_t flag = 0) = 0;

    /**
     * @brief Closes socket connection.
     */
    virtual void Close() = 0;
};
} // namespace client
} // namespace vhal

#endif /* ISTREAM_SOCKET_CLIENT_H */
