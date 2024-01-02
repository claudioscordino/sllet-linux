#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

#include <Proxy.hpp>
#include <Stats.hpp>

#define ERROR(msg) std::cerr << msg << std::endl;

template <class T>
void Proxy<T>::Connect(const uint16_t port)
{
    //socket()
    fd_ = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
    if (fd_ < 0) {
        ERROR("Socket creation");
        throw std::runtime_error ("Socket error");
    }

    // bind()
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd_, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        close(fd_);
        ERROR("Socket binding");
        throw std::runtime_error ("Bind error");
    }
}

template class Proxy<int>;
