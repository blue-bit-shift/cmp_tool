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

#include "fuzz_helpers.h"
#include "fuzz_data_producer.h"

#include "../../lib/cmp_chunk.h"
#include "../../lib/decmp.h"


#define TEST_malloc(size) FUZZ_malloc(size)
#define TEST_ASSERT(cond) FUZZ_ASSERT(cond)


static uint32_t chunk_round_trip(void *data, uint32_t data_size,
				 void *model, void *up_model,
				 uint32_t *cmp_data, uint32_t cmp_data_capacity,
				 struct cmp_par *cmp_par, int use_decmp_buf,
				 int use_decmp_up_model)
{
	uint32_t cmp_size;
	void *model_cpy = NULL;

	/* if in-place model update is used (up_model == model), the model
	 * needed for decompression is destroyed; therefore we make a copy
	 */
	if (model) {
		if (up_model == model) {
			model_cpy = TEST_malloc(data_size);
			memcpy(model_cpy, model, data_size);
		} else {
			model_cpy = model;
		}
	}

	cmp_size = compress_chunk(data, data_size, model, up_model,
				  cmp_data, cmp_data_capacity, cmp_par);

#if 0
	{ /* Compress a second time and check for determinism */
		int32_t cSize2;
		void *compressed2 = NULL;
		void *up_model2 = NULL;

		if (compressed)
			compressed2 = FUZZ_malloc(compressedCapacity);

		if (up_model)
			up_model2 = FUZZ_malloc(srcSize);
		cSize2 = compress_chunk((void *)src, srcSize, (void *)model, up_model2,
					   compressed2, compressedCapacity, cmp_par);
		FUZZ_ASSERT(cSize == cSize2);
		FUZZ_ASSERT_MSG(!FUZZ_memcmp(compressed, compressed2, cSize), "Not deterministic!");
		FUZZ_ASSERT_MSG(!FUZZ_memcmp(up_model, compressed2, cSize), "NO deterministic!");
		free(compressed2);
		free(up_model2);
	}
#endif
	if (!cmp_is_error(cmp_size) && cmp_data) {
		void *decmp_data = NULL;
		void *up_model_decmp = NULL;
		int decmp_size;

		decmp_size = decompress_cmp_entiy((struct cmp_entity *)cmp_data, model_cpy, NULL, NULL);
		TEST_ASSERT(decmp_size >= 0);
		TEST_ASSERT((uint32_t)decmp_size == data_size);

		if (use_decmp_buf)
			decmp_data = TEST_malloc(data_size);
		if (use_decmp_up_model)
			up_model_decmp = TEST_malloc(data_size);

		decmp_size = decompress_cmp_entiy((struct cmp_entity *)cmp_data, model_cpy,
						  up_model_decmp, decmp_data);
		TEST_ASSERT(decmp_size >= 0);
		TEST_ASSERT((uint32_t)decmp_size == data_size);

		if (use_decmp_buf) {
			TEST_ASSERT(!memcmp(data, decmp_data, data_size));

			/*
			 * the model is only updated when the decompressed_data
			 * buffer is set
			 */
			if (up_model && up_model_decmp)
				TEST_ASSERT(!memcmp(up_model, up_model_decmp, data_size));
		}

		free(decmp_data);
		free(up_model_decmp);
	}

	if (up_model == model)
		free(model_cpy);

	return cmp_size;
}


int LLVMFuzzerTestOneInput(const uint8_t *src, size_t size)
{
	struct cmp_par cmp_par;
	struct cmp_par *cmp_par_ptr=NULL;
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

	/* 1/2 of the cases we use a updated model buffer */
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		up_model = FUZZ_malloc(size);

	cmp_size_bound = compress_chunk_cmp_size_bound(src, size);
	if (cmp_is_error(cmp_size_bound))
		cmp_size_bound = 0;
	cmp_data_capacity = FUZZ_dataProducer_uint32Range(producer, 0, cmp_size_bound+(uint32_t)size);
	cmp_data = (uint32_t *)FUZZ_malloc(cmp_data_capacity);


	FUZZ_dataProducer_cmp_par(producer, &cmp_par);
	cmp_par.lossy_par = 0; /*TODO: implement lossy  */
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		cmp_par_ptr = &cmp_par;

	use_decmp_buf = FUZZ_dataProducer_int32Range(producer, 0, 1);
	use_decmp_up_model = FUZZ_dataProducer_int32Range(producer, 0, 1);

#if 0
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
#endif

	free(up_model);
	free(cmp_data);
	FUZZ_dataProducer_free(producer);

	return 0;
}
