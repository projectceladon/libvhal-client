#include "camera_sink.h"
#include "camera_sink_impl.h"
#include <functional>
#include <memory>

namespace vhal {
namespace client {

using CameraCallback = std::function<void(const CtrlMessage& ctrl_msg)>;

CameraSink::CameraSink(std::unique_ptr<UnixSocket> socket,
                       CameraCallback              cb = nullptr)
  : impl_{ std::make_unique<Impl>(std::move(socket), cb) }
{}

/**
 * @brief Destroy the Camera Sink object
 *
 */
~CameraSink();

/**
 * @brief Set the Callback Handler object
 *
 * @param cb
 * @return true
 * @return false
 */
bool
SetCallbackHandler(CameraCallback cb);

}; // namespace client
} // namespace vhal