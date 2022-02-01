/**
 * @file   cmp_data_types.h
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at),
 * @date   2020
 * @brief definition of the different data types
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
 *
 * Three data rates:
 * fast cadence (nominally 25s)
 * short cadence (nominally 50s)
 * long cadence (nominally 600s)
 *
 * The science products are identified as this:
 * FX = normal light flux
 * NCOB = normal center of brightness
 * EFX = extended flux
 * ECOB = extended center of brightness
 * The prefixes F, S and L stand for Fast, Short and Long cadence
 */

#ifndef CMP_DATA_TYPE_H
#define CMP_DATA_TYPE_H

#include <stdint.h>

#include "compiler.h"

#define MODE_RAW_S_FX           100
#define MODE_MODEL_ZERO_S_FX	101
#define MODE_DIFF_ZERO_S_FX	102
#define MODE_MODEL_MULTI_S_FX	103
#define MODE_DIFF_MULTI_S_FX	104

#define MODE_MODEL_ZERO_S_FX_EFX	110
#define MODE_DIFF_ZERO_S_FX_EFX		111
#define MODE_MODEL_MULTI_S_FX_EFX	112
#define MODE_DIFF_MULTI_S_FX_EFX	113

#define MODE_MODEL_ZERO_S_FX_NCOB	120
#define MODE_DIFF_ZERO_S_FX_NCOB	121
#define MODE_MODEL_MULTI_S_FX_NCOB	122
#define MODE_DIFF_MULTI_S_FX_NCOB	123

#define MODE_MODEL_ZERO_S_FX_EFX_NCOB_ECOB	130
#define MODE_DIFF_ZERO_S_FX_EFX_NCOB_ECOB	131
#define MODE_MODEL_MULTI_S_FX_EFX_NCOB_ECOB	132
#define MODE_DIFF_MULTI_S_FX_EFX_NCOB_ECOB	133

#define MODE_MODEL_ZERO_F_FX	140
#define MODE_DIFF_ZERO_F_FX	141
#define MODE_MODEL_MULTI_F_FX	142
#define MODE_DIFF_MULTI_F_FX	143

#define MODE_MODEL_ZERO_F_FX_EFX	150
#define MODE_DIFF_ZERO_F_FX_EFX		151
#define MODE_MODEL_MULTI_F_FX_EFX	152
#define MODE_DIFF_MULTI_F_FX_EFX	153

#define MODE_MODEL_ZERO_F_FX_NCOB	160
#define MODE_DIFF_ZERO_F_FX_NCOB	161
#define MODE_MODEL_MULTI_F_FX_NCOB	162
#define MODE_DIFF_MULTI_F_FX_NCOB	163

#define MODE_MODEL_ZERO_F_FX_EFX_NCOB_ECOB	170
#define MODE_DIFF_ZERO_F_FX_EFX_NCOB_ECOB	171
#define MODE_MODEL_MULTI_F_FX_EFX_NCOB_ECOB	172
#define MODE_DIFF_MULTI_F_FX_EFX_NCOB_ECOB	173

#define MODE_RAW_32		200
#define MODE_DIFF_ZERO_32	201
#define MODE_DIFF_MULTI_32	202
#define MODE_MODEL_ZERO_32	203
#define MODE_MODEL_MULTI_32	204

int lossy_rounding_16(uint16_t *data_buf, unsigned int samples, unsigned int
		      round);
int de_lossy_rounding_16(uint16_t *data_buf, uint32_t samples_used, uint32_t
			 round_used);

int lossy_rounding_32(uint32_t *data_buf, unsigned int samples, unsigned int
		      round);
int de_lossy_rounding_32(uint32_t *data_buf, uint32_t samples_used, uint32_t
			 round_used);

void cmp_input_big_to_cpu_endiannessy(void *data, u_int32_t data_size_byte,
				      uint32_t cmp_mode);

/* @see for packed definition: PLATO-LESIA-PL-RP-0031 Issue: 1.9 (N-DPU->ICU data rate) */
#define N_DPU_ICU_MULTI_ENTRY_HDR_SIZE 12

struct n_dpu_icu_multi_entry_hdr {
	uint64_t ncxx_timestamp:48;
	uint16_t ncxx_configuration_id;
	uint16_t ncxx_collection_id;
	uint16_t ncxx_collection_length;
} __attribute__((packed));
compile_time_assert(sizeof(struct n_dpu_icu_multi_entry_hdr) == N_DPU_ICU_MULTI_ENTRY_HDR_SIZE, N_DPU_ICU_MULTI_ENTRY_HDR_SIZE_IS_NOT_CORRECT);


struct S_FX {
	uint8_t EXPOSURE_FLAGS;
	uint32_t FX;
} __attribute__((packed));

struct S_FX sub_S_FX(struct S_FX a, struct S_FX b);
struct S_FX add_S_FX(struct S_FX a, struct S_FX b);
int lossy_rounding_S_FX(struct S_FX *data_buf, unsigned int samples,
			       unsigned int round);
int de_lossy_rounding_S_FX(struct S_FX *data_buf, unsigned int samples_used,
			   unsigned int round_used);
struct S_FX cal_up_model_S_FX(struct S_FX data_buf, struct S_FX model_buf,
			      unsigned int model_value);


struct S_FX_EFX {
	uint8_t EXPOSURE_FLAGS;
	uint32_t FX;
	uint32_t EFX;
} __attribute__((packed));

struct S_FX_EFX sub_S_FX_EFX(struct S_FX_EFX a, struct S_FX_EFX b);
struct S_FX_EFX add_S_FX_EFX(struct S_FX_EFX a, struct S_FX_EFX b);
int lossy_rounding_S_FX_EFX(struct S_FX_EFX *data, unsigned int samples,
				   unsigned int round);
int de_lossy_rounding_S_FX_EFX(struct S_FX_EFX *data_buf, unsigned int
			       samples_used, unsigned int round_used);
struct S_FX_EFX cal_up_model_S_FX_EFX(struct S_FX_EFX data_buf, struct S_FX_EFX
				      model_buf, unsigned int model_value);


struct S_FX_NCOB {
	uint8_t EXPOSURE_FLAGS;
	uint32_t FX;
	uint32_t NCOB_X;
	uint32_t NCOB_Y;
} __attribute__((packed));

struct S_FX_NCOB sub_S_FX_NCOB(struct S_FX_NCOB a, struct S_FX_NCOB b);
struct S_FX_NCOB add_S_FX_NCOB(struct S_FX_NCOB a, struct S_FX_NCOB b);
int lossy_rounding_S_FX_NCOB(struct S_FX_NCOB *data_buf, unsigned int samples,
			     unsigned int round);
int de_lossy_rounding_S_FX_NCOB(struct S_FX_NCOB *data_buf, unsigned int
				samples_used, unsigned int round_used);
struct S_FX_NCOB cal_up_model_S_FX_NCOB(struct S_FX_NCOB data_buf,
					struct S_FX_NCOB model_buf,
					unsigned int model_value);


struct S_FX_EFX_NCOB_ECOB {
	uint8_t EXPOSURE_FLAGS;
	uint32_t FX;
	uint32_t NCOB_X;
	uint32_t NCOB_Y;
	uint32_t EFX;
	uint32_t ECOB_X;
	uint32_t ECOB_Y;
} __attribute__((packed));

struct S_FX_EFX_NCOB_ECOB sub_S_FX_EFX_NCOB_ECOB(struct S_FX_EFX_NCOB_ECOB a,
						 struct S_FX_EFX_NCOB_ECOB b);
struct S_FX_EFX_NCOB_ECOB add_S_FX_EFX_NCOB_ECOB(struct S_FX_EFX_NCOB_ECOB a,
						 struct S_FX_EFX_NCOB_ECOB b);
int lossy_rounding_S_FX_EFX_NCOB_ECOB(struct S_FX_EFX_NCOB_ECOB *data_buf,
				      unsigned int samples, unsigned int round);
int de_lossy_rounding_S_FX_EFX_NCOB_ECOB(struct S_FX_EFX_NCOB_ECOB *data_buf,
					 unsigned int samples,
					 unsigned int round);

struct F_FX {
	uint32_t FX;
} __attribute__((packed));


struct F_FX_EFX {
	uint32_t FX;
	uint32_t EFX;
} __attribute__((packed));


struct F_FX_NCOB {
	uint32_t FX;
	uint32_t NCOB_X;
	uint32_t NCOB_Y;
} __attribute__((packed));


struct F_FX_EFX_NCOB_ECOB {
	uint32_t FX;
	uint32_t NCOB_X;
	uint32_t NCOB_Y;
	uint32_t EFX;
	uint32_t ECOB_X;
	uint32_t ECOB_Y;
} __attribute__((packed));

#endif /* CMP_DATA_TYPE_H */
