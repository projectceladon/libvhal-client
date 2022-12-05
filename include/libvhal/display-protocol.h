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

#ifndef _DISPLAY_PROTOCOL_H_
#define _DISPLAY_PROTOCOL_H_

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

namespace vhal {
namespace client {

enum gralloc_mode {
    /* vHAL mode : 0 - client mode (default),  1 - server mode */
    VHAL_GRALLOC_PERFORM_VHAL_MODE = 0x70000001,
};
enum dd_event {
    VHAL_DD_EVENT_DISPINFO_REQ = 0x1000,
    VHAL_DD_EVENT_DISPINFO_ACK = 0x1001,
    VHAL_DD_EVENT_CREATE_BUFFER = 0x1002,
    VHAL_DD_EVENT_REMOVE_BUFFER = 0x1003,
    VHAL_DD_EVENT_DISPLAY_REQ = 0x1004,
    VHAL_DD_EVENT_DISPLAY_ACK = 0x1005,
    VHAL_DD_EVENT_SERVER_IP_REQ = 0x1006,
    VHAL_DD_EVENT_SERVER_IP_ACK = 0x1007,
    VHAL_DD_EVENT_SERVER_IP_SET = 0x1008,

    VHAL_DD_EVENT_DISPPORT_REQ = 0x1700,
    VHAL_DD_EVENT_DISPPORT_ACK = 0x1701,

    VHAL_DD_EVENT_SETUP_RESOLUTION = 0x1800,

    VHAL_DD_EVENT_SET_VIDEO_ALPHA_REQ = 0x1900,
};

struct display_info_t {
  unsigned int flags;
  unsigned int width;
  unsigned int height;
  int stride;
  int format;
  float xdpi;
  float ydpi;
  float fps;
  int minSwapInterval;
  int maxSwapInterval;
  int numFramebuffers;
};

struct buffer_info_t {
  uint64_t remote_handle;
  int data[0];  // local handle
};

struct display_event_t {
  unsigned int type;
  unsigned int size; /* Payload size */
  unsigned int id; /* AIC ID */
  unsigned int renderNode; /* GPU Render Node */
};

struct display_info_event_t {
  display_event_t event;
  display_info_t info;
};

struct buffer_info_event_t {
  display_event_t event;
  buffer_info_t info;
};


struct display_port_t {
  unsigned int port;
  unsigned int reserve;
};

struct display_port_event_t {
  display_event_t event;
  display_port_t dispPort;
};

struct set_video_alpha_t {
  uint32_t enable;
  uint32_t reserved[3];
};

struct display_set_video_alpha_event_t {
  display_event_t event;
  set_video_alpha_t alpha;
};

typedef struct native_handle {
  int version; /* sizeof(native_handle_t) */
  int numFds;  /* number of file-descriptors at &data[0] */
  int numInts; /* number of ints at &data[numFds] */
  int data[0]; /* numFds + numInts ints */
} native_handle_t;

typedef native_handle_t* buffer_handle_t;

#define DRV_MAX_PLANES 4
struct cros_gralloc_handle {
  native_handle_t base;
  int32_t fds[DRV_MAX_PLANES];
  uint32_t strides[DRV_MAX_PLANES];
  uint32_t offsets[DRV_MAX_PLANES];
  uint32_t sizes[DRV_MAX_PLANES];
  uint32_t format_modifiers[2 * DRV_MAX_PLANES];
  uint32_t width;
  uint32_t height;
  uint32_t format;       /* DRM format */
  uint32_t tiling_mode;
  uint32_t use_flags[2]; /* Buffer creation flags */
  uint32_t magic;
  uint32_t pixel_stride;
  int32_t droid_format;
  int32_t usage; /* Android usage. */
  uint32_t consumer_usage;
  uint32_t producer_usage;
  uint32_t yuv_color_range;  // YUV Color range.
  uint32_t is_updated;       // frame updated flag
  uint32_t is_encoded;       // frame encoded flag
  uint32_t is_encrypted;
  uint32_t is_key_frame;
  uint32_t is_interlaced;
  uint32_t is_mmc_capable;
  uint32_t compression_mode;
  uint32_t compression_hint;
  uint32_t codec;
  uint32_t aligned_width;
  uint32_t aligned_height;
};

typedef struct cros_gralloc_handle* cros_gralloc_handle_t;

struct display_control_t {
  uint32_t alpha         :1;  // alpha video content
  uint32_t top_layer     :1;  // top layer to be blend
  uint32_t rotation      :2;  // content is rotated
  uint32_t reserved      :28;
  struct {
    int16_t l, t, r, b;
  } viewport;
};

struct frame_info_t {
  cros_gralloc_handle* handle;
  display_control_t*   ctrl;
};

}
}

#endif
