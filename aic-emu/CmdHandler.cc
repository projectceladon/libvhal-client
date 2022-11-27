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
#include <unistd.h>

using namespace aic_emu::server;

using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::system_clock;

CmdHandler::CmdHandler(AicConfigData_t& config):
    m_ymlFileName(config.ymlFileName),
    m_inputFileName(config.contentFileName),
    m_gfxDeviceStr(config.deviceString),
    m_props(nullptr),
    m_lastDispReqSentTS(0),
    m_lastDispReqYmlTS(0),
    m_firstDispReqSentTS(0)
{
    //Socket related data
    if (!config.socketInfo.hwc_sock.empty())
    {
        std::string path = config.socketInfo.hwc_sock;
        if (path.find("/hwc-sock") == std::string::npos)
            return;

        size_t pos = path.find("/hwc-sock");
        m_UnixConnInfo.socket_dir = path.substr(0, pos);

        if (getenv("K8S_ENV") == NULL || strcmp(getenv("K8S_ENV"), "true") != 0) {
            // docker env
            m_UnixConnInfo.android_instance_id = config.socketInfo.session_id;
        }
        else
        {
            // k8s env
            m_UnixConnInfo.android_instance_id = -1; //dont need id for k8s env
        }
    }
}

CmdHandler::~CmdHandler()
{
    if (m_inputStream.is_open())
        m_inputStream.close();
}

int CmdHandler::Init()
{
    CHECK_STATUS(InitYmlParser());

    CHECK_STATUS(InitGfxSurfaceHandler());

    CHECK_STATUS(InitSocket());

    return AICS_ERR_NONE;
}

int CmdHandler::InitSocket()
{
    auto sockPath = m_UnixConnInfo.socket_dir;
    if (sockPath.length() == 0) {
        std::cout << "Empty or invalid socket_dir path" << std::endl;
        return AICS_ERR_SOCKET_INIT;
    }
    else
    {
        sockPath += HWC_UNIX_SOCKET;
        if (m_UnixConnInfo.android_instance_id >= 0) {
            sockPath += std::to_string(m_UnixConnInfo.android_instance_id);
        }
    }

    //Creating interface to communicate to libvhal
    m_socket = std::make_unique<UnixServerSocket>(std::move(sockPath));
    if (!m_socket)
        return AICS_ERR_SOCKET_INIT;

    try {
        //Block till client connects
        m_socket->AwaitConnection();
    }
    catch (std::system_error& e) {
        return AICS_ERR_SOCKET_CONNECT;
    }

    return AICS_ERR_NONE;
}

int CmdHandler::WriteIntoSocket(void* data, uint32_t sizeInBytes)
{
    if (!m_socket)
        return AICS_ERR_SOCKET_INIT;

    if (!m_socket->Connected())
        return AICS_ERR_SOCKET_CONNECT;

    if (!data)
        return AICS_ERR_NULL_PTR;

    auto [bytes, errMsg] = m_socket->Send((uint8_t *)data, sizeInBytes);
    if (bytes == -1)
    {
        std::cout << "WriteSocket call failed with error: " << errMsg << std::endl;
        return AICS_ERR_SOCKET;
    }
    return AICS_ERR_NONE;
}

int CmdHandler::ReadFromSocket(void* dst, uint32_t sizeInBytes)
{
    if (!m_socket)
        return AICS_ERR_SOCKET_INIT;

    if (!m_socket->Connected())
        return AICS_ERR_SOCKET_CONNECT;

    if (!dst)
        return AICS_ERR_NULL_PTR;

    auto [bytes, errMsg] = m_socket->Recv((uint8_t*) dst, sizeInBytes);
    if (bytes == -1)
    {
        std::cout << "WriteSocket call failed with error: " << errMsg << std::endl;
        return AICS_ERR_SOCKET;
    }

    return AICS_ERR_NONE;
}

int CmdHandler::SendFDs(boData_t* bo_data)
{
    AIC_LOG();

    if (!bo_data)
        return AICS_ERR_NULL_PTR;

    int fds[MAX_FD_SUPPORTED];
    fds[0] = bo_data->fd;

    auto [bytes, errMsg] = m_socket->SendMsg(fds, sizeof(fds));

    if (bytes < 0)
    {
        std::cout << "SendMsg socket call failed with error: " << errMsg << std::endl;
        return AICS_ERR_SOCKET;
    }

    return AICS_ERR_NONE;
}

void CmdHandler::ManageDisplayReqTime(AicEventMetadataPtr& metadata)
{
    if (m_lastDispReqSentTS == 0)
        return;

    long int waitTime = metadata->timeStampUs - m_lastDispReqYmlTS;

    if (waitTime > 1000000) //Clamp to 1s
        waitTime = 1000000;

    auto curTime = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

    long int remainingWaitUs = waitTime - ((long int)curTime - m_lastDispReqSentTS);

    if (remainingWaitUs > 0)
        usleep(remainingWaitUs);
}

int CmdHandler::Xchng_DisplayRequest(std::vector<std::shared_ptr<void>>& payload, AicEventMetadataPtr& metadata)
{
    // send display_event_t + buffer_info_t (+ optional display_control_t)

    if (payload.empty())
        return AICS_ERR_PAYLOAD_EMPTY;

    auto evInfo = std::static_pointer_cast<buffer_info_event_t, void>(payload[0]);

    // Check if payload data is consistent
    int requestSize = evInfo->event.size - sizeof(display_event_t);
    bool hasCtrl = (requestSize == (sizeof(buffer_info_t) + sizeof(display_control_t)));
    if (hasCtrl && payload.size() != 2)
    {
        std::cout << "Error: display_control_t data specified but missing from parser" << std::endl;
        return AICS_ERR_UNKNOWN;
    }

    AIC_LOG();

    // Prepare frame for display
    int status = 0;
    boData_t* bo_data = m_gfx.GetBo(evInfo->info.remote_handle);
    if (!bo_data)
        return AICS_ERR_NULL_PTR;

    SurfaceParams_t surf = bo_data->surf;
    size_t sizeToRead = surf.width * surf.height * surf.pixelSize;
    uint8_t* pFrame = new uint8_t[sizeToRead];

    status = GetOneFrame(pFrame, sizeToRead);
    CHECK_STATUS(status);

    // Copy data to GPU memory indicated by file-descriptor
    status = m_gfx.Copy(bo_data, pFrame);
    CHECK_STATUS(status);

    // Maintain fps
    ManageDisplayReqTime(metadata);

    // Write Data to sockets
    AIC_LOG();
    status = WriteIntoSocket(&evInfo->event, sizeof(display_event_t));
    CHECK_STATUS(status);
    status = WriteIntoSocket(&evInfo->info, sizeof(buffer_info_t));
    CHECK_STATUS(status);
    if (hasCtrl)
    {
        auto ctrl = std::static_pointer_cast<display_control_t, void>(payload[1]);
        status = WriteIntoSocket(ctrl.get(), sizeof(display_control_t));
        CHECK_STATUS(status);
    }

    // Clean up and Timing book-keeping
    if (pFrame)
        delete[] pFrame;

    m_lastDispReqYmlTS  = metadata->timeStampUs;
    m_lastDispReqSentTS = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    if (m_firstDispReqSentTS == 0)
        m_firstDispReqSentTS = m_lastDispReqSentTS;

    return AICS_ERR_NONE;
}


int CmdHandler::DataExchange(std::vector<std::shared_ptr<void>>& payload, AicEventMetadataPtr& metadata)
{
    int status = AICS_ERR_NONE;

    if (payload.empty())
        return AICS_ERR_PAYLOAD_EMPTY;

    if (metadata->direction == DIRECTION_SEND)
    {
        switch (metadata->eventType)
        {
        case VHAL_DD_EVENT_DISPINFO_REQ:
        case VHAL_DD_EVENT_DISPPORT_REQ:
        {
            AIC_LOG();

            //send display_event_t
            auto event = std::static_pointer_cast<display_event_t>(payload[0]);
            status = WriteIntoSocket(event.get(), sizeof(display_event_t));
            CHECK_STATUS(status);
            break;
        }
        case VHAL_DD_EVENT_CREATE_BUFFER:
        {
            AIC_LOG();
            //send display_event_t + buffer_info_t + cros_gralloc_handle + Special FDs msg
            auto evInfo = std::static_pointer_cast<buffer_info_event_t, void>(payload[0]);
            auto grallocHandle = std::static_pointer_cast<cros_gralloc_handle, void>(payload[1]);
            m_props = grallocHandle;
            std::cout << "width = " << m_props->width << " , height = " << m_props->height << std::endl;
            std::cout << "fds = " << m_props->base.numFds << ", numInts - " << m_props->base.numInts << std::endl;

            SurfaceParams_t surf;
            m_gfx.DetermineSurfaceParams(surf, m_props->width, m_props->height, m_props->format);

            status = m_gfx.AllocateBo(surf, evInfo->info.remote_handle);
            CHECK_STATUS((int)status);

            //Write all msgs to socket
            AIC_LOG();
            status = WriteIntoSocket(&evInfo->event, sizeof(display_event_t));
            CHECK_STATUS(status);
            status = WriteIntoSocket(&evInfo->info, sizeof(buffer_info_t));
            CHECK_STATUS(status);
            status = WriteIntoSocket(payload[1].get(), sizeof(cros_gralloc_handle));
            CHECK_STATUS(status);

            //Prepare and send msg with FDs
            AIC_LOG();
            boData_t* bo_data = m_gfx.GetBo(evInfo->info.remote_handle);
            status = SendFDs(bo_data);
            CHECK_STATUS(status);
            break;
        }
        case VHAL_DD_EVENT_REMOVE_BUFFER:
        {
            AIC_LOG();
            //send display_event_t + buffer_info_t
            auto evInfo = std::static_pointer_cast<buffer_info_event_t, void>(payload[0]);

            status = WriteIntoSocket(&evInfo->event, sizeof(display_event_t));
            CHECK_STATUS(status);
            status = WriteIntoSocket(&evInfo->info, sizeof(buffer_info_t));
            CHECK_STATUS(status);

            status = m_gfx.ClearBo(evInfo->info.remote_handle);
            CHECK_STATUS(status);
            break;
        }
        case VHAL_DD_EVENT_DISPLAY_REQ:
        {
            AIC_LOG();
            status = Xchng_DisplayRequest(payload, metadata);
            CHECK_STATUS(status);
            break;
        }
        default:
        {
            AIC_LOG();
            return AICS_ERR_BAD_METADATA;
        }
        }//switch
    }
    else if (metadata->direction == DIRECTION_RECEIVE)
    {
        switch (metadata->eventType)
        {
        case VHAL_DD_EVENT_DISPINFO_ACK:
        case VHAL_DD_EVENT_SETUP_RESOLUTION:
        {
            AIC_LOG();
            //recv display_info_t
            display_info_t dispInfo;
            int status = ReadFromSocket(&dispInfo, sizeof(display_info_t));
            CHECK_STATUS(status);
            break;
        }
        case VHAL_DD_EVENT_DISPLAY_ACK:
        {
            AIC_LOG();
            buffer_info_event_t dispInfoEvent;
            int status = ReadFromSocket(&dispInfoEvent, sizeof(buffer_info_event_t));
            CHECK_STATUS(status);
            break;
        }
        case VHAL_DD_EVENT_SET_VIDEO_ALPHA_REQ:
        {
            AIC_LOG();
            display_set_video_alpha_event_t alphaEvent;
            int status = ReadFromSocket(&alphaEvent, sizeof(display_set_video_alpha_event_t));
            CHECK_STATUS(status);
            break;
        }
        default:
        {
            AIC_LOG();
            return AICS_ERR_BAD_METADATA;
        }
        }
    }
    else
    {
        AIC_LOG();
        return AICS_ERR_BAD_METADATA;
    }
    std::cout << "==================" << std::endl;

    return AICS_ERR_NONE;
}


int CmdHandler::ProcessNextEntry()
{
    if (m_nextNode == m_ymlDocs.end())
        return AICS_ERR_EOF;

    AIC_LOG();
    std::vector<std::shared_ptr<void>> payload;
    AicEventMetadataPtr metadata = std::make_shared<AicEventMetadata_t>();

    try {
        DecodeNode(*m_nextNode, payload, metadata);
        std::cout << "\tpayload size in count: " << payload.size() << std::endl;
    }
    catch(...) {
        std::cout << "Exception in Parsing  " << m_ymlFileName << std::endl;
        m_nextNode++;
        return AICS_YML_PARSER_ERR;
    }

    m_nextNode++;

    return DataExchange(payload, metadata);
}


int CmdHandler::DecodeNode(YAML::Node& rootNode, std::vector<std::shared_ptr<void>>& payload, AicEventMetadataPtr& metadata)
{
    if (!rootNode.IsMap())
        return AICS_YML_PARSER_ERR;

    auto node = rootNode["event"];

    //---Process Metadata
    if (metadata == nullptr)
        return AICS_ERR_NULL_PTR;

    if (node["metadata"] && node["metadata"].IsMap())
        YAML::convert<AicEventMetadataPtr>::decode(node["metadata"], metadata);
    else
        return AICS_YML_PARSER_ERR;

    std::cout << "{id, type, direction, timestamp} : {" << metadata->eventId << ", " << metadata->eventType << ", " << metadata->direction << ", " << metadata->timeStampUs << "}\n";

    //---Get Event Info
    //Only one of below events is expected
    switch (metadata->eventType)
    {
    case VHAL_DD_EVENT_DISPINFO_REQ:
    case VHAL_DD_EVENT_DISPPORT_REQ:
    {
        //Both cases involve display_event_t
        auto mptr = std::make_shared<display_event_t>();
        YAML::convert<DisplayEventPtr>::decode(node["display_event_t"], mptr);
        std::shared_ptr<void> v_mptr = mptr;
        payload.push_back(v_mptr);
        break;
    }

    case VHAL_DD_EVENT_DISPINFO_ACK:
    case VHAL_DD_EVENT_SETUP_RESOLUTION:
    {
        //Both cases involve display_info_event_t
        auto mptr = std::make_shared<display_info_event_t>();
        YAML::convert<DisplayInfoEventPtr>::decode(node["display_info_event_t"], mptr);
        std::shared_ptr<void> v_mptr = mptr;
        payload.push_back(v_mptr);
        break;
    }

    case VHAL_DD_EVENT_CREATE_BUFFER:
    {
        auto mptr_headers = std::make_shared<buffer_info_event_t>();
        YAML::Node createBufferNode = node["buffer_info_event_t"];
        YAML::convert<BufferInfoEventPtr>::decode(createBufferNode, mptr_headers);
        std::shared_ptr<void> v_mptrHeaders = mptr_headers;
        payload.push_back(v_mptrHeaders);

        auto mptr_gralloc_handle = std::make_shared<struct cros_gralloc_handle>();
        YAML::convert<CrosGrallocHandlePtr>::decode(createBufferNode["cros_gralloc_handle_t"], mptr_gralloc_handle);
        std::shared_ptr<void> v_mptrGrallocHandle = mptr_gralloc_handle;
        payload.push_back(v_mptrGrallocHandle);
        break;
    }

    case VHAL_DD_EVENT_REMOVE_BUFFER:
    case VHAL_DD_EVENT_DISPLAY_ACK:
    {
        //These cases all involve a buffer_info_event_t. Can process them all the same
        auto mptr = std::make_shared<buffer_info_event_t>();
        YAML::convert<BufferInfoEventPtr>::decode(node["buffer_info_event_t"], mptr);
        std::shared_ptr<void> v_mptr = mptr;
        payload.push_back(v_mptr);
        break;
    }

    case VHAL_DD_EVENT_DISPLAY_REQ:
    {
        //This case involves a buffer_info_event_t (Same as above)
        //And potentially an additional display_control_t
        auto mptr = std::make_shared<buffer_info_event_t>();
        YAML::convert<BufferInfoEventPtr>::decode(node["buffer_info_event_t"], mptr);
        std::shared_ptr<void> v_mptr = mptr;
        payload.push_back(v_mptr);

        int requestSize = mptr->event.size - sizeof(display_event_t);
        bool hasCtrl = (requestSize == (sizeof(buffer_info_t) + sizeof(display_control_t)));
        if (hasCtrl)
        {
            auto ctrl = std::make_shared<display_control_t>();
            if (YAML::convert<DisplayCtrlPtr>::decode(node["display_control_t"], ctrl))
            {
                v_mptr = ctrl;
                payload.push_back(v_mptr);
            }
        }
        break;
    }

    case VHAL_DD_EVENT_DISPPORT_ACK:
    {
        auto mptr = std::make_shared<display_port_event_t>();
        YAML::convert<DisplayPortEventPtr>::decode(node["display_port_event_t"], mptr);
        std::shared_ptr<void> v_mptr = mptr;
        payload.push_back(v_mptr);
        break;
    }

    case VHAL_DD_EVENT_SET_VIDEO_ALPHA_REQ:
    {
        auto mptr = std::make_shared<display_set_video_alpha_event_t>();
        YAML::convert<SetAlphaEventPtr>::decode(node["display_set_video_alpha_event_t"], mptr);
        std::shared_ptr<void> v_mptr = mptr;
        payload.push_back(v_mptr);
        break;
    }

    default:
        std::cout << "Unknown event: " << std::to_string(metadata->eventType) << std::endl;
        return AICS_YML_PARSER_ERR;
        break;
    }

    return AICS_ERR_NONE;
}


int CmdHandler::InitYmlParser()
{
    try {
        m_ymlDocs = YAML::LoadAllFromFile(m_ymlFileName);
    }
    catch (...) {
        std::cout << "Exception in Loading  " << m_ymlFileName << std::endl;
        return AICS_YML_LOAD_ERR;
    }

    m_nextNode = m_ymlDocs.begin();
    return AICS_ERR_NONE;
}


int CmdHandler::InitGfxSurfaceHandler()
{
    m_inputStream.open(m_inputFileName, std::ios::binary);

    if (! m_inputStream.good())
        return AICS_ERR_INPUT_STREAM;

    GfxStatus status = GFX_OK;
    status = m_gfx.GfxInit(m_gfxDeviceStr);
    if (status)
        return AICS_ERR_GFX;

    return AICS_ERR_NONE;
}


int CmdHandler::GetOneFrame(uint8_t* pFrame, int bytesToRead)
{
    if (!m_inputStream.good())
        return AICS_ERR_INPUT_STREAM;

    if (!pFrame)
        return AICS_ERR_NULL_PTR;

    if (bytesToRead <= 0)
        return AICS_ERR_STREAM_PARAMS;

    m_inputStream.read((char *)pFrame, bytesToRead);

    //Loop over if meeting EoF
    if (m_inputStream.eof())
    {
        m_inputStream.clear();
        m_inputStream.seekg(0, m_inputStream.beg);
    }

    return AICS_ERR_NONE;
}
