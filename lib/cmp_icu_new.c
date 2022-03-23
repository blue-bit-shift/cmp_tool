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

/* pointer to a code word generation function */
typedef uint32_t (*generate_cw_pt)(uint32_t value, uint32_t encoder_par1,
				   uint32_t encoder_par2, uint32_t *cw);

/* typedef uint32_t (*next_model_f_pt)(uint32_t model_mode_val, uint32_t diff_model_var); */

struct encoder_setupt {
	/* unsigned int lossy_par; */
	/* next_model_f_pt *next_model_f; */
	generate_cw_pt generate_cw; /* pointer to the code word generation function */
	/* uint32_t updated_model; */
	uint32_t encoder_par1;
	uint32_t encoder_par2;
	uint32_t outlier_par;
	uint32_t model_value;
	uint32_t max_value_bits; /* how many bits are needed to represent the highest possible value */
	uint32_t max_bit_len; /* maximum length of the bitstream/icu_output_buf in bits */
	uint32_t *bitstream_adr;
};


/**
 * @brief map a signed value into a positive value range
 *
 * @param value_to_map		signed value to map
 * @param max_value_bits	how many bits are needed to represent the
 *				highest possible value
 *
 * @returns the positive mapped value
 */

static uint32_t map_to_pos(uint32_t value_to_map, unsigned int max_value_bits)
{
	uint32_t mask = (~0U >> (32 - max_value_bits)); /* mask the used bits */

	value_to_map &= mask;
	if (value_to_map >> (max_value_bits - 1)) { /* check the leading signed bit */
		value_to_map |= ~mask; /* convert to 32-bit signed integer */
		/* map negative values to uneven numbers */
		return (-value_to_map) * 2 - 1; /* possible integer overflow is intended */
	} else {
		/* map positive values to even numbers */
		return value_to_map * 2; /* possible integer overflow is intended */
	}
}


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
		return stream_len;

	if (n_bits > 32)
		return -1;

	/* Do we need to write data to the bitstream? */
	if (!bitstream_adr)
		return stream_len;

	/* Check if bitstream buffer is large enough */
	if ((unsigned int)stream_len > max_bit_len) {
		debug_print("Error: The buffer for the compressed data is too small to hold the compressed data. Try a larger buffer_length parameter.\n");
		return CMP_ERROR_SAMLL_BUF;
	}

	/* (M) is the n_bits parameter large enough to cover all value bits; the
	 * calculations can be re-used in the unsegmented code, so we have no overhead
	 */
	shiftRight = 32 - n_bits;
	mask = 0xFFFFFFFFU >> shiftRight;
	value &= mask;

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


/**
 * @brief forms the codeword according to the Rice code
 *
 * @param value		value to be encoded
 * @param m		Golomb parameter, only m's which are power of 2 are allowed
 * @param log2_m	Rice parameter, is log_2(m) calculate outside function
 *			for better performance
 * @param cw		address were the encode code word is stored
 *
 * @returns the length of the formed code word in bits
 * @note no check if the generated code word is not longer than 32 bits.
 */

static uint32_t Rice_encoder(uint32_t value, uint32_t m, uint32_t log2_m,
				 uint32_t *cw)
{
	uint32_t g;  /* quotient of value/m */
	uint32_t q;  /* quotient code without ending zero */
	uint32_t r;  /* remainder of value/m */
	uint32_t rl; /* remainder length */

	g = value >> log2_m; /* quotient, number of leading bits */
	q = (1U << g) - 1;   /* prepare the quotient code without ending zero */

	r = value & (m-1);   /* calculate the remainder */
	rl = log2_m + 1;     /* length of the remainder (+1 for the 0 in the quotient code) */
	*cw = (q << rl) | r; /* put the quotient and remainder code together */
	/*
	 * NOTE: If log2_m = 31 -> rl = 32, (q << rl) leads to an undefined
	 * behavior. However, in this case, a valid code with a maximum of 32
	 * bits can only be formed if q = 0. Any shift with 0 << x always
	 * results in 0, which forms the correct codeword in this case. For
	 * performance reasons, this undefined behaviour is not caught.
	 */

	return rl + g;	      /* calculate the length of the code word */
}


/**
 * @brief forms a codeword according to the Golomb code
 *
 * @param value		value to be encoded
 * @param m		Golomb parameter (have to be bigger than 0)
 * @param log2_m	is log_2(m) calculate outside function for better
 *			performance
 * @param cw		address were the formed code word is stored
 *
 * @returns the length of the formed code word in bits
 */

static uint32_t Golomb_encoder(uint32_t value, uint32_t m, uint32_t log2_m,
				   uint32_t *cw)
{
	uint32_t len0, b, g, q, lg;
	uint32_t len;
	uint32_t cutoff;

	len0 = log2_m + 1;                 /* codeword length in group 0 */
	cutoff = (1U << (log2_m + 1)) - m; /* members in group 0 */

	if (value < cutoff) { /* group 0 */
		*cw = value;
		len = len0;
	} else { /* other groups */
		g = (value-cutoff) / m; /* this group is which one  */
		b = cutoff << 1;        /* form the base codeword */
		lg = len0 + g;          /* it has lg remainder bits */
		q = (1U << g) - 1;      /* prepare the left side in unary */
		*cw = (q << (len0+1)) + b + (value-cutoff) - g*m; /* composed codeword */
		len = lg + 1;           /* length of the codeword */
	}
	return len;
}


/**
 * @brief generate a code word without a outlier mechanism and put in the
 *	bitstream
 *
 * @param value		value to encode in the bitstream
 * @param stream_len	length of the bitstream in bits
 * @param setup		pointer to the encoder setup
 *
 * @returns the bit length of the bitstream with the added encoded value on
 *	success; negative on error
 */

static int encode_normal(uint32_t value, int stream_len,
			 struct encoder_setupt *setup)
{
	uint32_t code_word, cw_len;

	cw_len = setup->generate_cw(value, setup->encoder_par1,
				    setup->encoder_par2, &code_word);

	return put_n_bits32(code_word, cw_len, stream_len, setup->bitstream_adr,
			    setup->max_bit_len);
}


/**
 * @brief subtract the model from the data, encode the result and put it into
 *	bitstream, for encodeing outlier use the zero escape symbol mechanism
 *
 * @param data		data to encode
 * @param model		model of the data (0 if not used)
 * @param stream_len	length of the bitstream in bits
 * @param setup		pointer to the encoder setup
 *
 * @returns the bit length of the bitstream with the added encoded value on
 *	success; negative on error, CMP_ERROR_SAMLL_BUF if the bitstream buffer
 *	is too small to put the value in the bitstream
 *
 * @note no check if data or model are in the allowed range
 * @note no check if the setup->outlier_par is in the allowed range
 */

static int encode_value_zero(uint32_t data, uint32_t model, int stream_len,
			     struct encoder_setupt *setup)
{
	data -= model;

	data = map_to_pos(data, setup->max_value_bits);

	/* For performance reasons, we check to see if there is an outlier
	 * before adding one, rather than the other way around:
	 * data++;
	 * if (data < setup->outlier_par && data != 0)
	 *	return ...
	 */
	if (data < (setup->outlier_par - 1)) { /* detect r */
		data++; /* add 1 to every value so we can use 0 as escape symbol */
		return encode_normal(data, stream_len, setup);
	}

	data++; /* add 1 to every value so we can use 0 as escape symbol */

	/* use zero as escape symbol */
	stream_len = encode_normal(0, stream_len, setup);
	if (stream_len <= 0)
		return stream_len;

	/* put the data unencoded in the bitstream */
	stream_len = put_n_bits32(data, setup->max_value_bits, stream_len,
				  setup->bitstream_adr, setup->max_bit_len);

	return stream_len;

}


/**
 * @brief subtract the model from the data, encode the result and put it into
 *	bitstream, for encoding outlier use the multi escape symbol mechanism
 *
 * @param data		data to encode
 * @param model		model of the data (0 if not used)
 * @param stream_len	length of the bitstream in bits
 * @param setup		pointer to the encoder setup
 *
 * @returns the bit length of the bitstream with the added encoded value on
 *	success; negative on error, CMP_ERROR_SAMLL_BUF if the bitstream buffer
 *	is too small to put the value in the bitstream
 *
 * @note no check if data or model are in the allowed range
 * @note no check if the setup->outlier_par is in the allowed ragne
 */

static int encode_value_multi(uint32_t data, uint32_t model, int stream_len,
			      struct encoder_setupt *setup)
{
	uint32_t unencoded_data;
	unsigned int unencoded_data_len;
	uint32_t escape_sym, escape_sym_offset;

	data -= model; /* possible underflow is intended */

	data = map_to_pos(data, setup->max_value_bits);

	if (data < setup->outlier_par) /* detect non-outlier */
		return  encode_normal(data, stream_len, setup);

	/*
	 * In this mode we put the difference between the data and the spillover
	 * threshold value (unencoded_data) after a encoded escape symbol, which
	 * indicate that the next codeword is unencoded.
	 * We use different escape symbol depended on the size the needed bit of
	 * unencoded data:
	 * 0, 1, 2 bits needed for unencoded data -> escape symbol is outlier_par + 0
	 * 3, 4 bits needed for unencoded data -> escape symbol is outlier_par + 1
	 * 5, 6 bits needed for unencoded data -> escape symbol is outlier_par + 2
	 * and so on
	 */
	unencoded_data = data - setup->outlier_par;

	if (!unencoded_data) /* catch __builtin_clz(0) because the result is undefined.*/
		escape_sym_offset = 0;
	else
		escape_sym_offset = (31U - (uint32_t)__builtin_clz(unencoded_data)) >> 1;

	escape_sym = setup->outlier_par + escape_sym_offset;
	unencoded_data_len = (escape_sym_offset + 1U) << 1;

	/* put the escape symbol in the bitstream */
	stream_len = encode_normal(escape_sym, stream_len, setup);
	if (stream_len <= 0)
		return stream_len;

	/* put the unencoded data in the bitstream */
	stream_len = put_n_bits32(unencoded_data, unencoded_data_len, stream_len,
				  setup->bitstream_adr, setup->max_bit_len);

	return stream_len;
}
