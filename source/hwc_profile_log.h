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

#ifndef __HWC_PROFILE_LOG__
#define __HWC_PROFILE_LOG__

#include <iostream>
#include <cstdio>
#include <string>
#include <mutex>
#include "display-protocol.h"

using namespace vhal::client;
using std::string;
using std::to_string;

#define DEFAULT_LOGFILE_NAME "/tmp/ICR.yml"
#define MAX_STRING_LENGTH 1024
#define MAX_FILENAME_LENGTH MAX_STRING_LENGTH
#define SPC4 "    "

#define WriteYmlLog(level, ...)                 \
    do  {                                       \
        for (int i = 0; i < (level); i++) {     \
            fprintf(m_logFilePtr, "%s", SPC4);  \
        }                                       \
        fprintf(m_logFilePtr,  __VA_ARGS__);    \
        fprintf(m_logFilePtr, "\n");            \
        fflush(m_logFilePtr);                   \
    }  while(0);

#define RET_IF_FAIL(statements)                 \
    do {                                        \
        log_err_t res = (statements);           \
        if (res != ERR_NONE)                    \
            return res;                         \
    }                                           \
    while(0);

enum log_err_t
{
    ERR_NONE = 0,
    ERR_UNWRITABLE_LOGFILE = 1,
    ERR_UNINITIALIZED=2,
    ERR_NULL_SRC_DATA=3,
    ERR_LOCKING=4,
    ERR_UNKNOWN=5,
    ERR_TYPE_COUNT
};

enum log_event_t
{
    EVENT_INITIALIZE = 0,
    RECV_EVENTS_START,
    EVENT_CREATE_BUFFER = RECV_EVENTS_START ,
    EVENT_REMOVE_BUFFER,
    EVENT_DISPLAY_REQ,
    EVENT_DISPINFO_REQ,
    EVENT_DISPPORT_REQ,
    SEND_EVENTS_START,
    EVENT_DISPLAY_REQ_ACK = SEND_EVENTS_START,
    EVENT_DISPINFO_REQ_ACK,
    EVENT_DISPPORT_REQ_ACK,
    EVENT_CHANGE_RESOLUTION,
    EVENT_SET_ALPHA,
    EVENT_UNKNOWN,
    EVENT_TYPE_COUNT
};

class ProfileLogger
{
public:
    ProfileLogger(const char* fname = nullptr)
    {
        m_logFilePath = (fname != nullptr) ? fname : DEFAULT_LOGFILE_NAME;
    }

    ~ProfileLogger()
    {
        if(m_logFilePtr)
            fclose(m_logFilePtr);
    }

    log_err_t Initialize(int width=0, int height=0);
    log_err_t LogGenericEventInfo(log_event_t eventType, display_event_t* ev=nullptr);
    log_err_t LogChangeResolutionEvent(int width, int height, display_info_event_t* ev);
    log_err_t LogSetAlphaEvent(display_set_video_alpha_event_t* alpha_ev);

    log_err_t AddDisplayInfoEventStruct(display_info_event_t* ev, int level=1);
    log_err_t AddBufferInfoEventStruct(buffer_info_event_t* ev, int level=1);
    log_err_t AddDisplayPortEventStruct(display_port_event_t* ev, int level=1);

    log_err_t AddDisplayEventStruct(display_event_t* ev, int level=1);
    log_err_t AddDisplayInfoStruct(display_info_t* ev, int level=1);
    log_err_t AddGrallocHandleStruct(cros_gralloc_handle_t handle, int level=1);
    log_err_t AddNativeHandleStruct(native_handle_t* handle, int level=1);
    log_err_t AddDisplayPortStruct(display_port_t* dp, int level=1);
    log_err_t AddBufferInfoStruct(buffer_info_t* handle, int level=1);

    log_err_t AddSetVideoAlphaStruct(set_video_alpha_t* alpha, int level=1);
    log_err_t AddVideoAlphaEventStruct(display_set_video_alpha_event_t* alpha_ev, int level=1);
    log_err_t AddDisplayControlStruct(display_control_t* ctrl, int level=1);

    bool isSendEvent(log_event_t event);
    log_event_t TranslateEvType(int event);
    void UpdateEventCount(log_event_t event);
    void SetResolution(int width, int height);
    int CheckProtocol_BufferInfoEvent(log_event_t event);

    inline bool IsEnabled() { return m_enabled; }

    log_err_t AcquireMutex();
    log_err_t ReleaseMutex();

protected:
    string m_logFilePath;
    FILE* m_logFilePtr = nullptr;

    bool m_enabled = false;
    bool m_initialized = false;
    int m_width  = 0;
    int m_height = 0;

    int m_reportId = 0;
    int m_createBufEventCnt    = 0;
    int m_removeBufEventCnt    = 0;
    int m_displayReqEventCnt   = 0;
    int m_changeResolutionCnt  = 0;
    int m_dispInfoReqEventCnt  = 0;
    int m_dispPortReqEventCnt  = 0;
    int m_setAlphaCnt          = 0;

    //Two threads send msgs to AiC. They both need to write to same log file and need mutex to avoid overlap
    //One of these threads also receives and responds to AiC events. This threads sends ACKs to AiC messages
    //Another thread handles pipe messages from client, that get forwarded to AiC
    std::mutex m_log_mutex;
};

#endif
