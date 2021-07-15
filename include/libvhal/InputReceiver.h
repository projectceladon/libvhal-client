#ifndef __INPUT_RECEIVER_H__
#define __INPUT_RECEIVER_H__

#include <inttypes.h>
#include <string>

struct TouchInfo
{
    int version;
    int max_contacts;
    int max_x;
    int max_y;
    int max_pressure;
    int pid;
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

struct IInputReceiver
{
    virtual ~IInputReceiver(){};
    virtual int  getTouchInfo(TouchInfo* info)               = 0;
    virtual int  onInputMessage(const std::string& msg)      = 0;
    virtual int  onKeyCode(uint16_t scanCode, uint32_t mask) = 0;
    virtual int  onKeyChar(char ch)                          = 0;
    virtual int  onText(const char* msg)                     = 0;
    virtual int  onJoystickMessage(const std::string& msg)   = 0;
    virtual bool joystickEnable()                            = 0;
    virtual bool joystickDisable()                           = 0;
    virtual bool getJoystickStatus()                         = 0;
};

#endif //__INPUT_RECEIVER_H__