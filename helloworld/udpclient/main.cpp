#include "mbed.h"
#include "EthernetInterface.h"
#include "test_env.h"

namespace {
     //const char *HTTP_SERVER_NAME = "utcnist.colorado.edu";
     const char *HTTP_SERVER_NAME = "128.138.140.44";
     const int HTTP_SERVER_PORT = 37;
     const float YEARS_TO_PASS = 114.0;
}

static volatile bool received = false;
static bool result = false;

void on_recv(const uint8_t* p, uint16_t n, Endpoint& remote) {
    union {
        char in_buffer_tab[4];
        unsigned int in_buffer_uint;
    };

    memcpy(in_buffer_tab, p, 4);

    result = true;
    const unsigned int timeRes = ntohl(in_buffer_uint);
    const float years = timeRes / 60.0 / 60.0 / 24.0 / 365;
    printf("UDP: Received %d bytes from server %s on port %d\r\n", n, remote.get_address(), remote.get_port());
    printf("UDP: %u seconds since 01/01/1900 00:00 GMT ... %s\r\n", timeRes, timeRes > 0 ? "[OK]" : "[FAIL]");
    printf("UDP: %.2f years since 01/01/1900 00:00 GMT ... %s\r\n", years, timeRes > YEARS_TO_PASS ? "[OK]" : "[FAIL]");
    received = true;
}

int main() {
    EthernetInterface eth;
    printf("Starting.\r\n");
    eth.init(); //Use DHCP
    eth.connect();
    printf("UDP client IP Address is %s\n", eth.getIPAddress());

    UDPSocket sock;
    sock.init();
    sock.setReceiveCallback(on_recv);

    Endpoint nist;
    nist.set_address(HTTP_SERVER_NAME, HTTP_SERVER_PORT);
    printf("Connecting to: %s\n", nist.get_address());
    char out_buffer[] = "plop"; // Does not matter
    sock.sendTo(nist, out_buffer, sizeof(out_buffer));
    while (!received);

    sock.close();
    eth.disconnect();
    notify_completion(result);
    return 0;
}
