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

#ifndef __TCP_SOCKET_H__
#define __TCP_SOCKET_H__

#include <stdexcept>
#include <netinet/in.h> // for IPPROTO_TCP, sockadd_in


class TCPSocketClient
{
public:
    /* Constructors */
    /**
    * Construct generic TCPSocket
    * XXX: Don't use, use the constructor with explicit port specification instead
    **/
    //TCPSocketClient() throw(std::runtime_error);

    /**
    * Construct TCPSocket that connects to the given remote server.
    * parameters:
    * - remoteAddr: address of server to connect to
    * - remotePort: port of server to connect to
    **/
    TCPSocketClient(const std::string &remoteAddr, const unsigned short &remotePort) throw(std::runtime_error);


    /* Destructor */
    ~TCPSocketClient();


    /* Communication over socket */
    /**
    * Receive data from remote peer.
    * parameters:
    * - buffer: buffer to receive data
    * - bufferLen: maximum number of bytes to receive
    * return value:
    *   number of bytes received, 0 means connection closed by peer
    **/
    ssize_t recv(void *buffer, const size_t &bufferLen) throw (std::runtime_error);

    /**
    * Send data to remote peer.
    * parameters:
    * - buffer:     buffer to send
    * - bufferLen:  number of bytes in buffer
    * return value:
    *   number of bytes actually written
    **/
    ssize_t send(const void *buffer, const int &bufferLen) throw (std::runtime_error);

    /**
    * Sends the given string over the TCP connection.
    * This is a convenience method which calls the previous method with the correct
    * length parameter.
    * parameters:
    * - message:    message to send
    * return value:
    *   number of bytes actually written
    **/
    ssize_t send(const std::string &message) throw (std::runtime_error);

    ssize_t send(unsigned int val) throw (std::runtime_error);

private:
    // don't allow value semantics on this object
    TCPSocketClient(const TCPSocketClient &sock);
    void operator=(const TCPSocketClient &sock);

    int sockDesc;       // socket descriptor
    sockaddr_in sockAddr; // structure keeping IP and port of peer
};

#endif // __TCP_SOCKET_H__
