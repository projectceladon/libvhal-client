/**
 * Copyright (C) 2022 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __YML_PARSER__
#define __YML_PARSER__

#include "display-protocol.h"
#include "yaml-cpp/yaml.h"

struct AicEventMetadata_t;

using namespace vhal::client;

using AicEventMetadataPtr = std::shared_ptr<AicEventMetadata_t>;
using DisplayEventPtr     = std::shared_ptr<display_event_t>;
using DisplayInfoPtr      = std::shared_ptr<display_info_t>;
using DisplayPortPtr      = std::shared_ptr<display_port_t>;
using BufferInfoPtr       = std::shared_ptr<buffer_info_t>;
using CrosGrallocHandlePtr = std::shared_ptr<struct cros_gralloc_handle>;

using DisplayInfoEventPtr = std::shared_ptr<display_info_event_t>;
using DisplayPortEventPtr = std::shared_ptr<display_port_event_t>;
using BufferInfoEventPtr  = std::shared_ptr<buffer_info_event_t>;

using SetAlphaPtr         = std::shared_ptr<set_video_alpha_t>;
using SetAlphaEventPtr    = std::shared_ptr<display_set_video_alpha_event_t>;
using DisplayCtrlPtr      = std::shared_ptr<display_control_t>;

namespace YAML
{
    template <> struct convert<AicEventMetadataPtr> {
        static bool decode(const Node& node, AicEventMetadataPtr &mptr);
    };

    template <> struct convert<DisplayEventPtr> {
        static bool decode(const Node& node, DisplayEventPtr &mptr);
    };

    template <> struct convert<DisplayInfoPtr> {
        static bool decode(const Node& node, DisplayInfoPtr &mptr);
    };

    template <> struct convert<DisplayPortPtr> {
        static bool decode(const Node& node, DisplayPortPtr &mptr);
    };

    template <> struct convert<BufferInfoPtr> {
        static bool decode(const Node& node, BufferInfoPtr &mptr);
    };

    template <> struct convert<CrosGrallocHandlePtr> {
        static bool decode(const Node& node, CrosGrallocHandlePtr &mptr);
    };

    template <> struct convert<DisplayInfoEventPtr> {
        static bool decode(const Node& node, DisplayInfoEventPtr &mptr);
    };

    template <> struct convert<DisplayPortEventPtr> {
        static bool decode(const Node& node, DisplayPortEventPtr &mptr);
    };

    template <> struct convert<BufferInfoEventPtr> {
        static bool decode(const Node& node, BufferInfoEventPtr &mptr);
    };

    template <> struct convert<SetAlphaPtr> {
        static bool decode(const Node& node, SetAlphaPtr &mptr);
    };

    template <> struct convert<SetAlphaEventPtr> {
        static bool decode(const Node& node, SetAlphaEventPtr &mptr);
    };

    template <> struct convert<DisplayCtrlPtr> {
        static bool decode(const Node& node, DisplayCtrlPtr &mptr);
    };
}
#endif
