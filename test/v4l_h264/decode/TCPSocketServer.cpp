/*
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
* C++ wrapper around an TCP socket
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>  // for data types
#include <sys/socket.h> // for socket(), connect(), send(), recv()
#include <netinet/in.h> // for IPPROTO_TCP, sockadd_in
#include <arpa/inet.h>  // for inet_ntoa()
#include <unistd.h> // for close()
#include <netdb.h>  // for hostent, gethostbyname()
#include <fcntl.h>  // for fcntl()
#include <errno.h>

#include <cstring>  // for memset

#include "TCPSocketServer.h"

using std::string;


TCPSocketServer::TCPSocketServer(unsigned short localPort) throw(std::runtime_error) :
sockDesc(-1),
    connSockDesc(-1)
{
    // create new socket
    if ((sockDesc = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("Socket creation failed (socket())");
    }

    ::memset(&sockAddr, 0, sizeof(sockAddr));
    ::memset(&connSockAddr, 0, sizeof(connSockAddr));

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockAddr.sin_port = htons(localPort);

    if (bind(sockDesc, (sockaddr *) &sockAddr, sizeof(sockAddr)) < 0)
    {
        throw std::runtime_error("Bind to local port failed (bind())");
    }

    if (listen(sockDesc, 2) < 0)
    {
        throw std::runtime_error("Socket initialization failed (listen())");
    }
}


/* Destructor */
TCPSocketServer::~TCPSocketServer()
{
    if (connSockDesc > 0) {
        ::close(connSockDesc);
    }

    ::close(sockDesc);
}


/* Listen for an incoming connection */
void TCPSocketServer::accept(string &remoteAddr, unsigned short &remotePort) throw (std::runtime_error)
{
    if (connSockDesc > 0) {
        throw std::runtime_error("accept() called, but socket is already connected");
    }

    socklen_t connSockLen = sizeof(connSockAddr);
    if ((connSockDesc = ::accept(sockDesc, (sockaddr *) &connSockAddr, &connSockLen)) < 0) {
        throw std::runtime_error("Connection accept failed (accept())");
    }

    remoteAddr = inet_ntoa(connSockAddr.sin_addr);
    remotePort = ntohs(connSockAddr.sin_port);
}


/* Communication over socket */
/* Receive data */
ssize_t TCPSocketServer::recv(void *buffer, const size_t &bufferLen) throw (std::runtime_error)
{
    if (connSockDesc <= 0) {
        throw std::runtime_error("recv() called, but socket is not connected. Call accept() first");
    }

    int rval = ::read(connSockDesc, buffer, bufferLen);

    if (rval <= 0) {
        // EOF (connection closed by remote host) or error:
        // reset state, so a new accept() call will succeed
        connSockDesc = -1;
        ::memset(&connSockAddr, 0, sizeof(connSockAddr));
    }

    if (rval == -1) {
        throw std::runtime_error("Error reading from socket (read())");
    }

    return rval;
}


/* Send data */
ssize_t TCPSocketServer::send(const void *buffer, const int &bufferLen) throw (std::runtime_error)
{
    if (connSockDesc <= 0) {
        throw std::runtime_error("send() called, but socket is not connected. Call accept() first");
    }

    int rval = ::write(connSockDesc, buffer, bufferLen);

    if (rval <= 0) {
        // EOF (connection closed by remote host) or error:
        // reset state, so a new accept() call will succeed
        connSockDesc = -1;
        ::memset(&connSockAddr, 0, sizeof(connSockAddr));
    }

    if (rval == -1) {
        throw std::runtime_error("Error reading from socket (read())");
    }

    return rval;
}

ssize_t TCPSocketServer::send(const string &message) throw (std::runtime_error)
{
    send(message.c_str(), message.length());
}


int TCPSocketServer::recv_data(unsigned char *data, int size)
{
    int total = 0;
    try {
        int pos = 0;
        int recvMsgSize;
        for (;;) {
            // try to receive a message
            recvMsgSize = recv(&data[pos], size);
            if ((recvMsgSize < 0) && (errno == EWOULDBLOCK)) {
                // no data received on non-blocking socket
                usleep(100);
            } else {
                total += recvMsgSize;
                if (recvMsgSize != size) {
                    pos += recvMsgSize;
                    size -= recvMsgSize;
                } else {
                    break;
                }
            }
        }
    }
    catch (const std::exception& e) {
        printf("%s\n", e.what());
        exit(1);
    }
    return total;
}

unsigned int TCPSocketServer::recv_uint32()
{
    unsigned int buffer;
    recv_data((unsigned char*)&buffer, 4);
    return buffer;
}
