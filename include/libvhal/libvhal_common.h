/**
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

#ifndef LIBVHAL_COMMON_H
#define LIBVHAL_COMMON_H

#include <string>

namespace vhal {
namespace client {

/**
 * @brief TCP connection info to the Android instance
 *
 */
struct TcpConnectionInfo
{
    // IP Address of the Android instance.
    std::string ip_addr = "";
    /** \brief The port number of INET socket.
     *
     * If port=0, domain (input, audio, camera, etc.) might select port by
     * default.
     */
    uint16_t port = 0;
    // Status dir path to update the connection status.
    std::string status_dir = "";
};

/**
 * @brief UNIX connection to the Android instance
 *
 */
struct UnixConnectionInfo
{
    // Streamer dir path to the Android sockets.
    std::string socket_dir = "";
    // specifies the Instance/Session id of the Android instance, if valid.
    // In K8S-like environments(1 instance per pod), this can be omitted.
    int android_instance_id = -1;
    // Status dir path to update the connection status.
    std::string status_dir = "";
};

/**
 * @brief VSOCK connection to the Android instance
 *
 */
struct VsockConnectionInfo
{
    // Specifies the Context identifier of the Android VM instance.
    int android_vm_cid = -1;
};

/**
 * @brief IOResult
 *          { >=0, "" } on Success
 *          see function description for ssize_t value interpretation.
 *          {errno, "error msg"} on Failure
 */
using IOResult = std::tuple<ssize_t, std::string>;

/**
 * @brief ConnectionResult
 *          { True, "" } on Success
 *          { False, "error msg"} on Failure
 */
using ConnectionResult = std::tuple<bool, std::string>;

} // namespace client
} // namespace vhal
#endif /* LIBVHAL_COMMON_H */
