This is a set of simple python bindings for Diamond Apparatus.

# Building
You should have python's **distutils** installed already (most Pythons
come with this). Build and install Diamond Apparatus, then run from
this directory
```
python setup.py build
sudo python setup.py install
```
and you should be ready to run.

# The API
This is fairly similar to the C++ API, but lists and tuples mean we
don't need to worry about the Topic and Datum types.

## diamondapparatus.init()
will connect to Diamond Apparatus if it is running.

## diamondapparatus.destroy()
will disconnect.

## diamondapparatus.isrunning()
will return true if the server is running.

## diamondapparatus.publish(name,sequence)
will publish the data in the sequence (list or tuple) to the topic named.
The sequence must contain only strings and numbers (integers will be
converted to floats).

## diamondapparatus.subscribe(name)
will subscribe to the given topic (and the server will immediately send
a message containing the data if some is already there).

## diamondapparatus.get(name,[waitmode=mode])
will wait for data on a given topic and return it as a tuple
of floats and strings. The modes are:
- **diamondapparatus.WaitAny** (default) waits until the topic contains data, and will not block
block at all if the topic contains even old data. Use this for routine
access to topics.
- **diamondapparatus.WaitNew** waits until new data arrives, and will block if the
topic contains no data or old data. Use this to wait for updated data.
- **diamondapparatus.WaitNone** always returns immediately - if there
was no data yet, the function will return False.

## diamondapparatus.waitforany()
will wait for new data on a topic. If a topic has been subscribed to,
the server will send data to the client immediately so this call may return
immediately - it depends on the timing. If you want to wait for new data,
put a short delay between the subscribe() and the waitforany().


# Example code
Here's a publisher:
```python
from diamondapparatus import *
init()
publish("foo",[0,1,2,"fish"])
```

And here's a subscriber:
```python
from diamondapparatus import *
init()
subscribe("foo")
while True:
    print get("foo",wait=WaitNew)
```
    

