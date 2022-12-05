/**
 *
 * Copyright (c) 2021-2022 Intel 
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

#ifndef _RECEIVER_LOG_H
#define _RECEIVER_LOG_H

#include <cstring>
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace vhal {
namespace client {

#ifdef __android__
#include <android/log.h>
#endif

#define LIBVHAL_INFO 1
#define LIBVHAL_DEBUG 2
#define LIBVHAL_WARNING 3
#define LIBVHAL_ERROR 4

#ifndef __android__
#define AIC_LOG(level, fmt, ...)                                               \
    do {                                                                       \
        if (level >= LIBVHAL_INFO) {                                            \
            printf("%s %ld %s(%d): " fmt "\n",                                 \
                   (level == 4)                                                \
                     ? "ERR"                                                   \
                     : ((level == 3)                                           \
                          ? "WRN"                                              \
                          : ((level == 2) ? "DBG" : "LIBVHAL_INFO")),          \
                   syscall(__NR_gettid),                                       \
                   __FUNCTION__,                                               \
                   __LINE__,                                                   \
                   ##__VA_ARGS__);                                             \
            fflush(stdout);                                                    \
        }                                                                      \
    } while (0)
#else
#define AIC_LOG(level, fmt, ...)                                               \
    do {                                                                       \
        if (level >= LIBVHAL_INFO) {                                            \
            __android_log_print(                                               \
              (level == 4)                                                     \
                ? ANDROID_LOG_ERROR                                            \
                : ((level == 3)                                                \
                     ? ANDROID_LOG_WARN                                        \
                     : ((level == 2) ? ANDROID_LOG_DEBUG : ANDROID_LOG_INFO)), \
              LOG_TAG,                                                         \
              "%s(%d): " fmt,                                                  \
              __FUNCTION__,                                                    \
              __LINE__,                                                        \
              ##__VA_ARGS__);                                                  \
        }                                                                      \
    } while (0)
#endif

} // namespace client
} // namespace vhal
#endif //_RECEIVER_LOG_H