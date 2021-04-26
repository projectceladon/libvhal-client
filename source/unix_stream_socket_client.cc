
#include "unix_stream_socket_client.h"
#include "unix_stream_socket_client_impl.h"

namespace vhal {
namespace client {
UnixStreamSocketClient::UnixStreamSocketClient(
  const std::string& remote_server_path)
  : impl_{ std::make_unique<Impl>(remote_server_path) }
{}

UnixStreamSocketClient::~UnixStreamSocketClient() = default;

IStreamSocketClient::ConnectionResult
UnixStreamSocketClient::Connect()
{
    return impl_->Connect();
}

bool
UnixStreamSocketClient::Connected() const
{
    return impl_->Connected();
}

IStreamSocketClient::IOResult
UnixStreamSocketClient::Send(const uint8_t* data, size_t size)
{
    return impl_->Send(data, size);
}

IStreamSocketClient::IOResult
UnixStreamSocketClient::Recv(uint8_t* data, size_t size)
{
    return impl_->Recv(data, size);
}

void
UnixStreamSocketClient::Close()
{
    impl_->Close();
}

} // namespace client
} // namespace vhal
