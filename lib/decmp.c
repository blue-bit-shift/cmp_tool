#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "../include/cmp_support.h"
#include "../include/cmp_icu.h"
#include "../include/cmp_data_types.h"
#include "../include/byteorder.h"
#include "../include/cmp_debug.h"


double get_compression_ratio(const struct cmp_info *info)
{
	unsigned long orign_len_bits = info->samples_used * size_of_a_sample(info->cmp_mode_used) * CHAR_BIT;

	return (double)orign_len_bits/(double)info->cmp_size;
}


void *malloc_decompressed_data(const struct cmp_info *info)
{
	size_t sample_len;

	if (!info)
		return NULL;

	if (info->samples_used == 0)
		return NULL;

	sample_len = size_of_a_sample(info->cmp_mode_used);

	return malloc(info->samples_used * sample_len);
}


/**
 * @brief decompression data pre-processing in RAW mode
 *
 * @note in RAW mode the data are uncompressed no pre_processing needed
 *
 * @param  cmp_mode_used used compression mode
 *
 * @returns 0 on success, error otherwise
 */

static int de_raw_pre_process(uint8_t cmp_mode_used)
{
	if (!raw_mode_is_used(cmp_mode_used))
		return -1;

	return 0;
}


/**
 * @brief model decompression pre-processing
 *
 * @note
 *
 * @param data_buf	pointer to the data to process
 * @param model_buf	pointer to the model of the data to process
 * @param samples_used	the size of the data and model buffer in 16 bit units
 * @param model_value_used used model weighting parameter
 * @param round_used	used number of bits to round; if zero no rounding takes place
 *
 * @returns 0 on success, error otherwise
 */

static int de_model_16(uint16_t *data_buf, uint16_t *model_buf, uint32_t
		       samples_used, uint8_t model_value_used, uint8_t
		       round_used)
{
	size_t i;
	int err;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	if (!model_buf)
		return -1;

	if (model_value_used > MAX_MODEL_VALUE)
		return -1;

	for (i = 0; i < samples_used; i++) {
		/* overflow is intended */
		data_buf[i] = (uint16_t)(data_buf[i] + round_fwd(model_buf[i],
								  round_used));
	}

	err = de_lossy_rounding_16(data_buf, samples_used, round_used);
	if (err)
		return -1;

	for (i = 0; i < samples_used; i++) {
		model_buf[i] = (uint16_t)cal_up_model(data_buf[i], model_buf[i],
						      model_value_used);
	}
	return 0;
}


static int de_model_S_FX(struct S_FX *data_buf, struct S_FX *model_buf, uint32_t
		       samples_used, uint8_t model_value_used, uint8_t
		       round_used)
{
	size_t i;
	int err;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	if (!model_buf)
		return -1;

	if (model_value_used > MAX_MODEL_VALUE)
		return -1;

	for (i = 0; i < samples_used; i++) {
		/* overflow is intended */
		struct S_FX round_model = model_buf[i];

		lossy_rounding_S_FX(&round_model, 1, round_used);
		data_buf[i] = add_S_FX(data_buf[i], model_buf[i]);
	}

	err = de_lossy_rounding_S_FX(data_buf, samples_used, round_used);
	if (err)
		return -1;

	for (i = 0; i < samples_used; i++)
		model_buf[i] = cal_up_model_S_FX(data_buf[i], model_buf[i],
						 model_value_used);

	return 0;
}


/**
 * @brief 1d-differencing decompression per-processing
 *
 * @param data_buf	pointer to the data to process
 * @param samples_used	the size of the data and model buffer in 16 bit units
 * @param round_used	used number of bits to round; if zero no rounding takes place
 *
 * @returns 0 on success, error otherwise
 */

static int de_diff_16(uint16_t *data_buf, uint32_t samples_used, uint8_t
		      round_used)
{
	size_t i;
	int err;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 1; i < samples_used; i++) {
		/* overflow intended */
		data_buf[i] = data_buf[i] + data_buf[i-1];
	}

	err = de_lossy_rounding_16(data_buf, samples_used, round_used);
	if (err)
		return -1;

	return 0;
}


static int de_diff_32(uint32_t *data_buf, uint32_t samples_used, uint8_t
		      round_used)
{
	size_t i;
	int err;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 1; i < samples_used; i++) {
		/* overflow intended */
		data_buf[i] = data_buf[i] + data_buf[i-1];
	}

	err = de_lossy_rounding_32(data_buf, samples_used, round_used);
	if (err)
		return -1;

	return 0;
}


static int de_diff_S_FX(struct S_FX *data_buf, uint32_t samples_used, uint8_t
			round_used)
{
	size_t i;
	int err;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 1; i < samples_used; i++) {
		/* overflow intended */
		data_buf[i] = add_S_FX(data_buf[i], data_buf[i-1]);
	}

	err = de_lossy_rounding_S_FX(data_buf, samples_used, round_used);
	if (err)
		return -1;

	return 0;
}


static int de_diff_S_FX_EFX(struct S_FX_EFX *data_buf, uint32_t samples_used,
			    uint8_t round_used)
{
	size_t i;
	int err;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 1; i < samples_used; i++) {
		/* overflow intended */
		data_buf[i] = add_S_FX_EFX(data_buf[i], data_buf[i-1]);
	}

	err = de_lossy_rounding_S_FX_EFX(data_buf, samples_used, round_used);
	if (err)
		return -1;

	return 0;
}


static int de_diff_S_FX_NCOB(struct S_FX_NCOB *data_buf, uint32_t samples_used,
			     uint8_t round_used)
{
	size_t i;
	int err;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 1; i < samples_used; i++) {
		/* overflow intended */
		data_buf[i] = add_S_FX_NCOB(data_buf[i], data_buf[i-1]);
	}

	err = de_lossy_rounding_S_FX_NCOB(data_buf, samples_used, round_used);
	if (err)
		return -1;

	return 0;
}


static int de_diff_S_FX_EFX_NCOB_ECOB(struct S_FX_EFX_NCOB_ECOB *data_buf,
				      uint32_t samples_used, uint8_t round_used)
{
	size_t i;
	int err;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 1; i < samples_used; i++) {
		/* overflow intended */
		data_buf[i] = add_S_FX_EFX_NCOB_ECOB(data_buf[i], data_buf[i-1]);
	}

	err = de_lossy_rounding_S_FX_EFX_NCOB_ECOB(data_buf, samples_used,
						   round_used);
	if (err)
		return -1;

	return 0;
}


static int de_pre_process(void *decoded_data, void *de_model_buf,
			  const struct cmp_info *info)
{
	if (!decoded_data)
		return -1;

	if (!info)
		return -1;

	if (info->samples_used == 0)
		return 0;

	switch (info->cmp_mode_used) {
	case MODE_RAW:
	case MODE_RAW_S_FX:
		return de_raw_pre_process(info->cmp_mode_used);
		break;
	case MODE_MODEL_ZERO:
	case MODE_MODEL_MULTI:
		return de_model_16((uint16_t *)decoded_data,
				   (uint16_t *)de_model_buf, info->samples_used,
				   info->model_value_used, info->round_used);
		break;
	case MODE_DIFF_ZERO:
	case MODE_DIFF_MULTI:
		return de_diff_16((uint16_t *)decoded_data, info->samples_used,
				  info->round_used);
		break;
	case MODE_MODEL_ZERO_S_FX:
	case MODE_MODEL_MULTI_S_FX:
		return de_model_S_FX((struct S_FX *)decoded_data,
				   (struct S_FX *)de_model_buf,
				   info->samples_used, info->model_value_used,
				   info->round_used);
		break;
	case MODE_DIFF_ZERO_S_FX:
	case MODE_DIFF_MULTI_S_FX:
		return de_diff_S_FX((struct S_FX *)decoded_data,
				    info->samples_used, info->round_used);
		break;
	case MODE_DIFF_ZERO_S_FX_EFX:
	case MODE_DIFF_MULTI_S_FX_EFX:
		return de_diff_S_FX_EFX((struct S_FX_EFX *)decoded_data,
					info->samples_used, info->round_used);
		break;
	case MODE_MODEL_ZERO_S_FX_EFX:
	case MODE_MODEL_MULTI_S_FX_EFX:
		return -1;
		break;
	case MODE_DIFF_ZERO_S_FX_NCOB:
	case MODE_DIFF_MULTI_S_FX_NCOB:
		return de_diff_S_FX_NCOB((struct S_FX_NCOB *)decoded_data,
					 info->samples_used, info->round_used);
		break;
	case MODE_MODEL_ZERO_S_FX_NCOB:
	case MODE_MODEL_MULTI_S_FX_NCOB:
		return -1;
		break;
	case MODE_DIFF_ZERO_S_FX_EFX_NCOB_ECOB:
	case MODE_DIFF_MULTI_S_FX_EFX_NCOB_ECOB:
		return de_diff_S_FX_EFX_NCOB_ECOB((struct S_FX_EFX_NCOB_ECOB *)
						  decoded_data,
						  info->samples_used,
						  info->round_used);
		break;
	case MODE_MODEL_ZERO_S_FX_EFX_NCOB_ECOB:
	case MODE_MODEL_MULTI_S_FX_EFX_NCOB_ECOB:
		return -1;
		break;
	case MODE_DIFF_ZERO_F_FX:
	case MODE_DIFF_MULTI_F_FX:
		return de_diff_32((uint32_t *)decoded_data, info->samples_used,
				  info->round_used);
		break;
	case MODE_MODEL_ZERO_F_FX:
	case MODE_MODEL_MULTI_F_FX:
		return -1;
		break;
	default:
		debug_print("Error: Compression mode not supported.\n");
		break;
	}

	return -1;
}


static uint8_t de_map_to_pos_alg_8(uint8_t value_to_unmap)
{
	if (value_to_unmap & 0x1) /* if uneven */
		return (value_to_unmap + 1) / -2;
	else
		return value_to_unmap / 2;
}


static uint16_t de_map_to_pos_alg_16(uint16_t value_to_unmap)
{
	if (value_to_unmap & 0x1) /* if uneven */
		return (value_to_unmap + 1) / -2;
	else
		return value_to_unmap / 2;
}


static uint32_t de_map_to_pos_alg_32(uint32_t value_to_unmap)
{

	if (value_to_unmap & 0x1) /* if uneven */
		return ((int64_t)value_to_unmap + 1) / -2; /* typecast to prevent overflow */
	else
		return value_to_unmap / 2;
}


/**
 * @brief map the unsigned output of the pre-stage to a signed value range for a
 *	16-bit buffer
 *
 * @note change the data_buf in-place
 *
 * @param data_buf	pointer to the uint16_t data buffer to process
 * @param samples_used	amount of data samples in the data_buf
 * @param zero_mode_used needs to be set if the zero escape symbol mechanism is used
 *
 * @returns 0 on success, error otherwise
 */

static int de_map_to_pos_16(uint16_t *data_buf, uint32_t samples_used, int
			    zero_mode_used)
{
	size_t i;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 0; i < samples_used; i++) {
		if (zero_mode_used)
			data_buf[i] -= 1;

		data_buf[i] = (uint16_t)de_map_to_pos_alg_16(data_buf[i]);
	}
	return 0;
}


/**
 * @brief map the unsigned output of the pre-stage to a signed value range for a
 *	32-bit buffer
 *
 * @note change the data_buf in-place
 *
 * @param data_buf	pointer to the uint16_t data buffer to process
 * @param samples_used	amount of data samples in the data_buf
 * @param zero_mode_used needs to be set if the zero escape symbol mechanism is used
 *
 * @returns 0 on success, error otherwise
 */

static int de_map_to_pos_32(uint32_t *data_buf, uint32_t samples_used, int
			    zero_mode_used)
{
	size_t i;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 0; i < samples_used; i++) {
		if (zero_mode_used)
			data_buf[i] -= 1;

		data_buf[i] = (uint32_t)de_map_to_pos_alg_32(data_buf[i]);
	}
	return 0;
}


/**
 * @brief map the unsigned output of the pre-stage to a signed value range for a
 *	S_FX buffer
 *
 * @note change the data_buf in-place
 *
 * @param data_buf	pointer to the S_FX data buffer to process
 * @param samples_used	amount of data samples in the data_buf
 * @param zero_mode_used needs to be set if the zero escape symbol mechanism is used
 *
 * @returns 0 on success, error otherwise
 */

static int de_map_to_pos_S_FX(struct S_FX *data_buf, uint32_t samples_used, int
			      zero_mode_used)
{
	size_t i;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 0; i < samples_used; i++) {
		if (zero_mode_used) {
			/* data_buf[i].EXPOSURE_FLAGS -= 1; */
			data_buf[i].FX -= 1;
		}

		data_buf[i].EXPOSURE_FLAGS =
			de_map_to_pos_alg_8(data_buf[i].EXPOSURE_FLAGS);
		data_buf[i].FX = de_map_to_pos_alg_32(data_buf[i].FX);
	}
	return 0;
}


/**
 * @brief map the unsigned output of the pre-stage to a signed value range for a
 *	S_FX_EFX buffer
 *
 * @note change the data_buf in-place
 *
 * @param data_buf	pointer to the S_FX_EFX data buffer to process
 * @param samples_used	amount of data samples in the data_buf
 * @param zero_mode_used needs to be set if the zero escape symbol mechanism is used
 *
 * @returns 0 on success, error otherwise
 */

static int de_map_to_pos_S_FX_EFX(struct S_FX_EFX *data_buf, uint32_t
				  samples_used, int zero_mode_used)
{
	size_t i;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 0; i < samples_used; i++) {
		if (zero_mode_used) {
			/* data_buf[i].EXPOSURE_FLAGS -= 1; */
			data_buf[i].FX -= 1;
			data_buf[i].EFX -= 1;
		}

		data_buf[i].EXPOSURE_FLAGS =
			de_map_to_pos_alg_8(data_buf[i].EXPOSURE_FLAGS);
		data_buf[i].FX = de_map_to_pos_alg_32(data_buf[i].FX);
		data_buf[i].EFX = de_map_to_pos_alg_32(data_buf[i].EFX);
	}
	return 0;
}


/**
 * @brief map the unsigned output of the pre-stage to a signed value range for a
 *	S_FX_NCOB buffer
 *
 * @note change the data_buf in-place
 *
 * @param data_buf	pointer to the S_FX_NCOB data buffer to process
 * @param samples_used	amount of data samples in the data_buf
 * @param zero_mode_used needs to be set if the zero escape symbol mechanism is used
 *
 * @returns 0 on success, error otherwise
 */

static int de_map_to_pos_S_FX_NCOB(struct S_FX_NCOB *data_buf, uint32_t
				   samples_used, int zero_mode_used)
{
	size_t i;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 0; i < samples_used; i++) {
		if (zero_mode_used) {
			/* data_buf[i].EXPOSURE_FLAGS -= 1; */
			data_buf[i].FX -= 1;
			data_buf[i].NCOB_X -= 1;
			data_buf[i].NCOB_Y -= 1;
		}

		data_buf[i].EXPOSURE_FLAGS =
			de_map_to_pos_alg_8(data_buf[i].EXPOSURE_FLAGS);
		data_buf[i].FX = de_map_to_pos_alg_32(data_buf[i].FX);
		data_buf[i].NCOB_X = de_map_to_pos_alg_32(data_buf[i].NCOB_X);
		data_buf[i].NCOB_Y = de_map_to_pos_alg_32(data_buf[i].NCOB_Y);
	}
	return 0;
}


/**
 * @brief map the unsigned output of the pre-stage to a signed value range for a
 *	S_FX_EFX_NCOB_ECOB buffer
 *
 * @note change the data_buf in-place
 *
 * @param data_buf	pointer to the S_FX_EFX_NCOB_ECOB data buffer to process
 * @param samples_used	amount of data samples in the data_buf
 * @param zero_mode_used needs to be set if the zero escape symbol mechanism is used
 *
 * @returns 0 on success, error otherwise
 */

static int de_map_to_pos_S_FX_EFX_NCOB_ECOB(struct S_FX_EFX_NCOB_ECOB *data_buf,
					    uint32_t samples_used,
					    int zero_mode_used)
{
	size_t i;

	if (!samples_used)
		return 0;

	if (!data_buf)
		return -1;

	for (i = 0; i < samples_used; i++) {
		if (zero_mode_used) {
			/* data_buf[i].EXPOSURE_FLAGS -= 1; */
			data_buf[i].FX -= 1;
			data_buf[i].NCOB_X -= 1;
			data_buf[i].NCOB_Y -= 1;
			data_buf[i].EFX -= 1;
			data_buf[i].ECOB_X -= 1;
			data_buf[i].ECOB_Y -= 1;
		}

		data_buf[i].EXPOSURE_FLAGS =
			de_map_to_pos_alg_8(data_buf[i].EXPOSURE_FLAGS);
		data_buf[i].FX = de_map_to_pos_alg_32(data_buf[i].FX);
		data_buf[i].NCOB_X = de_map_to_pos_alg_32(data_buf[i].NCOB_X);
		data_buf[i].NCOB_Y = de_map_to_pos_alg_32(data_buf[i].NCOB_Y);
		data_buf[i].EFX = de_map_to_pos_alg_32(data_buf[i].EFX);
		data_buf[i].ECOB_X = de_map_to_pos_alg_32(data_buf[i].ECOB_X);
		data_buf[i].ECOB_Y = de_map_to_pos_alg_32(data_buf[i].ECOB_Y);
	}
	return 0;
}


/**
 * @brief map the unsigned output of the pre-stage to a signed value range for a
 *	F_FX buffer
 *
 * @note change the data_buf in-place
 *
 * @param data_buf	pointer to the F_FX data buffer to process
 * @param samples_used	amount of data samples in the data_buf
 * @param zero_mode_used needs to be set if the zero escape symbol mechanism is used
 *
 * @returns 0 on success, error otherwise
 */

static int de_map_to_pos_F_FX(uint32_t *data_buf, uint32_t samples_used, int
			      zero_mode_used)
{
	return de_map_to_pos_32(data_buf, samples_used, zero_mode_used);
}


/**
 * @brief map the unsigned output of the pre-stage to a signed value range
 *
 * @note change the data_buf in-place
 *
 * @param decompressed_data	pointer to the data to process
 * @param info			compressor information contains information of
 *				an executed compression
 *
 * @returns 0 on success, error otherwise
 */

static int de_map_to_pos(void *decompressed_data, const struct cmp_info *info)
{
	int zero_mode_used;

	if (!info)
		return -1;

	if (info->samples_used == 0)
		return 0;

	if (!decompressed_data)
		return -1;

	zero_mode_used = zero_escape_mech_is_used(info->cmp_mode_used);

	switch (info->cmp_mode_used) {
	case MODE_RAW:
	case MODE_RAW_S_FX:
		return 0; /* in raw mode no mapping is necessary */
		break;
	case MODE_MODEL_ZERO:
	case MODE_MODEL_MULTI:
	case MODE_DIFF_ZERO:
	case MODE_DIFF_MULTI:
		return de_map_to_pos_16((uint16_t *)decompressed_data,
					info->samples_used, zero_mode_used);
		break;
	case MODE_MODEL_ZERO_S_FX:
	case MODE_MODEL_MULTI_S_FX:
	case MODE_DIFF_ZERO_S_FX:
	case MODE_DIFF_MULTI_S_FX:
		return de_map_to_pos_S_FX((struct S_FX *)decompressed_data,
					  info->samples_used, zero_mode_used);
		break;
	case MODE_MODEL_ZERO_S_FX_EFX:
	case MODE_MODEL_MULTI_S_FX_EFX:
	case MODE_DIFF_ZERO_S_FX_EFX:
	case MODE_DIFF_MULTI_S_FX_EFX:
		return de_map_to_pos_S_FX_EFX((struct S_FX_EFX *)
					      decompressed_data,
					      info->samples_used,
					      zero_mode_used);
		break;
	case MODE_MODEL_ZERO_S_FX_NCOB:
	case MODE_MODEL_MULTI_S_FX_NCOB:
	case MODE_DIFF_ZERO_S_FX_NCOB:
	case MODE_DIFF_MULTI_S_FX_NCOB:
		return de_map_to_pos_S_FX_NCOB((struct S_FX_NCOB *)
					       decompressed_data,
					       info->samples_used,
					       zero_mode_used);
		break;
	case MODE_MODEL_ZERO_S_FX_EFX_NCOB_ECOB:
	case MODE_MODEL_MULTI_S_FX_EFX_NCOB_ECOB:
	case MODE_DIFF_ZERO_S_FX_EFX_NCOB_ECOB:
	case MODE_DIFF_MULTI_S_FX_EFX_NCOB_ECOB:
		return de_map_to_pos_S_FX_EFX_NCOB_ECOB((struct S_FX_EFX_NCOB_ECOB *)
							decompressed_data,
							info->samples_used,
							zero_mode_used);
		break;
	case MODE_MODEL_ZERO_F_FX:
	case MODE_MODEL_MULTI_F_FX:
	case MODE_DIFF_ZERO_F_FX:
	case MODE_DIFF_MULTI_F_FX:
		return de_map_to_pos_F_FX((uint32_t *)decompressed_data,
					  info->samples_used, zero_mode_used);
		break;
	default:
		debug_print("Error: Compression mode not supported.\n");
		break;
	}
	return -1;
}


static unsigned int GetNBits32 (uint32_t *p_value, unsigned int bitOffset,
				unsigned int nBits, const unsigned int *srcAddr)
{
	const unsigned int *localAddr;
	unsigned int bitsLeft, bitsRight, localEndPos;
	unsigned int mask;
	/*leave in case of erroneous input */
	if (nBits == 0)
		return 0;
	if (nBits > 32)
		return 0;
	if (!srcAddr)
		return 0;
	if (!p_value)
		return 0;
	/* separate the bitOffset into word offset (set localAddr pointer) and
	 * local bit offset (bitsLeft)
	 */
	localAddr = srcAddr + (bitOffset >> 5);
	bitsLeft = bitOffset & 0x1f;

	localEndPos = bitsLeft + nBits;

	if (localEndPos <= 32) {
		unsigned int shiftRight = 32 - nBits;

		bitsRight = shiftRight - bitsLeft;

		*(p_value) = *(localAddr) >> bitsRight;

		mask = (0xffffffff >> shiftRight);

		*(p_value) &= mask;
	} else {
		unsigned int n1 = 32 - bitsLeft;
		unsigned int n2 = nBits - n1;
		/* part 1 ; */
		mask = 0xffffffff >> bitsLeft;
		*(p_value) = (*localAddr) & mask;
		*(p_value) <<= n2;
		/*part 2: */
		/* adjust address*/
		localAddr += 1;

		bitsRight = 32 - n2;
		*(p_value) |= *(localAddr) >> bitsRight;
	}
	return nBits;
}

static unsigned int count_leading_ones(unsigned int value)
{
	unsigned int n_ones = 0; /* number of leading 1s */

	while (1) {
		unsigned int leading_bit;

		leading_bit = value & 0x80000000;
		if (!leading_bit)
			break;
		else {
			n_ones++;
			value <<= 1;
		}
	}
	return n_ones;
}


static unsigned int Rice_decoder(uint32_t code_word, unsigned int m,
				 unsigned int log2_m, unsigned int *decoded_cw)
{
	unsigned int q; /* quotient code */
	unsigned int ql; /* length of the quotient code */
	unsigned int r; /* remainder code */
	unsigned int rl; /* length of the remainder code */
	unsigned int cw_len; /* length of the decoded code word in bits */

	(void)m;

	q = count_leading_ones(code_word);
	ql = q + 1; /* Number of 1's + following 0 */

	rl = log2_m;

	cw_len = rl + ql;

	if (cw_len > 32) /* can only decode code words with maximum 32 bits */
		return 0;

	code_word = code_word << ql;  /* shift quotient code out */

	/* Right shifting an integer by a number of bits equal orgreater than
	 * its size is undefined behavior
	 */
	if (rl == 0)
		r = 0;
	else
		r = code_word >> (32 - rl);

	*decoded_cw = (q << log2_m) + r;

	return cw_len;
}


static unsigned int Golomb_decoder(unsigned int code_word, unsigned int m,
				   unsigned int log2_m, unsigned int
				   *decoded_cw)
{
	unsigned int q; /* quotient code */
	unsigned int r1; /* remainder code group 1 */
	unsigned int r2; /* remainder code group 2 */
	unsigned int r; /* remainder code */
	unsigned int rl; /* length of the remainder code */
	unsigned int cutoff; /* cutoff between group 1 and 2 */
	unsigned int cw_len; /* length of the decoded code word in bits */

	q = count_leading_ones(code_word);

	rl = log2_m + 1;
	code_word <<= (q+1);  /* shift quotient code out */

	r2 = code_word >> (32 - rl);
	r1 = r2 >> 1;

	cutoff = (1UL << rl) - m;

	if (r1 < cutoff) {
		cw_len = q + rl;
		r = r1;
	} else {
		cw_len = q + rl + 1;
		r = r2 - cutoff;
	}

	if (cw_len > 32)
		return 0;

	*decoded_cw = q*m + r;
	return cw_len;
}


typedef unsigned int (*decoder_ptr)(unsigned int, unsigned int, unsigned int, unsigned int *);

static decoder_ptr select_decoder(unsigned int golomb_par)
{
	if (!golomb_par)
		return NULL;

	if (is_a_pow_of_2(golomb_par))
		return &Rice_decoder;
	else
		return &Golomb_decoder;
}


static int decode_raw(const void *compressed_data, const struct cmp_info
			*info, void *const decompressed_data)
{
	if (!info)
		return -1;
	if (info->samples_used == 0)
		return 0;
	if (!compressed_data)
		return -1;
	if (!decompressed_data)
		return -1;

	if (info->samples_used*size_of_a_sample(info->cmp_mode_used)*CHAR_BIT
	    != info->cmp_size) {
		debug_print("Warning: The size of the decompressed bitstream does not match the size of the compressed bitstream. Check if the parameters used for decompression are the same as those used for compression.\n");
		return 1;
	}
	memcpy(decompressed_data, compressed_data, info->cmp_size/CHAR_BIT);

	return 0;
}

static int decode_raw_16(const void *compressed_data, const struct cmp_info
			*info, uint16_t *const decompressed_data)
{
	int err = decode_raw(compressed_data, info, decompressed_data);
	if (err)
		return err;

#if defined(__LITTLE_ENDIAN)
	{
		size_t i;
		for (i = 0; i < info->samples_used; i++) {
			decompressed_data[i] = cpu_to_be16(decompressed_data[i]);
		}
	}
#endif
	return 0;
}


static int decode_raw_S_FX(const void *compressed_data, const struct cmp_info
			   *info, struct S_FX *const decompressed_data)
{
	int err = decode_raw(compressed_data, info, decompressed_data);
	if (err)
		return err;

#if defined(LITTLE_ENDIAN)
	{
		size_t i;
		for (i = 0; i < info->samples_used; i++) {
			decompressed_data[i].FX = cpu_to_be32(decompressed_data[i].FX);
		}
	}
#endif
	return 0;
}

static unsigned int decode_normal(const void *compressed_data,
				  const struct cmp_info *info,
				  unsigned int read_pos,
				  unsigned int max_cw_len,
				  uint32_t *const decoded_val)
{
	decoder_ptr decoder;
	unsigned int n_read_bits;
	uint32_t read_val;
	unsigned int n_bits;
	unsigned int read_bits;
	unsigned int log2_g;

	if (!compressed_data)
		return -1U;

	if (!info)
		return -1U;

	if (!decoded_val)
		return -1U;

	if (read_pos > info->cmp_size)
		return -1U;

	if (max_cw_len > 32)
		return -1U;

	if (max_cw_len == 0)
		return read_pos;

	decoder = select_decoder(info->golomb_par_used);
	if (!decoder)
		return -1U;

	if (read_pos + max_cw_len > info->cmp_size)   /* check buffer overflow */
		n_read_bits = info->cmp_size - read_pos;
	else
		n_read_bits = max_cw_len;

	read_bits = GetNBits32(&read_val, read_pos, n_read_bits,
			       compressed_data);
	if (!read_bits)
		return -1U;

	read_val = read_val << (32 - n_read_bits);

	log2_g = ilog_2(info->golomb_par_used);
	n_bits = decoder(read_val, info->golomb_par_used, log2_g, decoded_val);

	return read_pos + n_bits;
}


static unsigned int decode_zero(const void *compressed_data,
				const struct cmp_info *info,
				unsigned int read_pos, unsigned int max_cw_len,
				uint32_t *const decoded_val)
{
	if (!info)
		return -1U;

	if (info->samples_used == 0)
		return read_pos;

	if (!compressed_data)
		return -1U;

	if (!decoded_val)
		return -1U;

	if (read_pos > info->cmp_size)
		return -1U;

	if (max_cw_len > 32)
		return -1U;

	if (max_cw_len == 0)
		return read_pos;

	read_pos = decode_normal(compressed_data, info, read_pos, max_cw_len,
				 decoded_val);
	if (read_pos == -1U)
		return -1U;
	if (*decoded_val >= info->spill_used) /* consistency check */
		return -1U;

	if (*decoded_val == 0) {/* escape symbol mechanism was used; read unencoded value */
		unsigned int n_bits;
		uint32_t unencoded_val;

		if (read_pos + max_cw_len > info->cmp_size) /* check buffer overflow */
			return -1U;
		n_bits = GetNBits32(&unencoded_val, read_pos, max_cw_len,
				    compressed_data);
		if (!n_bits)
			return -1U;
		if (unencoded_val < info->spill_used && unencoded_val != 0) /* consistency check */
			return -1U;

		*decoded_val = unencoded_val;
		read_pos += n_bits;
	}
	return read_pos;
}


static unsigned int decode_multi(const void *compressed_data,
				 const struct cmp_info *info,
				 unsigned int read_pos, unsigned int max_cw_len,
				 uint32_t *const decoded_val)
{
	if (!info)
		return -1U;

	if (info->samples_used == 0)
		return read_pos;

	if (!compressed_data)
		return -1U;

	if (!decoded_val)
		return -1U;

	if (read_pos > info->cmp_size)
		return -1U;

	if (max_cw_len > 32)
		return -1U;

	if (max_cw_len == 0)
		return read_pos;

	read_pos = decode_normal(compressed_data, info, read_pos, max_cw_len,
				 decoded_val);
	if (read_pos == -1U)
		return -1U;

	if (*decoded_val >= info->spill_used) {
		/* escape symbol mechanism was used; read unencoded value */
		unsigned int n_bits;
		uint32_t unencoded_val;
		unsigned int unencoded_len;

		unencoded_len = (*decoded_val - info->spill_used + 1) * 2;
		if (unencoded_len > max_cw_len) /* consistency check */
			return -1U;

		/* check buffer overflow */
		if ((read_pos + unencoded_len) > info->cmp_size) {
			/*TODO: debug message */
			return -1U;
		}
		n_bits = GetNBits32(&unencoded_val, read_pos, unencoded_len,
				    compressed_data);
		if (!n_bits)
			return -1U;

		*decoded_val = unencoded_val + info->spill_used;
		read_pos += n_bits;
	}
	return read_pos;
}


static unsigned int decode_value(const void *compressed_data,
				 const struct cmp_info *info,
				 unsigned int read_pos,
				 unsigned int max_cw_len, uint32_t *decoded_val)
{
	if (multi_escape_mech_is_used(info->cmp_mode_used))
		return decode_multi(compressed_data, info, read_pos, max_cw_len,
				    decoded_val);

	if (zero_escape_mech_is_used(info->cmp_mode_used))
		return decode_zero(compressed_data, info, read_pos, max_cw_len,
				   decoded_val);
	return -1U;
}


static int decode_16(const void *compressed_data, const struct cmp_info *info,
		     uint16_t *decoded_data)
{
	size_t i;
	unsigned int read_pos = 0;

	if (!info)
		return -1;

	if (info->samples_used == 0)
		return 0;

	if (!decoded_data)
		return -1;

	for (i = 0; i < info->samples_used; i++) {
		uint32_t decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 16,
					&decoded_val);
		if (read_pos == -1U) {
			debug_print("Error: Compressed values could not be decoded.\n");
			return -1;
		}

		if (decoded_val > UINT16_MAX)
			return -1;

		decoded_data[i] = (uint16_t)decoded_val;
	}

	if (read_pos != info->cmp_size) {
		debug_print("Warning: The size of the decompressed bitstream does not match the size of the compressed bitstream. Check if the parameters used for decompression are the same as those used for compression.\n");
		return 1;
	}
	return 0;
}


static int decode_S_FX(const void *compressed_data, const struct cmp_info *info,
		       struct S_FX *decoded_data)
{
	size_t i;
	unsigned int read_pos = 0;
	struct cmp_info info_exp_flag = *info;

	info_exp_flag.golomb_par_used = GOLOMB_PAR_EXPOSURE_FLAGS;

	if (!info)
		return -1;

	if (info->samples_used == 0)
		return 0;

	if (!decoded_data)
		return -1;

	for (i = 0; i < info->samples_used; i++) {
		uint32_t decoded_val;

		/* read_pos = decode_value(compressed_data, &info_exp_flag, read_pos, 8, */
		/* 			&decoded_val); */
		read_pos = decode_normal(compressed_data, &info_exp_flag, read_pos, 8,
					 &decoded_val);
		if (read_pos == -1U)
			return -1;
		if (decoded_val > UINT8_MAX)
			return -1;
		decoded_data[i].EXPOSURE_FLAGS = (uint8_t)decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].FX = decoded_val;
	}

	if (read_pos != info->cmp_size) {
		debug_print("Warning: The size of the decompressed bitstream does not match the size of the compressed bitstream. Check if the parameters used for decompression are the same as those used for compression.");
		return 1;
	}
	return 0;
}


static int decode_S_FX_EFX(const void *compressed_data, const struct cmp_info
			   *info, struct S_FX_EFX *decoded_data)
{
	size_t i;
	unsigned int read_pos = 0;

	if (!info)
		return -1;

	if (info->samples_used == 0)
		return 0;

	if (!decoded_data)
		return -1;

	for (i = 0; i < info->samples_used; i++) {
		uint32_t decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 8,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		if (decoded_val > UINT8_MAX)
			return -1;
		decoded_data[i].EXPOSURE_FLAGS = (uint8_t)decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].FX = decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].EFX = decoded_val;
	}

	if (read_pos != info->cmp_size) {
		debug_print("Warning: The size of the decompressed bitstream does not match the size of the compressed bitstream. Check if the parameters used for decompression are the same as those used for compression.\n");
		return 1;
	}
	return 0;
}


static int decode_S_FX_NCOB(const void *compressed_data, const struct cmp_info
			    *info, struct S_FX_NCOB *decoded_data)
{
	size_t i;
	unsigned int read_pos = 0;

	if (!info)
		return -1;

	if (info->samples_used == 0)
		return 0;

	if (!decoded_data)
		return -1;

	for (i = 0; i < info->samples_used; i++) {
		uint32_t decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 8,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		if (decoded_val > UINT8_MAX)
			return -1;
		decoded_data[i].EXPOSURE_FLAGS = (uint8_t)decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].FX = decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].NCOB_X = decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].NCOB_Y = decoded_val;
	}

	if (read_pos != info->cmp_size) {
		debug_print("Warning: The size of the decompressed bitstream does not match the size of the compressed bitstream. Check if the parameters used for decompression are the same as those used for compression.\n");
		return 1;
	}
	return 0;
}


static int decode_S_FX_EFX_NCOB_ECOB(const void *compressed_data,
				     const struct cmp_info *info,
				     struct S_FX_EFX_NCOB_ECOB *decoded_data)
{
	size_t i;
	unsigned int read_pos = 0;

	if (!info)
		return -1;

	if (info->samples_used == 0)
		return 0;

	if (!decoded_data)
		return -1;

	for (i = 0; i < info->samples_used; i++) {
		uint32_t decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 8,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		if (decoded_val > UINT8_MAX)
			return -1;
		decoded_data[i].EXPOSURE_FLAGS = (uint8_t)decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].FX = decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].NCOB_X = decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].NCOB_Y = decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].EFX = decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].ECOB_X = decoded_val;

		read_pos = decode_value(compressed_data, info, read_pos, 32,
					&decoded_val);
		if (read_pos == -1U)
			return -1;
		decoded_data[i].ECOB_Y = decoded_val;
	}

	if (read_pos != info->cmp_size) {
		debug_print("Warning: The size of the decompressed bitstream does not match the size of the compressed bitstream. Check if the parameters used for decompression are the same as those used for compression.");
		return 1;
	}
	return 0;
}


static int decode_data(const void *compressed_data, const struct cmp_info *info,
		       void *decompressed_data)
{
	if (!info)
		return -1;

	if (info->samples_used == 0)
		return 0;

	if (!compressed_data)
		return -1;

	if (!decompressed_data)
		return -1;

	switch (info->cmp_mode_used) {
	case MODE_RAW:
		return decode_raw_16(compressed_data, info, decompressed_data);
		break;
	case MODE_MODEL_ZERO:
	case MODE_DIFF_ZERO:
	case MODE_MODEL_MULTI:
	case MODE_DIFF_MULTI:
		return decode_16(compressed_data, info,
				 (uint16_t *)decompressed_data);
		break;
	case MODE_RAW_S_FX:
		return decode_raw_S_FX(compressed_data, info, decompressed_data);
		break;
	case MODE_MODEL_ZERO_S_FX:
	case MODE_MODEL_MULTI_S_FX:
	case MODE_DIFF_ZERO_S_FX:
	case MODE_DIFF_MULTI_S_FX:
		return decode_S_FX(compressed_data, info,
				   (struct S_FX *)decompressed_data);
		break;
	case MODE_MODEL_ZERO_S_FX_EFX:
	case MODE_MODEL_MULTI_S_FX_EFX:
	case MODE_DIFF_ZERO_S_FX_EFX:
	case MODE_DIFF_MULTI_S_FX_EFX:
		return decode_S_FX_EFX(compressed_data, info,
				       (struct S_FX_EFX *)decompressed_data);
		break;
	case MODE_MODEL_ZERO_S_FX_NCOB:
	case MODE_MODEL_MULTI_S_FX_NCOB:
	case MODE_DIFF_ZERO_S_FX_NCOB:
	case MODE_DIFF_MULTI_S_FX_NCOB:
		return decode_S_FX_NCOB(compressed_data, info,
					(struct S_FX_NCOB *)decompressed_data);
		break;
	case MODE_MODEL_ZERO_S_FX_EFX_NCOB_ECOB:
	case MODE_MODEL_MULTI_S_FX_EFX_NCOB_ECOB:
	case MODE_DIFF_ZERO_S_FX_EFX_NCOB_ECOB:
	case MODE_DIFF_MULTI_S_FX_EFX_NCOB_ECOB:
		return decode_S_FX_EFX_NCOB_ECOB(compressed_data, info,
						 (struct S_FX_EFX_NCOB_ECOB *)
						 decompressed_data);
		break;
#if 0
	case MODE_MODEL_ZERO_F_FX:
	case MODE_MODEL_MULTI_F_FX:
	case MODE_DIFF_ZERO_F_FX:
	case MODE_DIFF_MULTI_F_FX:
		break;
#endif
	default:
		debug_print("Error: Compression mode not supported.\n");
		break;

	}
	return -1;
}

/* model buffer is overwritten with updated model*/

int decompress_data(const void *compressed_data, void *de_model_buf, const
		    struct cmp_info *info, void *decompressed_data)
{
	int err;

	if (!compressed_data)
		return -1;

	if (!info)
		return -1;

	if (info->cmp_err)
		return -1;

	if (model_mode_is_used(info->cmp_mode_used))
		if (!de_model_buf)
			return -1;
	/* TODO: add ohter modes */

	if (!decompressed_data)
		return -1;


	err = decode_data(compressed_data, info, decompressed_data);
	if (err)
		return err;

	err = de_map_to_pos(decompressed_data, info);
	if (err)
		return err;

	err = de_pre_process(decompressed_data, de_model_buf, info);
	if (err)
		return err;

	return 0;
}
