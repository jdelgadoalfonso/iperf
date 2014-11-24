# Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009,
# 2011 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

## Find the install dirs for the python installation.
##  By James Henstridge

dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[
	AC_REQUIRE([AM_PATH_PYTHON])
	AC_MSG_CHECKING(for headers required to compile python extensions)
	dnl Win32 doesn't have a versioned directory for headers
	if test "$PYTHON_PLATFORM" != "win32"; then
		py_versiondir="/python${PYTHON_VERSION}"
	else
		py_versiondir=
	fi
	dnl deduce PYTHON_INCLUDES
	py_major_version=`$PYTHON -c "import sys; print(sys.version[[:1]])"`
	PYTHON_INCLUDES=`pkg-config --cflags python$py_major_version`
	dnl deduce PYTHON_LIBS
	PYTHON_LIBS=`pkg-config --libs python$py_major_version`

	AC_SUBST(PYTHON_INCLUDES)
	AC_SUBST(PYTHON_LIBS)

	dnl check if the headers exist:
	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
	AC_TRY_CPP([#include <Python.h>],dnl
	[
		AC_MSG_RESULT(found)
		$1
	],dnl
	[
		AC_MSG_RESULT(not found)
		$2
	])
	CPPFLAGS="$save_CPPFLAGS"

	AC_MSG_CHECKING(for libraries required to compile python extensions)
	dnl check if the libraries exist:
	save_LIBS="$LIBS"
	save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
	LIBS="$LIBS $PYTHON_LIBS"
	AC_TRY_LINK([#include <Python.h>],
	[
		PyObject *m;
		m = Py_BuildValue("foo");
	  return m ? 0 : 1;
	],dnl
	[
		AC_MSG_RESULT(found)
		$1
	],dnl
	[
		AC_MSG_RESULT(not found)
		$2
	])
	CPPFLAGS="$save_CPPFLAGS"
	LIBS="$save_LIBS"
])
