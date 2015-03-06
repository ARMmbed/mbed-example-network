/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright 2015 ARM Holdings PLC
 */
#include "mbed.h"
#include "UDPaSocket.h"
#include "socket_types.h"
#include "EthernetInterface.h"
#include "test_env.h"

// TODO: Remove when yotta supports init.
#include "picotcp_init.h"

#include "socket_buffer.h"

#define UDP_TIME_PORT 37

namespace {
     //const char *HTTP_SERVER_NAME = "utcnist.colorado.edu";
     const char *HTTP_SERVER_NAME = "128.138.140.44";
     const float YEARS_TO_PASS = 114.0;
}

using std::memset;
using std::memcpy;

void * socket_malloc( void * context, size_t size) {
    (void) context;
    return malloc(size);
}
void socket_free(void * context, void * ptr) {
    (void) context;
    free(ptr);
}


class UDPGetTime {
public:
    UDPGetTime(const socket_stack_t stack) :
        sock((handler_t) _default_irq.entry(), stack, &_alloc),
        _udpTimePort(UDP_TIME_PORT),
        _default_irq(this),
        _recv_irq(this),
        _send_irq(this),
        _dns_irq(this)
        {
        strcpy(_txBuf,"foo");
        _alloc.alloc = socket_malloc;
        _alloc.dealloc = socket_free;
        _alloc.context = NULL;
        _default_irq.callback(&UDPGetTime::defaultHandler);
        _send_irq.callback(&UDPGetTime::sendHandler);
        _recv_irq.callback(&UDPGetTime::recvHandler);
        _dns_irq.callback(&UDPGetTime::onDNS);

    }
    uint32_t getTime(const char *address) {
        socket_error_t err;
        // TODO: add abstracted address functions
        resolved = false;
        memset(&_resolvedAddr,0,sizeof(_resolvedAddr));
        err = sock.resolve(address,(handler_t)_dns_irq.entry());
        if (err == SOCKET_ERROR_BUSY) {
            while (!resolved) {__WFI();}
        } else if (err != SOCKET_ERROR_NONE) {
            defaultHandler(NULL);
        }

        received = false;
        // Zero the buffer memory
        memset(_rxBuf,0,sizeof(_rxBuf));
        handler_t rxh = (handler_t)_recv_irq.entry();
        printf("starting receive...\r\n");
        sock.onRecv(rxh);

        handler_t txh = (handler_t)_send_irq.entry();
        printf("starting send...\r\n");
        err = sock.send_to(&_resolvedAddr, _udpTimePort, _txBuf, strlen(_txBuf), txh);
        if(err != SOCKET_ERROR_NONE) {
            printf("TX Socket Error %d\r\n",err);
            return 0;
        }

        while(!received) { __WFI(); }

        uint32_t time;
        // Correct for possible non 32-bit alignment
        memcpy(&time, _rxBuf, sizeof(time));
        // Switch to host order
        time = ntohl(time);
        return time;
    }
protected:
    void defaultHandler(void* arg) {
        (void) arg;
        const char * str;
        socket_event_t *event = sock.getEvent(); // TODO: (CThunk upgrade/Alpha2)
        printf("Default Event Handler\r\n");
        switch(event->event) {
        case SOCKET_EVENT_ERROR:
            str = "error";
            break;
        case SOCKET_EVENT_NONE:
            str = "no event";
            break;
        case SOCKET_EVENT_RX_DONE:
            str = "rx done";
            break;
        case SOCKET_EVENT_RX_ERROR:
            str = "rx error";
            break;
        case SOCKET_EVENT_TX_DONE:
            str = "tx done";
            break;
        case SOCKET_EVENT_TX_ERROR:
            str = "tx error";
            break;
        default:
            str = "unknown event";
            break;
        }
        printf("Event type: %s\r\n", str);
    }
    void sendHandler(void *arg) {
        (void) arg;
        printf("message sent\r\n");
    }
    void recvHandler(void *arg) {
        (void) arg;
        sock.recv(_rxBuf,sizeof(_rxBuf));
        received = true;
    }
    void onDNS(void * arg) {
        (void) arg;
        socket_event_t *event = sock.getEvent(); // TODO: (CThunk upgrade/Alpha2)
        _resolvedAddr.setAddr(&event->i.d.addr);
        resolved=true;
    }

protected:
    UDPaSocket sock;
    SocketAddr _resolvedAddr;
    volatile bool received;
    volatile bool resolved;
    const uint16_t _udpTimePort;

    socket_allocator_t _alloc;
protected:
    SocketBuffer _sbRX;
    char _rxBuf[32];
    char _txBuf[32];

    // These should not be CThunk.  We will remove CThunk from this class once
    // we decide on the function pointer format we will use for event handlers
    CThunk<UDPGetTime> _default_irq;
    CThunk<UDPGetTime> _recv_irq;
    CThunk<UDPGetTime> _send_irq;
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
    uint32_t timeRes = gt.getTime(HTTP_SERVER_NAME);


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
