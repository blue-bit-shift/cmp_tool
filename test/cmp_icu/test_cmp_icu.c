#include <string.h>
#include <stdlib.h>

#include "unity.h"

#include "cmp_support.h"
/* this is a hack to test static functions */
#include "../lib/cmp_icu.c"


/**
 * @test map_to_pos
 */

void test_map_to_pos(void)
{
	uint32_t value_to_map;
	uint32_t max_data_bits;
	uint32_t mapped_value;

	/* test mapping 32 bits values */
	max_data_bits = 32;

	value_to_map = 0;
	mapped_value = map_to_pos(value_to_map, max_data_bits);
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


/**
 * @test put_n_bits32
 */

#define SDP_PB_N 3


static void init_PB32_arrays(uint32_t *z, uint32_t *o)
{
	uint32_t i;

	/* init testarray with all 0 and all 1 */
	for (i = 0; i < SDP_PB_N; i++) {
		z[i] = 0;
		o[i] = 0xffffffff;
	}
}


void test_put_n_bits32(void)
{
	uint32_t v, n;
	int o, rval; /* return value */
	uint32_t testarray0[SDP_PB_N];
	uint32_t testarray1[SDP_PB_N];
	const uint32_t l = sizeof(testarray0) * CHAR_BIT;

	/* hereafter, the value is v,
	 * the number of bits to write is n,
	 * the offset of the bit is o,
	 * the max length the bitstream in bits is l
	 */

	init_PB32_arrays(testarray0, testarray1);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);
	TEST_ASSERT(testarray0[2] == 0);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xffffffff);
	TEST_ASSERT(testarray1[2] == 0xffffffff);

	/*** n=0 ***/

	/* do not write, left border */
	v = 0; n = 0; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(0, rval);

	v = 0xffffffff; n = 0; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(0, rval);

	/* do not write, right border */
	v = 0; n = 0; o = l;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(l, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(l, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(l, rval);

	/* test value = 0xffffffff; N = 0 */
	v = 0xffffffff; n = 0; o = l;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(l, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(l, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(l, rval);

	/*** n=1 ***/

	/* left border, write 0 */
	v = 0; n = 1; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(1, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(1, rval);
	TEST_ASSERT(testarray1[0] == 0x7fffffff);

	/* left border, write 1 */
	v = 1; n = 1; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(1, rval);
	TEST_ASSERT(testarray0[0] == 0x80000000);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(1, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	/* left border, write 32 */
	v = 0xf0f0abcd; n = 32; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 32);
	TEST_ASSERT(testarray0[0] == 0xf0f0abcd);
	TEST_ASSERT(testarray0[1] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 32);
	TEST_ASSERT(testarray1[0] == 0xf0f0abcd);
	TEST_ASSERT(testarray1[1] == 0xffffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* middle, write 2 bits */
	v = 3; n = 2; o = 29;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 31);
	TEST_ASSERT(testarray0[0] == 0x6);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT_EQUAL_INT(rval, 31);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/*** n=5, unsegmented ***/

	/* left border, write 0 */
	v = 0; n = 5; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 5);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT(testarray1[0] == 0x07ffffff);
	TEST_ASSERT_EQUAL_INT(rval, 5);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* left border, write 11111 */
	v = 0x1f; n = 5; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 5);
	TEST_ASSERT(testarray0[0] == 0xf8000000);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 5);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* middle, write 0 */
	v = 0; n = 5; o = 7;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 12);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 12);
	TEST_ASSERT(testarray1[0] == 0xfe0fffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* middle, write 11111 */
	v = 0x1f; n = 5; o = 7;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 12);
	TEST_ASSERT(testarray0[0] == 0x01f00000);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 12);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* right, write 0 */
	v = 0; n = 5; o = 91;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 96);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 96);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xffffffff);
	TEST_ASSERT(testarray1[2] == 0xffffffe0);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* right, write 11111 */
	v = 0x1f; n = 5; o = 91;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 96);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);
	TEST_ASSERT(testarray0[2] == 0x0000001f);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 96);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xffffffff);
	TEST_ASSERT(testarray1[2] == 0xffffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* 32 bit, write 0 */
	v = 0; n = 32; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 32);
	TEST_ASSERT(testarray0[0] == 0x00000000);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 32);
	TEST_ASSERT(testarray1[0] == 0x00000000);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* 32 bit, write -1 */
	v = 0xffffffff; n = 32; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 32);
	TEST_ASSERT(testarray0[0] == 0xffffffff);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 32);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* SEGMENTED cases */
	/* 5 bit, write 0 */
	v = 0; n = 5; o = 62;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 67);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);
	TEST_ASSERT(testarray0[2] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 67);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xfffffffc);
	TEST_ASSERT(testarray1[2] == 0x1fffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* 5 bit, write 1f */
	v = 0x1f; n = 5; o = 62;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 67);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 3);
	TEST_ASSERT(testarray0[2] == 0xe0000000);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 67);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xffffffff);
	TEST_ASSERT(testarray1[2] == 0xffffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* 32 bit, write 0 */
	v = 0; n = 32; o = 1;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 33);
	TEST_ASSERT(testarray0[0] == 0x00000000);
	TEST_ASSERT(testarray0[1] == 0x00000000);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 33);
	TEST_ASSERT(testarray1[0] == 0x80000000);
	TEST_ASSERT(testarray1[1] == 0x7fffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* 32 bit, write -1 */
	v = 0xffffffff; n = 32; o = 1;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, 33);
	TEST_ASSERT(testarray0[0] == 0x7fffffff);
	TEST_ASSERT(testarray0[1] == 0x80000000);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 33);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xffffffff);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/* test NULL buffer */
	v = 0; n = 0; o = 0;
	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(rval, 0);

	v = 0; n = 1; o = 0;
	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(rval, 1);

	v = 0; n = 5; o = 31;
	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(rval, 36);

	v = 0; n = 2; o = 95;
	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(rval, 97); /* rval can be longer than l */

	/* value larger than n allows */
	v = 0x7f; n = 6; o = 10;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(16, rval);
	TEST_ASSERT(testarray0[0] == 0x003f0000);
	TEST_ASSERT(testarray0[1] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(16, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xffffffff);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(16, rval);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	v = 0xffffffff; n = 6; o = 10;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(16, rval);
	TEST_ASSERT(testarray0[0] == 0x003f0000);
	TEST_ASSERT(testarray0[1] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(16, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xffffffff);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(16, rval);
	/* re-init input arrays after clobbering */
	init_PB32_arrays(testarray0, testarray1);

	/*** error cases ***/
	/* n too large */
	v = 0x0; n = 33; o = 1;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, -1);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(rval, -1);

	/* try to put too much in the bitstream */
	v = 0x1; n = 1; o = 96;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, rval);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);
	TEST_ASSERT(testarray0[2] == 0);

	/* this should work (if bitstream=NULL no length check) */
	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(97, rval);

	/* offset lager than max_stream_len(l) */
	v = 0x0; n = 32; o = INT32_MAX;
	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);
	TEST_ASSERT(testarray1[1] == 0xffffffff);
	TEST_ASSERT(testarray1[2] == 0xffffffff);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT(rval < 0);

	/* negative offset */
	v = 0x0; n = 0; o = -1;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(-1, rval);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(-1, rval);

	v = 0x0; n = 0; o = -2;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(-1, rval);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(-1, rval);
}


/**
 * @test rice_encoder
 */

void test_rice_encoder(void)
{
	uint32_t value, g_par, log2_g_par, cw, cw_len;

	/* test minimum Golomb parameter */
	value = 0; log2_g_par = (uint32_t)ilog_2(MIN_ICU_GOLOMB_PAR); g_par = 1U << log2_g_par; cw = ~0U;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(1, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 31;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFE, cw);

	/* test some arbitrary values */
	value = 0; log2_g_par = 4; g_par = 1U << log2_g_par; cw = ~0U;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(5, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(5, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1, cw);

	value = 42;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(7, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x6a, cw);

	value = 446;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFEE, cw);

	value = 447;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFEF, cw);

	/* test maximum Golomb parameter for rice_encoder */
	value = 0; log2_g_par = (uint32_t)ilog_2(MAX_ICU_GOLOMB_PAR); g_par = 1U << log2_g_par; cw = ~0U;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1, cw);

	value = 0x7FFFFFFE;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFE, cw);

	value = 0x7FFFFFFF;
	cw_len = rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFF, cw);
}


/**
 * @test golomb_encoder
 */

void test_Golomb_encoder(void)
{
	uint32_t value, g_par, log2_g_par, cw, cw_len;

	/* test minimum Golomb parameter */
	value = 0; g_par = MIN_ICU_GOLOMB_PAR; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(1, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 31;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFE, cw);


	/* test some arbitrary values with g_par = 16 */
	value = 0; g_par = 16; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(5, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(5, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1, cw);

	value = 42;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(7, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x6a, cw);

	value = 446;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFEE, cw);

	value = 447;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFEF, cw);


	/* test some arbitrary values with g_par = 3 */
	value = 0; g_par = 3; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(2, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(3, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x2, cw);

	value = 42;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(16, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFC, cw);

	value = 44;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(17, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1FFFB, cw);

	value = 88;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFA, cw);

	value = 89;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFB, cw);


	/* test maximum Golomb parameter for golomb_encoder */
	value = 0; g_par = MAX_ICU_GOLOMB_PAR; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1; g_par = MAX_ICU_GOLOMB_PAR; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1, cw);

	value = 0x7FFFFFFE;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFE, cw);

	value = 0x7FFFFFFF;
	cw_len = golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFF, cw);
}


/**
 * @test encode_value_zero
 */

void test_encode_value_zero(void)
{
	uint32_t data, model;
	int stream_len;
	struct encoder_setupt setup = {0};
	uint32_t bitstream[3] = {0};

	/* setup the setup */
	setup.encoder_par1 = 1;
	setup.encoder_par2 = (uint32_t)ilog_2(setup.encoder_par1);
	setup.spillover_par = 32;
	setup.max_data_bits = 32;
	setup.generate_cw_f = rice_encoder;
	setup.bitstream_adr = bitstream;
	setup.max_stream_len = sizeof(bitstream) * CHAR_BIT;

	stream_len = 0;

	data = 0; model = 0;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(2, stream_len);
	TEST_ASSERT_EQUAL_HEX(0x80000000, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);

	data = 5; model = 0;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(14, stream_len);
	TEST_ASSERT_EQUAL_HEX(0xBFF80000, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);

	data = 2; model = 7;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(25, stream_len);
	TEST_ASSERT_EQUAL_HEX(0xBFFBFF00, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);

	/* zero escape mechanism */
	data = 100; model = 42;
	/* (100-42)*2+1=117 -> cw 0 + 0x0000_0000_0000_0075 */
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(58, stream_len);
	TEST_ASSERT_EQUAL_HEX(0xBFFBFF00, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00001D40, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);

	/* test overflow */
	data = INT32_MIN; model = 0;
	/* (INT32_MIN)*-2-1+1=0(overflow) -> cw 0 + 0x0000_0000_0000_0000 */
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(91, stream_len);
	TEST_ASSERT_EQUAL_HEX(0xBFFBFF00, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00001D40, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);

	/* small buffer error */
	data = 23; model = 26;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, stream_len);

	/* reset bitstream to all bits set */
	bitstream[0] = ~0U;
	bitstream[1] = ~0U;
	bitstream[2] = ~0U;
	stream_len = 0;

	/* we use now values with maximum 6 bits */
	setup.max_data_bits = 6;

	/* lowest value before zero encoding */
	data = 53; model = 38;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(32, stream_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFE, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[2]);

	/* lowest value with zero encoding */
	data = 0; model = 16;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(39, stream_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFE, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x41FFFFFF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[2]);

	/* maximum positive value to encode */
	data = 31; model = 0;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(46, stream_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFE, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x40FFFFFF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[2]);

	/* maximum negative value to encode */
	data = 0; model = 32;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(53, stream_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFE, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x40FC07FF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[2]);

	/* small buffer error when creating the zero escape symbol*/
	bitstream[0] = 0;
	bitstream[1] = 0;
	bitstream[2] = 0;
	stream_len = 32;
	setup.max_stream_len = 32;
	data = 31; model = 0;
	stream_len = encode_value_zero(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, stream_len);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[2]);
}


/**
 * @test encode_value_multi
 */

void test_encode_value_multi(void)
{
	uint32_t data, model;
	int stream_len;
	struct encoder_setupt setup = {0};
	uint32_t bitstream[4] = {0};

	/* setup the setup */
	setup.encoder_par1 = 1;
	setup.encoder_par2 = (uint32_t)ilog_2(setup.encoder_par1);
	setup.spillover_par = 16;
	setup.max_data_bits = 32;
	setup.generate_cw_f = golomb_encoder;
	setup.bitstream_adr = bitstream;
	setup.max_stream_len = sizeof(bitstream) * CHAR_BIT;

	stream_len = 0;

	data = 0; model = 0;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(1, stream_len);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[3]);

	data = 0; model = 1;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(3, stream_len);
	TEST_ASSERT_EQUAL_HEX(0x40000000, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[3]);

	data = 1+23; model = 0+23;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(6, stream_len);
	TEST_ASSERT_EQUAL_HEX(0x58000000, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[3]);

	/* highest value without multi outlier encoding */
	data = 0+42; model = 8+42;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(22, stream_len);
	TEST_ASSERT_EQUAL_HEX(0x5BFFF800, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[3]);

	/* lowest value with multi outlier encoding */
	data = 8+42; model = 0+42;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(41, stream_len);
	TEST_ASSERT_EQUAL_HEX(0x5BFFFBFF, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0xFC000000, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[3]);

	/* highest value with multi outlier encoding */
	data = INT32_MIN; model = 0;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(105, stream_len);
	TEST_ASSERT_EQUAL_HEX(0x5BFFFBFF, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0xFC7FFFFF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0xFF7FFFFF, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0xF7800000, bitstream[3]);

	/* small buffer error */
	data = 0; model = 38;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, stream_len);

	/* small buffer error when creating the multi escape symbol*/
	bitstream[0] = 0;
	bitstream[1] = 0;
	setup.max_stream_len = 32;

	stream_len = 32;
	data = 31; model = 0;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, stream_len);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[1]);
}


/**
 * @test encode_value
 */

void test_encode_value(void)
{
	struct encoder_setupt setup = {0};
	uint32_t bitstream[4] = {0};
	uint32_t data, model;
	int cmp_size;

	setup.encode_method_f = encode_value_none;
	setup.bitstream_adr = bitstream;
	setup.max_stream_len = 128;
	cmp_size = 0;

	/* test 32 bit input */
	setup.encoder_par1 = 32;
	setup.max_data_bits = 32;
	setup.lossy_par = 0;

	data = 0; model = 0;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(32, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[3]);

	data = UINT32_MAX; model = 0;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(64, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[3]);

	/* test rounding */
	setup.lossy_par = 1;
	data = UINT32_MAX; model = 0;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(96, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFF, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[3]);

	setup.lossy_par = 2;
	data = 0x3; model = 0;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(128, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFF, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0x00000000, bitstream[3]);

	/* small buffer error bitstream can not hold more data*/
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, cmp_size);

	/* reset bitstream */
	bitstream[0] = 0;
	bitstream[1] = 0;
	bitstream[2] = 0;
	bitstream[3] = 0;
	cmp_size = 0;

	/* test 31 bit input */
	setup.encoder_par1 = 31;
	setup.max_data_bits = 31;
	setup.lossy_par = 0;

	data = 0; model = 0;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(31, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[3]);

	data = 0x7FFFFFFF; model = 0;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(62, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0x00000001, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFC, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[3]);

	/* round = 1 */
	setup.lossy_par = 1;
	data = UINT32_MAX; model = UINT32_MAX;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(93, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0x00000001, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFF, bitstream[1]);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFF8, bitstream[2]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[3]);

	/* data are bigger than max_data_bits */
	setup.lossy_par = 0;
	data = UINT32_MAX; model = 0;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_HIGH_VALUE, cmp_size);

	/* model are bigger than max_data_bits */
	setup.lossy_par = 0;
	cmp_size = 93;
	data = 0; model = UINT32_MAX;
	cmp_size = encode_value(data, model, cmp_size, &setup);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_HIGH_VALUE, cmp_size);
}


/**
 * @test cmp_get_max_used_bits
 */

void test_cmp_get_max_used_bits(void)
{
	struct cmp_max_used_bits max_used_bits = cmp_get_max_used_bits();

	TEST_ASSERT_EQUAL_INT(max_used_bits.nc_imagette, MAX_USED_NC_IMAGETTE_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.saturated_imagette, MAX_USED_SATURATED_IMAGETTE_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.fc_imagette, MAX_USED_FC_IMAGETTE_BITS);

	TEST_ASSERT_EQUAL_INT(max_used_bits.f_fx, MAX_USED_F_FX_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.f_efx, MAX_USED_F_EFX_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.f_ncob, MAX_USED_F_NCOB_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.f_ecob, MAX_USED_F_ECOB_BITS);

	TEST_ASSERT_EQUAL_INT(max_used_bits.s_exp_flags, MAX_USED_S_FX_EXPOSURE_FLAGS_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.s_fx, MAX_USED_S_FX_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.s_efx, MAX_USED_S_EFX_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.s_ncob, MAX_USED_S_NCOB_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.s_ecob, MAX_USED_S_ECOB_BITS);

	TEST_ASSERT_EQUAL_INT(max_used_bits.l_fx_variance, MAX_USED_L_FX_VARIANCE_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.l_efx, MAX_USED_L_EFX_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.l_ncob, MAX_USED_L_NCOB_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.l_ecob, MAX_USED_L_ECOB_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.l_cob_variance, MAX_USED_L_COB_VARIANCE_BITS);

	TEST_ASSERT_EQUAL_INT(max_used_bits.nc_offset_mean, MAX_USED_NC_OFFSET_MEAN_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.nc_offset_variance, MAX_USED_NC_OFFSET_VARIANCE_BITS);

	TEST_ASSERT_EQUAL_INT(max_used_bits.nc_background_mean, MAX_USED_NC_BACKGROUND_MEAN_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.nc_background_variance, MAX_USED_NC_BACKGROUND_VARIANCE_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.nc_background_outlier_pixels, MAX_USED_NC_BACKGROUND_OUTLIER_PIXELS_BITS);

	TEST_ASSERT_EQUAL_INT(max_used_bits.smeating_mean, MAX_USED_SMEARING_MEAN_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.smeating_variance_mean, MAX_USED_SMEARING_VARIANCE_MEAN_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.smearing_outlier_pixels, MAX_USED_SMEARING_OUTLIER_PIXELS_BITS);

	TEST_ASSERT_EQUAL_INT(max_used_bits.fc_offset_mean, MAX_USED_FC_OFFSET_MEAN_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.fc_offset_variance, MAX_USED_FC_OFFSET_VARIANCE_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.fc_offset_pixel_in_error, MAX_USED_FC_OFFSET_PIXEL_IN_ERROR_BITS);

	TEST_ASSERT_EQUAL_INT(max_used_bits.fc_background_mean, MAX_USED_FC_BACKGROUND_MEAN_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.fc_background_variance, MAX_USED_FC_BACKGROUND_VARIANCE_BITS);
	TEST_ASSERT_EQUAL_INT(max_used_bits.fc_background_outlier_pixels, MAX_USED_FC_BACKGROUND_OUTLIER_PIXELS_BITS);
}


void test_compress_imagette_diff(void)
{
	uint16_t data[] = {0xFFFF, 1, 0, 42, 0x8000, 0x7FFF, 0xFFFF};
	uint32_t output_buf[3] = {0xFFFF, 0xFFFF, 0xFFFF};
	struct cmp_cfg cfg = {0};
	int cmp_size;

	cfg.data_type = DATA_TYPE_IMAGETTE;
	cfg.cmp_mode = CMP_MODE_DIFF_ZERO;
	cfg.input_buf = data;
	cfg.samples = 7;
	cfg.golomb_par = 1;
	cfg.spill = 8;
	cfg.icu_output_buf = (uint32_t *)output_buf;
	cfg.buffer_length = 7;

	cmp_size = icu_compress_data(&cfg);
	TEST_ASSERT_EQUAL_INT(66, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0xDF6002AB, be32_to_cpu(output_buf[0]));
	TEST_ASSERT_EQUAL_HEX(0xFEB70000, be32_to_cpu(output_buf[1]));
	TEST_ASSERT_EQUAL_HEX(0x00000000, be32_to_cpu(output_buf[2]));
}

void test_compress_imagette_model(void)
{
	uint16_t data[]  = {0x0000, 0x0001, 0x0042, 0x8000, 0x7FFF, 0xFFFF, 0xFFFF};
	uint16_t model[] = {0x0000, 0xFFFF, 0xF301, 0x8FFF, 0x0000, 0xFFFF, 0x0000};
	uint16_t model_up[7] = {0};
	uint32_t output_buf[3] = {0xFFFF, 0xFFFF, 0xFFFF};
	struct cmp_cfg cfg = {0};
	int cmp_size;

	cfg.data_type = DATA_TYPE_IMAGETTE;
	cfg.cmp_mode = CMP_MODE_MODEL_MULTI;
	cfg.input_buf = data;
	cfg.model_buf = model;
	cfg.icu_new_model_buf = model_up;
	cfg.samples = 7;
	cfg.golomb_par = 3;
	cfg.spill = 8;
	cfg.model_value = 8;
	cfg.icu_output_buf = (uint32_t *)output_buf;
	cfg.buffer_length = 8;

	cmp_size = icu_compress_data(&cfg);

	TEST_ASSERT_EQUAL_INT(76, cmp_size);
	TEST_ASSERT_EQUAL_HEX(0x2BDB4F5E, be32_to_cpu(output_buf[0]));
	TEST_ASSERT_EQUAL_HEX(0xDFF5F9FF, be32_to_cpu(output_buf[1]));
	TEST_ASSERT_EQUAL_HEX(0xEC200000, be32_to_cpu(output_buf[2]));

	TEST_ASSERT_EQUAL_HEX(0x0000, model_up[0]);
	TEST_ASSERT_EQUAL_HEX(0x8000, model_up[1]);
	TEST_ASSERT_EQUAL_HEX(0x79A1, model_up[2]);
	TEST_ASSERT_EQUAL_HEX(0x87FF, model_up[3]);
	TEST_ASSERT_EQUAL_HEX(0x3FFF, model_up[4]);
	TEST_ASSERT_EQUAL_HEX(0xFFFF, model_up[5]);
	TEST_ASSERT_EQUAL_HEX(0x7FFF, model_up[6]);
}


void test_compress_imagette_stuff(void)
{
	uint16_t data[] = {0x0, 0x1, 0x23, 0x42, 0x8000, 0x7FFF, 0xFFFF};
	uint32_t output_buf[4] = {0};
	struct cmp_cfg cfg = {0};

	int cmp_size;
	uint8_t output_buf_exp[] = {
		0x00, 0x00, 0x00, 0x01,
		0x00, 0x23, 0x00, 0x42,
		0x80, 0x00, 0x7F, 0xFF,
		0xFF, 0xFF, 0x00, 0x00};

	cfg.data_type = DATA_TYPE_IMAGETTE;
	cfg.cmp_mode = CMP_MODE_STUFF;
	cfg.input_buf = data;
	cfg.samples = 7;
	cfg.icu_output_buf = (uint32_t *)output_buf;
	cfg.buffer_length = 8;
	cfg.golomb_par = 16; /* how many used bits has the maximum data value */

	cmp_size = icu_compress_data(&cfg);

	uint32_t *output_buf_exp_32 = (uint32_t *)output_buf_exp;
	TEST_ASSERT_EQUAL_INT(7*16, cmp_size);
	TEST_ASSERT_EQUAL_HEX16(output_buf_exp_32[0], output_buf[0]);
	TEST_ASSERT_EQUAL_HEX16(output_buf_exp_32[1], output_buf[1]);
	TEST_ASSERT_EQUAL_HEX16(output_buf_exp_32[2], output_buf[2]);
	TEST_ASSERT_EQUAL_HEX16(output_buf_exp_32[3], output_buf[3]);
}


void test_compress_imagette_raw(void)
{
	uint16_t data[] = {0x0, 0x1, 0x23, 0x42, INT16_MIN, INT16_MAX, UINT16_MAX};
	uint16_t output_buf[7] = {0};
	struct cmp_cfg cfg = {0};

	int cmp_size;

	cfg.data_type = DATA_TYPE_IMAGETTE;
	cfg.model_buf = NULL;
	cfg.input_buf = data;
	cfg.samples = 7;
	cfg.icu_output_buf = (uint32_t *)output_buf;
	cfg.buffer_length = 7;


	cmp_size = icu_compress_data(&cfg);

	TEST_ASSERT_EQUAL_INT(7*16, cmp_size);
	TEST_ASSERT_EQUAL_HEX16(0x0, be16_to_cpu(output_buf[0]));
	TEST_ASSERT_EQUAL_HEX16(0x1, be16_to_cpu(output_buf[1]));
	TEST_ASSERT_EQUAL_HEX16(0x23, be16_to_cpu(output_buf[2]));
	TEST_ASSERT_EQUAL_HEX16(0x42, be16_to_cpu(output_buf[3]));
	TEST_ASSERT_EQUAL_HEX16(INT16_MIN, be16_to_cpu(output_buf[4]));
	TEST_ASSERT_EQUAL_HEX16(INT16_MAX, be16_to_cpu(output_buf[5]));
	TEST_ASSERT_EQUAL_HEX16(UINT16_MAX, be16_to_cpu(output_buf[6]));
}


void test_compress_s_fx_raw(void)
{
	struct s_fx data[7];
	struct cmp_cfg cfg = {0};
	int cmp_size, cmp_size_exp;
	size_t i;

	cfg.data_type = DATA_TYPE_S_FX;
	cfg.model_buf = NULL;
	cfg.samples = 7;
	cfg.input_buf = malloc(cmp_cal_size_of_data(cfg.samples, cfg.data_type));
	cfg.buffer_length = 7;
	cfg.icu_output_buf = malloc(cmp_cal_size_of_data(cfg.buffer_length, cfg.data_type));
	TEST_ASSERT_NOT_NULL(cfg.icu_output_buf);
	TEST_ASSERT_NOT_NULL(cfg.input_buf);

	data[0].exp_flags = 0x0;
	data[0].fx = 0x0;
	data[1].exp_flags = 0x1;
	data[1].fx = 0x1;
	data[2].exp_flags = 0x2;
	data[2].fx = 0x23;
	data[3].exp_flags = 0x3;
	data[3].fx = 0x42;
	data[4].exp_flags = 0x0;
	data[4].fx = INT32_MIN;
	data[5].exp_flags = 0x3;
	data[5].fx = INT32_MAX;
	data[6].exp_flags = 0x1;
	data[6].fx = UINT32_MAX;

	struct multi_entry_hdr *hdr = cfg.input_buf;
	memset(hdr, 0x42, sizeof(struct multi_entry_hdr));
	memcpy(hdr->entry, data, sizeof(data));

	cmp_size = icu_compress_data(&cfg);

	cmp_size_exp = (sizeof(data) + sizeof(struct multi_entry_hdr)) * CHAR_BIT;
	TEST_ASSERT_EQUAL_INT(cmp_size_exp, cmp_size);

	for (i = 0; i < ARRAY_SIZE(data); i++) {
		hdr = (struct multi_entry_hdr *)cfg.icu_output_buf;
		struct s_fx *p = (struct s_fx *)hdr->entry;

		TEST_ASSERT_EQUAL_HEX(data[i].exp_flags, p[i].exp_flags);
		TEST_ASSERT_EQUAL_HEX(data[i].fx, cpu_to_be32(p[i].fx));
	}
}


void test_compress_s_fx_staff(void)
{
	struct s_fx data[5];
	struct cmp_cfg cfg = {0};
	int cmp_size, cmp_size_exp;
	struct multi_entry_hdr *hdr;
	uint32_t *cmp_data;

	/* setup configuration */
	cfg.data_type = DATA_TYPE_S_FX;
	cfg.cmp_mode = CMP_MODE_STUFF;
	cfg.samples = 5;
	cfg.input_buf = malloc(cmp_cal_size_of_data(cfg.samples, cfg.data_type));
	TEST_ASSERT_NOT_NULL(cfg.input_buf);
	cfg.buffer_length = 5;
	cfg.icu_output_buf = malloc(cmp_cal_size_of_data(cfg.buffer_length, cfg.data_type));
	TEST_ASSERT_NOT_NULL(cfg.icu_output_buf);
	cfg.cmp_par_exp_flags = 2;
	cfg.cmp_par_fx = 21;

	/* generate input data */
	hdr = cfg.input_buf;
	/* use dummy data for the header */
	memset(hdr, 0x42, sizeof(struct multi_entry_hdr));
	data[0].exp_flags = 0x0;
	data[0].fx = 0x0;
	data[1].exp_flags = 0x1;
	data[1].fx = 0x1;
	data[2].exp_flags = 0x2;
	data[2].fx = 0x23;
	data[3].exp_flags = 0x3;
	data[3].fx = 0x42;
	data[4].exp_flags = 0x0;
	data[4].fx = 0x001FFFFF;
	memcpy(hdr->entry, data, sizeof(data));

	cmp_size = icu_compress_data(&cfg);

	cmp_size_exp = 5 * (2 + 21) + MULTI_ENTRY_HDR_SIZE * CHAR_BIT;
	TEST_ASSERT_EQUAL_INT(cmp_size_exp, cmp_size);
	TEST_ASSERT_FALSE(memcmp(cfg.input_buf, cfg.icu_output_buf, MULTI_ENTRY_HDR_SIZE));
	hdr = (void *)cfg.icu_output_buf;
	cmp_data = (uint32_t *)hdr->entry;
	TEST_ASSERT_EQUAL_HEX(0x00000080, be32_to_cpu(cmp_data[0]));
	TEST_ASSERT_EQUAL_HEX(0x00060001, be32_to_cpu(cmp_data[1]));
	TEST_ASSERT_EQUAL_HEX(0x1E000423, be32_to_cpu(cmp_data[2]));
	TEST_ASSERT_EQUAL_HEX(0xFFFFE000, be32_to_cpu(cmp_data[3]));

	free(cfg.input_buf);
	free(cfg.icu_output_buf);
}


void test_compress_s_fx_model_multi(void)
{
	struct s_fx data[6], model[6];
	struct s_fx *up_model_buf;
	struct cmp_cfg cfg = {0};
	int cmp_size;
	struct multi_entry_hdr *hdr;
	uint32_t *cmp_data;
	struct cmp_max_used_bits max_used_bits;

	/* setup configuration */
	cfg.data_type = DATA_TYPE_S_FX;
	cfg.cmp_mode = CMP_MODE_MODEL_MULTI;
	cfg.model_value = 11;
	cfg.samples = 6;
	cfg.input_buf = malloc(cmp_cal_size_of_data(cfg.samples, cfg.data_type));
	TEST_ASSERT_NOT_NULL(cfg.input_buf);
	cfg.model_buf = malloc(cmp_cal_size_of_data(cfg.samples, cfg.data_type));
	TEST_ASSERT_NOT_NULL(cfg.model_buf);
	cfg.icu_new_model_buf = malloc(cmp_cal_size_of_data(cfg.samples, cfg.data_type));
	TEST_ASSERT_NOT_NULL(cfg.icu_new_model_buf);
	cfg.buffer_length = 6;
	cfg.icu_output_buf = malloc(cmp_cal_size_of_data(cfg.buffer_length, cfg.data_type));
	TEST_ASSERT_NOT_NULL(cfg.icu_output_buf);
	cfg.cmp_par_exp_flags = 1;
	cfg.spill_exp_flags = 8;
	cfg.cmp_par_fx = 3;
	cfg.spill_fx = 35;


	/* generate input data */
	hdr = cfg.input_buf;
	/* use dummy data for the header */
	memset(hdr, 0x42, sizeof(struct multi_entry_hdr));
	data[0].exp_flags = 0x0;
	data[0].fx = 0x0;
	data[1].exp_flags = 0x1;
	data[1].fx = 0x1;
	data[2].exp_flags = 0x2;
	data[2].fx = 0x23;
	data[3].exp_flags = 0x3;
	data[3].fx = 0x42;
	data[4].exp_flags = 0x0;
	data[4].fx = 0x001FFFFF;
	data[5].exp_flags = 0x0;
	data[5].fx = 0x0;
	memcpy(hdr->entry, data, sizeof(data));

	/* generate model data */
	hdr = cfg.model_buf;
	/* use dummy data for the header */
	memset(hdr, 0x41, sizeof(struct multi_entry_hdr));
	model[0].exp_flags = 0x0;
	model[0].fx = 0x0;
	model[1].exp_flags = 0x3;
	model[1].fx = 0x1;
	model[2].exp_flags = 0x0;
	model[2].fx = 0x42;
	model[3].exp_flags = 0x0;
	model[3].fx = 0x23;
	model[4].exp_flags = 0x3;
	model[4].fx = 0x0;
	model[5].exp_flags = 0x2;
	model[5].fx = 0x001FFFFF;
	memcpy(hdr->entry, model, sizeof(model));

	max_used_bits = cmp_get_max_used_bits();
	max_used_bits.s_exp_flags = 2;
	max_used_bits.s_fx = 21;
	cmp_set_max_used_bits(&max_used_bits);

	cmp_size = icu_compress_data(&cfg);

	TEST_ASSERT_EQUAL_INT(166, cmp_size);
	TEST_ASSERT_FALSE(memcmp(cfg.input_buf, cfg.icu_output_buf, MULTI_ENTRY_HDR_SIZE));
	cmp_data = &cfg.icu_output_buf[MULTI_ENTRY_HDR_SIZE/sizeof(uint32_t)];
	TEST_ASSERT_EQUAL_HEX(0x1C77FFA6, be32_to_cpu(cmp_data[0]));
	TEST_ASSERT_EQUAL_HEX(0xAFFF4DE5, be32_to_cpu(cmp_data[1]));
	TEST_ASSERT_EQUAL_HEX(0xCC000000, be32_to_cpu(cmp_data[2]));

	TEST_ASSERT_FALSE(memcmp(cfg.input_buf, cfg.icu_output_buf, MULTI_ENTRY_HDR_SIZE));
	hdr = cfg.icu_new_model_buf;
	up_model_buf = (struct s_fx *)hdr->entry;
	TEST_ASSERT_EQUAL_HEX(0x0, up_model_buf[0].exp_flags);
	TEST_ASSERT_EQUAL_HEX(0x0, up_model_buf[0].fx);
	TEST_ASSERT_EQUAL_HEX(0x2, up_model_buf[1].exp_flags);
	TEST_ASSERT_EQUAL_HEX(0x1, up_model_buf[1].fx);
	TEST_ASSERT_EQUAL_HEX(0x0, up_model_buf[2].exp_flags);
	TEST_ASSERT_EQUAL_HEX(0x38, up_model_buf[2].fx);
	TEST_ASSERT_EQUAL_HEX(0x0, up_model_buf[3].exp_flags);
	TEST_ASSERT_EQUAL_HEX(0x2C, up_model_buf[3].fx);
	TEST_ASSERT_EQUAL_HEX(0x2, up_model_buf[4].exp_flags);
	TEST_ASSERT_EQUAL_HEX(0x9FFFF, up_model_buf[4].fx);
	TEST_ASSERT_EQUAL_HEX(0x1, up_model_buf[5].exp_flags);
	TEST_ASSERT_EQUAL_HEX(0x15FFFF, up_model_buf[5].fx);

	free(cfg.input_buf);
	free(cfg.model_buf);
	free(cfg.icu_new_model_buf);
	free(cfg.icu_output_buf);
}


