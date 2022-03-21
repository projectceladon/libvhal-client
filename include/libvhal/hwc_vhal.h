#ifndef HWC_VHAL_H
#define HWC_VHAL_H
#include <atomic>
#include <string>
#include <functional>
#include <memory>
#include <map>
#include <thread>
#include "libvhal_common.h"
#include "display-protocol.h"

namespace vhal {
namespace client {

enum CommandType
{
    FRAME_CREATE  = 0, // create frame
    FRAME_REMOVE  = 1, // remove frame
    FRAME_DISPLAY = 2, // display frame
};

struct ConfigInfo
{
    UnixConnectionInfo unix_conn_info;
    int video_res_width = 0;
    int video_res_height = 0;
    std::string video_device = "";
    int user_id = 0;
};

using HwcHandler = std::function<void(CommandType cmd, cros_gralloc_handle_t handle)>;

class VirtualHwcReceiver
{
public:
    VirtualHwcReceiver(struct ConfigInfo info, HwcHandler handler);
    ~VirtualHwcReceiver();
    IOResult start();
    IOResult stop();
    IOResult setMode(int width, int height);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
}
#endif
