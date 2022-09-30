#include <stdint.h>

#include "unity.h"
#include "cmp_data_types.h"

/**
 * @test cmp_cal_size_of_data
 */

void test_cmp_cal_size_of_data(void)
{
	unsigned int s;

	s = cmp_cal_size_of_data(1, DATA_TYPE_IMAGETTE);
	TEST_ASSERT_EQUAL_UINT(sizeof(uint16_t), s);

	s = cmp_cal_size_of_data(1, DATA_TYPE_F_FX);
	TEST_ASSERT_EQUAL_UINT(sizeof(struct f_fx)+MULTI_ENTRY_HDR_SIZE, s);

	/* overflow tests */
	s = cmp_cal_size_of_data(0x1999999A, DATA_TYPE_BACKGROUND);
	TEST_ASSERT_EQUAL_UINT(0, s);
	s = cmp_cal_size_of_data(0x19999999, DATA_TYPE_BACKGROUND);
	TEST_ASSERT_EQUAL_UINT(0, s);
}

