#ifndef SENSOR_INTERFACE_IMPL_H
#define SENSOR_INTERFACE_IMPL_H
/**
 * @file sensor_interface_impl.h
 * @author  Jaikrishna Nemallapudi (nemallapudi.jaikrishna@intel.com)
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

#include "istream_socket_client.h"
#include "sensor_interface.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
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
    Impl(unique_ptr<IStreamSocketClient> socket_client)
      : socket_client_{ move(socket_client) }
    {
        vhal_talker_thread_ = thread([this]() {
            while (should_continue_) {
                if (not socket_client_->Connected()) {
                    if (auto [connected, error_msg] = socket_client_->Connect();
                        !connected) {
                        cout << "Failed to connect to VHal: " << error_msg
                             << ". Retry after 3ms...\n";
                        this_thread::sleep_for(3ms);
                        continue;
                    }
                }
                // connected ...
                cout << "Connected to Sensor VHal!\n";

                struct pollfd fds[1];
                const int     timeout_ms = 1 * 1000; // 1 sec timeout
                int           ret;

                // watch socket for input
                fds[0].fd     = socket_client_->GetNativeSocketFd();
                fds[0].events = POLLIN;

                do {
                    ret = poll(fds, 1, timeout_ms);
                    if (ret == -1) {
                        throw system_error(errno, system_category());
                    }
                    if (!ret) {
                        continue;
                    }
                    if (fds[0].revents & POLLIN) {
                        cout << "Sensor VHal has some message for us!\n";

                        SensorInterface::ConfPacket ctrl_msg;

                        if (auto [received, recv_err_msg] =
                              socket_client_->Recv(
                                reinterpret_cast<uint8_t*>(&ctrl_msg),
                                sizeof(ctrl_msg));
                            received != sizeof(SensorInterface::ConfPacket)) {
                            cout << "Failed to read message from SensorInterface: "
                                 << recv_err_msg
                                 << ", going to disconnect and reconnect.\n";
                            socket_client_->Close();
                            continue;
                            // FIXME: What to do ?? Exit ?
                        }

                        if (IsValidConfPacket(ctrl_msg.type)) {
                            // success, invoke client callback
                            callback_(cref(ctrl_msg));
                        }
                    }
                } while (true);
            }
        });
    }

    ~Impl()
    {
        should_continue_ = false;
        vhal_talker_thread_.join();
    }

    bool RegisterCallback(SensorCallback callback)
    {
        callback_ = move(callback);
        return true;
    }

    SensorInterface::IOResult SendDataPacket(const SensorDataPacket *event)
    {
        vhal_sensor_event_t *sensor_event = nullptr;
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

        if (!sensor_event)
            sensor_event = (vhal_sensor_event_t *)
                new char[sizeof(vhal_sensor_event_t) + sizeof(float) * 8];

        int32_t totalPayloadLen = sizeof(vhal_sensor_event_t) +
                                        dataCount * sizeof(float);
        sensor_event->type = event->type;
        sensor_event->dataNum = dataCount;
        sensor_event->timestamp = event->timestamp;
        for (int i = 0; i < dataCount; i++)
             sensor_event->data.fdata[i] = event->fdata[i];

        if (auto [sent, error_msg] =
                socket_client_->Send(reinterpret_cast<uint8_t*>(sensor_event),
                totalPayloadLen); sent == -1) {
            delete sensor_event;
            return { sent, error_msg };
        }
        // success
        delete sensor_event;
        return { totalPayloadLen, "" };
    }

    bool IsValidConfPacket(int32_t SensorType)
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
        return  (1ULL << (SENSOR_TYPE_ACCELEROMETER -1))  |
                (1ULL << (SENSOR_TYPE_MAGNETIC_FIELD -1)) |
                (1ULL << (SENSOR_TYPE_GYROSCOPE -1)) |
                (1ULL << (SENSOR_TYPE_AMBIENT_TEMPERATURE -1)) |
                (1ULL << (SENSOR_TYPE_PROXIMITY -1)) |
                (1ULL << (SENSOR_TYPE_LIGHT -1)) |
                (1ULL << (SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED -1)) |
                (1ULL << (SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED -1)) |
                (1ULL << (SENSOR_TYPE_GYROSCOPE_UNCALIBRATED -1));
    }
private:
    SensorCallback                  callback_ = nullptr;
    unique_ptr<IStreamSocketClient> socket_client_;
    thread                          vhal_talker_thread_;
    atomic<bool>                    should_continue_ = true;
};

} // namespace client
} // namespace vhal

#endif /* VHAL_SENSOR_INTERFACE_IMPL_H */
