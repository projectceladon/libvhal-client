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

#include "virtual_input_receiver.h"
#include "receiver_log.h"
#include "status_prober.h"

namespace vhal {
namespace client {

VirtualInputReceiver::VirtualInputReceiver(struct UnixConnectionInfo uci)
  : mUci(uci)
{
    mStatusProber = std::make_unique<StatusProber>(mUci.status_dir);
    mStatusProber->UpdateStatus("disconnected");
    CreateTouchDevice(uci);
}

VirtualInputReceiver::~VirtualInputReceiver()
{
    if (mFd >= 0) {
        mStatusProber->UpdateStatus("disconnected");
        close(mFd);
    }
}

bool
VirtualInputReceiver::CreateTouchDevice(struct UnixConnectionInfo uci)
{
    if (mFd > -1) {
        AIC_LOG(LIBVHAL_INFO,
                "Virtual touch channel is already open. mFd = %d",
                mFd);
        return true;
    }
    mFd = open(uci.socket_dir.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (mFd >= 0) {
        AIC_LOG(LIBVHAL_INFO, "Open %s successfully.", uci.socket_dir.c_str());
        mStatusProber->UpdateStatus("connected");
    } else {
        AIC_LOG(LIBVHAL_ERROR, "Failed to open pipe for read error");
        return false;
    }
    return true;
}

bool
VirtualInputReceiver::SendEvent(uint16_t type, uint16_t code, int32_t value)
{
    struct input_event ev;
    timespec           ts;

    memset(&ev, 0, sizeof(struct input_event));
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ev.time.tv_sec  = ts.tv_sec;
    ev.time.tv_usec = ts.tv_nsec / 1000;
    ev.type         = type;
    ev.code         = code;
    ev.value        = value;

    if (mDebug)
        AIC_LOG(mDebug, "type: %d code: %d value: %d", type, code, value);

    if (write(mFd, &ev, sizeof(struct input_event)) < 0) {
        AIC_LOG(LIBVHAL_ERROR, "Failed to send event");
        mStatusProber->UpdateStatus("disconnected");
        close(mFd);
        mFd = -1;
        return CreateTouchDevice(mUci);
    }
    return true;
}

bool
VirtualInputReceiver::SendDown(int32_t slot,
                               int32_t x,
                               int32_t y,
                               int32_t pressure)
{
    if ((uint32_t)slot >= kMaxSlot) {
        return false;
    }
    if (mContacts[slot].enabled || mEnabledSlots >= kMaxSlot) {
        SendReset();
    }
    mContacts[slot].enabled    = true;
    mContacts[slot].trackingId = mTrackingId++;
    mEnabledSlots++;

    SendEvent(EV_ABS, ABS_MT_SLOT, slot);
    SendEvent(EV_ABS, ABS_MT_TRACKING_ID, mContacts[slot].trackingId);

    if (mEnabledSlots == 1) {
        SendEvent(EV_KEY, BTN_TOUCH, 1);
    }
    SendEvent(EV_ABS, ABS_MT_TOUCH_MAJOR, 0x00000004);
    SendEvent(EV_ABS, ABS_MT_WIDTH_MAJOR, 0x00000006);
    SendEvent(EV_ABS, ABS_MT_PRESSURE, pressure);
    SendEvent(EV_ABS, ABS_MT_POSITION_X, x);
    SendEvent(EV_ABS, ABS_MT_POSITION_Y, y);

    return true;
}

bool
VirtualInputReceiver::SendUp(int32_t slot)
{
    if (mEnabledSlots == 0 || (uint32_t)slot >= kMaxSlot ||
        !mContacts[slot].enabled) {
        return false;
    }

    mContacts[slot].enabled = false;
    mEnabledSlots--;

    SendEvent(EV_ABS, ABS_MT_SLOT, slot);
    SendEvent(EV_ABS, ABS_MT_TRACKING_ID, -1);
    if (mEnabledSlots == 0) {
        SendEvent(EV_KEY, BTN_TOUCH, 0);
    }
    return true;
}

bool
VirtualInputReceiver::SendMove(int32_t slot,
                               int32_t x,
                               int32_t y,
                               int32_t pressure)
{
    if ((uint32_t)slot >= kMaxSlot || !mContacts[slot].enabled) {
        return false;
    }

    SendEvent(EV_ABS, ABS_MT_SLOT, slot);
    SendEvent(EV_ABS, ABS_MT_TOUCH_MAJOR, 0x00000004);
    SendEvent(EV_ABS, ABS_MT_WIDTH_MAJOR, 0x00000006);
    SendEvent(EV_ABS, ABS_MT_PRESSURE, pressure);
    SendEvent(EV_ABS, ABS_MT_POSITION_X, x);
    SendEvent(EV_ABS, ABS_MT_POSITION_Y, y);

    return true;
}

bool
VirtualInputReceiver::SendCommit()
{
    SendEvent(EV_SYN, SYN_REPORT, 0);
    return true;
}

bool
VirtualInputReceiver::SendReset()
{
    bool report = false;
    for (uint32_t slot = 0; slot < kMaxSlot; slot++) {
        if (mContacts[slot].enabled) {
            mContacts[slot].enabled = false;
            report                  = true;
        }
    }
    if (report) {
        SendEvent(EV_SYN, SYN_REPORT, 0);
    }
    return true;
}

void
VirtualInputReceiver::SendWait(uint32_t ms)
{
    usleep(ms * 1000);
}

bool
VirtualInputReceiver::ProcessOneCommand(const std::string& cmd)
{
    char    type     = 0;
    int32_t slot     = -1;
    int32_t x        = -1;
    int32_t y        = -1;
    int32_t pressure = -1;
    int32_t ms       = 0;

    if (mDebug)
        AIC_LOG(mDebug, "cmd: %s", cmd.c_str());

    switch (cmd[0]) {
        case 'c': // commit
            SendCommit();
            break;
        case 'r': // reset
            SendReset();
            break;
        case 'd': // down
            sscanf(cmd.c_str(),
                   "%c %d %d %d %d",
                   &type,
                   &slot,
                   &x,
                   &y,
                   &pressure);
            if (slot < 0 || (uint32_t)slot > kMaxSlot || x < 0 ||
                (uint32_t)x > kMaxPositionX || y < 0 ||
                (uint32_t)y > kMaxPositionY || pressure < 0 ||
                (uint32_t)pressure > kMaxPressure) {
                printf(
                  "%s: Error: Parameter error. slot=%d x=%d y=%d pressure=%d\n",
                  __func__,
                  slot,
                  x,
                  y,
                  pressure);
                return false;
            }
            SendDown(slot, x, y, pressure);
            break;
        case 'u': // up
            sscanf(cmd.c_str(), "%c %d", &type, &slot);
            if (slot < 0 || (uint32_t)slot > kMaxSlot) {
                printf("%s: Error: Parameter error. slot=%d\n", __func__, slot);
                return false;
            }
            SendUp(slot);
            break;
        case 'm': // move
            sscanf(cmd.c_str(),
                   "%c %d %d %d %d",
                   &type,
                   &slot,
                   &x,
                   &y,
                   &pressure);
            if (slot < 0 || (uint32_t)slot > kMaxSlot || x < 0 ||
                (uint32_t)x > kMaxPositionX || y < 0 ||
                (uint32_t)y > kMaxPositionY || pressure < 0 ||
                (uint32_t)pressure > kMaxPressure) {
                printf(
                  "%s: Error: Parameter error. slot=%d x=%d y=%d pressure=%d\n",
                  __func__,
                  slot,
                  x,
                  y,
                  pressure);
                return false;
            }
            SendMove(slot, x, y, pressure);
            break;
        case 'w': // wait ms
            sscanf(cmd.c_str(), "%c %d", &type, &ms);
            if (ms <= 0) {
                printf("%s: Error: Parameter error. ms=%d\n", __func__, ms);
                return false;
            }
            SendWait(ms);
            break;
        default:
            break;
    }
    return true;
}

bool
VirtualInputReceiver::getTouchInfo(TouchInfo* info)
{
    if (!info) {
        return false;
    }

    info->max_contacts = kMaxSlot;
    info->max_pressure = kMaxPressure;
    info->max_x        = kMaxPositionX;
    info->max_y        = kMaxPositionY;
    info->pid          = getpid();
    info->version      = 1;

    return true;
}

IOResult
VirtualInputReceiver::onInputMessage(const std::string& msg)
{
    size_t      begin     = 0;
    size_t      end       = 0;
    std::string error_msg = "";

    while (true) {
        end = msg.find("\n", begin);
        if (end == std::string::npos)
            break;

        std::string cmd = msg.substr(begin, end);
        ProcessOneCommand(cmd);
        begin = end + 1;
        if (msg[begin] == '\r')
            begin++;
    }
    return { 0, error_msg };
}

IOResult
VirtualInputReceiver::onJoystickMessage(const std::string& msg)
{
    size_t      begin     = 0;
    size_t      end       = 0;
    std::string error_msg = "";

    while (true) {
        end = msg.find("\n", begin);
        if (end == std::string::npos)
            break;

        std::string cmd = msg.substr(begin, end);
        ProcessOneJoystickCommand(cmd);
        begin = end + 1;
        if (msg[begin] == '\r')
            begin++;
    }
    return { 0, error_msg };
}

bool
VirtualInputReceiver::ProcessOneJoystickCommand(const std::string& cmd)
{
    if (mDebug)
        printf("%s\n", __func__);

    char     type = 0;
    uint16_t code;
    int32_t  value;

    switch (cmd[0]) {
        case 'c': // Commit
            if (mDebug)
                printf("SendCommit\n");
            SendCommit();
            break;

        case 'k': // EV_KEY 1
            sscanf(cmd.c_str(),
                   "%c %" SCNu16 " %" SCNd32,
                   &type,
                   &code,
                   &value);
            if (mDebug)
                printf("code = %d, value = %d\n", code, value);
            SendEvent(EV_KEY, code, value);
            break;

        case 'm': // EV_MSC 4
            sscanf(cmd.c_str(),
                   "%c %" SCNu16 " %" SCNd32,
                   &type,
                   &code,
                   &value);
            if (mDebug)
                printf("code = %d, value = %d\n", code, value);
            SendEvent(EV_MSC, code, value);
            break;

        case 'a': // EV_ABS 3
            sscanf(cmd.c_str(),
                   "%c %" SCNu16 " %" SCNd32,
                   &type,
                   &code,
                   &value);
            if (mDebug)
                printf("code = %d, value = %d\n", code, value);
            SendEvent(EV_ABS, code, value);
            break;
        case 'i': // insert joystick
            if (mDebug)
                printf("%s:%d enable joystick down\n", __func__, __LINE__);
            SendEvent(EV_KEY, 631, 1);
            SendCommit();

            usleep(2000);

            if (mDebug)
                printf("%s:%d enable joystick up\n", __func__, __LINE__);
            SendEvent(EV_KEY, 631, 0);
            SendCommit();
            break;
        case 'p': // pull out joystick
            if (mDebug)
                printf("%s:%d disable joystick down\n", __func__, __LINE__);
            SendEvent(EV_KEY, 632, 1);
            SendCommit();

            usleep(2000);

            if (mDebug)
                printf("%s:%d disable joystick up\n", __func__, __LINE__);
            SendEvent(EV_KEY, 632, 0);
            SendCommit();

            break;
        default:
            break;
    }
    return true;
}

IOResult
VirtualInputReceiver::onKeyCode(uint16_t scanCode, uint32_t mask)
{
    std::string error_msg = "";

    if (mDebug)
        printf("%s\n", __func__);

    if (mask & KEY_STATE_MASK::Shift) {
        SendEvent(EV_KEY, KEY_LEFTSHIFT, 1);
        SendCommit();
    }
    if (mask & KEY_STATE_MASK::Control) {
        SendEvent(EV_KEY, KEY_LEFTCTRL, 1);
        SendCommit();
    }
    if (mask & KEY_STATE_MASK::Mod1) {
        SendEvent(EV_KEY, KEY_LEFTALT, 1);
        SendCommit();
    }

    SendEvent(EV_KEY, scanCode, 1);
    SendCommit();
    SendEvent(EV_KEY, scanCode, 0);
    SendCommit();

    if (mask & KEY_STATE_MASK::Shift) {
        SendEvent(EV_KEY, KEY_LEFTSHIFT, 0);
        SendCommit();
    }
    if (mask & KEY_STATE_MASK::Control) {
        SendEvent(EV_KEY, KEY_LEFTCTRL, 0);
        SendCommit();
    }
    if (mask & KEY_STATE_MASK::Mod1) {
        SendEvent(EV_KEY, KEY_LEFTALT, 0);
        SendCommit();
    }
    return { 0, error_msg };
}

} // namespace client
} // namespace vhal
