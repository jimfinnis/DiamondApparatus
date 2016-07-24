Diamond Apparatus (named randomly by http://pale.org/rainbow.php)
is a simple publish/subscribe broker and library, using C++ and
TCP/IP. It's a bit slow, not sure why -- about 10 msgs a second
on loopback.

Data is in the form of topics, named blocks of data. Each topic
contains an array of Datum objects, which are either floats or strings.
Publishers send topics, and any subscribers subscribed
to that topic receive the new data. The broker (server) also stores
the last data on a topic, and sends it to any new subscriber.

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

### diamond pub <name> <types> <val> 
This will publish values to a topic. The types string consists of
"s" for string and "f" for float, so 

```
diamond pub foo sff Hello 0.1 0.2
```

will publish a string ("Hello") and two floats to the "foo" topic.

### diamond listen <name>
This will start a loop listening for changes with a frequency of 10Hz.
Changed data will be written to stdout. If the server exits, the program
will exit. It can be killed with the usual signals.

## Special topics
The server publishes several special topics to which other programs
can subscribe. These are:
- **topics** : a list of all published topics


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
periodically for new data with **get()**.

### publish(const char *name, Topic *t)
Publishes data to a topic.

### Topic get(const char *n)
Gets the latest value of a topic, as a new copy to avoid threading problems.
See below for how to access the data and state.

## Topics
Topics, used by **publish()** and **get()**, support the following operations:
- **size()** returns the size (number of Data)
- **square brackets** access individual Datum objects (as constant refs). If an
attempt is made to access an out of range datum, a float zero datum will be returned.
- **add(const Datum&)** adds a datum
- **clear()** empties the topic
- the **state** member contains the state of the topic.

### Topic states
as set in topic copies returned from **get()**:
- **Topic::NotConnected** - the client is not connected and this topic contains no data
- **Topic::NotFound** - no data has yet been received for this topic.
- **Topic::Unchanged** - the topic is unchanged since **get()** was last called on it.
- **Topic::Changed** - the topic has changed since **get()** was last called.

## Data
Each individual Datum can be created with a string or float constructor
and added to the topic:

```c++
Topic t;
t.add(Datum("string item"))
t.add(Datum(0.1));
publish("foo",&t);
```

The value of data can be retrieved with the **s()** and **f()**
functions, returning string (as const char pointer) and float
respectively. If the retrieval of the wrong type is attempted,
default values will be returned:

```c++
Topic t = get("foo")
if(t.state == Topic::Changed){
    printf("%s\n",t[0].s());
}   
```
