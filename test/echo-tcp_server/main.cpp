#include "mbed.h"
#include "EthernetInterface.h"
#include <mbed-net-sockets/TCPListener.h>
#include <mbed-net-socket-abstract/socket_api.h>
#include <Timer.h>
#include <mbed-net-lwip/lwipv4_init.h>

namespace {
    const int ECHO_SERVER_PORT = 7;
    const int BUFFER_SIZE = 64;
}
class TCPEchoServer {
public:
    TCPEchoServer():
        _server(SOCKET_STACK_LWIP_IPV4), _stream(NULL),
        _disconnect_pending(false)
        {}
    socket_error_t start(uint16_t port) {

        socket_error_t err = _server.open(SOCKET_AF_INET4);
        if (err) {
            return err;
        }
        err = _server.bind("0.0.0.0",port);
        if (err) {
            return err;
        }
        err = _server.start_listening(handler_t(this, &TCPEchoServer::onIncoming));
        if (err) {
            return err;
        }
        return SOCKET_ERROR_NONE;
    }
    void checkStream()
    {
        if (_disconnect_pending) {
            if (_stream != NULL) {
                delete _stream;
            }
            _stream = NULL;
            _disconnect_pending = false;
        }
    }
protected:
    void onIncoming(socket_error_t err)
    {
        socket_event_t *event = _server.getEvent();
        if (err != SOCKET_ERROR_NONE || _stream != NULL) {
            event->i.a.reject = 1;
            return;
        }
        _stream = _server.accept(static_cast<struct socket *>(event->i.a.newimpl));
        if (_stream == NULL) {
            event->i.a.reject = 1;
            return;
        }
        _stream->setOnReadable(handler_t(this, &TCPEchoServer::onRX));
        _stream->setOnDisconnect(handler_t(this, &TCPEchoServer::onDisconnect));

    }
    void onRX(socket_error_t err) {
        if (err != SOCKET_ERROR_NONE) {
            _stream->close();
            return;
        }
        size_t size = BUFFER_SIZE-1;
        err = _stream->recv(buffer, &size);
        if (err != SOCKET_ERROR_NONE) {
            _stream->close();
            return;
        }
        buffer[size]=0;
        err = _stream->send(buffer,size);
        if (err != SOCKET_ERROR_NONE) {
            _stream->close();
        }
    }
    void onDisconnect(socket_error_t err) {
        (void) err;
        _disconnect_pending = true;
    }
protected:
    TCPListener _server;
    TCPStream*  _stream;
    volatile bool _disconnect_pending;
protected:
    char buffer[BUFFER_SIZE];
};

int main (void) {
    EthernetInterface eth;
    eth.init(); //Use DHCP
    eth.connect();
    lwipv4_socket_init();
    printf("MBED: Server IP Address is %s:%d\r\n", eth.getIPAddress(), ECHO_SERVER_PORT);

    TCPEchoServer server;
    socket_error_t err = server.start(ECHO_SERVER_PORT);
    if (err) {
        printf("Socket error: %s (%d)\n", socket_strerror(err), err);
    }

    while (true) {
        __WFI();
        server.checkStream();
    }
}
