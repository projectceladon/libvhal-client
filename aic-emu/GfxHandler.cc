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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include <unistd.h>
#include <fcntl.h>

#include <dirent.h>

#include <errno.h>

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#include "GfxHandler.h"

#define DEFAULT_GFX_DEVICE "/dev/dri/renderD128"

GfxHandler::~GfxHandler()
{
    for (auto it = m_boList.begin(); it != m_boList.end();)
    {
        //De-allocate BO
        ClearBo(it->first, false);

        //Erase from BO map
        it = m_boList.erase(it);
    }

    if (m_device_fd > 0)
    {
        GfxStatus status = GFX_OK;
        status = GfxClose();
        GFX_LOG_STATUS(status != GFX_OK, "GfxClose");
    }
}

GfxStatus GfxHandler::RunIoctl(unsigned long request, void* buf)
{
   int sts = GFX_OK;

   if (!buf)
   {
       std::cout <<std::string(__FUNCTION__) << ": Non-null pointer required for buf" << std::endl;
       return GFX_FAIL;
   }

   do
   {
     sts = ioctl(m_device_fd, request, buf);
   }
   while (sts == -1 && (errno == EINTR || errno == EAGAIN));

   std::string errorString = std::string(__FUNCTION__) + ": Req " + std::to_string(request);
   GFX_LOG_FAILURE(sts != 0, errorString.c_str());

   return (GfxStatus) sts;
}

GfxStatus GfxHandler::GetDrmParam(unsigned long request, int* val)
{
   if (!val)
   {
       std::cout <<std::string(__FUNCTION__) << ": Non-null pointer required for val" << std::endl;
       return GFX_FAIL;
   }

   struct drm_i915_getparam gp = { .param = (int)request, .value = val };

   return RunIoctl(DRM_IOCTL_I915_GETPARAM, &gp);
}

GfxStatus GfxHandler::GfxInit(std::string deviceString)
{
    int sts = GFX_OK;
    int mmap_gtt_version = -1;

    m_device_str = deviceString;
    if(m_device_str.empty())
    {
        std::cout << "Using Default Gfx Device: " << DEFAULT_GFX_DEVICE << std::endl;
        m_device_str = DEFAULT_GFX_DEVICE;
    }

    std::cout << "Using device " << m_device_str << std::endl;

    //Init device props to 0
    m_device_props = {.chipsetId = 0, .hasMMAPoffset = 0};

    //Open device
    m_device_fd = open(m_device_str.c_str(), O_RDWR);
    sts = (m_device_fd < 0) ? GFX_FAIL : GFX_OK;
    GFX_LOG_STATUS(sts != GFX_OK, "open device");
    if (!GFX_SUCCESS(sts))
        goto exit_logic;

    //Get version info
    struct drm_version drm_ver;
    memset(&drm_ver, 0, sizeof(drm_ver));

    sts = RunIoctl(DRM_IOCTL_VERSION, &drm_ver);

    GFX_LOG_FAILURE(sts != GFX_OK, "drmIoctl DRM_IOCTL_VERSION first");
    if (!GFX_SUCCESS(sts))
        goto exit_logic;

    //Allocate memory and call the ioctl again to actually get the name
    if (drm_ver.name_len > 0)
    {
        drm_ver.name = (char *)malloc(drm_ver.name_len + 1);

        sts = RunIoctl(DRM_IOCTL_VERSION, &drm_ver);

        GFX_LOG_FAILURE(sts != GFX_OK, "drmIoctl DRM_IOCTL_VERSION second");

        if (drm_ver.name_len >=0)
            drm_ver.name[drm_ver.name_len] = '\0';

        m_device_props.version = (const char *) drm_ver.name;

        if (drm_ver.name)
            free(drm_ver.name);

        if (!GFX_SUCCESS(sts))
            goto exit_logic;
    }

    //Get Chipset ID
    sts = GetDrmParam(I915_PARAM_CHIPSET_ID, &m_device_props.chipsetId);
    GFX_LOG_FAILURE(sts != 0, "drmIoctl I915_PARAM_CHIPSET_ID");
    if (!GFX_SUCCESS(sts))
        goto exit_logic;


    //MMAP offset
    sts = GetDrmParam(I915_PARAM_MMAP_GTT_VERSION, &mmap_gtt_version);
    if (!GFX_SUCCESS(sts))
        goto exit_logic;
    m_device_props.hasMMAPoffset = (mmap_gtt_version >= 4);

    if (!m_device_props.hasMMAPoffset)
    {
        std::cout << "Error: Device doesn't have MMAPoffset function." << std::endl;
        sts = GFX_FAIL;
    }

exit_logic:
    if (!GFX_SUCCESS(sts))
    {
        if (m_device_fd >= 0)
        {
            close(m_device_fd);
            m_device_fd = -1;
        }
    }

    std::cout << std::string(__FUNCTION__) << ": drmVersion.name = " << m_device_props.version << "\n"
              << "\tchipsetID = 0x" << std::hex  << m_device_props.chipsetId << std::dec << "\n"
              << "\thasMMAPoffset = " << m_device_props.hasMMAPoffset << std::endl;

    return (GfxStatus)sts;
}

GfxStatus GfxHandler::GfxClose()
{
    GfxStatus sts = GFX_OK;

    if (m_device_fd >= 0)
    {
        close(m_device_fd);
        m_device_fd = -1;
    }
    return sts;
}

GfxStatus GfxHandler::DetermineSurfaceParams(SurfaceParams_t& surf, unsigned width, unsigned height, unsigned format)
{
    if (width == 0 || height == 0 || format == 0)
    {
        std::cout << "Error: Unexpected surface parameter: width = " << width
                  << ", height = " << height << ", format = " << format << std::endl;

        return GFX_FAIL;
    }

	uint32_t horizontal_alignment = 4;
	uint32_t vertical_alignment = 4;

    memset(&surf, 0, sizeof(SurfaceParams_t));

    surf.width  = width;
    surf.height = height;
    surf.format = format;
    surf.pitch  = surf.width;

    surf.tilingFormat = GetTilingFormat(format);

    switch (surf.tilingFormat)
    {
    default:
    case I915_TILING_NONE:
        horizontal_alignment = 64;
        break;

    case I915_TILING_X:
        horizontal_alignment = 512;
        vertical_alignment = 8;
        break;

    case I915_TILING_Y:
        horizontal_alignment = 128;
        vertical_alignment = 32;
        break;
    }

    /*
	 * The alignment calculated above is based on the full size luma plane and to have chroma
	 * planes properly aligned with subsampled formats, we need to multiply luma alignment by
	 * subsampling factor.
	 */
	switch (surf.format)
    {
	case DRM_FORMAT_YVU420_ANDROID:
	case DRM_FORMAT_YVU420:
		horizontal_alignment *= 2;

	case DRM_FORMAT_NV12:
		vertical_alignment *= 2;
		break;

    case DRM_FORMAT_NV12_Y_TILED_INTEL:
        vertical_alignment = 64;
        break;

    default:
        break;
	}

	surf.alignedHeight = ALIGN(surf.height, vertical_alignment);

    surf.alignedWidth = ALIGN(surf.width, horizontal_alignment);

    surf.pitch = surf.alignedWidth;

    surf.pixelSize = GetPixelSize(surf.format);

    surf.totalSize = surf.alignedWidth * surf.alignedHeight * surf.pixelSize;

    /*
	 * Quoting Mesa ISL library:
	 *
	 *    - For linear surfaces, additional padding of 64 bytes is required at
	 *      the bottom of the surface. This is in addition to the padding
	 *      required above.
	 */
	if (surf.tilingFormat == I915_TILING_NONE)
		surf.totalSize += 64;

	return GFX_OK;
}

unsigned GfxHandler::GetTilingFormat(unsigned int format)
{
    //Currently only linear format is supported.
    return I915_TILING_NONE;
}

size_t GfxHandler::GetPixelSize(unsigned int format)
{
    //Currently only 4-byte packed format is supported.
    return 4;
}

GfxStatus GfxHandler::AllocateBo(SurfaceParams_t& surf, uint64_t remote_handle)
{
    int sts = GFX_OK;

    std::string errString;

    if (surf.tilingFormat != I915_TILING_NONE)
    {
        std::cout << "Error: Unsupported Tiling format: " << surf.tilingFormat << std::endl;
        return GFX_FAIL;
    }

#if (__DEBUG)
    printf("surf = {w = %d, h = %d, p = %d, f = %d,\n", surf.width, surf.height, surf.pitch, surf.format);
    printf("surf = {tf = %d, aH = %d, aW = %d,\n", surf.tilingFormat, surf.alignedHeight, surf.alignedWidth);
    printf("surf = {pixelSize = %d, totalSize = %d\n", surf.pixelSize, surf.totalSize);
#endif

    // -- Ioctl to create surface
    struct drm_i915_gem_create gem_create;
    memset(&gem_create, 0, sizeof(gem_create));
    gem_create.size = surf.totalSize;

    sts = RunIoctl(DRM_IOCTL_I915_GEM_CREATE, &gem_create);

    errString = "Ioctl I915_GEM_CREATE: size = " + std::to_string(gem_create.size);
    GFX_LOG_FAILURE(sts != 0, errString.c_str());
    if (!GFX_SUCCESS(sts))
        return GFX_FAIL;


    // -- Get FD
    struct drm_prime_handle gem_handle_fd;
    memset(&gem_handle_fd, 0, sizeof(gem_handle_fd));

    gem_handle_fd.fd     = -1;
    gem_handle_fd.handle = gem_create.handle;
    gem_handle_fd.flags  = DRM_CLOEXEC | DRM_RDWR;

    sts = RunIoctl(DRM_IOCTL_PRIME_HANDLE_TO_FD, &gem_handle_fd);

    errString = "Ioctl PRIME_HANDLE_TO_FD: handle = " + std::to_string(gem_handle_fd.handle);
    GFX_LOG_FAILURE(sts != 0, errString.c_str());
    if (!GFX_SUCCESS(sts))
        return GFX_FAIL;

    // -- Insert into vector, referencing with remote handle specified in the YML log
    boData_t boData = {.surf      = surf,
                       .fd        = gem_handle_fd.fd,
                       .handle    = gem_create.handle,
                       .mmap_addr = nullptr};

    m_boList.insert(std::make_pair(remote_handle, boData));

#if (__DEBUG)
    printf("%s: remote_handle = %lu .. prime_fd = %d .. real_handle = %lu\n",
           __FUNCTION__, remote_handle, boData.fd, boData.handle);
#endif

    return GFX_OK;
}

GfxStatus GfxHandler::ClearBo(uint64_t remote_handle, bool erase)
{
    GfxStatus sts = GFX_OK;
    std::string errString;

    auto bo_entry = m_boList.find(remote_handle);
    if (bo_entry == m_boList.end())
    {
        std::cout << "Could not find bo with remote_handle= " << remote_handle << std::endl;
        return GFX_FAIL;
    }

    struct drm_gem_close gem_close;
    memset(&gem_close, 0, sizeof(gem_close));
    gem_close.handle = bo_entry->second.handle;

    sts = RunIoctl(DRM_IOCTL_GEM_CLOSE, &gem_close);

    errString = "Ioctl GEM_CLOSE: handle = " + std::to_string(gem_close.handle);
    GFX_LOG_FAILURE(sts != 0, errString.c_str());
    if (!GFX_SUCCESS(sts))
        return GFX_FAIL;

    //Remove record from map, if enabled
    if (erase)
        m_boList.erase(bo_entry);

    return GFX_OK;
}

boData_t* GfxHandler::GetBo(uint64_t handle)
{
    if (m_boList.size() == 0)
        return nullptr;

    auto bo_entry = m_boList.find(handle);
    if (bo_entry == m_boList.end())
    {
        std::cout << "Could not find bo with handle= " << handle << std::endl;
        return nullptr;
    }

    return &bo_entry->second;
}

GfxStatus GfxHandler::Copy(boData_t* dstData, const void* srcBuf)
{
    if (!srcBuf || !dstData)
    {
        std::cout << "Error: Unexpected Null Ptr: srcBuf = 0x"<< std::hex << srcBuf
                  << " , dstData = 0x" << std::hex << dstData << std::dec << std::endl;
        return GFX_FAIL;
    }

    GfxStatus sts = GFX_OK;
    std::string errString;

    SurfaceParams_t surf = dstData->surf;
    size_t sizeDstPerRow = surf.pitch * surf.pixelSize;
    size_t sizeSrcPerRow = surf.width * surf.pixelSize;

    //Map buffer for CPU write
    sts = MmapBo(dstData, true);

    errString = "MmapBo call: ";
    GFX_LOG_FAILURE(sts != 0, errString.c_str());
    if (!GFX_SUCCESS(sts))
        return GFX_FAIL;

    uint8_t* dst = (uint8_t*) dstData->mmap_addr;
    uint8_t* src = (uint8_t*) srcBuf;
    for (unsigned i = 0; i < surf.height; i++)
    {
        memcpy(dst, src, sizeDstPerRow);
        dst += sizeDstPerRow;
        src += sizeSrcPerRow;
    }

    //Unmap and return
    return UnmapBo(dstData);
}

int GfxHandler::GetMmapType(boData_t* bo_gem)
{
    // Options:
    // - I915_MMAP_OFFSET_WC (Write combine)
    // - I915_MMAP_OFFSET_WC (Write combine)

    //Support only Write Combine now
    return I915_MMAP_OFFSET_WC;
}

GfxStatus GfxHandler::MmapBo(boData_t* bo_gem, bool writeEnable)
{
    int sts = 0;
    std::string errString;

    if (m_device_props.hasMMAPoffset == 0)
    {
        std::cout << "Error: mmap relies on offset mechanism" << std::endl;
        return GFX_FAIL;
    }

    // Run GEM_MMAP_OFFSET Ioctl to set up
    struct drm_i915_gem_mmap_offset mmap_offset_arg;
    memset(&mmap_offset_arg, 0, sizeof(mmap_offset_arg));

    mmap_offset_arg.handle = bo_gem->handle;
    mmap_offset_arg.flags = GetMmapType();

    sts = RunIoctl(DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_offset_arg);

    errString = "Ioctl I915_GEM_MMAP_OFFSET";
    GFX_LOG_FAILURE(sts != 0, errString.c_str());
    if (!GFX_SUCCESS(sts))
        return GFX_FAIL;

    // Issue the mmap system call
    bo_gem->mmap_addr = mmap(NULL,
                             bo_gem->surf.totalSize,
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED,
                             m_device_fd,
                             mmap_offset_arg.offset);

    if (bo_gem->mmap_addr == MAP_FAILED)
    {
        bo_gem->mmap_addr = nullptr;

        errString = "Ioctl I915_GEM_MMAP_OFFSET";
        GFX_LOG_FAILURE(sts != 0, errString.c_str());
        if (!GFX_SUCCESS(sts))
            return GFX_FAIL;
    }

    // Specify domain

    struct drm_i915_gem_set_domain set_domain;
    memset(&set_domain, 0, sizeof(set_domain));

    set_domain.handle = bo_gem->handle;
    set_domain.read_domains = I915_GEM_DOMAIN_CPU;
    if (writeEnable)
        set_domain.write_domain = I915_GEM_DOMAIN_CPU;
    else
        set_domain.write_domain = 0;

    sts = RunIoctl(DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);

    errString = "Ioctl I915_GEM_SET_DOMAIN";
    GFX_LOG_FAILURE(sts != 0, errString.c_str());
    if (!GFX_SUCCESS(sts))
        return GFX_FAIL;

    return GFX_OK;
}


GfxStatus GfxHandler::UnmapBo(boData_t* bo_gem)
{
    int sts;
    std::string errString;

    if (bo_gem == nullptr)
    {
        std::cout << "Error: bo cannot be null to unmap" << std::endl;
        return GFX_FAIL;
    }

    if (bo_gem->mmap_addr == nullptr)
    {
        std::cout << "Warning: mmap_addr is null. Nothing to unmap" << std::endl;
        return GFX_OK;
    }

    struct drm_i915_gem_sw_finish sw_finish;
    memset(&sw_finish, 0, sizeof(sw_finish));

    sw_finish.handle = bo_gem->handle;

    sts = RunIoctl(DRM_IOCTL_I915_GEM_SW_FINISH, &sw_finish);

    errString = "Ioctl I915_GEM_SW_FINISH";
    GFX_LOG_FAILURE(sts != 0, errString.c_str());
    if (!GFX_SUCCESS(sts))
        return GFX_FAIL;

    bo_gem->mmap_addr = nullptr;

    return GFX_OK;
}
