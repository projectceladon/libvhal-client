#ifndef TCP_STREAM_SOCKET_CLIENT_IMPL_H
#define TCP_STREAM_SOCKET_CLIENT_IMPL_H

#include "tcp_stream_socket_client.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <system_error>
extern "C"
{
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
}

namespace vhal {
namespace client {

class TcpStreamSocketClient::Impl
{
public:
    Impl(const std::string& remote_server_ip, const int port)
    {
        tcp_sock_addr_.sin_family = AF_INET;
        tcp_sock_addr_.sin_port = htons(port);
        inet_pton(AF_INET,remote_server_ip.c_str(), &tcp_sock_addr_.sin_addr);

        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            throw std::system_error(errno, std::system_category());
        }
    }
    ~Impl() { Close(); }

    ConnectionResult Connect()
    {
        std::string error_msg = "";

        connected_ = ::connect(fd_, (struct sockaddr*)&tcp_sock_addr_, sizeof(struct sockaddr_in)) == 0;
        if (!connected_) {
            error_msg = std::strerror(errno);
        }
        return { connected_, error_msg };
    }

    bool Connected() const { return connected_; }

    int GetNativeSocketFd() const { return fd_; }

    IOResult Send(const uint8_t* data, size_t size)
    {
        std::string error_msg = "";

        ssize_t sent;
        if ((sent = ::send(fd_, data, size, 0)) == -1) {
            std::cout << ". Send() args: fd: " << fd_ << ", data: " << data
                      << ", size: " << size << "\n";
            error_msg = std::strerror(errno);
        }
        return { sent, error_msg };
    }

    IOResult Recv(uint8_t* data, size_t size)
    {
        std::string error_msg = "";
        ssize_t left = size;
        while (left > 0 ) {
            ssize_t received = ::recv(fd_,data, size,0);
            if (received <= 0) {
                std::cout << ". Recv() args: fd: " << fd_ << ", data: " << data
                      << ", size: " << size << "\n";
                error_msg = std::strerror(errno);
                break;
            }
            else {
                data += received;
                left -= received;
            }
        }
        return { size-left, error_msg };
    }

    void Close() {
        if (fd_ < 0) return;
        shutdown(fd_, SHUT_RDWR);
        close(fd_);
        fd_ = -1;
    }

private:
    int  fd_;
    bool connected_ = false;
    struct sockaddr_in tcp_sock_addr_;
};

} // namespace client
} // namespace vhal

#endif /* TCP_STREAM_SOCKET_CLIENT_IMPL_H */
