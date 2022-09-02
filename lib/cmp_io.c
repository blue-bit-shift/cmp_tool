/**
 * @file   cmp_io.c
 * @author Johannes Seelig (johannes.seelig@univie.ac.at)
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
 * @brief compression tool Input/Output library
 * @warning this part of the software is not intended to run on-board on the ICU.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include <cmp_tool-config.h>
#include <cmp_io.h>
#include <cmp_support.h>
#include <rdcu_cmd.h>
#include <byteorder.h>
#include <cmp_data_types.h>


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
	{DATA_TYPE_UNKNOWN, "DATA_TYPE_UNKNOWN"}
};


/**
 * @brief print help information
 *
 * @param program_name	name of the program
 */

void print_help(const char *program_name)
{
	printf("usage: %s [options] [<argument>]\n", program_name);
	printf("General Options:\n");
	printf("  -h, --help               Print this help text and exit\n");
	printf("  -o <prefix>              Use the <prefix> for output files\n");
	printf("  -n, --model_cfg          Print a default model configuration and exit\n");
	printf("  --diff_cfg               Print a default 1d-differencing configuration and exit\n");
	printf("  --no_header              Do not add a compression entity header in front of the compressed data\n");
	printf("  -a, --rdcu_par           Add additional RDCU control parameters\n");
	printf("  -V, --version            Print program version and exit\n");
	printf("  -v, --verbose            Print various debugging information\n");
	printf("Compression Options:\n");
	printf("  -c <file>                File containing the compressing configuration\n");
	printf("  -d <file>                File containing the data to be compressed\n");
	printf("  -m <file>                File containing the model of the data to be compressed\n");
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
	pathname = (char *) alloca((size_t)n + 1);

	errno = 0;
	n = snprintf(pathname, (size_t)n + 1, "%s%s", dirname, filename);
	if (n < 0) {
		perror("snprintf failed");
		abort();
	}

	return fopen(pathname, "w");
}


/**
 * @brief write uncompressed input data to an output file
 *
 * @param data		the data to write a file
 * @param data_size	size of the data in bytes
 * @param output_prefix  file name without file extension
 * @param name_extension file extension (with leading point character)
 * @param verbose	print verbose output if not zero
 *
 * @returns 0 on success, error otherwise
 */

int write_input_data_to_file(void *data, uint32_t data_size, enum cmp_data_type data_type,
			     const char *output_prefix, const char *name_extension, int verbose)
{
	uint32_t i = 0;
	FILE *fp;
	uint8_t *tmp_buf;
	size_t sample_size = size_of_a_sample(data_type);

	if (!data)
		abort();

	if (data_size == 0)
		return 0;

	if (!sample_size)
		return -1;

	fp = open_file(output_prefix, name_extension);
	if (fp == NULL) {
		fprintf(stderr, "%s: %s%s: %s\n", PROGRAM_NAME, output_prefix,
			name_extension, strerror(errno));
		return -1;
	}

	tmp_buf = malloc(data_size);
	memcpy(tmp_buf, data, data_size);
	cmp_input_big_to_cpu_endianness(tmp_buf, data_size, data_type);

	for (i = 0 ; i < data_size; i++) {
		fprintf(fp, "%02X",  tmp_buf[i]);
		if ((i + 1) % 16 == 0)
			fprintf(fp, "\n");
		else
			fprintf(fp, " ");
	}
	fprintf(fp, "\n");

	fclose(fp);

	if (verbose) {
		printf("\n\n");
			for (i = 0; i < data_size; i++) {
				printf("%02X", tmp_buf[i]);
			if ((i + 1) % 16 == 0)
				printf("\n");
			else
				printf(" ");
		}

		printf("\n\n");
	}

	free(tmp_buf);
	return 0;
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
 * @param verbose	print verbose output if not zero
 *
 * @returns 0 on success, error otherwise
 */

int write_cmp_data_file(const void *buf, uint32_t buf_size, const char *output_prefix,
			const char *name_extension, int verbose)
{
	unsigned int i;
	FILE *fp;
	const uint8_t *p = (const uint8_t *)buf;

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

	for (i = 0; i < buf_size; i++) {
		fprintf(fp, "%02X", p[i]);
		if ((i + 1) % 32 == 0)
			fprintf(fp, "\n");
		else
			fprintf(fp, " ");
	}
	fprintf(fp, "\n");

	fclose(fp);

	if (verbose) {
		printf("\n\n");
		for (i = 0; i < buf_size; i++) {
			printf("%02X", p[i]);
			if ((i + 1) % 32 == 0)
				printf("\n");
			else
				printf(" ");
		}
		printf("\n\n");
	}
	return 0;
}


/**
 * @brief remove white spaces and tabs
 *
 * @param s input string
 */

static void remove_spaces(char *s)
{
	const char *d;

	if (!s)
		return;

	d = s;

	do {
		while (*d == ' ' || *d == '\t')
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
		printf("%s: The sram address is out of the rdcu range\n",
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
 * @param val_str	value string contain the value to convert in uint32_t
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

			for (j = 0;  j < sizeof(data_type_string_table) / sizeof(data_type_string_table[0]); j++) {
				if (!strcmp(data_type_str, data_type_string_table[j].str)) {
					data_type = data_type_string_table[j].data_type;
					break;
				}
			}
		} else {
			uint32_t read_val;

			if (!atoui32("Compression Data Type", data_type_str, &read_val)) {
				data_type = read_val;
				if (!cmp_data_type_valid(data_type))
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

	for (j = 0;  j < sizeof(data_type_string_table) / sizeof(data_type_string_table[0]); j++) {
		if (data_type == data_type_string_table[j].data_type) {
			string = data_type_string_table[j].str;
			break;
		}
	}

	return string;
}


/**
 * @brief parse a compression mode vale string to an integer
 * @note string can be either a number or the name of the compression mode
 *
 * @param cmp_mode_str	string containing the compression mode value to parse
 * @param cmp_mode	address where the parsed result is written
 *
 * @returns 0 on success, error otherwise
 */

int cmp_mode_parse(const char *cmp_mode_str, uint32_t *cmp_mode)
{
	size_t j;
	static const struct {
		uint32_t  cmp_mode;
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
		for (j = 0;  j < sizeof(conversion) / sizeof(conversion[0]);  ++j) {
			if (!strcmp(cmp_mode_str, conversion[j].str)) {
				*cmp_mode = conversion[j].cmp_mode;
				return 0;
			}
		}
		return -1;
	} else {
		if (atoui32(cmp_mode_str, cmp_mode_str, cmp_mode))
			return -1;
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
 * @param cfg	compression configuration structure holding the read in
 *		configuration
 *
 * @returns 0 on success, error otherwise
 */

static int parse_cfg(FILE *fp, struct cmp_cfg *cfg)
{
	char *token1, *token2;
	char line[MAX_CONFIG_LINE];
	enum {CMP_MODE, SAMPLES, BUFFER_LENGTH, LAST_ITEM};
	int j, must_read_items[LAST_ITEM] = {0};

	if (!fp)
		return -1;

	if (!cfg)
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

		token1 = strtok(line, "=");
		token2 = strtok(NULL, "=");
		if (token1 == NULL)
			continue;
		if (token2 == NULL)
			continue;


		if (!strcmp(token1, "data_type")) {
			cfg->data_type = string2data_type(token2);
			if (cfg->data_type == DATA_TYPE_UNKNOWN)
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_mode")) {
			must_read_items[CMP_MODE] = 1;
			if (cmp_mode_parse(token2, &cfg->cmp_mode))
				return -1;
			continue;
		}
		if (!strcmp(token1, "golomb_par")) {
			if (atoui32(token1, token2, &cfg->golomb_par))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill")) {
			if (atoui32(token1, token2, &cfg->spill))
				return -1;
			continue;
		}
		if (!strcmp(token1, "model_value")) {
			if (atoui32(token1, token2, &cfg->model_value))
				return -1;
			continue;
		}
		if (!strcmp(token1, "round")) {
			if (atoui32(token1, token2, &cfg->round))
				return -1;
			continue;
		}
		if (!strcmp(token1, "ap1_golomb_par")) {
			if (atoui32(token1, token2, &cfg->ap1_golomb_par))
				return -1;
			continue;
		}
		if (!strcmp(token1, "ap1_spill")) {
			if (atoui32(token1, token2, &cfg->ap1_spill))
				return -1;
			continue;
		}
		if (!strcmp(token1, "ap2_golomb_par")) {
			if (atoui32(token1, token2, &cfg->ap2_golomb_par))
				return -1;
			continue;
		}
		if (!strcmp(token1, "ap2_spill")) {
			if (atoui32(token1, token2, &cfg->ap2_spill))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_exp_flags")) {
			if (atoui32(token1, token2, &cfg->cmp_par_exp_flags))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_exp_flags")) {
			if (atoui32(token1, token2, &cfg->spill_exp_flags))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_fx")) {
			if (atoui32(token1, token2, &cfg->cmp_par_fx))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_fx")) {
			if (atoui32(token1, token2, &cfg->spill_fx))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_ncob")) {
			if (atoui32(token1, token2, &cfg->cmp_par_ncob))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_ncob")) {
			if (atoui32(token1, token2, &cfg->spill_ncob))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_efx")) {
			if (atoui32(token1, token2, &cfg->cmp_par_efx))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_efx")) {
			if (atoui32(token1, token2, &cfg->spill_efx))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_ecob")) {
			if (atoui32(token1, token2, &cfg->cmp_par_ecob))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_ecob")) {
			if (atoui32(token1, token2, &cfg->spill_ecob))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_fx_cob_variance")) {
			if (atoui32(token1, token2, &cfg->cmp_par_fx_cob_variance))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_fx_cob_variance")) {
			if (atoui32(token1, token2, &cfg->spill_fx_cob_variance))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_mean")) {
			if (atoui32(token1, token2, &cfg->cmp_par_mean))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_mean")) {
			if (atoui32(token1, token2, &cfg->spill_mean))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_variance")) {
			if (atoui32(token1, token2, &cfg->cmp_par_variance))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_variance")) {
			if (atoui32(token1, token2, &cfg->spill_variance))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_par_pixels_error")) {
			if (atoui32(token1, token2, &cfg->cmp_par_pixels_error))
				return -1;
			continue;
		}
		if (!strcmp(token1, "spill_pixels_error")) {
			if (atoui32(token1, token2, &cfg->spill_pixels_error))
				return -1;
			continue;
		}
		if (!strcmp(token1, "rdcu_data_adr")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				printf("%s: Error read in rdcu_data_adr_par\n",
				       PROGRAM_NAME);
				return -1;
			}
			cfg->rdcu_data_adr = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "rdcu_model_adr")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				printf("%s: Error read in rdcu_model_adr_par\n",
				       PROGRAM_NAME);
				return -1;
			}
			cfg->rdcu_model_adr = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "rdcu_new_model_adr")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				printf("%s: Error read in rdcu_new_model_adr_par\n",
				       PROGRAM_NAME);
				return -1;
			}
			cfg->rdcu_new_model_adr = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "samples")) {
			if (atoui32(token1, token2, &cfg->samples))
				return -1;
			must_read_items[SAMPLES] = 1;
			continue;
		}
		if (!strcmp(token1, "rdcu_buffer_adr")) {
			int i = sram_addr_to_int(token2);

			if (i < 0) {
				printf("%s: Error read in rdcu_buffer_adr_par\n",
				       PROGRAM_NAME);
				return -1;
			}
			cfg->rdcu_buffer_adr = (uint32_t)i;
			continue;
		}
		if (!strcmp(token1, "buffer_length")) {
			if (atoui32(token1, token2, &cfg->buffer_length))
				return -1;
			must_read_items[BUFFER_LENGTH] = 1;
			continue;
		}
	}

	if (raw_mode_is_used(cfg->cmp_mode))
		if (must_read_items[CMP_MODE] == 1 &&
		    must_read_items[BUFFER_LENGTH] == 1)
			return 0;

	for (j = 0; j < LAST_ITEM; j++) {
		if (must_read_items[j] < 1) {
			fprintf(stderr, "%s: Some parameters are missing. Check if the following parameters: cmp_mode, golomb_par, spill, samples and buffer_length are all set in the configuration file.\n",
				PROGRAM_NAME);
			return -1;
		}
	}

	return 0;
}


/**
 * @brief reading a compressor configuration file
 *
 * @param file_name	file containing the compression configuration file
 * @param cfg		compression configuration structure holding the read in
 *			configuration
 * @param verbose_en	print verbose output if not zero
 *
 * @returns 0 on success, error otherwise
 */

int read_cmp_cfg(const char *file_name, struct cmp_cfg *cfg, int verbose_en)
{
	int err;
	FILE *fp;

	if (!file_name)
		abort();

	if (!cfg)
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

	err = parse_cfg(fp, cfg);
	fclose(fp);
	if (err)
		return err;

	if (verbose_en) {
		printf("\n\n");
		print_cmp_cfg(cfg);
		printf("\n");
	}

	return 0;
}


/**
 * @brief  parse a file containing a decompression information
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
			} else {
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

int read_cmp_info(const char *file_name, struct cmp_info *info, int verbose_en)
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
 * @brief Interprets an hex-encoded 8 bit integer value in a byte string pointed
 *	to by str.
 * @details Discards any whitespace characters (as identified by calling isspace)
 *	until the first non-whitespace character is found, then takes maximum 2
 *	characters (with base 16) unsigned integer number representation and
 *	converts them to an uint8_t value. The function set the pointer
 *	pointed to by str_end to point to the character past the last character
 *	interpreted. If str_end is a null pointer, it is ignored.
 *
 * @param str     pointer to the null-terminated byte string to be interpreted
 * @param str_end pointer to a pointer to character (can be NULL)
 *
 * @returns Integer value corresponding to the contents of str on success. If no
 *	conversion can be performed, 0 is returned (errno is set to EINVAL)).
 */

static uint8_t str_to_uint8(const char *str, char **str_end)
{
	const int BASE = 16;
	int i;
	uint8_t res = 0;
	const char *orig;

	str = skip_space(str);

	orig = str;

	for (i = 0; i < 2; i++) {
		unsigned char c = *str;

		if (c >= 'a')
			c = c - 'a' + 10;
		else if (c >= 'A')
			c = c - 'A' + 10;
		else if (c <= '9')
			c = c - '0';
		else
			c = 0xff;

		if (c >= BASE)
			break;

		res = res * BASE + c;

		str++;
	}

	if (str_end)
		*str_end = (char *)str;

	if (str == orig) { /* no value read in */
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

static ssize_t str2uint8_arr(const char *str, uint8_t *data, uint32_t buf_size,
			     const char *file_name, int verbose_en)
{
	const char *nptr = str;
	char *eptr = NULL;
	size_t i = 0;

	errno = 0;

	if (!data)
		buf_size = ~0;

	if (!file_name)
		file_name = "unknown file name";

	for (i = 0; i < buf_size; ) {
		uint8_t read_val;
		unsigned char c = *nptr;

		if (c == '\0') {
			if (!data)  /* finished counting the sample */
				break;

			fprintf(stderr, "%s: %s: Error: The files do not contain enough data as requested.\n",
				PROGRAM_NAME, file_name);
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
			fprintf(stderr, "%s: %s: Error read in '%.*s'. The data are not correct formatted.\n",
				PROGRAM_NAME, file_name, (int)(eptr-nptr+1), nptr);
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

	/* did we read all data in the string? */
	while (isspace(*nptr) || *nptr == '#') {
		if (*nptr == '#')
			nptr = skip_comment(nptr);
		else
			nptr = skip_space(nptr);
	}
	if (*nptr != '\0') {
		fprintf(stderr, "%s: %s: Warning: The file may contain more data than specified by the samples or cmp_size parameter.\n",
			PROGRAM_NAME, file_name);
	}

	return i;
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
 * @param verbose_en	print verbose output if not zero
 *
 * @returns the size in bytes to store the file content; negative on error
 */

ssize_t read_file8(const char *file_name, uint8_t *buf, uint32_t buf_size, int verbose_en)
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
		fprintf(stderr, "%s: %s: Warning: The file is empty.\n", PROGRAM_NAME, file_name);
		fclose(fp);
		return 0;
	}
	if (file_size < buf_size) {
		fprintf(stderr, "%s: %s: Error: The files do not contain enough data as requested.\n", PROGRAM_NAME, file_name);
		goto fail;
	}
	/* reset the file position indicator to the beginning of the file */
	if (fseek(fp, 0L, SEEK_SET) != 0)
		goto fail;

	file_cpy = (char *)calloc(file_size+1, sizeof(char));
	if (file_cpy == NULL) {
		fprintf(stderr, "%s: %s: Error: allocating memory!\n", PROGRAM_NAME, file_name);
		goto fail;
	}

	/* copy all the text into the file_cpy buffer */
	ret_code = fread(file_cpy, sizeof(char), file_size, fp);
	if (ret_code != (size_t)file_size) {
		if (feof(fp))
			printf("%s: %s: Error: unexpected end of file.\n", PROGRAM_NAME, file_name);
		goto fail;
	}
	/* just to be save we have a zero terminated string */
	file_cpy[file_size] = '\0';

	fclose(fp);
	fp = NULL;

	size = str2uint8_arr(file_cpy, buf, buf_size, file_name, verbose_en);

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
 * @param data_type	compression data type used for the data
 * @param buf		buffer to write the file content (can be NULL)
 * @param buf_size	size in bytes of the buffer
 * @param verbose_en	print verbose output if not zero
 *
 * @returns the size in bytes to store the file content; negative on error
 */

ssize_t read_file_data(const char *file_name, enum cmp_data_type data_type,
		       void *buf, uint32_t buf_size, int verbose_en)
{
	ssize_t size;
	int samples, err;

	size = read_file8(file_name, (uint8_t *)buf, buf_size, verbose_en);

	if (size < 0)
		return size;

	samples = cmp_input_size_to_samples(size, data_type);
	if (samples < 0) {
		fprintf(stderr, "%s: %s: Error: The data are not correct formatted for the used compression data type.\n",
				PROGRAM_NAME, file_name);
		return -1;
	}

	err = cmp_input_big_to_cpu_endianness(buf, size, data_type);
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
 * @param verbose_en	print verbose output if not zero
 *
 * @returns the size in bytes to store the file content; negative on error
 */

ssize_t read_file_cmp_entity(const char *file_name, struct cmp_entity *ent,
			     uint32_t ent_size, int verbose_en)
{
	ssize_t size;

	size = read_file8(file_name, (uint8_t *)ent, ent_size, 0);
	if (size < 0)
		return size;

	if (size < GENERIC_HEADER_SIZE) {
		fprintf(stderr, "%s: %s: Error: The file is too small to contain a compression entity header.\n",
			PROGRAM_NAME, file_name);
		return -1;
	}

	if (ent) {
		enum cmp_data_type data_type = cmp_ent_get_data_type(ent);

		if (data_type == DATA_TYPE_UNKNOWN) {
			fprintf(stderr, "%s: %s: Error: Compression data type is not supported.\n",
				PROGRAM_NAME, file_name);
			return -1;
		}
		if (size != cmp_ent_get_size(ent)) {
			fprintf(stderr, "%s: %s: The size of the compression entity set in the header of the compression entity is not the same size as the read-in file has.\n",
				PROGRAM_NAME, file_name);
			return -1;
		}

		if (verbose_en)
			cmp_ent_parse(ent);
	}

	return size;
}


/**
 * @brief reads hex-encoded uint32_t samples from a file into a buffer
 *
 * @param file_name	data/model file name
 * @param buf		buffer to write the file content (can be NULL)
 * @param buf_size	size of the buf buffer in bytes
 * @param verbose_en	print verbose output if not zero
 *
 * @returns the size in bytes to store the file content; negative on error
 */

ssize_t read_file32(const char *file_name, uint32_t *buf, uint32_t buf_size,
		    int verbose_en)
{
	ssize_t size = read_file8(file_name, (uint8_t *)buf, buf_size, verbose_en);

	if (size < 0)
		return -1;

	if (size & 0x3) {
		fprintf(stderr, "%s: %s: Error: The data are not correct formatted. Expected multiple of 4 hex words.\n",
				PROGRAM_NAME, file_name);
		return -1;
	}

	if (buf) {
		size_t i;
		for (i = 0; i < buf_size/sizeof(uint32_t); i++)
			be32_to_cpus(&buf[i]);
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
	n = atoi(token);
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
	n = atoi(token);
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
 * @param cfg		configuration to print
 * @param rdcu_cfg	if set additional RDCU parameter are printed as well
 */

static void write_cfg_internal(FILE *fp, const struct cmp_cfg *cfg, int rdcu_cfg)
{
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Default Configuration File\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Selected compression data type\n");
	fprintf(fp, "\n");
	fprintf(fp, "data_type = %s\n", data_type2string(cfg->data_type));
	fprintf(fp, "\n");
	fprintf(fp, "# Selected compression mode\n");
	fprintf(fp, "# 0: raw mode\n");
	fprintf(fp, "# 1: model mode with zero escape symbol mechanism\n");
	fprintf(fp, "# 2: 1d differencing mode without input model with zero escape symbol mechanism\n");
	fprintf(fp, "# 3: model mode with multi escape symbol mechanism\n");
	fprintf(fp, "# 4: 1d differencing mode without input model multi escape symbol mechanism\n");
	fprintf(fp, "\n");
	fprintf(fp, "cmp_mode = %u\n", cfg->cmp_mode);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Number of samples (16 bit value) to compress, length of the data and model buffer\n");
	fprintf(fp, "\n");
	fprintf(fp, "samples = %u\n", cfg->samples);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Length of the compressed data buffer in number of samples (16 bit values)\n");
	fprintf(fp, "\n");
	fprintf(fp, "buffer_length = %u\n", cfg->buffer_length);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Golomb parameter for dictionary selection\n");
	fprintf(fp, "\n");
	fprintf(fp, "golomb_par = %u\n", cfg->golomb_par);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Spillover threshold for encoding outliers\n");
	fprintf(fp, "\n");
	fprintf(fp, "spill = %u\n", cfg->spill);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Model weighting parameter\n");
	fprintf(fp, "\n");
	fprintf(fp, "model_value = %u\n", cfg->model_value);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Number of noise bits to be rounded\n");
	fprintf(fp, "\n");
	fprintf(fp, "round = %u\n", cfg->round);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");

	if (rdcu_cfg) {
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Hardware Compressor Settings (not need for SW compression)\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "\n");
		fprintf(fp, "# Adaptive 1 Golomb parameter; HW only\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap1_golomb_par = %u\n", cfg->ap1_golomb_par);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Adaptive 1 spillover threshold; HW only\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap1_spill = %u\n", cfg->ap1_spill);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Adaptive 2 Golomb parameter; HW only\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap2_golomb_par = %u\n", cfg->ap2_golomb_par);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Adaptive 2 spillover threshold; HW only\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap2_spill = %u\n", cfg->ap2_spill);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# RDCU data to compress start address, the first data address in the RDCU SRAM; HW only\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_data_adr = 0x%06X\n",
		       cfg->rdcu_data_adr);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# RDCU model start address, the first model address in the RDCU SRAM\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_model_adr = 0x%06X\n",
		       cfg->rdcu_model_adr);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# RDCU updated model start address, the address in the RDCU SRAM where the updated model is stored\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_new_model_adr = 0x%06X\n",
		       cfg->rdcu_new_model_adr);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# RDCU compressed data start address, the first output data address in the RDCU SRAM\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_buffer_adr = 0x%06X\n", cfg->rdcu_buffer_adr);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
	}
}


/**
 * @brief prints config struct
 *
 * @param cfg		configuration to print
 * @param rdcu_cfg	if set additional RDCU parameter are printed as well
 */

void print_cfg(const struct cmp_cfg *cfg, int rdcu_cfg)
{
	write_cfg_internal(stdout, cfg, rdcu_cfg);
}


/**
 * @brief write compression configuration to a file
 *
 * @param cfg		configuration to print
 * @param output_prefix prefix of the written file (.cfg is added to the prefix)
 * @param rdcu_cfg	if non-zero additional RDCU parameter are printed as well
 * @param verbose	print verbose output if not zero
 *
 * @returns 0 on success, error otherwise
 */

int write_cfg(const struct cmp_cfg *cfg, const char *output_prefix, int rdcu_cfg,
	      int verbose)
{
	FILE *fp = open_file(output_prefix, ".cfg");

	if (fp == NULL) {
		fprintf(stderr, "%s: %s%s: %s\n", PROGRAM_NAME, output_prefix,
			".cfg", strerror(errno));
		return -1;
	}

	write_cfg_internal(fp, cfg, rdcu_cfg);

	fclose(fp);

	if (verbose)
		print_cfg(cfg, rdcu_cfg);
	return 0;
}


/**
 * @brief write a decompression information structure to a file
 *
 * @param info	 compressor information contains information of an executed
 *		 compression
 * @param output_prefix prefix of the written file (.info is added to the prefix)
 * @param rdcu_cfg - if non-zero write additional RDCU parameter in the file
 *
 * @returns 0 on success, error otherwise
 */

int write_info(const struct cmp_info *info, const char *output_prefix,
	       int rdcu_cfg)
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
	fprintf(fp, "cmp_mode_used = %u\n", info->cmp_mode_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Number of samples used, measured in 16 bit units, length of the data and model buffer\n");
	fprintf(fp, "\n");
	fprintf(fp, "samples_used = %u\n", info->samples_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Compressed data size; measured in bits\n");
	fprintf(fp, "\n");
	fprintf(fp, "cmp_size = %u\n", info->cmp_size);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Golomb parameter used\n");
	fprintf(fp, "\n");
	fprintf(fp, "golomb_par_used = %u\n", info->golomb_par_used);
	fprintf(fp, "\n");
	fprintf(fp, "#-------------------------------------------------------------------------------\n");
	fprintf(fp, "# Spillover threshold used\n");
	fprintf(fp, "\n");
	fprintf(fp, "spill_used = %u\n", info->spill_used);
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

	if (rdcu_cfg) {
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Hardware Compressor Settings (not need for SW compression)\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "\n");
		fprintf(fp, "# Adaptive compressed data size 1; measured in bits\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap1_cmp_size = %u\n", info->ap1_cmp_size);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Adaptive compressed data size 2; measured in bits\n");
		fprintf(fp, "\n");
		fprintf(fp, "ap2_cmp_size = %u\n", info->ap2_cmp_size);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, "# Updated model info start address used\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_new_model_adr_used = 0x%06X\n", info->rdcu_new_model_adr_used);
		fprintf(fp, "\n");
		fprintf(fp, "#-------------------------------------------------------------------------------\n");
		fprintf(fp, " #RDCU compressed data start address\n");
		fprintf(fp, "\n");
		fprintf(fp, "rdcu_cmp_adr_used = 0x%06X\n", info->rdcu_cmp_adr_used);
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
