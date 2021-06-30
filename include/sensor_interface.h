#ifndef SENSOR_INTERFACE
#define SENSOR_INTERFACE
/**
 * @file sensor_interface.h
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
#include "istream_socket_client.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#include <tuple>

namespace vhal {
namespace client {

typedef enum sensor_type_t {
    SENSOR_TYPE_INVALID                     = -1,
    SENSOR_TYPE_ACCELEROMETER               = 1,
    SENSOR_TYPE_MAGNETIC_FIELD              = 2,
    SENSOR_TYPE_GYROSCOPE                   = 4,
    SENSOR_TYPE_LIGHT                       = 5,
    SENSOR_TYPE_PRESSURE                    = 6,
    SENSOR_TYPE_PROXIMITY                   = 8,
    SENSOR_TYPE_GRAVITY                     = 9,
    SENSOR_TYPE_LINEAR_ACCELERATION         = 10,
    SENSOR_TYPE_ROTATION_VECTOR             = 11,
    SENSOR_TYPE_RELATIVE_HUMIDITY           = 12,
    SENSOR_TYPE_AMBIENT_TEMPERATURE         = 13,
    SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED = 14,
    SENSOR_TYPE_GAME_ROTATION_VECTOR        = 15,
    SENSOR_TYPE_GYROSCOPE_UNCALIBRATED      = 16,
    SENSOR_TYPE_SIGNIFICANT_MOTION          = 17,
    SENSOR_TYPE_STEP_DETECTOR               = 18,
    SENSOR_TYPE_STEP_COUNTER                = 19,
    SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR = 20,
    SENSOR_TYPE_HEART_RATE                  = 21,
    SENSOR_TYPE_POSE_6DOF                   = 28,
    SENSOR_TYPE_STATIONARY_DETECT           = 29,
    SENSOR_TYPE_MOTION_DETECT               = 30,
    SENSOR_TYPE_HEART_BEAT                  = 31,
    SENSOR_TYPE_ADDITIONAL_INFO             = 33,
    SENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT  = 34,
    SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED  = 35
} sensor_type_t;

typedef struct vhal_sensor_event_t {
    int32_t type;  //sensor type
    int32_t dataNum; //Number of data fileds
    int64_t timestamp; //time is in nanosecond
    union {
        float fdata[0];
        int32_t idata[0];
        char cdata[0];
    } data;
} vhal_sensor_event_t;

/**
 * @brief Class that acts as a pipe between Sensor client and VHAL.
 * VHAL writes sensors configuration to the pipe and
 * sensor client writes sensor data to the pipe.
 *
 */
class SensorInterface
{
public:
    using IOResult = std::tuple<ssize_t, std::string>;

    /**
     * @brief Sensor config packet received from VHAL
     *
     */
    typedef struct ConfPacket
    {
        int32_t type;
        int32_t enabled;
        int32_t samplePeriod;
    } ConfPacket;

    /**
     * @brief Sensor data packet sent by streamer
     *
     */
    typedef struct SensorDataPacket
    {
        int32_t type;
        /* time is in nanosecond */
        int64_t timestamp;
        float fdata[6];
    } SensorDataPacket;

    /**
     * @brief Sensor VHAL version.
     *
     */
    enum class VHalVersion
    {
        kV1 = 1, //Sensor HAL Major version number
        kV2 = 3, //Sensor HAL Minor version number
    };

    /**
     * @brief Type of the Sensor callback which Sensor VHAL triggers for
     * config message.
     *
     */
    using SensorCallback = std::function<void(const ConfPacket& ctrl_msg)>;

    /**
     * @brief Construct a new SensorInterface object
     *
     * @param WritePacket Client instance id.
     */
    SensorInterface(int InstanceId);

    /**
     * @brief Destroy the SensorInterface object
     *
     */
    ~SensorInterface();

    /**
     * @brief Registers Sensor callback.
     *
     * @param callback Sensor callback function object or lambda or function
     * pointer.
     *
     * @return true Sensor callback registered successfully.
     * @return false Sensor callback failed to register.
     */
    bool RegisterCallback(SensorCallback callback);

    /**
     * @brief Send sensor data to VHAL.
     *
     * @param event Sensor data.
     *
     * @return IOResult tuple<ssize_t, std::string>.
     *         ssize_t is number of bytes sent and -1 incase of failure
     *         string is thr status message.
     */
    IOResult SendDataPacket(const SensorDataPacket *event);

    /**
     * @brief Get suppoeted sensor list bitmap.
     *
     * @return uint64_t Bitmap of VHAL supported sensor types.
     */
    uint64_t GetSupportedSensorList();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace client
} // namespace vhal
#endif /* SENSOR_INTERFACE */
