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

#ifndef HWC_VHAL_IMPL_H
#define HWC_VHAL_IMPL_H

#include <atomic>
#include <string>
#include <functional>
#include <memory>
#include <map>
#include <iostream>
#include <thread>
#include "libvhal_common.h"
#include "hwc_vhal.h"
#include "istream_socket_client.h"

#include "display-protocol.h"
#include "hwc_profile_log.h"
using namespace std;

#define fourcc_code(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
    ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define DRM_FORMAT_NV12_Y_TILED_INTEL fourcc_code('9', '9', '9', '6')
#define DRM_FORMAT_NV12               fourcc_code('N', 'V', '1', '2')

namespace vhal {
namespace client {

class VirtualHwcReceiver::Impl
{
public:
    Impl(unique_ptr<IStreamSocketClient> unix_sock_client, ConfigInfo info, HwcHandler handler): socket_client_{move(unix_sock_client)}, mInfo{move(info)}, mHwcHandler{move(handler)}
    {
        AIC_LOG(mDebug, "info.video_res_width: %d", info.video_res_width);
        AIC_LOG(mDebug, "info.video_res_height: %d", info.video_res_height);
        AIC_LOG(mDebug, "info.video_device: %s", info.video_device.c_str());
        AIC_LOG(mDebug, "info.user_id: %d", info.user_id);
        AIC_LOG(mDebug, "info.unix_conn_info.socket_dir: %s", info.unix_conn_info.socket_dir.c_str());
        AIC_LOG(mDebug, "info.unix_conn_info.android_instance_id: %d", info.unix_conn_info.android_instance_id);

        m_pLog = std::make_unique<ProfileLogger>();
        m_pLog->Initialize(info.video_res_width, info.video_res_height);
    }

    ~Impl()
    {
        if (should_continue_) {
            stop();
        }
    }

    cros_gralloc_handle_t get_handle(uint64_t h)
    {
        try {
	    cros_gralloc_handle_t handle = mHandles.at(h);
	    if (!handle) {
	        AIC_LOG(mDebug, "broken handle: %ld\n", h);
	    }
	    return handle;
	} catch(...) {
	    AIC_LOG(mDebug, "remote handle not found: %ld\n", h);
	    return nullptr;
        }
    }

    int recvFds(int fd, int* pfd, size_t fdlen)
    {
        int ret = 0;
        int count = 0;
        int i = 0;
        struct msghdr msg;
        int rdata[4] = {0};
        struct iovec vec;
        char cmsgbuf[CMSG_SPACE(fdlen * sizeof(int))];
        struct cmsghdr* p_cmsg;
        int* p_fds;

	if (pfd == NULL) {
	    return -1;
	}

	if ((fdlen == 0) || (fdlen > DRV_MAX_PLANES)) {
	    AIC_LOG(mDebug, "wrong fdlen(%zd), it should be less than the DRV_MAX_PLANES\n", fdlen);
	    return -1;
	}

        vec.iov_base = rdata;
        vec.iov_len = 16;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &vec;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgbuf;
        msg.msg_controllen = sizeof(cmsgbuf);
        msg.msg_flags = 0;

        p_fds = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
        count = recvmsg(fd, &msg, MSG_WAITALL);
        if (count < 0) {
            printf("Failed to recv fd from remote\n");
            ret = -1;
        } else {
            p_cmsg = CMSG_FIRSTHDR(&msg);
            if (p_cmsg == NULL) {
                printf("No msg hdr\n");
                ret = -1;
            } else {
                p_fds = (int*)CMSG_DATA(p_cmsg);
                for (i = 0; i < (int)fdlen; i++) {
                    pfd[i] = p_fds[i];
               }
            }
        }
        return ret;
    }

    bool init()
    {
        if ((mInfo.video_res_width <= 0) || (mInfo.video_res_height <=0)) {
            AIC_LOG(mDebug, "%s", "video width/height not set\n");
            return false;
        }

        if (mInfo.video_device.empty()) {
            AIC_LOG(mDebug, "%s", "video device not specified\n");
            return false;
        }

        const size_t pos = mInfo.video_device.find_last_of('D');
        if (pos == std::string::npos) {
            AIC_LOG(mDebug, "wrong video device: %s\n", mInfo.video_device.c_str());
            return false;
        }

        const auto randerNodeStr = mInfo.video_device.substr(pos + 1);
        renderNode = std::stoi(randerNodeStr);
        return true;
    }
    IOResult start()
    {
        std::string error_msg = "";

        if (should_continue_ == true) {
            error_msg = "ERROR: Already started!";
            return {-1, error_msg};
        }

        should_continue_ = true;
        mWorkThread = std::unique_ptr<std::thread>(new std::thread(&Impl::workThreadProc, this));
        AIC_LOG(mDebug, "start is: %s", "successful!");
        return {0, error_msg};
    }
    IOResult stop()
    {
        std::string error_msg = "";
        if (should_continue_ == false) {
            error_msg = "ERROR: it's not started, please start firstly!";
	    return {-1, error_msg};
        }

        should_continue_ = false;
        mWorkThread->join();
        socket_client_->Close();
        // Free the buffer handles
        for (auto const& [rh, lh] : mHandles) {
            if (mHwcHandler) {
                frame_info_t frame = {.handle = lh, .ctrl = nullptr};
                mHwcHandler(FRAME_REMOVE, &frame);
            }
            close(lh->fds[0]);
            free(lh);
        }
        mHandles.clear();

        AIC_LOG(mDebug, "stop is: %s", "successful!");
        return {0, error_msg};
    }
    IOResult setMode(int width, int height)
    {
        std::string error_msg = "";
        if (should_continue_ == false) {
            error_msg = "ERROR: it's not started, please start firstly!";
            return {-1, error_msg};
        }
        display_info_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.event.type = VHAL_DD_EVENT_SETUP_RESOLUTION;
        ev.event.size = sizeof(ev);

        ev.info.flags = 1;
        ev.info.width = width;
        ev.info.height = height;
        ssize_t len = 0;
        len = send(socket_client_->GetNativeSocketFd(), &ev, sizeof(ev), 0);
        if (len <= 0) {
            error_msg = std::strerror(errno);
            return {-1, error_msg};
        }
        m_pLog->LogChangeResolutionEvent(width, height, &ev);

        return {0, error_msg};
    }

    IOResult setVideoAlpha(uint32_t action)
    {
        std::string error_msg = "";
        if (should_continue_ == false) {
            error_msg = "ERROR: it's not started, please start firstly!";
            return {-1, error_msg};
        }
        display_set_video_alpha_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.event.type = VHAL_DD_EVENT_SET_VIDEO_ALPHA_REQ;
        ev.event.size = sizeof(ev);

        ev.alpha.enable = action;
        ssize_t len = 0;
        len = send(socket_client_->GetNativeSocketFd(), &ev, sizeof(ev), 0);
        if (len <= 0) {
            error_msg = std::strerror(errno);
            return {-1, error_msg};
        }
        m_pLog->LogSetAlphaEvent(&ev);

        return {0, error_msg};
    }

    void workThreadProc()
    {
        while (should_continue_) {
            while ((not socket_client_->Connected()) && (should_continue_)) {
                if (auto [connected, error_msg] = socket_client_->Connect();
                    !connected) {
                    cout << "Failed to connect to VHal: "
                         << error_msg
                         << ". Retry after 1s...\n";
                    this_thread::sleep_for(1s);
                    continue;
                }
                cout << "Connected to HWC VHal!\n";
            }

            struct pollfd fds[1];
            const int     timeout_ms = 1 * 1000; // 1 sec timeout
            int           ret;

            // watch socket for input
            fds[0].fd     = socket_client_->GetNativeSocketFd();
            fds[0].events = POLLIN;
            // connected ...

	    ret = poll(fds, std::size(fds), timeout_ms);
	    if (ret < 0) {
	        socket_client_->Close();
		continue; //this will goto Connect();
	    } else if (ret == 0) {
	        continue;
	    } else {
                display_event_t ev{};
                ssize_t len = read(socket_client_->GetNativeSocketFd(), &ev, sizeof(ev));
                if (len < 0) {
                    AIC_LOG(mDebug, "can't receive data: %s\n", strerror(errno));
                    break;
                } else if (len == 0) {
                    AIC_LOG(mDebug, "client disconnected: %s\n", strerror(errno));
                    break;
                } else {
                    log_event_t eventType = m_pLog->TranslateEvType(ev.type);
                    m_pLog->UpdateEventCount(eventType);

                    m_pLog->AcquireMutex();
                    m_pLog->LogGenericEventInfo(eventType, &ev);

                    switch (ev.type) {
                        case VHAL_DD_EVENT_DISPINFO_REQ:
                          if (checkDispConfig(ev.id, ev.renderNode) == -1) {
                            goto error;
                          }
                          AIC_LOG(mDebug, "VHAL_DD_EVENT_DISPINFO_REQ\n");
                          UpdateDispConfig(socket_client_->GetNativeSocketFd());
                          break;
                        case VHAL_DD_EVENT_DISPPORT_REQ:
                          AIC_LOG(mDebug, "VHAL_DD_EVENT_DISPPORT_REQ\n");
                          UpdateDispPort(socket_client_->GetNativeSocketFd());
                          break;
                        case VHAL_DD_EVENT_CREATE_BUFFER:
                          AIC_LOG(mDebug, "VHAL_DD_EVENT_CREATE_BUFFER\n");
                          if (ev.size == sizeof(buffer_info_event_t) + sizeof(cros_gralloc_handle)) {
                              CreateBuffer(socket_client_->GetNativeSocketFd());
                          }
                          break;
                        case VHAL_DD_EVENT_REMOVE_BUFFER:
                          AIC_LOG(mDebug, "VHAL_DD_EVENT_REMOVE_BUFFER\n");
                          RemoveBuffer(socket_client_->GetNativeSocketFd());
                          break;
                        case VHAL_DD_EVENT_DISPLAY_REQ:
                          //AIC_LOG(mDebug, "VHAL_DD_EVENT_DISPLAY_REQ\n");
                          DisplayRequest(socket_client_->GetNativeSocketFd(), ev.size - sizeof(ev));
                          break;
                        default:
                          AIC_LOG(mDebug, "VHAL_DD_EVENT_<unknown>: ev.type=%d\n", ev.type);
                          break;
                    } // end of switch

                    m_pLog->ReleaseMutex();
                } //end of else
            }
        } // end of while

        error:
            socket_client_->Close();
            AIC_LOG(mDebug, "working thread stopped, please re-start() !!!");

        return;
    }

    int checkDispConfig(int aicSession, int aicRenderNodeMinus128) {
        if (mInfo.unix_conn_info.android_instance_id < 0) {
	    //for K8S case
	    return 0;
	}
        const int aicRenderNode = aicRenderNodeMinus128 + 128;
        if (renderNode == -1 || renderNode != aicRenderNode) {
            AIC_LOG(mDebug, "wrong renderNode. AIC renderNode=%d, expected renderNode=%d\n", aicRenderNode, renderNode);
	    return -1;
	}

	if (mInfo.unix_conn_info.android_instance_id != aicSession) {
	    AIC_LOG(mDebug, "unmatched AIC session ID. AIC id=%d, expected id=%d\n", aicSession, mInfo.unix_conn_info.android_instance_id);
	    return -1;
	}
	AIC_LOG(mDebug, "current AIC ID=%d\n", mInfo.unix_conn_info.android_instance_id);
	AIC_LOG(mDebug, "current renderNode=%d\n", renderNode);
	return 0;
    }

    void UpdateDispConfig(int fd)
    {
        display_info_event_t ev{};

        ev.event.type = VHAL_DD_EVENT_DISPINFO_ACK;
        ev.event.size = sizeof(ev);

        ev.info.flags = 1;
        ev.info.width = mInfo.video_res_width;
        ev.info.height = mInfo.video_res_height;
        ev.info.stride = ev.info.width;
        ev.info.format = 5;
        ev.info.xdpi = 240;
        ev.info.ydpi = 240;
        ev.info.fps = 60;
        ev.info.minSwapInterval = 1;
        ev.info.maxSwapInterval = 1;
        ev.info.numFramebuffers = 2;

        ssize_t len = 0;
        len = send(fd, &ev, sizeof(ev), 0);
        if (len <= 0) {
            AIC_LOG(mDebug, "send() failed: %s\n", strerror(errno));
        }

        m_pLog->LogGenericEventInfo(EVENT_DISPINFO_REQ_ACK);
        m_pLog->AddDisplayInfoEventStruct(&ev);
    }

    void UpdateDispPort(int fd) {
      display_port_event_t ev{};

      ev.event.type = VHAL_DD_EVENT_DISPPORT_ACK;
      ev.event.size = sizeof(ev);
      ev.dispPort.port = mInfo.user_id;

      ssize_t len = 0;
      len = send(fd, &ev, sizeof(ev), 0);
      if (len <= 0) {
          AIC_LOG(mDebug, "send() failed: %s\n", strerror(errno));
      }

      m_pLog->LogGenericEventInfo(EVENT_DISPPORT_REQ_ACK);
      m_pLog->AddDisplayPortEventStruct(&ev);
    }

    /* msg has 2 parts: header and body
     *
     * header:
     * 1) display_event_t::type, create/remove/display
     * 2) display_event_t::size, total size
     * 3) buffer_info_t::remote_handle, buffer handle
     *
     * body: cros_gralloc_handle_t
     */
    int CreateBuffer(int fd)
    {
        buffer_info_event_t ev{};
        ssize_t len = 0;
        int ret = -1;

        auto handle = (cros_gralloc_handle_t)malloc(sizeof(cros_gralloc_handle));
        if (handle == nullptr) {
            AIC_LOG(mDebug, "Failed to allocate local buffer handle: %s\n", strerror(errno));
            ret = -errno;
            return ret;
        }

        len = recv(fd, &ev.info, sizeof(ev.info), 0);
        if (len <= 0) {
            free(handle);
            AIC_LOG(mDebug, "Failed to read buffer info: %s\n", strerror(errno));
            return -1;
        }
        m_pLog->AddBufferInfoStruct(&ev.info, 2);

        len = recv(fd, handle, sizeof(cros_gralloc_handle), 0);
        if (len <= 0) {
            free(handle);
            AIC_LOG(mDebug, "Failed to read buffer info: %s\n", strerror(errno));
            return -1;
        }

        if (recvFds(fd, handle->fds, handle->base.numFds) == -1) {
            free(handle);
            return -1;
        }
        // Unified NV12 format
        if (handle->format == DRM_FORMAT_NV12_Y_TILED_INTEL) {
            handle->format = DRM_FORMAT_NV12;
        }

        //Dump YML log after file descriptors are captured into "handle"
        m_pLog->AddGrallocHandleStruct(handle, 2);

        AIC_LOG(mDebug, "createBuffer width(%d)height(%d)\n", handle->width, handle->height);
        mHandles.insert(std::make_pair(ev.info.remote_handle, handle));
        frame_info_t frame = {.handle = handle, .ctrl = nullptr};
        mHwcHandler(FRAME_CREATE, &frame);

        return 0;
    }

    int RemoveBuffer(int fd)
    {
        buffer_info_event_t ev{};
        ssize_t len = 0;

        len = recv(fd, &ev.info, sizeof(ev.info), 0);
        if (len <= 0) {
            AIC_LOG(mDebug, "Failed to read buffer info: %s\n", strerror(errno));
            return -1;
        }
        m_pLog->AddBufferInfoStruct(&ev.info, 2);

        cros_gralloc_handle_t handle = get_handle(ev.info.remote_handle);
        if (!handle) {
            return -1;
        }
        frame_info_t frame = {.handle = handle, .ctrl = nullptr};
        mHwcHandler(FRAME_REMOVE, &frame);

        close(handle->fds[0]);
        free(handle);

        mHandles.erase(ev.info.remote_handle);

        return 0;
    }

    int DisplayRequest(int fd, int size) {
        buffer_info_event_t ev{};
        ssize_t len = 0;
        if (size < (int)sizeof(ev.info)) {
            AIC_LOG(mDebug, "Wrong data size in displayBuffer message\n");
            return -1;
        }
        len = recv(fd, &ev.info, sizeof(ev.info), 0);
        if (len <= 0) {
            AIC_LOG(mDebug, "Failed to read buffer info: %s\n", strerror(errno));
            return -1;
        }
        m_pLog->AddBufferInfoStruct(&ev.info, 2);

        display_control_t ctrl{};
        bool hasCtrl = (size == (sizeof(ev.info) + sizeof(display_control_t)));
        if (hasCtrl) {
            len = recv(fd, &ctrl, sizeof(display_control_t), 0);
            if (len <= 0) {
                AIC_LOG(mDebug, "Failed to read display control info: %s\n", strerror(errno));
                return -1;
            }
            m_pLog->AddDisplayControlStruct(&ctrl, 1);
        }

        cros_gralloc_handle_t handle = get_handle(ev.info.remote_handle);
        if (!handle) {
            return -1;
        }

        frame_info_t frame = {.handle = handle, .ctrl = hasCtrl ? &ctrl : nullptr};
        mHwcHandler(FRAME_DISPLAY, &frame);

        ev.event.type = VHAL_DD_EVENT_DISPLAY_ACK;
        ev.event.size = sizeof(ev);
        len = send(fd, &ev, sizeof(ev), 0);
        if (len <= 0) {
            AIC_LOG(mDebug, "send() failed: %s\n", strerror(errno));
            return -1;
        }
        m_pLog->LogGenericEventInfo(EVENT_DISPLAY_REQ_ACK);
        m_pLog->AddBufferInfoEventStruct(&ev);

        return 0;
    }

    private:
        unique_ptr<IStreamSocketClient> socket_client_;
        struct ConfigInfo mInfo;
        HwcHandler  mHwcHandler = nullptr;
        atomic<bool> should_continue_ = false;
        int renderNode = -1;
        std::unique_ptr<std::thread> mWorkThread;
        int sockClientFd = -1;
        int mDebug = 2;
        std::map<uint64_t, cros_gralloc_handle_t> mHandles;
        std::unique_ptr<ProfileLogger> m_pLog;

};
}
}
#endif
