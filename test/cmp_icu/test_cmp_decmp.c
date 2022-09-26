/**
 * @file   test_cmp_decmp.c
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at),
 * @date   2022
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
 * @brief random compression decompression test
 * @detail We generate random data and compress them with random parameters.
 *	After that we put the data in a compression entity. We decompress the
 *	compression entity and compare the decompressed data with the original
 *	data.
 */


#include <string.h>
#include <stdlib.h>

#include <unity.h>

#include <cmp_icu.h>
#include <decmp.h>
#include <cmp_data_types.h>

#if defined __has_include
#  if __has_include(<time.h>)
#    include <time.h>
#    include <unistd.h>
#    define HAS_TIME_H 1
#  endif
#endif

#define IMAX_BITS(m) ((m)/((m)%255+1) / 255%255*8 + 7-86/((m)%255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

#define set_n_bits(n)  (~(~0UL << (n)))


/**
 * @brief  Seeds the pseudo-random number generator used by rand()
 */

void setUp(void)
{
	unsigned int seed;
	static int n;

#if HAS_TIME_H
	seed = time(NULL) * getpid();
#else
	seed = 1;
#endif

	if (!n) {
		n = 1;
		srand(seed);
		printf("seed: %u\n", seed);
	}
}


/**
 * @brief generate a uint32_t random number
 *
 * @return a "random" uint32_t value
 * @see https://stackoverflow.com/a/33021408
 */

uint32_t rand32(void)
{
	int i;
	uint32_t r = 0;

	for (i = 0; i < 32; i += RAND_MAX_WIDTH) {
		r <<= RAND_MAX_WIDTH;
		r ^= (unsigned int) rand();
	}
	return r;
}


/**
 * @brief generate a random number in a range
 *
 * @param min minimum value (inclusive)
 * @param max maximum value (inclusive)
 *
 * @returns "random" numbers in the range [min, max]
 *
 * @see https://c-faq.com/lib/randrange.html
 */

uint32_t random_between(unsigned int min, unsigned int max)
{
	TEST_ASSERT(min < max);
	if (max-min < RAND_MAX)
		return min + rand() / (RAND_MAX / (max - min + 1ULL) + 1);
	else
		return min + rand32() / (UINT32_MAX / (max - min + 1ULL) + 1);
}


static void gen_ima_data(uint16_t *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i] = random_between(0, set_n_bits(max_used_bits.nc_imagette));
	}
}


static void gen_offset_data(struct nc_offset *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].mean = random_between(0, set_n_bits(max_used_bits.nc_offset_mean));
		data[i].variance = random_between(0, set_n_bits(max_used_bits.nc_offset_variance));
	}
}


static void gen_background_data(struct nc_background *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].mean = random_between(0, set_n_bits(max_used_bits.nc_background_mean));
		data[i].variance = random_between(0, set_n_bits(max_used_bits.nc_background_variance));
		data[i].outlier_pixels = random_between(0, set_n_bits(max_used_bits.nc_background_outlier_pixels));
	}
}


static void gen_smearing_data(struct smearing *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].mean = random_between(0, set_n_bits(max_used_bits.smearing_mean));
		data[i].variance_mean = random_between(0, set_n_bits(max_used_bits.smearing_variance_mean));
		data[i].outlier_pixels = random_between(0, set_n_bits(max_used_bits.smearing_outlier_pixels));
	}
}


static void gen_s_fx_data(struct s_fx *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].exp_flags = random_between(0, set_n_bits(max_used_bits.s_exp_flags));
		data[i].fx = random_between(0, set_n_bits(max_used_bits.s_fx));
	}
}


static void gen_s_fx_efx_data(struct s_fx_efx *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].exp_flags = random_between(0, set_n_bits(max_used_bits.s_exp_flags));
		data[i].fx = random_between(0, set_n_bits(max_used_bits.s_fx));
		data[i].efx = random_between(0, set_n_bits(max_used_bits.s_efx));
	}
}


static void gen_s_fx_ncob_data(struct s_fx_ncob *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].exp_flags = random_between(0, set_n_bits(max_used_bits.s_exp_flags));
		data[i].fx = random_between(0, set_n_bits(max_used_bits.s_fx));
		data[i].ncob_x = random_between(0, set_n_bits(max_used_bits.s_ncob));
		data[i].ncob_y = random_between(0, set_n_bits(max_used_bits.s_ncob));
	}
}


static void gen_s_fx_efx_ncob_ecob_data(struct s_fx_efx_ncob_ecob *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].exp_flags = random_between(0, set_n_bits(max_used_bits.s_exp_flags));
		data[i].fx = random_between(0, set_n_bits(max_used_bits.s_fx));
		data[i].ncob_x = random_between(0, set_n_bits(max_used_bits.s_ncob));
		data[i].ncob_y = random_between(0, set_n_bits(max_used_bits.s_ncob));
		data[i].efx = random_between(0, set_n_bits(max_used_bits.s_efx));
		data[i].ecob_x = random_between(0, set_n_bits(max_used_bits.s_ecob));
		data[i].ecob_y = random_between(0, set_n_bits(max_used_bits.s_ecob));
	}
}


static void gen_f_fx_data(struct f_fx *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].fx = random_between(0, set_n_bits(max_used_bits.f_fx));
	}
}


static void gen_f_fx_efx_data(struct f_fx_efx *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].fx = random_between(0, set_n_bits(max_used_bits.f_fx));
		data[i].efx = random_between(0, set_n_bits(max_used_bits.f_efx));
	}
}


static void gen_f_fx_ncob_data(struct f_fx_ncob *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].fx = random_between(0, set_n_bits(max_used_bits.f_fx));
		data[i].ncob_x = random_between(0, set_n_bits(max_used_bits.f_ncob));
		data[i].ncob_y = random_between(0, set_n_bits(max_used_bits.f_ncob));
	}
}


static void gen_f_fx_efx_ncob_ecob_data(struct f_fx_efx_ncob_ecob *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].fx = random_between(0, set_n_bits(max_used_bits.f_fx));
		data[i].ncob_x = random_between(0, set_n_bits(max_used_bits.f_ncob));
		data[i].ncob_y = random_between(0, set_n_bits(max_used_bits.f_ncob));
		data[i].efx = random_between(0, set_n_bits(max_used_bits.f_efx));
		data[i].ecob_x = random_between(0, set_n_bits(max_used_bits.f_ecob));
		data[i].ecob_y = random_between(0, set_n_bits(max_used_bits.f_ecob));
	}
}


static void gen_l_fx_data(struct l_fx *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].exp_flags = random_between(0, set_n_bits(max_used_bits.l_exp_flags));
		data[i].fx = random_between(0, set_n_bits(max_used_bits.l_fx));
		data[i].fx_variance = random_between(0, set_n_bits(max_used_bits.l_fx_variance));
	}
}


static void gen_l_fx_efx_data(struct l_fx_efx *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].exp_flags = random_between(0, set_n_bits(max_used_bits.l_exp_flags));
		data[i].fx = random_between(0, set_n_bits(max_used_bits.l_fx));
		data[i].efx = random_between(0, set_n_bits(max_used_bits.l_efx));
		data[i].fx_variance = random_between(0, set_n_bits(max_used_bits.l_fx_variance));
	}
}


static void gen_l_fx_ncob_data(struct l_fx_ncob *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].exp_flags = random_between(0, set_n_bits(max_used_bits.l_exp_flags));
		data[i].fx = random_between(0, set_n_bits(max_used_bits.l_fx));
		data[i].ncob_x = random_between(0, set_n_bits(max_used_bits.l_ncob));
		data[i].ncob_y = random_between(0, set_n_bits(max_used_bits.l_ncob));
		data[i].fx_variance = random_between(0, set_n_bits(max_used_bits.l_fx_variance));
		data[i].cob_x_variance = random_between(0, set_n_bits(max_used_bits.l_cob_variance));
		data[i].cob_y_variance = random_between(0, set_n_bits(max_used_bits.l_cob_variance));
	}
}


static void gen_l_fx_efx_ncob_ecob_data(struct l_fx_efx_ncob_ecob *data, uint32_t samples)
{
	uint32_t i;
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	for (i = 0; i < samples; i++) {
		data[i].exp_flags = random_between(0, set_n_bits(max_used_bits.l_exp_flags));
		data[i].fx = random_between(0, set_n_bits(max_used_bits.l_fx));
		data[i].ncob_x = random_between(0, set_n_bits(max_used_bits.l_ncob));
		data[i].ncob_y = random_between(0, set_n_bits(max_used_bits.l_ncob));
		data[i].efx = random_between(0, set_n_bits(max_used_bits.l_efx));
		data[i].ecob_x = random_between(0, set_n_bits(max_used_bits.l_ecob));
		data[i].ecob_y = random_between(0, set_n_bits(max_used_bits.l_ecob));
		data[i].fx_variance = random_between(0, set_n_bits(max_used_bits.l_fx_variance));
		data[i].cob_x_variance = random_between(0, set_n_bits(max_used_bits.l_cob_variance));
		data[i].cob_y_variance = random_between(0, set_n_bits(max_used_bits.l_cob_variance));
	}
}


/**
 * @brief generate random test data
 *
 * @param samples	number of random test samples
 * @param data_type	compression data type of the test data
 *
 * @returns a pointer to the generated random test data
 */

void *generate_random_test_data(uint32_t samples, enum cmp_data_type data_type)
{
	size_t data_size = cmp_cal_size_of_data(samples, data_type);
	void *data = malloc(data_size);
	void *data_cpy = data;
	uint8_t *p = data;

	TEST_ASSERT_NOT_EQUAL_INT(data_size, 0);
	TEST_ASSERT(data_size < (CMP_ENTITY_MAX_SIZE - NON_IMAGETTE_HEADER_SIZE));
	TEST_ASSERT_NOT_NULL(data);

	if (!rdcu_supported_data_type_is_used(data_type)) {
		int i;
		TEST_ASSERT(data_size > MULTI_ENTRY_HDR_SIZE);
		for (i = 0; i < MULTI_ENTRY_HDR_SIZE; ++i) {
			*p++ = random_between(0, UINT8_MAX);
		}
		data = p;
	}

	switch (data_type) {
	case DATA_TYPE_IMAGETTE:
	case DATA_TYPE_IMAGETTE_ADAPTIVE:
	case DATA_TYPE_SAT_IMAGETTE:
	case DATA_TYPE_SAT_IMAGETTE_ADAPTIVE:
	case DATA_TYPE_F_CAM_IMAGETTE:
	case DATA_TYPE_F_CAM_IMAGETTE_ADAPTIVE:
		gen_ima_data(data, samples);
		break;
	case DATA_TYPE_OFFSET:
		gen_offset_data(data, samples);
		break;
	case DATA_TYPE_BACKGROUND:
		gen_background_data(data, samples);
		break;
	case DATA_TYPE_SMEARING:
		gen_smearing_data(data, samples);
		break;
	case DATA_TYPE_S_FX:
		gen_s_fx_data(data, samples);
		break;
	case DATA_TYPE_S_FX_EFX:
		gen_s_fx_efx_data(data, samples);
		break;
	case DATA_TYPE_S_FX_NCOB:
		gen_s_fx_ncob_data(data, samples);
		break;
	case DATA_TYPE_S_FX_EFX_NCOB_ECOB:
		gen_s_fx_efx_ncob_ecob_data(data, samples);
		break;
	case DATA_TYPE_L_FX:
		gen_l_fx_data(data, samples);
		break;
	case DATA_TYPE_L_FX_EFX:
		gen_l_fx_efx_data(data, samples);
		break;
	case DATA_TYPE_L_FX_NCOB:
		gen_l_fx_ncob_data(data, samples);
		break;
	case DATA_TYPE_L_FX_EFX_NCOB_ECOB:
		gen_l_fx_efx_ncob_ecob_data(data, samples);
		break;
	case DATA_TYPE_F_FX:
		gen_f_fx_data(data, samples);
		break;
	case DATA_TYPE_F_FX_EFX:
		gen_f_fx_efx_data(data, samples);
		break;
	case DATA_TYPE_F_FX_NCOB:
		gen_f_fx_ncob_data(data, samples);
		break;
	case DATA_TYPE_F_FX_EFX_NCOB_ECOB:
		gen_f_fx_efx_ncob_ecob_data(data, samples);
		break;
	case DATA_TYPE_F_CAM_OFFSET: /* TODO: implement this */
	case DATA_TYPE_F_CAM_BACKGROUND: /* TODO: implement this */
	default:
		TEST_FAIL();
	}

	return data_cpy;
}


/**
 * @brief generate random compression configuration
 *
 * @param cfg	pointer to a compression configuration
 *
 */

void generate_random_cmp_par(struct cmp_cfg *cfg)
{
	cfg->golomb_par = random_between(MIN_IMA_GOLOMB_PAR, MAX_IMA_GOLOMB_PAR);
	cfg->ap1_golomb_par = random_between(MIN_IMA_GOLOMB_PAR, MAX_IMA_GOLOMB_PAR);
	cfg->ap2_golomb_par = random_between(MIN_IMA_GOLOMB_PAR, MAX_IMA_GOLOMB_PAR);

	cfg->cmp_par_exp_flags = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);
	cfg->cmp_par_fx = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);
	cfg->cmp_par_ncob = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);
	cfg->cmp_par_efx = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);
	cfg->cmp_par_ecob = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);
	cfg->cmp_par_fx_cob_variance = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);
	cfg->cmp_par_mean = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);
	cfg->cmp_par_variance = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);
	cfg->cmp_par_pixels_error = random_between(MIN_ICU_GOLOMB_PAR, MAX_ICU_GOLOMB_PAR);


	cfg->spill = random_between(MIN_IMA_SPILL, cmp_ima_max_spill(cfg->golomb_par));
	cfg->ap1_spill = random_between(MIN_IMA_SPILL, cmp_ima_max_spill(cfg->ap1_golomb_par));
	cfg->ap2_spill = random_between(MIN_IMA_SPILL, cmp_ima_max_spill(cfg->ap2_golomb_par));

	cfg->spill_exp_flags = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_exp_flags));
	cfg->spill_fx = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_fx));
	cfg->spill_ncob = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_ncob));
	cfg->spill_efx = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_efx));
	cfg->spill_ecob = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_ecob));
	cfg->spill_fx_cob_variance = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_fx_cob_variance));
	cfg->spill_mean = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_mean));
	cfg->spill_variance = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_variance));
	cfg->spill_pixels_error = random_between(MIN_ICU_SPILL, cmp_icu_max_spill(cfg->cmp_par_pixels_error));
#if 0
	if (cfg->cmp_mode == CMP_MODE_STUFF) {
		/* cfg->golomb_par = random_between(16, MAX_STUFF_CMP_PAR); */
		cfg->golomb_par = 16;
		cfg->ap1_golomb_par = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->ap2_golomb_par = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_exp_flags = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_fx = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_ncob = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_efx = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_ecob = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_fx_cob_variance = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_mean = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_variance = random_between(0, MAX_STUFF_CMP_PAR);
		cfg->cmp_par_pixels_error = random_between(0, MAX_STUFF_CMP_PAR);
		return;
	}
#endif
}


/**
 * @brief compress the given configuration and decompress it afterwards; finally
 *	compare the results
 *
 * @param cfg	pointer to a compression configuration
 */

void compression_decompression(struct cmp_cfg *cfg)
{
	int cmp_size_bits, s, error;
	int data_size, cmp_data_size;
	struct cmp_entity *ent;
	void *decompressed_data;
	static void *model_of_data;
	void *updated_model = NULL;

	TEST_ASSERT_NOT_NULL(cfg);

	TEST_ASSERT_NULL(cfg->icu_output_buf);

	data_size = cmp_cal_size_of_data(cfg->samples, cfg->data_type);

	/* create a compression entity */
	cmp_data_size = cmp_cal_size_of_data(cfg->buffer_length, cfg->data_type);
	cmp_data_size &= ~0x3; /* the size of the compressed data should be a multiple of 4 */
	TEST_ASSERT_NOT_EQUAL_INT(0, cmp_data_size);

	s = cmp_ent_create(NULL, cfg->data_type, cfg->cmp_mode == CMP_MODE_RAW, cmp_data_size);
	TEST_ASSERT_NOT_EQUAL_INT(0, s);
	ent = malloc(s); TEST_ASSERT_TRUE(ent);
	s = cmp_ent_create(ent, cfg->data_type, cfg->cmp_mode == CMP_MODE_RAW, cmp_data_size);
	TEST_ASSERT_NOT_EQUAL_INT(0, s);

	/* we put the coompressed data direct into the compression entity */
	cfg->icu_output_buf = cmp_ent_get_data_buf(ent);
	TEST_ASSERT_NOT_NULL(cfg->icu_output_buf);

	/* now compress the data */
	cmp_size_bits = icu_compress_data(cfg);
	TEST_ASSERT(cmp_size_bits > 0);

	/* put the compression parameters in the entity header */
	error = cmp_ent_write_cmp_pars(ent, cfg, cmp_size_bits);
	TEST_ASSERT_FALSE(error);

	/* allocate the buffers for decompression */
	TEST_ASSERT_NOT_EQUAL_INT(0, data_size);
	s = decompress_cmp_entiy(ent, model_of_data, NULL, NULL);
	decompressed_data = malloc(s); TEST_ASSERT_NOT_NULL(decompressed_data);

	if (model_mode_is_used(cfg->cmp_mode)) {
		updated_model = malloc(data_size);
		TEST_ASSERT_NOT_NULL(updated_model);
	}

	/* now we try to decompress the data */
	s = decompress_cmp_entiy(ent, model_of_data, updated_model, decompressed_data);
	TEST_ASSERT_EQUAL_INT(data_size, s);
	TEST_ASSERT_FALSE(memcmp(decompressed_data, cfg->input_buf, data_size));

	if (model_mode_is_used(cfg->cmp_mode)) {
		TEST_ASSERT_NOT_NULL(updated_model);
		TEST_ASSERT_NOT_NULL(model_of_data);
		TEST_ASSERT_FALSE(memcmp(updated_model, cfg->icu_new_model_buf, data_size));
		memcpy(model_of_data, updated_model, data_size);
	} else { /* non-model mode */
		/* reset model */
		free(model_of_data);
		model_of_data = malloc(data_size);
		memcpy(model_of_data, decompressed_data, data_size);
	}

	cfg->icu_output_buf = NULL;
	free(ent);
	free(decompressed_data);
	free(updated_model);
}


#define CMP_BUFFER_FAKTOR 3 /* compression data buffer size / data to compress buffer size */

/**
 * @brief random compression decompression test
 * @detail We generate random data and compress them with random parameters.
 *	After that we put the data in a compression entity. We decompress the
 *	compression entity and compare the decompressed data with the original
 *	data.
 * @test icu_compress_data
 * @test decompress_cmp_entiy
 */

void test_random_compression_decompression(void)
{
	enum cmp_data_type data_type;
	enum cmp_mode cmp_mode;
	struct cmp_cfg cfg;
	int cmp_buffer_size;

	/* TODO: extend test for DATA_TYPE_F_CAM_BACKGROUND, DATA_TYPE_F_CAM_OFFSET  */
	for (data_type = 1; data_type <= DATA_TYPE_F_CAM_IMAGETTE_ADAPTIVE; data_type++) {
		/* printf("%s\n", data_type2string(data_type)); */
		/* generate random data*/
		uint32_t samples = random_between(1, 430179/CMP_BUFFER_FAKTOR);
		uint32_t model_value = random_between(0, MAX_MODEL_VALUE);
		void *data_to_compress1 = generate_random_test_data(samples, data_type);
		void *data_to_compress2 = generate_random_test_data(samples, data_type);
		void *updated_model = calloc(1, cmp_cal_size_of_data(samples, data_type));
		/* for (cmp_mode = CMP_MODE_RAW; cmp_mode <= CMP_MODE_STUFF; cmp_mode++) { */
		for (cmp_mode = CMP_MODE_RAW; cmp_mode < CMP_MODE_STUFF; cmp_mode++) {
			/* printf("cmp_mode: %i\n", cmp_mode); */
			cfg = cmp_cfg_icu_create(data_type, cmp_mode, model_value,
						 CMP_LOSSLESS);
			TEST_ASSERT_NOT_EQUAL_INT(cfg.data_type, DATA_TYPE_UNKNOWN);

			generate_random_cmp_par(&cfg);

			if (!model_mode_is_used(cmp_mode))
				cmp_buffer_size = cmp_cfg_icu_buffers(&cfg, data_to_compress1,
					samples, NULL, NULL, NULL, samples*CMP_BUFFER_FAKTOR);
			else
				cmp_buffer_size = cmp_cfg_icu_buffers(&cfg, data_to_compress2,
					samples, data_to_compress1, updated_model, NULL, samples*CMP_BUFFER_FAKTOR);

			TEST_ASSERT_EQUAL_INT(cmp_buffer_size, cmp_cal_size_of_data(CMP_BUFFER_FAKTOR*samples, data_type));

			compression_decompression(&cfg);
		}
		free(data_to_compress1);
		free(data_to_compress2);
		free(updated_model);
	}
}
