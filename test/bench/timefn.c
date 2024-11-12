/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE.BSD-3.Zstandard file in the 3rdparty_licenses directory) and the GPLv2
 * (found in the LICENSE.GPL-2 file in the 3rdparty_licenses directory).
 * You may select, at your option, one of the above-listed licenses.
 */

/*
 * Modifications made by
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date 2024
 *
 * - Added support for the LEON3 processor.
 * - Modified the code to comply with the Linux kernel coding style.
 *
 * Modifications are also licensed under the same license for consistency
 */


/* ===  Dependencies  === */
#include "timefn.h"
#ifdef __has_include
#  if __has_include(<time.h>)
#    include <time.h>  /* CLOCK_MONOTONIC, TIME_UTC */
#  endif
#endif

/*-****************************************
*  Time functions
******************************************/

#if defined(__sparc__) /* LEON3 processor */

#include <stdlib.h> /* abort */
#include "leon3_grtimer_longcount.h"

/* timer config */
#define CPU_CPS 80000000 /* in Hz */
#define GRTIMER_RELOAD 4
#define GRTIMER_MAX 0xffffffff
#define GRTIMER_TICKS_PER_SEC ((CPU_CPS / (GRTIMER_RELOAD + 1)))

UTIL_time_t UTIL_getTime(void)
{
	static struct grtimer_unit *rtu = (struct grtimer_unit *)LEON3_BASE_ADDRESS_GRTIMER;
	static struct grtimer_uptime start;
	static int init;

	if (!init) {
		int32_t const err = grtimer_longcount_start(rtu, GRTIMER_RELOAD,
						GRTIMER_TICKS_PER_SEC, GRTIMER_MAX);
		if (err)
			abort();

		grtimer_longcount_get_uptime(rtu, &start);
		init = 1;
	}
	{
		UTIL_time_t r;
		struct grtimer_uptime up;

		grtimer_longcount_get_uptime(rtu, &up);
		r.t = (PTime)(grtimer_longcount_difftime(rtu, up, start) * 1000000000ULL);
		return r;
	}
}

#elif defined(_WIN32) /* Windows */

#include <windows.h> /* LARGE_INTEGER */
#include <stdlib.h> /* abort */
#include <stdio.h> /* perror */

UTIL_time_t UTIL_getTime(void)
{
	static LARGE_INTEGER ticksPerSecond;
	static int init;

	if (!init) {
		if (!QueryPerformanceFrequency(&ticksPerSecond)) {
			perror("timefn::QueryPerformanceFrequency");
			abort();
		}
		init = 1;
	}
	{
		UTIL_time_t r;
		LARGE_INTEGER x;

		QueryPerformanceCounter(&x);
		r.t = (PTime)(x.QuadPart * 1000000000ULL / ticksPerSecond.QuadPart);
		return r;
	}
}

#elif defined(__APPLE__) && defined(__MACH__)

#include <mach/mach_time.h> /* mach_timebase_info_data_t, mach_timebase_info, mach_absolute_time */

UTIL_time_t UTIL_getTime(void)
{
	static mach_timebase_info_data_t rate;
	static int init;

	if (!init) {
		mach_timebase_info(&rate);
		init = 1;
	}
	{
		UTIL_time_t r;

		r.t = mach_absolute_time() * (PTime)rate.numer /
		      (PTime)rate.denom;
		return r;
	}
}

/* POSIX.1-2001 (optional) */
#elif defined(CLOCK_MONOTONIC)

#include <stdlib.h> /* abort */
#include <stdio.h> /* perror */

UTIL_time_t UTIL_getTime(void)
{
	/*
	 * time must be initialized, othersize it may fail msan test.  No good
	 * reason, likely a limitation of timespec_get() for some target
	 */
	struct timespec time = { 0, 0 };

	if (clock_gettime(CLOCK_MONOTONIC, &time) != 0) {
		perror("timefn::clock_gettime(CLOCK_MONOTONIC)");
		abort();
	}
	{
		UTIL_time_t r;

		r.t = (PTime)time.tv_sec * 1000000000ULL + (PTime)time.tv_nsec;
		return r;
	}
}

/*
 * C11 requires support of timespec_get().
 * However, FreeBSD 11 claims C11 compliance while lacking timespec_get().
 * Double confirm timespec_get() support by checking the definition of TIME_UTC.
 * However, some versions of Android manage to simultaneously define TIME_UTC
 * and lack timespec_get() support...
 */
#elif (defined(__STDC_VERSION__) &&			\
	(__STDC_VERSION__ >= 201112L) /* C11 */) &&	\
	defined(TIME_UTC) && !defined(__ANDROID__)

#include <stdlib.h> /* abort */
#include <stdio.h> /* perror */

UTIL_time_t UTIL_getTime(void)
{
	/*
	 * time must be initialized, othersize it may fail msan test.  No good
	 * reason, likely a limitation of timespec_get() for some target
	 */
	struct timespec time = { 0, 0 };

	if (timespec_get(&time, TIME_UTC) != TIME_UTC) {
		perror("timefn::timespec_get(TIME_UTC)");
		abort();
	}
	{
		UTIL_time_t r;

		r.t = (PTime)time.tv_sec * 1000000000ULL + (PTime)time.tv_nsec;
		return r;
	}
}

#else /* relies on standard C90 (note : clock_t produces wrong measurements for multi-threaded workloads) */

UTIL_time_t UTIL_getTime(void)
{
	UTIL_time_t r;

	r.t = (PTime)clock() * 1000000000ULL / CLOCKS_PER_SEC;
	return r;
}

#define TIME_MT_MEASUREMENTS_NOT_SUPPORTED

#endif

/* ==== Common functions, valid for all time API ==== */

PTime UTIL_getSpanTimeNano(UTIL_time_t clockStart, UTIL_time_t clockEnd)
{
	return clockEnd.t - clockStart.t;
}

PTime UTIL_getSpanTimeMicro(UTIL_time_t begin, UTIL_time_t end)
{
	return UTIL_getSpanTimeNano(begin, end) / 1000ULL;
}

PTime UTIL_clockSpanMicro(UTIL_time_t clockStart)
{
	UTIL_time_t const clockEnd = UTIL_getTime();

	return UTIL_getSpanTimeMicro(clockStart, clockEnd);
}

PTime UTIL_clockSpanNano(UTIL_time_t clockStart)
{
	UTIL_time_t const clockEnd = UTIL_getTime();

	return UTIL_getSpanTimeNano(clockStart, clockEnd);
}

void UTIL_waitForNextTick(void)
{
	UTIL_time_t const clockStart = UTIL_getTime();
	UTIL_time_t clockEnd;

	do {
		clockEnd = UTIL_getTime();
	} while (UTIL_getSpanTimeNano(clockStart, clockEnd) == 0);
}

int UTIL_support_MT_measurements(void)
{
#if defined(TIME_MT_MEASUREMENTS_NOT_SUPPORTED)
	return 0;
#else
	return 1;
#endif
}
