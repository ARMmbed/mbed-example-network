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

9. Start the serial terminal emulator and connect to the virtual serial port presented by FRDM-K64F. For settings, use 115200 baud, 8N1, no flow control.

8. Press the reset button on the board.

9. The output in the terminal window should look like:

    ```
    UDP client IP Address is 192.168.2.2
    UDP: 3635245075 seconds since 01/01/1900 00:00 GMT
    ```

## Using a debugger

Optionally, connect using a debugger to set breakpoints and follow program flow. Proceed normally up to and including step 5 (building the example), then:

1. yotta debug mbed-example-network-test-helloworld-udpclient

    The output should look like this:

    ```
    info: found mbed-example-network-test-helloworld-udpclient at test/mbed-example-network-test-helloworld-udpclient
    info: preparing PyOCD gdbserver...
    info: finding connected board...
    info: new board id detected: ...
    info: board allows 5 concurrent packets
    info: DAP SWD MODE initialised
    info: IDCODE: ...
    info: K64F not in secure state
    info: 6 hardware breakpoints, 4 literal comparators
    info: CPU core is Cortex-M4
    info: FPU present
    info: 4 hardware watchpoints
    info: starting PyOCD gdbserver...
    info: Telnet: server started on port 4444
    info: GDB server started at port:3333
    GNU gdb (GNU Tools for ARM Embedded Processors) 7.6.0.20131129-cvs
    Copyright (C) 2013 Free Software Foundation, Inc.
    License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
    This is free software: you are free to change and redistribute it.
    There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
    and "show warranty" for details.
    This GDB was configured as "--target=arm-none-eabi".
    For bug reporting instructions, please see:
    <http://www.gnu.org/software/gdb/bugs/>...
    Reading symbols from <path to firmware image>...done.
    info: One client connected!
    (gdb)
    ```

2. Start a terminal emulator and connect to the virtual serial port presented by the FRDM-K64F at 115200, 8N1.

3. Load the program

    ```
    (gdb) load
    ```

4. Once the program has loaded, start it.

    ```
    (gdb) c
    ```

5. The output in the serial terminal window should look like in step 9 above (the final step of the regular run).
