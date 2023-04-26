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
#include "cmp_rdcu.h"
#include "decmp.h"
#include "cmp_guess.h"
#include "cmp_entity.h"
#include "rdcu_pkt_to_file.h"
#include "cmp_data_types.h"


#define BUFFER_LENGTH_DEF_FAKTOR 2

#define DEFAULT_MODEL_ID 53264  /* random default id */
#define DEFAULT_MODEL_COUNTER 0


/* parse a data_type option argument */
static enum cmp_data_type parse_data_type(const char *data_type_str);

/* find a good set of compression parameters for a given dataset */
static int guess_cmp_pars(struct cmp_cfg *cfg, const char *guess_cmp_mode,
			  int guess_level);

/* compress the data and write the results to files */
static int compression(struct cmp_cfg *cfg, struct cmp_info *info);

/* decompress the data and write the results in file(s)*/
static int decompression(struct cmp_entity *ent, uint16_t *input_model_buf);

/* create a default configuration for a compression data type */
enum cfg_default_opt {DIFF_CFG, MODEL_CFG};
static int cmp_cfg_create_default(struct cmp_cfg *cfg, enum cmp_data_type data_type,
				  enum cfg_default_opt mode);


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
	MODEL_COUTER,
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

/* model ID string set by the --model_id option */
static const char *model_id_str;

/* model counter string set by the --model_counter option */
static const char *model_counter_str;


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

	struct cmp_info info = {0}; /* decompression information struct */
	struct cmp_cfg cfg = {0}; /* compressor configuration struct */

	cfg.data_type = DATA_TYPE_IMAGETTE; /* use imagette as default data type */

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
			cfg.data_type = parse_data_type(optarg);
			if (cfg.data_type == DATA_TYPE_UNKNOWN)
				exit(EXIT_FAILURE);
			break;
		case 'o':
			output_prefix = optarg;
			break;
		case 'v': /* --verbose */
			io_flags |= CMP_IO_VERBOSE;
			break;
		case 'V': /* --version */
			printf("%s version %s\n", PROGRAM_NAME, CMP_TOOL_VERSION);
			exit(EXIT_SUCCESS);
			break;
		case DIFF_CFG_OPTION:
			print_diff_cfg = 1;
			cfg.data_type = parse_data_type(optarg);
			if (cfg.data_type == DATA_TYPE_UNKNOWN)
				exit(EXIT_FAILURE);
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
			model_id_str = optarg;
			break;
		case MODEL_COUTER:
			model_counter_str = optarg;
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

	if (print_model_cfg == 1) {
		if (add_rdcu_pars)
			cfg.data_type = DATA_TYPE_IMAGETTE_ADAPTIVE;
		cmp_cfg_create_default(&cfg, cfg.data_type, MODEL_CFG);
		cmp_cfg_print(&cfg);
		exit(EXIT_SUCCESS);
	}

	if (print_diff_cfg == 1) {
		if (add_rdcu_pars)
			cfg.data_type = DATA_TYPE_IMAGETTE_ADAPTIVE;
		cmp_cfg_create_default(&cfg, cfg.data_type, DIFF_CFG);
		cmp_cfg_print(&cfg);
		exit(EXIT_SUCCESS);
	}

	{
		char str[] = "### PLATO Compression/Decompression Tool Version " CMP_TOOL_VERSION " ###\n";
		size_t str_len = strlen(str) - 1; /* -1 for \n */
		size_t i;
		for (i = 0; i < str_len; ++i)
			printf("#");
		printf("\n");
		printf("%s", str);
		for (i = 0; i < str_len; ++i)
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
		uint32_t input_size;

		if (cmp_operation) {
			printf("## Starting the compression ##\n");
			printf("Importing configuration file %s ... ", cfg_file_name);
			error = cmp_cfg_read(cfg_file_name, &cfg, io_flags & CMP_IO_VERBOSE);
			if (error)
				goto fail;
			printf("DONE\n");
		} else {
			printf("## Search for a good set of compression parameters ##\n");
		}

		printf("Importing data file %s ... ", data_file_name);
		/* count the samples in the data file when samples == 0 */
		if (cfg.samples == 0) {
			int32_t samples;

			size = read_file_data(data_file_name, cfg.data_type, NULL, 0, io_flags);
			if (size <= 0 || size > UINT32_MAX) /* empty file is treated as an error */
				goto fail;
			samples = cmp_input_size_to_samples((uint32_t)size, cfg.data_type);
			if (samples < 0)
				goto fail;
			cfg.samples = (uint32_t)samples;
			printf("\nNo samples parameter set. Use samples = %u.\n... ", cfg.samples);
		}

		input_size = cmp_cal_size_of_data(cfg.samples, cfg.data_type);
		cfg.input_buf = malloc(input_size);
		if (!cfg.input_buf) {
			fprintf(stderr, "%s: Error allocating memory for input data buffer.\n", PROGRAM_NAME);
			goto fail;
		}

		size = read_file_data(data_file_name, cfg.data_type, cfg.input_buf,
				      input_size, io_flags);
		if (size < 0)
			goto fail;
		printf("DONE\n");

	} else { /* decompression mode*/
		printf("## Starting the decompression ##\n");
		if (info_file_name) {
			ssize_t f_size;
			size_t ent_size;
			uint32_t cmp_size_byte;

			printf("Importing decompression information file %s ... ", info_file_name);
			error  = cmp_info_read(info_file_name, &info, io_flags & CMP_IO_VERBOSE);
			if (error)
				goto fail;
			printf("DONE\n");

			printf("Importing compressed data file %s ... ", data_file_name);

			ent_size = cmp_ent_create(NULL, DATA_TYPE_IMAGETTE, info.cmp_mode_used == CMP_MODE_RAW,
						  cmp_bit_to_4byte(info.cmp_size));
			if (!ent_size)
				goto fail;
			decomp_entity = calloc(1, ent_size);
			if (!decomp_entity) {
				fprintf(stderr, "%s: Error allocating memory for decompression input buffer.\n", PROGRAM_NAME);
				goto fail;
			}
			ent_size = cmp_ent_create(decomp_entity, DATA_TYPE_IMAGETTE, info.cmp_mode_used == CMP_MODE_RAW,
						  cmp_bit_to_4byte(info.cmp_size));
			if (!ent_size)
				goto fail;

			cmp_size_byte = (info.cmp_size+7)/CHAR_BIT;
			f_size = read_file8(data_file_name, cmp_ent_get_data_buf(decomp_entity),
					    cmp_size_byte, io_flags);
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
			if (size < 0 || size > UINT32_MAX)
				goto fail;
			/* to be save allocate at least the size of the cmp_entity struct */
			buf_size = (size_t)size;
			if (buf_size < sizeof(struct cmp_entity))
				buf_size = sizeof(struct cmp_entity);
			/* The compressed data is read in 4-byte words, so our
			 * data buffer must be a multiple of 4 bytes.
			 */
			buf_size = (buf_size + 3) & ~((size_t)0x3);

			decomp_entity = calloc(1, buf_size);
			if (!decomp_entity) {
				fprintf(stderr, "%s: Error allocating memory for the compression entity buffer.\n", PROGRAM_NAME);
				goto fail;
			}
			size = read_file_cmp_entity(data_file_name, decomp_entity,
						    (uint32_t)size, io_flags);
			if (size < 0)
				goto fail;

			if (cmp_ent_get_size(decomp_entity) & 0x3) {
				printf("\nThe size of the compression entity is not a multiple of 4 bytes. Padding the compression entity to a multiple of 4 bytes.\n");
				cmp_ent_set_size(decomp_entity, (uint32_t)buf_size);
			}

			if (io_flags & CMP_IO_VERBOSE) {
				cmp_ent_print(decomp_entity);
				printf("\n");
			}
		}
		printf("DONE\n");
	}

	if (model_file_name && !guess_operation &&
	    ((cmp_operation && !model_mode_is_used(cfg.cmp_mode)) ||
	     (!cmp_operation && !model_mode_is_used(cmp_ent_get_cmp_mode(decomp_entity)))))
		printf("Warring: Model file (-m option) specified but no model is used.\n");

	/* read in model */
	if ((cmp_operation && model_mode_is_used(cfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(cmp_ent_get_cmp_mode(decomp_entity))) ||
	    (guess_operation && model_file_name)) {
		ssize_t size;
		uint32_t model_size;
		enum cmp_data_type data_type;

		printf("Importing model file %s ... ", model_file_name ? model_file_name : "");
		if (!model_file_name) {
			fprintf(stderr, "%s: No model file (-m option) specified.\n", PROGRAM_NAME);
			goto fail;
		}

		if (cmp_operation || guess_operation) {
			data_type = cfg.data_type;
			model_size = cmp_cal_size_of_data(cfg.samples, cfg.data_type);
		} else {
			data_type = cmp_ent_get_data_type(decomp_entity);
			model_size = cmp_ent_get_original_size(decomp_entity);
		}

		input_model_buf = malloc(model_size);
		if (!input_model_buf) {
			fprintf(stderr, "%s: Error allocating memory for model buffer.\n", PROGRAM_NAME);
			goto fail;
		}

		size = read_file_data(model_file_name, data_type, input_model_buf,
				      model_size, io_flags);
		if (size < 0)
			goto fail;
		printf("DONE\n");

		cfg.model_buf = input_model_buf;
		cfg.icu_new_model_buf = input_model_buf; /* in-place model update */
	}

	if (guess_operation) {
		error = guess_cmp_pars(&cfg, guess_cmp_mode, guess_level);
		if (error)
			goto fail;
	} else if (cmp_operation) {
		error = compression(&cfg, &info);
		if (error)
			goto fail;
	} else {
		error = decompression(decomp_entity, input_model_buf);
		if (error)
			goto fail;
	}

	/* write our the updated model for compressed or decompression */
	if (!guess_operation &&
	    ((cmp_operation && model_mode_is_used(cfg.cmp_mode)) ||
	    (!cmp_operation && model_mode_is_used(cmp_ent_get_cmp_mode(decomp_entity))))) {
		enum cmp_data_type data_type = DATA_TYPE_UNKNOWN;
		uint32_t model_size;

		printf("Write updated model to file %s_upmodel.dat ... ", output_prefix);
		if (cmp_operation) {
			data_type = cfg.data_type;
			model_size = cmp_cal_size_of_data(cfg.samples, data_type);
		} else {
			data_type = cmp_ent_get_data_type(decomp_entity);
			model_size = cmp_ent_get_original_size(decomp_entity);
		}

		error = write_input_data_to_file(input_model_buf, model_size, data_type,
						 output_prefix, "_upmodel.dat", io_flags);
		if (error)
			goto fail;
		printf("DONE\n");
	}

	free(cfg.input_buf);
	free(decomp_entity);
	free(input_model_buf);

	exit(EXIT_SUCCESS);

fail:
	printf("FAILED\n");

	free(cfg.input_buf);
	free(decomp_entity);
	free(input_model_buf);

	exit(EXIT_FAILURE);
}


/**
 * @brief parse a data_type option argument
 */

static enum cmp_data_type parse_data_type(const char *data_type_str)
{
	/* default data type if no optional argument is used */
	enum cmp_data_type data_type = DATA_TYPE_IMAGETTE;

	if (data_type_str) {
		data_type = string2data_type(optarg);
		if (data_type == DATA_TYPE_UNKNOWN)
			printf("Do not recognize %s compression data type.\n",
			       data_type_str);
	}
	return data_type;
}


/**
 * @brief find a good set of compression parameters for a given dataset
 */

static int guess_cmp_pars(struct cmp_cfg *cfg, const char *guess_cmp_mode,
			  int guess_level)
{
	int error;
	uint32_t cmp_size_bit;
	double cr;

	printf("Search for a good set of compression parameters (level: %d) ... ", guess_level);
	if (!strcmp(guess_cmp_mode, "RDCU")) {
		if (add_rdcu_pars)
			cfg->data_type = DATA_TYPE_IMAGETTE_ADAPTIVE;
		else
			cfg->data_type = DATA_TYPE_IMAGETTE;
		if (cfg->model_buf)
			cfg->cmp_mode = CMP_GUESS_DEF_MODE_MODEL;
		else
			cfg->cmp_mode = CMP_GUESS_DEF_MODE_DIFF;
	} else {
		cfg->data_type = DATA_TYPE_IMAGETTE; /* TODO*/
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

	cmp_size_bit = cmp_guess(cfg, guess_level);
	if (!cmp_size_bit)
		return -1;

	if (include_cmp_header)
		cmp_size_bit = CHAR_BIT * (cmp_bit_to_4byte(cmp_size_bit) +
			cmp_ent_cal_hdr_size(cfg->data_type, cfg->cmp_mode == CMP_MODE_RAW));

	printf("DONE\n");

	printf("Write the guessed compression configuration to file %s.cfg ... ", output_prefix);
	error = cmp_cfg_fo_file(cfg, output_prefix, io_flags & CMP_IO_VERBOSE);
	if (error)
		return -1;
	printf("DONE\n");

	cr = (8.0 * cmp_cal_size_of_data(cfg->samples, cfg->data_type))/cmp_size_bit;
	printf("Guessed parameters can compress the data with a CR of %.2f.\n", cr);

	return 0;
}


/**
 * @brief generate packets to setup an RDCU compression
 */

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

		error  = cmp_info_read(last_info_file_name, &last_info, io_flags & CMP_IO_VERBOSE);
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


/**
 * @brief generate the compression information used based on the compression
 *	configuration, to emulate the RDCU behaviour
 *
 * @param cfg		compression configuration struct
 * @param cmp_size_bit	length of the bitstream in bits
 * @param ap1_cmp_size_bit	length of the adaptive 1 bitstream in bits
 * @param ap2_cmp_size_bit	length of the adaptive 2 bitstream in bits
 * @param info		compressor information struct to set the used compression
 *	parameters (can be NULL)
 *
 * @returns 0 on success, error otherwise
 * TODO: set cmp_mode_err, set model_value_err, etc, in error case
 */

static int cmp_gernate_rdcu_info(const struct cmp_cfg *cfg, int cmp_size_bit,
				 int ap1_cmp_size_bit, int ap2_cmp_size_bit,
				 struct cmp_info *info)
{
	if (!cfg)
		return -1;

	if (cfg->cmp_mode > UINT8_MAX)
		return -1;

	if (cfg->round > UINT8_MAX)
		return -1;

	if (cfg->model_value > UINT8_MAX)
		return -1;

	if (info) {
		info->cmp_err = 0;
		info->cmp_mode_used = (uint8_t)cfg->cmp_mode;
		info->model_value_used = (uint8_t)cfg->model_value;
		info->round_used = (uint8_t)cfg->round;
		info->spill_used = cfg->spill;
		info->golomb_par_used = cfg->golomb_par;
		info->samples_used = cfg->samples;
		info->rdcu_new_model_adr_used = cfg->rdcu_new_model_adr;
		info->rdcu_cmp_adr_used = cfg->rdcu_buffer_adr;
		info->ap1_cmp_size = (uint32_t)ap1_cmp_size_bit;
		info->ap2_cmp_size = (uint32_t)ap2_cmp_size_bit;

		if (cmp_size_bit == CMP_ERROR_SMALL_BUF)
			/* the icu_output_buf is to small to store the whole bitstream */
			info->cmp_err |= 1UL << SMALL_BUFFER_ERR_BIT; /* set small buffer error */
		if (cmp_size_bit < 0)
			info->cmp_size = 0;
		else
			info->cmp_size = (uint32_t)cmp_size_bit;

	}
	return 0;
}


/**
 * @brief compress the data and write the results to files
 */

static int compression(struct cmp_cfg *cfg, struct cmp_info *info)
{
	int cmp_size, error;
	int ap1_cmp_size = 0, ap2_cmp_size = 0;
	uint32_t cmp_size_byte, out_buf_size;
	size_t s;
	uint64_t start_time = cmp_ent_create_timestamp(NULL);
	struct cmp_entity *cmp_entity = NULL;
	uint8_t model_counter = DEFAULT_MODEL_COUNTER;
	uint16_t model_id = DEFAULT_MODEL_ID;
	void *data_to_write_to_file;

	if (cfg->buffer_length == 0) {
		cfg->buffer_length = (cfg->samples+1) * BUFFER_LENGTH_DEF_FAKTOR; /* +1 to prevent malloc(0)*/
		printf("No buffer_length parameter set. Use buffer_length = %u as compression buffer size.\n",
		       cfg->buffer_length);
	}

	if (rdcu_pkt_mode) {
		void *tmp = cfg->icu_new_model_buf;

		cfg->icu_new_model_buf = NULL;
		printf("Generate compression setup packets ...\n");
		error = gen_rdcu_write_pkts(cfg);
		if (error)
			goto error_cleanup;
		printf("... DONE\n");
		cfg->icu_new_model_buf = tmp;
	}
	if (add_rdcu_pars) {
		struct cmp_cfg cfg_cpy = *cfg;

		cfg_cpy.icu_output_buf = NULL;
		cfg_cpy.icu_new_model_buf = NULL;

		cfg_cpy.golomb_par = cfg_cpy.ap1_golomb_par;
		cfg_cpy.spill = cfg_cpy.ap1_spill;
		ap1_cmp_size = icu_compress_data(&cfg_cpy);
		if (ap1_cmp_size < 0)
			ap1_cmp_size = 0;

		cfg_cpy.golomb_par = cfg_cpy.ap2_golomb_par;
		cfg_cpy.spill = cfg_cpy.ap2_spill;
		ap2_cmp_size = icu_compress_data(&cfg_cpy);
		if (ap2_cmp_size < 0)
			ap2_cmp_size = 0;
	}

	printf("Compress data ... ");
	/* round up to a multiple of 4 */
	out_buf_size = (cmp_cal_size_of_data(cfg->buffer_length, cfg->data_type) + 3) & ~0x3U;

	cmp_entity = calloc(1, out_buf_size + sizeof(struct cmp_entity));
	if (cmp_entity == NULL) {
		fprintf(stderr, "%s: Error allocating memory for output buffer.\n", PROGRAM_NAME);
		goto error_cleanup;
	}
	s = cmp_ent_create(cmp_entity, cfg->data_type, cfg->cmp_mode == CMP_MODE_RAW, out_buf_size);
	if (!s) {
		fprintf(stderr, "%s: error occurred while creating the compression entity header.\n", PROGRAM_NAME);
		goto error_cleanup;
	}
	cfg->icu_output_buf = cmp_ent_get_data_buf(cmp_entity);

	cmp_size = icu_compress_data(cfg);
	if (cmp_size < 0)
		goto error_cleanup;

	if (model_id_str) {
		uint32_t red_val;

		error = atoui32("model_id", model_id_str, &red_val);
		if (error || red_val > UINT16_MAX)
			return -1;
		model_id = (uint16_t)red_val;
	}
	if (model_counter_str) {
		uint32_t red_val;

		error = atoui32("model_counter", model_counter_str, &red_val);
		if (error || red_val > UINT8_MAX)
			return -1;
		model_counter = (uint8_t)red_val;
	} else {
		if (model_mode_is_used(cfg->cmp_mode))
			model_counter = DEFAULT_MODEL_COUNTER + 1;
	}

	s = cmp_ent_build(cmp_entity,  cmp_tool_gen_version_id(CMP_TOOL_VERSION),
			  start_time, cmp_ent_create_timestamp(NULL), model_id,
			  model_counter, cfg, cmp_size);
	if (!s) {
		fprintf(stderr, "%s: error occurred while creating the compression entity header.\n", PROGRAM_NAME);
		goto error_cleanup;
	}
	if (include_cmp_header) {
		data_to_write_to_file = cmp_entity;
		cmp_size_byte = cmp_ent_get_size(cmp_entity);
	} else {
		if (cmp_gernate_rdcu_info(cfg, cmp_size, ap1_cmp_size, ap2_cmp_size, info))
			goto error_cleanup;
		data_to_write_to_file = cmp_ent_get_data_buf(cmp_entity);
		if (cfg->cmp_mode == CMP_MODE_RAW)
			cmp_size_byte = info->cmp_size/CHAR_BIT;
		else
			cmp_size_byte = cmp_bit_to_4byte(info->cmp_size);
	}

	printf("DONE\n");

	if (rdcu_pkt_mode) {
		printf("Generate the read results packets ... ");
		error = gen_read_rdcu_pkts(info);
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
		error = cmp_info_to_file(info, output_prefix, add_rdcu_pars);
		if (error)
			goto error_cleanup;
		printf("DONE\n");

		if (io_flags & CMP_IO_VERBOSE) {
			printf("\n");
			print_cmp_info(info);
			printf("\n");
		}
	}

	free(cmp_entity);
	cfg->icu_output_buf = NULL;

	return 0;

error_cleanup:
	free(cmp_entity);
	cfg->icu_output_buf = NULL;

	return -1;
}


/**
 * @brief decompress the data and write the results in file(s)
 */

static int decompression(struct cmp_entity *ent, uint16_t *input_model_buf)
{
	int error;
	int decomp_size;
	uint16_t *decomp_output;

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

	error = write_input_data_to_file(decomp_output, (uint32_t)decomp_size, cmp_ent_get_data_type(ent),
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

static int cmp_cfg_create_default(struct cmp_cfg *cfg, enum cmp_data_type data_type,
				  enum cfg_default_opt mode)
{
	if (cmp_data_type_is_invalid(data_type))
		return -1;

	if (!cfg) /* nothing to do */
		return 0;

	if (cmp_imagette_data_type_is_used(data_type)) {
		switch (mode) {
		case MODEL_CFG:
			*cfg = rdcu_cfg_create(data_type, CMP_DEF_IMA_MODEL_CMP_MODE,
					       CMP_DEF_IMA_MODEL_MODEL_VALUE, CMP_DEF_IMA_MODEL_LOSSY_PAR);
			rdcu_cfg_buffers(cfg, NULL, 0, NULL, CMP_DEF_IMA_MODEL_RDCU_DATA_ADR,
					 CMP_DEF_IMA_MODEL_RDCU_MODEL_ADR, CMP_DEF_IMA_MODEL_RDCU_UP_MODEL_ADR,
					 CMP_DEF_IMA_MODEL_RDCU_BUFFER_ADR, 0);
			rdcu_cfg_imagette(cfg,
					  CMP_DEF_IMA_MODEL_GOLOMB_PAR, CMP_DEF_IMA_MODEL_SPILL_PAR,
					  CMP_DEF_IMA_MODEL_AP1_GOLOMB_PAR, CMP_DEF_IMA_MODEL_AP1_SPILL_PAR,
					  CMP_DEF_IMA_MODEL_AP2_GOLOMB_PAR, CMP_DEF_IMA_MODEL_AP2_SPILL_PAR);
			break;
		case DIFF_CFG:
			*cfg = rdcu_cfg_create(data_type, CMP_DEF_IMA_DIFF_CMP_MODE,
					       CMP_DEF_IMA_DIFF_MODEL_VALUE, CMP_DEF_IMA_DIFF_LOSSY_PAR);
			rdcu_cfg_buffers(cfg, NULL, 0, NULL, CMP_DEF_IMA_DIFF_RDCU_DATA_ADR,
					 CMP_DEF_IMA_DIFF_RDCU_MODEL_ADR, CMP_DEF_IMA_DIFF_RDCU_UP_MODEL_ADR,
					 CMP_DEF_IMA_DIFF_RDCU_BUFFER_ADR, 0);
			rdcu_cfg_imagette(cfg,
					  CMP_DEF_IMA_DIFF_GOLOMB_PAR, CMP_DEF_IMA_DIFF_SPILL_PAR,
					  CMP_DEF_IMA_DIFF_AP1_GOLOMB_PAR, CMP_DEF_IMA_DIFF_AP1_SPILL_PAR,
					  CMP_DEF_IMA_DIFF_AP2_GOLOMB_PAR, CMP_DEF_IMA_DIFF_AP2_SPILL_PAR);
			break;
		}
	}
	/* TODO: implement other data types */
	return 0;
}
