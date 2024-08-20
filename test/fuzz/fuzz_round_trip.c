/**
 * @file fuzz_round_trip.c
 * @date 2024
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
 * @brief chunk compression/decompression fuzz target
 *
 * This fuzzer tests the (de)compression functionality with random data/model
 * and parameters. It uses a random portion of the input data for parameter
 * generation, while the rest is used for compression. If the compression
 * succeeds, the data are decompressed and checked to see whether they match the
 * input data.
 */

#include <string.h>
#include <stdlib.h>

#include "fuzz_helpers.h"
#include "fuzz_data_producer.h"
#include "../test_common/chunk_round_trip.h"
#include "../test_common/test_common.h"


int LLVMFuzzerTestOneInput(const uint8_t *src, size_t size)
{
	struct cmp_par cmp_par;
	struct cmp_par *cmp_par_ptr = NULL;
	const uint8_t *model = NULL;
	void *up_model = NULL;
	uint32_t cmp_size_bound;
	uint32_t *cmp_data;
	uint32_t cmp_data_capacity;
	uint32_t return_value;
	int use_decmp_buf;
	int use_decmp_up_model;

	/* Give a random portion of src data to the producer, to use for
	   parameter generation. The rest will be used for (de)compression */
	FUZZ_dataProducer_t *producer = (FUZZ_dataProducer_t *)FUZZ_dataProducer_create(src, size);
	size = FUZZ_dataProducer_reserveDataPrefix(producer);

	/* 1/2 of the cases we use a model */
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1) && size > 2) {
		model = src + size/2;
		size /= 2;
	}
	FUZZ_ASSERT(size <= UINT32_MAX);

	/* generate compression parameters */
	FUZZ_dataProducer_cmp_par(producer, &cmp_par);
	cmp_par.lossy_par = 0; /*TODO: implement lossy  */
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		cmp_par_ptr = &cmp_par;

	/* 1/2 of the cases we use a updated model buffer */
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1)) {
		up_model = TEST_malloc(size);
		if (!model_mode_is_used(cmp_par.cmp_mode))
			memset(up_model, 0, size); /* up_model is not used */
	}

	cmp_size_bound = compress_chunk_cmp_size_bound(src, size);
	if (cmp_is_error(cmp_size_bound))
		cmp_size_bound = 0;
	cmp_data_capacity = FUZZ_dataProducer_uint32Range(producer, 0, cmp_size_bound+(uint32_t)size);
	cmp_data = (uint32_t *)TEST_malloc(cmp_data_capacity);

	use_decmp_buf = FUZZ_dataProducer_int32Range(producer, 0, 1);
	use_decmp_up_model = FUZZ_dataProducer_int32Range(producer, 0, 1);

	return_value = chunk_round_trip(src, size, model, up_model, cmp_data,
				        cmp_data_capacity, cmp_par_ptr,
				        use_decmp_buf, use_decmp_up_model);
	switch (cmp_get_error_code(return_value)) {
	case CMP_ERROR_NO_ERROR:
	case CMP_ERROR_GENERIC:
	case CMP_ERROR_SMALL_BUF_:
	/* compression parameter errors */
	case CMP_ERROR_PAR_GENERIC:
	case CMP_ERROR_PAR_SPECIFIC:
	case CMP_ERROR_PAR_BUFFERS:
	case CMP_ERROR_PAR_MAX_USED_BITS:
	case CMP_ERROR_PAR_NULL:
	/* chunk errors */
	case CMP_ERROR_CHUNK_NULL:
	case CMP_ERROR_CHUNK_TOO_LARGE:
	case CMP_ERROR_CHUNK_TOO_SMALL:
	case CMP_ERROR_CHUNK_SIZE_INCONSISTENT:
	case CMP_ERROR_CHUNK_SUBSERVICE_INCONSISTENT:
	/* collection errors */
	case CMP_ERROR_COL_SUBSERVICE_UNSUPPORTED:
	case CMP_ERROR_COL_SIZE_INCONSISTENT:
		break;
	/* compression entity errors */
	case CMP_ERROR_ENTITY_NULL:
		FUZZ_ASSERT(0);
	case CMP_ERROR_ENTITY_TOO_SMALL:
		FUZZ_ASSERT(0);
	case CMP_ERROR_ENTITY_HEADER:
		break;
	case CMP_ERROR_ENTITY_TIMESTAMP:
		FUZZ_ASSERT(0);
	/* internal compressor errors */
	case CMP_ERROR_INT_DECODER:
		FUZZ_ASSERT(0);
	case CMP_ERROR_INT_DATA_TYPE_UNSUPPORTED:
		FUZZ_ASSERT(0);
	case CMP_ERROR_INT_CMP_COL_TOO_LARGE:
		FUZZ_ASSERT(0);

	case CMP_ERROR_DATA_VALUE_TOO_LARGE:
		FUZZ_ASSERT(0);
	default:
		FUZZ_ASSERT(0);
	}

	free(up_model);
	free(cmp_data);
	FUZZ_dataProducer_free(producer);

	return 0;
}
