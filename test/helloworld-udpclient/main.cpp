/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright 2015 ARM Holdings PLC
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

class UDPGetTime {
public:
    UDPGetTime(const socket_stack_t stack) :
        sock(stack),
        _udpTimePort(UDP_TIME_PORT),
		_time(0),
        _recv_irq(this),
        _dns_irq(this)
	{
        _recv_irq.callback(&UDPGetTime::onRecv);
        _dns_irq.callback(&UDPGetTime::onDNS);
        sock.open(SOCKET_AF_INET4);
    }
    socket_error_t startGetTime(const char *address) {
        socket_error_t err;
        resolved = false;
        received = false;
        memset(&_resolvedAddr,0,sizeof(_resolvedAddr));
        err = sock.resolve(address,(handler_t)_dns_irq.entry());
		return err;
    }
    bool isReceived() {return received;}
    uint32_t getTime() { return _time;}
protected:
    void onDNS(void * arg) {
        (void) arg;
        socket_event_t *event = sock.getEvent(); // TODO: (CThunk upgrade/Alpha2)
        _resolvedAddr.setAddr(&event->i.d.addr);
        sock.setOnReadable((handler_t)_recv_irq.entry());
        sock.send_to("foo",strlen("foo"),&_resolvedAddr,_udpTimePort);
        // TODO: Add a timeout?
    }
    void onRecv(void *arg) {
    	(void) arg;
    	size_t nRx = sizeof(_rxBuf);
    	socket_error_t err = sock.recv(_rxBuf, &nRx);
    	if (err != SOCKET_ERROR_NONE) {
    		printf("Socket Error %d\r\n", err);
    	    notify_completion(false);
    	}
        uint32_t time;
        // Correct for possible non 32-bit alignment
        memcpy(&time, _rxBuf, sizeof(time));
        // Switch to host order
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

    // These should not be CThunk.  We will remove CThunk from this class once
    // we decide on the function pointer format we will use for event handlers
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

    while (!gt.isReceived()) { __WFI(); }

    uint32_t timeRes = gt.getTime();

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
