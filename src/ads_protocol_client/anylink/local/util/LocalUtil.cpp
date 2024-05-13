/*
 * LocalUtil.cpp
 *
 *  Created on: 2020. 4. 21.
 *      Author: jeongtaek
 */

#include "LocalUtil.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
namespace anylink {

std::string LocalUtil::getLocalAddress() {
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    sockaddr_in loopback;
    if (sock == -1) {
        std::cerr << "Could not socket\n";
    }
    std::memset(&loopback, 0, sizeof(loopback));
    loopback.sin_family = AF_INET;
    loopback.sin_addr.s_addr = INADDR_LOOPBACK;   // using loopback ip address
    loopback.sin_port = htons(9);                 // using debug port
    if (connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) == -1) {
        close(sock);
        std::cerr << "Could not connect\n";
    }
    socklen_t addrlen = sizeof(loopback);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) == -1) {
        close(sock);
        std::cerr << "Could not getsockname\n";
    }
    close(sock);
    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) == 0x0) {
        std::cerr << "Could not inet_ntop\n";
    } else {
        std::cout << "Local ip address: " << buf << "\n";
    }
    std::string addr("0.0.0.0");
    return addr;
}
} /* namespace anylink */
