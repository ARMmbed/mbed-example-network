/*
 * Copyright (c) 2015, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/** \file main.cpp
 *  \brief An example TCP Server application
 *  This listens on TCP Port 7 for incoming connections. The server rejects incoming connections
 *  while it has one connection active. Once an incoming connection is received, the connected socket
 *  echos any incoming buffers back to the remote host. On disconnect, the server shuts down the echoing
 *  socket and resumes accepting connections.
 *
 *  This example is implemented as a logic class (TCPEchoServer) wrapping a TCP server socket.
 *  The logic class handles all events, leaving the main loop to just check for disconnected sockets.
 */
#include "mbed.h"
#include "EthernetInterface.h"
#include "mbed-net-sockets/TCPListener.h"
#include "mbed-net-socket-abstract/socket_api.h"
#include "Timer.h"
#include "mbed-net-lwip/lwipv4_init.h"
#include "minar/minar.h"
#include "core-util/FunctionPointer.h"

namespace {
    const int ECHO_SERVER_PORT = 7;
    const int BUFFER_SIZE = 64;
}
using namespace mbed::Sockets::v0;
/**
 * \brief TCPEchoServer implements the logic for listening for TCP connections and
 *        echoing characters back to the sender.
 */
class TCPEchoServer {
public:
    /**
     * The TCPEchoServer Constructor
     * Initializes the server socket
     */
    TCPEchoServer():
        _server(SOCKET_STACK_LWIP_IPV4), _stream(NULL),
        _disconnect_pending(false)
        {
            _server.setOnError(TCPStream::ErrorHandler_t(this, &TCPEchoServer::onError));
        }
    /**
     * Start the server socket up and start listening
     * @param[in] port the port to listen on
     * @return SOCKET_ERROR_NONE on success, or an error code on failure
     */
    void start(const uint16_t port)
    {
        do {
            socket_error_t err = _server.open(SOCKET_AF_INET4);
            if (_server.error_check(err)) break;
            err = _server.bind("0.0.0.0",port);
            if (_server.error_check(err)) break;
            err = _server.start_listening(TCPListener::IncomingHandler_t(this, &TCPEchoServer::onIncoming));
            if (_server.error_check(err)) break;
        } while (0);
    }
protected:
    void onError(Socket *s, socket_error_t err) {
        (void) s;
        printf("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
        if (_stream != NULL) {
            _stream->close();
        }

    }
    /**
     * onIncomming handles the allocation of new streams when an incoming connection request is received.
     * @param[in] err An error code.  If not SOCKET_ERROR_NONE, the server will reject the incoming connection
     */
    void onIncoming(TCPListener *s, void *impl)
    {
        do {
            if (impl == NULL) {
                onError(s, SOCKET_ERROR_NULL_PTR);
                break;
            }
            _stream = _server.accept(impl);
            if (_stream == NULL) {
                onError(s, SOCKET_ERROR_BAD_ALLOC);
                break;
            }
            _stream->setOnError(TCPStream::ErrorHandler_t(this, &TCPEchoServer::onError));
            _stream->setOnReadable(TCPStream::ReadableHandler_t(this, &TCPEchoServer::onRX));
            _stream->setOnDisconnect(TCPStream::DisconnectHandler_t(this, &TCPEchoServer::onDisconnect));
        } while (0);
    }
    /**
     * onRX handles incoming buffers and returns them to the sender.
     * @param[in] err An error code.  If not SOCKET_ERROR_NONE, the server close the connection.
     */
    void onRX(Socket *s) {
        socket_error_t err;
        size_t size = BUFFER_SIZE-1;
        err = s->recv(buffer, &size);
        if (!s->error_check(err)) {
            buffer[size]=0;
            err = s->send(buffer,size);
            if (err != SOCKET_ERROR_NONE)
                onError(s, err);
        }
    }
    /**
     * onDisconnect deletes closed streams
     */
    void onDisconnect(TCPStream *s) {
        if(s != NULL)
            delete s;
    }
protected:
    TCPListener _server;
    TCPStream*  _stream;
    volatile bool _disconnect_pending;
protected:
    char buffer[BUFFER_SIZE];
};
EthernetInterface eth;
TCPEchoServer server;

void app_start(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    eth.init(); //Use DHCP
    eth.connect();
    lwipv4_socket_init();
    printf("MBED: Server IP Address is %s:%d\r\n", eth.getIPAddress(), ECHO_SERVER_PORT);
    mbed::util::FunctionPointer1<void, uint16_t> fp(&server, &TCPEchoServer::start);
    minar::Scheduler::postCallback(fp.bind(ECHO_SERVER_PORT));
}
