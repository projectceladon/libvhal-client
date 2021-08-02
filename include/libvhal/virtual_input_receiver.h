#ifndef _VIRTUAL_INPUT_RECEIVER_H
#define _VIRTUAL_INPUT_RECEIVER_H

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/input.h>

#include "input_receiver.h"

class VirtualInputReceiver : public IInputReceiver
{
public:
    VirtualInputReceiver(int id, int inputId);
    virtual ~VirtualInputReceiver();

    // IInputReceiver
    int  getTouchInfo(TouchInfo* info) override;
    int  onInputMessage(const std::string& msg) override;
    int  onJoystickMessage(const std::string& msg) override;
    bool joystickEnable() override;
    bool joystickDisable() override;
    bool getJoystickStatus() override;
    int  onKeyCode(uint16_t scanCode, uint32_t mask) override;
    int  onKeyChar(char ch) override;
    int  onText(const char* msg) override;

protected:
    // Process one mini-touch command
    bool ProcessOneCommand(const std::string& cmd);
    bool ProcessOneJoystickCommand(const std::string& cmd);

    uint32_t GetMaxPositionX() { return kMaxPositionX - 1; }
    uint32_t GetMaxPositionY() { return kMaxPositionY - 1; }

    bool CreateTouchDevice(int id, int inputId);
    bool SendEvent(uint16_t type, uint16_t code, int32_t value);
    bool SendDown(int32_t slot, int32_t x, int32_t y, int32_t pressure);
    bool SendUp(int32_t slot, int32_t x, int32_t y);
    bool SendMove(int32_t slot, int32_t x, int32_t y, int32_t pressure);
    bool SendCommit();
    bool SendReset();
    void SendWait(uint32_t ms);

private:
    const char* kDevName =
      (getenv("K8S_ENV") != NULL && strcmp(getenv("K8S_ENV"), "true") == 0)
        ? "/conn/input-pipe"
        : "./workdir/ipc/input-pipe";
    static const uint32_t kMaxSlot       = 9;
    static const uint32_t kMaxMajor      = 15;
    static const uint32_t kMaxPositionX  = 1920;
    static const uint32_t kMaxPositionY  = 1080;
    static const uint32_t kMaxPressure   = 255;
    static const uint32_t kMaxTrackingId = 65535;

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
    int32_t  mTrackingId     = 0;
    uint32_t mEnabledSlots   = 0;
    int      mDebug          = 0;
    bool     mJoystickStatus = false;
};

#endif //_VIRTUAL_INPUT_RECEIVER_H