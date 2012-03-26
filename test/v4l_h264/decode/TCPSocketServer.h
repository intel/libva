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

#ifndef SERVER_ADDR
#define SERVER_ADDR "localhost"
#endif
#ifndef SERVER_PORT
#define SERVER_PORT 8888
#endif


class TCPSocketServer
{
public:
    /* Constructors */
    /**
    * Construct generic TCPSocket
    * XXX: Don't use, use the constructor with explicit port specification instead
    **/
    //TCPSocketServer() throw(std::runtime_error);

    /**
    * Construct TCPSocket that binds to the given local port
    * parameters:
    * - localPort: port to bind to
    **/
    TCPSocketServer(unsigned short localPort) throw(std::runtime_error);


    /* Destructor */
    ~TCPSocketServer();


    /* Handle incoming connections */
    /**
    * Listen for an incoming connection.
    * This call blocks until a connection with a remote peer has been established.
    * parameters:
    * - remoteAddr: (OUT) contains address of peer
    * - remotePort: (OUT) contains port of peer
    * return value:
    *   none
    **/
    void accept(std::string &remoteAddr, unsigned short &remotePort) throw(std::runtime_error);


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


    int recv_data(unsigned char *data, int size);
    unsigned int recv_uint32();


private:
    // don't allow value semantics on this object
    TCPSocketServer(const TCPSocketServer &sock);
    void operator=(const TCPSocketServer &sock);

    int sockDesc;       // listening socket descriptor
    sockaddr_in sockAddr; // structure keeping IP and port of peer

    int connSockDesc;   // connected socket descriptor
    sockaddr_in connSockAddr;
};

#endif // __TCP_SOCKET_H__
