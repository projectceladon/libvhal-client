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

#include "hwc_profile_log.h"
#include <chrono>
#include <cassert>

using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::system_clock;

char evNames[][MAX_STRING_LENGTH] = {
    "EVENT_INITIALIZE",
    "VHAL_DD_EVENT_CREATE_BUFFER",
    "VHAL_DD_EVENT_REMOVE_BUFFER",
    "VHAL_DD_EVENT_DISPLAY_REQ",
    "VHAL_DD_EVENT_DISPINFO_REQ",
    "VHAL_DD_EVENT_DISPPORT_REQ",
    "VHAL_DD_EVENT_DISPLAY_ACK",
    "VHAL_DD_EVENT_DISPINFO_ACK",
    "VHAL_DD_EVENT_DISPPORT_ACK"  ,
    "VHAL_DD_EVENT_SETUP_RESOLUTION",
    "VHAL_DD_EVENT_SET_VIDEO_ALPHA_REQ",
    "EVENT_UNKNOWN"
};

void intArrayToString(string& tmpString, int length, int* intArray)
{
    assert(intArray != nullptr);

    tmpString = "[";
    for (int i = 0; i < length - 1; i++)
        tmpString += to_string(intArray[i]) + ", ";
    tmpString += to_string(intArray[length - 1]) + "]";

    return;
}

void ProfileLogger::SetResolution(int width, int height)
{
    m_width = width;
    m_height = height;
}

bool ProfileLogger::isSendEvent(log_event_t event)
{
    return (event >= SEND_EVENTS_START);
}

log_event_t ProfileLogger::TranslateEvType(int event)
{
    log_event_t translatedEvent;
    switch (event)
    {
        case VHAL_DD_EVENT_DISPINFO_REQ:
            translatedEvent = EVENT_DISPINFO_REQ; break;
        case VHAL_DD_EVENT_DISPINFO_ACK:
            translatedEvent = EVENT_DISPINFO_REQ_ACK ; break;
        case VHAL_DD_EVENT_CREATE_BUFFER:
            translatedEvent = EVENT_CREATE_BUFFER; break;
        case VHAL_DD_EVENT_REMOVE_BUFFER:
            translatedEvent = EVENT_REMOVE_BUFFER; break;
        case VHAL_DD_EVENT_DISPLAY_REQ:
            translatedEvent = EVENT_DISPLAY_REQ; break;
        case VHAL_DD_EVENT_DISPLAY_ACK:
            translatedEvent = EVENT_DISPLAY_REQ_ACK; break;
        case VHAL_DD_EVENT_DISPPORT_REQ:
            translatedEvent = EVENT_DISPPORT_REQ; break;
        case VHAL_DD_EVENT_DISPPORT_ACK:
            translatedEvent = EVENT_DISPPORT_REQ_ACK; break;
        case VHAL_DD_EVENT_SETUP_RESOLUTION:
            translatedEvent = EVENT_CHANGE_RESOLUTION; break;
        case VHAL_DD_EVENT_SET_VIDEO_ALPHA_REQ:
            translatedEvent = EVENT_SET_ALPHA; break;
        default:
            translatedEvent = EVENT_UNKNOWN; break;
    }

    return translatedEvent;
}

log_err_t ProfileLogger::Initialize(int width, int height)
{
    char* env_string = nullptr;
    env_string = getenv("ENABLE_PROFILE_YMLLOG");

    if (env_string != nullptr && atoi(env_string) == 1)
    {
        printf("YML Log of events is enabled\n");
        m_enabled = true;
    }

    if (m_enabled == false)
        return ERR_NONE;

    env_string = getenv("YMLLOG_PATH");
    if (env_string != nullptr)
        m_logFilePath= env_string;

    m_logFilePtr = fopen(m_logFilePath.c_str(), "w");
    if (! m_logFilePtr) {
        printf("Cannot open file to write: %s\n", m_logFilePath.c_str());
        m_enabled = false;
        return ERR_UNWRITABLE_LOGFILE;
    }
    else
        printf("YML Log file opened at %s\n", m_logFilePath.c_str());

    SetResolution(width, height);
    m_initialized = true;

    //Comments to clarify the log
    WriteYmlLog(0, "# This log captures events submitted to libvhal and ICR shared lib");
    WriteYmlLog(0, "# Events Code:");
    for (int i = 1; i < EVENT_TYPE_COUNT; i++)
        WriteYmlLog(0, "# %d: %s", i, evNames[i]);

    return ERR_NONE;
}

log_err_t ProfileLogger::AcquireMutex()
{
    if (!IsEnabled())
        return ERR_NONE;

    try {
        m_log_mutex.lock();
    }
    catch (std::system_error& e) {
        std::cout << "Exception attempting to acquire log mutex: " << e.code()
                  << " (" << e.what() << ")" << std::endl;
        return ERR_LOCKING;
    }

    return ERR_NONE;
}

log_err_t ProfileLogger::ReleaseMutex()
{
    if (!IsEnabled())
        return ERR_NONE;

    try {
        m_log_mutex.unlock();
    }
    catch (std::system_error& e) {
        std::cout << "Exception attempting to release log mutex: " << e.code()
                  << " (" << e.what() << ")" << std::endl;
        return ERR_LOCKING;
    }

    return ERR_NONE;
}

int ProfileLogger::CheckProtocol_BufferInfoEvent(log_event_t event)
{
    //This function checks if display_event_t struct received should be logged as part
    //of a buffer_info_event_t struct in the display protocol.
    //The return value indicates the level for the Subsequent structures to be dumped

    int nextStructLevel = 1;

    if (event == EVENT_CREATE_BUFFER ||
        event == EVENT_REMOVE_BUFFER ||
        event == EVENT_DISPLAY_REQ)
    {
        WriteYmlLog(1, "buffer_info_event_t:");
        WriteYmlLog(2, "struct_size: %lu", sizeof(buffer_info_event_t));
        nextStructLevel += 1;
    }

    return nextStructLevel;
}


log_err_t ProfileLogger::LogGenericEventInfo(log_event_t type, display_event_t* ev)
{
    if (!IsEnabled())
        return ERR_NONE;

    if (m_initialized == false)
        return ERR_UNINITIALIZED;

    auto us_since_epoch = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

    //If not first event, mark end of previous document (each event is one YML "document")
    //This is required to simplify parsing of the generated YML
    //End of document is an optional flag, so it isn't a problem if the end of the last doc isn't marked.
    if (m_reportId != 0)
        WriteYmlLog(0, "\n...");
    //Mark beginning of new YML document
    WriteYmlLog(0, "\n---");

    //Write Event related data
    WriteYmlLog(0, "event:");
    WriteYmlLog(1, "metadata:");
    WriteYmlLog(2, "eventId: %d", m_reportId);
    WriteYmlLog(2, "eventName: %s", evNames[type]);
    WriteYmlLog(2, "direction: %s", isSendEvent(type) ? "send" : "receive");
    WriteYmlLog(2, "timeStampUs: %ld", (long int)us_since_epoch);
    WriteYmlLog(2, "counts:");
    WriteYmlLog(3, "CreateBufferEvents: %d", m_createBufEventCnt);
    WriteYmlLog(3, "RemoveBufferEvents: %d", m_removeBufEventCnt);
    WriteYmlLog(3, "DisplayReqEvents: %d", m_displayReqEventCnt);
    WriteYmlLog(3, "ChangeResolutionEvents: %d", m_changeResolutionCnt);
    WriteYmlLog(3, "DispInfoReqEvents: %d", m_dispInfoReqEventCnt);
    WriteYmlLog(3, "DispPortReqEvents: %d", m_dispPortReqEventCnt);
    WriteYmlLog(3, "SetAlphaEvents: %d", m_setAlphaCnt);

    m_reportId++;

    log_err_t res = ERR_NONE;
    if (ev)
    {
        int level = CheckProtocol_BufferInfoEvent(type);
        res = AddDisplayEventStruct(ev, level);
    }
    return res;
}

log_err_t ProfileLogger::AddDisplayInfoEventStruct(display_info_event_t* ev, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;

    int fl = level + 1; //fieldLevel
    WriteYmlLog(level, "display_info_event_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(display_info_event_t));

    RET_IF_FAIL(AddDisplayEventStruct(&ev->event, fl));
    return AddDisplayInfoStruct(&ev->info, fl);
}

log_err_t ProfileLogger::AddBufferInfoEventStruct(buffer_info_event_t* ev, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;

    int fl = level + 1; //fieldLevel
    WriteYmlLog(level, "buffer_info_event_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(buffer_info_event_t));

    RET_IF_FAIL(AddDisplayEventStruct(&ev->event, fl));
    return AddBufferInfoStruct(&ev->info, fl);
}

log_err_t ProfileLogger::AddDisplayPortEventStruct(display_port_event_t* ev, int level)
{
    if (!IsEnabled())
        return ERR_NONE;

    if (m_initialized == false)
        return ERR_UNINITIALIZED;

    int fl = level + 1; //fieldLevel
    WriteYmlLog(level, "display_port_event_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(display_port_event_t));

    RET_IF_FAIL(AddDisplayEventStruct(&ev->event, fl));
    return AddDisplayPortStruct(&ev->dispPort, fl);
}

log_err_t ProfileLogger::AddDisplayEventStruct(display_event_t* ev, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;

    int fl = level + 1; //fieldLevel

    WriteYmlLog(level, "display_event_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(display_event_t));
    WriteYmlLog(fl, "type: 0x%X", ev->type);
    WriteYmlLog(fl, "size: %d", ev->size);
    WriteYmlLog(fl, "id: %d", ev->id);
    WriteYmlLog(fl, "renderNode: %d", ev->renderNode);

    return ERR_NONE;
}

log_err_t ProfileLogger::AddDisplayInfoStruct(display_info_t* ev, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;
    if (ev == nullptr)
        return ERR_NULL_SRC_DATA;

    int fl = level + 1; //fieldLevel

    WriteYmlLog(level, "display_info_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(display_info_t));
    WriteYmlLog(fl ,"flags: 0x%X", ev->flags);
    WriteYmlLog(fl, "width: %d", ev->width);
    WriteYmlLog(fl, "height: %d", ev->height);
    WriteYmlLog(fl, "stride: %d", ev->stride);
    WriteYmlLog(fl, "format: %d", ev->format);
    WriteYmlLog(fl, "xdpi: %f", ev->xdpi);
    WriteYmlLog(fl, "ydpi: %f", ev->ydpi);
    WriteYmlLog(fl, "fps: %f", ev->fps);
    WriteYmlLog(fl, "minSwapInterval: %d", ev->minSwapInterval);
    WriteYmlLog(fl, "maxSwapInterval: %d", ev->maxSwapInterval);
    WriteYmlLog(fl, "numFramebuffers: %d", ev->numFramebuffers);

    return ERR_NONE;
}

log_err_t ProfileLogger::AddGrallocHandleStruct(cros_gralloc_handle_t handle, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;
    if (handle == nullptr)
        return ERR_NULL_SRC_DATA;

    string tmpString;
    int fl = level + 1; //field level

    WriteYmlLog(level, "cros_gralloc_handle_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(struct cros_gralloc_handle));

    AddNativeHandleStruct(&handle->base, fl);

    intArrayToString(tmpString, DRV_MAX_PLANES, (int*)handle->fds);
    WriteYmlLog(fl, "fds: %s", tmpString.c_str());
    intArrayToString(tmpString, DRV_MAX_PLANES, (int*)handle->strides);
    WriteYmlLog(fl, "strides: %s", tmpString.c_str());
    intArrayToString(tmpString, DRV_MAX_PLANES, (int*)handle->offsets);
    WriteYmlLog(fl, "offsets: %s", tmpString.c_str());
    intArrayToString(tmpString, DRV_MAX_PLANES, (int*)handle->sizes);
    WriteYmlLog(fl, "sizes: %s", tmpString.c_str());
    intArrayToString(tmpString, DRV_MAX_PLANES*2, (int*)handle->format_modifiers);
    WriteYmlLog(fl, "format_modifiers: %s", tmpString.c_str());

    WriteYmlLog(fl, "width: %d", handle->width);
    WriteYmlLog(fl, "height: %d", handle->height);
    WriteYmlLog(fl, "format: %d", handle->format);
    WriteYmlLog(fl, "tiling_mode: %d", handle->tiling_mode);

    intArrayToString(tmpString, 2, (int*)handle->use_flags);
    WriteYmlLog(fl, "use_flags: %s", tmpString.c_str());

    WriteYmlLog(fl, "magic: %u", handle->magic);
    WriteYmlLog(fl, "pixel_stride: %d", handle->pixel_stride);
    WriteYmlLog(fl, "droid_format: %d", handle->droid_format);
    WriteYmlLog(fl, "usage: %d", handle->usage);
    WriteYmlLog(fl, "consumer_usage: %d", handle->consumer_usage);
    WriteYmlLog(fl, "producer_usage: %d", handle->producer_usage);
    WriteYmlLog(fl, "yuv_color_range: %d", handle->yuv_color_range);
    WriteYmlLog(fl, "is_updated: %d", handle->is_updated);
    WriteYmlLog(fl, "is_encoded: %d", handle->is_encoded);
    WriteYmlLog(fl, "is_encrypted: %d", handle->is_encrypted);
    WriteYmlLog(fl, "is_key_frame: %d", handle->is_key_frame);
    WriteYmlLog(fl, "is_interlaced: %d", handle->is_interlaced);
    WriteYmlLog(fl, "is_mmc_capable: %d", handle->is_mmc_capable);
    WriteYmlLog(fl, "compression_mode: %d", handle->compression_mode);
    WriteYmlLog(fl, "compression_hint: %d", handle->compression_hint);
    WriteYmlLog(fl, "codec: %d", handle->codec);
    WriteYmlLog(fl, "aligned_width: %d", handle->aligned_width);
    WriteYmlLog(fl, "aligned_height: %d", handle->aligned_height);

    return ERR_NONE;
}

log_err_t ProfileLogger::AddNativeHandleStruct(native_handle_t* handle, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;
    if (handle == nullptr)
        return ERR_NULL_SRC_DATA;

    int fl = level + 1; //field level
    WriteYmlLog(level, "native_handle_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(native_handle_t));

    WriteYmlLog(fl, "version: %d", handle->version);
    WriteYmlLog(fl, "numFds: %d", handle->numFds);
    WriteYmlLog(fl, "numInts: %d", handle->numInts);
    WriteYmlLog(fl, "data: %ld", sizeof(handle->data));

    return ERR_NONE;
}

log_err_t ProfileLogger::AddDisplayPortStruct(display_port_t* dp, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;
    if (dp == nullptr)
        return ERR_NULL_SRC_DATA;

    int fl = level + 1; //field level
    WriteYmlLog(level, "display_port_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(display_port_t));

    WriteYmlLog(fl, "port: %d", dp->port);
    WriteYmlLog(fl, "reserve: %d", dp->reserve);

    return ERR_NONE;
}

log_err_t ProfileLogger::AddBufferInfoStruct(buffer_info_t* handle, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;
    if (handle == nullptr)
        return ERR_NULL_SRC_DATA;

    int fl = level + 1; //field level
    WriteYmlLog(level, "buffer_info_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(buffer_info_t));
    WriteYmlLog(fl, "remote_handle: %ld", handle->remote_handle);
    WriteYmlLog(fl, "data: %ld", sizeof(handle->data));

    return ERR_NONE;
}

void ProfileLogger::UpdateEventCount(log_event_t event)
{
    if (!IsEnabled())
        return;

    switch (event)
    {
        case EVENT_CREATE_BUFFER:
            m_createBufEventCnt++; break;
        case EVENT_REMOVE_BUFFER:
            m_removeBufEventCnt++; break;
        case EVENT_DISPLAY_REQ:
            m_displayReqEventCnt++; break;
        case EVENT_DISPINFO_REQ:
            m_dispInfoReqEventCnt++; break;
        case EVENT_DISPPORT_REQ:
            m_dispInfoReqEventCnt++; break;
        case EVENT_CHANGE_RESOLUTION:
            m_changeResolutionCnt++; break;
        case EVENT_SET_ALPHA:
            m_setAlphaCnt++; break;
        default:
            break;
    }
}

log_err_t ProfileLogger::LogChangeResolutionEvent(int width, int height, display_info_event_t* ev)
{
    SetResolution(width, height);
    UpdateEventCount(EVENT_CHANGE_RESOLUTION);

    AcquireMutex();

    LogGenericEventInfo(EVENT_CHANGE_RESOLUTION);
    AddDisplayInfoEventStruct(ev);

    ReleaseMutex();

    return ERR_NONE;
}

log_err_t ProfileLogger::AddSetVideoAlphaStruct(set_video_alpha_t* alpha, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;
    if (alpha == nullptr)
        return ERR_NULL_SRC_DATA;

    int fl = level + 1;
    WriteYmlLog(level, "set_video_alpha_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(set_video_alpha_t));

    string tmpString;
    intArrayToString(tmpString, sizeof(alpha->reserved)/sizeof(uint32_t), (int *)alpha->reserved);

    WriteYmlLog(fl, "enable: %d", alpha->enable);
    WriteYmlLog(fl, "reserved: %s", tmpString.c_str());

    return ERR_NONE;
}

log_err_t ProfileLogger::AddVideoAlphaEventStruct(display_set_video_alpha_event_t* alpha_ev, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;
    if (alpha_ev == nullptr)
        return ERR_NULL_SRC_DATA;

    int fl = level + 1;
    WriteYmlLog(level, "display_set_video_alpha_event_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(display_set_video_alpha_event_t));

    RET_IF_FAIL(AddDisplayEventStruct(&alpha_ev->event, fl));

    return AddSetVideoAlphaStruct(&alpha_ev->alpha, fl);
}

log_err_t ProfileLogger::LogSetAlphaEvent(display_set_video_alpha_event_t* alpha_ev)
{
    UpdateEventCount(EVENT_SET_ALPHA);

    AcquireMutex();

    LogGenericEventInfo(EVENT_SET_ALPHA);

    AddVideoAlphaEventStruct(alpha_ev);

    ReleaseMutex();

    return ERR_NONE;
}

log_err_t ProfileLogger::AddDisplayControlStruct(display_control_t* ctrl, int level)
{
    if (!IsEnabled())
        return ERR_NONE;
    if (m_initialized == false)
        return ERR_UNINITIALIZED;
    if (ctrl == nullptr)
        return ERR_NULL_SRC_DATA;

    int fl = level + 1;
    WriteYmlLog(level, "display_control_t:");
    WriteYmlLog(fl, "struct_size: %lu", sizeof(display_control_t));

    WriteYmlLog(fl, "alpha: %d", ctrl->alpha);
    WriteYmlLog(fl, "top_layer: %d", ctrl->top_layer);
    WriteYmlLog(fl, "rotation: %d",  ctrl->rotation);
    WriteYmlLog(fl, "reserved: %d",  ctrl->reserved);

    WriteYmlLog(fl, "viewport:");
    fl++;
    WriteYmlLog(fl, "struct_size: %lu", sizeof(ctrl->viewport));
    WriteYmlLog(fl, "l: %d", ctrl->viewport.l);
    WriteYmlLog(fl, "t: %d", ctrl->viewport.t);
    WriteYmlLog(fl, "r: %d", ctrl->viewport.r);
    WriteYmlLog(fl, "b: %d", ctrl->viewport.b);

    return ERR_NONE;
}
