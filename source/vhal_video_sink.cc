/**
 * @file vhal_video_sink.cc
 * @author Shakthi Prashanth M (shakthi.prashanth.m@intel.com)
 * @brief
 * @version 1.0
 * @date 2021-04-27
 *
 * Copyright (c) 2021 Intel Corporation
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
#include "vhal_video_sink.h"
#include "vhal_video_sink_impl.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>

namespace vhal {
namespace client {

VHalVideoSink::VHalVideoSink(std::unique_ptr<IStreamSocketClient> socket)
  : impl_{ std::make_unique<Impl>(std::move(socket)) }
{}

VHalVideoSink::~VHalVideoSink() {}

bool
VHalVideoSink::RegisterCallback(CameraCallback callback)
{
    return impl_->RegisterCallback(callback);
}

VHalVideoSink::IOResult
VHalVideoSink::WritePacket(const uint8_t* packet, size_t size)
{
    return impl_->WritePacket(packet, size);
}

}; // namespace client
} // namespace vhal