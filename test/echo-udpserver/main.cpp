#include "mbed.h"
#include "EthernetInterface.h"
#include <mbed-net-sockets/UDPSocket.h>
#include <mbed-net-lwip/lwipv4_init.h>

namespace {
    const int ECHO_SERVER_PORT = 7;
    const int BUFFER_SIZE = 512;
}


char buffer[BUFFER_SIZE] = {0};
mbed::UDPSocket * server = NULL;

void onRx(socket_error_t err) {
    if (err != SOCKET_ERROR_NONE) {
        return;
    }
    mbed::SocketAddr addr;
    uint16_t port;
    size_t len = BUFFER_SIZE-1;
    err = server->recv_from(buffer, &len, &addr, &port);
    if (err != SOCKET_ERROR_NONE)
    {
        return;
    }
    if (len) {
        buffer[len] = 0;
        server->send_to(buffer,len,&addr,port);
        printf("MBED: Received message: %s\r\n",buffer);
    }
}
/* Python Test Script
 *
#!/usr/bin/python
import socket

#IMPORTANT: Set the address of your mbed below:
DUTAddress = '192.168.2.2'

def testEcho(addr):
     s = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
     ok = True
     for x in range(100):
             tso = "Test%i!"%x
             s.sendto(tso,(addr,7))
             tsr = s.recv(100)
             if tso != tsr:
                 print "[FAIL] expected %s, got %s"%(tso.tsr)
                 ok = False
     return ok

if __name__ == '__main__':
    if testEcho(DUTAddress):
        print '[PASS]'
    else:
        print '[FAIL]'
 */
int main (void) {
    EthernetInterface eth;
    eth.init(); //Use DHCP
    eth.connect();

    socket_error_t err = lwipv4_socket_init();
    if (err) {
        printf("MBED: Failed to initialize socket stack (%d)\r\n", err);
        return 1;
    }
    mbed::UDPSocket udpserver(SOCKET_STACK_LWIP_IPV4);
    server = &udpserver;
    printf("MBED: Server IP Address is %s:%d\r\n", eth.getIPAddress(), ECHO_SERVER_PORT);

    server->bind("0.0.0.0",ECHO_SERVER_PORT);
    server->setOnReadable(handler_t(onRx));

    printf("MBED: Waiting for packet...\r\n");
    while (true) {
        __WFI();
    }
}
