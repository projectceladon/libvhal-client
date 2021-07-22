
#include "VirtualGpsReceiver.h"
#include "ReceiverLog.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

const char*       VirtualGpsReceiver::kGpsSock;
const std::string VirtualGpsReceiver::gpsQuitMsg = "{ \"key\" : \"gps-quit\" }";
const std::string VirtualGpsReceiver::gpsStartMsg =
  "{ \"key\" : \"gps-start\" }";
const std::string VirtualGpsReceiver::gpsStopMsg = "{ \"key\" : \"gps-stop\" }";
const unsigned int VirtualGpsReceiver::mDebug    = 0;

VirtualGpsReceiver::VirtualGpsReceiver(const std::string& ip,
                                       int                port,
                                       CommandHandler     ch)
  : mIp(ip), mPort(port), mCmdHandler(ch)
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

bool
VirtualGpsReceiver::Connect()
{
    AIC_LOG(DEBUG, "GPS connect VHAL server...");
    if (mSockGps >= 0) {
        Disconnect();
    }

    char kSockWithId[30];
    sprintf(kSockWithId, "%s", mIp.c_str());
    AIC_LOG(DEBUG, "GPS server_ip = %s, port = %d", kSockWithId, mPort);
    mSockGps = socket(AF_INET, SOCK_STREAM, 0);
    if (mSockGps < 0) {
        AIC_LOG(ERROR, "Can't create GPS socket");
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(mPort);
    inet_pton(AF_INET, kSockWithId, &addr.sin_addr);

    int res =
      connect(mSockGps, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    if (res < 0) {
        AIC_LOG(ERROR, "Can't connect to GPS remote, %s", strerror(errno));
        close(mSockGps);
        mSockGps = -1;
        return false;
    }
    return true;
}

bool
VirtualGpsReceiver::Disconnect()
{
    if (mSockGps < 0) {
        AIC_LOG(DEBUG,
                "GPS socket is already disconnect. mSockGps = %d mStop = %s",
                mSockGps,
                mStop ? "true" : "false");
        return true;
    }

    AIC_LOG(DEBUG,
            "Shutdown and close GPS socket. mSockGps = %d mStop = %s",
            mSockGps,
            mStop ? "true" : "false");
    shutdown(mSockGps, SHUT_RDWR);
    close(mSockGps);
    mSockGps = -1;
    return true;
}

bool
VirtualGpsReceiver::Connected()
{
    return mSockGps > 0;
}

ssize_t
VirtualGpsReceiver::Write(const char* buf, size_t len)
{
    size_t       offset  = 0;
    size_t       left    = len;
    ssize_t      size    = 0;
    unsigned int attempt = 100;

    while (left > 0) {
        do {
            size = write(mSockGps, buf + offset, left);
        } while (size < 0 && errno == EINTR && (--attempt > 0));
        if (size == 0) {
            AIC_LOG(ERROR, "GPS socket server is closed.");
            Disconnect();
            return -1;
        } else if (size < 0) {
            AIC_LOG(ERROR, "GPS socket write error: %s", strerror(errno));
            return size;
        } else {
            left -= size;
            offset += size;
        }
    }
    return offset > 0 ? offset : size;
}

ssize_t
VirtualGpsReceiver::Read(uint8_t* buf, size_t len)
{
    size_t       offset  = 0;
    size_t       left    = len;
    ssize_t      size    = 0;
    unsigned int attempt = 100;

    while (left > 0) {
        do {
            size = read(mSockGps, buf + offset, left);
        } while (size < 0 && errno == EINTR && (--attempt > 0));
        if (size == 0) {
            AIC_LOG(ERROR, "GPS socket server is closed.");
            Disconnect();
            return -1;
        } else if (size < 0) {
            AIC_LOG(ERROR, "GPS socket write error: %s", strerror(errno));
            return size;
        } else if (size > 0) {
            offset += size;
            left -= size;
        }
    }
    return offset > 0 ? offset : size;
}

void
VirtualGpsReceiver::workThreadProc()
{
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            if (mStop) {
                AIC_LOG(WARNING,
                        "mStop = %s. Break from while.",
                        mStop ? "true" : "false");
                break;
            }
        }

        if (!Connected()) {
            AIC_LOG(WARNING,
                    "Not connected to GPS server. Need to connect again.");
            if (!Connect()) {
                sleep(3);
                AIC_LOG(ERROR, "Try to connect GPS server again");
                continue;
            } else {
                AIC_LOG(DEBUG, "Connected to GPS server.");
            }
        }

        char     cmd = 0xFF;
        uint8_t* ptr = reinterpret_cast<uint8_t*>(&cmd);
        int      res = Read(ptr, sizeof(cmd));
        if (res < 0) {
            AIC_LOG(WARNING,
                    "%s",
                    mStop ? "mStop = true. Stop"
                          : "Read error, try to connect and read again");
        } else if (res == sizeof(cmd)) {
            uint32_t command;
            switch (cmd) {
                case CMD_QUIT:
                    command = Command::kGpsQuit;
                    AIC_LOG(DEBUG, "CMD_QUIT");
                    mCmdHandler(static_cast<uint32_t>(command));
                    break;
                case CMD_START:
                    command = Command::kGpsStart;
                    AIC_LOG(DEBUG, "GPS_START");
                    mCmdHandler(static_cast<uint32_t>(command));
                    break;
                case CMD_STOP:
                    command = Command::kGpsStop;
                    AIC_LOG(DEBUG, "GPS_STOP");
                    mCmdHandler(static_cast<uint32_t>(command));
                    break;

                default:
                    AIC_LOG(ERROR, "GPS unkown command. cmd = %c", cmd);
                    break;
            }
        } else {
            AIC_LOG(ERROR, "GPS work thread, should not be here, res %d", res);
            break;
        }
    }
    AIC_LOG(DEBUG, "GPS work thread exit");
}