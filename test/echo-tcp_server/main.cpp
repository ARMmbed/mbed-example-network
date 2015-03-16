#include "mbed.h"
#include "EthernetInterface.h"
#include <mbed-net-sockets/TCPListener.h>
#include <Timer.h>

namespace {
    const int ECHO_SERVER_PORT = 7;
    const int BUFFER_SIZE = 64;
}
class TCPEchoServer {
public:
    TCPEchoServer():
        _server(SOCKET_STACK_LWIP_IPV4), _stream(NULL) {}
    void start(uint16_t port) {
        _server.bind("0.0.0.0",port);
        _server.start_listening(handler_t(this, &TCPEchoServer::onIncoming));
    }
    void checkStream()
    {
        if (_stream != NULL) {
            if (!_stream->isConnected()) {
                delete _stream;
                _stream =  NULL;
            }
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
protected:
    TCPListener _server;
    TCPStream*  _stream;
protected:
    char buffer[BUFFER_SIZE];
};

int main (void) {
    EthernetInterface eth;
    eth.init(); //Use DHCP
    eth.connect();
    printf("MBED: Server IP Address is %s:%d\r\n", eth.getIPAddress(), ECHO_SERVER_PORT);

    TCPEchoServer server;
    server.start(ECHO_SERVER_PORT);

    while (true) {
        __WFI();
        server.checkStream();
    }
}
