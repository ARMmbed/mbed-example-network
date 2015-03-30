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
 *  \brief An example TCP Client application
 *  This application sends an HTTP request to developer.mbed.org and searches for a string in
 *  the result.
 *
 *  This example is implemented as a logic class (HelloHTTP) wrapping a TCP socket.
 *  The logic class handles all events, leaving the main loop to just check if the process
 *  has finished.
 */
#include "mbed.h"
#include <mbed-net-lwip-eth/EthernetInterface.h>
#include <mbed-net-sockets/TCPStream.h>
#include "test_env.h"

#include "lwipv4_init.h"

namespace {
const char *HTTP_SERVER_NAME = "developer.mbed.org";
const int HTTP_SERVER_PORT = 80;
const int RECV_BUFFER_SIZE = 600;

const char HTTP_PATH[] = "/media/uploads/mbed_official/hello.txt";
const size_t HTTP_PATH_LEN = sizeof(HTTP_PATH) - 1;

/* Test related data */
const char *HTTP_OK_STR = "200 OK";
const char *HTTP_HELLO_STR = "Hello world!";
}

/**
 * \brief HelloHTTP implements the logic for fetching a file from a webserver
 * using a TCP socket and parsing the result.
 */
class HelloHTTP {
public:
    /**
     * HelloHTTP Constructor
     * Initializes the TCP socket, sets up event handlers and flags.
     *
     * Note that CThunk is used for event handlers.  This will be changed to a C++
     * function pointer in an upcoming release.
     *
     *
     * @param[in] domain The domain name to fetch from
     * @param[in] port The port of the HTTP server
     */
    HelloHTTP(const char * domain, const uint16_t port) :
            _stream(SOCKET_STACK_LWIP_IPV4), _domain(domain), _port(port)
    {

        _error = false;
        _gothello = false;
        _got200 = false;
        _bpos = 0;
        _stream.open(SOCKET_AF_INET4);
    }
    /**
     * Initiate the test.
     *
     * Starts by clearing test flags, then resolves the address with DNS.
     *
     * @param[in] path The path of the file to fetch from the HTTP server
     * @return SOCKET_ERROR_NONE on success, or an error code on failure
     */
    socket_error_t startTest(const char *path) {
        /* Initialize the flags */
        _got200 = false;
        _gothello = false;
        _error = false;
        _disconnected = false;
        /* Fill the request buffer */
        _bpos = snprintf(_buffer, sizeof(_buffer) - 1, "GET %s HTTP/1.1\nHost: %s\n\n", path, HTTP_SERVER_NAME);

        /* Connect to the server */
        printf("Connecting to %s:%d\r\n", _domain, _port);
        /* Resolve the domain name: */
        socket_error_t err = _stream.resolve(_domain, handler_t(this, &HelloHTTP::onDNS));
        return err;
    }
    /**
     * Check if the test has completed.
     * @return Returns true if done, false otherwise.
     */
    bool done() {
        return _error || (_got200 && _gothello);
    }
    /**
     * Check if there was an error
     * @return Returns true if there was an error, false otherwise.
     */
    bool error() {
        return _error;
    }
    /**
     * Closes the TCP socket
     */
    void close() {
        _stream.close();
        while (!_disconnected)
            __WFI();
    }
protected:
    /**
     * On Connect handler
     * Sends the request which was generated in startTest
     */
    void onConnect(socket_error_t err) {
        /* Send the request */
        _stream.setOnReadable(handler_t(this, &HelloHTTP::onReceive));
        _stream.setOnDisconnect(handler_t(this, &HelloHTTP::onDisconnect));
        err = _stream.send(_buffer, _bpos);
        if (err != SOCKET_ERROR_NONE) {
            _error = true;
        }
    }
    /**
     * On Receive handler
     * Parses the response from the server, to check for the HTTP 200 status code and the expected response ("Hello World!")
     */
    void onReceive(socket_error_t err) {
        _bpos = sizeof(_buffer);
        /* Read data out of the socket */
        err = _stream.recv(_buffer, &_bpos);
        if (err != SOCKET_ERROR_NONE) {
            _error = true;
            return;
        }
        _buffer[_bpos] = 0;
        /* Check each of the flags */
        _got200 = _got200 || strstr(_buffer, HTTP_OK_STR) != NULL;
        _gothello = _gothello || strstr(_buffer, HTTP_HELLO_STR) != NULL;
        /* Print status messages */
        printf("HTTP: Received %d chars from server\r\n", _bpos);
        printf("HTTP: Received 200 OK status ... %s\r\n", _got200 ? "[OK]" : "[FAIL]");
        printf("HTTP: Received '%s' status ... %s\r\n", HTTP_HELLO_STR, _gothello ? "[OK]" : "[FAIL]");
        printf("HTTP: Received message:\r\n\r\n");
        printf("%s", _buffer);
        _error = !(_got200 && _gothello);
    }
    /**
     * On DNS Handler
     * Reads the address returned by DNS, then starts the connect process.
     */
    void onDNS(socket_error_t err) {
        socket_event_t *e = _stream.getEvent();
        /* Check that the result is a valid DNS response */
        if (e->i.d.addr.type == SOCKET_STACK_UNINIT) {
            /* Could not find DNS entry */
            _error = true;
            printf("Could not find DNS entry for %s", HTTP_SERVER_NAME);
            return;
        } else {
            /* Start connecting to the remote host */
            _remoteAddr.setAddr(&e->i.d.addr);
            err = _stream.connect(&_remoteAddr, _port, handler_t(this, &HelloHTTP::onConnect));

            if (err != SOCKET_ERROR_NONE) {
                _error = true;
            }
        }
    }
    void onDisconnect(socket_error_t err) {
        _disconnected = true;
    }

protected:
    mbed::TCPStream _stream;        /**< The TCP Socket */
    const char *_domain;            /**< The domain name of the HTTP server */
    const uint16_t _port;           /**< The HTTP server port */
    char _buffer[RECV_BUFFER_SIZE]; /**< The response buffer */
    size_t _bpos;                   /**< The current offset in the response buffer */
    mbed::SocketAddr _remoteAddr;   /**< The remote address */
    volatile bool _got200;          /**< Status flag for HTTP 200 */
    volatile bool _gothello;        /**< Status flag for finding the test string */
    volatile bool _error;           /**< Status flag for an error */
    volatile bool _disconnected;
};

/**
 * The main loop of the TCP Hello World test
 */
int main() {
    EthernetInterface eth;
    /* Initialise with DHCP, connect, and start up the stack */
    eth.init();
    eth.connect();
    lwipv4_socket_init();

    printf("TCP client IP Address is %s\r\n", eth.getIPAddress());

    HelloHTTP hello(HTTP_SERVER_NAME, HTTP_SERVER_PORT);
    socket_error_t rc = hello.startTest(HTTP_PATH);
    if (rc != SOCKET_ERROR_NONE) {
        return 1;
    }
    while (!hello.done()) {
        __WFI();
    }
    if (hello.error()) {
        printf("Failed to fetch %s from %s:%d\r\n", HTTP_PATH, HTTP_SERVER_NAME, HTTP_SERVER_PORT);
    }
    /* Shut down the socket before the ethernet interface */
    hello.close();
    eth.disconnect();
    return (int)hello.error();
}

