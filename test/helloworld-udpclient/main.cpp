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
 *  \brief An example UDP Time application
 *  This application reads the current UTC time by sending a packet to
 *  utcnist.colorado.edu (128.138.140.44)
 *
 *  This example is implemented as a logic class (UDPGetTime) wrapping a UDP socket.
 *  The logic class handles all events, leaving the main loop to just check if the process
 *  has finished.
 */
#include "mbed.h"
#include <mbed-net-lwip-eth/EthernetInterface.h>
#include <mbed-net-sockets/UDPSocket.h>
#include "test_env.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* TODO: Remove when yotta supports init. */
#include "lwipv4_init.h"

#define UDP_TIME_PORT 37

namespace {
     /* const char *HTTP_SERVER_NAME = "utcnist.colorado.edu"; */
     const char *HTTP_SERVER_NAME = "128.138.140.44";
     const float YEARS_TO_PASS = 114.0;
}

/**
 * \brief UDPGetTime implements the logic for fetching the UTC time over UDP
 */
class UDPGetTime {
public:
    /**
     * UDPGetTime Constructor
     * Initializes the UDP socket, sets up event handlers and flags.
     *
     * Note that CThunk is used for event handlers.  This will be changed to a C++
     * function pointer in an upcoming release.
     */
    UDPGetTime() :
        sock(SOCKET_STACK_LWIP_IPV4),
        _udpTimePort(UDP_TIME_PORT),
        _time(0)
    {
        /* Open the UDP socket so that DNS will work */
        sock.open(SOCKET_AF_INET4);
    }
    /**
     * Initiate the get time operation
     * Starts by resolving the address (optionally with DNS)
     * @param[in] address The address from which to query the time
     * @return SOCKET_ERROR_NONE on success, or an error code on failure
     */
    socket_error_t startGetTime(const char *address) {
        /* Initialize the GetTime flags */
        resolved = false;
        received = false;
        /* Start the DNS operation */
        return sock.resolve(address,handler_t(this, &UDPGetTime::onDNS));
    }
    /**
     * Check if the UDP time response has been received
     * @return true if the time response has been received, false otherwise.
     */
    bool isReceived() {return received;}
    /**
     * Gets the time response.
     * Once the response has been received from the remote host, getTime() can be used
     * to extract the time from UDPGetTime
     * @return The 32-bit time since 1970-01-01T00:00:00 in seconds.
     */
    uint32_t time() { return _time;}
protected:
    /**
     * The DNS Response Handler
     * @param[in] arg (unused)
     */
    void onDNS(socket_error_t err) {
        /* Extract the Socket event to read the resolved address */
        socket_event_t *event = sock.getEvent();
        _resolvedAddr.setAddr(&event->i.d.addr);
        /* Register the read handler */
        sock.setOnReadable(handler_t(this, &UDPGetTime::onRecv));
        /* Send the query packet to the remote host */
        err = sock.send_to("time",strlen("time"),&_resolvedAddr,_udpTimePort);
        /* A failure on send is a fatal error in this example */
        if (err != SOCKET_ERROR_NONE) {
            printf("Socket Error %d\r\n", err);
            notify_completion(false);
        }
    }
    /**
     * The Time Query response handler
     * @param[in] arg (unused)
     */
    void onRecv(socket_error_t err) {
        /* Initialize the buffer size */
        size_t nRx = sizeof(_rxBuf);
        /* Receive some bytes */
        err = sock.recv(_rxBuf, &nRx);
        /* A failure on recv is a fatal error in this example */
        if (err != SOCKET_ERROR_NONE) {
            printf("Socket Error %d\r\n", err);
            notify_completion(false);
        }
        uint32_t time;
        /* Correct for possible non 32-bit alignment */
        memcpy(&time, _rxBuf, sizeof(time));
        /* Switch to host order */
        _time = ntohl(time);
        received = true;
    }

protected:
    mbed::UDPSocket sock;
    mbed::SocketAddr _resolvedAddr;
    volatile bool received;
    volatile bool resolved;
    const uint16_t _udpTimePort;
    uint32_t _time;

protected:
    char _rxBuf[32];
};

int main() {
    EthernetInterface eth;
    /* Initialise with DHCP, connect, and start up the stack */
    eth.init();
    eth.connect();
    lwipv4_socket_init();

    printf("UDP client IP Address is %s\r\n", eth.getIPAddress());

    /* Get the current time */
    UDPGetTime gt;
    gt.startGetTime(HTTP_SERVER_NAME);

    /* Wait until a response is received */
    while (!gt.isReceived()) {
        __WFI();
    }

    printf("UDP: %lu seconds since 01/01/1900 00:00 GMT\r\n", gt.time());

    eth.disconnect();
    return 0;
}
