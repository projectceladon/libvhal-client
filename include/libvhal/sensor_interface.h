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
#include "libvhal_common.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#include <tuple>

#define MAX_DATA_CNT 6
#define SENSOR_TYPE_MASK(X) (1ULL << X)
#define IS_SENSOR_SUPPORTED(M, S) (M & SENSOR_TYPE_MASK(S))

namespace vhal {
namespace client {
/**
 * @brief enum sensor_type_t All sensor type values are
 *        defined from android sensor types.
 *        Taken reference from android source
 *        hardware/libhardware/include/hardware/sensors-base.h
 */
enum sensor_type_t {
    SENSOR_TYPE_ACCELEROMETER = 1,
    SENSOR_TYPE_MAGNETIC_FIELD = 2,
    SENSOR_TYPE_ORIENTATION = 3,
    SENSOR_TYPE_GYROSCOPE = 4,
    SENSOR_TYPE_LIGHT = 5,
    SENSOR_TYPE_PRESSURE = 6,
    SENSOR_TYPE_TEMPERATURE = 7,
    SENSOR_TYPE_PROXIMITY = 8,
    SENSOR_TYPE_GRAVITY = 9,
    SENSOR_TYPE_LINEAR_ACCELERATION = 10,
    SENSOR_TYPE_ROTATION_VECTOR = 11,
    SENSOR_TYPE_RELATIVE_HUMIDITY = 12,
    SENSOR_TYPE_AMBIENT_TEMPERATURE = 13,
    SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED = 14,
    SENSOR_TYPE_GAME_ROTATION_VECTOR = 15,
    SENSOR_TYPE_GYROSCOPE_UNCALIBRATED = 16,
    SENSOR_TYPE_SIGNIFICANT_MOTION = 17,
    SENSOR_TYPE_STEP_DETECTOR = 18,
    SENSOR_TYPE_STEP_COUNTER = 19,
    SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR = 20,
    SENSOR_TYPE_HEART_RATE = 21,
    SENSOR_TYPE_TILT_DETECTOR = 22,
    SENSOR_TYPE_WAKE_GESTURE = 23,
    SENSOR_TYPE_GLANCE_GESTURE = 24,
    SENSOR_TYPE_PICK_UP_GESTURE = 25,
    SENSOR_TYPE_WRIST_TILT_GESTURE = 26,
    SENSOR_TYPE_DEVICE_ORIENTATION = 27,
    SENSOR_TYPE_POSE_6DOF = 28,
    SENSOR_TYPE_STATIONARY_DETECT = 29,
    SENSOR_TYPE_MOTION_DETECT = 30,
    SENSOR_TYPE_HEART_BEAT = 31,
    SENSOR_TYPE_DYNAMIC_SENSOR_META = 32,
    SENSOR_TYPE_ADDITIONAL_INFO = 33,
    SENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT = 34,
    SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED = 35,
    SENSOR_TYPE_HINGE_ANGLE = 36
};

/**
 * @brief vhal_sensor_event_t is shared between
 * LibVHAL-Client and Sensor VHAL server.
 */
struct vhal_sensor_event_t {
    sensor_type_t type;  //sensor type
    union {
        int32_t userId; //User Id for multiclient use case
        int32_t fdataCount; //Number of data fields(fdata).
    };
    int64_t timestamp_ns; //time is in nanoseconds
    float *fdata; //Sensor data
};

/**
 * @brief Class that acts as a pipe between Sensor client and VHAL.
 * VHAL writes sensors configuration to the pipe and
 * sensor client writes sensor data to the pipe.
 *
 */
class SensorInterface
{
public:
    /**
     * @brief Sensor control packet received from VHAL
     *
     */
    struct CtrlPacket
    {
        sensor_type_t type;
        int32_t enabled;
        /**
         * samplingPeriod_ms is the frequency at which data events are expected.
         */
        int32_t samplingPeriod_ms;
    };

    /**
     * @brief Sensor data packet sent by streamer
     *
     */
    struct SensorDataPacket
    {
        sensor_type_t type;
        /* time is in nanosecond */
        int64_t timestamp_ns;
        float fdata[MAX_DATA_CNT];
    };

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
     * @brief Type of the Sensor callback which Sensor VHAL triggers to send
     * control message.
     *
     */
    using SensorCallback = std::function<void(const CtrlPacket& ctrl_msg)>;

    /**
     * @brief Construct a new SensorInterface object
     *        Throws std::invalid_argument excpetion.
     *
     * @param unix_conn_info Information needed to connect to the unix vhal socket.
     * @param callback Sensor callback function object or lambda or function
     * @param userId valid id >=0  for multi-client use case
     * pointer.
     */
    SensorInterface(UnixConnectionInfo unix_conn_info, SensorCallback callback,
                                                        const int32_t userId);

    /**
     * @brief Destroy the SensorInterface object
     *
     */
    ~SensorInterface();

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
     * @brief Get supported sensor list in bitmap format.
     *        Supported sensor's bit gets set using respective
     *        sensor type defined in sensor_type_t.
     *        Example: If only ACCELEROMETER and GEOMAGNETIC_ROTATION_VECTOR
     *        sensors are supported. Then this api returns 0x100002.
     *
     * @return uint64_t 64 bit number with only supported sensor bits are set.
     *         See #IS_SENSOR_SUPPORTED macro to identify.
     */
    uint64_t GetSupportedSensorList();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    void sendStreamerUserId(int32_t user_id);
};
} // namespace client
} // namespace vhal
#endif /* SENSOR_INTERFACE */
