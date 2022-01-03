#ifndef AUDIO_SINK_H
#define AUDIO_SINK_H
/**
 * @file audio_sink.h
 * @author Nitisha Tomar (nitisha.tomar@intel.com)
 * @brief
 * @version 0.1
 * @date 2021-07-27
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
#include "audio_common.h"
#include "istream_socket_client.h"
#include "libvhal_common.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#include <tuple>

namespace vhal {
namespace client {
namespace audio {

/**
 * @brief Class that acts as a pipe between Audio client and VHAL.
 * Audio client writes raw pcm data to the pipe.
 *
 */
class AudioSink
{
public:
    /**
     * @brief Constructs a new AudioSink object with the ip address of android
     *        instance
     *
     * @param tcp_conn_info Information needed to connect to the tcp vhal socket.
     * @param callback Audio callback function object or lambda function
     * pointer.
     * @param user_id Optional parameter to specify the user# in multi-user config.
     *
     */
    AudioSink(TcpConnectionInfo tcp_conn_info, AudioCallback callback, const int32_t user_id = -1);

    /**
     * @brief Destroy the AudioSink object
     *
     */
    ~AudioSink();

    /**
     * @brief Sends raw pcm audio packet to VHAL.
     *
     * @param packet Raw pcm audio packet.
     * @param size Size of the audio packet.
     *
     * @return IOResult tuple<ssize_t, std::string>.
     *         ssize_t No of bytes sent and -1 incase of failure
     *         string is the status message.
     */
    IOResult SendDataPacket(const uint8_t* packet, size_t size);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace audio
} // namespace client
} // namespace vhal
#endif /* AUDIO_SINK_H */
