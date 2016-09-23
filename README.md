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
publish new data, and kill the server.

### diamond server
This will run the server and never exit. With the -d option, the
server will daemonize itself (detach from the terminal and run
in the background).

### diamond kill
This will kill the server and disconnect all subscribers. They will
not die automatically, but their client thread will exit. This can
be tested for with **isRunning()**.

### diamond pub _name_ _types_ _val_ _[val..]_
This will publish values to a topic. The types string consists of
"s" for string and "f" for float, so 

```
diamond pub foo sff Hello 0.1 0.2
```

will publish a string ("Hello") and two floats to the "foo" topic.

### diamond show _name_
This will wait for data to be present for the topic _name_, and 
will print it to stdout. If the server exits, the wait will abort;
if the server already has data for this topic, it will send it
to the program and the wait will be brief.
### diamond listen _name_
This will start a loop listening for changes with a frequency of 10Hz.
Changed data will be written to stdout. If the server exits, the program
will exit. It can be killed with the usual signals.

### diamond version
Will print the current version number and name, and exit.

### diamond check
Just checks to see if the server is alive, silently exiting with
return code 0 if it is and failing with return code 1 if not. Useful
in scripts. The server also does this, so two servers don't run.
Finally, the diamond application also does this when no command line options
are given, so you can just type "diamond" and ignore the usage notes.

## Special topics
The server publishes several special topics to which other programs
can subscribe. These are:
- **topics** : a list of all published topics
- **stats** : server stats (subscriber count, msgs sent, msgs received,
uptime in seconds)


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
and port for the server. Running init() twice has no effect, EXCEPT
that destroy() must be called as many times as init() is run.
(Internally, init() increments a counter and destroy() decrements it -
if the counter goes down to zero, destroy() does its job.)

### destroy()
Closes down the client connection politely and should be called
when the code exits. Does so by killing the thread and closing the socket.
See init() above for what happens with more than one init() or destroy().

### subscribe(const char *name)
Subscribes to a topic of a given name. This can then be checked
for new data with **get()**, which has options to wait for data.

### publish(const char *name, Topic& t)
Publishes data to a topic.

### Topic get(const char *n,int wait=GET_WAITNONE)
Gets the latest value of a topic, as a new copy to avoid threading problems.
See below for how to access the data and state. By default, this will
check the data asynchronously and return immediately. If used in this
way, the state of the returned topic should be checked for validity
(with **Topic::isValid()**). Otherwise, we can wait for new data
to arrive or wait for data only if there is no data yet. This is done
by setting the **wait** value:
- **GET_WAITANY** waits until the topic contains data, and will not block
block at all if the topic contains even old data. Use this for routine
access to topics.
- **GET_WAITNEW** waits until new data arrive, and will block if the
topic contains no data or old data. Use this to wait for updated data.
- **GET_WAITNONE** (default) always returns immediately - you should
use **isValid()** on the topic to check if data is present yet.

### waitForAny()
Waits for new data on any topic to which the client is subscribed.

### killServer()
Sends a message to the server to kill itself.

### clearServer()
Deletes all stored topic data on the server, but leaves subscriptions
untouched. Mainly used in testing.

## Topics
Topics, used by **publish()** and **get()**, support the following operations:
- **size()** returns the size (number of Data)
- **square brackets** access individual Datum objects (as constant refs). If an
attempt is made to access an out of range datum, a float zero datum will be returned.
- **add(const Datum&)** adds a datum (for publishing)
- **isValid()** returns true if a topic received with **get()** has valid data
- **clear()** empties the topic
- the **state** member contains the state of the topic.
- **double age()** returns the number of seconds since data was received
on this topic.
- **dump()** will print the topic's data as a list of lines to stdout.

### Topic states
as set in topic copies returned from **get()**:
- **TOPIC_NOTCONNECTED** - the client is not connected and this topic contains no data
- **TOPIC_NOTFOUND** - we have not subscribed to this topic.
- **TOPIC_NODATA** - no data has yet been received for this topic.
- **TOPIC_UNCHANGED** - the topic is unchanged since **get()** was last called on it.
- **TOPIC_CHANGED** - the topic has changed since **get()** was last called.

## Data
Each individual Datum can be created with a string or float constructor
and added to the topic:

```c++
Topic t;
t.add(Datum("string item"));
t.add(Datum(0.1));
publish("foo",&t);
```

The value of data can be retrieved with the **s()** and **f()**
functions, returning string (as const char pointer) and float
respectively. If the retrieval of the wrong type is attempted,
default values will be returned:

```c++
subscribe("foo");
Topic t = get("foo",GET_WAITANY);
printf("%s\n",t[0].s());
}   
```
Additionally, like a topic, a Datum has a **dump()** call to write
to standard out:
```c++
subscribe("foo");
Topic t = get("foo",GET_WAITANY);
t[0].dump();
}   
```

## An example: what "diamond show" does
This is the code for the **diamond show** command:

```c++
// connect to the server and start the client thread

init();

// subscribe to the topic - the server will send any
// data it already has

subscribe(argv[2]);

// wait for any data on that topic

Topic t = get(argv[2],GET_WAITANY);

// the data has arrived, print out each datum in the topic

for(int i=0;i<t.size();i++)
    t[i].dump();
```

## C linkage
Functions are provided for use from plain C, permitting access
to error codes without exceptions, and Topics without access
to the Topic and Datum classes. These are also available from
C++, if you prefer to code this way.

```C
// return error if a function returned -1
const char *diamondapparatus_error();
// 0 if OK, -1 on error
int diamondapparatus_init();
// 0 if OK, -1 on error
int diamondapparatus_server();
// will return -1 in error, >0 if not last destroy, and 0 if did actually
// shutdown
int diamondapparatus_isrunning();
int diamondapparatus_destroy();
int diamondapparatus_killserver();
int diamondapparatus_clearserver();
/// publish the topic built up by diamondapparatus_add..()
int diamondapparatus_publish(const char *n);
/// clear the topic to publish
void diamondapparatus_newtopic();
/// add a float to the topic to publish
void diamondapparatus_addfloat(float f);
/// add a string to the topic to publish
void diamondapparatus_addstring(const char *s);


int diamondapparatus_subscribe(const char *n);
/// read a topic, returning its state or -1. The topic can be accessed
/// with diamondapparatus_fetch...
int diamondapparatus_get(const char *n,int wait);
/// is the last topic got a valid topic to fetch data from?
int diamondapparatus_isfetchvalid();
/// wait for a message on any topic we are subscribed to 
int waitforany();
/// read a string from the topic got
const char *diamondapparatus_fetchstring(int n);
/// read a float from the topic got
float diamondapparatus_fetchfloat(int n);
/// get the type of a datum in the topic got
uint32_t diamondapparatus_fetchtype(int n);
/// get the number of data in the topic got
int diamondapparatus_fetchsize();
```
