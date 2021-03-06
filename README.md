## madasd

This is a simulator for the Gumstix Overo server for the modified madas project.

The server will provide two sockets for clients.

A control socket for ASCII commands to be relayed to the ads1278 device driver (start, stop, config, status, etc...)

A data socket that will stream the ads1278 binary data.

The data socket does not accept commands, it is read-only.

Data will be dropped if a client is not continuously reading.

The server logs to syslog as well stderr.

### Fetch and Build

    ~$ git clone https://github.com/scottellis/madasd.git

    ~$ cd madasd

    ~/madasd$ make
    -Wall -O2 -c -o ads127x.o ads127x.c
    gcc -Wall -O2 -c -o madasd.o madasd.c
    gcc -Wall -O2 -c -o utility.o utility.c
    gcc -Wall -O2 ads127x.o madasd.o utility.o -o madasd -lpthread

There are a few command line options

    ~/madasd$ ./madasd -h
    Usage: ./madasd [-p<port>][-f<file>][-d][-v]
      -p     control listener port, data listener will be port + 1
      -d     daemonize
      -f     simulated data from a previously captured file
      -v     verbose mode, enable some extra logging

Some raw data capture files are provided in madasd/data

    data/ch2_1khz.raw
    data/ch2_2khz.raw
    data/ch2_noise.raw

They can be passed to the server at startup in which case data returned will come from the file.

    ~/madasd$ ./madasd -f data/ch2_1khz.raw

The data has the raw ADC format

    <ch1><ch2><ch3><ch4><ch5><ch6><ch7><ch8>[repeat]

Each channel sample is a 32-bit integer, so one 'complete sample' is 8 * 4 = 32 bytes.

Each 4096 byte block contains 4096 / 32 = 128 samples.

You need to know which channels are active in the driver and disregard the ones that are not.

I will implement some config commands to handle this.

The server returns 32 blocks of data at a time on the **data** socket.

This will also be one of the config options, but for now is a constant in the madasd.c source file.

If no data file is provided for simulation, the server will stuff ascii characters into the data, a different character for each block.

### Testing with Netcat

I am using the 'netcat-openbsd' version of netcat from an Ubuntu machine for these examples.

I am going to use three terminals **server**, **control** and **data**.

Start the **server** in one terminal

    ~/madasd$ ./madasd -v
    ./madasd: control listening on port 6000

Either on the same machine or another, start a **control** terminal replacing the IP as appropriate.

    /tmp$ nc 192.168.10.6 6000
    ok
    status
    idle
    start
    ok
    status
    running
    stop
    ok
    disconnect
    ok


On the **server** terminal where you started madasd you would have seen this with verbose (-v) enabled.

    ~/madasd$ ./madasd -v
    ./madasd: control listening on port 6000
    ./madasd: new client: 192.168.10.12
    ./madasd: data thread started: port: 6001
    ./madasd: status
    ./madasd: start
    ./madasd: status
    ./madasd: stop
    ./madasd: disconnect
    ./madasd: data thread stopped
    ./madasd: client disconnect: 192.168.10.12


Now in the **control** terminal start the driver

    /tmp$ nc localhost 6000

In another **data** terminal open a second connection to the server for the data.

The data port is always the control port + 1.

Pipe this data to a file.

    /tmp$ nc localhost 6001 > data.bin

Back in the control terminal, start the driver, let it run for a little then issue a stop and disconnect.

The data accumulates fast, ten seconds is fine.

    /tmp$ nc localhost 6000   < this is already going
    start
    ok
    stop
    ok
    disconnect
    ok


The **data** connection should close automatically.

You should have gotten some data.

    /tmp$ ls -l data.bin
    -rw-rw-r-- 1 scott scott 1310720 Dec  3 11:00 data.bin

You can look at the data with hexdump

    /tmp$ hexdump -C data.bin
    00000000  41 41 41 41 41 41 41 41  41 41 41 41 41 41 41 41  |AAAAAAAAAAAAAAAA|
    *
    00001000  42 42 42 42 42 42 42 42  42 42 42 42 42 42 42 42  |BBBBBBBBBBBBBBBB|
    *
    00002000  43 43 43 43 43 43 43 43  43 43 43 43 43 43 43 43  |CCCCCCCCCCCCCCCC|
    *

    ...

    0013e000  47 47 47 47 47 47 47 47  47 47 47 47 47 47 47 47  |GGGGGGGGGGGGGGGG|
    *
    0013f000  48 48 48 48 48 48 48 48  48 48 48 48 48 48 48 48  |HHHHHHHHHHHHHHHH|
    *
    00140000

The fundamental data block size is 4096, determined by the device driver.

The madasd server fetches 32 blocks at a time from the driver to stream to clients.

Just for testing, every block gets stuffed with a different character.

This is very simplistic right now.

The key being two sockets, one control and one data.

You can also test with some data files in the madasd/data dir.

    madasd$ ls -l data
    total 1200
    -rw-r--r-- 1 scott scott 409600 Jan  4 09:31 ch2_1khz.raw
    -rw-r--r-- 1 scott scott 409600 Jan  4 09:31 ch2_2khz.raw
    -rw-r--r-- 1 scott scott 409600 Jan 10 10:42 ch2_noise.raw

The server will cycle through the file for the data it returns.

    madasd$ ./madasd -f data/ch2_1khz.raw

There is additional timestamp and gps data that needs to be incorporated into the data stream and so there will be a header to describe this.

I used netcat for demonstration only.

Use the MadasUI Qt project for a GUI client that works with the madasd server.


