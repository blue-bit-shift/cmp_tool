/**
 * @file fuzz_copression.c
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
 * @brief chunk compression fuzz target
 *
 * This fuzzer tests the compression functionality with random data/model and
 * parameters. It uses a random portion of the input data for parameter
 * generation, while the rest is used for compression.
 */


#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "fuzz_helpers.h"
#include "fuzz_data_producer.h"

#include "../../lib/cmp_chunk.h"



int LLVMFuzzerTestOneInput(const uint8_t *src, size_t size)
{
	struct cmp_par cmp_par;
	struct cmp_par *cmp_par_ptr = NULL;
	const uint8_t *model = NULL;
	void *up_model;
	uint32_t *cmp_data;
	uint32_t cmp_data_capacity;
	int use_a_upmodel;
	uint32_t cmp_size_bound;
	uint32_t return_value;

	/* Give a random portion of src data to the producer, to use for
	   parameter generation. The rest will be used for data/model */
	FUZZ_dataProducer_t *producer = (FUZZ_dataProducer_t *)FUZZ_dataProducer_create(src, size);
	size = FUZZ_dataProducer_reserveDataPrefix(producer);

	FUZZ_dataProducer_cmp_par(producer, &cmp_par);

	/* 1/2 of the cases we use a model */
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1) && size > 2) {
		model = src + size/2;
		size /= 2;
	}
	FUZZ_ASSERT(size <= UINT32_MAX);

	cmp_size_bound = compress_chunk_cmp_size_bound(src, size);
	if (cmp_is_error(cmp_size_bound))
		cmp_size_bound = 0;
	cmp_data_capacity = FUZZ_dataProducer_uint32Range(producer, 0, cmp_size_bound+(uint32_t)size);
	cmp_data = (uint32_t *)FUZZ_malloc(cmp_data_capacity);

	FUZZ_dataProducer_cmp_par(producer, &cmp_par);
	cmp_par.lossy_par = 0; /*TODO: implement lossy  */
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		cmp_par_ptr = &cmp_par;

	use_a_upmodel = FUZZ_dataProducer_int32Range(producer, 0, 2);
	switch (use_a_upmodel) {
	case 0:
		up_model = NULL;
		break;
	case 1:
		up_model = FUZZ_malloc(size);
		break;
	case 2:
		up_model = FUZZ_malloc(size);
		if (model && up_model) {
			memcpy(up_model, model, size);
			model = up_model; /* in-place update */
		}
		break;
	default:
		FUZZ_ASSERT(0);
	}


	return_value = compress_chunk((void *)src, size, (void *)model, up_model,
				      cmp_data, cmp_data_capacity, cmp_par_ptr);

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

	free(cmp_data);
	free(up_model);
	FUZZ_dataProducer_free(producer);
	return 0;
}

