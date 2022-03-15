#include <string.h>

#include "unity.h"

/* this is a hack to test static functions */
#include "../lib/cmp_icu_new.c"



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
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	/*** n=0 ***/

	/* do not write, left border */
	v = 0; n = 0; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	/* TODO: not a valid test */
	v = 0xffffffff; n = 0; o = 0;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	/* do not write, right border */
	v = 0; n = 0; o = 31;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

	/* TODO: not a valid test */
	/* test value = 0xffffffff; N = 0 */
	v = 0xffffffff; n = 0; o = 31;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray0[0] == 0);

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(0, rval);
	TEST_ASSERT(testarray1[0] == 0xffffffff);

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

	rval = put_n_bits32(v, n, o, testarray1, l);
	TEST_ASSERT_EQUAL_INT(rval, 32);
	TEST_ASSERT(testarray1[0] == 0xf0f0abcd);
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

	/* error cases */
	/* n too large */
	v = 0x0; n = 33; o = 1;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(rval, -1);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(rval, -1);

	/* value larger than n allows */
	v = 0x7f; n = 6; o = 10;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(-1, rval);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);

	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(-1, rval);

	/* try to put too much in the bitstream */
	v = 0x1; n = 1; o = 96;
	rval = put_n_bits32(v, n, o, testarray0, l);
	TEST_ASSERT_EQUAL_INT(CMP_ERROR_SAMLL_BUF, rval);
	TEST_ASSERT(testarray0[0] == 0);
	TEST_ASSERT(testarray0[1] == 0);
	TEST_ASSERT(testarray0[2] == 0);

	/* this should work */
	rval = put_n_bits32(v, n, o, NULL, l);
	TEST_ASSERT_EQUAL_INT(97, rval);

	/* offset lager than max_bit_len */
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
}
