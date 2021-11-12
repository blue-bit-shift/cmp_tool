/**
 * @file   cmp_tool.c
 * @author Johannes Seelig (johannes.seelig@univie.ac.at)
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
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
 * @brief command line tool for PLATO ICU/RDCU compression/decompression
 * @see README.md
 * @see Data Compression User Manual PLATO-UVIE-PL-UM-0001
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>

#include "include/cmp_tool_lib.h"
#include "include/cmp_icu.h"
#include "include/decmp.h"
#include "include/cmp_guess.h"
#include "include/rdcu_pkt_to_file.h"


#define VERSION "0.06"

#define BUFFER_LENGTH_DEF_FAKTOR 2


/*
 * For long options that have no equivalent short option, use a
 * non-character as a pseudo short option, starting with CHAR_MAX + 1.
 */
enum {
	DIFF_CFG_OPTION = CHAR_MAX + 1,
	GUESS_OPTION,
	GUESS_LEVEL,
	RDCU_PKT_OPTION,
	LAST_INFO,

};

static const struct option long_options[] = {
	{"rdcu_par", no_argument, NULL, 'a'},
	{"model_cfg", no_argument, NULL, 'n'},
	{"help", no_argument, NULL, 'h'},
	{"verbose", no_argument, NULL, 'v'},
	{"version", no_argument, NULL, 'V'},
	{"rdcu_pkt", no_argument, NULL, RDCU_PKT_OPTION},
	{"diff_cfg", no_argument, NULL, DIFF_CFG_OPTION},
	{"guess", required_argument, NULL, GUESS_OPTION},
	{"guess_level", required_argument, NULL, GUESS_LEVEL},
	{"last_info", required_argument, NULL, LAST_INFO},
	{NULL, 0, NULL, 0}
};

/* prefix of the generated output file names */
static const char *output_prefix = DEFAULT_OUTPUT_PREFIX;

/* if non zero additional RDCU parameters are included in the compression
 * configuration and decompression information files */
static int print_rdcu_cfg;

/* if non zero generate RDCU setup packets */
static int rdcu_pkt_mode;

/* file name of the last compression information file to generate parallel RDCU
 * setup packets */
static const char *last_info_file_name;

/* if non zero print additional verbose output */
static int verbose_en;


/* find a good set of compression parameters for a given dataset */
static int guess_cmp_pars(struct cmp_cfg *cfg, const char *guess_cmp_mode,
			  int guess_level);

/* compress the data and write the results to files */
static int compression(struct cmp_cfg *cfg, struct cmp_info *info);

/* decompress the data and write the results in file(s)*/
static int decompression(uint32_t *decomp_input_buf, uint16_t *input_model_buf,
			 struct cmp_info *info);


/**
 * @brief This is the main function of the compression / decompression tool
 *
 * @param argc argument count
 * @param argv argument vector
 * @see README.md
 *
 * @returns EXIT_SUCCESS on success, EXIT_FAILURE on error
 */

int main(int argc, char **argv)
{
	int opt;
	int error;

	const char *cfg_file_name = NULL;
	const char *info_file_name = NULL;
	const char *data_file_name = NULL;
	const char *model_file_name = NULL;
	const char *guess_cmp_mode = NULL;
	const char *program_name = argv[0];

	int cmp_operation = 0;
	int print_model_cfg = 0;
	int guess_operation = 0;
	int guess_level = DEFAULT_GUESS_LEVEL;
	int print_diff_cfg = 0;

	struct cmp_cfg cfg = {0}; /* compressor configuration struct */
	struct cmp_info info = {0}; /* decompression information struct */

	uint32_t *decomp_input_buf = NULL;
	uint16_t *input_model_buf = NULL;
	ssize_t size;

	/* show help if no arguments are provided */
	if (argc < 2) {
		print_help(program_name);
		exit(EXIT_FAILURE);
	}

	while ((opt = getopt_long(argc, argv, "ac:d:hi:m:no:vV", long_options,
				  NULL)) != -1) {
		switch (opt) {
		case 'a': /* --rdcu_par */
			print_rdcu_cfg = 1;
			break;
		case 'c':
			cmp_operation = 1;
			cfg_file_name = optarg;
			break;
		case 'd':
			data_file_name = optarg;
			break;
		case 'h': /* --help */
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		case 'i':
			info_file_name = optarg;
			break;
		case 'm': /* read model */
			model_file_name = optarg;
			break;
		case 'n': /* --model_cfg */
			print_model_cfg = 1;
			break;
		case 'o':
			output_prefix = optarg;
			break;
		case 'v': /* --verbose */
			verbose_en = 1;
			break;
		case 'V': /* --version */
			printf("%s version %s\n", PROGRAM_NAME, VERSION);
			exit(EXIT_SUCCESS);
			break;
		case DIFF_CFG_OPTION:
			print_diff_cfg = 1;
			break;
		case GUESS_OPTION:
			guess_operation = 1;
			guess_cmp_mode = optarg;
			break;
		case GUESS_LEVEL:
			guess_level = atoi(optarg);
			break;
		case RDCU_PKT_OPTION:
			rdcu_pkt_mode = 1;
			break;
		case LAST_INFO:
			rdcu_pkt_mode = 1;
			last_info_file_name = optarg;
			break;
		default:
			print_help(program_name);
			exit(EXIT_FAILURE);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0) {
		printf("%s: To many arguments.\n", PROGRAM_NAME);
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}
#if 0
	if (argc > 0) {
		if(!data_file_name)
			data_file_name = argv[0];
		else {
			printf("You can define the data file using either the -d option or the first argument, but not both.\n");
			print_help(program_name);
			exit(EXIT_FAILURE);
		}
	}
	if (argc > 1) {
		if(!model_file_name)
			model_file_name = argv[1];
		else {
			printf("You can define the model file using either the -m option or the second argument, but not both.\n");
			print_help(program_name);
			exit(EXIT_FAILURE);
		}
	}
#endif

	if (print_model_cfg == 1) {
		print_cfg(&DEFAULT_CFG_MODEL, print_rdcu_cfg);
		exit(EXIT_SUCCESS);
	}

	if (print_diff_cfg == 1) {
		print_cfg(&DEFAULT_CFG_DIFF, print_rdcu_cfg);
		exit(EXIT_SUCCESS);
	}

	printf("#########################################################\n");
	printf("### PLATO Compression/Decompression Tool Version %s ###\n",
	       VERSION);
	printf("#########################################################\n\n");

	if (!data_file_name) {
		fprintf(stderr, "%s: No data file (-d option) specified.\n",
			PROGRAM_NAME);
			exit(EXIT_FAILURE);
	}

	if (!cfg_file_name && !info_file_name && !guess_operation) {
		fprintf(stderr, "%s: No configuration file (-c option) or decompression information file (-i option) specified.\n",
			PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}

	/* read input data and .cfg or .info */
	if (cmp_operation || guess_operation) { /* compression mode */
		if (cmp_operation) {
			/* printf("### Compression ###\n"); */
			printf("Importing configuration file %s ... ", cfg_file_name);
			error = read_cmp_cfg(cfg_file_name, &cfg, verbose_en);
			if (error)
				goto fail;
			else
				printf("DONE\n");

		}

		printf("Importing data file %s ... ", data_file_name);

		/* count the samples in the data file when samples == 0 */
		if (cfg.samples == 0) {
			size = read_file16(data_file_name, NULL, 0, 0);
			if (size < 0)
				goto fail;
			if (size%size_of_a_sample(cfg.cmp_mode) != 0) {
				fprintf(stderr, "\n%s: %s: Error: The data file is not correct formatted. Expected multiple of %zu hex words.\n",
					PROGRAM_NAME, data_file_name, size_of_a_sample(cfg.cmp_mode));
				goto fail;
			}

			cfg.samples = size/size_of_a_sample(cfg.cmp_mode);
			printf("\nNo samples parameter set. Use samples = %u.\n... ",
			       cfg.samples);
		}

		cfg.input_buf = malloc(cfg.samples * size_of_a_sample(cfg.cmp_mode));
		if (!cfg.input_buf) {
			fprintf(stderr, "%s: Error allocating memory for input data buffer.\n", PROGRAM_NAME);
			goto fail;
		}

		size = read_file16(data_file_name, cfg.input_buf, cfg.samples,
				   verbose_en);
		if (size < 0)
			goto fail;
		else
			printf("DONE\n");

	} else { /* decompression mode*/
		uint32_t cmp_size_byte;

		/* printf("### Decompression ###\n\n"); */
		printf("Importing decompression information file %s ... ", info_file_name);

		error  = read_cmp_info(info_file_name, &info, verbose_en);
		if (error)
			goto fail;
		else
			printf("DONE\n");

		printf("Importing compressed data file %s ... ", data_file_name);
		/* cmp_size unit is in bits */
		cmp_size_byte = (info.cmp_size + 31) / 8;
		decomp_input_buf = malloc(cmp_size_byte);
		if (!decomp_input_buf) {
			fprintf(stderr, "%s: Error allocating memory for decompression input buffer.\n", PROGRAM_NAME);
			goto fail;
		}

		size = read_file32(data_file_name, decomp_input_buf,
				   cmp_size_byte/4, verbose_en);
		if (size < 0)
			goto fail;
		else
			printf("DONE\n");
	}

	/* read in model */
	if ((cmp_operation && model_mode_is_used(cfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(info.cmp_mode_used)) ||
	    (guess_operation && model_file_name)) {
		uint32_t model_length;

		if (!model_file_name) {
			fprintf(stderr, "%s: No model file (-m option) specified.\n", PROGRAM_NAME);
			goto fail;
		}

		printf("Importing model file %s ... ", model_file_name);

		if (cmp_operation || guess_operation)
			model_length = cfg.samples;
		else
			model_length = info.samples_used;

		input_model_buf = malloc(model_length * size_of_a_sample(cfg.cmp_mode));
		if (!input_model_buf) {
			fprintf(stderr, "%s: Error allocating memory for model buffer.\n", PROGRAM_NAME);
			goto fail;
		}

		size = read_file16(model_file_name, input_model_buf, model_length, verbose_en);
		if (size < 0)
			goto fail;
		else
			printf("DONE\n");

		if (cmp_operation || guess_operation)
			cfg.model_buf = input_model_buf;
	}

	if (guess_operation) {
		error = guess_cmp_pars(&cfg, guess_cmp_mode, guess_level);
		if (error)
			goto fail;
	} else if (cmp_operation) { /* perform a compression */
		error = compression(&cfg, &info);
		if (error)
			goto fail;
	} else { /* perform a decompression */
		error = decompression(decomp_input_buf, input_model_buf, &info);
		if (error)
			goto fail;
	}

	/* write our the updated model for compressed or decompression */
	if (!guess_operation &&
	    ((cmp_operation && model_mode_is_used(cfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(info.cmp_mode_used)))) {
		printf("Write updated model to file %s_upmodel.dat ... ",
		       output_prefix);
		error = write_to_file16(input_model_buf, info.samples_used,
					output_prefix, "_upmodel.dat", verbose_en);
		if (error)
			goto fail;
		else
			printf("DONE\n");
	}

	free(cfg.input_buf);
	free(decomp_input_buf);
	free(input_model_buf);

	exit(EXIT_SUCCESS);

fail:
	printf("FAILED\n");

	free(cfg.input_buf);
	free(decomp_input_buf);
	free(input_model_buf);

	exit(EXIT_FAILURE);
}


/* generate packets to setup a RDCU compression */
static int gen_rdcu_write_pkts(struct cmp_cfg *cfg)
{
	int error;

	error = init_rmap_pkt_to_file();
	if (error) {
		fprintf(stderr, "%s: Read RMAP packet config file .rdcu_pkt_mode_cfg failed.\n",
			PROGRAM_NAME);
		return -1;
	}

	if (last_info_file_name) {
		/* generation of packets for parallel read/write RDCU setup */
		struct cmp_info last_info = {0};

		error  = read_cmp_info(last_info_file_name, &last_info, verbose_en);
		if (error) {
			fprintf(stderr, "%s: %s: Importing last decompression information file failed.\n",
				PROGRAM_NAME, last_info_file_name);
			return -1;
		}

		error = gen_rdcu_parallel_pkts(cfg, &last_info);
		if (error)
			return -1;
	}

	/* generation of packets for non-parallel read/write RDCU setup */
	error = gen_write_rdcu_pkts(cfg);
	if (error)
		return -1;

	return 0;
}


/* find a good set of compression parameters for a given dataset */
static int guess_cmp_pars(struct cmp_cfg *cfg, const char *guess_cmp_mode,
			  int guess_level)
{
	int error;
	uint32_t cmp_size;
	float cr;

	printf("Search for a good set of compression parameters (level: %d) ... ", guess_level);
	if (!strcmp(guess_cmp_mode, "RDCU")) {
		if (cfg->model_buf)
			cfg->cmp_mode = CMP_GUESS_DEF_MODE_MODEL;
		else
			cfg->cmp_mode = CMP_GUESS_DEF_MODE_DIFF;
	} else {
		error = cmp_mode_parse(guess_cmp_mode, &cfg->cmp_mode);
		if (error) {
			fprintf(stderr, "%s: Error: unknown compression mode: %s\n", PROGRAM_NAME, guess_cmp_mode);
			return -1;
		}
	}
	if (model_mode_is_used(cfg->cmp_mode) && !cfg->model_buf) {
		fprintf(stderr, "%s: Error: model mode needs model data (-m option)\n", PROGRAM_NAME);
		return -1;
	}

	cmp_size = cmp_guess(cfg, guess_level);
	if (!cmp_size)
		return -1;
	printf("DONE\n");

	printf("Write the guessed compression configuration to file %s.cfg ... ", output_prefix);
	error = write_cfg(cfg, output_prefix, print_rdcu_cfg, verbose_en);
	if (error)
		return -1;
	printf("DONE\n");

	cr = (8.0 * cfg->samples * size_of_a_sample(cfg->cmp_mode))/cmp_size;
	printf("Guessed parameters can compress the data with a CR of %.2f.\n", cr);

	return 0;
}


/* compress the data and write the results to files */
static int compression(struct cmp_cfg *cfg, struct cmp_info *info)
{
	int error;
	uint32_t cmp_size_byte;

	cfg->icu_output_buf = NULL;

	if (cfg->buffer_length == 0) {
		cfg->buffer_length = (cfg->samples+1) * BUFFER_LENGTH_DEF_FAKTOR; /* +1 to prevent malloc(0)*/
		printf("No buffer_length parameter set. Use buffer_length = %u as compression buffer size.\n",
		       cfg->buffer_length);
	}

	if (rdcu_pkt_mode) {
		printf("Generate compression setup packets ...\n");
		error = gen_rdcu_write_pkts(cfg);
		if (error)
			goto error_cleanup;
		else
			printf("... DONE\n");
	}

	printf("Compress data ... ");

	cfg->icu_output_buf = malloc(cfg->buffer_length * size_of_a_sample(cfg->cmp_mode));
	if (cfg->icu_output_buf == NULL) {
		fprintf(stderr, "%s: Error allocating memory for output buffer.\n", PROGRAM_NAME);
		goto error_cleanup;
	}

	error = icu_compress_data(cfg, info);
	if (error || info->cmp_err != 0) {
		printf("\nCompression error 0x%02X\n... ", info->cmp_err);
		if ((info->cmp_err >> SMALL_BUFFER_ERR_BIT) & 1U)
			fprintf(stderr, "%s: the buffer for the compressed data is too small. Try a larger buffer_length parameter.\n", PROGRAM_NAME);
		goto error_cleanup;
	} else
		printf("DONE\n");

	if (rdcu_pkt_mode) {
		printf("Generate the read results packets ... ");
		error = gen_read_rdcu_pkts(info);
		if (error)
			goto error_cleanup;
		else
			printf("DONE\n");
	}

	printf("Write compressed data to file %s.cmp ... ", output_prefix);
	/* length of cmp_size in bytes words (round up to 4 bytes) */
	cmp_size_byte = (info->cmp_size + 31)/32 * 4;
	error = write_cmp_data_file(cfg->icu_output_buf, cmp_size_byte,
				    output_prefix, ".cmp", verbose_en);
	if (error)
		goto error_cleanup;
	else
		printf("DONE\n");
	free(cfg->icu_output_buf);
	cfg->icu_output_buf = NULL;

	printf("Write decompression information to file %s.info ... ",
	       output_prefix);
	error = write_info(info, output_prefix, print_rdcu_cfg);
	if (error)
		goto error_cleanup;
	else
		printf("DONE\n");

	if (verbose_en) {
		printf("\n");
		print_cmp_info(info);
		printf("\n");
	}

	return 0;

error_cleanup:
	free(cfg->icu_output_buf);
	return -1;
}


/* decompress the data and write the results in file(s)*/
static int decompression(uint32_t *decomp_input_buf, uint16_t *input_model_buf,
			 struct cmp_info *info)
{
	int error;
	uint16_t *decomp_output;

	printf("Decompress data ... ");

	if (info->samples_used == 0)
		return 0; /* nothing to decompress */

	decomp_output = malloc(info->samples_used *
			       size_of_a_sample(info->cmp_mode_used));
	if (decomp_output == NULL) {
		fprintf(stderr, "%s: Error allocating memory for decompressed data.\n", PROGRAM_NAME);
		return -1;
	}

	error = decompress_data(decomp_input_buf, input_model_buf, info,
				decomp_output);
	if (error) {
		free(decomp_output);
		return -1;
	}
	printf("DONE\n");

	printf("Write decompressed data to file %s.dat ... ", output_prefix);

	error = write_to_file16(decomp_output, info->samples_used,
				output_prefix, ".dat", verbose_en);
	free(decomp_output);
	if (error)
		return -1;

	printf("DONE\n");

	return 0;
}
