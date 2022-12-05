/**
 * @file sensor_interface_impl.h
 * @author  Jaikrishna Nemallapudi (nemallapudi.jaikrishna@intel.com)
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

#ifndef SENSOR_INTERFACE_IMPL_H
#define SENSOR_INTERFACE_IMPL_H

#include "istream_socket_client.h"
#include "sensor_interface.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <cstring>
#include <string>
#include <system_error>
extern "C"
{
#include <sys/poll.h>
#include <sys/types.h>
#include <unistd.h>
}
#include <thread>
#include <tuple>

using namespace std;
using namespace chrono_literals;

namespace vhal {
namespace client {

class SensorInterface::Impl
{
public:
    Impl(unique_ptr<IStreamSocketClient> socket_client, SensorCallback callback,
                                                        const int32_t user_id)
      : socket_client_{ move(socket_client) },
        callback_{ move(callback) }
    {
        vhal_talker_thread_ = thread([this, user_id]() {
            while (should_continue_) {
                if (not socket_client_->Connected()) {
                    if (auto [connected, error_msg] = socket_client_->Connect();
                        !connected) {
                        cout << "SensorInterface Failed to connect to VHal: "
                             << error_msg
                             << ". Retry after 1s...\n";
                        this_thread::sleep_for(1s);
                        continue;
                    }
                }
                // connected ...
                cout << "Connected to Sensor VHal!\n";
                sendStreamerUserId(user_id);

                struct pollfd fds[1];
                const int     timeout_ms = 1 * 1000; // 1 sec timeout
                int           ret;

                // watch socket for input
                fds[0].fd     = socket_client_->GetNativeSocketFd();
                fds[0].events = POLLIN;

                do {
                    ret = poll(fds, std::size(fds), timeout_ms);
                    if (ret == -1) {
                        throw system_error(errno, system_category());
                    }
                    if (!ret) {
                        continue;
                    }
                    if (fds[0].revents & POLLIN) {
                        cout << "Sensor VHal has some message for us!\n";

                        SensorInterface::CtrlPacket ctrl_msg;

                        if (auto [received, recv_err_msg] =
                              socket_client_->Recv(
                                reinterpret_cast<uint8_t*>(&ctrl_msg),
                                sizeof(ctrl_msg));
                            received != sizeof(SensorInterface::CtrlPacket)) {
                            cout << "Failed to read message from SensorInterface: "
                                 << recv_err_msg
                                 << ", going to disconnect and reconnect.\n";
                            socket_client_->Close();
                            break;
                        }

                        if (IsValidCtrlPacket(ctrl_msg.type)) {
                            // success, invoke client callback
                            callback_(cref(ctrl_msg));
                        }
                    } else {
                        if (fds[0].revents & (POLLERR|POLLHUP)) {
                            cout << "SensorInterface Poll Fail event: "
                                << fds[0].revents
                                << ", reconnect\n";
                            socket_client_->Close();
                            break;
                        }
                        cout << "SensorInterface : Poll revents " << fds[0].revents << "\n";
                    }
                } while (should_continue_);
            }
        });
    }

    ~Impl()
    {
        should_continue_ = false;
        vhal_talker_thread_.join();
    }

    IOResult SendDataPacket(const SensorDataPacket *event)
    {
        vhal_sensor_event_t sensor_event;
        int dataCount = 0;

        if (not socket_client_->Connected())
            return {0, "VHAL Not connected"};

        switch (event->type) {
            case SENSOR_TYPE_ACCELEROMETER:
            case SENSOR_TYPE_MAGNETIC_FIELD:
            case SENSOR_TYPE_GYROSCOPE:
                dataCount = 3;
                break;

            case SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED:
            case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
            case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
                dataCount = 6;
                break;

            case SENSOR_TYPE_LIGHT:
            case SENSOR_TYPE_PROXIMITY:
            case SENSOR_TYPE_AMBIENT_TEMPERATURE:
                dataCount = 1;
                break;

            default:
                cout << "LibVHAL[Sensor]: Sensor type %d not supported."
                        "Dropping data event." << event->type << endl;
                return {-1, "Sensor Type not supported"};
        }

        const size_t dataHeaderLen = sizeof(vhal_sensor_event_t) - sizeof(sensor_event.fdata);
        const size_t dataPayLoadLen = dataCount * sizeof(float);
        const size_t totalPayloadLen = dataHeaderLen + dataPayLoadLen;
        sensor_event.type = event->type;
        sensor_event.fdataCount = dataCount;
        sensor_event.timestamp_ns = event->timestamp_ns;
        sensor_event.fdata = nullptr;
        uint8_t dataPtr[totalPayloadLen];
        std::memmove(dataPtr, &sensor_event, dataHeaderLen);
        std::memmove((dataPtr + dataHeaderLen), event->fdata, dataPayLoadLen);
        if (auto [sent, error_msg] =
                socket_client_->Send(dataPtr, totalPayloadLen); sent == -1) {
            return { sent, error_msg };
        }

        // success
        return { totalPayloadLen, "" };
    }

    bool IsValidCtrlPacket(int32_t SensorType)
    {
        //configure sensor packet
        switch (SensorType) {
            case SENSOR_TYPE_ACCELEROMETER:
            case SENSOR_TYPE_MAGNETIC_FIELD:
            case SENSOR_TYPE_GYROSCOPE:
            case SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED:
            case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
            case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
            case SENSOR_TYPE_LIGHT:
            case SENSOR_TYPE_PROXIMITY:
            case SENSOR_TYPE_AMBIENT_TEMPERATURE:
                return true;
            default:
                return false;
       }
    }

    uint64_t GetSupportedSensorList() {
        return  SENSOR_TYPE_MASK(SENSOR_TYPE_ACCELEROMETER)  |
                SENSOR_TYPE_MASK(SENSOR_TYPE_MAGNETIC_FIELD) |
                SENSOR_TYPE_MASK(SENSOR_TYPE_GYROSCOPE) |
                SENSOR_TYPE_MASK(SENSOR_TYPE_AMBIENT_TEMPERATURE) |
                SENSOR_TYPE_MASK(SENSOR_TYPE_PROXIMITY) |
                SENSOR_TYPE_MASK(SENSOR_TYPE_LIGHT) |
                SENSOR_TYPE_MASK(SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED) |
                SENSOR_TYPE_MASK(SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED) |
                SENSOR_TYPE_MASK(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED);
    }
private:
    SensorCallback                  callback_ = nullptr;
    unique_ptr<IStreamSocketClient> socket_client_;
    thread                          vhal_talker_thread_;
    atomic<bool>                    should_continue_ = true;

    void sendStreamerUserId(int32_t user_id) {
        if (not socket_client_->Connected())
            return;

        if (user_id >= 0) {
            vhal_sensor_event_t sensor_event;
            sensor_event.type = SENSOR_TYPE_ADDITIONAL_INFO;
            sensor_event.userId = user_id;
            const int32_t dataLen = sizeof(vhal_sensor_event_t) - sizeof(sensor_event.fdata);
            uint8_t dataPtr[dataLen];
            std::memmove(dataPtr, &sensor_event, dataLen);
            socket_client_->Send(dataPtr, dataLen);
        }
    }
};

} // namespace client
} // namespace vhal

#endif /* VHAL_SENSOR_INTERFACE_IMPL_H */
