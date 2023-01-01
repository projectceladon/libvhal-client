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

#ifndef __GFX_HANDLER_H__
#define __GFX_HANDLER_H__

#include <sys/ioctl.h>
#include <drm/i915_drm.h>
#include <drm/drm_fourcc.h>

#include <map>

#define GFX_SUCCESS(_res) \
    (GFX_OK == (_res))

#define GFX_LOG_STATUS(_failed, _msg)   \
    GFX_LOG(_failed, _msg, true);

#define GFX_LOG_FAILURE(_failed, _msg)  \
    GFX_LOG(_failed, _msg, _failed);

#define GFX_LOG(failed, msg, condition) do {                                                          \
    std::string printMsg(msg);                                                                        \
    printMsg += ((failed) ? " failed " : " succeeded");                                               \
    if (condition) {                                                                                  \
        printMsg += ":(" + std::string(__FILE__) +  ":line " + std::to_string(__LINE__) + ")";        \
        std::cout << printMsg << std::endl;                                                           \
        if ((failed))                                                                                 \
            std::cout <<"\tError " << errno << ":" << strerror(errno) <<" (" << __FUNCTION__ <<")\n"; \
    }                                                                                                 \
} while(0);                                                                                           \

#define CHECK_ERRNO_STATUS(status)                                      \
    if ((status) != 0) {                                                \
        printf("Encountered error in %s (line %d) : Error %d (%s)\n", __FUNCTION__, __LINE__, errno, strerror(errno)); \
    }

#define ALIGN(X, N) (N)*(((X) + (N) - 1)/(N))

enum GfxStatus
{
    GFX_OK = 0,
    GFX_FAIL = -1
};

enum surf_format
{
    DRM_FORMAT_LINEAR             = 0,
    DRM_FORMAT_YVU420_ANDROID     = 1,
    DRM_FORMAT_NV12_Y_TILED_INTEL = 2
};

struct SurfaceParams_t
{
    // --- Sent to ICR and used
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int format;

    // --- Not used by ICR, but sent to it
    unsigned int tilingFormat;
    unsigned int alignedHeight;
    unsigned int alignedWidth;

    // --- For internal usage
    unsigned int pixelSize;
    unsigned int totalSize;
};

struct boData_t
{
    // Parameters of a bo / surface
    SurfaceParams_t surf;

    // DRM Prime File descriptor corresponding to allocated surface
    // Passed to ICR along with a remote handle from the YML log (generated in AIC allocation)
    int             fd;

    // Real handle capturing the surface allocation in this emulator app. This is NOT sent to ICR
    // The remote handle is simply used to lookup the corresponding prime FD.
    // This is served by the handle listed in the YML log dumped by AIC, so we just send that instead.
    unsigned int    handle;

    // Memory mapping address
    void*           mmap_addr;
};

struct GfxDeviceProps_t
{
    std::string   version;
    int           chipsetId;
    int           hasMMAPoffset;
};

class GfxHandler
{
public:

    GfxHandler() {};
    ~GfxHandler();

    GfxStatus GfxInit(std::string deviceString);
    GfxStatus GfxClose();
    GfxStatus AllocateBo(SurfaceParams_t& surf, uint64_t handle);
    GfxStatus ClearBo(uint64_t handle, bool erase = true);
    GfxStatus Copy(boData_t* dstData, const void* srcBuf);

    GfxStatus AdjustSurfaceParams(SurfaceParams_t& surf);
    boData_t*  GetBo(uint64_t handle);
    unsigned  GetRenderNode() { return m_renderNode; } ;

protected:

    //Internal functions interfacing with Kernel
    GfxStatus GetDrmParam(unsigned long request, int* outPtr);
    GfxStatus RunIoctl(unsigned long request, void* buf);

    GfxStatus MmapBo(boData_t* bo_gem, bool writeEnable);
    GfxStatus UnmapBo(boData_t* bo_gem);

    //Other internal helper functions
    int        GetMmapType(boData_t* bo_gem = nullptr);
    size_t     GetPixelSize(unsigned int format);
    unsigned   GetTilingFormat(unsigned int format);
    void       DetermineRenderNode();

    // State variables
    std::string                  m_device_str;
    int                          m_device_fd;
    GfxDeviceProps_t             m_device_props;
    std::map<uint64_t, boData_t> m_boList;
    int                          m_renderNode = 0;
};
#endif

