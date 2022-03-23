#include <string.h>

#include "unity.h"

#include "cmp_support.h"
/* this is a hack to test static functions */
#include "../lib/cmp_icu_new.c"


/**
 * @test map_to_pos
 */

void test_map_to_pos(void)
{
	uint32_t value_to_map;
	uint32_t max_value_bits;
	uint32_t mapped_value;

	/* test mapping 32 bits values */
	max_value_bits = 32;

	value_to_map = 0;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(0, mapped_value);

	value_to_map = -1U;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = 1;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(2, mapped_value);

	value_to_map = 42;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(84, mapped_value);

	value_to_map = INT32_MAX;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_HEX(UINT32_MAX-1, mapped_value);

	value_to_map = INT32_MIN;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_HEX(UINT32_MAX, mapped_value);

	/* test mapping 16 bits values */
	max_value_bits = 16;

	value_to_map = -1U;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	/* test mapping 6 bits values */
	max_value_bits = 6;

	value_to_map = 0;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(0, mapped_value);

	value_to_map = -1U;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = UINT32_MAX;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = -1U & 0x3FU;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = 63;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(1, mapped_value);

	value_to_map = 1;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(2, mapped_value);

	value_to_map = 31;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(62, mapped_value);

	value_to_map = -33U; /* aka 31 */
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(62, mapped_value);

	value_to_map = -32U;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
	TEST_ASSERT_EQUAL_INT(63, mapped_value);

	value_to_map = 32;
	mapped_value = map_to_pos(value_to_map, max_value_bits);
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

	/* offset lager than max_bit_len(l) */
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
 * @test Rice_encoder
 */

void test_Rice_encoder(void)
{
	uint32_t value, g_par, log2_g_par, cw, cw_len;

	/* test minimum Golomb parameter */
	value = 0; log2_g_par = (uint32_t)ilog_2(MIN_ICU_GOLOMB_PAR); g_par = 1U << log2_g_par; cw = ~0U;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(1, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 31;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFE, cw);

	/* test some arbitrary values */
	value = 0; log2_g_par = 4; g_par = 1U << log2_g_par; cw = ~0U;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(5, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(5, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1, cw);

	value = 42;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(7, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x6a, cw);

	value = 446;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFEE, cw);

	value = 447;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFEF, cw);

	/* test maximum Golomb parameter for Rice_encoder */
	value = 0; log2_g_par = (uint32_t)ilog_2(MAX_ICU_GOLOMB_PAR); g_par = 1U << log2_g_par; cw = ~0U;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1, cw);

	value = 0x7FFFFFFE;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFE, cw);

	value = 0x7FFFFFFF;
	cw_len = Rice_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFF, cw);
}


/**
 * @test Golomb_encoder
 */

void test_Golomb_encoder(void)
{
	uint32_t value, g_par, log2_g_par, cw, cw_len;

	/* test minimum Golomb parameter */
	value = 0; g_par = MIN_ICU_GOLOMB_PAR; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(1, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 31;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFE, cw);


	/* test some arbitrary values with g_par = 16 */
	value = 0; g_par = 16; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(5, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(5, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1, cw);

	value = 42;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(7, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x6a, cw);

	value = 446;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFEE, cw);

	value = 447;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFEF, cw);


	/* test some arbitrary values with g_par = 3 */
	value = 0; g_par = 3; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(2, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(3, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x2, cw);

	value = 42;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(16, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFC, cw);

	value = 44;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(17, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1FFFB, cw);

	value = 88;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFA, cw);

	value = 89;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0xFFFFFFFB, cw);


	/* test maximum Golomb parameter for Golomb_encoder */
	value = 0; g_par = MAX_ICU_GOLOMB_PAR; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x0, cw);

	value = 1; g_par = MAX_ICU_GOLOMB_PAR; log2_g_par = (uint32_t)ilog_2(g_par); cw = ~0U;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x1, cw);

	value = 0x7FFFFFFE;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
	TEST_ASSERT_EQUAL_INT(32, cw_len);
	TEST_ASSERT_EQUAL_HEX(0x7FFFFFFE, cw);

	value = 0x7FFFFFFF;
	cw_len = Golomb_encoder(value, g_par, log2_g_par, &cw);
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
	setup.outlier_par = 32;
	setup.max_value_bits = 32;
	setup.generate_cw = Rice_encoder;
	setup.bitstream_adr = bitstream;
	setup.max_bit_len = sizeof(bitstream) * CHAR_BIT;

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
	setup.max_value_bits = 6;

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
	setup.max_bit_len = 32;
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
	setup.outlier_par = 16;
	setup.max_value_bits = 32;
	setup.generate_cw = Rice_encoder;
	setup.bitstream_adr = bitstream;
	setup.max_bit_len = sizeof(bitstream) * CHAR_BIT;

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
	setup.max_bit_len = 32;

	stream_len = 32;
	data = 31; model = 0;
	stream_len = encode_value_multi(data, model, stream_len, &setup);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, stream_len);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[0]);
	TEST_ASSERT_EQUAL_HEX(0, bitstream[1]);
}
