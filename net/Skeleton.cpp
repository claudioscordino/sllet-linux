#include "Skeleton.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <arpa/inet.h>

#define ERROR(msg) std::cerr << msg << std::endl;

template <class T>
Skeleton<T>::Skeleton(const std::string& address, const uint16_t port)
{
    // socket()
    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        ERROR("Client socket creation");
        throw std::runtime_error ("Client socket error");
    }

    // connect()
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    struct in_addr addr;
    inet_aton(address.c_str(), &addr);
    bcopy(&addr, &serv_addr.sin_addr.s_addr, sizeof(addr));

    if (connect(fd_, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        close(fd_);
        ERROR("connect()");
        throw std::runtime_error("Client socket error");
    }
}

template <class T>
bool Skeleton<T>::Send(Msg<T> msg)
{
    msg.SetTime();
    size_t remaining = sizeof(msg);
    size_t size = remaining;
    while (remaining > 0) {
        ssize_t ret = write (fd_, ((char*)&msg)+(size-remaining), remaining);
        if (ret == 0) {
            //Cannot write more
            break;
        } else if (ret < 0) {
            throw std::runtime_error ("Write error");
            return false; 
        }
        remaining -= ret;
    }
    return true;
}

template class Skeleton<int>;
