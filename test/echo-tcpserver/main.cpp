/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
#include <mbed-net-sockets/TCPListener.h>
#include <mbed-net-socket-abstract/socket_api.h>
#include <Timer.h>
#include <mbed-net-lwip/lwipv4_init.h>

namespace {
    const int ECHO_SERVER_PORT = 7;
    const int BUFFER_SIZE = 64;
}
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
        {}
    /**
     * Start the server socket up and start listening
     * @param[in] port the port to listen on
     * @return SOCKET_ERROR_NONE on success, or an error code on failure
     */
    socket_error_t start(const uint16_t port)
    {
        socket_error_t err = _server.open(SOCKET_AF_INET4);
        if (err) {
            return err;
        }
        err = _server.bind("0.0.0.0",port);
        if (err) {
            return err;
        }
        err = _server.start_listening(handler_t(this, &TCPEchoServer::onIncoming));
        if (err) {
            return err;
        }
        return SOCKET_ERROR_NONE;
    }
    /**
     * This function is called after each interrupt to check for streams that have disconnected and
     * require teardown.
     */
    void checkStream()
    {
        if (_disconnect_pending) {
            if (_stream != NULL) {
                delete _stream;
            }
            _stream = NULL;
            _disconnect_pending = false;
        }
    }
protected:
    /**
     * onIncomming handles the allocation of new streams when an incoming connection request is received.
     * @param[in] err An error code.  If not SOCKET_ERROR_NONE, the server will reject the incoming connection
     */
    void onIncoming(socket_error_t err)
    {
        socket_event_t *event = _server.getEvent();
        if (err != SOCKET_ERROR_NONE || _stream != NULL) {
            event->i.a.reject = 1;
            return;
        }
        _stream = _server.accept(static_cast<struct socket *>(event->i.a.newimpl));
        if (_stream == NULL) {
            event->i.a.reject = 1;
            return;
        }
        _stream->setOnReadable(handler_t(this, &TCPEchoServer::onRX));
        _stream->setOnDisconnect(handler_t(this, &TCPEchoServer::onDisconnect));
    }
    /**
     * onRX handles incoming buffers and returns them to the sender.
     * @param[in] err An error code.  If not SOCKET_ERROR_NONE, the server close the connection.
     */
    void onRX(socket_error_t err) {
        if (err != SOCKET_ERROR_NONE) {
            _stream->close();
            return;
        }
        size_t size = BUFFER_SIZE-1;
        err = _stream->recv(buffer, &size);
        if (err != SOCKET_ERROR_NONE) {
            _stream->close();
            return;
        }
        buffer[size]=0;
        err = _stream->send(buffer,size);
        if (err != SOCKET_ERROR_NONE) {
            _stream->close();
        }
    }
    /**
     * onDisconnect sets the _disconnect_pending flag to shut down the
     * @param[in] err An error code.  Ignored in onDisconnect.
     */
    void onDisconnect(socket_error_t err) {
        (void) err;
        _disconnect_pending = true;
    }
protected:
    TCPListener _server;
    TCPStream*  _stream;
    volatile bool _disconnect_pending;
protected:
    char buffer[BUFFER_SIZE];
};
/**
 * The main loop of the TCP Echo Server example
 * @return 0; however the main loop should never return.
 */
int main (void) {
    EthernetInterface eth;
    eth.init(); //Use DHCP
    eth.connect();
    lwipv4_socket_init();
    printf("MBED: Server IP Address is %s:%d\r\n", eth.getIPAddress(), ECHO_SERVER_PORT);

    TCPEchoServer server;
    socket_error_t err = server.start(ECHO_SERVER_PORT);
    if (err) {
        printf("Socket error: %s (%d)\n", socket_strerror(err), err);
    }

    while (true) {
        __WFI();
        server.checkStream();
    }
    return 0;
}
