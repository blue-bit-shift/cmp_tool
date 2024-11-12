/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE.BSD-3.Zstandard file in the 3rdparty_licenses directory) and the GPLv2
 * (found in the LICENSE.GPL-2 file in the 3rdparty_licenses directory).
 * You may select, at your option, one of the above-listed licenses.
 */

/**
 * Modifications made by
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date 2024
 * @see https://github.com/facebook/zstd/issues/1723
 *
 * - Added function providing a cmp_par struct
 *
 * Modifications are also licensed under the same license for consistency
 *
 */


#include "fuzz_helpers.h"
#include "fuzz_data_producer.h"
#include <cmp_chunk.h>

struct FUZZ_dataProducer_s {
	const uint8_t *data;
	size_t size;
};

FUZZ_dataProducer_t *FUZZ_dataProducer_create(const uint8_t *data, size_t size)
{
	FUZZ_dataProducer_t *producer = FUZZ_malloc(sizeof(FUZZ_dataProducer_t));

	producer->data = data;
	producer->size = size;
	return producer;
}

void FUZZ_dataProducer_free(FUZZ_dataProducer_t *producer)
{
	free(producer);
}

uint32_t FUZZ_dataProducer_uint32Range(FUZZ_dataProducer_t *producer, uint32_t min,
				       uint32_t max)
{
	uint32_t range = max - min;
	uint32_t rolling = range;
	uint32_t result = 0;

	FUZZ_ASSERT(min <= max);

	while (rolling > 0 && producer->size > 0) {
		uint8_t next = *(producer->data + producer->size - 1);

		producer->size -= 1;
		result = (result << 8) | next;
		rolling >>= 8;
	}

	if (range == 0xffffffff)
		return result;

	return min + result % (range + 1);
}

uint32_t FUZZ_dataProducer_uint32(FUZZ_dataProducer_t *producer)
{
	return FUZZ_dataProducer_uint32Range(producer, 0, 0xffffffff);
}

int32_t FUZZ_dataProducer_int32Range(FUZZ_dataProducer_t *producer,
				     int32_t min, int32_t max)
{
	FUZZ_ASSERT(min <= max);

	if (min < 0)
		return (int)FUZZ_dataProducer_uint32Range(producer, 0, max - min) + min;

	return FUZZ_dataProducer_uint32Range(producer, min, max);
}

size_t FUZZ_dataProducer_remainingBytes(FUZZ_dataProducer_t *producer)
{
	return producer->size;
}

void FUZZ_dataProducer_rollBack(FUZZ_dataProducer_t *producer, size_t remainingBytes)
{
	FUZZ_ASSERT(remainingBytes >= producer->size);
	producer->size = remainingBytes;
}

int FUZZ_dataProducer_empty(FUZZ_dataProducer_t *producer)
{
	return producer->size == 0;
}

size_t FUZZ_dataProducer_contract(FUZZ_dataProducer_t *producer, size_t newSize)
{
	size_t remaining;

	newSize = newSize > producer->size ? producer->size : newSize;
	remaining = producer->size - newSize;
	producer->data = producer->data + remaining;
	producer->size = newSize;
	return remaining;
}

size_t FUZZ_dataProducer_reserveDataPrefix(FUZZ_dataProducer_t *producer)
{
	size_t producerSliceSize = FUZZ_dataProducer_uint32Range(
								 producer, 0, producer->size);
	return FUZZ_dataProducer_contract(producer, producerSliceSize);
}

void FUZZ_dataProducer_cmp_par(FUZZ_dataProducer_t *producer, struct cmp_par *cmp_par)
{
	cmp_par->cmp_mode = (enum cmp_mode)FUZZ_dataProducer_uint32(producer);
	cmp_par->model_value = FUZZ_dataProducer_uint32(producer);
	cmp_par->lossy_par = FUZZ_dataProducer_uint32(producer);

	cmp_par->nc_imagette = FUZZ_dataProducer_uint32(producer);

	cmp_par->s_exp_flags = FUZZ_dataProducer_uint32(producer);
	cmp_par->s_fx = FUZZ_dataProducer_uint32(producer);
	cmp_par->s_ncob = FUZZ_dataProducer_uint32(producer);
	cmp_par->s_efx = FUZZ_dataProducer_uint32(producer);
	cmp_par->s_ecob = FUZZ_dataProducer_uint32(producer);

	cmp_par->l_exp_flags = FUZZ_dataProducer_uint32(producer);
	cmp_par->l_fx = FUZZ_dataProducer_uint32(producer);
	cmp_par->l_ncob = FUZZ_dataProducer_uint32(producer);
	cmp_par->l_efx = FUZZ_dataProducer_uint32(producer);
	cmp_par->l_ecob = FUZZ_dataProducer_uint32(producer);
	cmp_par->l_fx_cob_variance = FUZZ_dataProducer_uint32(producer);

	cmp_par->saturated_imagette = FUZZ_dataProducer_uint32(producer);

	cmp_par->nc_offset_mean = FUZZ_dataProducer_uint32(producer);
	cmp_par->nc_offset_variance = FUZZ_dataProducer_uint32(producer);
	cmp_par->nc_background_mean = FUZZ_dataProducer_uint32(producer);
	cmp_par->nc_background_variance = FUZZ_dataProducer_uint32(producer);
	cmp_par->nc_background_outlier_pixels = FUZZ_dataProducer_uint32(producer);

	cmp_par->smearing_mean = FUZZ_dataProducer_uint32(producer);
	cmp_par->smearing_variance_mean = FUZZ_dataProducer_uint32(producer);
	cmp_par->smearing_outlier_pixels = FUZZ_dataProducer_uint32(producer);

	cmp_par->fc_imagette = FUZZ_dataProducer_uint32(producer);
	cmp_par->fc_offset_mean = FUZZ_dataProducer_uint32(producer);
	cmp_par->fc_offset_variance = FUZZ_dataProducer_uint32(producer);
	cmp_par->fc_background_mean = FUZZ_dataProducer_uint32(producer);
	cmp_par->fc_background_variance = FUZZ_dataProducer_uint32(producer);
	cmp_par->fc_background_outlier_pixels = FUZZ_dataProducer_uint32(producer);
}
