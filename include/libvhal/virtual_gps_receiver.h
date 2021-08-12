#ifndef __VIRTUAL_GPS_RECEIVER_H__
#define __VIRTUAL_GPS_RECEIVER_H__

#include "istream_socket_client.h"
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace vhal {
namespace client {

using CommandHandler = std::function<void(uint32_t cmd)>;
enum
{
    CMD_QUIT  = 0,
    CMD_START = 1,
    CMD_STOP  = 2
};

class VirtualGpsReceiver
{
public:
    VirtualGpsReceiver(const std::string& ip, int port, CommandHandler ch);
    ~VirtualGpsReceiver();

    ConnectionResult Connect();
    ConnectionResult Disconnect();
    bool             Connected();
    IOResult         Write(const char* buf, size_t len);

private:
    IOResult Read(uint8_t* buf, size_t len);
    void     workThreadProc();

public:
    enum Command
    {
        kGpsQuit  = 20,
        kGpsStart = 21,
        kGpsStop  = 22
    };
    static const std::string  gpsQuitMsg;
    static const std::string  gpsStartMsg;
    static const std::string  gpsStopMsg;
    static const unsigned int mDebug;

private:
    std::string                  mIp;
    uint32_t                     mPort;
    CommandHandler               mCmdHandler;
    int                          mSockGps = -1;
    static const char*           kGpsSock;
    volatile Command             mCommand;
    std::unique_ptr<std::thread> mWorkThread;
    std::mutex                   mMutex;
    bool                         mStop = false;
};

} // namespace client
} // namespace vhal
#endif // __VIRTUAL_GPS_RECEIVER_H__
