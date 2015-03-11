#include "mbed.h"
#include "EthernetInterface.h"
#include "TCPStream.h"
#include "test_env.h"
// TODO: Remove when yotta supports init.
#include "lwipv4_init.h"

namespace {
const char *HTTP_SERVER_NAME = "developer.mbed.org";
//    const char *HTTP_SERVER_NAME = "google.com";
const int HTTP_SERVER_PORT = 80;
const int RECV_BUFFER_SIZE = 600;

const char HTTP_GET_CMD[] = "GET ";
const size_t HTTP_GET_CMD_SIZE = sizeof(HTTP_GET_CMD) - 1;

const char HTTP_PATH[] = "/media/uploads/mbed_official/hello.txt";
const size_t HTTP_PATH_LEN = sizeof(HTTP_PATH) - 1;

const char HTTP_GET_VER[] = " HTTP/1.0\n\n";
const size_t HTTP_GET_VER_SIZE = sizeof(HTTP_GET_VER) - 1;

// Test related data
const char *HTTP_OK_STR = "200 OK";
const char *HTTP_HELLO_STR = "Hello world!";
}

class HelloHTTP {
public:
	HelloHTTP(const char * domain, const uint16_t port,
			const socket_stack_t stack, uint16_t localPort = 0) :
			_stream(stack), _domain(domain), _port(port), _connect(this), _receive(
					this), _resolved(this) {
		_connect.callback(&HelloHTTP::onConnect);
		_receive.callback(&HelloHTTP::onReceive);
		_resolved.callback(&HelloHTTP::onDNS);

		_error = false;
		_gothello = false;
		_got200 = false;
		_bpos = 0;
		_stream.open(SOCKET_AF_INET4);
		_stream.bind("0.0.0.0",localPort);
	}
	~HelloHTTP() {

	}
	int startTest(const char *path) {
		// Initialize the flags
		_got200 = false;
		_gothello = false;
		_error = false;
		// Fill the request buffer
		_bpos = snprintf(_buffer, sizeof(_buffer) - 1,
				"GET %s HTTP/1.1\nHost: %s\n\n", path, HTTP_SERVER_NAME);

		// Connect to the server
		printf("Connecting to %s:%d\n", _domain, _port);
		// Resolve the domain name:
		socket_error_t err = _stream.resolve(_domain,
				(handler_t) (void *) _resolved.entry());
		return err;
	}
	bool done() {
		return _error || (_got200 && _gothello);
	}
	int error() {
		return _error;
	}
	void close() {
		_stream.close();
	}
protected:
	void onConnect() {
		//Send the request
		handler_t h = (handler_t) _receive.entry(); // TODO: CThunk replacement/Alpha3
		_stream.setOnReadable(h);
		socket_error_t rc = _stream.send(_buffer, _bpos); // No onSent needed
		if (rc != SOCKET_ERROR_NONE) {
			_error = true;
		}
	}
	void onReceive(void * arg) {
		(void) arg;
		_bpos = sizeof(_buffer);
		socket_error_t err = _stream.recv(_buffer, &_bpos);
		if (err != SOCKET_ERROR_NONE) {
			_error = true;
			return;
		}
		_buffer[_bpos] = 0;
		_got200 = _got200 || strstr(_buffer, HTTP_OK_STR) != NULL;
		_gothello = _gothello || strstr(_buffer, HTTP_HELLO_STR) != NULL;
		printf("HTTP: Received %d chars from server\r\n", _bpos);
		printf("HTTP: Received 200 OK status ... %s\r\n",
				_got200 ? "[OK]" : "[FAIL]");
		printf("HTTP: Received '%s' status ... %s\r\n", HTTP_HELLO_STR,
				_gothello ? "[OK]" : "[FAIL]");
		printf("HTTP: Received message:\r\n\r\n");
		printf("%s", _buffer);
	}
	void onDNS() {
		handler_t h = (handler_t) _connect.entry(); // TODO: CThunk replacement/Alpha3
		socket_event_t *e = _stream.getEvent();
		if (e->i.d.addr.type == SOCKET_STACK_UNINIT) {
			//Could not find DNS entry
			_error = true;
			printf("Could not find DNS entry for %s", HTTP_SERVER_NAME);
			return;
		} else {
			_remoteAddr.setAddr(&e->i.d.addr);

			int rc = _stream.connect(&_remoteAddr, _port, h);
			(void) rc;
			//TODO: Error checking for connect
		}
	}

protected:
	TCPStream _stream;
	const char *_domain;
	const uint16_t _port;
	char _buffer[RECV_BUFFER_SIZE];
	size_t _bpos;

	SocketAddr _remoteAddr;

	bool _got200;bool _gothello;bool _error;
protected:
	//TODO: Remove this section using std::function (Alpha 3)
	/* <EventHandler:toRemove> */
	CThunk<HelloHTTP> _connect, _receive, _resolved;
	/* </EventHandler:toRemove> */
};

int main() {
	EthernetInterface eth;
	Timer tp;
	tp.start();
	eth.init(); //Use DHCP
	eth.connect();
	tp.stop();
	uint16_t localPort = (tp.read_us() % 32768) + 32768;
	printf("TCP client IP Address is %s\n", eth.getIPAddress());

	// TODO: Remove when yotta supports init
	if (lwipv4_socket_init() != SOCKET_ERROR_NONE) {
		notify_completion(false);
	}

	HelloHTTP hello(HTTP_SERVER_NAME, HTTP_SERVER_PORT, SOCKET_STACK_LWIP_IPV4,
			localPort);
	int rc = hello.startTest(HTTP_PATH);
	if ((rc == SOCKET_ERROR_BUSY) || (rc == SOCKET_ERROR_NONE)) {
		while (!hello.done()) {
			__WFI();
		}
	}
	hello.close();
	eth.disconnect();
	notify_completion(!hello.error());
	return 0;
}
extern "C" void HardFault_Handler() {
	asm ("bkpt"::);
}
;
