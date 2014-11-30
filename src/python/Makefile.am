pyexec_LTLIBRARIES				= pyiperf.la		 				          # Build and install an iperf library

# Specify the source files and flags for the iperf library
pyiperf_la_SOURCES				= \
														iperf_python.c \
														iperf_python.h \
														cpython.c \
														cpython.h \
														iperf_handlers.c \
														iperf_handlers.h

pyiperf_la_CFLAGS					= $(PYTHON_INCLUDES)
pyiperf_la_LDFLAGS				= -module -avoid-version
pyiperf_la_LIBADD					= $(PYTHON_LIBS) ../libiperf.la
