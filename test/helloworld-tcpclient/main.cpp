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
#include "EthernetInterface.h"
#include "mbed-net-sockets/TCPStream.h"
#include "test_env.h"
#include "minar/minar.h"

#include "lwipv4_init.h"

namespace {
const char *HTTP_SERVER_NAME = "mbed.org";
const int HTTP_SERVER_PORT = 80;
const int RECV_BUFFER_SIZE = 600;

const char HTTP_PATH[] = "/assets/uploads/hello.txt";
const size_t HTTP_PATH_LEN = sizeof(HTTP_PATH) - 1;

/* Test related data */
const char *HTTP_OK_STR = "200 OK";
const char *HTTP_HELLO_STR = "Hello world!";
}

using namespace mbed::Sockets::v0;

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
        _stream.setOnError(TCPStream::ErrorHandler_t(this, &HelloHTTP::onError));
    }
    /**
     * Initiate the test.
     *
     * Starts by clearing test flags, then resolves the address with DNS.
     *
     * @param[in] path The path of the file to fetch from the HTTP server
     */
    void startTest(const char *path) {
        /* Initialize the flags */
        _got200 = false;
        _gothello = false;
        _error = false;
        _disconnected = false;
        /* Fill the request buffer */
        _bpos = snprintf(_buffer, sizeof(_buffer) - 1, "GET %s HTTP/1.1\nHost: %s\n\n", path, HTTP_SERVER_NAME);

        /* Connect to the server */
        printf("Starting DNS lookup for %s\r\n", _domain);
        /* Resolve the domain name: */
        socket_error_t err = _stream.resolve(_domain, TCPStream::DNSHandler_t(this, &HelloHTTP::onDNS));
        _stream.error_check(err);
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
protected:
    void onError(Socket *s, socket_error_t err) {
        (void) s;
        printf("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
        _stream.close();
        _error = true;
        printf("{{%s}}\r\n",(error()?"failure":"success"));
        printf("{{end}}\r\n");
    }
    /**
     * On Connect handler
     * Sends the request which was generated in startTest
     */
    void onConnect(TCPStream *s) {
        char buf[16];
        _remoteAddr.fmtIPv4(buf,sizeof(buf));
        printf("Connected to %s:%d\r\n", buf, _port);
        /* Send the request */
        s->setOnReadable(TCPStream::ReadableHandler_t(this, &HelloHTTP::onReceive));
        s->setOnDisconnect(TCPStream::DisconnectHandler_t(this, &HelloHTTP::onDisconnect));
        printf("Sending HTTP Get Request...\r\n");
        socket_error_t err = _stream.send(_buffer, _bpos);
        s->error_check(err);
    }
    /**
     * On Receive handler
     * Parses the response from the server, to check for the HTTP 200 status code and the expected response ("Hello World!")
     */
    void onReceive(Socket *s) {
        printf("HTTP Response received.\r\n");
        _bpos = sizeof(_buffer);
        /* Read data out of the socket */
        socket_error_t err = s->recv(_buffer, &_bpos);
        if (err != SOCKET_ERROR_NONE) {
            onError(s, err);
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

        s->close();
    }
    /**
     * On DNS Handler
     * Reads the address returned by DNS, then starts the connect process.
     */
    void onDNS(Socket *s, struct socket_addr addr, const char *domain) {
        /* Check that the result is a valid DNS response */
        if (socket_addr_is_any(&addr)) {
            /* Could not find DNS entry */
            printf("Could not find DNS entry for %s", HTTP_SERVER_NAME);
            onError(s, SOCKET_ERROR_DNS_FAILED);
        } else {
            /* Start connecting to the remote host */
            char buf[16];
            _remoteAddr.setAddr(&addr);
            _remoteAddr.fmtIPv4(buf,sizeof(buf));
            printf("DNS Response Received:\r\n%s: %s\r\n", domain, buf);
            printf("Connecting to %s:%d\r\n", buf, _port);
            socket_error_t err = _stream.connect(_remoteAddr, _port, TCPStream::ConnectHandler_t(this, &HelloHTTP::onConnect));

            if (err != SOCKET_ERROR_NONE) {
                onError(s, err);
            }
        }
    }
    void onDisconnect(TCPStream *s) {
        s->close();
        printf("{{%s}}\r\n",(error()?"failure":"success"));
        printf("{{end}}\r\n");
    }

protected:
    TCPStream _stream;              /**< The TCP Socket */
    const char *_domain;            /**< The domain name of the HTTP server */
    const uint16_t _port;           /**< The HTTP server port */
    char _buffer[RECV_BUFFER_SIZE]; /**< The response buffer */
    size_t _bpos;                   /**< The current offset in the response buffer */
    SocketAddr _remoteAddr;         /**< The remote address */
    volatile bool _got200;          /**< Status flag for HTTP 200 */
    volatile bool _gothello;        /**< Status flag for finding the test string */
    volatile bool _error;           /**< Status flag for an error */
    volatile bool _disconnected;
};

/**
 * The main loop of the TCP Hello World test
 */
EthernetInterface eth;
HelloHTTP *hello;

void app_start(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    printf("{{start}}\r\n");
    /* Initialise with DHCP, connect, and start up the stack */
    eth.init();
    eth.connect();
    lwipv4_socket_init();

    hello = new HelloHTTP(HTTP_SERVER_NAME, HTTP_SERVER_PORT);

    printf("TCP client IP Address is %s\r\n", eth.getIPAddress());

    mbed::FunctionPointer1<void, const char*> fp(hello, &HelloHTTP::startTest);
    minar::Scheduler::postCallback(fp.bind(HTTP_PATH));
}
