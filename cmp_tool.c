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

#include <limits.h>
#include <getopt.h>

#include "include/cmp_tool_lib.h"
#include "include/cmp_icu.h"
#include "include/cmp_rdcu.h"
#include "include/decmp.h"

#define VERSION "0.06"


/*
 * For long options that have no equivalent short option, use a
 * non-character as a pseudo short option, starting with CHAR_MAX + 1.
 */
enum {
	RDCU_PKT_OPTION = CHAR_MAX + 1,
	DIFF_CFG_OPTION,
	RDCU_PAR_OPTION,
	LAST_INFO,

};

static struct option const long_options[] = {
	{"rdcu_par", no_argument, NULL, 'a'},
	{"model_cfg", no_argument, NULL, 'n'},
	{"help", no_argument, NULL, 'h'},
	{"verbose", no_argument, NULL, 'v'},
	{"version", no_argument, NULL, 'V'},
	{"rdcu_pkt", no_argument, NULL, RDCU_PKT_OPTION},
	{"diff_cfg", no_argument, NULL, DIFF_CFG_OPTION},
	{"last_info", required_argument, NULL, LAST_INFO}
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


/* compress the data and write the results to files */
static int compression(struct cmp_cfg *cfg, struct cmp_info *info)
{
	int error;
	uint32_t cmp_size_byte;

	cfg->icu_output_buf = NULL;

	if (rdcu_pkt_mode) {
		printf("Generate compression setup packets ... \n");
		error = gen_rdcu_write_pkts(cfg);
		if (error)
			goto error;
		else
			printf("... DONE\n");
	}

	printf("Compress data ... ");

	if (cfg->buffer_length == 0) {
		cfg->buffer_length = (cfg->samples+1) * BUFFER_LENGTH_DEF_FAKTOR; /* +1 to prevent malloc(0)*/
		printf("No buffer_length parameter set. Use buffer_length = %u as compression buffer size.\n",
		       cfg->buffer_length);
	}

	cfg->icu_output_buf = malloc(cfg->buffer_length * size_of_a_sample(cfg->cmp_mode));
	if (cfg->icu_output_buf == NULL) {
		fprintf(stderr, "%s: Error allocating memory for output buffer.\n", PROGRAM_NAME);
		goto error;
	}

	error = icu_compress_data(cfg, info);
	if (error || info->cmp_err != 0) {
		printf("Compression error %#X\n", info->cmp_err);
		goto error;
	} else
		printf("DONE\n");

	if (rdcu_pkt_mode) {
		printf("Generate the read results packets ... ");
		error = gen_read_rdcu_pkts(info);
		if (error)
			goto error;
		else
			printf("DONE\n");
	}

	printf("Write compressed data to file %s.cmp ... ", output_prefix);
	/* length of cmp_size in bytes words (round up to 4 bytes) */
	cmp_size_byte = (info->cmp_size + 31)/32 * 4;
	error = write_cmp_data_file(cfg->icu_output_buf, cmp_size_byte,
		 		    output_prefix, ".cmp", verbose_en);
	if (error)
		goto error;
	else
		printf("DONE\n");
	free(cfg->icu_output_buf);
	cfg->icu_output_buf = NULL;

	printf("Write decompression information to file %s.info ... ",
	       output_prefix);
	error = write_info(info, output_prefix, print_rdcu_cfg);
	if (error)
		goto error;
	else
		printf("DONE\n");

	if (verbose_en) {
		printf("\n");
		print_cmp_info(info);
		printf("\n");
	}

	return 0;

error:
	printf("FAILED\n");
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
		printf("FAILED\n");
		fprintf(stderr, "%s: Error allocating memory for decompressed data.\n", PROGRAM_NAME);
		return -1;
	}

	error = decompress_data(decomp_input_buf, input_model_buf, info,
				decomp_output);
	if (error) {
		printf("FAILED\n");
		free(decomp_output);
		return -1;
	}
	else
		printf("DONE\n");

	printf("Write decompressed data to file %s.dat ... ", output_prefix);

	error = write_to_file16(decomp_output, info->samples_used,
				output_prefix, ".dat", verbose_en);
	free(decomp_output);
	if (error) {
		printf("FAILED\n");
		return -1;
	} else
		printf("DONE\n");

	return 0;
}


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

	int print_model_cfg = 0;
	int print_diff_cfg = 0;
	int cmp_operation = 0;

	struct cmp_cfg cfg = {0}; /* compressor configuration struct */
	struct cmp_info info = {0}; /* decompression information struct */

	uint32_t *decomp_input_buf = NULL;
	uint16_t *input_model_buf = NULL;
	ssize_t size;

	/* show help if no arguments are provided */
	if (argc < 2) {
		Print_Help(*argv);
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
			Print_Help(*argv);
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
			printf("%s %s\n", PROGRAM_NAME, VERSION);
			exit(EXIT_SUCCESS);
			break;
		case DIFF_CFG_OPTION:
			print_diff_cfg = 1;
			break;
		case RDCU_PKT_OPTION:
			rdcu_pkt_mode = 1;
			break;
		case LAST_INFO:
			rdcu_pkt_mode = 1;
			last_info_file_name = optarg;
			break;
		default:
			Print_Help(*argv);
			exit(EXIT_FAILURE);
			break;
		}
	}

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

	if (!cfg_file_name && !info_file_name) {
		fprintf(stderr, "%s: No configuration file (-c option) or decompression information file (-i option) specified.\n",
			PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}

	if (!data_file_name) {
		fprintf(stderr, "%s: No data file (-d option) specified.\n",
			PROGRAM_NAME);
			exit(EXIT_FAILURE);
	}

	/* read input data and .cfg or .info */
	if (cmp_operation) { /* compression mode */
		printf("### Compression ###\n");

		printf("Importing configuration file %s ... ", cfg_file_name);
		error = read_cmp_cfg(cfg_file_name, &cfg, verbose_en);
		if (error)
			goto fail;
		else
			printf("DONE\n");

		/* count the samples in the data file when samples == 0 */
		if (cfg.samples == 0) {
			size = read_file16(data_file_name, NULL, 0, 0);
			if (size < 0)
				goto fail;
			/* TODO: check size%size_of_a_sample(cfg.cmp_mode) == 0 */
			cfg.samples = size/size_of_a_sample(cfg.cmp_mode);
			printf("No samples parameter set. Use samples = %u.\n",
			       cfg.samples);
		}

		printf("Importing data file %s ... ", data_file_name);

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

		printf("### Decompression ###\n\n");
		printf("Importing decompression information file ... ");

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

	/* read in model for compressed or decompression */
	if ((cmp_operation && model_mode_is_used(cfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(info.cmp_mode_used))) {
		uint32_t model_length;

		if (!model_file_name) {
			fprintf(stderr, "%s: No model file (-m option) specified.\n", PROGRAM_NAME);
			goto fail;
		}

		printf("Importing model file %s ... ", model_file_name);

		if (cmp_operation)
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

		if (cmp_operation)
			cfg.model_buf = input_model_buf;
	}

	if (cmp_operation) { /* perform a compression */
		error = compression(&cfg, &info);
		if (error)
			goto fail;

	} else { /* perform a decompression */
		error = decompression(decomp_input_buf, input_model_buf, &info);
		if (error)
			goto fail;
	}

	/* write our the updated model for compressed or decompression */
	if ((cmp_operation && model_mode_is_used(cfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(info.cmp_mode_used))) {
		printf("Write updated model to file  %s_upmodel.dat ... " ,
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
