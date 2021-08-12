
#include "virtual_gps_receiver.h"
#include "receiver_log.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace vhal {
namespace client {

const char*       VirtualGpsReceiver::kGpsSock;
const std::string VirtualGpsReceiver::gpsQuitMsg = "{ \"key\" : \"gps-quit\" }";
const std::string VirtualGpsReceiver::gpsStartMsg =
  "{ \"key\" : \"gps-start\" }";
const std::string VirtualGpsReceiver::gpsStopMsg = "{ \"key\" : \"gps-stop\" }";
const unsigned int VirtualGpsReceiver::mDebug    = 0;

VirtualGpsReceiver::VirtualGpsReceiver(struct TcpConnectionInfo tci) : mTci(tci)
{
    mWorkThread = std::unique_ptr<std::thread>(
      new std::thread(&VirtualGpsReceiver::workThreadProc, this));
}

VirtualGpsReceiver::~VirtualGpsReceiver()
{
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mStop = true;
    }
    if (mSockGps >= 0) {
        Disconnect();
    }
    mWorkThread->join();
}

ConnectionResult
VirtualGpsReceiver::Connect()
{
    std::string error_msg = "";
    AIC_LOG(LIBVHAL_DEBUG, "GPS connect VHAL server...");
    if (mSockGps >= 0) {
        Disconnect();
    }

    if (mTci.port != 0) {
        mPort = mTci.port;
    }

    AIC_LOG(LIBVHAL_DEBUG,
            "GPS server_ip = %s, port = %d",
            mTci.ip_addr.c_str(),
            mPort);
    mSockGps = socket(AF_INET, SOCK_STREAM, 0);
    if (mSockGps < 0) {
        AIC_LOG(LIBVHAL_ERROR, "Can't create GPS socket");
        error_msg = std::strerror(errno);
        return { false, error_msg };
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(mPort);
    inet_pton(AF_INET, mTci.ip_addr.c_str(), &addr.sin_addr);

    int res =
      connect(mSockGps, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    if (res < 0) {
        AIC_LOG(LIBVHAL_ERROR,
                "Can't connect to GPS remote, %s",
                strerror(errno));
        error_msg = std::strerror(errno);
        close(mSockGps);
        mSockGps = -1;
        return { false, error_msg };
    }
    return { true, error_msg };
}

ConnectionResult
VirtualGpsReceiver::Disconnect()
{
    std::string error_msg = "";
    if (mSockGps < 0) {
        AIC_LOG(LIBVHAL_DEBUG,
                "GPS socket is already disconnect. mSockGps = %d mStop = %s",
                mSockGps,
                mStop ? "true" : "false");
        return { true, error_msg };
    }

    AIC_LOG(LIBVHAL_DEBUG,
            "Shutdown and close GPS socket. mSockGps = %d mStop = %s",
            mSockGps,
            mStop ? "true" : "false");
    shutdown(mSockGps, SHUT_RDWR);
    close(mSockGps);
    mSockGps = -1;
    return { true, error_msg };
}

bool
VirtualGpsReceiver::Connected()
{
    return mSockGps > 0;
}

IOResult
VirtualGpsReceiver::Write(const uint8_t* buf, size_t len)
{
    size_t       offset    = 0;
    size_t       left      = len;
    ssize_t      size      = 0;
    unsigned int attempt   = 100;
    std::string  error_msg = "";

    while (left > 0) {
        do {
            size = write(mSockGps, buf + offset, left);
        } while (size < 0 && errno == EINTR && (--attempt > 0));
        if (size == 0) {
            AIC_LOG(LIBVHAL_ERROR, "GPS socket server is closed.");
            error_msg = std::strerror(errno);
            Disconnect();
            return { size, error_msg };
        } else if (size < 0) {
            AIC_LOG(LIBVHAL_ERROR,
                    "GPS socket write error: %s",
                    strerror(errno));
            error_msg = std::strerror(errno);
            return { size, error_msg };
        } else {
            left -= size;
            offset += size;
        }
    }
    return { offset > 0 ? offset : size, error_msg };
}

IOResult
VirtualGpsReceiver::Read(uint8_t* buf, size_t len)
{
    size_t       offset    = 0;
    size_t       left      = len;
    ssize_t      size      = 0;
    unsigned int attempt   = 100;
    std::string  error_msg = "";

    while (left > 0) {
        do {
            size = read(mSockGps, buf + offset, left);
        } while (size < 0 && errno == EINTR && (--attempt > 0));
        if (size == 0) {
            AIC_LOG(LIBVHAL_ERROR, "GPS socket server is closed.");
            error_msg = std::strerror(errno);
            Disconnect();
            return { size, error_msg };
        } else if (size < 0) {
            AIC_LOG(LIBVHAL_ERROR,
                    "GPS socket write error: %s",
                    strerror(errno));
            error_msg = std::strerror(errno);
            return { size, error_msg };
        } else if (size > 0) {
            offset += size;
            left -= size;
        }
    }

    return { offset > 0 ? offset : size, error_msg };
}

void
VirtualGpsReceiver::workThreadProc()
{
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            if (mStop) {
                AIC_LOG(LIBVHAL_WARNING,
                        "mStop = %s. Break from while.",
                        mStop ? "true" : "false");
                break;
            }
        }

        if (!Connected()) {
            AIC_LOG(LIBVHAL_WARNING,
                    "Not connected to GPS server. Need to connect again.");
            IOResult ior = Connect();
            if (!std::get<0>(ior)) {
                sleep(3);
                AIC_LOG(LIBVHAL_ERROR, "Try to connect GPS server again");
                continue;
            } else {
                AIC_LOG(LIBVHAL_DEBUG, "Connected to GPS server.");
            }
        }

        char     cmd = 0xFF;
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&cmd);
        IOResult ior = Read(ptr, sizeof(cmd));
        int      res = std::get<0>(ior);
        if (res < 0) {
            AIC_LOG(LIBVHAL_WARNING,
                    "%s",
                    mStop ? "mStop = true. Stop"
                          : "Read error, try to connect and read again");
        } else if (res == sizeof(cmd)) {
            uint32_t command;
            switch (cmd) {
                case GPS_CMD_QUIT:
                    command = Command::kGpsQuit;
                    AIC_LOG(LIBVHAL_DEBUG, "GPS_CMD_QUIT");
                    if (nullptr != mGpsCmdHandler)
                        mGpsCmdHandler(static_cast<uint32_t>(command));
                    else
                        AIC_LOG(LIBVHAL_WARNING, "mGpsCmdHandler is nullptr");
                    break;
                case GPS_CMD_START:
                    command = Command::kGpsStart;
                    AIC_LOG(LIBVHAL_DEBUG, "GPS_CMD_START");
                    if (nullptr != mGpsCmdHandler)
                        mGpsCmdHandler(static_cast<uint32_t>(command));
                    else
                        AIC_LOG(LIBVHAL_WARNING, "mGpsCmdHandler is nullptr");
                    break;
                case GPS_CMD_STOP:
                    command = Command::kGpsStop;
                    AIC_LOG(LIBVHAL_DEBUG, "GPS_CMD_STOP");
                    if (nullptr != mGpsCmdHandler)
                        mGpsCmdHandler(static_cast<uint32_t>(command));
                    else
                        AIC_LOG(LIBVHAL_WARNING, "mGpsCmdHandler is nullptr");
                    break;

                default:
                    AIC_LOG(LIBVHAL_ERROR, "GPS unkown command. cmd = %c", cmd);
                    break;
            }
        } else {
            AIC_LOG(LIBVHAL_ERROR,
                    "GPS work thread, should not be here, res %d",
                    res);
            break;
        }
    }
    AIC_LOG(LIBVHAL_DEBUG, "GPS work thread exit");
}

bool
VirtualGpsReceiver::RegisterCallback(GpsCommandHandler gch)
{
    mGpsCmdHandler = move(gch);
    return true;
}

} // namespace client
} // namespace vhal