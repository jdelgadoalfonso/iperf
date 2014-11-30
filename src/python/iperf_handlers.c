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
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */
#include <Python.h>

#include "iperf.h"
#include "iperf_api.h"
#include "iperf_util.h"
#include "iperf_locale.h"
#include "net.h"

static jmp_buf sigend_jmp_buf;
	
static void
sigend_handler(int sig)
{
	longjmp(sigend_jmp_buf, 1);
}

int
iperf_client(const char *host, int port)
{
	int ret = 0;
	struct iperf_test *test;

	test = iperf_new_test();
	if (!test) {
		PyErr_Format(PyExc_RuntimeError,
				"create new test error - %s", iperf_strerror(i_errno));
		return -1;
	}
	iperf_defaults(test);	/* sets defaults */
	iperf_set_test_role(test, 'c');
	iperf_set_test_server_hostname(test, (char *) host);
	test->server_port = port;
	iperf_set_test_json_output(test, 1);

	if ( (ret = iperf_run(test)) < 0 ) {
		PyErr_Format(PyExc_RuntimeError,
				"error - %s", iperf_strerror(i_errno));
	}

	iperf_free_test(test);
	return ret;
}

int
iperf_run(struct iperf_test * test)
{
	int startup;
	int result = 0;
	fd_set read_set, write_set;
	struct timeval now;
	struct timeval* timeout = NULL;
	struct iperf_stream *sp;

	/* Termination signals. */
	iperf_catch_sigend(sigend_handler);
	if (setjmp(sigend_jmp_buf))
		iperf_got_sigend(test);

	if (test->affinity != -1)
		if (iperf_setaffinity(test, test->affinity) != 0)
			return -1;

	if (test->json_output)
		if (iperf_json_start(test) < 0)
			return -1;

	if (test->json_output) {
		cJSON_AddItemToObject(test->json_start, "version", cJSON_CreateString(version));
		cJSON_AddItemToObject(test->json_start, "system_info", cJSON_CreateString(get_system_info()));
	} else if (test->verbose) {
		iprintf(test, "%s\n", version);
		iprintf(test, "%s", "");
		iprintf(test, "%s\n", get_system_info());
		iflush(test);
	}

	/* Start the client and connect to the server */
	if (iperf_connect(test) < 0)
		return -1;

	/* Begin calculating CPU utilization */
	cpu_util(NULL);

	startup = 1;
	while (test->state != IPERF_DONE) {
		memcpy(&read_set, &test->read_set, sizeof(fd_set));
		memcpy(&write_set, &test->write_set, sizeof(fd_set));
		(void) gettimeofday(&now, NULL);
		timeout = tmr_timeout(&now);
		result = select(test->max_fd + 1, &read_set, &write_set, NULL, timeout);
		if (result < 0 && errno != EINTR) {
			i_errno = IESELECT;
			return -1;
		}
		if (result > 0) {
			if (FD_ISSET(test->ctrl_sck, &read_set)) {
				if (iperf_handle_message_client(test) < 0) {
					return -1;
				}
				FD_CLR(test->ctrl_sck, &read_set);
			}
		}

		if (test->state == TEST_RUNNING) {

			/* Is this our first time really running? */
			if (startup) {
				startup = 0;

				// Set non-blocking for non-UDP tests
				if (test->protocol->id != Pudp) {
					SLIST_FOREACH(sp, &test->streams, streams) {
						setnonblocking(sp->socket, 1);
					}
				}
			}

			if (test->reverse) {
				// Reverse mode. Client receives.
				if (iperf_recv(test, &read_set) < 0)
					return -1;
			} else {
				// Regular mode. Client sends.
				if (iperf_send(test, &write_set) < 0)
					return -1;
			}

			/* Run the timers. */
			(void) gettimeofday(&now, NULL);
			tmr_run(&now);

			/* Is the test done yet? */
			if ((!test->omitting) &&
					((test->duration != 0 && test->done) ||
					(test->settings->bytes != 0 && test->bytes_sent >= test->settings->bytes) ||
					(test->settings->blocks != 0 && test->blocks_sent >= test->settings->blocks))) {

				// Unset non-blocking for non-UDP tests
				if (test->protocol->id != Pudp) {
					SLIST_FOREACH(sp, &test->streams, streams) {
						setnonblocking(sp->socket, 0);
					}
				}

				/* Yes, done!  Send TEST_END. */
				test->done = 1;
				cpu_util(test->cpu_util);
				test->stats_callback(test);
				if (iperf_set_send_state(test, TEST_END) != 0)
					return -1;
			}
		}
		// If we're in reverse mode, continue draining the data
		// connection(s) even if test is over.  This prevents a
		// deadlock where the server side fills up its pipe(s)
		// and gets blocked, so it can't receive state changes
		// from the client side.
		else if (test->reverse && test->state == TEST_END) {
			if (iperf_recv(test, &read_set) < 0)
				return -1;
		}
	}

	if (test->json_output) {
		if (iperf_json_finish(test) < 0)
			return -1;
	} else {
		iprintf(test, "\n");
		iprintf(test, "%s", report_done);
	}

	iflush(test);

	return 0;
}

void
iperf_on_test_start(struct iperf_test *test)
{
	if (test->json_output) {
		cJSON_AddItemToObject(test->json_start,
				"test_start", iperf_json_printf("protocol: %s  num_streams: %d  blksize: %d  omit: %d  duration: %d  bytes: %d  blocks: %d  reverse: %d",
				test->protocol->name, (int64_t) test->num_streams, (int64_t) test->settings->blksize, (int64_t) test->omit, (int64_t) test->duration,
				(int64_t) test->settings->bytes, (int64_t) test->settings->blocks, test->reverse ? (int64_t) 1 : (int64_t) 0 ));
	} else {
		if (test->verbose) {
			if (test->settings->bytes) {
				iprintf(test, test_start_bytes, test->protocol->name, test->num_streams, test->settings->blksize, test->omit, test->settings->bytes);
			} else if (test->settings->blocks) {
				iprintf(test, test_start_blocks, test->protocol->name, test->num_streams, test->settings->blksize, test->omit, test->settings->blocks);
			} else {
				iprintf(test, test_start_time, test->protocol->name, test->num_streams, test->settings->blksize, test->omit, test->duration);
			}
		}
	}
}

void
iperf_on_connect(struct iperf_test *test)
{
	time_t now_secs;
	const char* rfc1123_fmt = "%a, %d %b %Y %H:%M:%S GMT";
	char now_str[100];
	char ipr[INET6_ADDRSTRLEN];
	int port;
	struct sockaddr_storage sa;
	struct sockaddr_in *sa_inP;
	struct sockaddr_in6 *sa_in6P;
	socklen_t len;
	int opt;

	now_secs = time((time_t*) 0);
	(void) strftime(now_str, sizeof(now_str), rfc1123_fmt, gmtime(&now_secs));
	if (test->json_output) {
		cJSON_AddItemToObject(test->json_start, "timestamp", iperf_json_printf("time: %s  timesecs: %d", now_str, (int64_t) now_secs));
	} else if (test->verbose) {
		iprintf(test, report_time, now_str);
	}

	if (test->role == 'c') {
		if (test->json_output) {
			cJSON_AddItemToObject(test->json_start,
					"connecting_to", iperf_json_printf("host: %s  port: %d", test->server_hostname, (int64_t) test->server_port));
		} else {
			iprintf(test, report_connecting, test->server_hostname, test->server_port);
			if (test->reverse) {
				iprintf(test, report_reverse, test->server_hostname);
			}
		}
  } else {
		len = sizeof(sa);
		getpeername(test->ctrl_sck, (struct sockaddr *) &sa, &len);
    if (getsockdomain(test->ctrl_sck) == AF_INET) {
			sa_inP = (struct sockaddr_in *) &sa;
			inet_ntop(AF_INET, &sa_inP->sin_addr, ipr, sizeof(ipr));
	    port = ntohs(sa_inP->sin_port);
    } else {
			sa_in6P = (struct sockaddr_in6 *) &sa;
			inet_ntop(AF_INET6, &sa_in6P->sin6_addr, ipr, sizeof(ipr));
			port = ntohs(sa_in6P->sin6_port);
		}
	mapped_v4_to_regular_v4(ipr);
	if (test->json_output) {
		cJSON_AddItemToObject(test->json_start, "accepted_connection", iperf_json_printf("host: %s  port: %d", ipr, (int64_t) port));
	} else {
	  iprintf(test, report_accepted, ipr, port);
  }
  if (test->json_output) {
		cJSON_AddStringToObject(test->json_start, "cookie", test->cookie);
    if (test->protocol->id == SOCK_STREAM) {
	    if (test->settings->mss) {
				cJSON_AddIntToObject(test->json_start, "tcp_mss", test->settings->mss);
			} else {
				len = sizeof(opt);
				getsockopt(test->ctrl_sck, IPPROTO_TCP, TCP_MAXSEG, &opt, &len);
				cJSON_AddIntToObject(test->json_start, "tcp_mss_default", opt);
	    }
		}
  } else if (test->verbose) {
		iprintf(test, report_cookie, test->cookie);
		if (test->protocol->id == SOCK_STREAM) {
			if (test->settings->mss) {
				iprintf(test, "      TCP MSS: %d\n", test->settings->mss);
			} else {
				len = sizeof(opt);
        getsockopt(test->ctrl_sck, IPPROTO_TCP, TCP_MAXSEG, &opt, &len);
        iprintf(test, "      TCP MSS: %d (default)\n", opt);
			}
		}
  }
}

void
iperf_on_new_stream(struct iperf_stream *sp)
{
	connect_msg(sp);
}

void
iperf_on_test_finish(struct iperf_test *test)
{
}

/**
 * Main report-printing callback.
 * Prints results either during a test (interval report only) or 
 * after the entire test has been run (last interval report plus 
 * overall summary).
 */
void
iperf_reporter_callback(struct iperf_test *test)
{
	switch (test->state) {
		case TEST_RUNNING:
		case STREAM_RUNNING:
			/* print interval results for each stream */
			iperf_print_intermediate(test);
			break;
		case DISPLAY_RESULTS:
			iperf_print_intermediate(test);
			iperf_print_results(test);
			break;
	}
}
