/**
 * @file fuzz_decompression.c
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
 * @brief decompression fuzz target
 */


#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "fuzz_helpers.h"
#include "fuzz_data_producer.h"

#include "../../lib/decmp.h"


int decompress_cmp_entiy_save(const struct cmp_entity *ent, size_t ent_size, const void *model_of_data,
			 void *up_model_buf, void *decompressed_data, size_t decmp_size)
{
	if (ent && ent_size < GENERIC_HEADER_SIZE)
		return -1;
	if (cmp_ent_get_size(ent) > ent_size)
		return -1;

	if (ent && (decompressed_data || up_model_buf)) {
		int decmp_size_ent = decompress_cmp_entiy(ent, model_of_data, NULL, NULL);

		if (decmp_size < (size_t)decmp_size_ent || decmp_size_ent < 0)
			return -1;
	}

	return decompress_cmp_entiy(ent, model_of_data, up_model_buf, decompressed_data);
}

int LLVMFuzzerTestOneInput(const uint8_t *src, size_t size)
{
	const struct cmp_entity *ent = NULL;
	const void *model_of_data = NULL;
	void *up_model_buf;
	uint32_t model_of_data_size;
	uint32_t ent_size;
	void *decompressed_data;

	/*
	 * Give a random portion of src data to the producer, to use for
	 * parameter generation. The rest will be used for data/model
	 */
	FUZZ_dataProducer_t *producer = (FUZZ_dataProducer_t *)FUZZ_dataProducer_create(src, size);

	size = FUZZ_dataProducer_reserveDataPrefix(producer);
	FUZZ_ASSERT(size <= UINT32_MAX);

	/* spilt data to compressed data and model data */
	ent_size = FUZZ_dataProducer_uint32Range(producer, 0, (uint32_t)size);
	model_of_data_size = FUZZ_dataProducer_uint32Range(producer, 0, (uint32_t)size-ent_size);

	if (ent_size)
		ent = (const struct cmp_entity *)src;
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		model_of_data = src + ent_size;
	else
		model_of_data = NULL;


	switch (FUZZ_dataProducer_int32Range(producer, 0, 2)) {
	case 0:
		up_model_buf = NULL;
		break;
	case 1:
		up_model_buf = FUZZ_malloc(model_of_data_size);
		break;
	case 2: /* in-place update */
		up_model_buf = FUZZ_malloc(model_of_data_size);
		if (model_of_data && up_model_buf) {
			memcpy(up_model_buf, model_of_data, model_of_data_size);
			model_of_data = up_model_buf;
		}
		break;
	default:
		FUZZ_ASSERT(0);
	}

	decompressed_data = FUZZ_malloc((size_t)model_of_data_size);
	decompress_cmp_entiy_save(ent, ent_size, model_of_data, up_model_buf, decompressed_data, model_of_data_size);

	free(up_model_buf);
	free(decompressed_data);
	FUZZ_dataProducer_free(producer);

	return 0;
}
