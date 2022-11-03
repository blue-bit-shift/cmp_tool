#include <string.h>
#include <stdlib.h>

#include "unity.h"

#include "compiler.h"
#include "cmp_debug.h"
#include "cmp_entity.h"
#include "../../lib/cmp_icu.c" /* .c file included to test static functions */
#include "../../lib/decmp.c" /* .c file included to test static functions */


#define IMAX_BITS(m) ((m)/((m)%255+1) / 255%255*8 + 7-86/((m)%255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)


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
 * returns the needed size of the compression entry header plus the max size of the
 * compressed data if ent ==  NULL if ent is set the size of the compression
 * entry (entity header + compressed data)
 */
size_t icu_compress_data_entity(struct cmp_entity *ent, const struct cmp_cfg *cfg)
{
	size_t s;
	struct cmp_cfg cfg_cpy;
	int cmp_size_bits;

	if (!cfg)
		return 0;

	if (cfg->icu_output_buf)
		debug_print("Warning the set buffer for the compressed data is ignored! The compressed data are write to the compression entry.");

	s = cmp_cal_size_of_data(cfg->buffer_length, cfg->data_type);
	if (!s)
		return 0;
	/* we round down to the next 4-byte allied address because we access the
	 * cmp_buffer in uint32_t words
	 */
	if (cfg->cmp_mode != CMP_MODE_RAW)
		s &= ~0x3U;

	s = cmp_ent_create(ent, cfg->data_type, cfg->cmp_mode == CMP_MODE_RAW, s);

	if (!ent || !s)
		return s;

	cfg_cpy = *cfg;
	cfg_cpy.icu_output_buf = cmp_ent_get_data_buf(ent);
	if (!cfg_cpy.icu_output_buf)
		return 0;
	cmp_size_bits = icu_compress_data(&cfg_cpy);
	if (cmp_size_bits < 0)
		return 0;

	/* XXX overwrite the size of the compression entity with the size of the actual
	 * size of the compressed data; not all allocated memory is normally used */
	s = cmp_ent_create(ent, cfg->data_type, cfg->cmp_mode == CMP_MODE_RAW,
			   cmp_bit_to_4byte(cmp_size_bits));

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


/**
 * @test count_leading_ones
 */

void test_count_leading_ones(void)
{
	unsigned int n_leading_bit;
	uint32_t value;

	value = 0;
	n_leading_bit = count_leading_ones(value);
	TEST_ASSERT_EQUAL_INT(0, n_leading_bit);

	value = 0x7FFFFFFF;
	n_leading_bit = count_leading_ones(value);
	TEST_ASSERT_EQUAL_INT(0, n_leading_bit);

	value = 0x80000000;
	n_leading_bit = count_leading_ones(value);
	TEST_ASSERT_EQUAL_INT(1, n_leading_bit);

	value = 0xBFFFFFFF;
	n_leading_bit = count_leading_ones(value);
	TEST_ASSERT_EQUAL_INT(1, n_leading_bit);

	value = 0xFFFF0000;
	n_leading_bit = count_leading_ones(value);
	TEST_ASSERT_EQUAL_INT(16, n_leading_bit);

	value = 0xFFFF7FFF;
	n_leading_bit = count_leading_ones(value);
	TEST_ASSERT_EQUAL_INT(16, n_leading_bit);

	value = 0xFFFFFFFF;
	n_leading_bit = count_leading_ones(value);
	TEST_ASSERT_EQUAL_INT(32, n_leading_bit);
}


/**
 * @test rice_decoder
 */

void test_rice_decoder(void)
{
	int cw_len;
	uint32_t code_word;
	unsigned int m = ~0;  /* we don't need this value */
	unsigned int log2_m;
	unsigned int decoded_cw;

	/* test log_2 to big */
	code_word = 0xE0000000;
	log2_m = 33;
	cw_len = rice_decoder(code_word, m, log2_m, &decoded_cw);
	TEST_ASSERT_EQUAL(0, cw_len);
	log2_m = UINT_MAX;
	cw_len = rice_decoder(code_word, m, log2_m, &decoded_cw);
	TEST_ASSERT_EQUAL(0, cw_len);
}


/**
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
	uint32_t decoded_value = ~0;
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

	stream_pos = 0;
	for (sample = 0; sample < 7; sample++) {
		stream_pos = decode_normal(&decoded_value, stream_pos, &setup);
		TEST_ASSERT_EQUAL_HEX(sample, decoded_value);
	}

	 /* TODO error case: negative stream_pos */
}


void test_decode_zero(void)
{
	uint32_t decoded_value = ~0;
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
	uint32_t decoded_value = ~0;
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



#define DATA_SAMPLES 5
void test_cmp_decmp_s_fx_diff(void)
{
	size_t s;
	int err;

	struct cmp_entity *ent;
	const uint32_t MAX_VALUE = ~(~0U << MAX_USED_S_FX_BITS);
	struct s_fx data_entry[DATA_SAMPLES] = {
		{0,0}, {1,23}, {2,42}, {3,MAX_VALUE}, {3,MAX_VALUE>>1} };
	uint8_t data_to_compress[MULTI_ENTRY_HDR_SIZE + sizeof(data_entry)];
	struct s_fx *decompressed_data = NULL;
	/* uint32_t *compressed_data = NULL; */
	uint32_t compressed_data_len_samples = DATA_SAMPLES;
	struct cmp_cfg cfg;

	for (s = 0; s < MULTI_ENTRY_HDR_SIZE; s++)
		data_to_compress[s] = s;
	memcpy(&data_to_compress[MULTI_ENTRY_HDR_SIZE], data_entry, sizeof(data_entry));

	cfg = cmp_cfg_icu_create(DATA_TYPE_S_FX, CMP_MODE_DIFF_MULTI,
				 CMP_PAR_UNUSED, CMP_LOSSLESS);
	TEST_ASSERT(cfg.data_type != DATA_TYPE_UNKNOWN);

	s = cmp_cfg_icu_buffers(&cfg, data_to_compress, DATA_SAMPLES, NULL, NULL,
				NULL, compressed_data_len_samples);
	TEST_ASSERT_TRUE(s);

	err = cmp_cfg_fx_cob(&cfg, 2, 6, 4, 14, CMP_PAR_UNUSED, CMP_PAR_UNUSED,
			     CMP_PAR_UNUSED, CMP_PAR_UNUSED, CMP_PAR_UNUSED,
			     CMP_PAR_UNUSED, CMP_PAR_UNUSED, CMP_PAR_UNUSED);
	TEST_ASSERT_FALSE(err);

	s = icu_compress_data_entity(NULL, &cfg);
	TEST_ASSERT_TRUE(s);
	ent = malloc(s); TEST_ASSERT_TRUE(ent);
	s = icu_compress_data_entity(ent, &cfg);
	TEST_ASSERT_TRUE(s);

	/* now decompress the data */
	s = decompress_cmp_entiy(ent, NULL, NULL, decompressed_data);
	TEST_ASSERT_EQUAL_INT(cmp_cal_size_of_data(cfg.samples, cfg.data_type), s);
	decompressed_data = malloc(s); TEST_ASSERT_TRUE(decompressed_data);
	s = decompress_cmp_entiy(ent, NULL, NULL, decompressed_data);
	TEST_ASSERT_EQUAL_INT(cmp_cal_size_of_data(cfg.samples, cfg.data_type), s);

	TEST_ASSERT_FALSE(memcmp(data_to_compress, decompressed_data, s));
	/* for (i = 0; i < samples; ++i) { */
	/* 	printf("%u == %u (round: %u)\n", data[i], decompressed_data[i], round); */
	/* 	uint32_t mask = ~0U << round; */
	/* 	if ((data[i]&mask) != (decompressed_data[i]&mask)) */
	/* 		TEST_ASSERT(0); */
	/* 	if (model_mode_is_used(cmp_mode)) */
	/* 		if (up_model[i] != de_up_model[i]) */
	/* 			TEST_ASSERT(0); */
	/* } */
	free(ent);
	free(decompressed_data);
}
#undef DATA_SAMPLES


int my_random(unsigned int min, unsigned int max)
{
	TEST_ASSERT(min < max);
	if (max-min < RAND_MAX)
		return min + rand() / (RAND_MAX / (max - min + 1ULL) + 1);
	else
		return min + rand32() / (UINT32_MAX / (max - min + 1ULL) + 1);

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
	TEST_ASSERT(cfg.data_type != DATA_TYPE_UNKNOWN);


	cmp_data_size = cmp_cfg_icu_buffers(&cfg, data, samples, model, up_model,
					    NULL, compressed_data_len_samples);
	TEST_ASSERT_TRUE(cmp_data_size);

	uint32_t golomb_par = my_random(MIN_IMA_GOLOMB_PAR, MAX_IMA_GOLOMB_PAR);
	uint32_t max_spill = cmp_ima_max_spill(golomb_par);
	TEST_ASSERT(max_spill > 1);
	uint32_t spill = my_random(2, max_spill);

	error = cmp_cfg_icu_imagette(&cfg, golomb_par, spill);
	TEST_ASSERT_FALSE(error);

	/* print_cfg(&cfg, 0); */
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
		/* printf("%u == %u (round: %u)\n", data[i], decompressed_data[i], round); */
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


void test_s_fx_diff(void)
{
	size_t s, i;
	uint8_t cmp_entity[88] = {
		0x80, 0x00, 0x00, 0x09, 0x00, 0x00, 0x58, 0x00, 0x00, 0x20, 0x04, 0xEE, 0x21, 0xBD, 0xB0, 0x1C, 0x04, 0xEE, 0x21, 0xBD, 0xB0, 0x41, 0x00, 0x08, 0x02, 0x08, 0xD0, 0x10, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xAE, 0xDE, 0x00, 0x00, 0x00, 0x73, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00,
	};

	uint8_t result_data[32] = {0};
	struct multi_entry_hdr *hdr = (struct multi_entry_hdr *)result_data;
	struct s_fx *data = (struct s_fx *)hdr->entry;
	/* put some dummy data in the header*/
	for (i = 0; i < sizeof(*hdr); ++i)
		result_data[i] = i;
	data[0].exp_flags = 0;
	data[0].fx = 0;
	data[1].exp_flags = 1;
	data[1].fx = 0xFFFFFF;
	data[2].exp_flags = 3;
	data[2].fx = 0x7FFFFF;
	data[3].exp_flags = 0;
	data[3].fx = 0;

	s = decompress_cmp_entiy((void *)cmp_entity, NULL, NULL, NULL);
	TEST_ASSERT_EQUAL_INT(sizeof(result_data), s);
	uint8_t *decompressed_data = malloc(s);
	TEST_ASSERT_TRUE(decompressed_data);
	s = decompress_cmp_entiy((void *)cmp_entity, NULL, NULL, decompressed_data);
	TEST_ASSERT_EQUAL_INT(sizeof(result_data), s);
	for (i = 0; i < s; ++i) {
		TEST_ASSERT_EQUAL(result_data[i], decompressed_data[i]);
	}
	free(decompressed_data);
}


void test_s_fx_model(void)
{
	size_t s, i;
	uint8_t compressed_data_buf[92] = {
		0x80, 0x00, 0x00, 0x09, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x20, 0x04, 0xF0, 0xC2, 0xD3, 0x47, 0xE4, 0x04, 0xF0, 0xC2, 0xD3, 0x48, 0x16, 0x00, 0x08, 0x03, 0x08, 0xD0, 0x10, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x3B, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0x5B, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0x5D, 0x80, 0x00, 0x00,
	};
	struct cmp_entity * cmp_entity = (struct cmp_entity *)compressed_data_buf;

	uint8_t model_buf[32];
	uint8_t decompressed_data[32];
	uint8_t up_model_buf[32];
	uint8_t exp_data_buf[32] = {0}; /* expected uncompressed data */
	uint8_t exp_up_model_buf[32] = {0};

	struct multi_entry_hdr *model_collection = (struct multi_entry_hdr *)model_buf;
	struct s_fx *model_data = (struct s_fx *)model_collection->entry;

	memset(model_collection, 0xFF, sizeof(*model_collection));
	model_data[0].exp_flags = 0;
	model_data[0].fx = 0;
	model_data[1].exp_flags = 3;
	model_data[1].fx = 0x7FFFFF;
	model_data[2].exp_flags = 0;
	model_data[2].fx = 0xFFFFFF;
	model_data[3].exp_flags = 3;
	model_data[3].fx = 0xFFFFFF;

	struct multi_entry_hdr *exp_data_collection = (struct multi_entry_hdr *)exp_data_buf;
	struct s_fx *exp_data = (struct s_fx *)exp_data_collection->entry;
	/* put some dummy data in the header */
	for (i = 0; i < sizeof(*exp_data_collection); i++)
		exp_data_buf[i] = i;
	exp_data[0].exp_flags = 0;
	exp_data[0].fx = 0;
	exp_data[1].exp_flags = 1;
	exp_data[1].fx = 0xFFFFFF;
	exp_data[2].exp_flags = 3;
	exp_data[2].fx = 0x7FFFFF;
	exp_data[3].exp_flags = 0;
	exp_data[3].fx = 0;

	struct multi_entry_hdr *exp_up_model_collection = (struct multi_entry_hdr *)exp_up_model_buf;
	struct s_fx *exp_updated_model_data = (struct s_fx *)exp_up_model_collection->entry;
	/* put some dummy data in the header*/
	for (i = 0; i < sizeof(*exp_up_model_collection); i++)
		exp_up_model_buf[i] = i;
	exp_updated_model_data[0].exp_flags = 0;
	exp_updated_model_data[0].fx = 0;
	exp_updated_model_data[1].exp_flags = 2;
	exp_updated_model_data[1].fx = 0xBFFFFF;
	exp_updated_model_data[2].exp_flags = 1;
	exp_updated_model_data[2].fx = 0xBFFFFF;
	exp_updated_model_data[3].exp_flags = 1;
	exp_updated_model_data[3].fx = 0x7FFFFF;

	s = decompress_cmp_entiy(cmp_entity, model_buf, up_model_buf, decompressed_data);
	TEST_ASSERT_EQUAL_INT(sizeof(exp_data_buf), s);

	TEST_ASSERT_FALSE(memcmp(exp_data_buf, decompressed_data, s));
	TEST_ASSERT_FALSE(memcmp(exp_up_model_buf, up_model_buf, s));
}

/* TODO: implement this! */
void generate_random_test_data(void *data, int samples,
			       enum cmp_data_type data_type)
{
	uint32_t s = cmp_cal_size_of_data(samples, data_type);
	memset(data, 0x0, s);
}


void test_random_compression_decompression(void)
{
	size_t s;
	struct cmp_cfg cfg = {0};
	struct cmp_entity *cmp_ent;
	void *decompressed_data;
	void *decompressed_up_model = NULL;

	srand(0); /* TODO:XXX*/
	puts("--------------------------------------------------------------");

	/* for (cfg.data_type = DATA_TYPE_IMAGETTE; TODO:!! implement this */
	/*      cfg.data_type < DATA_TYPE_F_CAM_BACKGROUND+1; cfg.data_type++) { */
	for (cfg.data_type = DATA_TYPE_IMAGETTE;
	     cfg.data_type < DATA_TYPE_F_CAM_IMAGETTE_ADAPTIVE+1; cfg.data_type++) {
		cfg.samples = my_random(1,0x30000);
			if (cfg.data_type == DATA_TYPE_OFFSET)
				puts("FADF");

		cfg.buffer_length = (CMP_ENTITY_MAX_SIZE - NON_IMAGETTE_HEADER_SIZE - MULTI_ENTRY_HDR_SIZE)/size_of_a_sample(cfg.data_type);;
		s = cmp_cal_size_of_data(cfg.samples, cfg.data_type);
		printf("%s\n", data_type2string(cfg.data_type));
		TEST_ASSERT_TRUE(s);
		cfg.input_buf = calloc(1, s);
		TEST_ASSERT_NOT_NULL(cfg.input_buf);
		cfg.model_buf = calloc(1, s);
		TEST_ASSERT_TRUE(cfg.model_buf);
		decompressed_data = calloc(1, s);
		TEST_ASSERT_NOT_NULL(decompressed_data);
		cfg.icu_new_model_buf = calloc(1, s);
		TEST_ASSERT_TRUE(cfg.icu_new_model_buf);
		decompressed_up_model = calloc(1, s);
		TEST_ASSERT_TRUE(decompressed_up_model);
		cmp_ent = calloc(1, CMP_ENTITY_MAX_SIZE);

		generate_random_test_data(cfg.input_buf, cfg.samples, cfg.data_type);
		generate_random_test_data(cfg.model_buf, cfg.samples, cfg.data_type);

		cfg.model_value = my_random(0,16);
		/* cfg.round = my_random(0,3); /1* XXX *1/ */
		cfg.round = 0;

		cfg.golomb_par = my_random(MIN_IMA_GOLOMB_PAR, MAX_IMA_GOLOMB_PAR);
		cfg.ap1_golomb_par = my_random(MIN_IMA_GOLOMB_PAR, MAX_IMA_GOLOMB_PAR);
		cfg.ap2_golomb_par = my_random(MIN_IMA_GOLOMB_PAR, MAX_IMA_GOLOMB_PAR);
		cfg.cmp_par_exp_flags = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);
		cfg.cmp_par_fx = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);
		cfg.cmp_par_ncob = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);
		cfg.cmp_par_efx = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);
		cfg.cmp_par_ecob = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);
		cfg.cmp_par_fx_cob_variance = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);
		cfg.cmp_par_mean = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);
		cfg.cmp_par_variance = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);
		cfg.cmp_par_pixels_error = my_random(MIN_NON_IMA_GOLOMB_PAR, MAX_NON_IMA_GOLOMB_PAR);

		cfg.spill = my_random(MIN_IMA_SPILL, cmp_ima_max_spill(cfg.golomb_par));
		cfg.ap1_spill = my_random(MIN_IMA_SPILL, cmp_ima_max_spill(cfg.ap1_golomb_par));
		cfg.ap2_spill = my_random(MIN_IMA_SPILL, cmp_ima_max_spill(cfg.ap2_golomb_par));
		if (!rdcu_supported_data_type_is_used(cfg.data_type)) {
			cfg.spill_exp_flags = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_exp_flags));
			cfg.spill_fx = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_fx));
			cfg.spill_ncob = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_ncob));
			cfg.spill_efx = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_efx));
			cfg.spill_ecob = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_ecob));
			cfg.spill_fx_cob_variance = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_fx_cob_variance));
			cfg.spill_mean = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_mean));
			cfg.spill_variance = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_variance));
			cfg.spill_pixels_error = my_random(MIN_NON_IMA_SPILL, cmp_icu_max_spill(cfg.cmp_par_pixels_error));
		}

		for (cfg.cmp_mode = CMP_MODE_RAW; cfg.cmp_mode < CMP_MODE_STUFF; cfg.cmp_mode++) {
			int cmp_size, decompress_size;

			cmp_size = icu_compress_data_entity(cmp_ent, &cfg);
			if (cmp_size <= 0) {
				printf("cmp_size: %i\n", cmp_size);
				cmp_cfg_print(&cfg);
			}
			TEST_ASSERT_GREATER_THAN(0, cmp_size);

			/* now decompress the data */
			decompress_size = decompress_cmp_entiy(cmp_ent, cfg.model_buf,
							       decompressed_up_model, decompressed_data);

			TEST_ASSERT_EQUAL_INT(s, decompress_size);
			if (memcmp(cfg.input_buf, decompressed_data, s)) {
				cmp_cfg_print(&cfg);
				TEST_ASSERT_FALSE(memcmp(cfg.input_buf, decompressed_data, s));
			}
			if (model_mode_is_used(cfg.cmp_mode))
				TEST_ASSERT_FALSE(memcmp(cfg.icu_new_model_buf, decompressed_up_model, s));

			memset(cmp_ent, 0, CMP_ENTITY_MAX_SIZE);
			memset(decompressed_data, 0, s);
			memset(decompressed_up_model, 0, s);
			memset(cfg.icu_new_model_buf, 0, s);
		}

		free(cfg.model_buf);
		cfg.model_buf = NULL;
		free(cfg.input_buf);
		cfg.input_buf = NULL;
		free(cfg.icu_new_model_buf);
		cfg.icu_new_model_buf = NULL;
		free(cmp_ent);
		cmp_ent = NULL;
		free(decompressed_data);
		decompressed_data = NULL;
		free(decompressed_up_model);
		decompressed_up_model = NULL;

	}
}

void test_decompression_error_cases(void)
{
	/* error cases model decompression without a model Buffer */
	/* error cases wrong cmp parameter; model value; usw */
}
