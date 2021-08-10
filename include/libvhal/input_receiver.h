#ifndef __INPUT_RECEIVER_H__
#define __INPUT_RECEIVER_H__

#include <inttypes.h>
#include <string>

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

struct IInputReceiver
{
    virtual ~IInputReceiver(){};
    virtual int getTouchInfo(TouchInfo* info)               = 0;
    virtual int onInputMessage(const std::string& msg)      = 0;
    virtual int onJoystickMessage(const std::string& msg)   = 0;
    virtual int onKeyCode(uint16_t scanCode, uint32_t mask) = 0;
};

} // namespace client
} // namespace vhal
#endif //__INPUT_RECEIVER_H__