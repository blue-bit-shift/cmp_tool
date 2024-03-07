#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
/* #include "CUnit/Automated.h" */
/* #include "CUnit/Console.h" */


#include <stdio.h>
#include <stdlib.h>

#include "icu_cmp.c" /* .c file included to test static functions */
#include "decmp.c" /* .c file included to test static functions */

#if 0
static uint32_t re_map_to_pos(uint32_t value_to_unmap, unsigned int max_value_bits)
{
	uint32_t result;
	uint32_t mask;

	if (value_to_unmap & 0x1) { /* if uneven */
		result = -((value_to_unmap + 1) / 2);
		mask = (~0U >> (32-max_value_bits));
		result &= mask;
	} else
		result = value_to_unmap / 2;

	return result;
}
	/* TEST_ASSERT_EQUAL_INT(32, re_map_to_pos(map_to_pos(32, 6), 6)); */
	uint32_t i, j;

	for (i = 0; i < 64; ++i)
		TEST_ASSERT_EQUAL_INT(i, re_map_to_pos(map_to_pos(i, 6), 6));


	TEST_ASSERT_EQUAL_INT(63, re_map_to_pos(map_to_pos(-1U, 6), 6));
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) {
			uint32_t data = j;
			uint32_t model = i;
			/* printf("d:%d, m:%d, diff:%d\n", data, model, data-model); */
			TEST_ASSERT_EQUAL_HEX(data, (re_map_to_pos(map_to_pos(data-model, 6), 6)+model) & 0x3F);
		}
	}
	for (i = 0; i < 64*2; ++i) {
		for (j = 0; j < 64*2; j++) {
			uint32_t data = j;
			uint32_t model = i;
			/* printf("d:%d, m:%d, diff:%d\n", data, model, data-model); */
			TEST_ASSERT_EQUAL_HEX(data, (re_map_to_pos(map_to_pos(data-model, 7), 7)+model) & 0x7F);
		}
	}

#endif

/************* Test case functions ****************/
#if 0
void test_re_map_to_pos(void)
{
	uint32_t value_to_map;
	uint32_t max_data_bits;
	uint32_t mapped_value;

	/* test mapping 32 bits values */
	max_data_bits = 32;

	value_to_map = 0;
	mapped_value = re_map_to_pos(map_to_pos(value_to_map, max_data_bits), max_data_bits);
	TEST_ASSERT_EQUAL_INT(0, mapped_value);

	value_to_map = -1U;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = 1;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(2, mapped_value);

	value_to_map = 42;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(84, mapped_value);

	value_to_map = INT32_MAX;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_HEX(UINT32_MAX-1, mapped_value);

	value_to_map = INT32_MIN;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_HEX(UINT32_MAX, mapped_value);

	/* test mapping 16 bits values */
	max_data_bits = 16;

	value_to_map = -1U;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	/* test mapping 6 bits values */
	max_data_bits = 6;

	value_to_map = 0;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(0, mapped_value);

	value_to_map = -1U;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = UINT32_MAX;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = -1U & 0x3FU;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = 63;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = 1;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(2, mapped_value);

	value_to_map = 31;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(62, mapped_value);

	value_to_map = -33U; /* aka 31 */
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(62, mapped_value);

	value_to_map = -32U;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(63, mapped_value);

	value_to_map = 32;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
	TEST_ASSERT_EQUAL_INT(63, mapped_value);
}
#endif

static void test_re_map_to_pos(void)
{
	uint32_t input, result;
	unsigned int max_value_bits;

	input = 1; max_value_bits = 6;
	result = re_map_to_pos(value_to_map(input, max_value_bits));
	TEST_ASSERT_EQUAL_INT32(input, result);
}

static void lossy_rounding_16_integration_test(void)
{
	uint16_t data[8] = {42, 23, UINT16_MAX, UINT16_MAX, 0, UINT16_MAX, 0, 0};
	uint16_t comp_data[8]; /* compare data */
	uint32_t samples = 8;
	uint32_t round = 1;
	int error;
	uint32_t i;

	memcpy(comp_data, data, sizeof(comp_data));

	error = lossy_rounding_16(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_lossy_rounding_16(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		comp_data[i] &= (uint16_t)(~0x1); /* round the comparison data */
		CU_ASSERT_EQUAL(data[i], comp_data[i]);
	}
}

static void diff_16_integration_test(void)
{
	uint16_t data[8] = {42, 23, UINT16_MAX, UINT16_MAX, 0, UINT16_MAX, 0, 0};
	uint16_t comp_data[8]; /* compare data */
	uint32_t samples = 8;
	uint8_t round = 0;
	int error;
	uint32_t i;

	memcpy(comp_data, data, sizeof(comp_data));

	error = diff_16(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_16(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i], comp_data[i]);
	}
}

static void diff_16_round_integration_test(void)
{
	uint16_t data[8] = {42, 23, UINT16_MAX, UINT16_MAX, 0, UINT16_MAX, 0, 0};
	uint16_t comp_data[8]; /* compare data */
	uint32_t samples = 8;
	uint8_t round = 1;
	int error;
	uint32_t i;

	memcpy(comp_data, data, sizeof(comp_data));

	error = diff_16(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_16(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		comp_data[i] &= (uint16_t)(~0x1); /* round the comparison data */
		CU_ASSERT_EQUAL(data[i], comp_data[i]);
	}
}

static void diff_32_integration_test(void)
{
	uint32_t data[8] = {42, 23, UINT32_MAX, UINT32_MAX, 0, UINT32_MAX, 0, 0};
	uint32_t comp_data[8]; /* compare data */
	uint32_t samples = 8;
	uint8_t round = 0;
	int error;
	uint32_t i;

	memcpy(comp_data, data, sizeof(comp_data));

	error = diff_32(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_32(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i], comp_data[i]);
	}

	/* round test */
	round = 1;
	memcpy(data, comp_data, sizeof(data));

	error = diff_32(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_32(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		comp_data[i] &= (~0x1U); /* round the comparison data */
		CU_ASSERT_EQUAL(data[i], comp_data[i]);
	}
}

static void diff_S_FX_integration_test(void)
{
	struct S_FX data[8] = { {0, 42},
				{0, 23},
				{1, UINT32_MAX},
				{0, UINT32_MAX},
				{0, UINT32_MAX},
				{2, UINT32_MAX},
				{2, 0},
				{0, 0}
	};
	struct S_FX comp_data[8]; /* compare data */
	uint32_t samples = 8;
	uint8_t round = 0;
	int error;
	uint32_t i;

	memcpy(comp_data, data, sizeof(comp_data));

	error = diff_S_FX(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_S_FX(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, comp_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, comp_data[i].FX);
	}

	/* now test rounding */
	round = 2;
	memcpy(data, comp_data, sizeof(data)); /* reset data */

	error = diff_S_FX(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_S_FX(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		comp_data[i].FX &= ~0x3U; /* the last bits are lost to lossy compression */
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, comp_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, comp_data[i].FX);
	}
}


static void diff_S_FX_EFX_integration_test(void)
{
	struct S_FX_EFX data[8] = {
				  {0, 42, 1},
				  {0, 23, 0},
				  {1, UINT32_MAX, UINT32_MAX},
				  {0, UINT32_MAX, 0},
				  {0, UINT32_MAX, UINT32_MAX},
				  {2, UINT32_MAX,0},
				  {2, 0, 42},
				  {0, 0, UINT16_MAX}
	};
	struct S_FX_EFX comp_data[8]; /* compare data */
	uint32_t samples = 8;
	uint8_t round = 0;
	int error;
	uint32_t i;

	memcpy(comp_data, data, sizeof(comp_data));

	error = diff_S_FX_EFX(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_S_FX_EFX(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, comp_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, comp_data[i].FX);
		CU_ASSERT_EQUAL(data[i].EFX, comp_data[i].EFX);
	}

	/* now test rounding */
	round = 2;
	memcpy(data, comp_data, sizeof(data)); /* reset data */

	error = diff_S_FX_EFX(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_S_FX_EFX(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		comp_data[i].FX &= ~0x3U; /* the last bits are lost to lossy compression */
		comp_data[i].EFX &= ~0x3U; /* the last bits are lost to lossy compression */
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, comp_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, comp_data[i].FX);
		CU_ASSERT_EQUAL(data[i].EFX, comp_data[i].EFX);
	}
}

static void diff_S_FX_NCOB_integration_test(void)
{
	struct S_FX_NCOB data[8] = {
				  {0, 42, 1, UINT32_MAX},
				  {0, 23, 0, 0},
				  {1, UINT32_MAX, UINT32_MAX, 1},
				  {0, UINT32_MAX, 0, 2},
				  {0, UINT32_MAX, UINT32_MAX, 0x7FFFFFFF},
				  {2, UINT32_MAX, 0, 0xFFFFFFFE},
				  {2, 0, 42, 0xFFFFFFFD},
				  {0, 0, UINT16_MAX, 0xFFFFFFFA}
	};
	struct S_FX_NCOB comp_data[8]; /* compare data */
	uint32_t samples = 8;
	uint8_t round = 0;
	int error;
	uint32_t i;

	memcpy(comp_data, data, sizeof(comp_data));

	error = diff_S_FX_NCOB(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_S_FX_NCOB(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, comp_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, comp_data[i].FX);
		CU_ASSERT_EQUAL(data[i].NCOB_X, comp_data[i].NCOB_X);
		CU_ASSERT_EQUAL(data[i].NCOB_Y, comp_data[i].NCOB_Y);
	}

	/* now test rounding */
	round = 2;
	memcpy(data, comp_data, sizeof(data)); /* reset data */

	error = diff_S_FX_NCOB(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_S_FX_NCOB(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		comp_data[i].FX &= ~0x3U; /* the last bits are lost to lossy compression */
		comp_data[i].NCOB_X &= ~0x3U;
		comp_data[i].NCOB_Y &= ~0x3U;
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, comp_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, comp_data[i].FX);
		CU_ASSERT_EQUAL(data[i].NCOB_X, comp_data[i].NCOB_X);
		CU_ASSERT_EQUAL(data[i].NCOB_Y, comp_data[i].NCOB_Y);
	}
}

static void diff_S_FX_EFX_NCOB_ECOB_integration_test(void)
{
	struct S_FX_EFX_NCOB_ECOB data[8] = {
				  {0, 42, 1, UINT32_MAX, 0x7E58D953, 0x12F26EE8, 0x6579BF86},
				  {0, 23, 0, 0,0xBBDACBFA, 0x5E2630AC, 0x7286CAEC},
				  {1, UINT32_MAX, UINT32_MAX, 1, 0xE51536C7, 0x7F6B3DA6, 0x5301E79B},
				  {0, UINT32_MAX, 0, 2, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {0, UINT32_MAX, UINT32_MAX, 0x7FFFFFFF, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {2, UINT32_MAX, 0, 0xFFFFFFFE, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {2, 0, 42, 0xFFFFFFFD, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {0, 0, UINT16_MAX, 0xFFFFFFFA, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B}
	};
	struct S_FX_EFX_NCOB_ECOB comp_data[8]; /* compare data */
	uint32_t samples = 8;
	uint8_t round = 0;
	int error;
	uint32_t i;

	memcpy(comp_data, data, sizeof(comp_data));

	error = diff_S_FX_EFX_NCOB_ECOB(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_S_FX_EFX_NCOB_ECOB(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, comp_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, comp_data[i].FX);
		CU_ASSERT_EQUAL(data[i].NCOB_X, comp_data[i].NCOB_X);
		CU_ASSERT_EQUAL(data[i].NCOB_Y, comp_data[i].NCOB_Y);
		CU_ASSERT_EQUAL(data[i].EFX, comp_data[i].EFX);
		CU_ASSERT_EQUAL(data[i].ECOB_X, comp_data[i].ECOB_X);
		CU_ASSERT_EQUAL(data[i].ECOB_Y, comp_data[i].ECOB_Y);
	}

	/* now test rounding */
	round = 2;
	memcpy(data, comp_data, sizeof(data)); /* reset data */

	error = diff_S_FX_EFX_NCOB_ECOB(data, samples, round);
	CU_ASSERT_FALSE(error);

	error = de_diff_S_FX_EFX_NCOB_ECOB(data, samples, round);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		comp_data[i].FX &= ~0x3U; /* the last bits are lost to lossy compression */
		comp_data[i].NCOB_X &= ~0x3U;
		comp_data[i].NCOB_Y &= ~0x3U;
		comp_data[i].EFX &= ~0x3U;
		comp_data[i].ECOB_X &= ~0x3U;
		comp_data[i].ECOB_Y &= ~0x3U;
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, comp_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, comp_data[i].FX);
		CU_ASSERT_EQUAL(data[i].NCOB_X, comp_data[i].NCOB_X);
		CU_ASSERT_EQUAL(data[i].NCOB_Y, comp_data[i].NCOB_Y);
		CU_ASSERT_EQUAL(data[i].EFX, comp_data[i].EFX);
		CU_ASSERT_EQUAL(data[i].ECOB_X, comp_data[i].ECOB_X);
		CU_ASSERT_EQUAL(data[i].ECOB_Y, comp_data[i].ECOB_Y);
	}
}
static void model_16_integration_test(void)
{
	uint16_t data_buf[6] = {42, 23, 0, 0, UINT16_MAX, UINT16_MAX};
	uint16_t data_buf_cpy[6];
	uint16_t model_buf[6] = {23, 42, 0, UINT16_MAX, 0, UINT16_MAX};
	uint16_t de_model_buf[6];
	uint32_t samples = 6;
	uint8_t model_val = 8;
	uint8_t round = 0;
	size_t i;
	int error;

	memcpy(data_buf_cpy, data_buf, sizeof(data_buf_cpy));
	memcpy(de_model_buf, model_buf, sizeof(de_model_buf));

	error = model_16(data_buf, model_buf, samples, model_val, round);
	CU_ASSERT_FALSE(error);

	error = de_model_16(data_buf, de_model_buf, samples, model_val,
				     round);
	CU_ASSERT_FALSE(error);

	for(i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data_buf[i], data_buf_cpy[i]);
		CU_ASSERT_EQUAL(model_buf[i], de_model_buf[i]);
	}
}

static void model_16_integration_round_test(void)
{
	uint16_t data_buf[6] =  {42, 23, 0, 0, UINT16_MAX, UINT16_MAX};
	uint16_t data_buf_cpy[6];
	uint16_t model_buf[6] = {23, 42, 0, UINT16_MAX, 0, UINT16_MAX};
	uint16_t de_model_buf[6];
	uint32_t samples = 6;
	uint8_t model_val = 8;
	uint8_t round = 2;
	size_t i;
	int error;

	memcpy(data_buf_cpy, data_buf, sizeof(data_buf_cpy));
	memcpy(de_model_buf, model_buf, sizeof(de_model_buf));

	error = model_16(data_buf, model_buf, samples, model_val, round);
	CU_ASSERT_FALSE(error);

	error = de_model_16(data_buf, de_model_buf, samples, model_val,
				     round);
	CU_ASSERT_FALSE(error);

	for(i = 0; i < samples; i++) {
		data_buf_cpy[i] &= ~0x3; /* the last bits are lost to lossy compression */
	}
	CU_ASSERT_EQUAL(data_buf[0], data_buf_cpy[0]);
	CU_ASSERT_EQUAL(data_buf[1], data_buf_cpy[1]);
	CU_ASSERT_EQUAL(data_buf[2], data_buf_cpy[2]);
	CU_ASSERT_EQUAL(data_buf[3], data_buf_cpy[3]);
	CU_ASSERT_EQUAL(data_buf[4], data_buf_cpy[4]);
	CU_ASSERT_EQUAL(data_buf[5], data_buf_cpy[5]);

	CU_ASSERT_EQUAL(model_buf[0], de_model_buf[0]);
	CU_ASSERT_EQUAL(model_buf[1], de_model_buf[1]);
	CU_ASSERT_EQUAL(model_buf[2], de_model_buf[2]);
	CU_ASSERT_EQUAL(model_buf[3], de_model_buf[3]);
	CU_ASSERT_EQUAL(model_buf[4], de_model_buf[4]);
	CU_ASSERT_EQUAL(model_buf[5], de_model_buf[5]);
}

#if 0
static void model_pre_process_integration_round_all(void)
{
	uint16_t data_buf;
	uint16_t data_buf_cpy;
	uint16_t model_buf;
	uint16_t de_model_buf;
	uint32_t samples = 1;
	uint8_t model_val = 10;
	uint8_t round;
	size_t i,j,k;
	int error;
	uint16_t mask;

	for (i= 0; i <= UINT16_MAX; i+=32) {
		for (j = 0; j <= UINT16_MAX; j++) {
			for (k = 0; k < 4; k++) {
				data_buf = i;
				data_buf_cpy = i;
				model_buf = j;
				de_model_buf = j;
				round = k;
				printf("i %lX, j %lX, k %lX \n", i,j,k);

				error = model_16(&data_buf, &model_buf, samples, model_val, round);
				CU_ASSERT_FALSE(error);

				error = de_model_16(&data_buf, &de_model_buf, samples, model_val, round);
				CU_ASSERT_FALSE(error);

				mask = ~0U << round;
				data_buf_cpy &= mask;
				CU_ASSERT_EQUAL(data_buf, data_buf_cpy);

				CU_ASSERT_EQUAL(model_buf, de_model_buf);
			}
		}
	}
}
#endif

static void map_to_pos_16_multi_integration_test(void)
{
	uint16_t data_buf[6] = {0, 23, -42, INT16_MAX, INT16_MIN, UINT16_MAX};
	uint16_t compare_data[6];
	uint32_t buf_len = 6;
	struct cmp_cfg cfg;
	struct cmp_info info;
	int error;

	memcpy(compare_data, data_buf, sizeof(compare_data));
	cfg.cmp_mode = MODE_DIFF_MULTI;
	info.cmp_mode_used = MODE_DIFF_MULTI;
	cfg.input_buf = data_buf;
	cfg.samples = buf_len;
	info.samples_used = buf_len;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data_buf, &info);
	CU_ASSERT_FALSE(error);

	CU_ASSERT_EQUAL(data_buf[0], compare_data[0]);
	CU_ASSERT_EQUAL(data_buf[1], compare_data[1]);
	CU_ASSERT_EQUAL(data_buf[2], compare_data[2]);
	CU_ASSERT_EQUAL(data_buf[3], compare_data[3]);
	CU_ASSERT_EQUAL(data_buf[4], compare_data[4]);
	CU_ASSERT_EQUAL(data_buf[5], compare_data[5]);
	CU_ASSERT_FALSE(memcmp(data_buf, compare_data, sizeof(data_buf))); /* double check */
}

static void map_to_pos_16_zero_integration_test(void)
{
	uint16_t data_buf[6] = {0, 23, -42, INT16_MAX, INT16_MIN, UINT16_MAX};
	uint16_t compare_data[6];
	uint32_t buf_len = 6;
	struct cmp_cfg cfg;
	struct cmp_info info;
	int error;

	memcpy(compare_data, data_buf, sizeof(compare_data));
	cfg.cmp_mode = MODE_DIFF_ZERO;
	info.cmp_mode_used = MODE_DIFF_ZERO;
	cfg.input_buf = data_buf;
	cfg.samples = buf_len;
	info.samples_used = buf_len;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data_buf, &info);
	CU_ASSERT_FALSE(error);

	CU_ASSERT_EQUAL(data_buf[0], compare_data[0]);
	CU_ASSERT_EQUAL(data_buf[1], compare_data[1]);
	CU_ASSERT_EQUAL(data_buf[2], compare_data[2]);
	CU_ASSERT_EQUAL(data_buf[3], compare_data[3]);
	CU_ASSERT_EQUAL(data_buf[4], compare_data[4]);
	CU_ASSERT_EQUAL(data_buf[5], compare_data[5]);
	CU_ASSERT_FALSE(memcmp(data_buf, compare_data, sizeof(data_buf))); /* double check */
}

static void map_to_pos_32_multi_integration_test(void)
{
	uint32_t data_buf[6] = {0, 23, -42, INT32_MAX, INT32_MIN, UINT32_MAX};
	uint32_t compare_data[6];
	uint32_t buf_len = 6;
	struct cmp_cfg cfg;
	struct cmp_info info;
	int error;

	memcpy(compare_data, data_buf, sizeof(compare_data));
	cfg.cmp_mode = MODE_DIFF_MULTI;
	info.cmp_mode_used = MODE_DIFF_MULTI;
	cfg.input_buf = data_buf;
	cfg.samples = buf_len;
	info.samples_used = buf_len;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data_buf, &info);
	CU_ASSERT_FALSE(error);

	CU_ASSERT_EQUAL(data_buf[0], compare_data[0]);
	CU_ASSERT_EQUAL(data_buf[1], compare_data[1]);
	CU_ASSERT_EQUAL(data_buf[2], compare_data[2]);
	CU_ASSERT_EQUAL(data_buf[3], compare_data[3]);
	CU_ASSERT_EQUAL(data_buf[4], compare_data[4]);
	CU_ASSERT_EQUAL(data_buf[5], compare_data[5]);
	CU_ASSERT_FALSE(memcmp(data_buf, compare_data, sizeof(data_buf))); /* double check */
}

static void map_to_pos_32_zero_integration_test(void)
{
	uint32_t data_buf[6] = {0, 23, -42, INT32_MAX, INT32_MIN, UINT32_MAX};
	uint32_t compare_data[6];
	uint32_t buf_len = 6;
	struct cmp_cfg cfg;
	struct cmp_info info;
	int error;

	memcpy(compare_data, data_buf, sizeof(compare_data));
	cfg.cmp_mode = MODE_DIFF_MULTI_F_FX;
	info.cmp_mode_used = cfg.cmp_mode;
	cfg.input_buf = data_buf;
	cfg.samples = buf_len;
	info.samples_used = buf_len;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data_buf, &info);
	CU_ASSERT_FALSE(error);

	CU_ASSERT_EQUAL(data_buf[0], compare_data[0]);
	CU_ASSERT_EQUAL(data_buf[1], compare_data[1]);
	CU_ASSERT_EQUAL(data_buf[2], compare_data[2]);
	CU_ASSERT_EQUAL(data_buf[3], compare_data[3]);
	CU_ASSERT_EQUAL(data_buf[4], compare_data[4]);
	CU_ASSERT_EQUAL(data_buf[5], compare_data[5]);
	CU_ASSERT_FALSE(memcmp(data_buf, compare_data, sizeof(data_buf))); /* double check */
}

static void map_to_pos_S_FX_integration_test(void)
{
	struct S_FX data[8] = { {0, 42},
				{0, -23},
				{-1, -UINT32_MAX},
				{1, UINT32_MAX},
				{0, -UINT32_MAX},
				{2, UINT32_MAX},
				{-2, 0},
				{0, 0}
	};
	struct S_FX compare_data[8];
	uint32_t samples = 8;
	struct cmp_cfg cfg;
	struct cmp_info info;
	int error;
	uint32_t i;

	memcpy(compare_data, data, sizeof(compare_data));
	cfg.cmp_mode = MODE_MODEL_MULTI_S_FX;
	info.cmp_mode_used = cfg.cmp_mode;
	cfg.input_buf = data;
	cfg.samples = samples;
	info.samples_used = cfg.samples;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data, &info);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, compare_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, compare_data[i].FX);
	}
	CU_ASSERT_FALSE(memcmp(data, compare_data, sizeof(data))); /* double check */

	cfg.cmp_mode = MODE_MODEL_ZERO_S_FX;
	info.cmp_mode_used = cfg.cmp_mode;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data, &info);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, compare_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, compare_data[i].FX);
	}
	CU_ASSERT_FALSE(memcmp(data, compare_data, sizeof(data))); /* double check */
}

static void map_to_pos_S_FX_EFX_integration_test(void)
{
	struct S_FX_EFX data[8] = {
				  {0, 42, 1},
				  {0, 23, 0},
				  {1, UINT32_MAX, UINT32_MAX},
				  {0, UINT32_MAX, 0},
				  {0, UINT32_MAX, UINT32_MAX},
				  {2, UINT32_MAX,0},
				  {2, 0, 42},
				  {0, 0, UINT16_MAX}
	};
	struct S_FX_EFX compare_data[8];
	uint32_t samples = 8;
	struct cmp_cfg cfg;
	struct cmp_info info;
	int error;
	uint32_t i;

	memcpy(compare_data, data, sizeof(compare_data));
	cfg.cmp_mode = MODE_MODEL_MULTI_S_FX_EFX;
	info.cmp_mode_used = cfg.cmp_mode;
	cfg.input_buf = data;
	cfg.samples = samples;
	info.samples_used = cfg.samples;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data, &info);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, compare_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, compare_data[i].FX);
	}
	CU_ASSERT_FALSE(memcmp(data, compare_data, sizeof(data))); /* double check */

	cfg.cmp_mode = MODE_MODEL_ZERO_S_FX_EFX;
	info.cmp_mode_used = cfg.cmp_mode;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data, &info);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, compare_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, compare_data[i].FX);
	}
	CU_ASSERT_FALSE(memcmp(data, compare_data, sizeof(data))); /* double check */
}

static void map_to_pos_S_FX_NCOB_integration_test(void)
{
	struct S_FX_NCOB data[8] = {
				  {0, 42, 1, UINT32_MAX},
				  {0, 23, 0, 0},
				  {1, UINT32_MAX, UINT32_MAX, 1},
				  {0, UINT32_MAX, 0, 2},
				  {0, UINT32_MAX, UINT32_MAX, 0x7FFFFFFF},
				  {2, UINT32_MAX, 0, 0xFFFFFFFE},
				  {2, 0, 42, 0xFFFFFFFD},
				  {0, 0, UINT16_MAX, 0xFFFFFFFA}
	};
	struct S_FX_NCOB compare_data[8];
	uint32_t samples = 8;
	struct cmp_cfg cfg;
	struct cmp_info info;
	int error;
	uint32_t i;

	memcpy(compare_data, data, sizeof(compare_data));
	cfg.cmp_mode = MODE_MODEL_MULTI_S_FX_NCOB;
	info.cmp_mode_used = cfg.cmp_mode;
	cfg.input_buf = data;
	cfg.samples = samples;
	info.samples_used = cfg.samples;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data, &info);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, compare_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, compare_data[i].FX);
		CU_ASSERT_EQUAL(data[i].NCOB_X, compare_data[i].NCOB_X);
		CU_ASSERT_EQUAL(data[i].NCOB_Y, compare_data[i].NCOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(data, compare_data, sizeof(data)));

	cfg.cmp_mode = MODE_MODEL_ZERO_S_FX_NCOB;
	info.cmp_mode_used = cfg.cmp_mode;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data, &info);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, compare_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, compare_data[i].FX);
		CU_ASSERT_EQUAL(data[i].NCOB_X, compare_data[i].NCOB_X);
		CU_ASSERT_EQUAL(data[i].NCOB_Y, compare_data[i].NCOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(data, compare_data, sizeof(data)));
}

static void map_to_pos_S_FX_EFX_NCOB_ECOB_integration_test(void)
{
	struct S_FX_EFX_NCOB_ECOB data[8] = {
				  {0, 42, 1, UINT32_MAX, 0x7E58D953, 0x12F26EE8, 0x6579BF86},
				  {0, 23, 0, 0,0xBBDACBFA, 0x5E2630AC, 0x7286CAEC},
				  {1, UINT32_MAX, UINT32_MAX, 1, 0xE51536C7, 0x7F6B3DA6, 0x5301E79B},
				  {0, UINT32_MAX, 0, 2, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {0, UINT32_MAX, UINT32_MAX, 0x7FFFFFFF, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {2, UINT32_MAX, 0, 0xFFFFFFFE, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {2, 0, 42, 0xFFFFFFFD, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {0, 0, UINT16_MAX, 0xFFFFFFFA, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B}
	};
	struct S_FX_EFX_NCOB_ECOB compare_data[8];
	uint32_t samples = 8;
	struct cmp_cfg cfg;
	struct cmp_info info;
	int error;
	uint32_t i;

	memcpy(compare_data, data, sizeof(compare_data));
	cfg.cmp_mode = MODE_MODEL_MULTI_S_FX_EFX_NCOB_ECOB;
	info.cmp_mode_used = cfg.cmp_mode;
	cfg.input_buf = data;
	cfg.samples = samples;
	info.samples_used = cfg.samples;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data, &info);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, compare_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, compare_data[i].FX);
		CU_ASSERT_EQUAL(data[i].NCOB_X, compare_data[i].NCOB_X);
		CU_ASSERT_EQUAL(data[i].NCOB_Y, compare_data[i].NCOB_Y);
		CU_ASSERT_EQUAL(data[i].EFX, compare_data[i].EFX);
		CU_ASSERT_EQUAL(data[i].ECOB_X, compare_data[i].ECOB_X);
		CU_ASSERT_EQUAL(data[i].ECOB_Y, compare_data[i].ECOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(data, compare_data, sizeof(data)));

	cfg.cmp_mode = MODE_MODEL_ZERO_S_FX_EFX_NCOB_ECOB;
	info.cmp_mode_used = cfg.cmp_mode;

	error = map_to_pos(&cfg);
	CU_ASSERT_FALSE(error);

	error = de_map_to_pos(data, &info);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < samples; i++) {
		CU_ASSERT_EQUAL(data[i].EXPOSURE_FLAGS, compare_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(data[i].FX, compare_data[i].FX);
		CU_ASSERT_EQUAL(data[i].NCOB_X, compare_data[i].NCOB_X);
		CU_ASSERT_EQUAL(data[i].NCOB_Y, compare_data[i].NCOB_Y);
		CU_ASSERT_EQUAL(data[i].EFX, compare_data[i].EFX);
		CU_ASSERT_EQUAL(data[i].ECOB_X, compare_data[i].ECOB_X);
		CU_ASSERT_EQUAL(data[i].ECOB_Y, compare_data[i].ECOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(data, compare_data, sizeof(data)));
}

static void encode_data_16_integration_test(void)
{
	int error;
	size_t i;
	struct cmp_cfg cfg = {0};
	struct cmp_info info = {0};
	uint16_t input_data[11] = {0, 1, 2, 3, 4, 42, 23, 1000,  UINT16_MAX,
		UINT16_MAX-1, UINT16_MAX-3};
	uint16_t decompressed_data[11] = {0};
	uint16_t compressed_data[50] = {0};

	cfg.input_buf = input_data;
	cfg.samples = 11;
	cfg.cmp_mode = MODE_MODEL_ZERO;
	cfg.golomb_par = 8;
	cfg.spill = 23;
	cfg.icu_output_buf = compressed_data;
	cfg.buffer_length = 50;

	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i], decompressed_data[i]);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */

	cfg.cmp_mode = MODE_DIFF_MULTI;
	cfg.golomb_par = 7;
	cfg.spill = 42;
	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i], decompressed_data[i]);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */
}

static void encode_data_S_FX_integration_test(void)
{
	int error;
	size_t i;
	struct cmp_cfg cfg = {0};
	struct cmp_info info = {0};
	struct S_FX input_data[8] = {	{0, 0},
					{0, 1},
					{1, UINT32_MAX},
					{0, UINT32_MAX-1},
					{0, UINT32_MAX-2},
					{2, UINT32_MAX-3},
					{2, 42},
					{0, 23}
	};
	struct S_FX decompressed_data[8] = {0};
	uint16_t compressed_data[50] = {0};

	cfg.input_buf = input_data;
	cfg.samples = 8;
	cfg.cmp_mode = MODE_MODEL_ZERO_S_FX;
	cfg.golomb_par = 10;
	cfg.spill = 23;
	cfg.icu_output_buf = compressed_data;
	cfg.buffer_length = 50;

	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(input_data[i].FX, decompressed_data[i].FX);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */

	cfg.cmp_mode = MODE_DIFF_MULTI_S_FX;
	cfg.golomb_par = 17;
	cfg.spill = 42;
	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(input_data[i].FX, decompressed_data[i].FX);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */
}

static void encode_data_S_FX_EFX_integration_test(void)
{
	int error;
	size_t i;
	struct cmp_cfg cfg = {0};
	struct cmp_info info = {0};
	struct S_FX_EFX input_data[8] = {
					{0, 42, 1},
					{0, 23, 0},
					{1, UINT32_MAX, UINT32_MAX},
					{0, UINT32_MAX-1, 0},
					{0, UINT32_MAX-3, UINT32_MAX-0xFF},
					{2, UINT32_MAX-5,0},
					{2, 0, 42},
					{0, 0, UINT16_MAX}
	};
	struct  S_FX_EFX decompressed_data[8] = {0};
	uint16_t compressed_data[100] = {0};

	cfg.input_buf = input_data;
	cfg.samples = 8;
	cfg.cmp_mode = MODE_DIFF_ZERO_S_FX_EFX;
	cfg.golomb_par = 11;
	cfg.spill = 50;
	cfg.icu_output_buf = compressed_data;
	cfg.buffer_length = 100;

	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(input_data[i].FX, decompressed_data[i].FX);
		CU_ASSERT_EQUAL(input_data[i].EFX, decompressed_data[i].EFX);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */

	cfg.cmp_mode = MODE_MODEL_MULTI_S_FX_EFX;
	cfg.golomb_par = 20;
	cfg.spill = 100;
	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(input_data[i].FX, decompressed_data[i].FX);
		CU_ASSERT_EQUAL(input_data[i].EFX, decompressed_data[i].EFX);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */
}

static void encode_data_S_FX_NCOB_integration_test(void)
{
	int error;
	size_t i;
	struct cmp_cfg cfg = {0};
	struct cmp_info info = {0};
	struct S_FX_NCOB input_data[8] = {
				  {4, 42, 1, UINT32_MAX},
				  {0, 23, 0, 0},
				  {1, UINT32_MAX, UINT32_MAX, 1},
				  {0, UINT32_MAX, 0, 2},
				  {4, UINT32_MAX, UINT32_MAX, 0x7FFFFFFF},
				  {2, UINT32_MAX, 0, 0xFFFFFFFE},
				  {2, 0, 42, 0xFFFFFFFD},
				  {0, 0, UINT16_MAX, 0xFFFFFFFA}
	};
	struct S_FX_NCOB decompressed_data[8] = {0};
	uint16_t compressed_data[100] = {0};

	cfg.input_buf = input_data;
	cfg.samples = 8;
	cfg.cmp_mode = MODE_MODEL_ZERO_S_FX_NCOB;
	cfg.golomb_par = 10;
	cfg.spill = 23;
	cfg.icu_output_buf = compressed_data;
	cfg.buffer_length = 100;

	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(input_data[i].FX, decompressed_data[i].FX);
		CU_ASSERT_EQUAL(input_data[i].NCOB_X, decompressed_data[i].NCOB_X);
		CU_ASSERT_EQUAL(input_data[i].NCOB_Y, decompressed_data[i].NCOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */

	cfg.cmp_mode = MODE_DIFF_MULTI_S_FX_NCOB;
	cfg.golomb_par = 17;
	cfg.spill = 42;
	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(input_data[i].FX, decompressed_data[i].FX);
		CU_ASSERT_EQUAL(input_data[i].NCOB_X, decompressed_data[i].NCOB_X);
		CU_ASSERT_EQUAL(input_data[i].NCOB_Y, decompressed_data[i].NCOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */
}

static void encode_data_S_FX_EFX_NCOB_ECOB_integration_test(void)
{
	int error;
	size_t i;
	struct cmp_cfg cfg = {0};
	struct cmp_info info = {0};
	struct S_FX_EFX_NCOB_ECOB input_data[8] = {
				  {0, 42, 1, UINT32_MAX, 0x7E58D953, 0x12F26EE8, 0x6579BF86},
				  {0, 23, 0, 0,0xBBDACBFA, 0x5E2630AC, 0x7286CAEC},
				  {1, UINT32_MAX, UINT32_MAX, 1, 0xE51536C7, 0x7F6B3DA6, 0x5301E79B},
				  {0, UINT32_MAX, 0, 2, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {0, UINT32_MAX, UINT32_MAX, 0x7FFFFFFF, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {3, UINT32_MAX, 0, 0xFFFFFFFE, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {2, 0, 42, 0xFFFFFFFD, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {4, 0, UINT16_MAX, 0xFFFFFFFA, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B}
	};
	struct S_FX_EFX_NCOB_ECOB decompressed_data[8] = {0};
	uint16_t compressed_data[200] = {0};

	cfg.input_buf = input_data;
	cfg.samples = 8;
	cfg.cmp_mode = MODE_MODEL_ZERO_S_FX_EFX_NCOB_ECOB;
	cfg.golomb_par = 12;
	cfg.spill = 23;
	cfg.icu_output_buf = compressed_data;
	cfg.buffer_length = 200;

	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(input_data[i].FX, decompressed_data[i].FX);
		CU_ASSERT_EQUAL(input_data[i].NCOB_X, decompressed_data[i].NCOB_X);
		CU_ASSERT_EQUAL(input_data[i].NCOB_Y, decompressed_data[i].NCOB_Y);
		CU_ASSERT_EQUAL(input_data[i].EFX, decompressed_data[i].EFX);
		CU_ASSERT_EQUAL(input_data[i].ECOB_X, decompressed_data[i].ECOB_X);
		CU_ASSERT_EQUAL(input_data[i].ECOB_Y, decompressed_data[i].ECOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */

	cfg.cmp_mode = MODE_DIFF_MULTI_S_FX_NCOB;
	cfg.golomb_par = 17;
	cfg.spill = 42;
	set_info(&cfg, &info);
	error = encode_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decode_data(cfg.icu_output_buf, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(input_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(input_data[i].FX, decompressed_data[i].FX);
		CU_ASSERT_EQUAL(input_data[i].NCOB_X, decompressed_data[i].NCOB_X);
		CU_ASSERT_EQUAL(input_data[i].NCOB_Y, decompressed_data[i].NCOB_Y);
		CU_ASSERT_EQUAL(input_data[i].EFX, decompressed_data[i].EFX);
		CU_ASSERT_EQUAL(input_data[i].ECOB_X, decompressed_data[i].ECOB_X);
		CU_ASSERT_EQUAL(input_data[i].ECOB_Y, decompressed_data[i].ECOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(input_data, decompressed_data, sizeof(input_data))); /* double check */
}

static void compression_decompression_16_test(void)
{
	int error;
	size_t i;
	struct cmp_cfg cfg = {0};
	struct cmp_info info = {0};
	uint16_t input_data[11] = {0, 1, 2, 3, 4, 42, 23, 1000,  UINT16_MAX,
		UINT16_MAX-1, UINT16_MAX-3};
	uint16_t compare_data[11];
	uint16_t decompressed_data[11] = {0};
	uint16_t compressed_data[50] = {0};

	memcpy(compare_data, input_data, sizeof(compare_data));

	cfg.input_buf = input_data;
	cfg.samples = 11;
	cfg.cmp_mode = MODE_DIFF_MULTI;
	cfg.golomb_par = 10;
	cfg.spill = 8;
	cfg.round = 0;
	cfg.icu_output_buf = compressed_data;
	cfg.buffer_length = 50;

	error = icu_compress_data(&cfg, &info);
	CU_ASSERT_FALSE(error);
	CU_ASSERT_FALSE(info.cmp_err);

	error = decompress_data(cfg.icu_output_buf, NULL, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(compare_data[i], decompressed_data[i]);
	}
	CU_ASSERT_FALSE(memcmp(compare_data, decompressed_data, sizeof(compare_data))); /* double check */

	/* zero test */
	cfg.cmp_mode = MODE_DIFF_ZERO;
	memcpy(input_data, compare_data, sizeof(input_data));

	error = icu_compress_data(&cfg, &info);
	CU_ASSERT_FALSE(error);
	CU_ASSERT_FALSE(info.cmp_err);

	error = decompress_data(cfg.icu_output_buf, NULL, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(compare_data[i], decompressed_data[i]);
	}
	CU_ASSERT_FALSE(memcmp(compare_data, decompressed_data, sizeof(compare_data))); /* double check */
}

static void compression_decompression_test(void)
{
	int error;
	size_t i;
	struct cmp_cfg cfg = {0};
	struct cmp_info info = {0};
	struct S_FX_EFX_NCOB_ECOB input_data[8] = {
				  {0, 42, 1, UINT32_MAX, 0x7E58D953, 0x12F26EE8, 0x6579BF86},
				  {0, 23, 0, 0,0xBBDACBFA, 0x5E2630AC, 0x7286CAEC},
				  {1, UINT32_MAX, UINT32_MAX, 1, 0xE51536C7, 0x7F6B3DA6, 0x5301E79B},
				  {0, UINT32_MAX, 0, 2, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {0, UINT32_MAX, UINT32_MAX, 0x7FFFFFFF, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {3, UINT32_MAX, 0, 0xFFFFFFFE, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {2, 0, 42, 0xFFFFFFFD, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B},
				  {4, 0, UINT16_MAX, 0xFFFFFFFA, 0xE2875795, 0xF79DEF9E, 0x4AC36F6B}
	};
	struct S_FX_EFX_NCOB_ECOB compare_data[8];
	struct S_FX_EFX_NCOB_ECOB decompressed_data[8] = {0};
	uint16_t compressed_data[200] = {0};

	memcpy(compare_data, input_data, sizeof(compare_data));

	cfg.input_buf = input_data;
	cfg.samples = 8;
	cfg.cmp_mode = MODE_DIFF_ZERO_S_FX_EFX_NCOB_ECOB;
	cfg.golomb_par = 12;
	cfg.spill = 23;
	cfg.icu_output_buf = compressed_data;
	cfg.buffer_length = 200;

	error = icu_compress_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decompress_data(cfg.icu_output_buf, NULL, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(compare_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(compare_data[i].FX, decompressed_data[i].FX);
		CU_ASSERT_EQUAL(compare_data[i].NCOB_X, decompressed_data[i].NCOB_X);
		CU_ASSERT_EQUAL(compare_data[i].NCOB_Y, decompressed_data[i].NCOB_Y);
		CU_ASSERT_EQUAL(compare_data[i].EFX, decompressed_data[i].EFX);
		CU_ASSERT_EQUAL(compare_data[i].ECOB_X, decompressed_data[i].ECOB_X);
		CU_ASSERT_EQUAL(compare_data[i].ECOB_Y, decompressed_data[i].ECOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(compare_data, decompressed_data, sizeof(compare_data))); /* double check */


	/* zero test */
	cfg.cmp_mode = MODE_DIFF_ZERO;
	memcpy(input_data, compare_data, sizeof(input_data));

	error = icu_compress_data(&cfg, &info);
	CU_ASSERT_FALSE(error);

	error = decompress_data(cfg.icu_output_buf, NULL, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(compare_data[i].EXPOSURE_FLAGS, decompressed_data[i].EXPOSURE_FLAGS);
		CU_ASSERT_EQUAL(compare_data[i].FX, decompressed_data[i].FX);
		CU_ASSERT_EQUAL(compare_data[i].NCOB_X, decompressed_data[i].NCOB_X);
		CU_ASSERT_EQUAL(compare_data[i].NCOB_Y, decompressed_data[i].NCOB_Y);
		CU_ASSERT_EQUAL(compare_data[i].EFX, decompressed_data[i].EFX);
		CU_ASSERT_EQUAL(compare_data[i].ECOB_X, decompressed_data[i].ECOB_X);
		CU_ASSERT_EQUAL(compare_data[i].ECOB_Y, decompressed_data[i].ECOB_Y);
	}
	CU_ASSERT_FALSE(memcmp(compare_data, decompressed_data, sizeof(compare_data))); /* double check */
}

#include <stdlib.h>
#include <time.h>

static uint32_t randBetween(double min, double max)
{
	return (uint32_t)(((double)rand()/RAND_MAX) * (max - min) + min);
}

static void compression_decompression_random_test(void)
{
	struct cmp_cfg cfg = {0};
	struct cmp_info info = {0};
	uint16_t *data_pt=NULL;
	uint16_t *compare_data=NULL;
	uint16_t *decmp_model=NULL;
	uint16_t *decompressed_data =NULL;
	size_t i, sample_in_bytes;
	int error;
	time_t seed = time(NULL);
	printf("\nseed: %lX\n", seed);
	srand(seed); /* use current time as seed for random generator */

	cfg.cmp_mode = randBetween(0, 4);
	cfg.samples = randBetween(1, 2*UINT16_MAX);
	sample_in_bytes = cfg.samples * sizeof(*data_pt);
	CU_ASSERT_EQUAL_FATAL(sizeof(*data_pt), 2);

	cfg.input_buf = malloc(sample_in_bytes);
	CU_ASSERT_PTR_NOT_NULL_FATAL(cfg.input_buf);

	compare_data = malloc(sample_in_bytes);
	CU_ASSERT_PTR_NOT_NULL_FATAL(compare_data);

	for (i = 0; i < cfg.samples; i++) {
		data_pt = cfg.input_buf;
		data_pt[i] = randBetween(0, UINT16_MAX);
	}
	memcpy(compare_data, cfg.input_buf, sample_in_bytes);
	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(compare_data[i], data_pt[i]);
		/* printf("\n\n%X\n\n", data_pt[i]); */
	}

	if (model_mode_is_used(cfg.cmp_mode)) {
		cfg.model_buf = malloc(sample_in_bytes);
		CU_ASSERT_PTR_NOT_NULL_FATAL(cfg.model_buf);
		decmp_model = malloc(sample_in_bytes);
		CU_ASSERT_PTR_NOT_NULL_FATAL(decmp_model);
		for (i = 0; i < cfg.samples; i++) {
			data_pt = cfg.model_buf;
			data_pt[i] = randBetween(0, UINT16_MAX);
		}
		memcpy(decmp_model, cfg.model_buf, sample_in_bytes);
	} else {
		cfg.model_buf = NULL;
		decmp_model = NULL;
	}

	cfg.golomb_par = randBetween(1, 64);
	cfg.spill = randBetween(1, cmp_icu_max_spill(cfg.golomb_par));
	cfg.round = 0; /* randBetween(0, 8); */
	cfg.model_value = randBetween(0, 16);

	cfg.icu_output_buf = malloc(4 * sample_in_bytes);
	CU_ASSERT_PTR_NOT_NULL_FATAL(cfg.icu_output_buf);
	cfg.buffer_length = 4 * cfg.samples;

	decompressed_data = cfg.input_buf;

	error = icu_compress_data(&cfg, &info);
	CU_ASSERT_FALSE(error);
	CU_ASSERT_FALSE(info.cmp_err);
	CU_ASSERT_FALSE(info.cmp_err&SMALL_BUFFER_ERR_BIT);

	memset(decompressed_data, 0, sample_in_bytes);

	error = decompress_data(cfg.icu_output_buf, decmp_model, &info, decompressed_data);
	CU_ASSERT_FALSE(error);

	for (i = 0; i < cfg.samples; i++) {
		CU_ASSERT_EQUAL(compare_data[i], decompressed_data[i]);
		/* printf("%X == %X\n\n", compare_data[i], decompressed_data[i]); */
	}
	CU_ASSERT_FALSE(memcmp(compare_data, decompressed_data, sample_in_bytes)); /* double check */

	if (model_mode_is_used(cfg.cmp_mode)) {
		for (i = 0; i < cfg.samples; i++) {
			data_pt = cfg.model_buf;
			CU_ASSERT_EQUAL(data_pt[i], decmp_model[i]);
		}
		CU_ASSERT_FALSE(memcmp(cfg.model_buf, decmp_model, sample_in_bytes)); /* double check */
	}

	free(cfg.input_buf);
	free(compare_data);
	free(cfg.model_buf);
	free(decmp_model);
}


#if 0
void Rice_encoder_integration_test(test)
{
	unsigned int m, log2_m;
	uint32_t data[5] = {0, 1, 3, UINT16_MAX, UINT32_MAX};
	int i, j;
	for (j=0; i < 64; j++) {
		for (i=0; i<5; i++) {
			log2_m = j;
			m = 1 << log2_m;
			len = Rice_encoder(m, log2_m, value, &code_word);

		}
	}
	len = Rice_encoder(m, log2_m, value, &code_word);
	len = Rice_decoder(code_word, m, log2_m, &decoded_cw);
}
#endif

CU_ErrorCode run_integration_suite(void)
{
	CU_pSuite int_suite = NULL;

	int_suite = CU_add_suite( "Compression Integration Test Suite", NULL, NULL );
	if ( NULL == int_suite ) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if ( (NULL == CU_add_test(int_suite, "lossy rounding 16 integration test", lossy_rounding_16_integration_test)) ||

	     (NULL == CU_add_test(int_suite, "diff 16 pre-processing integration test", diff_16_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "diff 16 pre-processing round integration test", diff_16_round_integration_test)) ||

	     (NULL == CU_add_test(int_suite, "diff 32 pre-processing integration test", diff_32_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "diff S_FX pre-processing integration test", diff_S_FX_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "diff S_FX_EFX pre-processing integration test", diff_S_FX_EFX_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "diff S_FX_NCOB pre-processing integration test", diff_S_FX_NCOB_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "diff S_FX_EFX_NCOB_ECOB pre-processing integration test", diff_S_FX_EFX_NCOB_ECOB_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "model pre-processing integration test", model_16_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "model pre-processing round integration test", model_16_integration_round_test)) ||
	     /* (NULL == CU_add_test(int_suite, "model pre-processing round integration all test", model_pre_process_integration_round_all)) || */

	     (NULL == CU_add_test(int_suite, "map_to_pos_16 multi integration test", map_to_pos_16_multi_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "map_to_pos_16 zero integration test", map_to_pos_16_zero_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "map_to_pos_32 multi integration test", map_to_pos_32_multi_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "map_to_pos_32 zero integration test", map_to_pos_32_zero_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "map_to_pos_S_FX integration test", map_to_pos_S_FX_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "map_to_pos_S_FX_EFX integration test", map_to_pos_S_FX_EFX_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "map_to_pos_S_FX_NCOB integration test", map_to_pos_S_FX_NCOB_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "map_to_pos_S_FX_EFX_NCOB_ECOB integration test", map_to_pos_S_FX_EFX_NCOB_ECOB_integration_test)) ||

	     (NULL == CU_add_test(int_suite, "encode_data_16 integration test", encode_data_16_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "encode_data_S_FX integration test", encode_data_S_FX_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "encode_data_S_FX_EFX integration test", encode_data_S_FX_EFX_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "encode_data_S_FX_NCOB integration test", encode_data_S_FX_NCOB_integration_test)) ||
	     (NULL == CU_add_test(int_suite, "encode_data_S_FX_EFX_NCOB_ECOB integration test", encode_data_S_FX_EFX_NCOB_ECOB_integration_test)) ||

	     (NULL == CU_add_test(int_suite, "compression decompression 16 test", compression_decompression_16_test)) ||
	     (NULL == CU_add_test(int_suite, "compression decompression test", compression_decompression_test)) ||
	     (NULL == CU_add_test(int_suite, "compression decompression random test", compression_decompression_random_test))
	   )
	{
		CU_cleanup_registry();
		return CU_get_error();
	}
	return CUE_SUCCESS;
}

int main ( void )
{
	/* initialize the CUnit test registry */
	if ( CUE_SUCCESS != CU_initialize_registry() )
		return CU_get_error();

	/* add integration test suite to the registry */
	if (CUE_SUCCESS != run_integration_suite()) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	printf("\n");
	CU_basic_show_failures(CU_get_failure_list());
	printf("\n\n");
	/*
	// Run all tests using the automated interface
	CU_automated_run_tests();
	CU_list_tests_to_file();
	// Run all tests using the console interface
	CU_console_run_tests();
*/
	/* Clean up registry and return */
	CU_cleanup_registry();
	return CU_get_error();
}
