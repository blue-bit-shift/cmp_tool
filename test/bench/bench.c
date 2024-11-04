/**
 * @file   bench.c
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2024
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief software compression speed test bench
 */


/*_************************************
 *  Includes
 **************************************/
#include <stdlib.h> /* malloc */
#include <stdio.h> /* fprintf, fopen */
#include <string.h>

#include "benchfn.h"

#include <cmp_chunk.h>
#include <decmp.h>
#include <cmp_data_types.h>

#include "ref_short_cadence_1_cmp.h"
#include "ref_short_cadence_2_cmp.h"

/*_************************************
*  Constants
**************************************/
#ifdef __sparc__
#  define NBLOOPS 1
#else
#  define NBLOOPS 6
#endif

#define TIMELOOP_NANOSEC (1 * 1000000000ULL) /* 1 second */
#define MB_UNIT 1000000

enum bench_name {MEMCPY_BENCH, CMP_CHUNK_BENCH = 32};

/* TODO: replace with default config? */
const struct cmp_par DIFF_CMP_PAR = {
	CMP_MODE_DIFF_ZERO, /* cmp_mode cmp_mode */
	11, /* model_value */
	0, /* lossy_par */

	1, /* nc_imagette */

	1, /* s_exp_flags */
	1, /* s_fx */
	1, /* s_ncob */
	1, /* s_efx */
	1, /* s_ecob */

	1, /* l_exp_flags */
	1, /* l_fx */
	1, /* l_ncob */
	1, /* l_efx */
	1, /* l_ecob */
	1, /* l_fx_cob_variance */

	1, /* saturated_imagette */

	1, /* nc_offset_mean */
	1, /* nc_offset_variance */
	1, /* nc_background_mean */
	1, /* nc_background_variance */
	1, /* nc_background_outlier_pixels */

	1, /* smearing_mean */
	1, /* smearing_variance_mean */
	1, /* smearing_outlier_pixels */

	1, /* fc_imagette */
	1, /* fc_offset_mean */
	1, /* fc_offset_variance */
	1, /* fc_background_mean */
	1, /* fc_background_variance */
	1  /* fc_background_outlier_pixels */
};


const struct cmp_par MODEL_CMP_PAR = {
	CMP_MODE_MODEL_MULTI, /* cmp_mode cmp_mode */
	11, /* model_value */
	0, /* lossy_par */

	1, /* nc_imagette */

	1, /* s_exp_flags */
	1, /* s_fx */
	1, /* s_ncob */
	1, /* s_efx */
	1, /* s_ecob */

	1, /* l_exp_flags */
	1, /* l_fx */
	1, /* l_ncob */
	1, /* l_efx */
	1, /* l_ecob */
	1, /* l_fx_cob_variance */

	1, /* saturated_imagette */

	1, /* nc_offset_mean */
	1, /* nc_offset_variance */
	1, /* nc_background_mean */
	1, /* nc_background_variance */
	1, /* nc_background_outlier_pixels */

	1, /* smearing_mean */
	1, /* smearing_variance_mean */
	1, /* smearing_outlier_pixels */

	1, /* fc_imagette */
	1, /* fc_offset_mean */
	1, /* fc_offset_variance */
	1, /* fc_background_mean */
	1, /* fc_background_variance */
	1  /* fc_background_outlier_pixels */
};


/*_************************************
 *  Macros
 **************************************/
__extension__
#define DISPLAY(...) fprintf(stdout, __VA_ARGS__)

#define CONTROL(c)               \
	{                        \
		if (!(c)) {      \
			abort(); \
		}                \
	} /* like assert(), but cannot be disabled */


/*_************************************
 *  Benchmark Parameters
 **************************************/
static unsigned int g_nbIterations = NBLOOPS;

/*_*******************************************************
 *  Private functions
 *********************************************************/

/*_*******************************************************
 *  Benchmark wrappers
 *********************************************************/

static size_t local_memcpy(const void *src, size_t srcSize,
			   const void *model UNUSED, void *upmodel UNUSED,
			   void *dst, size_t dstSize UNUSED, void *payload UNUSED)
{
	memcpy(dst, src, srcSize);
	return srcSize;
}

static size_t local_compress_chunk(const void *src, size_t srcSize, const void *model,
				   void *upmodel, void *dst, size_t dstSize, void *payload)
{
	struct cmp_par *par = (struct cmp_par *)payload;

	return compress_chunk(src, (uint32_t)srcSize, model, upmodel,
			      dst, (uint32_t)dstSize, par);
}


static unsigned int is_error(size_t return_val)
{
	if ((int32_t)return_val > 0)
		return 0;
	return 1;
}


/*_*******************************************************
 *  Bench functions
 *********************************************************/
static int bench_mem(unsigned int benchNb, const void *src, size_t srcSize,
		     const void *model,  void *up_model,
		     void *dstBuff, size_t dstBuffSize, void *payload)
{
	const char *bench_name;
	BMK_benchFn_t bench_function;

	/* Selection */
	switch (benchNb) {
	case MEMCPY_BENCH:
		bench_name = "memcpy";
		bench_function = local_memcpy;
		break;
	case CMP_CHUNK_BENCH:
		bench_name = "compress_chunk";
		bench_function = local_compress_chunk;
		break;
	default:
		return 0;
	}

	/* benchmark loop */
	{
		BMK_timedFnState_t *const tfs = BMK_createTimedFnState(g_nbIterations * 1000, 1000);
		void *const avoidStrictAliasingPtr = &dstBuff;
		BMK_benchParams_t bp;
		BMK_runTime_t bestResult;

		bestResult.sumOfReturn = 0;
		bestResult.nanoSecPerRun = (double)TIMELOOP_NANOSEC * 2000000000; /* hopefully large enough : must be larger than any potential measurement */
		CONTROL(tfs != NULL);

		bp.benchFn = bench_function;
		bp.benchPayload = payload;
		bp.initFn = NULL;
		bp.initPayload = NULL;
		bp.errorFn = is_error;
		bp.blockCount = 1;
		bp.srcBuffers = &src;
		bp.srcSizes = &srcSize;
		bp.modelBuffers = &model;
		bp.upmodelBuffers = &up_model;
		bp.dstBuffers = (void *const *) avoidStrictAliasingPtr; /* circumvent strict aliasing warning on gcc-8, because gcc considers that `void *const *`  and `void**` are 2 different types */
		bp.dstCapacities = &dstBuffSize;
		bp.blockResults = NULL;

		while (1) {
			BMK_runOutcome_t const bOutcome = BMK_benchTimedFn(tfs, bp);
			BMK_runTime_t newResult;

			if (!BMK_isSuccessful_runOutcome(bOutcome)) {
				DISPLAY("ERROR benchmarking function ! !\n");
				return 1;
			}

			newResult = BMK_extract_runTime(bOutcome);
			if (newResult.nanoSecPerRun < bestResult.nanoSecPerRun)
				bestResult.nanoSecPerRun = newResult.nanoSecPerRun;
			DISPLAY("\r%2u#%-29.29s:%8.2f MB/s  (%8u) ", benchNb, bench_name,
				(double)srcSize * TIMELOOP_NANOSEC / bestResult.nanoSecPerRun / MB_UNIT,
				(unsigned int)newResult.sumOfReturn);

			if (BMK_isCompleted_TimedFn(tfs))
				break;
		}
		BMK_freeTimedFnState(tfs);
	}
	DISPLAY("\n");

	return 0;
}


static int bench_ref_data(void)
{
	int i, d, err = -1;
	enum {
		SHORT_CADENCE,
		NB_DATA_SETS
	};
#ifdef __sparc__
	void *data =            (void *)0x63000000;
	void *model =           (void *)0x64000000;
	void *updated_model =   (void *)0x65000000;
	void *compressed_data = (void *)0x66000000;
#else
	void *data = malloc(0x1000000);
	void *model = malloc(0x1000000);
	void *updated_model = malloc(0x1000000);
	void *compressed_data = malloc(0x1000000);
#endif

	if (!data || !model || !updated_model || !compressed_data) {
		DISPLAY("\nError: not enough memory!\n");
		err = 12;
		goto fail;
	}

	for (d = 0; d < NB_DATA_SETS; d++) {
		const char *data_set_name;
		size_t size = 0;
		uint32_t dst_capacity = 0;
		int decmp_size;

		switch (d) {
		case SHORT_CADENCE:
			data_set_name = "short cadence (1.4MB)";

			/* reference data are stored compressed */
			decmp_size = decompress_cmp_entiy((const void *)ref_short_cadence_1_cmp,
							  NULL, NULL, model);
			CONTROL(decmp_size > 0);
			size = (size_t)decmp_size;
			decmp_size = decompress_cmp_entiy((const void *)ref_short_cadence_2_cmp,
							  model, NULL, data);
			CONTROL(decmp_size == (int)size);

			dst_capacity = compress_chunk_cmp_size_bound(data, size);
			if (cmp_is_error(dst_capacity)) {
				err = (int)cmp_get_error_code(dst_capacity);
				goto fail;
			}
			break;
		default:
			err = -1;
			goto fail;
		}

		CONTROL(size < 0x1000000);
		CONTROL(dst_capacity < 0x1000000);

		DISPLAY("%s: memcpy\n", data_set_name);
		err = bench_mem(MEMCPY_BENCH, data, size, model, updated_model,
				compressed_data, dst_capacity, NULL);
		if (err) {
			DISPLAY("\nError: benchMem() failed!\n");
			err = -1;
			goto fail;
		}

		for (i = 0; i < 10; ++i) {
			struct cmp_par par;
			const char *setup_name;

			switch (i) {
			case 0:
				setup_name = "raw mode";

				par = DIFF_CMP_PAR;
				par.cmp_mode = CMP_MODE_RAW;
				break;
			case 1:
				setup_name = "1d-diff zero";
				par = DIFF_CMP_PAR;
				par.cmp_mode = CMP_MODE_DIFF_ZERO;
				break;
			case 2:
				setup_name = "1d-diff multi";
				par = DIFF_CMP_PAR;
				par.cmp_mode = CMP_MODE_DIFF_MULTI;
				break;
			case 3:
				setup_name = "model zero";
				par = MODEL_CMP_PAR;
				par.cmp_mode = CMP_MODE_MODEL_ZERO;
				break;
			case 4:
				setup_name = "model multi";
				par = MODEL_CMP_PAR;
				par.cmp_mode = CMP_MODE_MODEL_MULTI;
				break;
			default:
				continue;
			}

			DISPLAY("%s: %s\n", data_set_name, setup_name);
			err = bench_mem(CMP_CHUNK_BENCH, data, size, model, updated_model,
					compressed_data, dst_capacity, &par);
			if (err) {
				DISPLAY("\nError: benchMem() failed!\n");
				goto fail;
			}
		}
	}
fail:
#ifndef __sparc__
	free(data);
	free(model);
	free(updated_model);
	free(compressed_data);
#endif
	return err;
}


int main(void)
{
	return bench_ref_data();
}
