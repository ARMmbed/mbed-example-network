/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright 2015 ARM Holdings PLC
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
#include "socket_types.h"
#include "UDPaSocket.h"
#include "test_env.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// TODO: Remove when yotta supports init.
#include "lwipv4_init.h"

#define UDP_TIME_PORT 37

namespace {
     //const char *HTTP_SERVER_NAME = "utcnist.colorado.edu";
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
	 *
	 * @param[in] stack The stack to use for the UDP Socket.  Currently,
	 * only SOCKET_STACK_LWIP_IPV4 is supported
	 */
    UDPGetTime(const socket_stack_t stack) :
        sock(stack),
        _udpTimePort(UDP_TIME_PORT),
		_time(0),
        _recv_irq(this),
        _dns_irq(this)
	{
    	/* Initialize callbacks for events */
        _recv_irq.callback(&UDPGetTime::onRecv);
        _dns_irq.callback(&UDPGetTime::onDNS);
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
        socket_error_t err;
        /* Initialize the GetTime flags */
        resolved = false;
        received = false;
        /* Start the DNS operation */
        return sock.resolve(address,(handler_t)_dns_irq.entry());
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
    uint32_t getTime() { return _time;}
protected:
    /**
     * The DNS Response Handler
     * @param[in] arg (unused)
     */
    void onDNS(void * arg) {
        (void) arg;
        /* Extract the Socket event to read the resolved address */
        socket_event_t *event = sock.getEvent(); // TODO: (CThunk upgrade/Alpha2)
        _resolvedAddr.setAddr(&event->i.d.addr);
        /* Register the read handler */
        sock.setOnReadable((handler_t)_recv_irq.entry());
        /* Send the query packet to the remote host */
        socket_error_t err = sock.send_to("time",strlen("time"),&_resolvedAddr,_udpTimePort);
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
    void onRecv(void *arg) {
    	(void) arg;
    	/* Initialize the buffer size */
    	size_t nRx = sizeof(_rxBuf);
    	/* Receive some bytes */
    	socket_error_t err = sock.recv(_rxBuf, &nRx);
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
    UDPaSocket sock;
    SocketAddr _resolvedAddr;
    volatile bool received;
    volatile bool resolved;
    const uint16_t _udpTimePort;
    uint32_t _time;

protected:
    char _rxBuf[32];
    /*
	 * Note that CThunk is used for event handlers.  This will be changed to a C++
	 * function pointer in an upcoming release.
	 */
    CThunk<UDPGetTime> _recv_irq;
    CThunk<UDPGetTime> _dns_irq;
};

int main() {
    bool result = 0;
    EthernetInterface eth;
    printf("Starting.\r\n");
    eth.init(); //Use DHCP
    printf("Running eth.connect()\r\n");
    eth.connect();

    // TODO: Remove when yotta supports init
    lwipv4_socket_init();

    printf("UDP client IP Address is %s\r\n", eth.getIPAddress());
    UDPGetTime gt(SOCKET_STACK_LWIP_IPV4);
    printf("Connecting to: %s:%d\r\n", HTTP_SERVER_NAME, UDP_TIME_PORT);
    gt.startGetTime(HTTP_SERVER_NAME);

    /* Wait until the get time process has completed */
    while (!gt.isReceived()) { __WFI(); }

    /* Get the time */
    uint32_t timeRes = gt.getTime();

    /* Check that the response is sane */
    const float years = timeRes / 60.0 / 60.0 / 24.0 / 365;
    //printf("UDP: Received %d bytes from server %s on port %d\r\n", n, remote.get_address(), remote.get_port());
    printf("UDP: %lu seconds since 01/01/1900 00:00 GMT ... %s\r\n", timeRes, timeRes > 0 ? "[OK]" : "[FAIL]");
    printf("UDP: %.2f years since 01/01/1900 00:00 GMT ... %s\r\n", years, timeRes > YEARS_TO_PASS ? "[OK]" : "[FAIL]");

    result = (timeRes > 0) && (timeRes > YEARS_TO_PASS);
    eth.disconnect();
    notify_completion(result);
    return 0;
}
extern "C" void HardFault_Handler() {
    asm ("bkpt"::);
};
