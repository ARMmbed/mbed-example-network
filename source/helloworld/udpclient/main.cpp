#include "mbed.h"
#include "EthernetInterface.h"
#include "test_env.h"

#include "socket_types_impl.h"
#include "socket_buffer.h"
#include "UDPaSocket.h"

#define UDP_TIME_PORT 37

namespace {
     //const char *HTTP_SERVER_NAME = "utcnist.colorado.edu";
     const char *HTTP_SERVER_NAME = "128.138.140.44";
     const float YEARS_TO_PASS = 114.0;
}

using std::memset;
using std::memcpy;

class UDPGetTime {
public:
    UDPGetTime() :
                    sock((handler_t) _default_irq.entry()),
                    _udpTimePort(UDP_TIME_PORT),
                    _default_irq(this),
                    _recv_irq(this),
                    _send_irq(this)
    {
        strcpy(txBuf,"foo");
        inBuf.buffer = rxBuf;
        inBuf.length = sizeof(rxBuf);
        inBuf.pos    = 0;
        outBuf.buffer = txBuf;
        outBuf.length = sizeof(txBuf);
        outBuf.pos    = 0;

        _default_irq.callback(&UDPGetTime::defaultHandler);
        _send_irq.callback(&UDPGetTime::sendHandler);
        _recv_irq.callback(&UDPGetTime::recvHandler);

    }
    uint32_t getTime(const char *address) {
        ip_addr_t addr;
        socket_error_t err;
        // TODO: add abstracted address functions
        if (!ipaddr_aton(HTTP_SERVER_NAME,&addr)) {
            defaultHandler(0);
            return 0;
        }

        received = false;
        // Zero the buffer memory
        memset(inBuf.buffer,0,inBuf.length);
        handler_t rxh = (handler_t)_recv_irq.entry();
        printf("starting receive...\r\n");
        err = sock.start_recv(&inBuf, 0, rxh);
        if(err != SOCKET_ERROR_NONE) {
            printf("RX Socket Error %d\r\n",err);
            return 0;
        }

        handler_t txh = (handler_t)_send_irq.entry();
        printf("starting send...\r\n");
        sock.start_send_to((struct socket_addr *)&addr, _udpTimePort, &outBuf, 0, txh);
        if(err != SOCKET_ERROR_NONE) {
            printf("TX Socket Error %d\r\n",err);
            return 0;
        }

        while(!received) { __WFI(); }

        uint32_t time;
        // Correct for possible non 32-bit alignment
        memcpy(&time, rxBuf, sizeof(time));
        // Switch to host order
        time = ntohl(time);
        return time;
    }
protected:
    void defaultHandler(void* arg) {
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
        printf("message sent\r\n");
    }
    void recvHandler(void *arg) {
        socket_event_t *event = sock.getEvent(); // TODO: (CThunk upgrade/Alpha2)
        socket_copy_to_user(rxBuf,event->i.r.buf,sizeof(rxBuf));
        received = true;
    }
protected:
    UDPaSocket sock;
    volatile bool received;
    const uint16_t _udpTimePort;
protected:
    buffer_t inBuf;
    buffer_t outBuf;
    char rxBuf[32];
    char txBuf[32];

    // These should not be CThunk.  We will remove CThunk from this class once
    // we decide on the function pointer format we will use for event handlers
    CThunk<UDPGetTime> _default_irq;
    CThunk<UDPGetTime> _recv_irq;
    CThunk<UDPGetTime> _send_irq;
};

int main() {
    bool result = 0;
    EthernetInterface eth;
    printf("Starting.\r\n");
    eth.init(); //Use DHCP
    printf("Running eth.connect()\r\n");
    eth.connect();
    printf("UDP client IP Address is %s\r\n", eth.getIPAddress());
    UDPGetTime gt;
    printf("Connecting to: %s:%d\r\n", HTTP_SERVER_NAME, UDP_TIME_PORT);
    uint32_t timeRes = gt.getTime(HTTP_SERVER_NAME);


    const float years = timeRes / 60.0 / 60.0 / 24.0 / 365;
    //printf("UDP: Received %d bytes from server %s on port %d\r\n", n, remote.get_address(), remote.get_port());
    printf("UDP: %lu seconds since 01/01/1900 00:00 GMT ... %s\r\n", timeRes, timeRes > 0 ? "[OK]" : "[FAIL]");
    printf("UDP: %.2f years since 01/01/1900 00:00 GMT ... %s\r\n", years, timeRes > YEARS_TO_PASS ? "[OK]" : "[FAIL]");

    eth.disconnect();
    notify_completion(result);
    return 0;
}
