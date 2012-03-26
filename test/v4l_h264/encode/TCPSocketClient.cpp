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

#include "TCPSocketClient.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
using std::string;

TCPSocketClient::TCPSocketClient(const std::string &remoteAddr, const unsigned short &remotePort) throw(std::runtime_error) :
sockDesc(-1)
{
    ::memset(&sockAddr, 0, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    struct hostent* hp;
    if ((hp = gethostbyname(remoteAddr.c_str())) == NULL) {
        throw std::runtime_error("Unknown host " + remoteAddr);
    }
    bcopy(hp->h_addr, &sockAddr.sin_addr, hp->h_length);
    sockAddr.sin_port = htons(remotePort);
    if ((sockDesc = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("Socket creation failed (socket())");
    }
    /* Try to connect */
    if (connect(sockDesc, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) < 0) {
        throw std::runtime_error("Error connecting to remote host (connect())");
    }
}

/* Destructor */
TCPSocketClient::~TCPSocketClient()
{
    if (sockDesc >= 0) {
        ::close(sockDesc);
    }
}

/* Communication over socket */
/* Receive data */
ssize_t TCPSocketClient::recv(void *buffer, const size_t &bufferLen) throw (std::runtime_error)
{
    if (sockDesc < 0) {
        throw std::runtime_error("socket is not connected (recv())");
    }
    int rval = ::read(sockDesc, buffer, bufferLen);
    if (rval <= 0) {
        // EOF (connection closed by remote host) or error:
        // reset state, so a new accept() call will succeed
        sockDesc = -1;
        ::memset(&sockAddr, 0, sizeof(sockAddr));
    }
    if (rval == -1) {
        throw std::runtime_error("Error reading from socket (read())");
    }
    return rval;
}

/* Send data */
ssize_t TCPSocketClient::send(const void *buffer, const int &bufferLen) throw (std::runtime_error)
{
    if (sockDesc <= 0) {
        throw std::runtime_error("socket is not connected (send())");
    }

    int rval = ::write(sockDesc, buffer, bufferLen);

    if (rval <= 0) {
        // EOF (connection closed by remote host) or error:
        // reset state, so a new accept() call will succeed
        sockDesc = -1;
        ::memset(&sockAddr, 0, sizeof(sockAddr));
    }

    if (rval == -1) {
        throw std::runtime_error("Error reading from socket (read())");
    }

    return rval;
}

ssize_t TCPSocketClient::send(const string &message) throw (std::runtime_error)
{
    return send(message.c_str(), message.length());
}


ssize_t TCPSocketClient::send(unsigned int val) throw (std::runtime_error)
{
    return send(&val, 4);
}

