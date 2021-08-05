#ifndef LIBVHAL_COMMON_H
#define LIBVHAL_COMMON_H
/**
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

#include <string>

namespace vhal {
namespace client {

/**
 * @brief TCP connection info to the Android instance
 *
 */
struct TcpConnectionInfo {
    // IP Address of the Android instance.
    std::string ip_addr = "";
};

/**
 * @brief UNIX connection to the Android instance
 *
 */
struct UnixConnectionInfo {
    // Streamer dir path to the Android sockets.
    std::string socket_dir = "";
    // specifies the Instance/Session id of the Android instance, if valid.
    // In K8S-like environments(1 instance per pod), this can be omitted.
    int android_instance_id = -1;
};

/**
* @brief VSOCK connection to the Android instance
*
*/
struct VsockConnectionInfo {
    // Specifies the Context identifier of the Android VM instance.
    int android_vm_cid = -1;
};

} // namespace client
} // namespace vhal
#endif /* LIBVHAL_COMMON_H */
