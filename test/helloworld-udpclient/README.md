# UDP Time Example

This application reads the current UTC time by sending a packet to utcnist.colorado.edu (128.138.140.44).

This example is implemented as a logic class (UDPGetTime) wrapping a UDP socket. The logic class handles all events, leaving the main loop to just check if the process has finished.

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

3. Navigate to the root mbed-example-network directory that came with your release and open a terminal.

4. Set the yotta target:
	
	```
	yotta target frdm-k64f-gcc
	```

5. Build the examples. This will take a long time if it is the first time that the examples have been built:

    ```
    $ yt build
    ```

6. Copy `build/frdm-k64f-gcc/test/mbed-example-network-test-helloworld-udpclient.bin` to your mbed board and wait until the LED next to the USB port stops blinking.

7. Start the serial terminal emulator and connect to the virtual serial port presented by FRDM-K64F. For settings, use 9600 baud, 8N1, no flow control.

8. Press the reset button on the board.

9. The output in the terminal window should look like:

    ```
    UDP client IP Address is 192.168.2.2
    UDP: 3635245075 seconds since 01/01/1900 00:00 GMT
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
    $ arm-none-eabi-gdb -ex "target remote localhost:3333" -ex load ./build/frdm-k64f-gcc/test/mbed-example-network-test-helloworld-udpclient
    ```

3. In a third terminal window, start the serial terminal emulator and connect to the virtual serial port presented by FRDM-K64F.

4. Once the program has loaded, start it.

    ```
    (gdb) c
    ```

5. The output in the terminal window should look like in step 10 above (the final step of the regular run).
