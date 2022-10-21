/**
 * @file cmp_entity.h
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
 * @brief compression data types tests
 */


#include <stdint.h>

#include <unity.h>
#include <cmp_data_types.h>


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


/**
 * @test cmp_set_max_us
 */

void test_cmp_set_max_used_bits(void)
{
	struct cmp_max_used_bits set_max_used_bits = {0};
	cmp_set_max_used_bits(&set_max_used_bits);
	cmp_set_max_used_bits(NULL);
}
