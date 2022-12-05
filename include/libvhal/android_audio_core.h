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

#ifndef ANDROID_AUDIO_CORE_H
#define ANDROID_AUDIO_CORE_H
namespace vhal {
namespace client {
namespace audio {
/**
 * @brief enum audio_format_t All given audio pcm data format values are
 *        defined from android audio format type.
 *        Taken reference from android source
 *        /system/media/audio/include/system/audio-base.h
 */
enum audio_format_t : uint32_t {
    AUDIO_FORMAT_IEC61937              = 0x0D000000u,
    AUDIO_FORMAT_PCM_16_BIT            = 0x1u,        // (PCM | PCM_SUB_16_BIT)
    AUDIO_FORMAT_PCM_8_BIT             = 0x2u,        // (PCM | PCM_SUB_8_BIT)
    AUDIO_FORMAT_PCM_32_BIT            = 0x3u,        // (PCM | PCM_SUB_32_BIT)
    AUDIO_FORMAT_PCM_8_24_BIT          = 0x4u,        // (PCM | PCM_SUB_8_24_BIT)
    AUDIO_FORMAT_PCM_FLOAT             = 0x5u,        // (PCM | PCM_SUB_FLOAT)
    AUDIO_FORMAT_PCM_24_BIT_PACKED     = 0x6u,        // (PCM | PCM_SUB_24_BIT_PACKED)
};

/**
  * @brief Returns size information based on the audio pcm data format
  *
  * @param format Audio format type
  */
static inline size_t audio_bytes_per_sample(uint32_t format)
{
    size_t size = 0;

    switch (format) {
    case AUDIO_FORMAT_PCM_32_BIT:
    case AUDIO_FORMAT_PCM_8_24_BIT:
        size = sizeof(int32_t);
        break;
    case AUDIO_FORMAT_PCM_24_BIT_PACKED:
        size = sizeof(uint8_t) * 3;
        break;
    case AUDIO_FORMAT_PCM_16_BIT:
    case AUDIO_FORMAT_IEC61937:
        size = sizeof(int16_t);
        break;
    case AUDIO_FORMAT_PCM_8_BIT:
        size = sizeof(uint8_t);
        break;
    case AUDIO_FORMAT_PCM_FLOAT:
        size = sizeof(float);
        break;
    default:
        break;
    }
    return size;
}

} // namespace audio
} // namespace client
} // namespace vhal
#endif /* ANDROID_AUDIO_CORE_H */
