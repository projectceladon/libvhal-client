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

#ifndef _VIRTUAL_INPUT_RECEIVER_H
#define _VIRTUAL_INPUT_RECEIVER_H

#include "input_receiver.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/input.h>
#include <memory>
#include <string.h>
#include <unistd.h>

namespace vhal {
namespace client {

class VirtualInputReceiver : public IInputReceiver
{
public:
    /**
     * @brief Constructor.
     *
     * @param uci Unix connection information.
     */
    VirtualInputReceiver(struct UnixConnectionInfo uci);
    virtual ~VirtualInputReceiver();

    bool     getTouchInfo(TouchInfo* info) override;
    IOResult onInputMessage(const std::string& msg) override;
    IOResult onJoystickMessage(const std::string& msg) override;
    IOResult onKeyCode(uint16_t scanCode, uint32_t mask) override;

protected:
    // Process one mini-touch command
    bool ProcessOneCommand(const std::string& cmd);
    bool ProcessOneJoystickCommand(const std::string& cmd);

    uint32_t GetMaxPositionX() { return kMaxPositionX - 1; }
    uint32_t GetMaxPositionY() { return kMaxPositionY - 1; }

    bool CreateTouchDevice(struct UnixConnectionInfo uci);
    bool SendEvent(uint16_t type, uint16_t code, int32_t value);
    bool SendDown(int32_t slot, int32_t x, int32_t y, int32_t pressure);
    bool SendUp(int32_t slot);
    bool SendMove(int32_t slot, int32_t x, int32_t y, int32_t pressure);
    bool SendCommit();
    bool SendReset();
    void SendWait(uint32_t ms);

private:
    struct UnixConnectionInfo mUci;
    static const uint32_t     kMaxSlot       = 10;
    static const uint32_t     kMaxMajor      = 15;
    static const uint32_t     kMaxPositionX  = 32767;
    static const uint32_t     kMaxPositionY  = 32767;
    static const uint32_t     kMaxPressure   = 255;
    static const uint32_t     kMaxTrackingId = 65535;

    struct Contact
    {
        bool    enabled    = false;
        int32_t trackingId = 0;
        int32_t x          = 0;
        int32_t y          = 0;
        int32_t pressure   = 0;
    };

    int      mFd = -1;
    Contact  mContacts[kMaxSlot];
    int32_t  mTrackingId   = 0;
    uint32_t mEnabledSlots = 0;
    int      mDebug        = 0;
    class StatusProber;
    std::unique_ptr<StatusProber> mStatusProber;
};

} // namespace client
} // namespace vhal
#endif //_VIRTUAL_INPUT_RECEIVER_H
