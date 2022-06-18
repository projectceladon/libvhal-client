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

#ifndef __CMD_HANDLER
#define __CMD_HANDLER__

#include <stdio.h>
#include <cstring>
#include <thread>
#include <fstream>

#include "display-protocol.h"
#include "libvhal_common.h"
#include "yaml-cpp/yaml.h"
#include "YmlParser.h"
#include "GfxHandler.h"
#include "SocketServer.h"

#define DEFAULT_SOCKET_PATH "/ipc"
#define HWC_UNIX_SOCKET "/hwc-sock"

//Support only 1 File descriptor per create buffer event
#define MAX_FD_SUPPORTED 1

#define CHECK_STATUS(status)                                                  \
    if (status != AICS_ERR_NONE) {                                            \
        std::string errString = "Error " + std::to_string(status) + " at ";   \
        errString += std::string(__FILE__) + ": "+ std::to_string( __LINE__); \
        errString += " (" + std::string(__FUNCTION__) + ")";                  \
        std::cout << errString << std::endl;                                  \
        return status;                                                        \
    };

#if (__DEBUG)
#define AIC_LOG() {                                                     \
    std::string str = std::string(__FILE__) + ": "+ std::to_string( __LINE__); \
    str += " (" + std::string(__FUNCTION__) + ")";                      \
    std::cout << str << std::endl;                                      \
    };
#else
#define AIC_LOG() ;
#endif


using namespace aic_emu::server;

enum AicError_t {
    AICS_ERR_NONE = 0,
    AICS_ERR_INVALID_ARGS = 1,
    AICS_FILE_READ_ERR = 2,
    AICS_SOCKET_ERROR = 3,
    AICS_YML_LOAD_ERR = 4,
    AICS_YML_PARSER_ERR = 5,
    AICS_ERR_NULL_PTR = 6,
    AICS_ERR_EOF = 7,
    AICS_ERR_PAYLOAD_EMPTY = 8,
    AICS_ERR_BAD_METADATA = 9,
    AICS_ERR_INPUT_STREAM = 10,
    AICS_ERR_STREAM_PARAMS = 11,
    AICS_ERR_SOCKET_INIT = 12,
    AICS_ERR_SOCKET_CONNECT = 13,
    AICS_ERR_SOCKET = 14,
    AICS_ERR_GFX = 15,
    AICS_ERR_UNKNOWN = 16,
    AICS_TOTAL_ERRORS
};

struct AicSocketData_t
{
    int session_id; //server session Id
    int user_id;    //user Id
    std::string hwc_sock; //socket path
    int android_instance_id;
};


enum EventDirection_t {
    DIRECTION_SEND = 0,
    DIRECTION_RECEIVE = 1,
    DIRECTION_UNKNOWN = 2
};

struct AicEventCounters
{
    unsigned int CreateBufferEvents;
    unsigned int RemoveBufferEvents;
    unsigned int DisplayReqEvents;
    unsigned int ChangeResolutionEvents;
    unsigned int DispInfoReqEvents;
    unsigned int DispPortReqEvents;
    unsigned int SetAlphaEvents;
};


struct AicEventMetadata_t
{
    unsigned int eventId;
    unsigned int eventType;
    unsigned direction;
    unsigned long timeStampUs;
    AicEventCounters counts;
};

class CmdHandler
{
public:
    CmdHandler(std::string ymlFile, std::string inputFile, AicSocketData_t* info);
    ~CmdHandler();

    int Init();

    int ProcessNextEntry();



private:
    //Init functions
    int InitSocket();
    int InitGfxSurfaceHandler();
    int InitYmlParser();

    //YML Parsing
    int DecodeNode(YAML::Node& node, std::vector<std::shared_ptr<void>>& payload, AicEventMetadataPtr& metadata);

    //INPUT File and Gfx Surface Handling + FPS management
    int  GetOneFrame(uint8_t* pFrame, int sizeToRead);
    void ManageDisplayReqTime(AicEventMetadataPtr& metadata);

    //Data to ICR
    int DataExchange(std::vector<std::shared_ptr<void>>& payload, AicEventMetadataPtr& metadata);
    int Xchng_DisplayRequest(std::vector<std::shared_ptr<void>>& payload, AicEventMetadataPtr& metadata);

    //Socket related Functions
    int WriteIntoSocket(void* data, uint32_t sizeInBytes);
    int ReadFromSocket(void* dst, uint32_t sizeInBytes);
    int SendFDs(boData_t* boData);


    //Socket related variables
    UnixConnectionInfo m_UnixConnInfo;
    std::unique_ptr<UnixServerSocket> m_socket;

    //YML Parser Related variables
    std::string                       m_ymlFileName;
    std::vector<YAML::Node>           m_ymlDocs;
    std::vector<YAML::Node>::iterator m_nextNode;

    //INPUT File and Gfx Surface Handling
    std::string                       m_inputFileName;
    std::ifstream                     m_inputStream;
    GfxHandler                        m_gfx;

    //Struct holding operating surface parameters
    std::shared_ptr<cros_gralloc_handle> m_props;

    //fps management
    unsigned long                     m_lastDispReqSentTS;
    unsigned long                     m_lastDispReqYmlTS;
    unsigned long                     m_firstDispReqSentTS;
};

#endif
