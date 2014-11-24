/*
 * iperf, Copyright (c) 2014, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include <Python.h>

#include "cpython.h"

static void
Iperf_dealloc(iperf_IperfObject* self)
{
	Py_XDECREF(self->start);
	Py_XDECREF(self->intervals);
	Py_XDECREF(self->end);
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Iperf_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	iperf_IperfObject *self;

	self = (iperf_IperfObject *) type->tp_alloc(type, 0);
	if (self) {
		self->start = PyString_FromString("");
		if (!self->start) {
			Py_DECREF(self);
			return NULL;
		}

    self->intervals = PyString_FromString("");
    if (!self->intervals) {
			Py_DECREF(self);
			return NULL;
		}

    self->end = PyString_FromString("");
		if (!self->end) {
			Py_DECREF(self);
			return NULL;
	}

	return (PyObject *) self;
}

static int
Iperf_init(iperf_IperfObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *start = NULL, *intervals = NULL, end = NULL, *tmp;

	static char *kwlist[] = {"start", "intervals", "end", NULL};

	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOO", kwlist, 
																		&start, &intervals, &end) )
		return -1; 

	if (start) {
		tmp = self->start;
		Py_INCREF(start);
		self->start = start;
		Py_XDECREF(tmp);
	}

	if (intervals) {
		tmp = self->intervals;
		Py_INCREF(intervals);
		self->intervals = intervals;
		Py_XDECREF(tmp);
	}

	if (end) {
		tmp = self->end;
		Py_INCREF(end);
		seld->end = end;
		Py_XDECREF(tmp);
	}

	return 0;
}

static PyMemberDef Iperf_members[] = {
	{ "start", T_OBJECT_EX, offsetof(iperf_IperfObject, start), 0,
		"first name" },
	{ "intervals", T_OBJECT_EX, offsetof(iperf_IperfObject, intervals), 0,
		"last name" },
	{ "end", T_OBJECT_EX, offsetof(iperf_IperfObject, end), 0,
		"noddy number" },
	{ NULL }  /* Sentinel */
};

static PyObject *
Noddy_name(Noddy* self)
{
    static PyObject *format = NULL;
    PyObject *args, *result;

    if (format == NULL) {
        format = PyString_FromString("%s %s");
        if (format == NULL)
            return NULL;
    }

    if (self->first == NULL) {
        PyErr_SetString(PyExc_AttributeError, "first");
        return NULL;
    }

    if (self->last == NULL) {
        PyErr_SetString(PyExc_AttributeError, "last");
        return NULL;
    }

    args = Py_BuildValue("OO", self->first, self->last);
    if (args == NULL)
        return NULL;

    result = PyString_Format(format, args);
    Py_DECREF(args);
    
    return result;
}

static PyMethodDef Noddy_methods[] = {
    {"name", (PyCFunction)Noddy_name, METH_NOARGS,
     "Return the name, combining the first and last name"
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject iperf_IperfType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /* ob_size */
	"iperf.Iperf",             /* tp_name */
	sizeof(iperf_IperfObject), /* tp_basicsize */
	0,                         /* tp_itemsize */
	0,                         /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_compare */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	"Iperf objects",           /* tp_doc */
};

static PyMethodDef module_methods[] = {
	{ NULL }  /* Sentinel */
};

PyMODINIT_FUNC
initiperf(void) 
{
	PyObject* m;

	if (PyType_Ready(&iperf_IperfType) >= 0) {

		m = Py_InitModule3("iperf", module_methods,
				"Example module that creates an extension type.");

		if (m) {
			Py_INCREF(&iperf_IperfType);
			PyModule_AddObject(m, "Iperf", (PyObject *) &iperf_IperfType);
		}
	}
}
