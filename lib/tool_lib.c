/**
 * @file   tool_lib.c
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include "../include/tool_lib.h"
#include "../include/cmp_support.h"
#include "../include/rdcu_cmd.h"
#include "../include/byteorder.h"


/**
 * @brief print help information
 *
 * @param argv	argument vector
 */

void Print_Help(const char *argv)
{
	printf("usage: %s [options] [<argument>]\n", &argv[0]);
	printf("General Options:\n");
	printf("  -h, --help               Print this help text and exit\n");
	printf("  -V, --version            Print program version and exit\n");
	printf("  -v, --verbose            Print various debugging information\n");
	printf("  -n, --model_cfg          Print a default model configuration and exit\n");
	printf("  --diff_cfg               Print a default 1d-differencing configuration and exit\n");
	printf("  -a, --rdcu_par           Print additional RDCU control parameters\n");
	printf("  -o <prefix>              Use the <prefix> for output files\n");
	printf("Compression Options:\n");
	printf("  -c <file>                File containing the compressing configuration\n");
	printf("  -d <file>                File containing the data to be compressed\n");
	printf("  -m <file>                File containing the model of the data to be compressed\n");
	printf("  --rdcu_pkt               Generate RMAP packets for a RDCU compression\n");
	printf("Decompression Options:\n");
	printf("  -i <file>                File containing the decompression information\n");
	printf("  -d <file>                File containing the compressed data\n");
	printf("  -m <file>                File containing the model of the compressed data\n");
	printf("\n");
}


/**
 * @brief opens a file with a name that is a concatenation of directory and file
 *	name.
 *
 * @param dirname	first sting of concatenation
 * @param filename	security sting of concatenation
 *
 * @return a pointer to the opened file
 *
 * @see https://developers.redhat.com/blog/2018/05/24/detecting-string-truncation-with-gcc-8/
 */

static FILE *open_file(const char *dirname, const char *filename)
{
	FILE *fp;
	char *pathname;
	int n;

	if (!dirname)
		return NULL;

	if(!filename)
		return NULL;

	errno = 0;
	n = snprintf(0, 0, "%s%s", dirname, filename);

	if (n < 0) {
		perror("snprintf failed");
		abort();
	}

	errno = 0;
	pathname = (char *) malloc((size_t)n + 1);
	if (!pathname) {
		perror("malloc failed");
		abort();
	}

	errno = 0;
	n = snprintf(pathname, (size_t)n + 1, "%s%s", dirname, filename);
	if (n < 0) {
		perror("snprintf failed");
		abort();
	}

	fp = fopen(pathname, "w");
	free(pathname);
	return fp;
}


/**
 * @brief write a uint16_t buffer to a output file
 *
 * @param buf		the buffer to write a file
 * @param buf_len	length of the buffer
 *
 * @param output_prefix  file name without file extension
 * @param name_extension file extension (with leading point character)
 *
 * @param verbose	print verbose output if not zero
 *
 * @returns 0 on success, error otherwise
 */

int write_to_file16(const uint16_t *buf, uint32_t buf_len, const char
		    *output_prefix, const char *name_extension, int verbose)
{
	uint32_t i;
	FILE *fp;

	if (!buf)
		abort();

	if (buf_len == 0)
		return 0;

	fp = open_file(output_prefix, name_extension);
	if (fp == NULL) {
		fprintf(stderr, "%s: %s%s: %s\n", PROGRAM_NAME, output_prefix,
			name_extension, strerror(errno));
		return -1;
	}

	for (i = 0; i < buf_len; i++) {
		fprintf(fp, "%02X %02X",buf[i] >> 8 ,buf[i] & 0xFF);
		if ((i + 1) % 16 == 0)
			fprintf(fp, "\n");
		else
			fprintf(fp, " ");
	}
	fprintf(fp, "\n");

	fclose(fp);

	if (verbose) {
		printf("\n\n");
			for (i = 0; i < buf_len; i++) {
				printf("%02X %02X",buf[i] >> 8 ,buf[i] & 0xFF);
			if ((i + 1) % 16 == 0)
				printf("\n");
			else
				printf(" ");
		}

		printf("\n\n");
	}

	return 0;
}


/**
 * @brief write a buffer to a output file
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

int write_cmp_data_file(const void *buf, uint32_t buf_size, const char
		    *output_prefix, const char *name_extension, int verbose)
{
	unsigned int i;
	FILE *fp;
	uint8_t *p = (uint8_t *)buf;

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
			++d;
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
 * @brief convert RDCU SRAM Address sting to integer
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
 * @param dep_str	description sting of the read in value
 * @param val_str	value sting contain the value to convert in uint32_t
 * @param red_val	address for storing the converted result
 *
 * @returns 0 on success, error otherwise
 *
 * @see https://eternallyconfuzzled.com/atoi-is-evil-c-learn-why-atoi-is-awful
 */

static int atoui32(const char *dep_str , const char *val_str, uint32_t *red_val)
{
	long temp;
	char *end = NULL;

	if (!dep_str)
		return -1;

	if (!val_str)
		return -1;

	if (!red_val)
		return -1;

	temp = strtol(val_str, &end, 10);
	if (end != val_str && errno != ERANGE && temp >= 0 && temp <=
	    UINT32_MAX) {
		*red_val = (uint32_t)temp;
		return 0;
	} else {
		fprintf(stderr, "%s: Error read in %s.\n", PROGRAM_NAME,
			dep_str);
		*red_val = 0;
		return -1;
	}
}


/**
* @brief  parse a file containing a compressing configuration
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

	if (!fp)
		return -1;

	if (!cfg)
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (line[0] == '#' || line[0] == '\n')
			continue; /* skip #'ed or empty lines */

		if (!strchr(line, '\n')){ /* detect a to long line */
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


		if (!strcmp(token1, "cmp_mode")) {
			if (isalpha(*token2)) { /* check if mode is given as text or val*/
				if (!strcmp(token2, "MODE_RAW")) {
					cfg->cmp_mode = 0;
					continue;
				}
				if (!strcmp(token2, "MODE_MODEL_ZERO")) {
					cfg->cmp_mode = 1;
					continue;
				}
				if (!strcmp(token2, "MODE_DIFF_ZERO")) {
					cfg->cmp_mode = 2;
					continue;
				}
				if (!strcmp(token2, "MODE_MODEL_MULTI")) {
					cfg->cmp_mode = 3;
					continue;
				}
				if (!strcmp(token2, "MODE_DIFF_MULTI")) {
					cfg->cmp_mode = 4;
					continue;
				}
				fprintf(stderr, "%s: Error read in cmp_mode.\n",
					PROGRAM_NAME);
			} else {
				if (atoui32(token1, token2, &cfg->cmp_mode))
					return -1;
				continue;
			}
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
			continue;
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

	if (parse_cfg(fp, cfg))
		return -1;

	fclose(fp);

	if (verbose_en) {
		printf("\n\n");
		print_cmp_cfg(cfg);
		printf("\n");
	}

	if (cfg->buffer_length < cfg->samples/2) {
		fprintf(stderr,
			"%s: warning: buffer_length: %u is relative small to the chosen samples parameter of %u.\n",
			PROGRAM_NAME, cfg->buffer_length, cfg->samples);
	}

	return 0;
}


/**
 * @brief  parse a file containing a decompression information
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

	if (!fp)
		return -1;

	if (!info)
		return -1;

	while (fgets(line, sizeof(line), fp) != NULL) {
		if (line[0] == '#' || line[0] == '\n')
			continue; /* skip #'ed or empty lines */

		if (!strchr(line, '\n')){ /* detect a to long line */
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
			if (isalpha(*token2)) { /* check if mode is given as text or val*/
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
			continue;
		}
		if (!strcmp(token1, "golomb_par_used")) {
			uint32_t tmp;

			if (atoui32(token1, token2, &tmp))
				return -1;
			info->golomb_par_used = tmp;
			continue;
		}
		if (!strcmp(token1, "samples_used")) {
			if (atoui32(token1, token2, &info->samples_used))
				return -1;
			continue;
		}
		if (!strcmp(token1, "cmp_size")) {
			if (atoui32(token1, token2, &info->cmp_size))
				return -1;
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
				fprintf(stderr,"%s: Error read in "
				       "rdcu_cmp_adr_used\n",
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
	FILE *fp;

	if (!file_name)
		abort();

	if (!info)
		abort();

	if (strstr(file_name, ".cfg")) {
	    fprintf(stderr, "%s: %s: .cfg file extension found on decompression information file. You may have selected the wrong file.\n",
		    PROGRAM_NAME, file_name);
	}

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, file_name,
			strerror(errno));
		return -1;
	}

	parse_info(fp, info);

	fclose(fp);

	if (verbose_en) {
		printf("\n\n");
		print_cmp_info(info);
		printf("\n");
	}

	return 0;
}


/**
 * @brief reads a hex encoded uint8_t data (or model) file, returns a buffer
 *	containing the read in data
 *
 * @note data must be encode has 4 hex digits separated by a white space or new
 *	line character e.g. "ABCD 1234 AB12\n"
 *
 * @param file_name	data file name
 * @param samples	amount of uint16_t data samples to read in
 * @param verbose_en	print verbose output if not zero
 *
 * @returns address to the buffer containing the read in data, NULL on error
 */

uint8_t *read_file8(const char *file_name, uint32_t samples, int verbose_en)
{
	uint32_t i;
	FILE *fp;
	uint8_t *buffer;
	char tmp_str[4]; /* 6 = 2 hex digit's + 1 white space + 1 \0 */

	if (!file_name)
		abort();

	if (!samples)
		return NULL;

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, file_name,
			strerror(errno));
		return NULL;
	}

	buffer = (uint8_t *)malloc(samples * SAM2BYT);
	if (buffer == NULL) {
		fprintf(stderr, "%s: Error allocating memory.\n", PROGRAM_NAME);
		fclose(fp);
		return NULL;
	}

	for (i = 0; i < samples; i++) {
		int j;
		unsigned long read_val;
		char *end;

		if (!fgets(tmp_str, sizeof(tmp_str), fp)) {
			fprintf(stderr, "%s: %s: Error: The files does not contain enough data as given by the samples parameter.\n",
				PROGRAM_NAME, file_name);
			goto error;
		}

		if (tmp_str[0] == '#' || tmp_str[0] == '\n') {
			i--; /* a comment or empty line does not count as sample */
			continue;
		}

		for (j = 0; j < 2; j++) {
			if (!isxdigit(tmp_str[j])) {
				fprintf(stderr, "%s: %s: Error: The data are not correct formatted. Expected format is like: 12 AB 23 CD .. ..\n", PROGRAM_NAME,
					file_name);
				goto error;
			}
		}

		if (!(isspace(tmp_str[2]) || tmp_str[2] == '\n')) {
			fprintf(stderr, "%s: %s: Error: The data are not correct formatted. Expected format is: 12 AB 23 CD .. ..\n", PROGRAM_NAME, file_name);
			goto error;
		}

		read_val = strtoul(tmp_str, &end, 16);

		if (tmp_str == end || errno == ERANGE || read_val > UINT8_MAX) {
			fprintf(stderr, "%s: %s: Error: The data can not be converted to integer.\n",
				PROGRAM_NAME, file_name);
			goto error;
		}

		buffer[i] = (uint16_t)read_val;

		if (verbose_en) {
			if (i == 0)
				printf("\n\n");

			printf("%02X", buffer[i]);
			if (i && !((i + 1) % 28))
				printf("\n");
			else
				printf(" ");
		}
	}
	if (verbose_en)
		printf("\n\n");

	fgets(tmp_str, sizeof(tmp_str), fp);
	if (!feof(fp) && tmp_str[0] == '\n') /* read last line break */
		fgets(tmp_str, sizeof(tmp_str), fp);

	if (!feof(fp)) {
		fprintf(stderr, "%s: %s: Warning: The file may contain more data than specified by the samples parameter.\n",
			PROGRAM_NAME, file_name);
	}

	fclose(fp);

	return buffer;

	error:
		free(buffer);
		fclose(fp);
		return NULL;
}


/**
 * @brief reads a hex encoded uint16_t data (or model) file, returns a buffer
 *	containing the read in data
 *
 * @note data must be encode has 4 hex digits separated by a white space or new
 *	line character e.g. "ABCD 1234 AB12\n"
 *
 * @param file_name	data file name
 * @param samples	amount of uint16_t data samples to read in
 * @param verbose_en	print verbose output if not zero
 *
 * @returns address to the buffer containing the read in data, NULL on error
 */

uint16_t *read_file16(const char *file_name, uint32_t samples, int verbose_en)
{
	size_t i;
	uint16_t *buf = (uint16_t *)read_file8(file_name, samples*2, verbose_en);

	if (!buf)
		return NULL;

	for (i=0; i < samples; i++)
		be16_to_cpus(&buf[i]);

	return buf;
}


/**
 * @brief reads a hex encoded uint32_t data file, returns a buffer containing
 *	the read in data
 *
 * @note data must be encode has 2 hex digits separated by a white space or new
 *	line character e.g. AB CD 12 34
 *
 * @param file_name	data file name
 * @param buf_len	amount of uint32_t data samples to read in
 * @param verbose_en	print verbose output if not zero
 *
 * @returns address to the buffer containing the read in data, NULL on error
 */

uint32_t *read_file32(const char *file_name, uint32_t buf_len, int verbose_en)
{
	size_t i;

	uint32_t *buf = (uint32_t *)read_file8(file_name, buf_len*sizeof(uint32_t),
					       verbose_en);

	if (!buf)
		return NULL;

	for (i=0; i < buf_len; i++)
		be32_to_cpus(&buf[i]);

	return buf;
}


/**
 * @brief prints config struct
 *
 * @param cfg		configuration to print
 * @param rdcu_cfg	if set additional RDCU parameter are printed as well
 */

void print_cfg(const struct cmp_cfg *cfg, int rdcu_cfg)
{
	printf("#-------------------------------------------------------------------------------\n");
	printf("# Default Configuration File\n");
	printf("#-------------------------------------------------------------------------------\n");
	printf("# Selected compression mode\n");
	printf("# 0: raw mode\n");
	printf("# 1: model mode with zero escape symbol mechanism\n");
	printf("# 2: 1d differencing mode without input model with zero escape symbol mechanism\n");
	printf("# 3: model mode with multi escape symbol mechanism\n");
	printf("# 4: 1d differencing mode without input model multi escape symbol mechanism\n");
	printf("\n");
	printf("cmp_mode = %u\n", cfg->cmp_mode);
	printf("\n");
	printf("#-------------------------------------------------------------------------------\n");
	printf("# Number of samples (16 bit value) to compress, length of the data and model buffer\n");
	printf("\n");
	printf("samples = %u\n", cfg->samples);
	printf("\n");
	printf("#-------------------------------------------------------------------------------\n");
	printf("# Length of the compressed data buffer in number of samples (16 bit values)\n");
	printf("\n");
	printf("buffer_length = %u\n", cfg->buffer_length);
	printf("\n");
	printf("#-------------------------------------------------------------------------------\n");
	printf("# Golomb parameter for dictionary selection\n");
	printf("\n");
	printf("golomb_par = %u\n", cfg->golomb_par);
	printf("\n");
	printf("#-------------------------------------------------------------------------------\n");
	printf("# Spillover threshold for encoding outliers\n");
	printf("\n");
	printf("spill = %u\n", cfg->spill);
	printf("\n");
	printf("#-------------------------------------------------------------------------------\n");
	printf("# Model weighting parameter\n");
	printf("\n");
	printf("model_value = %u\n", cfg->model_value);
	printf("\n");
	printf("#-------------------------------------------------------------------------------\n");
	printf("# Number of noise bits to be rounded\n");
	printf("\n");
	printf("round = %u\n", cfg->round);
	printf("\n");
	printf("#-------------------------------------------------------------------------------\n");

	if (rdcu_cfg) {
		printf("#-------------------------------------------------------------------------------\n");
		printf("# Hardware Compressor Settings (not need for SW compression)\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("\n");
		printf("# Adaptive 1 Golomb parameter; HW only\n");
		printf("\n");
		printf("ap1_golomb_par = %u\n", cfg->ap1_golomb_par);
		printf("\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("# Adaptive 1 spillover threshold; HW only\n");
		printf("\n");
		printf("ap1_spill = %u\n", cfg->ap1_spill);
		printf("\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("# Adaptive 2 Golomb parameter; HW only\n");
		printf("\n");
		printf("ap2_golomb_par = %u\n", cfg->ap2_golomb_par);
		printf("\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("# Adaptive 2 spillover threshold; HW only\n");
		printf("\n");
		printf("ap2_spill = %u\n", cfg->ap2_spill);
		printf("\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("# RDCU data to compress start address, the first data address in the RDCU SRAM; HW only\n");
		printf("\n");
		printf("rdcu_data_adr = 0x%06X\n",
		       cfg->rdcu_data_adr);
		printf("\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("# RDCU model start address, the first model address in the RDCU SRAM\n");
		printf("\n");
		printf("rdcu_model_adr = 0x%06X\n",
		       cfg->rdcu_model_adr);
		printf("\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("# RDCU updated model start address, the address in the RDCU SRAM where the updated model is stored\n");
		printf("\n");
		printf("rdcu_new_model_adr = 0x%06X\n",
		       cfg->rdcu_new_model_adr);
		printf("\n");
		printf("#-------------------------------------------------------------------------------\n");
		printf("# RDCU compressed data start address, the first output data address in the RDCU SRAM\n");
		printf("\n");
		printf("rdcu_buffer_adr = 0x%06X\n", cfg->rdcu_buffer_adr);
		printf("\n");
		printf("#-------------------------------------------------------------------------------\n");
	}
}

/**
 * @brief write a decompression information structure to a file
 *
 * @param info	 compressor information contains information of an executed
 *		 compression
 * @param output_prefix prefix of the written file (.info is added to the prefix)
 * @param machine_cfg - if set write additional RDCU parameter in the file
 *
 * @returns 0 on success, error otherwise
 */

int write_info(const struct cmp_info *info, const char *output_prefix,
	       int machine_cfg)
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

	if (machine_cfg) {
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
