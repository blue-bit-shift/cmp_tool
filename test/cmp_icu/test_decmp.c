#include <string.h>
#include <stdlib.h>

#include "unity.h"

#include "compiler.h"
#include "cmp_debug.h"
#include "cmp_entity.h"
#include "../../lib/cmp_icu.c" /* .c file included to test static functions */
#include "../../lib/decmp.c" /* .c file included to test static functions */


/* returns the needed size of the compression entry header plus the max size of the
 * compressed data if ent ==  NULL if ent is set the size of the compression
 * entry */
size_t icu_compress_data_entity(struct cmp_entity *ent, const struct cmp_cfg *cfg)
{
	size_t s, hdr_size;
	struct cmp_cfg cfg_cpy;
	int cmp_size_bits;

	if (!cfg)
		return 0;

	if (cfg->icu_output_buf)
		debug_print("Warning the set buffer for the compressed data is ignored! The compressed data are write to the compression entry.");

	if (!ent) {
		s = cmp_cal_size_of_data(cfg->buffer_length, cfg->data_type);
		if (!s)
			return 0;

		hdr_size = cmp_ent_cal_hdr_size(cfg->data_type, cfg->cmp_mode == CMP_MODE_RAW);
		if (!hdr_size)
			return 0;

		return s + hdr_size;
	}

	cfg_cpy = *cfg;
	cfg_cpy.icu_output_buf = cmp_ent_get_data_buf(ent);

	cmp_size_bits = icu_compress_data(&cfg_cpy);
	if (cmp_size_bits < 0)
		return 0;

	s = cmp_ent_create(ent, cfg->data_type, cfg->cmp_mode == CMP_MODE_RAW,
					 cmp_bit_to_4byte(cmp_size_bits));
	if (!s)
		return 0;

	if (cmp_ent_write_cmp_pars(ent, cfg, cmp_size_bits))
		return 0;

	return s;
}


void test_cmp_decmp_n_imagette_raw(void)
{
	int cmp_size;
	size_t s, i;
	struct cmp_cfg cfg = cmp_cfg_icu_create(DATA_TYPE_IMAGETTE, CMP_MODE_RAW, 0, CMP_LOSSLESS);
	uint16_t data[] = {0, 1, 2, 0x42, INT16_MIN, INT16_MAX, UINT16_MAX};
	uint32_t *compressed_data;
	uint16_t *decompressed_data;
	struct cmp_entity *ent;

	s = cmp_cfg_icu_buffers(&cfg, data, ARRAY_SIZE(data), NULL, NULL,
				NULL, ARRAY_SIZE(data));
	TEST_ASSERT_TRUE(s);
	compressed_data = malloc(s);
	TEST_ASSERT_TRUE(compressed_data);
	s = cmp_cfg_icu_buffers(&cfg, data, ARRAY_SIZE(data), NULL, NULL,
				compressed_data, ARRAY_SIZE(data));
	TEST_ASSERT_TRUE(s);

	cmp_size = icu_compress_data(&cfg);
	TEST_ASSERT_EQUAL_INT(sizeof(data)*CHAR_BIT, cmp_size);

	s = cmp_ent_build(NULL, 0, 0, 0, 0, 0, &cfg, cmp_bit_to_4byte(cmp_size));
	TEST_ASSERT_TRUE(s);
	ent = malloc(s);
	TEST_ASSERT_TRUE(ent);
	s = cmp_ent_build(ent, 0, 0, 0, 0, 0, &cfg, cmp_bit_to_4byte(cmp_size));
	TEST_ASSERT_TRUE(s);
	memcpy(cmp_ent_get_data_buf(ent), compressed_data, (cmp_size+7)/8);

	s = decompress_cmp_entiy(ent, NULL, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(sizeof(data), s);
	decompressed_data = malloc(s);
	TEST_ASSERT_TRUE(decompressed_data);
	s = decompress_cmp_entiy(ent, NULL, NULL, decompressed_data);
	TEST_ASSERT_EQUAL_INT(sizeof(data), s);

	for (i = 0; i < ARRAY_SIZE(data); ++i) {
		TEST_ASSERT_EQUAL_INT(data[i], decompressed_data[i]);

	}

	free(compressed_data);
	free(ent);
	free(decompressed_data);
}


/*
 * @test re_map_to_pos
 */

void test_re_map_to_pos(void)
{
	int j;
	uint32_t input, result;
	unsigned int max_value_bits;

	input = INT32_MIN;
	result = re_map_to_pos(map_to_pos(input, 32));
	TEST_ASSERT_EQUAL_INT32(input, result);

	input = INT32_MAX;
	result = re_map_to_pos(map_to_pos(input, 32));
	TEST_ASSERT_EQUAL_INT32(input, result);

	input = -1;
	result = re_map_to_pos(map_to_pos(input, 32));
	TEST_ASSERT_EQUAL_INT32(input, result);

	input = 0;
	result = re_map_to_pos(map_to_pos(input, 32));
	TEST_ASSERT_EQUAL_INT32(input, result);

	input = 1; max_value_bits = 6;
	result = re_map_to_pos(map_to_pos(input, max_value_bits));
	TEST_ASSERT_EQUAL_INT32(input, result);

	for (j = -16; j < 15; j++) {
		uint32_t map_val =  map_to_pos(j, 16) & 0x3F;
		result = re_map_to_pos(map_val);
		TEST_ASSERT_EQUAL_INT32(j, result);
	}

	for (j = INT16_MIN; j < INT16_MAX; j++) {
		uint32_t map_val =  map_to_pos(j, 16) & 0xFFFF;
		result = re_map_to_pos(map_val);
		TEST_ASSERT_EQUAL_INT32(j, result);
	}
#if 0
	for (j = INT32_MIN; j < INT32_MAX; j++) {
		result = re_map_to_pos(map_to_pos(j, 32));
		TEST_ASSERT_EQUAL_INT32(j, result);
	}
#endif
}


void test_decode_normal(void)
{
	uint32_t decoded_value;
	int stream_pos, sample;
	 /* compressed data from 0 to 6; */
	uint32_t cmp_data[] = {0x5BBDF7E0};
	struct decoder_setup setup = {0};

	cpu_to_be32s(cmp_data);

	setup.decode_cw_f = rice_decoder;
	setup.encoder_par1 = 1;
	setup.encoder_par2 = ilog_2(setup.encoder_par1);
	setup.bitstream_adr = cmp_data;
	setup.max_stream_len = 32;
	setup.max_cw_len = 16;

	stream_pos = 0;
	for (sample = 0; sample < 7; sample++) {
		stream_pos = decode_normal(&decoded_value, stream_pos, &setup);
		TEST_ASSERT_EQUAL_HEX(sample, decoded_value);
	}

	 /* TODO error case: negativ stream_pos */
}


void test_decode_zero(void)
{
	uint32_t decoded_value;
	int stream_pos;
	uint32_t cmp_data[] = {0x88449FE0};
	struct decoder_setup setup = {0};
	struct cmp_cfg cfg = {0};

	cpu_to_be32s(cmp_data);

	cfg.data_type = DATA_TYPE_IMAGETTE;
	cfg.cmp_mode = CMP_MODE_DIFF_ZERO;
	cfg.icu_output_buf = cmp_data;
	cfg.buffer_length = 4;

	int err = configure_decoder_setup(&setup, 1, 8, CMP_LOSSLESS, 16, &cfg);
	TEST_ASSERT_FALSE(err);

	stream_pos = 0;

	stream_pos = decode_zero(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(0, decoded_value);
	stream_pos = decode_zero(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(0x4223, decoded_value);
	stream_pos = decode_zero(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(7, decoded_value);
	TEST_ASSERT_EQUAL_INT(28, stream_pos);

	 /* TODO error case: negativ stream_pos */
}

void test_decode_multi(void)
{
	uint32_t decoded_value;
	int stream_pos;
	uint32_t cmp_data[] = {0x16B66DF8, 0x84360000};
	struct decoder_setup setup = {0};
	struct cmp_cfg cfg = {0};

	cpu_to_be32s(&cmp_data[0]);
	cpu_to_be32s(&cmp_data[1]);

	cfg.data_type = DATA_TYPE_IMAGETTE;
	cfg.cmp_mode = CMP_MODE_DIFF_MULTI;
	cfg.icu_output_buf = cmp_data;
	cfg.buffer_length = 8;

	int err = configure_decoder_setup(&setup, 3, 8, CMP_LOSSLESS, 16, &cfg);
	TEST_ASSERT_FALSE(err);

	stream_pos = 0;

	stream_pos = decode_multi(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(0, decoded_value);
	stream_pos = decode_multi(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(1, decoded_value);
	stream_pos = decode_multi(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(7, decoded_value);
	stream_pos = decode_multi(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(8, decoded_value);
	stream_pos = decode_multi(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(9, decoded_value);
	stream_pos = decode_multi(&decoded_value, stream_pos, &setup);
	TEST_ASSERT_EQUAL_HEX(0x4223, decoded_value);
	TEST_ASSERT_EQUAL_INT(47, stream_pos);

}


void test_decompress_imagette_model(void)
{
	uint16_t data[5]  = {0};
	uint16_t model[5] = {0, 1, 2, 3, 4};
	uint16_t up_model[5]  = {0};
	uint32_t cmp_data[] = {0};
	struct cmp_cfg cfg = {0};
	int stream_pos;

	cmp_data[0] = cpu_to_be32(0x49240000);

	cfg.data_type = DATA_TYPE_IMAGETTE;
	cfg.cmp_mode = CMP_MODE_MODEL_MULTI;
	cfg.input_buf = data;
	cfg.model_buf = model;
	cfg.icu_new_model_buf = up_model;
	cfg.icu_output_buf = cmp_data;
	cfg.buffer_length = 4;
	cfg.samples = 5;
	cfg.model_value = 16;
	cfg.golomb_par = 4;
	cfg.spill = 48;

	stream_pos = decompress_imagette(&cfg);
	TEST_ASSERT_EQUAL_INT(15, stream_pos);
	TEST_ASSERT_EQUAL_HEX(1, data[0]);
	TEST_ASSERT_EQUAL_HEX(2, data[1]);
	TEST_ASSERT_EQUAL_HEX(3, data[2]);
	TEST_ASSERT_EQUAL_HEX(4, data[3]);
	TEST_ASSERT_EQUAL_HEX(5, data[4]);

	TEST_ASSERT_EQUAL_HEX(0, up_model[0]);
	TEST_ASSERT_EQUAL_HEX(1, up_model[1]);
	TEST_ASSERT_EQUAL_HEX(2, up_model[2]);
	TEST_ASSERT_EQUAL_HEX(3, up_model[3]);
	TEST_ASSERT_EQUAL_HEX(4, up_model[4]);
}

int my_random(unsigned int min, unsigned int max)
{
	if (max-min > RAND_MAX)
		TEST_ASSERT(0);
	if (min > max)
		TEST_ASSERT(0);
	return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

/* #include <cmp_io.h> */
/* void test_imagette_1(void) */
/* { */
/* 	size_t i; */

/* 	enum cmp_mode cmp_mode = 1; */
/* 	uint16_t model_value = 10; */
/* 	struct cmp_cfg cfg = cmp_cfg_icu_create(DATA_TYPE_IMAGETTE, cmp_mode, model_value, CMP_LOSSLESS); */
/* 	unsigned int samples = 1; */
/* 	uint16_t data[1] = {0x8000}; */
/* 	uint16_t model[1] = {0}; */
/* 	uint16_t up_model[1] = {0}; */
/* 	uint16_t de_up_model[1] = {0}; */
/* 	uint32_t compressed_data[1]; */

/* 	size_t s = cmp_cfg_buffers(&cfg, data, samples, model, up_model, */
/* 			      compressed_data, 2*samples); */
/* 	TEST_ASSERT(s > 0); */

/* 	uint32_t golomb_par = 9; */
/* 	uint32_t spill = 44; */

/* 	cfg.spill = spill; */
/* 	cfg.golomb_par = golomb_par; */
/* 	/1* print_cfg(&cfg, 0); *1/ */
/* 	int cmp_size = icu_compress_data(&cfg, NULL); */
/* 	TEST_ASSERT(cmp_size > 0); */


/* 	s = cmp_ent_build(NULL, 0, 0, 0, 0, 0, &cfg, cmp_size); */
/* 	TEST_ASSERT_TRUE(s); */
/* 	struct cmp_entity *ent = malloc(s); */
/* 	TEST_ASSERT_TRUE(ent); */
/* 	s = cmp_ent_build(ent, 0, 0, 0, 0, 0, &cfg, cmp_size); */
/* 	TEST_ASSERT_TRUE(s); */
/* 	memcpy(cmp_ent_get_data_buf(ent), compressed_data, cmp_bit_to_4byte(cmp_size)); */

/* 	s = decompress_cmp_entiy(ent, model, de_up_model, NULL); */
/* 	TEST_ASSERT_EQUAL_INT(samples * sizeof(*data), s); */
/* 	uint16_t *decompressed_data = malloc(s); */
/* 	TEST_ASSERT_TRUE(decompressed_data); */
/* 	s = decompress_cmp_entiy(ent, model, de_up_model, decompressed_data); */
/* 	TEST_ASSERT_EQUAL_INT(samples * sizeof(*data), s); */

/* 	for (i = 0; i < samples; ++i) { */
/* 		if (data[i] != decompressed_data[i]) */
/* 			TEST_ASSERT(0); */
/* 		/1* TEST_ASSERT_EQUAL_HEX16(data[i], decompressed_data[i]); *1/ */
/* 		/1* TEST_ASSERT_EQUAL_HEX16(up_model[i], de_up_model[i]); *1/ */
/* 	} */


/* 	free(ent); */
/* 	free(decompressed_data); */
/* } */

#include <unistd.h>
#include "cmp_io.h"

void test_imagette_random(void)
{
	unsigned int seed = time(NULL) * getpid();
	size_t i, s, cmp_data_size;
	int error;
	struct cmp_cfg cfg;
	struct cmp_entity *ent;

	enum cmp_mode cmp_mode = my_random(0, 4);
	enum cmp_data_type data_type = DATA_TYPE_IMAGETTE;
	uint16_t model_value = my_random(0, MAX_MODEL_VALUE);
	uint32_t round = my_random(0, 3);
	uint32_t samples, compressed_data_len_samples;
	uint16_t *data, *model = NULL, *up_model = NULL, *de_up_model = NULL;

	/* Seeds the pseudo-random number generator used by rand() */
	srand(seed);
	printf("seed: %u\n", seed);

	/* create random test _data */
	samples = my_random(1, 100000);
	s = cmp_cal_size_of_data(samples, data_type);
	data = malloc(s); TEST_ASSERT_TRUE(data);
	for (i = 0; i < samples; ++i) {
		data[i] = my_random(0, UINT16_MAX);
	}
	if (model_mode_is_used(cmp_mode)) {
		model = malloc(s); TEST_ASSERT_TRUE(model);
		up_model = malloc(s); TEST_ASSERT_TRUE(up_model);
		de_up_model = malloc(s); TEST_ASSERT(de_up_model);
		for (i = 0; i < samples; ++i) {
			model[i] = my_random(0, UINT16_MAX);
		}
	}
	compressed_data_len_samples = 6*samples;

	/* create a compression configuration */
	cfg = cmp_cfg_icu_create(data_type, cmp_mode, model_value, round);
	TEST_ASSERT(cfg.data_type != DATA_TYPE_UNKOWN);


	cmp_data_size = cmp_cfg_icu_buffers(&cfg, data, samples, model, up_model,
					    NULL, compressed_data_len_samples);
	TEST_ASSERT_TRUE(cmp_data_size);

	uint32_t golomb_par = my_random(MIN_RDCU_GOLOMB_PAR, MAX_RDCU_GOLOMB_PAR);
	uint32_t max_spill = get_max_spill(golomb_par, data_type);
	TEST_ASSERT(max_spill > 1);
	uint32_t spill = my_random(2, max_spill);

	error = cmp_cfg_icu_imagette(&cfg, golomb_par, spill);
	TEST_ASSERT_FALSE(error);

	print_cfg(&cfg, 0);
	s = icu_compress_data_entity(NULL, &cfg);
	TEST_ASSERT_TRUE(s);
	ent = malloc(s); TEST_ASSERT_TRUE(ent);
	s = icu_compress_data_entity(ent, &cfg);
	TEST_ASSERT_TRUE(s);

	s = decompress_cmp_entiy(ent, model, de_up_model, NULL);
	TEST_ASSERT_EQUAL_INT(samples * sizeof(*data), s);
	uint16_t *decompressed_data = malloc(s);
	TEST_ASSERT_TRUE(decompressed_data);
	s = decompress_cmp_entiy(ent, model, de_up_model, decompressed_data);
	TEST_ASSERT_EQUAL_INT(samples * sizeof(*data), s);

	for (i = 0; i < samples; ++i) {
		printf("%u == %u (round: %u)\n", data[i], decompressed_data[i], round);
		uint32_t mask = ~0U << round;
		if ((data[i]&mask) != (decompressed_data[i]&mask))
			TEST_ASSERT(0);
		if (model_mode_is_used(cmp_mode))
			if (up_model[i] != de_up_model[i])
				TEST_ASSERT(0);
	}

	free(data);
	free(model);
	free(up_model);
	free(de_up_model);
	free(ent);
	free(decompressed_data);
}
