#include "mbed.h"
#include "EthernetInterface.h"
#include "test_env.h"
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
            sock(_default_irq.entry() ), txBuf("foo"), _udpTimePort(UDP_TIME_PORT)
    {
        inBuf.buffer = rxBuf;
        inBuf.length = sizeof(rxBuf);
        inBuf.pos    = 0;
        outBuf.buffer = txBuf;
        outBuf.length = sizeof(txBuf);
        outBuf.pos    = 0;

        _default_irq.callback(&defaultHandler);
        _send_irq.callback(&sendHandler);
        _recv_irq.callback(&recvHandler);

    }
    uint32_t getTime(char *address) {
        ip_addr_t addr;
        if (!ipaddr_aton(HTTP_SERVER_NAME,addr)) {
            defaultHandler(0);
            return 0;
        }

        received = false;
        // Zero the buffer memory
        memset(inBuf.buffer,0,inBuf.length);
        sock.start_recv(inBuf, 0, _recv_irq.entry());
        sock.start_send_to(addr,  outBuf, 0, _send_irq.entry());
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
        printf("Caught a default event\n");
    }
    void sendHandler(void *arg) {
        printf("message sent\n");
    }
    void recvHandler(void *arg) {
        received = true;
    }
protected:
    UDPaSocket sock;
    volatile bool received;
    const uint16_t _udpTimePort;
protected:
    buffer_t inBuf;
    buffer_t outBuf;
    volatile char rxBuf[32];
    char txBuf[32];
    CThunk<UDPGetTime> _default_irq;
    CThunk<UDPGetTime> _send_irq;
    CThunk<UDPGetTime> _recv_irq;
};

int main() {
    EthernetInterface eth;
    printf("Starting.\r\n");
    eth.init(); //Use DHCP
    eth.connect();
    printf("UDP client IP Address is %s\n", eth.getIPAddress());
    UDPGetTime gt;
    printf("Connecting to: %s:%d\n", HTTP_SERVER_NAME, UDP_TIME_PORT);
    uint32_t timeRes = gt.getTime(HTTP_SERVER_NAME);


    const float years = timeRes / 60.0 / 60.0 / 24.0 / 365;
    printf("UDP: Received %d bytes from server %s on port %d\r\n", n, remote.get_address(), remote.get_port());
    printf("UDP: %u seconds since 01/01/1900 00:00 GMT ... %s\r\n", timeRes, timeRes > 0 ? "[OK]" : "[FAIL]");
    printf("UDP: %.2f years since 01/01/1900 00:00 GMT ... %s\r\n", years, timeRes > YEARS_TO_PASS ? "[OK]" : "[FAIL]");

    eth.disconnect();
    notify_completion(result);
    return 0;
}
