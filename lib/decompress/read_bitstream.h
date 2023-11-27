#ifndef READ_BITSTREAM_H
#define READ_BITSTREAM_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#include "../common/byteorder.h"


static __inline uint64_t bit_read_unaligned_64(const void* ptr)
{
	typedef __attribute__((aligned(1))) uint64_t unalign64;
	return *(const unalign64*)ptr;
}


static __inline uint64_t bit_read_unalingned_be64(const void* ptr)
{
	return cpu_to_be64(bit_read_unaligned_64(ptr));
}


/**
 * @brief bitstream decoding context type
 */

struct bit_decoder
{
	uint64_t bit_container;
	unsigned int bits_consumed;
	const uint8_t* cursor;
	const uint8_t* limit_ptr;
};


static __inline size_t bit_init_decoder(struct bit_decoder *dec, const void* buf,
				   size_t buf_size)
{
	if (buf_size < 1) {
		memset(dec, 0, sizeof(*dec));
		return 0;
	}

	dec->cursor = (const uint8_t *)buf;

	if (buf_size >= sizeof(dec->bit_container)) {
		dec->bits_consumed = 0;
		dec->bit_container = bit_read_unalingned_be64(dec->cursor);
		dec->limit_ptr = dec->cursor + buf_size - sizeof(dec->bit_container);
	} else {
		dec->bits_consumed = (unsigned int)(sizeof(dec->bit_container) - buf_size)*8;

		dec->bit_container = (uint64_t)(((const uint8_t*)(buf))[0]) << 56;
		switch(buf_size) {
		case 7:
			dec->bit_container += (uint64_t)(((const uint8_t*)(buf))[6]) <<  8;
			/* fall-through */
		case 6:
			dec->bit_container += (uint64_t)(((const uint8_t*)(buf))[5]) << 16;
			/* fall-through */
		case 5:
			dec->bit_container += (uint64_t)(((const uint8_t*)(buf))[4]) << 24;
			/* fall-through */
		case 4:
			dec->bit_container += (uint64_t)(((const uint8_t*)(buf))[3]) << 32;
			/* fall-through */
		case 3:
			dec->bit_container += (uint64_t)(((const uint8_t*)(buf))[2]) << 40;
			/* fall-through */
		case 2:
			dec->bit_container += (uint64_t)(((const uint8_t*)(buf))[1]) << 48;
			/* fall-through */
		default:
			break;
		}
		dec->bit_container >>= dec->bits_consumed;

		dec->limit_ptr = dec->cursor;
	}

	return buf_size;
}


static __inline uint64_t bit_peek_bits(const struct bit_decoder *dec, unsigned int nb_bits)
{
	/* mask for the shift value register to prevent undefined behavior */
	uint32_t const reg_mask = 0x3F;

	assert(nb_bits >= 1 && nb_bits <= (64 - 7)); /* TODO: why -7 */
	assert(dec->bits_consumed + nb_bits <= 64);

	/* shift out consumed bits; return the top nb_bits bits we want to peek */
	return (dec->bit_container << (dec->bits_consumed &reg_mask)) >> (64-nb_bits);
}


/**
 * @brief count the leading ones in the local register; local register is not modified
 * @warning if all bits are consumed in local register (bitD->bitsConsumed  >= 64),
 *	the result is undefined
 * @param dec	a pointer to a bit_DStream_t context
 * @returns number of leading ones; up to maximum 63
 */

static __inline unsigned int bit_count_leading_ones(const struct bit_decoder* dec)
{
	/* mask for the shift value register to prevent undefined behavior */
	uint32_t const reg_mask = 0x3F;
	/* shift out the bits we've already consumed */
	uint64_t remaining_flip = ~(dec->bit_container << (dec->bits_consumed & reg_mask));

	/* clzll(0) is undefined behavior */
	if (remaining_flip)
		return sizeof(dec->bit_container)*8;

	return (unsigned int)__builtin_clzll(remaining_flip);
}


static __inline void bit_consume_bits(struct bit_decoder *dec, unsigned int nb_bits)
{
	dec->bits_consumed += nb_bits;
}


static __inline uint64_t bit_read_bits(struct bit_decoder *dec, unsigned int nb_bits)
{
	uint64_t const read_bits = bit_peek_bits(dec, nb_bits);

	bit_consume_bits(dec, nb_bits);
	return read_bits;
}


/**
 * @brief Check if the end of the bitstream has been reached
 * @param dec	a bitstream decoding context
 * @returns 1 if DStream has _exactly_ reached its end (all bits consumed).
 */

static __inline unsigned int bit_end_of_stream(const struct bit_decoder* dec)
{
    return ((dec->cursor == dec->limit_ptr) &&
	    (dec->bits_consumed == sizeof(dec->bit_container)*8));
}


enum {BIT_OVERFLOW, BIT_END_OF_BUFFER, BIT_ALL_READ_IN, BIT_UNFINISHED};

static __inline int bit_refill(struct bit_decoder *dec)
{
	if (dec->bits_consumed > (sizeof(dec->bit_container)*8))
		return BIT_OVERFLOW;
	if (dec->cursor < dec->limit_ptr) {
		/* Advance the pointer by the number of full bytes we consumed */
		dec->cursor += dec->bits_consumed >> 3;
		/* Refill the bit container */
		dec->bit_container = bit_read_unalingned_be64(dec->cursor);
		/* The number of bits that we have already consumed in the current
		 * byte, excluding the bits that formed a complete byte and were already
		 * processed.
		 */
		dec->bits_consumed &= 0x7;
		return BIT_UNFINISHED;
	}

	if (bit_end_of_stream(dec))
		return BIT_ALL_READ_IN;
	else
		return BIT_END_OF_BUFFER;

	/* limit_ptr < cursor < end */
	dec->bits_consumed -= (dec->limit_ptr - dec->cursor)*8;
	dec->cursor = dec->limit_ptr;
	dec->bit_container = bit_read_unaligned_64(dec->cursor);

	return BIT_END_OF_BUFFER;
}


#endif /* READ_BITSTREAM_H */
