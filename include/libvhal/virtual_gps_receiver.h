#ifndef __VIRTUAL_GPS_RECEIVER_H__
#define __VIRTUAL_GPS_RECEIVER_H__

#include "libvhal_common.h"
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace vhal {
namespace client {

enum
{
    GPS_CMD_QUIT  = 0, // GPS quit
    GPS_CMD_START = 1, // GPS start
    GPS_CMD_STOP  = 2  // GPS stop
};

/**
 * @brief GPS command handler.
 *
 * @param cmd command is one of {GPS_CMD_QUIT, GPS_CMD_START, GPS_CMD_STOP}.
 *
 * @return void
 */
using GpsCommandHandler = std::function<void(uint32_t cmd)>;

class VirtualGpsReceiver
{
public:
    /**
     * @brief Constructor.
     *
     * @param tci TCP connection information.
     * @param callback GPS callback function object or lambda or function
     * pointer.
     *
     */
    VirtualGpsReceiver(struct TcpConnectionInfo tci, GpsCommandHandler gch);
    ~VirtualGpsReceiver();

    /**
     * @brief Connect to remote endpoint.
     *
     * @return <true, null-error-msg> connection successful.
     * @return <false, error-msg> connection failed. Reason for failure is found
     * in error msg string.
     */
    ConnectionResult Connect();

    /**
     * @brief Disconnect to remote endpoint.
     *
     * @return <true, null-error-msg> connection successful.
     * @return <false, error-msg> connection failed. Reason for failure is found
     * in error msg string.
     */
    ConnectionResult Disconnect();

    /**
     * @brief Check the status of connect.
     *
     * @return true It is connecting.
     * @return false It is disconnected.
     */
    bool Connected();

    /**
     * @brief Write GPS data to AIC.
     *
     * @param buf The pointer to the data. Currently, it only supports Global
     * Positioning System Fix Data(GPGGA). For more detail, please reference
     * http://aprs.gids.nl/nmea/#gga.
     * @param len The length of data.
     *
     * @return IOResult
     *         <Number of bytes sent, Empty string> on Success
     *         <Error number, Error message on Failure> on Failure
     */
    IOResult Write(const uint8_t* buf, size_t len);

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
    struct TcpConnectionInfo     mTci;
    uint16_t                     mPort          = 8766;
    GpsCommandHandler            mGpsCmdHandler = nullptr;
    int                          mSockGps       = -1;
    static const char*           kGpsSock;
    volatile Command             mCommand;
    std::unique_ptr<std::thread> mWorkThread;
    std::mutex                   mMutex;
    bool                         mStop = false;
};

} // namespace client
} // namespace vhal
#endif // __VIRTUAL_GPS_RECEIVER_H__
