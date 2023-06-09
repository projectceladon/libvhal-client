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

#include "CmdHandler.h"

static bool g_logEnable = true;

int GetEventCode(std::string str)
{
    if (g_logEnable)
        std::cout << str << "@@@@@@@@@" << std::endl;

    int event = 0;

    if (str == "VHAL_DD_EVENT_DISPINFO_REQ")
        return VHAL_DD_EVENT_DISPINFO_REQ;
    else if (str == "VHAL_DD_EVENT_DISPINFO_ACK")
        return VHAL_DD_EVENT_DISPINFO_ACK;
    else if (str == "VHAL_DD_EVENT_CREATE_BUFFER")
        return VHAL_DD_EVENT_CREATE_BUFFER;
    else if (str == "VHAL_DD_EVENT_REMOVE_BUFFER")
        return VHAL_DD_EVENT_REMOVE_BUFFER;
    else if (str == "VHAL_DD_EVENT_DISPLAY_REQ")
        return VHAL_DD_EVENT_DISPLAY_REQ;
    else if (str == "VHAL_DD_EVENT_DISPLAY_ACK")
        return VHAL_DD_EVENT_DISPLAY_ACK;
    else if (str == "VHAL_DD_EVENT_SERVER_IP_REQ")
        return VHAL_DD_EVENT_SERVER_IP_REQ;
    else if (str == "VHAL_DD_EVENT_SERVER_IP_ACK")
        return VHAL_DD_EVENT_SERVER_IP_ACK;
    else if (str == "VHAL_DD_EVENT_SERVER_IP_SET")
        return VHAL_DD_EVENT_SERVER_IP_SET;
    else if (str == "VHAL_DD_EVENT_DISPPORT_REQ")
        return VHAL_DD_EVENT_DISPPORT_REQ;
    else if (str == "VHAL_DD_EVENT_DISPPORT_ACK")
        return VHAL_DD_EVENT_DISPPORT_ACK;
    else if (str == "VHAL_DD_EVENT_SETUP_RESOLUTION")
        return VHAL_DD_EVENT_SETUP_RESOLUTION;

    return event;
}

void YAML::SetParserLogFlag(bool value)
{
    g_logEnable = value;
}

bool YAML::GetParserLogFlag()
{
    return g_logEnable;
}

bool YAML::convert<AicEventMetadataPtr>::decode(const YAML::Node& node, AicEventMetadataPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    if (node["eventId"] && node["eventId"].IsScalar()) {
        mptr->eventId = node["eventId"].as<unsigned int>();
    }

    if (node["eventName"] && node["eventName"].IsScalar()) {
        std::string eventTypeString = node["eventName"].as<std::string>();
        mptr->eventType = GetEventCode(eventTypeString);
    }

    if (node["direction"] && node["direction"].IsScalar()) {
        std::string directionString = node["direction"].as<std::string>();
        if (directionString == "send")
            mptr->direction = DIRECTION_RECEIVE; //Yaml file specifies this from pov of ICR. Flip for AIC pov
        else if (directionString == "receive")
            mptr->direction = DIRECTION_SEND; //Yaml file specifies this from pov of ICR. Flip for AIC pov
        else
            mptr->direction = DIRECTION_UNKNOWN;
    }

    if (node["timeStampUs"] && node["timeStampUs"].IsScalar()) {
        mptr->timeStampUs = node["timeStampUs"].as<unsigned long>();
    }

    if (node["counts"] && node["counts"].IsMap()) {
        YAML::Node counts = node["counts"];
        mptr->counts.CreateBufferEvents = counts["CreateBufferEvents"].as<unsigned int>();
        mptr->counts.RemoveBufferEvents = counts["RemoveBufferEvents"].as<unsigned int>();
        mptr->counts.DisplayReqEvents = counts["DisplayReqEvents"].as<unsigned int>();
        mptr->counts.ChangeResolutionEvents = counts["ChangeResolutionEvents"].as<unsigned int>();
        mptr->counts.DispInfoReqEvents = counts["DispInfoReqEvents"].as<unsigned int>();
        mptr->counts.DispPortReqEvents = counts["DispPortReqEvents"].as<unsigned int>();
        mptr->counts.SetAlphaEvents    = counts["SetAlphaEvents"].as<unsigned int>();
    }

    return true;
}

bool YAML::convert<DisplayEventPtr>::decode(const YAML::Node& node, DisplayEventPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    mptr->type = node["type"].as<unsigned int>();
    mptr->size = node["size"].as<unsigned int>();
    mptr->id   = node["id"].as<unsigned int>();
    mptr->renderNode = node["renderNode"].as<unsigned int>();

    return true;
}

bool YAML::convert<DisplayInfoPtr>::decode(const YAML::Node& node, DisplayInfoPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    mptr->flags  = node["flags"].as<unsigned int>();
    mptr->width  = node["width"].as<unsigned int>();
    mptr->height = node["height"].as<unsigned int>();
    mptr->stride = node["stride"].as<int>();
    mptr->format = node["format"].as<int>();
    mptr->xdpi   = node["xdpi"].as<float>();
    mptr->ydpi   = node["ydpi"].as<float>();
    mptr->fps    = node["fps"].as<float>();
    mptr->minSwapInterval = node["minSwapInterval"].as<int>();
    mptr->maxSwapInterval = node["maxSwapInterval"].as<int>();
    mptr->numFramebuffers = node["numFramebuffers"].as<int>();

    return true;
}

bool YAML::convert<DisplayPortPtr>::decode(const YAML::Node& node, DisplayPortPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    mptr->port = node["port"].as<unsigned int>();
    mptr->reserve = node["reserve"].as<unsigned int>();
    return true;
}

bool YAML::convert<BufferInfoPtr>::decode(const YAML::Node& node, BufferInfoPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    mptr->remote_handle = node["remote_handle"].as<unsigned long>();
    return true;
}

template <typename T> bool GetSequence(const YAML::Node node, T* dst)
{
    if (dst == nullptr)
        return false;

    if (node.IsSequence())
    {
        for (unsigned int i = 0; i < node.size(); i++) {
            dst[i] = node[i].as<T>();
        }
    }
    else
        return false;

    return true;
}

bool YAML::convert<CrosGrallocHandlePtr>::decode(const YAML::Node& node, CrosGrallocHandlePtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    if (node["native_handle_t"] && node["native_handle_t"].IsMap()) {
        YAML::Node nhNode = node["native_handle_t"];
        mptr->base.version = nhNode["version"].as<int>();
        mptr->base.numFds  = nhNode["numFds"].as<int>();
        mptr->base.numInts = nhNode["numInts"].as<int>();
    }
    else
        return false;


    GetSequence<int>(node["fds"], mptr->fds);

    GetSequence<unsigned int>(node["strides"], mptr->strides);

    GetSequence<unsigned int>(node["offsets"], mptr->offsets);

    GetSequence<unsigned int>(node["sizes"], mptr->sizes);

    GetSequence<unsigned int>(node["format_modifiers"], mptr->format_modifiers);

    mptr->width       = node["width"].as<unsigned int>();
    mptr->height      = node["height"].as<unsigned int>();
    mptr->format      = node["format"].as<unsigned int>();
    mptr->tiling_mode = node["tiling_mode"].as<unsigned int>();

    GetSequence<unsigned int>(node["use_flags"], mptr->use_flags);

    mptr->magic        = node["magic"].as<unsigned int>();
    mptr->pixel_stride = node["pixel_stride"].as<unsigned int>();
    mptr->droid_format = node["droid_format"].as<int>();
    mptr->usage        = node["usage"].as<int>();
    mptr->consumer_usage = node["consumer_usage"].as<unsigned int>();
    mptr->producer_usage = node["producer_usage"].as<unsigned int>();
    mptr->yuv_color_range= node["yuv_color_range"].as<unsigned int>();

    mptr->is_updated = node["is_updated"].as<unsigned int>();
    mptr->is_encoded = node["is_encoded"].as<unsigned int>();
    mptr->is_encrypted = node["is_encrypted"].as<unsigned int>();
    mptr->is_key_frame = node["is_key_frame"].as<unsigned int>();
    mptr->is_interlaced = node["is_interlaced"].as<unsigned int>();
    mptr->is_mmc_capable = node["is_mmc_capable"].as<unsigned int>();

    mptr->compression_mode = node["compression_mode"].as<unsigned int>();
    mptr->compression_hint = node["compression_hint"].as<unsigned int>();
    mptr->codec            = node["codec"].as<unsigned int>();
    mptr->aligned_width    = node["aligned_width"].as<unsigned int>();
    mptr->aligned_height   = node["aligned_height"].as<unsigned int>();

    return true;
}

bool YAML::convert<DisplayInfoEventPtr>::decode(const YAML::Node& node, DisplayInfoEventPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    if (! (node["display_event_t"] && node["display_event_t"].IsMap()))
        return false;

    if (! (node["display_info_t"] && node["display_info_t"].IsMap()))
        return false;

    auto eventPtr = std::make_shared<display_event_t>();
    YAML::convert<DisplayEventPtr>::decode(node["display_event_t"], eventPtr);
    mptr->event = *eventPtr;

    auto infoPtr = std::make_shared<display_info_t>();
    YAML::convert<DisplayInfoPtr>::decode(node["display_info_t"], infoPtr);
    mptr->info= *infoPtr;

    return true;
}

bool YAML::convert<DisplayPortEventPtr>::decode(const YAML::Node& node, DisplayPortEventPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    if (! (node["display_event_t"] && node["display_event_t"].IsMap()))
        return false;

    if (! (node["display_port_t"] && node["display_port_t"].IsMap()))
        return false;

    auto eventPtr = std::make_shared<display_event_t>();
    YAML::convert<DisplayEventPtr>::decode(node["display_event_t"], eventPtr);
    mptr->event = *eventPtr;

    auto portPtr = std::make_shared<display_port_t>();
    YAML::convert<DisplayPortPtr>::decode(node["display_port_t"], portPtr);
    mptr->dispPort= *portPtr;

    return true;
}

bool YAML::convert<BufferInfoEventPtr>::decode(const YAML::Node& node, BufferInfoEventPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    if (! (node["display_event_t"] && node["display_event_t"].IsMap()))
        return false;

    if (! (node["buffer_info_t"] && node["buffer_info_t"].IsMap()))
        return false;

    auto eventPtr = std::make_shared<display_event_t>();
    YAML::convert<DisplayEventPtr>::decode(node["display_event_t"], eventPtr);
    mptr->event = *eventPtr;

    auto infoPtr = std::make_shared<buffer_info_t>();
    YAML::convert<BufferInfoPtr>::decode(node["buffer_info_t"], infoPtr);
    mptr->info= *infoPtr;

    return true;
}

bool YAML::convert<SetAlphaPtr>::decode(const YAML::Node& node, SetAlphaPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    mptr->enable     = node["enable"].as<unsigned int>();
    GetSequence<unsigned int>(node["reserved"], mptr->reserved);

    return true;
}

bool YAML::convert<SetAlphaEventPtr>::decode(const YAML::Node& node, SetAlphaEventPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    if (! (node["display_event_t"] && node["display_event_t"].IsMap()))
        return false;

    if (! (node["set_video_alpha_t"] && node["set_video_alpha_t"].IsMap()))
        return false;

    auto eventPtr = std::make_shared<display_event_t>();
    YAML::convert<DisplayEventPtr>::decode(node["display_event_t"], eventPtr);
    mptr->event = *eventPtr;

    auto alphaPtr = std::make_shared<set_video_alpha_t>();
    YAML::convert<SetAlphaPtr>::decode(node["set_video_alpha_t"], alphaPtr);
    mptr->alpha= *alphaPtr;

    return true;
}

bool YAML::convert<DisplayCtrlPtr>::decode(const YAML::Node& node, DisplayCtrlPtr &mptr)
{
    if (mptr == nullptr || !node.IsMap())
        return false;

    if (! (node["viewport"] && node["viewport"].IsMap()))
        return false;

    mptr->alpha     = node["alpha"].as<unsigned int>();
    mptr->top_layer = node["top_layer"].as<unsigned int>();
    mptr->rotation  = node["rotation"].as<unsigned int>();
    mptr->reserved  = node["reserved"].as<unsigned int>();

    YAML::Node viewport = node["viewport"];
    mptr->viewport.l = viewport["l"].as<int16_t>();
    mptr->viewport.t = viewport["t"].as<int16_t>();
    mptr->viewport.r = viewport["r"].as<int16_t>();
    mptr->viewport.b = viewport["b"].as<int16_t>();

    return true;
}
