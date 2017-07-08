/**
 * @file python.c
 * @brief  Brief description of file.
 *
 */

#include <Python.h>
#include <diamondapparatus/diamondapparatus.h>

struct module_state {
    PyObject *error;
};



#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

static PyMethodDef DiamondMethods[];

#if PY_MAJOR_VERSION >= 3

static int diamond_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int diamond_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "diamondapparatus",
        NULL,
        sizeof(struct module_state),
        DiamondMethods,
        NULL,
        diamond_traverse,
        diamond_clear,
        NULL
};
#define INITERROR return NULL
PyMODINIT_FUNC
PyInit_diamondapparatus(void)

#else

#define INITERROR return
void 
initdiamondapparatus(void)
#endif
{
    PyObject *m;
#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&moduledef);
#else
    m = Py_InitModule("diamondapparatus",DiamondMethods);
#endif
    if(!m)INITERROR;
    
    struct module_state *st = GETSTATE(m);
    st->error = PyErr_NewException("diamondapparatus.Error",NULL,NULL);
    if(st->error==NULL){
        Py_DECREF(m);
        INITERROR;
    }
    
    // topic states
    PyModule_AddIntConstant(m,"NoData",TOPIC_NODATA);
    PyModule_AddIntConstant(m,"Unchanged",TOPIC_UNCHANGED);
    PyModule_AddIntConstant(m,"Changed",TOPIC_CHANGED);
    PyModule_AddIntConstant(m,"NotFound",TOPIC_NOTFOUND);
    PyModule_AddIntConstant(m,"NotConnected",TOPIC_NOTCONNECTED);
    
    // get types
    PyModule_AddIntConstant(m,"WaitNone",GET_WAITNONE);
    PyModule_AddIntConstant(m,"WaitNew",GET_WAITNEW);
    PyModule_AddIntConstant(m,"WaitAny",GET_WAITANY);
    
#if PY_MAJOR_VERSION >= 3
    return m;
#endif
    
}
    
#define CHKRUN if(!diamondapparatus_isrunning()){\
    PyErr_SetString(GETSTATE(self)->error,"unable to communicate with server");return NULL;}

static PyObject *diamond_init(PyObject *self,PyObject *args){
    if(diamondapparatus_init()<0){
        PyErr_SetString(GETSTATE(self)->error,"unable to communicate with server");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *diamond_destroy(PyObject *self,PyObject *args){
    CHKRUN;
    diamondapparatus_destroy();
    Py_RETURN_NONE;
}


static PyObject *diamond_isrunning(PyObject *self,PyObject *args){
    return diamondapparatus_isrunning() ? Py_True:Py_False;
}

static PyObject *diamond_publish(PyObject *self,PyObject *args){
    CHKRUN;
    const char *topic;
    PyObject *data,*seq;
    int i,len;
    
    if(!PyArg_ParseTuple(args,"sO",&topic,&data))
        return NULL;
    
    len = PySequence_Size(data);
    seq = PySequence_Fast(data,"expected a sequence");
    if(!seq)return NULL;
    diamondapparatus_newtopic();
    for(i=0;i<len;i++){
        PyObject *item = PySequence_Fast_GET_ITEM(seq,i);
        if(PyNumber_Check(item)){
            diamondapparatus_addfloat(PyFloat_AsDouble(item));
#if PY_MAJOR_VERSION < 3
        } else if(PyString_Check(item)){
            diamondapparatus_addstring(PyString_AsString(item));
#endif
        } else if(PyUnicode_Check(item)){
            PyObject *str = PyUnicode_AsEncodedString(item,"utf-8","ignore");
            diamondapparatus_addstring(PyBytes_AS_STRING(str));
            Py_DECREF(str);
        } else {
            Py_DECREF(seq);
            PyErr_SetString(GETSTATE(self)->error,"published topics must be sequences of numbers and strings");
            return NULL;
        }
        
    }
    Py_DECREF(seq);
    
    if(diamondapparatus_publish(topic)<0){
        PyErr_SetString(GETSTATE(self)->error,diamondapparatus_error());
        return NULL;
    }
    Py_RETURN_NONE;
}
static PyObject *diamond_subscribe(PyObject *self,PyObject *args){
    CHKRUN;
    const char *topic;
    if(!PyArg_ParseTuple(args,"s",&topic))
        return NULL;
    if(diamondapparatus_subscribe(topic)<0){
        PyErr_SetString(GETSTATE(self)->error,diamondapparatus_error());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *diamond_get(PyObject *self,PyObject *args,PyObject *keywds){
    CHKRUN;
    const char *topic;
    int waittype=GET_WAITANY;
    static char *kwlist[]={"topic","wait",NULL};
    
    if(!PyArg_ParseTupleAndKeywords(args,keywds,"s|i",kwlist,
                                    &topic,&waittype))
        return NULL;
    if(diamondapparatus_get(topic,waittype)<0){
        PyErr_SetString(GETSTATE(self)->error,diamondapparatus_error());
        return NULL;
    }
    
    // check it's valid
    if(diamondapparatus_isfetchvalid()){
        int i;
        int size = diamondapparatus_fetchsize();
        PyObject *o = PyTuple_New(size);
        for(i=0;i<size;i++){
            switch(diamondapparatus_fetchtype(i)){
            case DT_FLOAT:
                PyTuple_SetItem(o,i,
                   PyFloat_FromDouble(diamondapparatus_fetchfloat(i)));
                break;
            case DT_STRING:
#if PY_MAJOR_VERSION >= 3
                PyTuple_SetItem(o,i,
                                PyUnicode_FromString(diamondapparatus_fetchstring(i)));
#else
                PyTuple_SetItem(o,i,
                                PyString_FromString(diamondapparatus_fetchstring(i)));
#endif
                break;
            default:
                PyErr_SetString(GETSTATE(self)->error,"bad type in topic!");
                return NULL;
            }
        }
        return o;
    } else {
        // not valid
        return Py_False;
    }
}
static PyObject *diamond_waitforany(PyObject *self,PyObject *args){
    CHKRUN;
    if(diamondapparatus_waitforany()<0){
        PyErr_SetString(GETSTATE(self)->error,diamondapparatus_error());
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyMethodDef DiamondMethods[] = {
    {"init",diamond_init,METH_NOARGS,"Initialise diamond and connect to server"},
    {"destroy",diamond_destroy,METH_NOARGS,"Close down diamond connection"},
    {"isrunning",diamond_isrunning,METH_NOARGS,"Return true if diamond client is running"},
    
    {"publish",diamond_publish,METH_VARARGS,"Publish the built topic to diamond"},
    
    {"subscribe",diamond_subscribe,METH_VARARGS,"Subscribe to a topic"},
    {"get",(PyCFunction)diamond_get,METH_VARARGS|METH_KEYWORDS,"Read a topic"},
    {"waitforany",diamond_waitforany,METH_NOARGS,"Wait for any message"},
    
    {NULL,NULL,0,NULL}
};
