/**
 * @file   cmp_io.c
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @author Johannes Seelig (johannes.seelig@univie.ac.at)
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
 * @brief compression tool input/output library
 * @warning this part of the software is not intended to run on-board on the ICU.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>


#include <cmp_tool-config.h>
#include <compiler.h>

#include "cmp_io.h"
#include <cmp_support.h>
#include <cmp_chunk.h>
#include <rdcu_cmd.h>
#include <byteorder.h>
#include <cmp_data_types.h>
#include <leon_inttypes.h>


#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
/* Redefine (f)printf to do nothing */
__extension__
#define printf(...) do {} while (0)
#define fprintf(...) do {} while (0)
#endif


/* directory to convert from data_type to string */
static const struct {
	enum cmp_data_type data_type;
	const char *str;
} data_type_string_table[] = {
	{DATA_TYPE_IMAGETTE, "DATA_TYPE_IMAGETTE"},
	{DATA_TYPE_IMAGETTE_ADAPTIVE, "DATA_TYPE_IMAGETTE_ADAPTIVE"},
	{DATA_TYPE_SAT_IMAGETTE, "DATA_TYPE_SAT_IMAGETTE"},
	{DATA_TYPE_SAT_IMAGETTE_ADAPTIVE, "DATA_TYPE_SAT_IMAGETTE_ADAPTIVE"},
	{DATA_TYPE_OFFSET, "DATA_TYPE_OFFSET"},
	{DATA_TYPE_BACKGROUND, "DATA_TYPE_BACKGROUND"},
	{DATA_TYPE_SMEARING, "DATA_TYPE_SMEARING"},
	{DATA_TYPE_S_FX, "DATA_TYPE_S_FX"},
	{DATA_TYPE_S_FX_EFX, "DATA_TYPE_S_FX_EFX"},
	{DATA_TYPE_S_FX_NCOB, "DATA_TYPE_S_FX_NCOB"},
	{DATA_TYPE_S_FX_EFX_NCOB_ECOB, "DATA_TYPE_S_FX_EFX_NCOB_ECOB"},
	{DATA_TYPE_L_FX, "DATA_TYPE_L_FX"},
	{DATA_TYPE_L_FX_EFX, "DATA_TYPE_L_FX_EFX"},
	{DATA_TYPE_L_FX_NCOB, "DATA_TYPE_L_FX_NCOB"},
	{DATA_TYPE_L_FX_EFX_NCOB_ECOB, "DATA_TYPE_L_FX_EFX_NCOB_ECOB"},
	{DATA_TYPE_F_FX, "DATA_TYPE_F_FX"},
	{DATA_TYPE_F_FX_EFX, "DATA_TYPE_F_FX_EFX"},
	{DATA_TYPE_F_FX_NCOB, "DATA_TYPE_F_FX_NCOB"},
	{DATA_TYPE_F_FX_EFX_NCOB_ECOB, "DATA_TYPE_F_FX_EFX_NCOB_ECOB"},
	{DATA_TYPE_F_CAM_IMAGETTE, "DATA_TYPE_F_CAM_IMAGETTE"},
	{DATA_TYPE_F_CAM_IMAGETTE_ADAPTIVE, "DATA_TYPE_F_CAM_IMAGETTE_ADAPTIVE"},
	{DATA_TYPE_F_CAM_OFFSET, "DATA_TYPE_F_CAM_OFFSET"},
	{DATA_TYPE_F_CAM_BACKGROUND, "DATA_TYPE_F_CAM_BACKGROUND"},
	{DATA_TYPE_CHUNK, "DATA_TYPE_CHUNK"},
	{DATA_TYPE_UNKNOWN, "DATA_TYPE_UNKNOWN"}
};


/**
 * @brief print help information
 *
 * @param program_name	name of the program
 */

void print_help(const char *program_name MAYBE_UNUSED)
{
	printf("usage: %s [options] [<argument>]\n", program_name);
	printf("General Options:\n");
	printf("  -h, --help               Print this help text and exit\n");
	printf("  -o <prefix>              Use the <prefix> for output files\n");
	printf("  -n, --model_cfg          Print a default model configuration and exit\n");
	printf("  --diff_cfg               Print a default 1d-differencing configuration and exit\n");
	printf("  -b, --binary             Read and write files in binary format\n");
	printf("  -a, --rdcu_par           Add additional RDCU control parameters\n");
	printf("  -V, --version            Print program version and exit\n");
	printf("  -v, -vv, --verbose       Print various debugging information, -vv is extra verbose\n");
	printf("Compression Options:\n");
	printf("  -c <file>                File containing the compressing configuration\n");
	printf("  -d <file>                File containing the data to be compressed\n");
	printf("  -m <file>                File containing the model of the data to be compressed\n");
	printf("  --no_header              Do not add a compression entity header in front of the compressed data\n");
	printf("  --rdcu_pkt               Generate RMAP packets for an RDCU compression\n");
	printf("  --last_info <.info file> Generate RMAP packets for an RDCU compression with parallel read of the last results\n");
	printf("Decompression Options:\n");
	printf("  -d <file>                File containing the compressed data\n");
	printf("  -m <file>                File containing the model of the compressed data\n");
	printf("  -i <file>                File containing the decompression information (required if --no_header was used)\n");
	printf("Guessing Options:\n");
	printf("  --guess <mode>           Search for a good configuration for compression <mode>\n");
	printf("  -d <file>                File containing the data to be compressed\n");
	printf("  -m <file>                File containing the model of the data to be compressed\n");
	printf("  --guess_level <level>    Set guess level to <level> (optional)\n");
}


/**
 * @brief opens a file with a name that is a concatenation of directory and file
 *	name.
 *
 * @param dirname	first string of concatenation
 * @param filename	security string of concatenation
 *
 * @return a pointer to the opened file
 *
 * @see https://developers.redhat.com/blog/2018/05/24/detecting-string-truncation-with-gcc-8/
 */

static FILE *open_file(const char *dirname, const char *filename)
{
	char *pathname;
	int n;

	if (!dirname)
		return NULL;

	if (!filename)
		return NULL;

	errno = 0;
	n = snprintf(0, 0, "%s%s", dirname, filename);

	if (n < 0) {
		perror("snprintf failed");
		abort();
	}

	errno = 0;
	pathname = (char *)alloca((size_t)n + 1);

	errno = 0;
	n = snprintf(pathname, (size_t)n + 1, "%s%s", dirname, filename);
	if (n < 0) {
		perror("snprintf failed");
		abort();
	}

	return fopen(pathname, "wb");
}


/**
 * @brief write uncompressed input data in big-endian to an output file
 *
 * @param data			the data to write a file
 * @param data_size		size of the data in bytes
 * @param cmp_type		RDCU or chunk compression?
 * @param output_prefix		file name without file extension
 * @param name_extension	extension (with leading point character)
 * @param flags			CMP_IO_VERBOSE_EXTRA	print verbose output if set
 *				CMP_IO_BINARY	write the file in binary format if set
 *
 * @returns 0 on success, error otherwise
 */

int write_input_data_to_file(const void *data, uint32_t data_size, enum cmp_type cmp_type,
			     const char *output_prefix, const char *name_extension, int flags)
{
	uint8_t *tmp_buf;
	int return_value;

	if (!data)
		abort();

	if (data_size == 0)
		return 0;

	tmp_buf = malloc(data_size);
	if (!tmp_buf)
		return -1;

	memcpy(tmp_buf, data, data_size);

	switch (cmp_type) {
	case CMP_TYPE_CHUNK:
		return_value = cpu_to_be_chunk(tmp_buf, data_size);
		break;
	case CMP_TYPE_RDCU:
		return_value = be_to_cpu_data_type(tmp_buf, data_size, DATA_TYPE_IMAGETTE);
		break;
	case CMP_TYPE_ERROR:
	default:
		return_value = -1;
		break;
	}

	if (!return_value)
		return_value = write_data_to_file(tmp_buf, data_size, output_prefix,
						  name_extension, flags);

	free(tmp_buf);

	return return_value;
}


/**
 * @brief write a buffer to an output file
 *
 * @param buf		the buffer to write a file
 * @param buf_size	size of the buffer in bytes
 *
 * @param output_prefix  file name without file extension
 * @param name_extension file extension (with leading point character)
 *
 * @param flags		CMP_IO_VERBOSE_EXTRA	print verbose output if set
 *			CMP_IO_BINARY	write the file in binary format if set
 *
 * @returns 0 on success, error otherwise
 */

int write_data_to_file(const void *buf, uint32_t buf_size, const char *output_prefix,
		       const char *name_extension, int flags)
{
	FILE *fp;
	const uint8_t *p = (const uint8_t *)buf;
	const uint8_t *output_file_data;
	uint8_t *tmp_buf = NULL;
	size_t output_file_size;

	if (!buf)
		abort();

	if (buf_size == 0)
		return 0;

	fp = open_file(output_prefix, name_extension);
	if (fp == NULL) {
		fprintf(stderr, "%s: %s%s: %s\n", PROGRAM_NAME, output_prefix,
			name_extension, strerror(errno));
		return -1;
	}

	if (flags & CMP_IO_BINARY) {
		output_file_size = buf_size;
		output_file_data = buf;
	} else {
		size_t i, j;
		const uint8_t lut[0x10] = "0123456789abcdef";

		/* convert data to ASCII */
		output_file_size = buf_size*3 + 1;
		tmp_buf = malloc(output_file_size);
		if (!tmp_buf) {
			fclose(fp);
			return -1;
		}

		for (i = 0, j = 0; i < buf_size; i++) {
			tmp_buf[j++] = lut[(p[i] >> 4)];
			tmp_buf[j++] = lut[(p[i] & 0xF)];

			if ((i + 1) % 16 == 0)
				tmp_buf[j++] = '\n';
			else
				tmp_buf[j++] = ' ';
		}
		tmp_buf[j] = '\n';
		output_file_data = tmp_buf;
	}
	{
		size_t const size_check = fwrite(output_file_data, sizeof(uint8_t),
						 output_file_size, fp);
		fclose(fp);
		if (size_check != output_file_size) {
			free(tmp_buf);
			return -1;
		}
	}

	if (flags & CMP_IO_VERBOSE_EXTRA && !(flags & CMP_IO_BINARY)) {
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
		printf("\n");
		fwrite(output_file_data, 1, output_file_size, stdout);
		printf("\n");
#endif
	}

	free(tmp_buf);
	return 0;
}


/**
 * @brief remove white spaces and tabs
 *
 * @param s input string
 */

static void remove_spaces(char *s)
{
	const char *d = s;

	if (!s)
		return;

	do {
		while (isspace(*d))
			d++;
	} while ((*s++ = *d++) != '\0');
}


/**
 * @brief remove comments (starting with # or /) and \n
 *
 * @param s input string
 */

static void remove_comments(char *s)
{
	if (!s)
		return;

	while (*s) {
		if (*s == '#' || *s == '/' || *s == '\n') {
			*s = '\0';
			break;
		}
		s++;
	}

}


/**
 * @brief convert RDCU SRAM Address string to integer
 *
 * @param addr	string of the address
 *
 * @returns the value of the address, < 0 on error
 */

static int sram_addr_to_int(const char *addr)
{
	unsigned long i;
	char *end = NULL;

	if (addr == NULL)
		return -1;

	i = strtoul(addr, &end, 0);

	if (end == addr || errno == ERANGE) {
		printf("range error, got\n");
		errno = 0;
		return -1;
	}

	if (i > RDCU_SRAM_END) {
		printf("%s: The SRAM address is out of the rdcu range\n",
		       PROGRAM_NAME);
		return -1;
	}

	if (i & 0x3) {
		printf("The SRAM address is not 32 bit aligned\n");
		return -1;
	}

	return (int) i;
}


/**
 * @brief Interprets an uint32_t integer value in a byte string
 *
 * @param dep_str	description string of the read in value
 * @param val_str	value string contains the value to convert in uint32_t
 * @param red_val	address for storing the converted result
 *
 * @returns 0 on success, error otherwise
 *
 * @see https://eternallyconfuzzled.com/atoi-is-evil-c-learn-why-atoi-is-awful
 */

int atoui32(const char *dep_str, const char *val_str, uint32_t *red_val)
{
	unsigned long temp;
	char *end = NULL;

	if (!dep_str)
		return -1;

	if (!val_str)
		return -1;

	if (!red_val)
		return -1;

	errno = 0;
	temp = strtoul(val_str, &end, 0);
	if (end != val_str && errno != ERANGE && temp <= UINT32_MAX) {
		*red_val = (uint32_t)temp;
	} else {
		fprintf(stderr, "%s: Error read in %s.\n", PROGRAM_NAME, dep_str);
		*red_val = 0;
		return -1;
	}
	return 0;
}


/**
 * @brief parse a compression data_type string to a data_type
 * @note string can be either a number or the name of the compression data type
 *
 * @param data_type_str	string containing the compression data type to parse
 *
 * @returns data type on success, DATA_TYPE_UNKNOWN on error
 */

enum cmp_data_type string2data_type(const char *data_type_str)
{
	enum cmp_data_type data_type = DATA_TYPE_UNKNOWN;

	if (data_type_str) {
		if (isalpha(data_type_str[0])) {  /* check if mode is given as text */
			size_t j;

			for (j = 0; j < ARRAY_SIZE(data_type_string_table); j++) {
				if (!strcmp(data_type_str, data_type_string_table[j].str)) {
					data_type = data_type_string_table[j].data_type;
					break;
				}
			}
		} else {
			uint32_t read_val;

			if (!atoui32("Compression Data Type", data_type_str, &read_val)) {
				data_type = read_val;
				if (cmp_data_type_is_invalid(data_type))
					data_type = DATA_TYPE_UNKNOWN;
			}
		}
	}
	return data_type;
}

/**
 * @brief parse a compression data_type string to a data_type
 * @note string can be either a number or the name of the compression data type
 *
 * @param data_type compression data type to convert in string
 *
 * @returns data type on success, DATA_TYPE_UNKNOWN on error
 */

const char *data_type2string(enum cmp_data_type data_type)
{
	size_t j;
	const char *string = "DATA_TYPE_UNKNOWN";

	for (j = 0; j < ARRAY_SIZE(data_type_string_table); j++) {
		if (data_type == data_type_string_table[j].data_type) {
			string = data_type_string_table[j].str;
			break;
		}
	}

	return string;
}


/**
 * @brief compares two strings case-insensitively
 *
 * @param s1	the first string to compare
 * @param s2	the second string to compare
 *
 * @returns an integer greater than, equal to, or less than 0, according as s1
 *	is lexicographically greater than, equal to, or less than s2 after
 *	translation of each corresponding character to lower-case.  The strings
 *	themselves are not modified.
 */

int case_insensitive_compare(const char *s1, const char *s2)
{
	size_t i;

	if(s1 == NULL)
		abort();

	if(s2 == NULL)
		abort();

	for (i = 0;  ; ++i) {
		unsigned int x1 = (unsigned char)s1[i];
		unsigned int x2 = (unsigned char)s2[i];
		int r;

		if (x1 - 'A' < 26U)
			x1 += 32; /* tolower */
		if (x2 - 'A' < 26U)
			x2 += 32; /* tolower */

		r = (int)x1 - (int)x2;
		if (r)
			return r;

		if (!x1)
			return 0;
	}
}


/**
 * @brief parse a compression mode value string to an integer
 * @note string can be either a number or the name of the compression mode
 *
 * @param cmp_mode_str	string containing the compression mode value to parse
 * @param cmp_mode	address where the parsed result is written
 *
 * @returns 0 on success, error otherwise
 */

int cmp_mode_parse(const char *cmp_mode_str, enum cmp_mode *cmp_mode)
{
	static const struct {
		uint32_t cmp_mode;
		const char *str;
	} conversion[] = {
		{CMP_MODE_RAW, "MODE_RAW"},
		{CMP_MODE_MODEL_ZERO, "MODE_MODEL_ZERO"},
		{CMP_MODE_DIFF_ZERO, "MODE_DIFF_ZERO"},
		{CMP_MODE_MODEL_MULTI, "MODE_MODEL_MULTI"},
		{CMP_MODE_DIFF_MULTI, "MODE_DIFF_MULTI"},
		{CMP_MODE_RAW, "CMP_MODE_RAW"},
		{CMP_MODE_MODEL_ZERO, "CMP_MODE_MODEL_ZERO"},
		{CMP_MODE_DIFF_ZERO, "CMP_MODE_DIFF_ZERO"},
		{CMP_MODE_MODEL_MULTI, "CMP_MODE_MODEL_MULTI"},
		{CMP_MODE_DIFF_MULTI, "CMP_MODE_DIFF_MULTI"},
	};

	if (!cmp_mode_str)
		return -1;
	if (!cmp_mode)
		return -1;

	if (isalpha(cmp_mode_str[0])) {  /* check if mode is given as text */
		size_t j;

		for (j = 0; j < ARRAY_SIZE(conversion); j++) {
			if (!case_insensitive_compare(cmp_mode_str, conversion[j].str)) {
				*cmp_mode = conversion[j].cmp_mode;
				return 0;
			}
		}
		return -1;
	}
	{
		uint32_t read_val;

		if (atoui32(cmp_mode_str, cmp_mode_str, &read_val))
			return -1;
		*cmp_mode = read_val;
	}

	if (!cmp_mode_is_supported(*cmp_mode))
		return -1;

	return 0;
}


/**
 * @brief parse a file containing a compressing configuration
 * @note internal use only!
 *
 * @param fp	FILE pointer
 * @param rcfg	RDCU compression configuration structure holding the read in
 *		configuration
 * @param par	chunk compression parameters structure holding the read in
 *		chunk configuration
 *
 * @returns CMP_TYPE_CHUNK if a chunk configuration is detected (stored in par),
 *	CMP_TYPE_RDCU if an RDCU configuration is detected (stored in cfg) or
 *	CMP_TYPE_ERROR on error
 */

static enum cmp_type parse_cfg(FILE *fp, struct rdcu_cfg *rcfg, struct cmp_par *par)
{
#define chunk_parse_uint32_parameter(parameter)			\
	if (!strcmp(token1, #parameter)) {			\
		if (atoui32(token1, token2, &par->parameter))	\
			return CMP_TYPE_ERROR;			\
		cmp_type = CMP_TYPE_CHUNK;			\
		continue;					\
}
	enum cmp_type cmp_type = CMP_TYPE_RDCU;
	char *token1, *token2;
	char line[MAX_CONFIG_LINE];
	enum {CMP_MODE, SAMPLES, BUFFER_LENGTH, LAST_ITEM};
	int j, must_read_items[LAST_ITEM] = {0};

	if (!fp)
		abort();
	if (!rcfg)
		abort();
	if (!par)
		abort();

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (line[0] == '#' || line[0] == '\n')
			continue; /* skip #'ed or empty lines */

		if (!strchr(line, '\n')) { /* detect a to long line */
			fprintf(stderr, "%s: Error read in line to long. Maximal line length is %d characters.\n",
				PROGRAM_NAME, MAX_CONFIG_LINE-1);
			return CMP_TYPE_ERROR;
		}

		remove_comments(line);
		remove_spaces(line);

		token1 = strtok(line, "=");
		token2 = strtok(NULL, "=");
		if (token1 == NULL)
			continue;
		if (token2 == NULL)
			continue;


		if (!strcmp(token1, "cmp_mode")) {
			must_read_items[CMP_MODE] = 1;
			if (cmp_mode_parse(token2, &rcfg->cmp_mode))
				return CMP_TYPE_ERROR;
			par->cmp_mode = rcfg->cmp_mode;
			continue;
		}
		if (!strcmp(token1, "golomb_par")) {
			if (atoui32(token1, token2, &rcfg->golomb_par))
				return CMP_TYPE_ERROR;
			continue;
		}
		if (!strcmp(token1, "spill")) {
			if (atoui32(token1, token2, &rcfg->spill))
				return CMP_TYPE_ERROR;
			continue;
		}
		if (!strcmp(token1, "model_value")) {
			if (atoui32(token1, token2, &rcfg->model_value))
				return CMP_TYPE_ERROR;
			par->model_value = rcfg->model_value;
			continue;
		}
		if (!strcmp(token1, "round")) {
			if (atoui32(token1, token2, &rcfg->round))
				return CMP_TYPE_ERROR;
			continue;
		}
		if (!strcmp(token1, "ap1_golomb_par")) {
			if (atoui32(token1, token2, &rcfg->ap1_golomb_par))
				return CMP_TYPE_ERROR;
			continue;
		}
		if (!strcmp(token1, "ap1_spill")) {
			if (atoui32(token1, token2, &rcfg->ap1_spill))
				return CMP_TYPE_ERROR;
			continue;
		}
		if (!strcmp(token1, "ap2_golomb_par")) {
			if (atoui32(token1, token2, &rcfg->ap2_golomb_par))
				return CMP_TYPE_ERROR;
			continue;
		}
		if (!strcmp(token1, "ap2_spill")) {
			if (atoui32(token1, token2, &rcfg->ap2_spill))
				return CMP_TYPE_ERROR;
			continue;
		}

		if (!strcmp(token1, "rdcu_data_adr")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				printf("%s: Error read in rdcu_data_adr_par\n",
				       PROGRAM_NAME);
				return CMP_TYPE_ERROR;
			}
			rcfg->rdcu_data_adr = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "rdcu_model_adr")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				printf("%s: Error read in rdcu_model_adr_par\n",
				       PROGRAM_NAME);
				return CMP_TYPE_ERROR;
			}
			rcfg->rdcu_model_adr = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "rdcu_new_model_adr")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				printf("%s: Error read in rdcu_new_model_adr_par\n",
				       PROGRAM_NAME);
				return CMP_TYPE_ERROR;
			}
			rcfg->rdcu_new_model_adr = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "samples")) {
			if (atoui32(token1, token2, &rcfg->samples))
				return CMP_TYPE_ERROR;
			must_read_items[SAMPLES] = 1;
			continue;
		}
		if (!strcmp(token1, "rdcu_buffer_adr")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				printf("%s: Error read in rdcu_buffer_adr_par\n",
				       PROGRAM_NAME);
				return CMP_TYPE_ERROR;
			}
			rcfg->rdcu_buffer_adr = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "buffer_length")) {
			if (atoui32(token1, token2, &rcfg->buffer_length))
				return CMP_TYPE_ERROR;
			must_read_items[BUFFER_LENGTH] = 1;
			continue;
		}

		/* chunk_parse_uint32_parameter(model_value); */
		chunk_parse_uint32_parameter(lossy_par);

		chunk_parse_uint32_parameter(nc_imagette);

		chunk_parse_uint32_parameter(s_exp_flags);
		chunk_parse_uint32_parameter(s_fx);
		chunk_parse_uint32_parameter(s_ncob);
		chunk_parse_uint32_parameter(s_efx);
		chunk_parse_uint32_parameter(s_ecob);

		chunk_parse_uint32_parameter(l_exp_flags);
		chunk_parse_uint32_parameter(l_fx);
		chunk_parse_uint32_parameter(l_ncob);
		chunk_parse_uint32_parameter(l_efx);
		chunk_parse_uint32_parameter(l_ecob);
		chunk_parse_uint32_parameter(l_fx_cob_variance);

		chunk_parse_uint32_parameter(saturated_imagette);

		chunk_parse_uint32_parameter(nc_offset_mean);
		chunk_parse_uint32_parameter(nc_offset_variance);
		chunk_parse_uint32_parameter(nc_background_mean);
		chunk_parse_uint32_parameter(nc_background_variance);
		chunk_parse_uint32_parameter(nc_background_outlier_pixels);

		chunk_parse_uint32_parameter(smearing_mean);
		chunk_parse_uint32_parameter(smearing_variance_mean);
		chunk_parse_uint32_parameter(smearing_outlier_pixels);

		chunk_parse_uint32_parameter(fc_imagette);
		chunk_parse_uint32_parameter(fc_offset_mean);
		chunk_parse_uint32_parameter(fc_offset_variance);
		chunk_parse_uint32_parameter(fc_background_mean);
		chunk_parse_uint32_parameter(fc_background_variance);
		chunk_parse_uint32_parameter(fc_background_outlier_pixels);
	}

	if (cmp_type == CMP_TYPE_RDCU) {
		if (raw_mode_is_used(rcfg->cmp_mode))
			if (must_read_items[CMP_MODE] == 1 &&
			    must_read_items[BUFFER_LENGTH] == 1)
				return cmp_type;

		for (j = 0; j < LAST_ITEM; j++) {
			if (must_read_items[j] < 1) {
				fprintf(stderr, "%s: Some parameters are missing. Check if the following parameters: cmp_mode, golomb_par, spill, samples and buffer_length are all set in the configuration file.\n",
					PROGRAM_NAME);
				return CMP_TYPE_ERROR;
			}
		}
	}

	return cmp_type;
}


/**
 * @brief reading a compressor configuration file
 *
 * @param file_name	file containing the compression configuration file
 * @param rcfg		RDCU compression configuration structure holding the read in
 *			configuration
 * @param par		chunk compression parameters structure holding the read
 *			in chunk configuration
 * @param verbose_en	print verbose output if not zero
 *
 * @returns CMP_TYPE_CHUNK if a chunk configuration is detected (stored in par),
 *	CMP_TYPE_RDCU if an RDCU configuration is detected (stored in cfg) or
 *	CMP_TYPE_ERROR on error
 */

enum cmp_type cmp_cfg_read(const char *file_name, struct rdcu_cfg *rcfg,
			   struct cmp_par *par, int verbose_en)
{
	enum cmp_type cmp_type;
	FILE *fp;

	if (!file_name)
		abort();

	if (!rcfg)
		abort();

	if (strstr(file_name, ".info")) {
		fprintf(stderr, "%s: %s: .info file extension found on configuration file. You may have selected the wrong file.\n",
			PROGRAM_NAME, file_name);
	}

	fp = fopen(file_name, "r");

	if (fp == NULL) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, file_name,
			strerror(errno));
		return -1;
	}

	cmp_type = parse_cfg(fp, rcfg, par);
	fclose(fp);

	if (verbose_en && cmp_type == CMP_TYPE_RDCU) {
		printf("\n\n");
		cmp_cfg_print(rcfg, 1);
		printf("\n");
	}

	return cmp_type;
}


/**
 * @brief parse a file containing a decompression information
 * @note internal use only!
 *
 * @param fp	FILE pointer
 * @param info	decompression information structure holding the read in
 *		information
 *
 * @returns 0 on success, error otherwise
 */

static int parse_info(FILE *fp, struct cmp_info *info)
{
	char *token1, *token2;
	char line[MAX_CONFIG_LINE];
	enum {CMP_MODE_USED, GOLOMB_PAR_USED, SPILL_USED, SAMPLES_USED,
		CMP_SIZE, LAST_ITEM};
	int j, must_read_items[LAST_ITEM] = {0};

	if (!fp)
		return -1;

	if (!info)
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (line[0] == '#' || line[0] == '\n')
			continue; /* skip #'ed or empty lines */

		if (!strchr(line, '\n')) { /* detect a to long line */
			fprintf(stderr, "%s: Error read in line to long. Maximal line length is %d characters.\n",
				PROGRAM_NAME, MAX_CONFIG_LINE-1);
			return -1;
		}

		remove_comments(line);
		remove_spaces(line);

		/* Token will point to the part before the search-var. */
		token1 = strtok(line, "=");
		token2 = strtok(NULL, "=");
		if (token1 == NULL)
			continue;
		if (token2 == NULL)
			continue;


		if (!strcmp(token1, "cmp_mode_used")) {
			must_read_items[CMP_MODE_USED] = 1;
			if (isalpha(*token2)) { /* check if mode is given as text or val*/
				/* TODO: use conversion function for this: */
				if (!strcmp(token2, "MODE_RAW")) {
					info->cmp_mode_used = 0;
					continue;
				}
				if (!strcmp(token2, "MODE_MODEL_ZERO")) {
					info->cmp_mode_used = 1;
					continue;
				}
				if (!strcmp(token2, "MODE_DIFF_ZERO")) {
					info->cmp_mode_used = 2;
					continue;
				}
				if (!strcmp(token2, "MODE_MODEL_MULTI")) {
					info->cmp_mode_used = 3;
					continue;
				}
				if (!strcmp(token2, "MODE_DIFF_MULTI")) {
					info->cmp_mode_used = 4;
					continue;
				}
				fprintf(stderr, "%s: Error read in cmp_mode_used.\n",
					PROGRAM_NAME);
				return -1;
			}
			{
				uint32_t tmp;

				if (atoui32(token1, token2, &tmp))
					return -1;
				info->cmp_mode_used = tmp;
			}
			continue;
		}
		if (!strcmp(token1, "model_value_used")) {
			uint32_t tmp;

			if (atoui32(token1, token2, &tmp))
				return -1;
			if (tmp <= UINT8_MAX) {
				info->model_value_used = (uint8_t)tmp;
			} else {
				fprintf(stderr, "%s: Error read in %s.\n",
					PROGRAM_NAME, token1);
				return -1;
			}
			continue;
		}
		if (!strcmp(token1, "round_used")) {
			uint32_t tmp;

			if (atoui32(token1, token2, &tmp))
				return -1;
			if (tmp <= 0xF) { /* max value of 4 bits */
				info->round_used = (uint8_t)tmp;
			} else {
				fprintf(stderr, "%s: Error read in %s.\n",
					PROGRAM_NAME, token1);
				return -1;
			}
			continue;
		}
		if (!strcmp(token1, "spill_used")) {
			uint32_t tmp;

			if (atoui32(token1, token2, &tmp))
				return -1;
			info->spill_used = (unsigned char)tmp;
			must_read_items[SPILL_USED] = 1;
			continue;
		}
		if (!strcmp(token1, "golomb_par_used")) {
			uint32_t tmp;

			if (atoui32(token1, token2, &tmp))
				return -1;
			info->golomb_par_used = tmp;
			must_read_items[GOLOMB_PAR_USED] = 1;
			continue;
		}
		if (!strcmp(token1, "samples_used")) {
			if (atoui32(token1, token2, &info->samples_used))
				return -1;
			must_read_items[SAMPLES_USED] = 1;
			continue;
		}
		if (!strcmp(token1, "cmp_size")) {
			if (atoui32(token1, token2, &info->cmp_size))
				return -1;
			must_read_items[CMP_SIZE] = 1;
			continue;
		}

		if (!strcmp(token1, "ap1_cmp_size")) {
			if (atoui32(token1, token2, &info->ap1_cmp_size))
				return -1;
			continue;
		}
		if (!strcmp(token1, "ap2_cmp_size")) {
			if (atoui32(token1, token2, &info->ap2_cmp_size))
				return -1;
			continue;
		}

		if (!strcmp(token1, "rdcu_new_model_adr_used")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				fprintf(stderr, "%s: Error read in rdcu_new_model_adr_used\n",
				       PROGRAM_NAME);
				return -1;
			}
			info->rdcu_new_model_adr_used = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "rdcu_cmp_adr_used")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				fprintf(stderr, "%s: Error read in rdcu_cmp_adr_used\n",
				       PROGRAM_NAME);
				return -1;
			}
			info->rdcu_cmp_adr_used = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "cmp_err")) {
			uint32_t tmp;

			if (atoui32(token1, token2, &tmp))
				return -1;
			if (tmp <= UINT16_MAX) {
				info->cmp_err = (uint16_t)tmp;
			} else {
				fprintf(stderr, "%s: Error read in %s.\n",
					PROGRAM_NAME, token1);
				return -1;
			}
			continue;
		}
	}

	if (raw_mode_is_used(info->cmp_mode_used))
		if (must_read_items[CMP_MODE_USED] == 1 &&
		    must_read_items[SAMPLES_USED] == 1 &&
		    must_read_items[CMP_SIZE] == 1)
			return 0;

	for (j = 0; j < LAST_ITEM; j++) {
		if (must_read_items[j] < 1) {
			fprintf(stderr, "%s: Some parameters are missing. Check if the following parameters: cmp_mode_used, golomb_par_used, spill_used and samples_used are all set in the information file.\n",
				PROGRAM_NAME);
			return -1;
		}
	}

	return 0;
}


/**
 * @brief reading a compressor decompression information file
 *
 * @param file_name	file containing the compression configuration file
 * @param info		decompression information structure holding the read in
 *			information
 * @param verbose_en	print verbose output if not zero
 *
 * @returns 0 on success, error otherwise
 */

int cmp_info_read(const char *file_name, struct cmp_info *info, int verbose_en)
{
	int err;
	FILE *fp;

	if (!file_name)
		abort();

	if (!info)
		abort();

	if (strstr(file_name, ".cfg"))
		fprintf(stderr, "%s: %s: .cfg file extension found on decompression information file. You may have selected the wrong file.\n",
			PROGRAM_NAME, file_name);

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, file_name,
			strerror(errno));
		return -1;
	}

	err = parse_info(fp, info);
	fclose(fp);
	if (err)
		return -1;


	if (verbose_en) {
		printf("\n\n");
		print_cmp_info(info);
		printf("\n");
	}

	return 0;
}


/**
 * @brief skip a sequence of spaces (as identified by calling isspace)
 *
 * @param str pointer to the null-terminated byte string to be interpreted
 *
 * @returns a pointer to the character after the spaces
 */

static const char *skip_space(const char *str)
{
	while (isspace(*str))
		str++;
	return str;
}


/**
 * @brief skip a comment starting with '#' symbol ending with '\n' or '\0'
 *
 * @param str pointer to the null-terminated byte string to be interpreted
 *
 * @returns a pointer to the character after the comment
 */

static const char *skip_comment(const char *str)
{
	char c = *str;

	if (c == '#') {
		do {
			str++;
			c = *str;
		} while (c != '\0' && c != '\n');
		if (c != '\0')
			str++;
	}
	return str;
}


/**
 * @brief Interprets a hex-encoded 8-bit integer value in a byte string pointed
 *	to by str.
 * @details Discards any whitespace characters (as identified by calling isspace)
 *	until the first non-whitespace character is found, then takes a maximum
 *	of 2 characters (with base 16) unsigned integer number representation
 *	and converts them to an uint8_t value. The function sets the pointer
 *	pointed to by str_end to point to the character past the last character
 *	interpreted. If str_end is a null pointer, it is ignored.
 *
 * @param str		pointer to the null-terminated byte string to be interpreted
 * @param str_end	point to the character past the last numeric character
 *			interpreted (can be NULL)
 *
 * @returns Integer value corresponding to the contents of str on success. If no
 *	conversion can be performed, 0 is returned (errno is set to EINVAL)).
 */

static uint8_t str_to_uint8(const char *str, char const **str_end)
{
	const int BASE = 16;
	int i;
	uint8_t res = 0;
	const char *orig;

	str = skip_space(str);

	orig = str;

	for (i = 0; i < 2; i++) {
		unsigned char c = (unsigned char)*str;

		if (c >= 'a')
			c = c - 'a' + 10;
		else if (c >= 'A')
			c = c - 'A' + 10;
		else if (c <= '9')
			c = c - '0';
		else
			c = 0xff;

		if (unlikely(c >= BASE))
			break;

		res = res * BASE + c;

		str++;
	}

	if (str_end)
		*str_end = str;

	if (unlikely(str == orig)) { /* no value read in */
		errno = EINVAL;
		res = 0;
	}

	return res;
}


/**
 * @brief reads buf_size words of a hex-encoded string to a uint8_t buffer
 *
 * @note A whitespace (space (0x20), form feed (0x0c), line feed (0x0a), carriage
 *	return (0x0d), horizontal tab (0x09), or vertical tab (0x0b) or several in a
 *	sequence are used as separators. If a string contains more than three hexadecimal
 *	numeric characters (0123456789abcdefABCDEF) in a row without separators, a
 *	separator is added after every second hexadecimal numeric character.
 *	Comments after a '#' symbol until the end of the line are ignored.
 *	E.g. "# comment\n ABCD 1    2\n34B 12\n" are interpreted as {0xAB, 0xCD,
 *	0x01, 0x02, 0x34, 0x0B, 0x12}.
 *
 * @param str		pointer to the null-terminated byte string to be interpreted
 * @param data		buffer to write the interpreted content (can be NULL)
 * @param buf_size	number of uint8_t data words to read in
 * @param file_name	file name for better error output (can be NULL)
 * @param verbose_en	print verbose output if not zero
 *
 * @returns the size in bytes to store the string content; negative on error
 */

static __inline ssize_t str2uint8_arr(const char *str, uint8_t *data, uint32_t buf_size,
				      const char *file_name, int verbose_en)
{
	const char *nptr = str;
	const char *eptr = NULL;
	uint32_t i = 0;

	errno = 0;

	if (!data)
		buf_size = ~0U;

	if (!file_name)
		file_name = "unknown file name";

	for (i = 0; i < buf_size; ) {
		uint8_t read_val;
		char c = *nptr;

		if (c == '\0') {
			if (!data)  /* finished counting the sample */
				break;

			fprintf(stderr, "%s: %s: Error: The files do not contain enough data. Expected: 0x%x, has 0x%x.\n",
				PROGRAM_NAME, file_name, buf_size, i);
			return -1;
		}

		if (isspace(c)) {
			nptr = skip_space(nptr);
			continue;
		}
		if (c == '#') {
			nptr = skip_comment(nptr);
			continue;
		}

		read_val = str_to_uint8(nptr, &eptr);
		if (eptr < nptr) /* end pointer must be bigger or equal than the start pointer */
			abort();
		c = *eptr;
		if (c != '\0' && !isxdigit(c) && !isspace(c) && c != '#') {
			if (isprint(c))
				fprintf(stderr, "%s: %s: Error read in '%.*s'. The data are not correctly formatted.\n",
					PROGRAM_NAME, file_name, (int)(eptr-nptr+1), nptr);
			else
				fprintf(stderr, "%s: %s: Error: Non printable character found. If you want to read binary files, use the --binary option.\n",
					PROGRAM_NAME, file_name);
			return -1;
		}
		if (errno == EINVAL) {
			fprintf(stderr, "%s: %s: Error converting the data to integers.\n",
				PROGRAM_NAME, file_name);
			return -1;
		}
		if (data) {
			data[i] = read_val;
			if (verbose_en) { /* print data read in */
				if (i == 0)
					printf("\n\n");
				printf("%02X", data[i]);
				if (i && !((i + 1) % 32))
					printf("\n");
				else
					printf(" ");
			}
		}

		i++;
		nptr = eptr;
	}

	/* did we read all data in the string? 0 at the end are ignored */
	while (isspace(*nptr) || *nptr == '0' || *nptr == '#') {
		if (*nptr == '#')
			nptr = skip_comment(nptr);
		else
			nptr++;
	}
	if (*nptr != '\0') {
		fprintf(stderr, "%s: %s: Warning: The file may contain more data than read from it.\n",
			PROGRAM_NAME, file_name);
	}

	return (ssize_t)i;
}


/**
 * @brief reads hex-encoded uint8_t data form a file to a buffer
 *
 * @note A whitespace (space (0x20), form feed (0x0c), line feed (0x0a), carriage
 *	return (0x0d), horizontal tab (0x09), or vertical tab (0x0b) or several in a
 *	sequence are used as separators. If a string contains more than three hexadecimal
 *	numeric characters (0123456789abcdefABCDEF) in a row without separators, a
 *	separator is added after every second hexadecimal numeric character.
 *	Comments after a '#' symbol until the end of the line are ignored.
 *	E.g. "# comment\n ABCD 1    2\n34B 12\n" are interpreted as {0xAB, 0xCD,
 *	0x01, 0x02, 0x34, 0x0B, 0x12}.
 *
 * @param file_name	data/model file name
 * @param buf		buffer to write the file content (can be NULL)
 * @param buf_size	number of uint8_t data words to read in
 * @param flags		CMP_IO_VERBOSE_EXTRA	print verbose output if set
 *			CMP_IO_BINARY	read in file in binary format if set
 *
 * @returns the size in bytes to store the file content; negative on error
 */

ssize_t read_file8(const char *file_name, uint8_t *buf, uint32_t buf_size, int flags)
{
	FILE *fp;
	char *file_cpy = NULL;
	long file_size;
	ssize_t size;
	size_t ret_code;

	if (!file_name)
		abort();

	errno = 0;
	fp = fopen(file_name, "rb");
	if (fp == NULL)
		goto fail;

	/* Get the number of bytes */
	if (fseek(fp, 0L, SEEK_END) != 0)
		goto fail;
	file_size = ftell(fp);
	if (file_size < 0)
		goto fail;
	if (file_size == 0) {
		fprintf(stderr, "%s: %s: Error: The file is empty.\n", PROGRAM_NAME, file_name);
		goto fail;
	}
	if ((unsigned long)file_size < buf_size) {
		fprintf(stderr, "%s: %s: Error: The files do not contain enough data.\n", PROGRAM_NAME, file_name);
		goto fail;
	}
	/* reset the file position indicator to the beginning of the file */
	if (fseek(fp, 0L, SEEK_SET) != 0)
		goto fail;

	if (flags & CMP_IO_BINARY) {
		if (buf) {
			ret_code = fread(buf, sizeof(uint8_t), buf_size, fp);
			if (ret_code != (size_t)buf_size) {
				if (feof(fp))
					printf("%s: %s: Error: unexpected end of file.\n", PROGRAM_NAME, file_name);
				goto fail;
			}
		}
		fclose(fp);
		return file_size;
	}

	file_cpy = calloc((size_t)file_size+1, sizeof(char));
	if (file_cpy == NULL) {
		fprintf(stderr, "%s: %s: Error: allocating memory!\n", PROGRAM_NAME, file_name);
		goto fail;
	}

	/* copy all the text into the file_cpy buffer */
	ret_code = fread(file_cpy, sizeof(uint8_t), (unsigned long)file_size, fp);
	if (ret_code != (size_t)file_size) {
		if (feof(fp))
			printf("%s: %s: Error: unexpected end of file.\n", PROGRAM_NAME, file_name);
		goto fail;
	}
	/* just to be safe we have a zero terminated string */
	file_cpy[file_size] = '\0';

	fclose(fp);
	fp = NULL;

	size = str2uint8_arr(file_cpy, buf, buf_size, file_name, flags & CMP_IO_VERBOSE_EXTRA);

	free(file_cpy);
	file_cpy = NULL;

	return size;
fail:
	if (errno)
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, file_name,
			strerror(errno));
	if (fp)
		fclose(fp);
	free(file_cpy);
	return -1;
}


/**
 * @brief reads hex-encoded data from a file into a buffer
 *
 * @param file_name	data/model file name
 * @param cmp_type	RDCU or chunk compression?
 * @param buf		buffer to write the file content (can be NULL)
 * @param buf_size	size in bytes of the buffer
 * @param flags		CMP_IO_VERBOSE_EXTRA	print verbose output if set
 *			CMP_IO_BINARY	read in file in binary format if set
 *
 * @returns the size in bytes to store the file content; negative on error
 */

ssize_t read_file_data(const char *file_name, enum cmp_type cmp_type,
		       void *buf, uint32_t buf_size, int flags)
{
	ssize_t size;
	int err;

	size = read_file8(file_name, (uint8_t *)buf, buf_size, flags);
	if (size > INT32_MAX)
		return -1;

	if (size < 0)
		return size;

	switch (cmp_type) {
	case CMP_TYPE_RDCU:
		err = be_to_cpu_data_type(buf, buf_size, DATA_TYPE_IMAGETTE);
		break;
	case CMP_TYPE_CHUNK:
		err = be_to_cpu_chunk(buf, buf_size);
		break;
	case CMP_TYPE_ERROR:
	default:
		err = -1;
	}

	if (err)
		return -1;

	return size;
}


/**
 * @brief reads a file containing a compression entity
 *
 * @param file_name	file name of the file containing the compression entity
 * @param ent		pointer to the buffer where the content of the file is written (can be NULL)
 * @param ent_size	size in bytes of the compression entity to read in
 * @param flags		CMP_IO_VERBOSE	print verbose output if set
 *			CMP_IO_BINARY	read in file in binary format if set
 *
 * @returns the size in bytes to store the file content; negative on error
 */

ssize_t read_file_cmp_entity(const char *file_name, struct cmp_entity *ent,
			     uint32_t ent_size, int flags)
{
	ssize_t size;

	size = read_file8(file_name, (uint8_t *)ent, ent_size, flags);
	if (size < 0)
		return size;

	if (size < GENERIC_HEADER_SIZE) {
		fprintf(stderr, "%s: %s: Error: The file is too small to contain a compression entity header.\n",
			PROGRAM_NAME, file_name);
		return -1;
	}

	if (ent) {
		enum cmp_data_type data_type = cmp_ent_get_data_type(ent);

		if (flags & CMP_IO_VERBOSE) {
			printf("\n");
			cmp_ent_parse(ent);
		}

		if (data_type == DATA_TYPE_UNKNOWN) {
			fprintf(stderr, "%s: %s: Error: Compression data type is not supported. The header of the compression entity may be corrupted.\n",
				PROGRAM_NAME, file_name);
			return -1;
		}
		if (size != (ssize_t)cmp_ent_get_size(ent)) {
			fprintf(stderr, "%s: %s: The size of the compression entity set in the header of the compression entity is not the same size as the read-in file has. Expected: 0x%x, has 0x%lx.\n",
				PROGRAM_NAME, file_name, cmp_ent_get_size(ent), (unsigned long)size);
			return -1;
		}
	}

	return size;
}


/*
 * @brief generate from the cmp_tool version string a version_id for the
 * compression entity header
 *
 * @param version number string like: 1.04
 *
 * @returns version_id for the compression header; 0 on error
 */

uint32_t cmp_tool_gen_version_id(const char *version)
{
	char *copy, *token;
	unsigned int n;
	uint32_t version_id;

	/*
	 * version_id bits: msb |xxxx xxxx | xxxx xxxx| lsb
	 *                        ^^^ ^^^^ | ^^^^ ^^^^
	 *                        mayor num| minor version number
	 *                       ^
	 *                       CMP_TOOL_VERSION_ID_BIT
	 */
	copy = strdup(version);
	token = strtok(copy, ".");
	n = (unsigned int)atoi(token);
	if (n > UINT16_MAX) {
		free(copy);
		return 0;
	}
	version_id = n << 16U;
	if (version_id & CMP_TOOL_VERSION_ID_BIT) {
		free(copy);
		return 0;
	}

	token = strtok(NULL, ".");
	n = (unsigned int)atoi(token);
	free(copy);
	if (n > UINT16_MAX)
		return 0;

	version_id |= n;

	return version_id | CMP_TOOL_VERSION_ID_BIT;
}


/**
 * @brief write compression configuration to a stream
 * @note internal use only!
 *
 * @param fp		FILE pointer
 * @param rcfg		pointer to a RDCU configuration to print
 * @param add_ap_pars	if non-zero write the adaptive RDCU parameter in the
 *			file
 */

static void write_cfg_internal(FILE *fp, const struct rdcu_cfg *rcfg,
			       int add_ap_pars)
{
	if (!fp)
		return;

	if (!rcfg) {
		fprintf(fp, "Pointer to the compressor configuration is NULL.\n");
		return;
	}

	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "\n");
	fprintf(fp, "# RDCU compression configuration\n");
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Selected compression mode\n");
	fprintf(fp, "# 0: raw mode\n");
	fprintf(fp, "# 1: model mode with zero escape symbol mechanism\n");
	fprintf(fp, "# 2: 1d differencing mode without input model with zero escape symbol mechanism\n");
	fprintf(fp, "# 3: model mode with multi escape symbol mechanism\n");
	fprintf(fp, "# 4: 1d differencing mode without input model multi escape symbol mechanism\n");
	fprintf(fp, "\n");
	fprintf(fp, "cmp_mode = %d\n", rcfg->cmp_mode);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Number of samples to compress, length of the data and model buffer\n");
	fprintf(fp, "\n");
	fprintf(fp, "samples = %" PRIu32 "\n", rcfg->samples);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Length of the compressed data buffer in number of samples\n");
	fprintf(fp, "\n");
	fprintf(fp, "buffer_length = %" PRIu32 "\n", rcfg->buffer_length);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Model weighting parameter\n");
	fprintf(fp, "\n");
	fprintf(fp, "model_value = %" PRIu32 "\n", rcfg->model_value);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Number of noise bits to be rounded\n");
	fprintf(fp, "\n");
	fprintf(fp, "round = %" PRIu32 "\n", rcfg->round);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Golomb parameter for dictionary selection\n");
	fprintf(fp, "\n");
	fprintf(fp, "golomb_par = %" PRIu32 "\n", rcfg->golomb_par);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Spillover threshold for encoding outliers\n");
	fprintf(fp, "\n");
	fprintf(fp, "spill = %" PRIu32 "\n", rcfg->spill);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	if (add_ap_pars) {
		fprintf(fp, "# Adaptive 1 Golomb parameter\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap1_golomb_par = %" PRIu32 "\n", rcfg->ap1_golomb_par);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Adaptive 1 spillover threshold\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap1_spill = %" PRIu32 "\n", rcfg->ap1_spill);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Adaptive 2 Golomb parameter\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap2_golomb_par = %" PRIu32 "\n", rcfg->ap2_golomb_par);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Adaptive 2 spillover threshold\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap2_spill = %" PRIu32 "\n", rcfg->ap2_spill);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# RDCU data to compress start address, the first data address in the RDCU SRAM\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_data_adr = 0x%06"PRIX32"\n", rcfg->rdcu_data_adr);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# RDCU model start address, the first model address in the RDCU SRAM\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_model_adr = 0x%06"PRIX32"\n", rcfg->rdcu_model_adr);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# RDCU updated model start address, the first address in the RDCU SRAM where the\n");
		fprintf(fp, "# updated model is stored\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_new_model_adr = 0x%06"PRIX32"\n", rcfg->rdcu_new_model_adr);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# RDCU compressed data start address, the first output data address in the SRAM\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_buffer_adr = 0x%06"PRIX32"\n", rcfg->rdcu_buffer_adr);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
	}
}


/**
 * @brief prints config struct
 *
 * @param rcfg		pointer to a RDCU configuration to print
 * @param add_ap_pars	if non-zero write the adaptive RDCU parameter in the file
 */

void cmp_cfg_print(const struct rdcu_cfg *rcfg, int add_ap_pars)
{
	write_cfg_internal(stdout, rcfg, add_ap_pars);
}


/**
 * @brief write compression configuration to a file
 *
 * @param rcfg		pointer to a RDCU configuration to print
 * @param output_prefix prefix of the written file (.cfg is added to the prefix)
 * @param verbose	print verbose output if not zero
 * @param add_ap_pars	if non-zero write the adaptive RDCU parameter in the file
 *
 * @returns 0 on success, error otherwise
 */

int cmp_cfg_fo_file(const struct rdcu_cfg *rcfg, const char *output_prefix,
		    int verbose, int add_ap_pars)
{
	FILE *fp = open_file(output_prefix, ".cfg");

	if (fp == NULL) {
		fprintf(stderr, "%s: %s%s: %s\n", PROGRAM_NAME, output_prefix,
			".cfg", strerror(errno));
		return -1;
	}

	write_cfg_internal(fp, rcfg, add_ap_pars);

	fclose(fp);

	if (verbose)
		cmp_cfg_print(rcfg, add_ap_pars);

	return 0;
}


/**
 * @brief write a decompression information structure to a file
 *
 * @param info		compressor information contains information of an
 *			executed compression
 * @param output_prefix	prefix of the written file (.info is added to the prefix)
 * @param add_ap_pars	if non-zero write the additional RDCU parameter in the
 *			file
 *
 * @returns 0 on success, error otherwise
 */

int cmp_info_to_file(const struct cmp_info *info MAYBE_UNUSED,
		     const char *output_prefix, int add_ap_pars)
{
	FILE *fp = open_file(output_prefix, ".info");

	if (fp == NULL) {
		fprintf(stderr, "%s: %s%s: %s\n", PROGRAM_NAME, output_prefix,
			".info", strerror(errno));
		return -1;
	}

	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Decompression Information File\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Compression mode used\n");
	fprintf(fp, "# 0: raw mode\n");
	fprintf(fp, "# 1: model mode with zero escape symbol mechanism\n");
	fprintf(fp, "# 2: 1d differencing mode without input model with zero escape symbol mechanism\n");
	fprintf(fp, "# 3: model mode with multi escape symbol mechanism\n");
	fprintf(fp, "# 4: 1d differencing mode without input model multi escape symbol mechanism\n");
	fprintf(fp, "\n");
	fprintf(fp, "cmp_mode_used = %" PRIu32 "\n", info->cmp_mode_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Number of samples used, measured in 16 bit units, length of the data and model buffer\n");
	fprintf(fp, "\n");
	fprintf(fp, "samples_used = %" PRIu32 "\n", info->samples_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Compressed data size; measured in bits\n");
	fprintf(fp, "\n");
	fprintf(fp, "cmp_size = %" PRIu32 "\n", info->cmp_size);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Golomb parameter used\n");
	fprintf(fp, "\n");
	fprintf(fp, "golomb_par_used = %" PRIu32 "\n", info->golomb_par_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Spillover threshold used\n");
	fprintf(fp, "\n");
	fprintf(fp, "spill_used = %" PRIu32 "\n", info->spill_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Model weighting parameter used\n");
	fprintf(fp, "\n");
	fprintf(fp, "model_value_used = %u\n", info->model_value_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Number of noise bits to be rounded used\n");
	fprintf(fp, "\n");
	fprintf(fp, "round_used = %u\n", info->round_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	if (add_ap_pars) {
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Hardware Compressor Settings (not need for SW compression)\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "\n");
		fprintf(fp, "# Adaptive compressed data size 1; measured in bits\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap1_cmp_size = %" PRIu32 "\n", info->ap1_cmp_size);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Adaptive compressed data size 2; measured in bits\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap2_cmp_size = %" PRIu32 "\n", info->ap2_cmp_size);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Updated model info start address used\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_new_model_adr_used = 0x%06"PRIX32"\n", info->rdcu_new_model_adr_used);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, " #RDCU compressed data start address\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_cmp_adr_used = 0x%06"PRIX32"\n", info->rdcu_cmp_adr_used);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
	}
	fprintf(fp, "# Compressor errors\n");
	fprintf(fp, "\n");
	fprintf(fp, "# [bit 0] small_buffer_err; The length for the compressed data buffer is too small\n");
	fprintf(fp, "# [bit 1] cmp_mode_err; The cmp_mode parameter is not set correctly\n");
	fprintf(fp, "# [bit 2] model_value_err; The model_value parameter is not set correctly\n");
	fprintf(fp, "# [bit 3] cmp_par_err; The spill, golomb_par combination is not set correctly\n");
	fprintf(fp, "# [bit 4] ap1_cmp_par_err; The ap1_spill, ap1_golomb_par combination is not set correctly (only HW compression)\n");
	fprintf(fp, "# [bit 5] ap2_cmp_par_err; The ap2_spill, ap2_golomb_par combination is not set correctly (only HW compression)\n");
	fprintf(fp, "# [bit 6] mb_err; Multi bit error detected by the memory controller (only HW compression)\n");
	fprintf(fp, "# [bit 7] slave_busy_err; The bus master has received the 'slave busy' status (only HW compression)\n");
	fprintf(fp, "# [bit 8] slave_blocked_err; The bus master has received the “slave blocked” status (only HW compression)\n");
	fprintf(fp, "# [bit 9] invalid address_err; The bus master has received the “invalid address” status (only HW compression)\n");
	fprintf(fp, "\n");
	fprintf(fp, "cmp_err = %x\n", info->cmp_err);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fclose(fp);

	return 0;
}


/**
 * @brief write compression parameters to a stream
 * @note internal use only!
 *
 * @param fp	FILE pointer
 * @param par	pointer to a compression parameters struct to print
 */

static void write_cmp_par_internal(FILE *fp, const struct cmp_par *par)
{
	if (!fp)
		return;

	if (!par) {
		fprintf(fp, "Pointer to the compression parameters is NULL.\n");
		return;
	}


	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "\n");
	fprintf(fp, "# Chunk compression parameters\n");
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	fprintf(fp, "cmp_mode = %d\n", par->cmp_mode);
	fprintf(fp, "model_value = %" PRIu32 "\n", par->model_value);
	fprintf(fp, "lossy_par = %" PRIu32 "\n", par->lossy_par);
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	fprintf(fp, "nc_imagette = %" PRIu32 "\n", par->nc_imagette);
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	fprintf(fp, "s_exp_flags = %" PRIu32 "\n", par->s_exp_flags);
	fprintf(fp, "s_fx = %" PRIu32 "\n", par->s_fx);
	fprintf(fp, "s_ncob = %" PRIu32 "\n", par->s_ncob);
	fprintf(fp, "s_efx = %" PRIu32 "\n", par->s_efx);
	fprintf(fp, "s_ecob = %" PRIu32 "\n", par->s_ecob);
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	fprintf(fp, "l_exp_flags = %" PRIu32 "\n", par->l_exp_flags);
	fprintf(fp, "l_fx = %" PRIu32 "\n", par->l_fx);
	fprintf(fp, "l_ncob = %" PRIu32 "\n", par->l_ncob);
	fprintf(fp, "l_efx = %" PRIu32 "\n", par->l_efx);
	fprintf(fp, "l_ecob = %" PRIu32 "\n", par->l_ecob);
	fprintf(fp, "l_fx_cob_variance = %" PRIu32 "\n", par->l_fx_cob_variance);
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	fprintf(fp, "saturated_imagette = %" PRIu32 "\n", par->saturated_imagette);
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	fprintf(fp, "nc_offset_mean = %" PRIu32 "\n", par->nc_offset_mean);
	fprintf(fp, "nc_offset_variance = %" PRIu32 "\n", par->nc_offset_variance);
	fprintf(fp, "nc_background_mean = %" PRIu32 "\n", par->nc_background_mean);
	fprintf(fp, "nc_background_variance = %" PRIu32 "\n", par->nc_background_variance);
	fprintf(fp, "nc_background_outlier_pixels = %" PRIu32 "\n", par->nc_background_outlier_pixels);
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	fprintf(fp, "smearing_mean = %" PRIu32 "\n", par->smearing_mean);
	fprintf(fp, "smearing_variance_mean = %" PRIu32 "\n", par->smearing_variance_mean);
	fprintf(fp, "smearing_outlier_pixels = %" PRIu32 "\n", par->smearing_outlier_pixels);
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	fprintf(fp, "fc_imagette = %" PRIu32 "\n", par->fc_imagette);
	fprintf(fp, "fc_offset_mean = %" PRIu32 "\n", par->fc_offset_mean);
	fprintf(fp, "fc_offset_variance = %" PRIu32 "\n", par->fc_offset_variance);
	fprintf(fp, "fc_background_mean = %" PRIu32 "\n", par->fc_background_mean);
	fprintf(fp, "fc_background_variance = %" PRIu32 "\n", par->fc_background_variance);
	fprintf(fp, "fc_background_outlier_pixels = %" PRIu32 "\n", par->fc_background_outlier_pixels);
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
}


/**
 * @brief prints cmp_par struct
 *
 * @param par	pointer to a compression parameters struct to print
 */

void cmp_par_print(const struct cmp_par *par)
{
	write_cmp_par_internal(stdout, par);
}


/**
 * @brief write the compression parameters to a file
 *
 * @param par		pointer to a compression parameters struct
 * @param output_prefix	prefix of the written file (.par is added to the prefix)
 * @param verbose	print verbose output if not zero
 *
 * @returns 0 on success, error otherwise
 */

int cmp_par_fo_file(const struct cmp_par *par, const char *output_prefix,
		    int verbose)
{
	FILE *fp = open_file(output_prefix, ".par");

	if (fp == NULL) {
		fprintf(stderr, "%s: %s%s: %s\n", PROGRAM_NAME, output_prefix,
			".cfg", strerror(errno));
		return -1;
	}

	write_cmp_par_internal(fp, par);

	fclose(fp);

	if (verbose)
		cmp_par_print(par);

	return 0;
}
