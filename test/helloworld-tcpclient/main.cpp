/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright 2015 ARM Holdings PLC
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
            _stream(SOCKET_STACK_LWIP_IPV4), _domain(domain), _port(port), _connect(this), _receive(this), _resolved(this)
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
	int startTest(const char *path) {
        /* Initialize the flags */
        _got200 = false;
        _gothello = false;
        _error = false;
        /* Fill the request buffer */
        _bpos = snprintf(_buffer, sizeof(_buffer) - 1, "GET %s HTTP/1.1\nHost: %s\n\n", path, HTTP_SERVER_NAME);

        /* Connect to the server */
        printf("Connecting to %s:%d\n", _domain, _port);
        /* Resolve the domain name: */
        socket_error_t err = _stream.resolve(_domain,
                (handler_t) (void *) _resolved.entry());
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
    }
protected:
    /**
     * On Connect handler
     * Sends the request which was generated in startTest
     */
    void onConnect() {
        /* Send the request */
        handler_t h = (handler_t) _receive.entry();
        _stream.setOnReadable(h);
        socket_error_t rc = _stream.send(_buffer, _bpos);
        if (rc != SOCKET_ERROR_NONE) {
            _error = true;
        }
    }
    /**
     * On Receive handler
     * Parses the response from the server, to check for the HTTP 200 status code and the expected response ("Hello World!")
     */
    void onReceive() {
        _bpos = sizeof(_buffer);
        /* Read data out of the socket */
        socket_error_t err = _stream.recv(_buffer, &_bpos);
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
    }
    /**
     * On DNS Handler
     * Reads the address returned by DNS, then starts the connect process.
     */
    void onDNS() {
        handler_t h = (handler_t) _connect.entry();
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
            socket_error_t err = _stream.connect(&_remoteAddr, _port, h);
            if (err != SOCKET_ERROR_NONE) {
                _error = true;
            }
        }
    }

protected:
    mbed::TCPStream _stream;        /**< The TCP Socket */
    const char *_domain;            /**< The domain name of the HTTP server */
    const uint16_t _port;           /**< The HTTP server port */
    char _buffer[RECV_BUFFER_SIZE]; /**< The response buffer */
    size_t _bpos;                   /**< The current offset in the response buffer */
    mbed::SocketAddr _remoteAddr;   /**< The remote address */
    bool _got200;                   /**< Status flag for HTTP 200 */
    bool _gothello;                 /**< Status flag for finding the test string */
    bool _error;                    /**< Status flag for an error */
};

/**
 * The main loop of the TCP Hello World test
 */
int main() {
	EthernetInterface eth;
	Timer tp;      /**< Use a timer to record how long DHCP takes */

    printf("TCP client IP Address is %s\n", eth.getIPAddress());

    HelloHTTP hello(HTTP_SERVER_NAME, HTTP_SERVER_PORT);
    rc = hello.startTest(HTTP_PATH);
    if (rc != SOCKET_ERROR_NONE) {
        return 1;
    }
    while (!hello.done()) {
        __WFI();
    }
    /* Shut down the socket before the ethernet interface */
    hello.close();
    eth.disconnect();
    notify_completion(!hello.error());
    return (int)hello.error();
}

