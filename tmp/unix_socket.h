#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H

/**
 * @file unix_socket.h
 *
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 *
 * @brief
 *
 * @version 0.1
 * @date 2021-04-23
 *
 * @copyright Copyright (c) 2021 Intel Corporation
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
 */

#include "common.h"
#include <memory>
#include <string>
#include <vector>

namespace vhal {
namespace client {

/**
 * @brief
 *
 */
class UnixSocket
{
public:
    /**
     * @brief Construct a new Unix Socket object
     *
     */
    UnixSocket();

    /**
     * @brief Destroy the Unix Socket object
     *
     */
    ~UnixSocket();

    /**
     * @brief
     *
     * @return SocketFamily
     */
    SocketFamily Family() const;

    /**
     * @brief
     *
     * @return true
     * @return false
     */
    bool Valid() const;

    /**
     * @brief
     *
     * @param remote_server_socket_path
     * @return true
     * @return false
     */
    bool Connect(const std::string& remote_server_socket_path);

    /**
     * @brief
     *
     * @param data
     * @param size
     * @return size_t
     */
    size_t Send(const uint8_t* data, size_t size);

    /**
     * @brief
     *
     * @param data
     * @param size
     * @return size_t
     */
    size_t Recv(uint8_t* data, size_t size);

    /**
     * @brief
     *
     * @return true
     * @return false
     */
    void Close();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace client
} // namespace vhal

#endif /* UNIX_SOCKET_H */
