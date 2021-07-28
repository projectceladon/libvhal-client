/**
 * @file sensor_interface.cc
 * @author Jaikrishna Nemallapudi (nemallapudi.jaikrishna@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-07-22
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
#include "sensor_interface.h"
#include "sensor_interface_impl.h"
#include "unix_stream_socket_client.h"
#include <functional>
#include <memory>
#include <string>
#include <string.h>
#include <sys/types.h>

namespace vhal {
namespace client {

SensorInterface::SensorInterface(int InstanceId)
{
    std::string sockPath;

    if (getenv("K8S_ENV") != NULL && strcmp(getenv("K8S_ENV"), "true") == 0) {
        sockPath = "/conn/sensors-socket";
    } else {
        sockPath = "/opt/workdir/ipc/sensors-socket";
        sockPath += std::to_string(InstanceId);
    }

    //Creating interface to communicate to VHAL via libvhal
    auto unix_sock_client =
      make_unique<UnixStreamSocketClient>(move(sockPath));
    impl_ = std::make_unique<Impl>(std::move(unix_sock_client));
}

SensorInterface::~SensorInterface() {}

bool SensorInterface::RegisterCallback(SensorCallback callback)
{
    return impl_->RegisterCallback(callback);
}

IOResult SensorInterface::SendDataPacket(const SensorDataPacket *event)
{
    return impl_->SendDataPacket(event);
}

uint64_t SensorInterface::GetSupportedSensorList()
{
    return impl_->GetSupportedSensorList();
}

}; // namespace client
} // namespace vhal
