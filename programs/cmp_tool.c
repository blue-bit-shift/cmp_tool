/**
 * @file   cmp_tool.c
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
 * @brief command line tool for PLATO ICU/RDCU compression/decompression
 * @see README.md
 * @see Data Compression User Manual PLATO-UVIE-PL-UM-0001
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <getopt.h>

#include "cmp_tool-config.h"
#include "cmp_io.h"
#include "cmp_icu.h"
#include "cmp_chunk.h"
#include "cmp_rdcu.h"
#include "decmp.h"
#include "cmp_guess.h"
#include "cmp_entity.h"
#include "rdcu_pkt_to_file.h"
#include "cmp_data_types.h"


#define BUFFER_LENGTH_DEF_FAKTOR 2

#define DEFAULT_MODEL_ID 53264  /* random default id */


/* find a good set of compression parameters for a given dataset */
static int guess_cmp_pars(struct rdcu_cfg *rcfg, const char *guess_cmp_mode,
			  int guess_level);

/* compress chunk data and write the results to files */
static int compression_of_chunk(const void *chunk, uint32_t size, void *model,
				const struct cmp_par *chunk_par);

/* compress the data and write the results to files */
static int compression_for_rdcu(struct rdcu_cfg *rcfg);

/* decompress the data and write the results in file(s)*/
static int decompression(const struct cmp_entity *ent, uint16_t *input_model_buf);

/* create a default configuration for a compression data type */
enum cfg_default_opt {DIFF_CFG, MODEL_CFG};
static void cmp_cfg_create_default(struct rdcu_cfg *rcfg, enum cfg_default_opt mode);


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
	NO_HEADER,
	MODEL_ID,
	MODEL_COUTER
};

static const struct option long_options[] = {
	{"rdcu_par", no_argument, NULL, 'a'},
	{"model_cfg", optional_argument, NULL, 'n'},
	{"help", no_argument, NULL, 'h'},
	{"verbose", no_argument, NULL, 'v'},
	{"version", no_argument, NULL, 'V'},
	{"rdcu_pkt", no_argument, NULL, RDCU_PKT_OPTION},
	{"diff_cfg", optional_argument, NULL, DIFF_CFG_OPTION},
	{"guess", required_argument, NULL, GUESS_OPTION},
	{"guess_level", required_argument, NULL, GUESS_LEVEL},
	{"last_info", required_argument, NULL, LAST_INFO},
	{"no_header", no_argument, NULL, NO_HEADER},
	{"model_id", required_argument, NULL, MODEL_ID},
	{"model_counter", required_argument, NULL, MODEL_COUTER},
	{"binary", no_argument, NULL, 'b'},
	{NULL, 0, NULL, 0}
};

/* prefix of the generated output file names */
static const char *output_prefix = DEFAULT_OUTPUT_PREFIX;

/* if non zero additional RDCU parameters are included in the compression
 * configuration and decompression information files */
static int add_rdcu_pars;

/* if non zero generate RDCU setup packets */
static int rdcu_pkt_mode;

/* file name of the last compression information file to generate parallel RDCU
 * setup packets */
static const char *last_info_file_name;

/* option flags for file IO */
static int io_flags;

/* if non zero add a compression entity header in front of the compressed data */
static int include_cmp_header = 1;

/* model ID set by the --model_id option */
static uint32_t model_id = DEFAULT_MODEL_ID;

/* model counter set by the --model_counter option */
static uint32_t model_counter;


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

	/* buffer containing all read in compressed data for decompression */
	struct cmp_entity *decomp_entity = NULL;
	/* buffer containing the read in model */
	uint16_t *input_model_buf = NULL;
	/* size of the data to be compressed and the model of it */
	uint32_t input_size = 0;

	struct cmp_info info = {0}; /* RDCU decompression information struct */
	struct rdcu_cfg rcfg = {0}; /* RDCU compressor configuration struct */
	struct cmp_par chunk_par = {0}; /* compressor parameters for chunk compression */
	enum cmp_type cmp_type;

	/* show help if no arguments are provided */
	if (argc < 2) {
		print_help(program_name);
		exit(EXIT_FAILURE);
	}

	while ((opt = getopt_long(argc, argv, "abc:d:hi:m:no:vV", long_options,
				  NULL)) != -1) {
		switch (opt) {
		case 'a': /* --rdcu_par */
			add_rdcu_pars = 1;
			break;
		case 'b':
			io_flags |= CMP_IO_BINARY;
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
			include_cmp_header = 0;
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
			if (io_flags & CMP_IO_VERBOSE)
				io_flags |= CMP_IO_VERBOSE_EXTRA;
			io_flags |= CMP_IO_VERBOSE;
			break;
		case 'V': /* --version */
			printf("%s version %s\n", PROGRAM_NAME, CMP_TOOL_VERSION);
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
		case LAST_INFO:
			last_info_file_name = optarg;
			/* fall through */
		case RDCU_PKT_OPTION:
			rdcu_pkt_mode = 1;
			add_rdcu_pars = 1;
			/* fall through */
		case NO_HEADER:
			include_cmp_header = 0;
			break;
		case MODEL_ID:
			if (atoui32("model_id", optarg, &model_id))
				return -1;
			if (model_counter > UINT16_MAX) {
				fprintf(stderr, "%s: Error: model id value to large.\n", PROGRAM_NAME);
				return -1;
			}
			break;
		case MODEL_COUTER:
			if (atoui32("model_counter", optarg, &model_counter))
				return -1;
			if (model_counter > UINT8_MAX) {
				fprintf(stderr, "%s: Error: model counter value to large.\n", PROGRAM_NAME);
				return -1;
			}
			break;
		default:
			print_help(program_name);
			exit(EXIT_FAILURE);
			break;
		}
	}
	argc -= optind;

#ifdef ARGUMENT_INPUT_MODE

	argv += optind;
	if (argc > 2) {
		printf("%s: To many arguments.\n", PROGRAM_NAME);
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (argc > 0) {
		if (!data_file_name)
			data_file_name = argv[0];
		else {
			printf("You can define the data file using either the -d option or the first argument, but not both.\n");
			print_help(program_name);
			exit(EXIT_FAILURE);
		}
	}
	if (argc > 1) {
		if (!model_file_name)
			model_file_name = argv[1];
		else {
			printf("You can define the model file using either the -m option or the second argument, but not both.\n");
			print_help(program_name);
			exit(EXIT_FAILURE);
		}
	}
#else
	if (argc > 0) {
		printf("%s: To many arguments.\n", PROGRAM_NAME);
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}
#endif

	if (print_model_cfg || print_diff_cfg) {
		if (print_model_cfg && print_diff_cfg) {
			fprintf(stderr, "%s: Cannot use -n, --model_cfg and -diff_cfg together.\n",
				PROGRAM_NAME);
			exit(EXIT_FAILURE);
		}
		if (print_model_cfg)
			cmp_cfg_create_default(&rcfg, MODEL_CFG);
		else
			cmp_cfg_create_default(&rcfg, DIFF_CFG);
		cmp_cfg_print(&rcfg, add_rdcu_pars);
		exit(EXIT_SUCCESS);
	}

	{
		static const char str[] = "### PLATO Compression/Decompression Tool Version " CMP_TOOL_VERSION " ###\n";
		size_t str_len = strlen(str) - 1; /* -1 for \n */
		size_t i;

		for (i = 0; i < str_len; i++)
			printf("#");
		printf("\n");
		printf("%s", str);
		for (i = 0; i < str_len; i++)
			printf("#");
		printf("\n");
	}

	if (!data_file_name) {
		fprintf(stderr, "%s: No data file (-d option) specified.\n",
			PROGRAM_NAME);
			exit(EXIT_FAILURE);
	}

	if (!cfg_file_name && !info_file_name && !guess_operation && !include_cmp_header) {
		fprintf(stderr, "%s: No configuration file (-c option) or decompression information file (-i option) specified.\n",
			PROGRAM_NAME);
		exit(EXIT_FAILURE);
	}


	if (cmp_operation || guess_operation) {
		ssize_t size;

		if (cmp_operation) {
			printf("## Starting the compression ##\n");
			printf("Importing configuration file %s ... ", cfg_file_name);
			cmp_type = cmp_cfg_read(cfg_file_name, &rcfg, &chunk_par, io_flags & CMP_IO_VERBOSE);
			if (cmp_type == CMP_TYPE_ERROR)
				goto fail;
			printf("DONE\n");
		} else {
			printf("## Search for a good set of compression parameters ##\n");
			cmp_type = CMP_TYPE_RDCU; /* guess_cmp_pars only works for RDCU like compression */
		}

		printf("Importing data file %s ... ", data_file_name);
		if (cmp_type == CMP_TYPE_RDCU) {
			if (rcfg.samples == 0) {
				/* count the samples in the data file when samples == 0 */
				size = read_file_data(data_file_name, cmp_type, NULL, 0, io_flags);
				if (size <= 0 || size > INT32_MAX || (size_t)size % sizeof(uint16_t)) /* empty file is treated as an error */
					goto fail;
				rcfg.samples = (uint32_t)((size_t)size/sizeof(uint16_t));
				printf("\nNo samples parameter set. Use samples = %u.\n... ", rcfg.samples);
			}

			input_size = rcfg.samples * sizeof(uint16_t);
		} else {
			size  = read_file_data(data_file_name, cmp_type, NULL, 0, io_flags);
			if (size <= 0 || size > INT32_MAX) /* empty file is treated as an error */
				goto fail;
			input_size = (uint32_t)size;
		}

		rcfg.input_buf = malloc(input_size);
		if (!rcfg.input_buf) {
			fprintf(stderr, "%s: Error allocating memory for input data buffer.\n", PROGRAM_NAME);
			goto fail;
		}

		size = read_file_data(data_file_name, cmp_type, rcfg.input_buf,
				      input_size, io_flags);
		if (size < 0)
			goto fail;
		printf("DONE\n");

	} else { /* decompression mode*/
		printf("## Starting the decompression ##\n");
		if (info_file_name) {
			ssize_t f_size;
			size_t ent_size;

			printf("Importing decompression information file %s ... ", info_file_name);
			error  = cmp_info_read(info_file_name, &info, io_flags & CMP_IO_VERBOSE);
			if (error)
				goto fail;
			printf("DONE\n");

			printf("Importing compressed data file %s ... ", data_file_name);

			ent_size = cmp_ent_create(NULL, DATA_TYPE_IMAGETTE, info.cmp_mode_used == CMP_MODE_RAW,
						  cmp_bit_to_byte(info.cmp_size));
			if (!ent_size)
				goto fail;
			decomp_entity = calloc(1, ent_size);
			if (!decomp_entity) {
				fprintf(stderr, "%s: Error allocating memory for decompression input buffer.\n", PROGRAM_NAME);
				goto fail;
			}
			ent_size = cmp_ent_create(decomp_entity, DATA_TYPE_IMAGETTE, info.cmp_mode_used == CMP_MODE_RAW,
						  cmp_bit_to_byte(info.cmp_size));
			if (!ent_size)
				goto fail;

			f_size = read_file8(data_file_name, cmp_ent_get_data_buf(decomp_entity),
					    cmp_bit_to_byte(info.cmp_size), io_flags);
			if (f_size < 0)
				goto fail;

			error = cmp_ent_write_rdcu_cmp_pars(decomp_entity, &info, NULL);
			if (error)
				goto fail;
		} else { /* read in compressed data with header */
			ssize_t size;
			size_t buf_size;

			printf("Importing compressed data file %s ... ", data_file_name);
			size = read_file_cmp_entity(data_file_name, NULL, 0, io_flags);
			if (size < 0 || size > INT32_MAX)
				goto fail;
			/* to be save allocate at least the size of the cmp_entity struct */
			buf_size = (size_t)size;
			if (buf_size < sizeof(struct cmp_entity))
				buf_size = sizeof(struct cmp_entity);

			decomp_entity = calloc(1, buf_size);
			if (!decomp_entity) {
				fprintf(stderr, "%s: Error allocating memory for the compression entity buffer.\n", PROGRAM_NAME);
				goto fail;
			}
			size = read_file_cmp_entity(data_file_name, decomp_entity,
						    (uint32_t)size, io_flags);
			if (size < 0)
				goto fail;

			if (io_flags & CMP_IO_VERBOSE_EXTRA) {
				cmp_ent_print(decomp_entity);
				printf("\n");
			}

		}
		if (cmp_ent_get_data_type(decomp_entity) == DATA_TYPE_CHUNK)
			cmp_type = CMP_TYPE_CHUNK;
		else
			cmp_type = CMP_TYPE_RDCU;

		printf("DONE\n");
	}

	if (model_file_name && !guess_operation &&
	    ((cmp_operation && !model_mode_is_used(rcfg.cmp_mode)) ||
	     (!cmp_operation && !model_mode_is_used(cmp_ent_get_cmp_mode(decomp_entity)))))
		printf("Warring: Model file (-m option) specified but no model is used.\n");

	/* read in model */
	if ((cmp_operation && model_mode_is_used(rcfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(cmp_ent_get_cmp_mode(decomp_entity))) ||
	    (guess_operation && model_file_name)) {
		ssize_t size;
		uint32_t model_size;

		printf("Importing model file %s ... ", model_file_name ? model_file_name : "");
		if (!model_file_name) {
			fprintf(stderr, "%s: No model file (-m option) specified.\n", PROGRAM_NAME);
			goto fail;
		}

		if (cmp_operation || guess_operation)
			model_size = input_size;
		else
			model_size = cmp_ent_get_original_size(decomp_entity);


		input_model_buf = malloc(model_size);
		if (!input_model_buf) {
			fprintf(stderr, "%s: Error allocating memory for model buffer.\n", PROGRAM_NAME);
			goto fail;
		}

		size = read_file_data(model_file_name, cmp_type, input_model_buf,
				      model_size, io_flags);
		if (size < 0)
			goto fail;
		printf("DONE\n");

		rcfg.model_buf = input_model_buf;
		rcfg.icu_new_model_buf = input_model_buf; /* in-place model update */
	}

	if (guess_operation) {
		error = guess_cmp_pars(&rcfg, guess_cmp_mode, guess_level);
	} else if (cmp_operation) {
		if (cmp_type == CMP_TYPE_CHUNK)
			error = compression_of_chunk(rcfg.input_buf, input_size,
						     input_model_buf, &chunk_par);
		else
			error = compression_for_rdcu(&rcfg);
	} else {
		error = decompression(decomp_entity, input_model_buf);
	}
	if (error)
		goto fail;

	/* write our the updated model for compressed or decompression */
	if (!guess_operation &&
	    ((cmp_operation && model_mode_is_used(rcfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(cmp_ent_get_cmp_mode(decomp_entity))))) {
		uint32_t model_size;

		printf("Write updated model to file %s_upmodel.dat ... ", output_prefix);
		if (cmp_operation)
			model_size = input_size;
		else
			model_size = cmp_ent_get_original_size(decomp_entity);


		error = write_input_data_to_file(input_model_buf, model_size, cmp_type,
						 output_prefix, "_upmodel.dat", io_flags);
		if (error)
			goto fail;
		printf("DONE\n");
	}

	free(rcfg.input_buf);
	free(decomp_entity);
	free(input_model_buf);

	exit(EXIT_SUCCESS);

fail:
	printf("FAILED\n");

	free(rcfg.input_buf);
	free(decomp_entity);
	free(input_model_buf);

	exit(EXIT_FAILURE);
}


/**
 * @brief find a good set of compression parameters for a given dataset
 */

static int guess_cmp_pars(struct rdcu_cfg *rcfg, const char *guess_cmp_mode,
			  int guess_level)
{
	int error;
	uint32_t cmp_size_bit;
	double cr;
	enum cmp_data_type data_type;

	printf("Search for a good set of compression parameters (level: %d) ... ", guess_level);
	if (!strcmp(guess_cmp_mode, "RDCU")) {
		if (add_rdcu_pars)
			data_type = DATA_TYPE_IMAGETTE_ADAPTIVE;
		else
			data_type = DATA_TYPE_IMAGETTE;
		if (rcfg->model_buf)
			rcfg->cmp_mode = CMP_GUESS_DEF_MODE_MODEL;
		else
			rcfg->cmp_mode = CMP_GUESS_DEF_MODE_DIFF;
	} else {
		data_type = DATA_TYPE_IMAGETTE;
		error = cmp_mode_parse(guess_cmp_mode, &rcfg->cmp_mode);
		if (error) {
			fprintf(stderr, "%s: Error: unknown compression mode: %s\n", PROGRAM_NAME, guess_cmp_mode);
			return -1;
		}
	}
	if (model_mode_is_used(rcfg->cmp_mode) && !rcfg->model_buf) {
		fprintf(stderr, "%s: Error: model mode needs model data (-m option)\n", PROGRAM_NAME);
		return -1;
	}

	cmp_size_bit = cmp_guess(rcfg, guess_level);
	if (!cmp_size_bit)
		return -1;

	if (include_cmp_header)
		cmp_size_bit = CHAR_BIT * (cmp_bit_to_byte(cmp_size_bit) +
			cmp_ent_cal_hdr_size(data_type, rcfg->cmp_mode == CMP_MODE_RAW));

	printf("DONE\n");

	printf("Write the guessed compression configuration to file %s.cfg ... ", output_prefix);
	error = cmp_cfg_fo_file(rcfg, output_prefix, io_flags & CMP_IO_VERBOSE, add_rdcu_pars);
	if (error)
		return -1;
	printf("DONE\n");

	cr = (8.0 * rcfg->samples * sizeof(uint16_t))/cmp_size_bit;
	printf("Guessed parameters can compress the data with a CR of %.2f.\n", cr);

	return 0;
}


/**
 * @brief generate packets to setup an RDCU compression
 */

static int gen_rdcu_write_pkts(const struct rdcu_cfg *rcfg)
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

		error  = cmp_info_read(last_info_file_name, &last_info, io_flags & CMP_IO_VERBOSE);
		if (error) {
			fprintf(stderr, "%s: %s: Importing last decompression information file failed.\n",
				PROGRAM_NAME, last_info_file_name);
			return -1;
		}

		error = gen_rdcu_parallel_pkts(rcfg, &last_info);
		if (error)
			return -1;
	}

	/* generation of packets for non-parallel read/write RDCU setup */
	error = gen_write_rdcu_pkts(rcfg);
	if (error)
		return -1;

	return 0;
}


/**
 * return a current PLATO timestamp
 */

static uint64_t return_timestamp(void)
{
	return cmp_ent_create_timestamp(NULL);
}


/**
 * @brief compress chunk data and write the results to files
 */

static int compression_of_chunk(const void *chunk, uint32_t size, void *model,
				const struct cmp_par *chunk_par)
{
	uint32_t bound = compress_chunk_cmp_size_bound(chunk, size);
	uint32_t *cmp_data;
	uint32_t cmp_size;
	int error = 0;

	compress_chunk_init(&return_timestamp, cmp_tool_gen_version_id(CMP_TOOL_VERSION));

	if (!bound)
		return -1;
	cmp_data = calloc(1, bound);
	if (cmp_data == NULL) {
		fprintf(stderr, "%s: Error allocating memory for output buffer.\n", PROGRAM_NAME);
		return -1;
	}

	printf("Compress chunk data ... ");
	cmp_size = compress_chunk(chunk, size, model, model,
				  cmp_data, bound, chunk_par);
	if (cmp_is_error(cmp_size))
		goto cmp_chunk_fail;

	cmp_size = compress_chunk_set_model_id_and_counter(cmp_data, cmp_size,
			(uint16_t)model_id, (uint8_t)model_counter);
	if (cmp_is_error(cmp_size))
		goto cmp_chunk_fail;

	printf("DONE\nWrite compressed data to file %s.cmp ... ", output_prefix);
	error = write_data_to_file(cmp_data, (uint32_t)cmp_size, output_prefix,
				   ".cmp", io_flags);

cmp_chunk_fail:
	free(cmp_data);
	cmp_data = NULL;
	if (cmp_is_error(cmp_size)) {
		fprintf(stderr, "%s: %s.\n", PROGRAM_NAME, cmp_get_error_name(cmp_size));
		printf("FAILED\n");
		return (int)cmp_get_error_code(cmp_size);
	}
	if (error) {
		printf("FAILED\n");
		return -1;
	}
	printf("DONE\n");
	return 0;
}


/**
 * @brief compress the data and write the results to files
 */

static int compression_for_rdcu(struct rdcu_cfg *rcfg)
{
	uint64_t start_time = cmp_ent_create_timestamp(NULL);
	enum cmp_data_type data_type = add_rdcu_pars ?
		DATA_TYPE_IMAGETTE_ADAPTIVE : DATA_TYPE_IMAGETTE;
	uint32_t cmp_size;
	int error;
	uint32_t cmp_size_byte, out_buf_size;
	size_t s;
	struct cmp_entity *cmp_entity = NULL;
	void *data_to_write_to_file;
	struct cmp_info info;

	if (rcfg->buffer_length == 0) {
		rcfg->buffer_length = (rcfg->samples+1) * BUFFER_LENGTH_DEF_FAKTOR; /* +1 to prevent malloc(0)*/
		printf("No buffer_length parameter set. Use buffer_length = %u as compression buffer size.\n",
		       rcfg->buffer_length);
	}

	if (rdcu_pkt_mode) {
		void *tmp = rcfg->icu_new_model_buf;

		rcfg->icu_new_model_buf = NULL;
		printf("Generate compression setup packets ...\n");
		error = gen_rdcu_write_pkts(rcfg);
		if (error)
			goto error_cleanup;
		printf("... DONE\n");
		rcfg->icu_new_model_buf = tmp;
	}

	printf("Compress data ... ");
	out_buf_size = rcfg->buffer_length * sizeof(uint16_t);
	cmp_entity = calloc(1, out_buf_size + sizeof(struct cmp_entity));
	if (cmp_entity == NULL) {
		fprintf(stderr, "%s: Error allocating memory for output buffer.\n", PROGRAM_NAME);
		goto error_cleanup;
	}
	s = cmp_ent_create(cmp_entity, data_type, rcfg->cmp_mode == CMP_MODE_RAW, out_buf_size);
	if (!s) {
		fprintf(stderr, "%s: error occurred while creating the compression entity header.\n", PROGRAM_NAME);
		goto error_cleanup;
	}
	rcfg->icu_output_buf = cmp_ent_get_data_buf(cmp_entity);

	cmp_size = compress_like_rdcu(rcfg, &info);
	if (cmp_is_error(cmp_size)) {
		if (cmp_get_error_code(cmp_size) == CMP_ERROR_SMALL_BUF_)
			fprintf(stderr, "Error: The buffer for the compressed data is too small to hold the compressed data. Try a larger buffer_length parameter.\n");
		goto error_cleanup;
	}

	if (!model_counter && model_mode_is_used(rcfg->cmp_mode))
		model_counter++;

	s = cmp_ent_create(cmp_entity, data_type, rcfg->cmp_mode == CMP_MODE_RAW, cmp_bit_to_byte(cmp_size));
	if (!s) {
		fprintf(stderr, "%s: error occurred while creating the compression entity header.\n", PROGRAM_NAME);
		goto error_cleanup;
	}
	error = cmp_ent_set_version_id(cmp_entity, cmp_tool_gen_version_id(CMP_TOOL_VERSION));
	error |= cmp_ent_set_start_timestamp(cmp_entity, start_time);
	error |= cmp_ent_set_end_timestamp(cmp_entity, cmp_ent_create_timestamp(NULL));
	error |= cmp_ent_set_model_id(cmp_entity, model_id);
	error |= cmp_ent_set_model_counter(cmp_entity, model_counter);
	error |= cmp_ent_write_rdcu_cmp_pars(cmp_entity, &info, rcfg);
	if (error) {
		fprintf(stderr, "%s: error occurred while creating the compression entity header.\n", PROGRAM_NAME);
		goto error_cleanup;
	}

	if (include_cmp_header) {
		data_to_write_to_file = cmp_entity;
		cmp_size_byte = cmp_ent_get_size(cmp_entity);
	} else {
		data_to_write_to_file = cmp_ent_get_data_buf(cmp_entity);
		cmp_size_byte = cmp_ent_get_cmp_data_size(cmp_entity);
	}

	printf("DONE\n");

	if (rdcu_pkt_mode) {
		printf("Generate the read results packets ... ");
		error = gen_read_rdcu_pkts(&info);
		if (error)
			goto error_cleanup;
		printf("DONE\n");
	}

	printf("Write compressed data to file %s.cmp ... ", output_prefix);
	error = write_data_to_file(data_to_write_to_file, cmp_size_byte,
				   output_prefix, ".cmp", io_flags);
	if (error)
		goto error_cleanup;
	printf("DONE\n");

	if (!include_cmp_header) {
		printf("Write decompression information to file %s.info ... ",
		       output_prefix);
		error = cmp_info_to_file(&info, output_prefix, add_rdcu_pars);
		if (error)
			goto error_cleanup;
		printf("DONE\n");

		if (io_flags & CMP_IO_VERBOSE) {
			printf("\n");
			print_cmp_info(&info);
			printf("\n");
		}
	}

	free(cmp_entity);
	rcfg->icu_output_buf = NULL;

	return 0;

error_cleanup:
	free(cmp_entity);
	rcfg->icu_output_buf = NULL;

	return -1;
}


/**
 * @brief decompress the data and write the results in file(s)
 */

static int decompression(const struct cmp_entity *ent, uint16_t *input_model_buf)
{
	int error;
	int decomp_size;
	uint16_t *decomp_output;
	enum cmp_type cmp_type;

	printf("Decompress data ... ");

	decomp_size = decompress_cmp_entiy(ent, input_model_buf, input_model_buf, NULL);
	if (decomp_size < 0)
		return -1;
	if (decomp_size == 0) {
		printf("\nWarring: No data are decompressed.\n... ");
		printf("DONE\n");
		return 0;
	}

	decomp_output = malloc((size_t)decomp_size);
	if (decomp_output == NULL) {
		fprintf(stderr, "%s: Error allocating memory for decompressed data.\n", PROGRAM_NAME);
		return -1;
	}

	decomp_size = decompress_cmp_entiy(ent, input_model_buf, input_model_buf, decomp_output);
	if (decomp_size <= 0) {
		free(decomp_output);
		return -1;
	}

	printf("DONE\n");

	printf("Write decompressed data to file %s.dat ... ", output_prefix);

	if (cmp_ent_get_data_type(ent) == DATA_TYPE_CHUNK)
		cmp_type = CMP_TYPE_CHUNK;
	else
		cmp_type = CMP_TYPE_RDCU;
	error = write_input_data_to_file(decomp_output, (uint32_t)decomp_size, cmp_type,
					 output_prefix, ".dat", io_flags);

	free(decomp_output);
	if (error)
		return -1;

	printf("DONE\n");

	return 0;
}


/**
 * @brief create a default configuration for a compression data type
 */

static void cmp_cfg_create_default(struct rdcu_cfg *rcfg, enum cfg_default_opt mode)
{
	if (!rcfg) /* nothing to do */
		return;

	switch (mode) {
	case MODEL_CFG:
		rdcu_cfg_create(rcfg, CMP_DEF_IMA_MODEL_CMP_MODE, CMP_DEF_IMA_MODEL_MODEL_VALUE,
				CMP_DEF_IMA_MODEL_LOSSY_PAR);
		rdcu_cfg_buffers(rcfg, NULL, 0, NULL, CMP_DEF_IMA_MODEL_RDCU_DATA_ADR,
				CMP_DEF_IMA_MODEL_RDCU_MODEL_ADR, CMP_DEF_IMA_MODEL_RDCU_UP_MODEL_ADR,
				CMP_DEF_IMA_MODEL_RDCU_BUFFER_ADR, 0);
		rdcu_cfg_imagette_default(rcfg);
		break;
	case DIFF_CFG:
		rdcu_cfg_create(rcfg, CMP_DEF_IMA_DIFF_CMP_MODE,
				CMP_DEF_IMA_DIFF_MODEL_VALUE,
				CMP_DEF_IMA_DIFF_LOSSY_PAR);
		rdcu_cfg_buffers(rcfg, NULL, 0, NULL, CMP_DEF_IMA_DIFF_RDCU_DATA_ADR,
				CMP_DEF_IMA_DIFF_RDCU_MODEL_ADR, CMP_DEF_IMA_DIFF_RDCU_UP_MODEL_ADR,
				CMP_DEF_IMA_DIFF_RDCU_BUFFER_ADR, 0);
		rdcu_cfg_imagette_default(rcfg);
		break;
	}
}
