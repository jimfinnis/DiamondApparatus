Diamond Apparatus (named randomly by http://pale.org/rainbow.php)
is a simple publish/subscribe broker and library, using C++ and
TCP/IP. It's a bit slow, not sure why -- about 10 msgs a second
on loopback.

Data is in the form of topics, named blocks of data (currently
arrays of floats). Publishers send topics, and any subscribers subscribed
to that topic receive the new data.
The server does not store topic data -- it simply passes
any publications received onto subscribers which have expressed an
interest in that topic. Thus, a new subscriber which joins the server
will not receive a topic until it changes.

## Environment variables: hostname and port
The default port number is **29921**, but can be changed by setting
the **DIAMOND_PORT** environment variable. Similarly, the hostname
for client connection can be set by the **DIAMOND_HOST** environment
variable.

## Main program
The main program can be used to run the server, listen for changes,
and publish new data, and kill the server.

### diamond server
This will run the server and never exit.

### diamond kill
This will kill the server and disconnect all subscribers. They will
not die automatically, but their client thread will exit. This can
be tested for with **isRunning()**.

### diamond pub <name> <val> 
This will publish a single float to a topic.

### diamond listen <name>
This will start a loop listening for changes with a frequency of 10Hz.
Changed data will be written to stdout. If the server exits, the program
will exit. It can be killed with the usual signals.

## API
The API is defined in **diamondapparatus.h**. All functions are in
the **diamondapparatus** namespace.

### server()
This starts the server and never exits. See above for how to
set the port number.

### init()
Initialises a client connection to the server, and is required for
all other functions except **server()**. This starts the client thread, which
constantly waits for messages. See above for how to change the host
and port for the server.

### destroy()
Closes down the client connection politely and should be called
when the code exits. Does so by killing the thread and closing the socket.

### subscribe(const char *name)
Subscribes to a topic of a given name. This can then be checked
with **get()**.

### publish(const char *name, float *f,int count)
Publishes an array of floats to a topic.

### get(const char *n,float *out,int maxsize)
Gets the latest value of a topic into an array of floats of
appropriate size. Returns the number of floats actually
in the array. If this would have been >maxsize, the array
will be truncated.
Return values:
*  0    - value has not been changed so no copying was done
* -1   - topic has not been subscribed to
* -2   - topic has not yet received data
* -3   - not connected (thread not running)
* n    - number of floats copied (will be <= maxsize)
