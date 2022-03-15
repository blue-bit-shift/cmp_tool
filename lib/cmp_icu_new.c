/**
 * @file   icu_cmp.c
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at),
 * @date   2020
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
 * @brief software compression library
 * @see Data Compression User Manual PLATO-UVIE-PL-UM-0001
 */


#include <stdint.h>

#include "cmp_debug.h"


/* return code if the bitstream buffer is too small to store the whole bitstream */
#define CMP_ERROR_SAMLL_BUF -2


/**
 * @brief put the value of up to 32 bits into a bitstream accessed as 32-bit
 *	RAM in big-endian
 *
 * @param value		the value to put
 * @param n_bits	number of bits to put in the bitstream
 * @param bit_offset	bit index where the bits will be put, seen from the very
 *			beginning of the bitstream
 * @param bitstream_adr	this is the pointer to the beginning of the bitstream
 *			(can be NULL)
 * @param max_bit_len	maximum length of the bitstream in bits; is ignored if
 *			bitstream_adr is NULL
 *
 * @returns length in bits of the generated bitstream on success; returns
 *          negative in case of erroneous input; returns CMP_ERROR_SAMLL_BUF if
 *          the bitstream buffer is too small to put the value in the bitstream
 * @note a value with more bits set as the n_bits parameter is considered as an
 *	erroneous input.
 */

static int put_n_bits32(uint32_t value, unsigned int n_bits, int bit_offset,
			uint32_t *bitstream_adr, unsigned int max_bit_len)
{
	uint32_t *local_adr;
	uint32_t mask;
	unsigned int shiftRight, shiftLeft, bitsLeft, bitsRight;
	int stream_len = (int)(n_bits + (unsigned int)bit_offset); /* overflow results in a negative return value */

	/* leave in case of erroneous input */
	if (bit_offset < 0)
		return -1;

	if (n_bits == 0)
		return 0;

	if (n_bits > 32)
		return -1;

	/* (M) is the n_bits parameter large enough to cover all value bits; the
	 * calculations can be re-used in the unsegmented code, so we have no overhead
	 */
	shiftRight = 32 - n_bits;
	mask = 0xFFFFFFFFU >> shiftRight;
	if (value & ~mask) {
		debug_print("Error: Not all set bits in the put value are added to the bitstream. Check value n_bits parameter combination.\n");
		return -1;
	}

	/* Do we need to write data to the bitstream? */
	if (!bitstream_adr)
		return stream_len;

	/* Check if bitstream buffer is large enough */
	if ((unsigned int)stream_len > max_bit_len) {
		debug_print("Error: The buffer for the compressed data is too small to hold the compressed data. Try a larger buffer_length parameter.\n");
		return CMP_ERROR_SAMLL_BUF;
	}

	/* Separate the bit_offset into word offset (set local_adr pointer) and local bit offset (bitsLeft) */
	local_adr = bitstream_adr + (bit_offset >> 5);
	bitsLeft = bit_offset & 0x1F;

	/* Calculate the bitsRight for the unsegmented case. If bitsRight is
	 * negative we need to split the value over two words
	 */
	bitsRight = shiftRight - bitsLeft;

	if ((int)bitsRight >= 0) {
		/*         UNSEGMENTED
		 *
		 *|-----------|XXXXX|----------------|
		 *   bitsLeft    n       bitsRight
		 *
		 *  -> to get the mask:
		 *  shiftRight = bitsLeft + bitsRight = 32 - n
		 *  shiftLeft = bitsRight = 32 - n - bitsLeft = shiftRight - bitsLeft
		 */

		shiftLeft = bitsRight;

		/* generate the mask, the bits for the values will be true
		 * shiftRight = 32 - n_bits; see (M) above!
		 * mask = (0XFFFFFFFF >> shiftRight) << shiftLeft; see (M) above!
		 */
		mask <<= shiftLeft;
		value <<= shiftLeft;

		/* clear the destination with inverse mask */
		*(local_adr) &= ~mask;

		/* assign the value */
		*(local_adr) |= value;

	} else {
		/*                             SEGMENTED
		 *
		 *|-----------------------------|XXX| |XX|------------------------------|
		 *          bitsLeft              n1   n2          bitsRight
		 *
		 *  -> to get the mask part 1:
		 *  shiftRight = bitsLeft
		 *  n1 = n - (bitsLeft + n - 32) = 32 - bitsLeft
		 *
		 *  -> to get the mask part 2:
		 *  n2 = bitsLeft + n - 32 = -(32 - n - bitsLeft) = -(bitsRight_UNSEGMENTED)
		 *  shiftLeft = 32 - n2 = 32 - (bitsLeft + n - 32) = 64 - bitsLeft - n
		 *
		 */

		unsigned int n2 = -bitsRight;

		/* part 1: */
		shiftRight = bitsLeft;
		mask = 0XFFFFFFFFU >> shiftRight;

		/* clear the destination with inverse mask */
		*(local_adr) &= ~mask;

		/* assign the value part 1 */
		*(local_adr) |= (value >> n2);

		/* part 2: */
		/* adjust address */
		local_adr += 1;
		shiftLeft = 32 - n2;
		mask = 0XFFFFFFFFU << shiftLeft;

		/* clear the destination with inverse mask */
		*(local_adr) &= ~mask;

		/* assign the value part 2 */
		*(local_adr) |= (value << shiftLeft);
	}
	return stream_len;
}

