/**
 *
 * Copyright (c) 2021-2022 Intel Corporation
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

#ifndef __INPUT_RECEIVER_H__
#define __INPUT_RECEIVER_H__

#include "libvhal_common.h"
#include <inttypes.h>
#include <string>
#include <tuple>

namespace vhal {
namespace client {
struct TouchInfo
{
    uint32_t version;
    int      max_contacts;
    int      max_x;
    int      max_y;
    int      max_pressure;
    int      pid;
};

enum KEY_STATE_MASK
{ //  // same as X.h
    Shift   = (1 << 0),
    Lock    = (1 << 1),
    Control = (1 << 2),
    Mod1    = (1 << 3),
    Mod2    = (1 << 4),
    Mod3    = (1 << 5),
    Mod4    = (1 << 6),
    Mod5    = (1 << 7),
};

/**
 * @brief Class that acts as a pipe between input client and virtual touch and
 * virtual joystick.
 *
 */
struct IInputReceiver
{
    virtual ~IInputReceiver(){};

    /**
     * @brief Get touch information.
     *
     * @param info Fill with the number of slot which from 0 to 9, maximum x
     * coordinate, maximum y coordinate, maximum pressure, the process id.
     *
     * @return true Success.
     * @return false Fail.
     */
    virtual bool getTouchInfo(TouchInfo* info) = 0;

    /**
     * @brief Inject single or multiple touch event.
     *
     * @param msg Single or multiple commands. Commands delimiter is '\n'.
    Commands are:
     *  - "d <contact> <x> <y> <pressure>" Down command. For example: "d 0 16384
    16384 200", pressure value 200 is pressed at a point (16384, 16384) with a
    contact.
     *  - "m <contact> <x> <y> <pressure>" Move command. For example: "m 0 16384
    14022 200", the pressure value 200 slides at (16384, 14022).
     *  - "u <contact>" Up command. For example: "u 0", gesture raised.
     *  - "c" Commit command. For example: "c", it is commit.
     *  - "r" Reset command. For example: "r", it is reset.
     *  - "w <ms>" Wait command. For example: "w 10", Wait for 10 ms.
     * For more detail, please reference
    https://github.com/intel-innersource/os.android.bsp.libvhal-client#usage.
     *
     * @return {0, ""} All commands were injected successfully.
     * @return {-1, "error msg"} Command failed
     */
    virtual IOResult onInputMessage(const std::string& msg) = 0;

    /**
     * @brief Inject single or multiple joystick event.
     *
     * @param msg Single or multiple commands. Commands delimiter is '\n'.
     * Commands are:
     *  - "c" Commit command. For example: "c", it is commit.
     *  - "k <code> <value>"  Key command:  For example: "k 288 1", BUTTON_1 is
     * pressed.
     *  - "m <code> <value>" EV_MSC key command. For example: "m 4 81", MSC_SCAN
     * with code 81.
     *  - "a <code> <value>" EV_ABS key command. For example: "a 2 -127", Right
     * Stick West move.
     *  - "i" Joystick insert to AIC command. For example: "i", current joystick
     * insert to AIC.
     *  - "p" Joystick pull out command. For example: "p\n", current joystick
     * pull out from AIC.
     *
     * @return {0, ""} All commands were injected successfully.
     * @return {-1, "error msg"} Command failed
     */
    virtual IOResult onJoystickMessage(const std::string& msg) = 0;

    /**
     * @brief Inject keyboard event.
     *
     * @param scanCode Produced by key presses or used in the protocol with the
     * computer. Please reference /usr/include/linux/input-event-codes.h. In
     * theory, all codes in the header file are supported. This function is
     * mainly used to input keyboard keys.
     * @param mask It is one of KEY_STATE_MASK. It is required for the key
     * combination. Just reference it #KEY_STATE_MASK.
     *
     * @return {0, ""} All commands were injected successfully.
     * @return {-1, "error msg"} Command failed
     */
    virtual IOResult onKeyCode(uint16_t scanCode, uint32_t mask) = 0;
};

} // namespace client
} // namespace vhal
#endif //__INPUT_RECEIVER_H__