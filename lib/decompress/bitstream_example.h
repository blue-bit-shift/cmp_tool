/**
 * @file   bitstream.h
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2023
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
 * @brief this library handles the decoding of a MSB-first bitstream
 *
 * This API consists of small unitary functions, which must be inlined for best performance.
 * Since link-time-optimization is not available for all compilers, these
 * functions are defined into a .h to be included.
 *
 * Start by invoking bit_init_DStream(). A chunk of the bitstream is then stored
 * into a local register. Local register size is 64-bits.
 * You can then retrieve bit-fields stored into the local register. Local
 * register is explicitly reloaded from memory by the bit_refill_DStream() method.
 * A reload guarantee a minimum of 57 bits when its result is bit_DStream_unfinished.
 * Otherwise, it can be less than that, so proceed accordingly.
 * Checking if DStream has reached its end can be performed with bit_end_of_DStream().
 *
 * This is based on the bitstream part of the FiniteStateEntropy library, see:
 * https://github.com/Cyan4973/FiniteStateEntropy/blob/dev/lib/bitstream.h
 * by @author Yann Collet
 * As well as some ideas from this blog post:
 * https://fgiesen.wordpress.com/2018/02/20/reading-bits-in-far-too-many-ways-part-2/
 * by @author Fabian Giesen
 */


#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../cmp_tool_gitlab/include/byteorder.h"
#include "../../cmp_tool_gitlab/include/compiler.h"
#include <assert.h>


/*-********************************************
*  bitstream decoding API
**********************************************/

/**
 * @brief bitstream decoding context type
 */
typedef struct {
    uint64_t     bit_container;	/* last 64 bits we read */
    unsigned int bits_consumed;	/* number of consumed bits in bit_container */
    const char*  cursor;	/* position we read the bitstream */
    const char*  limit_ptr;	/* beginning of the end of the bitstream */
} bit_DStream_t;

/**
 * @brief result of bit_refill_DStream()
 */
enum bit_DStream_status{
	bit_DStream_unfinished = 0,
	bit_DStream_endOfBuffer = 1,
	bit_DStream_completed = 2,
	bit_DStream_overflow = 3
};

static __inline size_t   bit_init_DStream(bit_DStream_t* bitD, const void* src_buffer, size_t src_size);
static __inline size_t   bit_read_bits(bit_DStream_t* bitD, unsigned int nb_bits);
static __inline uint64_t bit_peek_bits(const bit_DStream_t* bitD, unsigned int nb_bits);
static __inline void     bit_consume_bits(bit_DStream_t* bitD, unsigned int nb_bits);
static __inline enum bit_DStream_status bit_refill_DStream(bit_DStream_t* bitD);


/*-**************************************************************
*  Internal functions
****************************************************************/

/**
 * @brief read 8 byte big endian data form an unaligned address
 * @param mem_ptr pointer to the data (can be unaligned)
 * @returns 64 bit data at mem_ptr address in big endian byte order
 */

static __inline uint64_t get_unaligned_be64(const void* mem_ptr)
{
	typedef struct { uint64_t x; } __attribute__((packed)) unalign64;

        return be64_to_cpu(((const unalign64*)mem_ptr)->x);
}


/**
 * @brief initialize a bit_DStream_t
 * @param src_size	buffer to read from
 * @param bitD		a pointer to an already allocated bit_DStream_t structure
 * @param src_size	size of the bitstream in bytes
 * @returns size of stream (== src_size), or negative if a problem is detected
 */

static __inline size_t bit_init_DStream(bit_DStream_t* bitD, const void* src_buffer,
					size_t src_size)
{
	if (src_size < 1) {
		memset(bitD, 0, sizeof(*bitD));
		return -1;
	}

	bitD->cursor = (const char*)src_buffer;

	if (src_size >= sizeof(bitD->bit_container)) {  /* normal case */
		bitD->bits_consumed = 0;
		bitD->bit_container = get_unaligned_be64(src_buffer);
		bitD->limit_ptr = (const char*)src_buffer + src_size - sizeof(bitD->bit_container);
	} else {
		bitD->bits_consumed = (sizeof(bitD->bit_container) - src_size)*8;

		bitD->bit_container = (uint64_t)(((const uint8_t*)(src_buffer))[0]) << 56;
		switch(src_size) {
		case 7:
			bitD->bit_container += (uint64_t)(((const uint8_t*)(src_buffer))[6]) <<  8;
			/* fall-through */
		case 6:
			bitD->bit_container += (uint64_t)(((const uint8_t*)(src_buffer))[5]) << 16;
			/* fall-through */
		case 5:
			bitD->bit_container += (uint64_t)(((const uint8_t*)(src_buffer))[4]) << 24;
			/* fall-through */
		case 4:
			bitD->bit_container += (uint64_t)(((const uint8_t*)(src_buffer))[3]) << 32;
			/* fall-through 
		case 3:
			bitD->bit_container += (uint64_t)(((const uint8_t*)(src_buffer))[2]) << 40;
			/* fall-through */
		case 2:
			bitD->bit_container += (uint64_t)(((const uint8_t*)(src_buffer))[1]) << 48;
			/* fall-through */
		default:
			break;
		}
		bitD->bit_container >>= bitD->bits_consumed;

		bitD->limit_ptr = bitD->cursor;
	}
	return src_size;
}


/**
 * @brief provides next n bits from local register; local register is not modified
 * @warning if reading more nb_bits than contained into local register
 *	(bitD->bitsConsumed + nbBits > 64), bitstream is likely corrupted,
 *	and the result is undefined
 * @param bitD		a pointer to a bit_DStream_t context
 * @param nb_bits	number of bits to look; only works if 1 <= nb_bits <= 56
 * @returns extracted value
 */

static __inline uint64_t bit_peek_bits(const bit_DStream_t* bitD, unsigned int nb_bits)
{
	/* mask for the shift value register to prevent undefined behavior */
	uint32_t const reg_mask = 0x3F;
	/* shift out the bits we've already consumed */
	uint64_t const remaining = bitD->bit_container << (bitD->bits_consumed & reg_mask);

	assert(nb_bits >= 1 && nb_bits <= 57);
	return remaining >> (64 - nb_bits); /* return the top nb_bits bits */
}


/**
 * @brief count the leading ones in the local register; local register is not modified
 * @warning if all bits are consumed in local register (bitD->bitsConsumed  >= 64),
 *	the result is undefined
 * @param bitD		a pointer to a bit_DStream_t context
 * @returns number of leading ones; up to maximum 63
 */

static __inline unsigned int bit_count_leading_ones(const bit_DStream_t* bitD)
{
	/* mask for the shift value register to prevent undefined behavior */
	uint32_t const reg_mask = 0x3F;
	/* shift out the bits we've already consumed */
	uint64_t remaining = bitD->bit_container << (bitD->bits_consumed & reg_mask);

	/* limit count to maximum 63 ones to prevent undefined behavior */
	remaining &= ~1ULL;

	return __builtin_clzll(~remaining);
}


/**
 * @brief mark next n bits in the local register as consumed
 * @param bitD		a pointer to a bit_DStream_t context
 * @param nb_bits	number of bits to skip
 */

static __inline void bit_consume_bits(bit_DStream_t* bitD, unsigned int nb_bits)
{
    bitD->bits_consumed += nb_bits;
}


/**
 * @brief read and consume next n bits from  the local register
 * @warning if reading more nb_bits than contained into local register
 *	(bitD->bitsConsumed + nbBits > 64), bitstream is likely corrupted,
 *	and the result is undefined
 * @param bitD		a pointer to a bit_DStream_t context
 * @param nb_bits	number of bits to skip
 * @returns extracted value
 */

static __inline size_t bit_read_bits(bit_DStream_t* bitD, unsigned int nb_bits)
{
    size_t const value = bit_peek_bits(bitD, nb_bits);
    bit_consume_bits(bitD, nb_bits);
    return value;
}


/**
 * @brief refill the local register from the buffer previously set in bit_init_DStream()
 *
 * similar to bit_refill_DStream(), but with two differences:
 * 1. bits_consumed <= 64 must hold!
 * 2. Returns bit_DStream_overflow when (bitD->cursor + bitD->bits_consumed/8) < bitD->limit_ptr,
 *	at this point you must use bit_refill_DStream() to reload.
 *
 * @param bitD	a pointer to a bit_DStream_t context
 *
 * @returns the status of bit_DStream_t internal register; when status ==
 *	BIT_DStream_unfinished, internal register is filled with at least 57 bits
 */

static __inline enum bit_DStream_status bit_refill_DStream_fast(bit_DStream_t* bitD)
{
	/* how many bytes to read into bit buffer? */
	unsigned int const nb_bytes = bitD->bits_consumed >> 3;

	if (unlikely((bitD->cursor + nb_bytes) > bitD->limit_ptr))
		return bit_DStream_overflow;

	assert(bitD->bits_consumed <= sizeof(bitD->bit_container)*8);

	bitD->cursor += nb_bytes;

	/* Number of bits in the current byte we've already consumed (we took
	 * care of the full bytes; these are the leftover bits that didn't make
	 * a full byte.)
	 */
	bitD->bits_consumed &= 7;
	bitD->bit_container = get_unaligned_be64(bitD->cursor);

	return bit_DStream_unfinished;
}


/**
 * @brief refill the local register from the buffer previously set in bit_init_DStream()
 * @param bitD	a pointer to a bit_DStream_t context
 * @returns the status of bit_DStream_t internal register; when status ==
 *	BIT_DStream_unfinished, internal register is filled with at least 57 bits
 */

static __inline enum bit_DStream_status bit_refill_DStream(bit_DStream_t* bitD)
{
	unsigned int const nb_bytes = bitD->bits_consumed >> 3;

	if (bitD->cursor + nb_bytes < bitD->limit_ptr)
		return bit_refill_DStream_fast(bitD);

	if (bitD->bits_consumed > (sizeof(bitD->bit_container)*8))  /* overflow detected, like end of stream */
		return bit_DStream_overflow;

	if (bitD->cursor == bitD->limit_ptr) {
		if (bitD->bits_consumed < sizeof(bitD->bit_container)*8)
			return bit_DStream_endOfBuffer;
		return bit_DStream_completed;
	}

	/* limit_ptr < cursor < end */
	bitD->bits_consumed -= (bitD->limit_ptr - bitD->cursor)*8;
	bitD->cursor = bitD->limit_ptr;
	bitD->bit_container = get_unaligned_be64(bitD->cursor);

	return bit_DStream_endOfBuffer;
}


/**
 * @brief Check if DStream has reached its end
 * @param bitD	a bitstream decoding context
 * @returns 1 if DStream has _exactly_ reached its end (all bits consumed).
 */

static __inline unsigned int bit_end_of_DStream(const bit_DStream_t* bitD)
{
    return ((bitD->cursor == bitD->limit_ptr) &&
	    (bitD->bits_consumed == sizeof(bitD->bit_container)*8));
}

#endif /* BITSTREAM_H */
