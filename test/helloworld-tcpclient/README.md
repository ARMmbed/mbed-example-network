# HTTP File Downloader (TCP Example)

This application downloads a file from an HTTP server (mbed.org) and looks for a specific string in that file.

This example is implemented as a logic class (HelloHTTP) wrapping a TCP socket. The logic class handles all events, leaving the main loop to just check if the process has finished.

## Pre-requisites

To build and run this example the following requirements are necessary:

* A computer with the following software installed:
	* [CMake](http://www.cmake.org/download/).
	* [yotta](https://github.com/ARMmbed/yotta). Please note that **yotta has its own set of dependencies**, listed in the [installation instructions](http://armmbed.github.io/yotta/#installing-on-windows).
	* [Python](https://www.python.org/downloads/).
	* [ARM GCC toolchain](https://launchpad.net/gcc-arm-embedded).
	* A serial terminal emulator (e.g. screen, pySerial, cu).
	* Optionally, for debugging, pyOCD (can be installed using Python's [pip](https://pypi.python.org/pypi/pip)). 
* An FRDM-K64F development board.
* An Ethernet connection to the internet.
* An Ethernet cable.
* A micro-USB cable.
* If your OS is Windows, please follow the installation instructions [for the serial port driver](https://developer.mbed.org/handbook/Windows-serial-configuration).

## Getting started

1. Connect the FRDM-K64F to the internet using the Ethernet cable.

2. Connect the FRDM-K64F to the computer with the micro-USB cable, being careful to use the micro-USB port labeled "OpenSDA".

4. Navigate to the root mbed-example-network directory that came with your release and open a terminal.

5. Set the yotta target:
	
	```
	yotta target frdm-k64f-gcc
	```

6. Check that there are no missing dependencies:

    ```
    $ yt ls
    ```
	
	If there are, yotta will list them in the terminal. Please install them before proceeding.

7. Build the examples. This will take a long time if it is the first time that the examples have been built:

    ```
    $ yt build
    ```

8. Copy `build/frdm-k64f-gcc/test/mbed-example-network-test-helloworld-tcpclient.bin` to your mbed board and wait until the LED next to the USB port stops blinking.

9. Start the serial terminal emulator and connect to the virtual serial port presented by FRDM-K64F. For settings, use 9600 baud, 8N1, no flow control.

10. Press the reset button on the board.

11. The output in the terminal window should look like:

    ```
    TCP client IP Address is 192.168.2.2
    Connecting to developer.mbed.org:80
    HTTP: Received 552 chars from server
    HTTP: Received 200 OK status ... [OK]
    HTTP: Received 'Hello world!' status ... [OK]
    HTTP: Received message:

    HTTP/1.1 200 OK
    Server: nginx/1.4.6 (Ubuntu)
    Date: Fri, 13 Mar 2015 14:17:36 GMT
    Content-Type: text/plain
    Content-Length: 14
    Connection: keep-alive
    Set-Cookie: route=4f2fe48bfdc4a79550bab031c790587a; Path=/compiler
    Last-Modified: Fri, 27 Jul 2012 13:30:34 GMT
    Accept-Ranges: bytes
    Cache-Control: max-age=36000
    Expires: Sat, 14 Mar 2015 00:17:36 GMT
    X-Upstream-L3: 217.140.101.28:14101
    X-Upstream-L2: sjc_production_router_dock0-prod-sjc
    X-Upstream-L2-pre: 217.140.101.28:14100
    X-Upstream-L1: primaryrouter_dock0-prod-sjc

    Hello world!
    ```
## Using a debugger

Optionally, connect using a debugger to set breakpoints and follow program flow. Proceed normally up to and including step 6 (building the example), then:

1. Open a new terminal window, then start the pyOCD GDB server.

    ```
    $ python pyOCD/test/gdb_server.py
    ```

    The output should look like this:

    ```
    Welcome to the PyOCD GDB Server Beta Version
    INFO:root:new board id detected: 02400201B1130E4E4CXXXXXX
    id => usbinfo | boardname
    0 => MB MBED CM (0xd28, 0x204) [k64f]
    INFO:root:DAP SWD MODE initialised
    INFO:root:IDCODE: 0x2BA01477
    INFO:root:K64F not in secure state
    INFO:root:6 hardware breakpoints, 4 literal comparators
    INFO:root:4 hardware watchpoints
    INFO:root:CPU core is Cortex-M4
    INFO:root:FPU present
    INFO:root:GDB server started at port:3333
    ```

2. Open a new terminal window, go to the root directory of your copy of mbed-network-examples, then start GDB and connect to the GDB server.

    ```
    $ arm-none-eabi-gdb -ex "target remote localhost:3333" -ex load ./build/frdm-k64f-gcc/test/mbed-example-network-test-helloworld-tcpclient
    ```

3. In a third terminal window, start the serial terminal emulator and connect to the virtual serial port presented by FRDM-K64F.

4. Once the program has loaded, start it.

    ```
    (gdb) c
    ```

5. The output in the terminal window should look like in step 10 above (the final step of the regular run).
