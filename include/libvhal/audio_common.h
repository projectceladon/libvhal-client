#ifndef AUDIO_COMMON_H
#define AUDIO_COMMON_H
/**
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

#include <functional>
#include "android_audio_core.h"

namespace vhal {
namespace client {
namespace audio {

struct audio_socket_configuration_info {
    uint32_t sample_rate;
    uint32_t channel_count;
    audio_format_t format;
    uint32_t frame_count;
};

/**
 * @brief Audio operation commands sent by Audio VHAL to client.
 *
 */
enum Command
{
    kOpen  = 0,
    kClose = 1,
    kData  = 2,
    kStartstream  = 3,
    kStopstream = 4,
    kUserId = 5,
    kNone = 6
};

/**
 * @brief Audio control message sent by Audio VHAL to client.
 *
 */
struct CtrlMessage
{
    Command cmd = Command::kNone;
    union {
        struct audio_socket_configuration_info asci;
        uint32_t data_size;
        uint32_t data;
    };
};

/**
 * @brief Type of the Audio callback which Audio VHAL triggers for
 * OpenAudio and CloseAudio cases.
 *
 */
using AudioCallback = std::function<void(const CtrlMessage& ctrl_msg)>;

} // namespace audio
} // namespace client
} // namespace vhal
#endif /* AUDIO_COMMON_H */

