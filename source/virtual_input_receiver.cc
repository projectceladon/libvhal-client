
#include "virtual_input_receiver.h"
#include "receiver_log.h"

VirtualInputReceiver::VirtualInputReceiver(int id, int inputId)
{
    CreateTouchDevice(id, inputId);
}

VirtualInputReceiver::~VirtualInputReceiver()
{
    if (mFd >= 0) {
        close(mFd);
    }
}

bool
VirtualInputReceiver::CreateTouchDevice(int id, int inputId)
{
    printf("%s:%d Create virtual touch channel\n", __func__, __LINE__);
    char kDevNameId[64] = {
        '\0',
    };
    if (getenv("K8S_ENV") != NULL && strcmp(getenv("K8S_ENV"), "true") == 0) {
        sprintf(kDevNameId, "%s-%d", kDevName, inputId);
    } else {
        sprintf(kDevNameId, "%s%d-%d", kDevName, id, inputId);
    }
    mFd = open(kDevNameId, O_RDWR | O_NONBLOCK, 0);
    if (mFd < 0) {
        perror("Failed to open pipe for read error");
        return false;
    } else {
        printf("%s:%d Open %s successfully.\n", __func__, __LINE__, kDevNameId);
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
        perror("Failed to send event\n");
        return false;
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
VirtualInputReceiver::SendUp(int32_t slot, int32_t x, int32_t y)
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
    int32_t slot     = 0;
    int32_t x        = 0;
    int32_t y        = 0;
    int32_t pressure = 0;
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
            SendDown(slot, x, y, pressure);
            break;
        case 'u': // up
            sscanf(cmd.c_str(), "%c %d %d %d", &type, &slot, &x, &y);
            SendUp(slot, x, y);
            break;
        case 'm': // move
            sscanf(cmd.c_str(),
                   "%c %d %d %d %d",
                   &type,
                   &slot,
                   &x,
                   &y,
                   &pressure);
            SendMove(slot, x, y, pressure);
            break;
        case 'w': // wait ms
            sscanf(cmd.c_str(), "%c %d", &type, &ms);
            SendWait(ms);
            break;
        default:
            break;
    }
    return true;
}

int
VirtualInputReceiver::getTouchInfo(TouchInfo* info)
{
    if (!info) {
        return -EINVAL;
    }

    info->max_contacts = kMaxSlot;
    info->max_pressure = kMaxPressure;
    info->max_x        = kMaxPositionX;
    info->max_y        = kMaxPositionY;
    info->pid          = getpid();
    info->version      = 1;

    return 0;
}
int
VirtualInputReceiver::onInputMessage(const std::string& msg)
{
    size_t begin = 0;
    size_t end   = 0;

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
    return 0;
}

int
VirtualInputReceiver::onJoystickMessage(const std::string& msg)
{
    size_t begin = 0;
    size_t end   = 0;

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
    return 0;
}

bool
VirtualInputReceiver::joystickEnable()
{
    if (mDebug)
        printf("%s:%d enable joystick down\n", __func__, __LINE__);
    onJoystickMessage("k 631 1\n");
    onJoystickMessage("c\n");
    usleep(2000);
    if (mDebug)
        printf("%s:%d enable joystick up\n", __func__, __LINE__);
    onJoystickMessage("k 631 0\n");
    onJoystickMessage("c\n");
    mJoystickStatus = true;
    return true;
}

bool
VirtualInputReceiver::joystickDisable()
{
    if (mDebug)
        printf("%s:%d disable joystick down\n", __func__, __LINE__);
    onJoystickMessage("k 632 1\n");
    onJoystickMessage("c\n");
    usleep(2000);
    if (mDebug)
        printf("%s:%d disable joystick up\n", __func__, __LINE__);
    onJoystickMessage("k 632 0\n");
    onJoystickMessage("c\n");
    mJoystickStatus = false;
    return true;
}

bool
VirtualInputReceiver::getJoystickStatus()
{
    return mJoystickStatus;
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
        default:
            break;
    }
    return true;
}

int
VirtualInputReceiver::onKeyCode(uint16_t scanCode, uint32_t mask)
{
    // printf("%s\n", __func__);

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
    return 0;
}

int
VirtualInputReceiver::onKeyChar(char ch)
{
    // printf("%s\n", __func__);

    return 0;
}

int
VirtualInputReceiver::onText(const char* msg)
{
    // printf("%s\n", __func__);

    return 0;
}
