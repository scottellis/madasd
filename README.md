## madasd

This is a simulator for the Gumstix Overo server for the modified madas project.

The Gumstix will provide two sockets for clients.

A control socket for ASCII commands to be relayed to the ads1278 device driver (start, stop, config, status, etc...)

A data socket that will stream the ads1278 binary data.

The control socket accepts one command and then closes the connection similar to http. Additionally a client has 2 seconds to send their command or the server kills the connection anyway.

We can change this if it is inconvenient, but it simplifies the server code error handling.

The data socket is only open if the driver is started and will close again when the driver is stopped.

The data socket does not accept commands, it is read-only.

Data will be dropped if a client is not continuously reading.

The madasng server logs to syslog as well stderr.

### Fetch and Build

    ~$ git clone https://github.com/scottellis/madasng.git

    ~$ cd madasng

    ~/madasng$ make
    -Wall -O2 -c -o ads127x.o ads127x.c
    gcc -Wall -O2 -c -o madasd.o madasd.c
    gcc -Wall -O2 -c -o utility.o utility.c
    gcc -Wall -O2 ads127x.o madasd.o utility.o -o madasd -lpthread

### Testing with Netcat

For explanation I am going to use three terminals **server**, **control** and **data**.

Start the **server** in a terminal

    ~/madasng$ ./madasd
    ./madasd: control listening on port 6000

Either on the same machine or another, start a **control** terminal

    /tmp$ nc localhost 6000
    status
    stopped

    /tmp$ nc localhost 6000
    start
    ok

    /tmp$ nc localhost 6000
    status
    started

    /tmp$ nc localhost 6000
    stop
    ok

    /tmp$ nc localhost 6000
    status
    stopped


On the **server** terminal where you started madasd you would have seen this

    ~/madasng$ ./madasd
    ./madasd: control listening on port 6000
    ./madasd: data thread started: port: 6001
    ./madasd: data thread stopped


Now in the **control** terminal start the driver

    /tmp$ nc localhost 6000
    start
    ok

In a new **data** terminal (any machine) use nc to pipe the data to a file

    /tmp$ nc localhost 6001 > data.bin

Let it run for a little, but not too long, the data accumulates fast.

Ten seconds is fine.

Stop the driver in the **control** terminal.

    /tmp$ nc localhost 6000
    stop
    ok

The **data** connection should close automatically.

    /tmp$ nc localhost 6001 > data.bin
    /tmp$ ls -l data.bin
    -rw-rw-r-- 1 scott scott 1310720 Dec  3 11:00 data.bin

You can look at the data with hexdump

    /tmp$ hd data.bin
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

This is very simplistic right now. There is additional block timestamp data and gps data that also needs to be incorporated into the data stream and so there will be a header describing incoming data.

This is just a start to let you write a client similar to the UI client that I am porting to this project for visualizing the acoustic data.

The key being two sockets, control and data.

I used netcat as the client to demonstrate that a client does not need any particularly difficult logic to receive the raw data.
