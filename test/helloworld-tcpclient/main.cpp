#include "mbed.h"
#include "TCPStream.h"
#include "SocketBuffer.h"
#include "EthernetInterface.h"
#include "test_env.h"

namespace {
    const char *HTTP_SERVER_NAME = "http://developer.mbed.org";
    const int HTTP_SERVER_PORT = 80;
    const int RECV_BUFFER_SIZE = 512;

    const char HTTP_GET_CMD[]      =  "GET ";
    const size_t HTTP_GET_CMD_SIZE = sizeof(HTTP_GET_CMD) - 1;

    const char HTTP_PATH[]         = "/media/uploads/mbed_official/hello.txt";
    const size_t HTTP_PATH_LEN     = sizeof(HTTP_PATH) - 1;

    const char HTTP_GET_VER[]      =  " HTTP/1.0\n\n";
    const size_t HTTP_GET_VER_SIZE = sizeof(HTTP_GET_VER) - 1;

    // Test related data
    const char *HTTP_OK_STR = "200 OK";
    const char *HTTP_HELLO_STR = "Hello world!";
}

class HelloHTTP {
public:
    HelloHTTP(const char * domain, const uint16_t port) :
        _stream((handler_t)(void *)_default.entry()),
        _domain(domain), _port(port),
        _connect(this), _receive(this), _default(this), _resolved(this)
    {
        _connect.callback(&HelloHTTP::onConnect);
        _receive.callback(&HelloHTTP::onReceive);
        _default.callback(&HelloHTTP::defaultHandler);
        _resolved.callback(&HelloHTTP::onDNS);
    }
    ~HelloHTTP()
    {

    }
    int startTest(const char *path, const size_t pathlen)
    {
        // Initialize the flags
        _got200 = false;
        _gothello = false;
        _error = false;
        // Fill the request buffer
        char * ptr = _buffer;
        ptr = strcpy(ptr, "GET ");
        ptr = strncpy(ptr, path, pathlen);
        ptr = strcpy(ptr, " HTTP/1.0\n\n");
        _bpos = (uintptr_t) _buffer - (uintptr_t) ptr;
        // Connect to the server
        printf("Connecting to %s:%d\n", _domain, _port);
        // Resolve the domain name:
        socket_error_t err = _stream.resolve(_domain, &_remoteAddr, (handler_t)(void *)_resolved.entry());

        return err;
  }
  bool done( ) {
    return _error || (_got200 && _gothello);
  }
  int error() {
      return _error;
  }
protected:
  void onConnect() {
    //Send the request
    handler_t h = (handler_t)_receive.entry(); // TODO: CThunk replacement/Alpha3
    _stream.start_recv(h);
    h = NULL;
    _stream.start_send(_buffer, _bpos, h); // No onSent needed
  }
  void onReceive(void * arg) {
    (void) arg;

    SocketBuffer& sb = _stream.getRxBuf();
    _bpos = sb.copyOut(_buffer, 256);
    _buffer[_bpos] = 0;
    _got200 = _got200 || strstr(_buffer, HTTP_OK_STR) != NULL;
    _gothello = _gothello || strstr(_buffer, HTTP_HELLO_STR) != NULL;
    printf("HTTP: Received %d chars from server\r\n", _bpos);
    printf("HTTP: Received 200 OK status ... %s\r\n", _got200 ? "[OK]" : "[FAIL]");
    printf("HTTP: Received '%s' status ... %s\r\n", HTTP_HELLO_STR, _gothello ? "[OK]" : "[FAIL]");
    printf("HTTP: Received message:\r\n\r\n");
    printf("%s", _buffer);
  }
  void onDNS() {
      handler_t h = (handler_t)_connect.entry(); // TODO: CThunk replacement/Alpha3
      int rc = _stream.connect(&_remoteAddr, _port, h);
      (void) rc;
      //TODO: Error checking for connect
  }
  void defaultHandler() {
    printf("Error\n\n");
    _got200 = true;
    _gothello = true;
  }
protected:
  TCPStream _stream;
  const char *_domain;
  const uint16_t _port;
  char _buffer[256];
  size_t _bpos;

  SocketAddr _remoteAddr;

  bool _got200;
  bool _gothello;
  bool _error;
protected:
  //TODO: Remove this section using std::function (Alpha 3)
  CThunk<HelloHTTP> _connect, _receive, _default, _resolved;
};

int main() {
    EthernetInterface eth;
    eth.init(); //Use DHCP
    eth.connect();
    printf("TCP client IP Address is %s\n", eth.getIPAddress());

    HelloHTTP hello(HTTP_SERVER_NAME,HTTP_SERVER_PORT);
    int rc = hello.startTest(HTTP_PATH,HTTP_PATH_LEN);
    while(!rc && !hello.done()){__WFI();}
    // sock.close();
    // eth.disconnect();
    notify_completion(hello.error());
    return 0;
}
