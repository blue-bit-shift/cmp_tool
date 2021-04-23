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

#include "include/tool_lib.h"
#include "include/cmp_icu.h"
#include "include/cmp_rdcu.h"
#include "include/decmp.h"

#define VERSION "0.05"

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
	int32_t opt;

	const char *cfg_file_name = NULL;
	const char *info_file_name = NULL;
	const char *data_file_name = NULL;
	const char *model_file_name = NULL;
	const char *last_info_file_name = NULL;
	const char *output_prefix = DEFAULT_OUTPUT_PREFIX;

	int print_model_cfg = 0;
	int print_diff_cfg = 0;
	int print_rdcu_cfg = 1;
	int cmp_operation = 0;
	int verbose_en = 0;
	int rdcu_pkt_mode = 0;

	struct cmp_cfg cfg = {0}; /* compressor configuration struct */
	struct cmp_info info = {0}; /* decompression information struct */
	struct cmp_info last_info = {0}; /* last decompression information struct */
	int error;

	uint32_t *decomp_input_buf = NULL;
	uint16_t *input_model_buf = NULL;

	/* show help if no arguments are provided */
	if (argc < 2) {
		Print_Help(*argv);
		exit(EXIT_FAILURE);
	}

	while ((opt = getopt_long (argc, argv, "ac:d:hi:m:no:vV", long_options,
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
		if (error) {
			printf("FAILED\n");
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");


		printf("Importing data file %s ... ", data_file_name);

		cfg.input_buf = read_file16(data_file_name, cfg.samples,
					   verbose_en);
		if (!cfg.input_buf) {
			printf("FAILED\n");
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");

	} else { /* decompression mode*/
		uint32_t cmp_size_byte;

		printf("### Decompression ###\n\n");
		printf("Importing decompression information file ... ");

		error  = read_cmp_info(info_file_name, &info, verbose_en);
		if (error) {
			printf("FAILED\n");
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");


		printf("Importing compressed data file %s ... ", data_file_name);

		cmp_size_byte = (info.cmp_size + 31) / 8;

		decomp_input_buf = read_file32(data_file_name, cmp_size_byte/4,
					      verbose_en);
		if (!decomp_input_buf) {
			printf("FAILED\n");
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");
	}

	/* read in model for compressed and decompression */
	if ((cmp_operation && model_mode_is_used(cfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(info.cmp_mode_used))) {
		uint32_t model_length;

		if (cmp_operation)
			model_length = cfg.samples;
		else
			model_length = info.samples_used;

		if (!model_file_name) {
			fprintf(stderr, "%s: No model file (-m "
				"option) specified.\n", PROGRAM_NAME);
			exit(EXIT_FAILURE);
		}

		printf("Importing model file %s ... ", model_file_name);


		input_model_buf = read_file16(model_file_name, model_length,
					     verbose_en);
		if (!input_model_buf) {
			printf("FAILED\n");
			free(cfg.input_buf);
			free(decomp_input_buf);
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");

		if (cmp_operation)
			cfg.model_buf = input_model_buf;
	}

	if (cmp_operation) { /* perform a compression */
		uint32_t cmp_size_byte;

		if (rdcu_pkt_mode) {
			printf("Generate compression setup packets ... \n");
			error = init_rmap_pkt_to_file();
			if (error) {
				printf("... FAILED\n");
				fprintf(stderr, "%s: Read RMAP packet config file .rdcu_pkt_mode_cfg failed.\n",
					PROGRAM_NAME);
				free(cfg.input_buf);
				free(input_model_buf);
				exit(EXIT_FAILURE);
			}

			if (last_info_file_name) {
				error  = read_cmp_info(last_info_file_name,
						       &last_info, verbose_en);
				if (error) {
					printf("... FAILED\n");
					fprintf(stderr, "%s: %s: Importing last decompression information file failed.\n",
						PROGRAM_NAME, last_info_file_name);
					exit(EXIT_FAILURE);
				}

				error = gen_rdcu_parallel_pkts(&cfg, &last_info);
				if (error) {
					printf("... FAILED\n");
					free(cfg.input_buf);
					free(input_model_buf);
					exit(EXIT_FAILURE);
				}
			}

			error = gen_write_rdcu_pkts(&cfg);

			if (error) {
				printf("... FAILED\n");
				free(cfg.input_buf);
				free(input_model_buf);
				exit(EXIT_FAILURE);
			} else
				printf("... DONE\n");
		}

		cfg.icu_output_buf = (uint32_t *)malloc(cfg.buffer_length *
							SAM2BYT);
		if (cfg.icu_output_buf == NULL) {
			printf("%s: Error allocating Memory for Output Buffer."
			       "\n", PROGRAM_NAME);
			free(cfg.input_buf);
			free(input_model_buf);
			abort();
		}

		printf("Compress data ... ");

		error = icu_compress_data(&cfg, &info);
		free(cfg.input_buf);
		if (error || info.cmp_err != 0) {
			printf("FAILED\n");
			printf("Compression error %#X\n", info.cmp_err);
			free(input_model_buf);
			free(cfg.icu_output_buf);
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");

		if (rdcu_pkt_mode) {
			printf("Generate the read results packets ... ");
			error = gen_read_rdcu_pkts(&info);
			if (error) {
				printf("FAILED\n");
				free(input_model_buf);
				free(cfg.icu_output_buf);
				exit(EXIT_FAILURE);
			} else
				printf("DONE\n");
		}
		printf("Write compressed data to file %s.cmp ... ",
		       output_prefix);

		/* length of cmp_size in bytes words (round up to 4 bytes) */
		cmp_size_byte = (info.cmp_size + 31)/32 * 4;

		error = write_cmp_data_file(cfg.icu_output_buf, cmp_size_byte,
					output_prefix, ".cmp", verbose_en);
		free(cfg.icu_output_buf);
		if (error) {
			printf("FAILED\n");
			free(input_model_buf);
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");

		if (model_mode_is_used(cfg.cmp_mode)) {
			printf("Write updated model to file  %s_upmodel.dat ... "
			       , output_prefix);
			error = write_to_file16(input_model_buf, info.samples_used,
						output_prefix, "_upmodel.dat",
						verbose_en);
			free(input_model_buf);
			if (error) {
				printf("FAILED\n");
				exit(EXIT_FAILURE);
			} else
				printf("DONE\n");
		}

		printf("Write decompression information to file %s.info ... ",
		       output_prefix);

		error = write_info(&info, output_prefix, print_rdcu_cfg);
		if (error) {
			printf("FAILED\n");
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");

		if (verbose_en) {
			printf("\n");
			print_cmp_info(&info);
			printf("\n");
		}
	} else { /* perform a decompression */
		uint16_t *decomp_output;

		decomp_output = (uint16_t *)malloc(info.samples_used * SAM2BYT);
		if (decomp_output == NULL) {
			printf("Error allocating Memory for decmpr Model Buffer\n");
			free(decomp_input_buf);
			free(input_model_buf);
			abort();
		}

		printf("Decompress data ... ");

		error = decompress_data(decomp_input_buf, input_model_buf,
					&info, decomp_output);
		free(decomp_input_buf);
		if (error != 0) {
			free(decomp_output);
			printf("FAILED\n");
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");

		printf("Write decompressed data to file %s.dat ... ",
		       output_prefix);

		error = write_to_file16(decomp_output, info.samples_used,
					output_prefix, ".dat", verbose_en);
		free(decomp_output);
		if (error) {
			printf("FAILED\n");
			exit(EXIT_FAILURE);
		} else
			printf("DONE\n");

		if (model_mode_is_used(info.cmp_mode_used)) {
			printf("Write updated model to file %s_upmodel.dat ... "
			       , output_prefix);
			error = write_to_file16(input_model_buf,
						info.samples_used,
						output_prefix, "_upmodel.dat",
						verbose_en);
			free(input_model_buf);
			if (error) {
				printf("FAILED\n");
				exit(EXIT_FAILURE);
			} else
				printf("DONE\n");
		}
	}

	exit(EXIT_SUCCESS);
}
