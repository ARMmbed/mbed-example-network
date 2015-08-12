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
#include "mbed.h"
#include "EthernetInterface.h"
#include "mbed-net-sockets/UDPSocket.h"
#include "mbed-net-lwip/lwipv4_init.h"
#include "mbed-net-socket-abstract/socket_api.h"
#include "minar/minar.h"

namespace {
    const int ECHO_SERVER_PORT = 7;
    const int BUFFER_SIZE = 512;
}
using namespace mbed::Sockets::v0;

char buffer[BUFFER_SIZE] = {0};

void onError(Socket *s, socket_error_t err) {
    (void) s;
    printf("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
    minar::Scheduler::stop();
}
void onRx(Socket *s) {
    SocketAddr addr;
    uint16_t port;
    size_t len = BUFFER_SIZE-1;
    /* Recieve the packet */
    socket_error_t err = s->recv_from(buffer, &len, &addr, &port);
    if (!s->error_check(err) && len) {
        buffer[len] = 0;
        /* Send the packet */
        err = s->send_to(buffer,len,&addr,port);
        printf("MBED: Received message: %s\r\n",buffer);
        if (err != SOCKET_ERROR_NONE) {
            onError(s, err);
        }
    }
}
/* Python Test Script
 *
#!/usr/bin/python
import socket

#IMPORTANT: Set the address of your mbed below:
DUTAddress = '192.168.2.2'
DUTPort = 7

def testEcho(addr,port):
     s = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
     ok = True
     for x in range(100):
             tso = "Test%i!"%x
             s.sendto(tso, (addr, port))
             tsr = s.recv(100)
             if tso != tsr:
                 print "[FAIL] expected %s, got %s"%(tso.tsr)
                 ok = False
     return ok

if __name__ == '__main__':
    if testEcho(DUTAddress, port):
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
    UDPSocket udpserver(SOCKET_STACK_LWIP_IPV4);

    printf("MBED: UDP Server IP Address is %s:%d\r\n", eth.getIPAddress(), ECHO_SERVER_PORT);

    udpserver.setOnError(UDPSocket::ErrorHandler_t(onError));
    udpserver.open(SOCKET_AF_INET4);
    err = udpserver.bind("0.0.0.0",ECHO_SERVER_PORT);
    udpserver.error_check(err);
    udpserver.setOnReadable(UDPSocket::ReadableHandler_t(onRx));

    printf("MBED: Waiting for packet...\r\n");
    minar::Scheduler::start();
    return 1;

}
