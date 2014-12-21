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
#include <structmember.h>

#include "cpython.h"

static void
Iperf_dealloc(Iperf *self)
{
	Py_XDECREF(self->start);
	Py_XDECREF(self->intervals);
	Py_XDECREF(self->end);
	self->ob_type->tp_free((PyObject *) self);
}

static PyObject *
Iperf_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Iperf *self;

	self = (Iperf *) type->tp_alloc(type, 0);
	if (self) {
		self->start = PyDict_New();
		if (!self->start) {
			Py_DECREF(self);
			return NULL;
		}

    self->intervals = PyList_New(0);
    if (!self->intervals) {
			Py_DECREF(self);
			return NULL;
		}

    self->end = PyDict_New();
		if (!self->end) {
			Py_DECREF(self);
			return NULL;
		}
		
		self->error = PyString_FromString("");
		if (!self->error) {
			Py_DECREF(self);
			return NULL;
		}
	}
	return (PyObject *) self;
}

static int
Iperf_init(Iperf *self, PyObject *args, PyObject *kwds)
{
	PyObject *start = NULL, *intervals = NULL, *end = NULL, *error = NULL, *tmp;

	static char *kwlist[] = { "start", "intervals", "end", "error", NULL };

	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, 
																		&start, &intervals, &end, &error) )
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
		self->end = end;
		Py_XDECREF(tmp);
	}

	if (error) {
		tmp = self->error;
		Py_INCREF(error);
		self->error = error;
		Py_XDECREF(tmp);
	}

	return 0;
}

static PyMemberDef Iperf_members[] = {
	{ "start", T_OBJECT_EX, offsetof(Iperf, start), 0,
		"first name" },
	{ "intervals", T_OBJECT_EX, offsetof(Iperf, intervals), 0,
		"last name" },
	{ "end", T_OBJECT_EX, offsetof(Iperf, end), 0,
		"noddy number" },	
	{ "error", T_OBJECT_EX, offsetof(Iperf, error), 0,
		"noddy number" },
	{ NULL }  /* Sentinel */
};

static PyMethodDef Iperf_methods[] = {
	{ NULL }  /* Sentinel */
};

PyTypeObject iperfType = {
	PyObject_HEAD_INIT(NULL)
	0,                          /* ob_size */
	"iperf.Iperf",              /* tp_name */
	sizeof(Iperf),              /* tp_basicsize */
	0,                          /* tp_itemsize */
	(destructor) Iperf_dealloc, /* tp_dealloc */
	0,                          /* tp_print */
	0,                          /* tp_getattr */
	0,                          /* tp_setattr */
	0,                          /* tp_compare */
	0,                          /* tp_repr */
	0,                          /* tp_as_number */
	0,                          /* tp_as_sequence */
	0,                          /* tp_as_mapping */
	0,                          /* tp_hash */
	0,                          /* tp_call */
	0,                          /* tp_str */
	0,                          /* tp_getattro */
	0,                          /* tp_setattro */
	0,                          /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT |
	Py_TPFLAGS_BASETYPE,        /* tp_flags */
	"Iperf objects",            /* tp_doc */
	0,		                      /* tp_traverse */
	0,		                      /* tp_clear */
	0,		                      /* tp_richcompare */
	0,		                      /* tp_weaklistoffset */
	0,		                      /* tp_iter */
	0,		                      /* tp_iternext */
	Iperf_methods,              /* tp_methods */
	Iperf_members,              /* tp_members */
	0,                          /* tp_getset */
	0,                          /* tp_base */
	0,                          /* tp_dict */
	0,                          /* tp_descr_get */
	0,                          /* tp_descr_set */
	0,                          /* tp_dictoffset */
	(initproc) Iperf_init,      /* tp_init */
	0,                          /* tp_alloc */
	Iperf_new,                  /* tp_new */
};

static PyObject *
cPython_CreateIperfObject() {
	/* Call the class object. */
	PyObject *obj = PyObject_CallObject((PyObject *) &iperfType, NULL);

	return obj;
}

static int
cPython_addItemToDict
(PyObject *o, const char *attr_name, const char *key, PyObject *val) {
	int fail = -1;
	PyObject *attr = PyObject_GetAttrString(o, attr_name);
	if (PyDict_Check(attr)) {
		if (PyDict_SetItemString(attr, key, val)) {
			if (!PyErr_Occurred()) {
				fail++;
			}
		}
	}
	Py_XDECREF(attr);
	return fail;
}
