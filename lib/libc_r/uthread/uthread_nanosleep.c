/*
 * Copyright (c) 1995 John Birrell <jb@cimlogic.com.au>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by John Birrell.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOHN BIRRELL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $OpenBSD: uthread_nanosleep.c,v 1.4 1999/06/09 07:16:17 d Exp $
 */
#include <stdio.h>
#include <errno.h>
#ifdef _THREAD_SAFE
#include <pthread.h>
#include "pthread_private.h"

int
nanosleep(const struct timespec * time_to_sleep,
		  struct timespec * time_remaining)
{
	int             ret = 0;
	struct timespec start_time;
	struct timespec wakeup_time;
	struct timespec current_time;
	struct timespec remaining_time;
	struct timeval  tv;

	/* This is a cancellation point: */
	_thread_enter_cancellation_point();

	/* Check if the time to sleep is legal: */
	if (time_to_sleep == NULL || time_to_sleep->tv_nsec < 0 || time_to_sleep->tv_nsec > 1000000000 || time_to_sleep->tv_sec < 0) {
		/* Return an EINVAL error : */
		errno = EINVAL;
		ret = -1;
	} else {
		/* Get the current time: */
		gettimeofday(&tv, NULL);
		TIMEVAL_TO_TIMESPEC(&tv, &start_time);

		/* Calculate the time for the current thread to wake up: */
		timespecadd(time_to_sleep, &start_time, &wakeup_time);

		_thread_run->wakeup_time.tv_sec = wakeup_time.tv_sec;
		_thread_run->wakeup_time.tv_nsec = wakeup_time.tv_nsec;
		_thread_run->interrupted = 0;

		/* Reschedule the current thread to sleep: */
		_thread_kern_sched_state(PS_SLEEP_WAIT, __FILE__, __LINE__);

		/* Get the current time: */
		gettimeofday(&tv, NULL);
		TIMEVAL_TO_TIMESPEC(&tv, &current_time);

		/* Calculate the remaining time to sleep: */
		timespecsub(&wakeup_time, &current_time, &remaining_time);

		/* Check if the sleep was longer than the required time: */
		if (remaining_time.tv_sec < 0) {
			/* Reset the time left: */
			timespecclear(&remaining_time);
		}

		/* Check if the time remaining is to be returned: */
		if (time_remaining != NULL) {
			/* Return the actual time slept: */
			time_remaining->tv_sec = remaining_time.tv_sec;
			time_remaining->tv_nsec = remaining_time.tv_nsec;
		}

		/* Check if the sleep was interrupted: */
		if (_thread_run->interrupted) {
			/* Return an EINTR error : */
			errno = EINTR;
			ret = -1;
		}
	}

	/* No longer in a cancellation point: */
	_thread_leave_cancellation_point();

	return (ret);
}
#endif
