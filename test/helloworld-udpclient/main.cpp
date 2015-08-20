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
 *  \brief An example UDP Time application
 *  This application reads the current UTC time by sending a packet to
 *  utcnist.colorado.edu (128.138.140.44)
 *
 *  This example is implemented as a logic class (UDPGetTime) wrapping a UDP socket.
 *  The logic class handles all events, leaving the main loop to just check if the process
 *  has finished.
 */
#include "mbed.h"
#include "EthernetInterface.h"
#include "mbed-net-sockets/UDPSocket.h"
#include "test_env.h"
#include "minar/minar.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* TODO: Remove when yotta supports init. */
#include "lwipv4_init.h"

#define UDP_TIME_PORT 37

namespace {
     const char *HTTP_SERVER_NAME = "utcnist.colorado.edu";
     /*const char *HTTP_SERVER_NAME = "128.138.140.44";*/
     const float YEARS_TO_PASS = 115.0;
}

using namespace mbed::Sockets::v0;

#define ief_check()
    //printf("%s:%d Interrupts are %s\r\n",__FILE__,__LINE__,(__get_PRIMASK()?"Disabled":"Enabled"))

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
     */
    void startGetTime(const char *address) {
        /* Initialize the GetTime flags */
        resolved = false;
        printf("Starting DNS Query for %s\r\n", address);
        /* Start the DNS operation */
        socket_error_t rc = sock.resolve(address,Socket::DNSHandler_t(this, &UDPGetTime::onDNS));
        sock.error_check(rc);
    }
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
    void onDNS(Socket *s, struct socket_addr sa, const char *domain) {
        (void) s;
        char buf[32];
        printf("DNS Response Received:\r\n");
        /* Extract the Socket event to read the resolved address */
        _resolvedAddr.setAddr(&sa);
        _resolvedAddr.fmtIPv4(buf, 32);
        printf("%s = %s\r\n", domain, buf);

        /* Register the read handler */
        sock.setOnReadable(Socket::ReadableHandler_t(this, &UDPGetTime::onRecv));
        /* Send the query packet to the remote host */
        printf("Sending \"time\" to %s:%d\r\n", buf, (int)_udpTimePort);
        socket_error_t err = sock.send_to("time",strlen("time"), &_resolvedAddr, _udpTimePort);
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
    void onRecv(Socket *s) {
        (void) s;
        printf("Data Available!\r\n");
        /* Initialize the buffer size */
        size_t nRx = sizeof(_rxBuf);
        /* Receive some bytes */
        socket_error_t err = s->recv(_rxBuf, &nRx);
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

        printf("UDP: %lu seconds since 01/01/1900 00:00 GMT\r\n", _time);
        float years = (float) _time / 60 / 60 / 24 / 365;
        printf("{{%s}}\r\n",(years < YEARS_TO_PASS ?"failure":"success"));
        printf("{{end}}\r\n");
    }

protected:
    UDPSocket sock;
    SocketAddr _resolvedAddr;
    volatile bool resolved;
    const uint16_t _udpTimePort;
    volatile uint32_t _time;

protected:
    char _rxBuf[32];
};

EthernetInterface eth;
UDPGetTime *gt;
void app_start(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    static Serial pc(USBTX, USBRX);
    pc.baud(115200);

    printf("{{start}}\r\n");
    /* Initialise with DHCP, connect, and start up the stack */
    eth.init();
    eth.connect();
    lwipv4_socket_init();

    printf("UDP client IP Address is %s\r\n", eth.getIPAddress());

    /* Get the current time */
    gt = new UDPGetTime();
    FunctionPointer1<void, const char*> fp(gt, &UDPGetTime::startGetTime);
    minar::Scheduler::postCallback(fp.bind(HTTP_SERVER_NAME));
}
