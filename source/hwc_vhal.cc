#include "hwc_vhal.h"
#include "receiver_log.h"

#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <poll.h>
#include <stdio.h>

#include "hwc_vhal_impl.h"
#include "unix_stream_socket_client.h"
#define HWC_UNIX_SOCKET "/hwc-sock"

namespace vhal {
namespace client {

VirtualHwcReceiver::VirtualHwcReceiver(struct ConfigInfo info, HwcHandler handler)
{
    auto sockPath = info.unix_conn_info.socket_dir;
    if (sockPath.length() == 0) {
        throw std::invalid_argument("Please set a valid sensors socket_dir");
    } else {
        sockPath += HWC_UNIX_SOCKET;
        if (info.unix_conn_info.android_instance_id >= 0) {
            sockPath += std::to_string(info.unix_conn_info.android_instance_id);
        }
    }

    //Creating interface to communicate to VHAL via libvhal
    auto unix_sock_client =
      std::make_unique<UnixStreamSocketClient>(std::move(sockPath));
    impl_ = std::make_unique<Impl>(std::move(unix_sock_client), std::move(info), std::move(handler));
    if (!impl_->init()) {
        throw std::logic_error("failed to create hwc");
    }
}

VirtualHwcReceiver::~VirtualHwcReceiver() {impl_->stop();}

IOResult VirtualHwcReceiver::start()
{
    return impl_->start();
}

IOResult VirtualHwcReceiver::stop()
{
    return impl_->stop();
}

IOResult VirtualHwcReceiver::setMode(int width, int height)
{
    return impl_->setMode(width, height);
}

IOResult VirtualHwcReceiver::setVideoAlpha(uint32_t action)
{
    return impl_->setVideoAlpha(action);
}

}
}
