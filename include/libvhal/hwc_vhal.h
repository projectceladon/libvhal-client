/**
 * Copyright (C) 2021-2022 Intel Corporation
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

#ifndef HWC_VHAL_H
#define HWC_VHAL_H

#include <atomic>
#include <string>
#include <functional>
#include <memory>
#include <map>
#include <thread>
#include "libvhal_common.h"
#include "display-protocol.h"

namespace vhal {
namespace client {

enum CommandType
{
    FRAME_CREATE  = 0, // create frame
    FRAME_REMOVE  = 1, // remove frame
    FRAME_DISPLAY = 2, // display frame
};

struct ConfigInfo
{
    UnixConnectionInfo unix_conn_info;
    int video_res_width = 0;
    int video_res_height = 0;
    std::string video_device = "";
    int user_id = 0;
};

using HwcHandler = std::function<void(CommandType cmd, const frame_info_t* frame)>;

class VirtualHwcReceiver
{
public:
    VirtualHwcReceiver(struct ConfigInfo info, HwcHandler handler);
    ~VirtualHwcReceiver();
    IOResult start();
    IOResult stop();
    IOResult setMode(int width, int height);
    IOResult setVideoAlpha(uint32_t action);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}
#endif
