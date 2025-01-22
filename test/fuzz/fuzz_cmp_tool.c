/**
 * @file fuzz_cmp_tool.c
 * @date 2025
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
 * @brief chunk compression fuzz target
 *
 * This fuzzer tests the compression functionality with random data/model and
 * parameters. It uses a random portion of the input data for parameter
 * generation, while the rest is used for compression.
 */


#include <stdint.h>
#include <limits.h>
#include <stdint.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "fuzz_helpers.h"
#include "fuzz_data_producer.h"

#define MAX_ARG_COUNT  32
#define MAX_ARG_SIZE  64


int testable_cmp_tool_main(int argc, char **argv);


static void add_argument_with_file(char **argv, int index, const char *flag, const char *file)
{
	if (index > 0) { /* zero is revert for program name */
		size_t flag_len = strlen(flag);
		size_t file_len = strlen(file);

		FUZZ_ASSERT(flag_len + file_len < MAX_ARG_SIZE);
		memcpy(argv[index], flag, flag_len);
		memcpy(argv[index]+flag_len, file, file_len + 1);
	}
}


static int gen_argv(FUZZ_dataProducer_t *producer, char **argv, const char *data_file,
		       const char *model_file, const char *cfg_file, const char *info_file,
		       const uint8_t *other_arguments, size_t o_args_size)
{
	int argc = FUZZ_dataProducer_int32Range(producer, 1, MAX_ARG_COUNT);
	int i, end;

	/* Add optional arguments no the end so they have higher priority */
	end = argc-1;
	/* TODO: How to clean up written stuff by the cmp_tool? */
	add_argument_with_file(argv, end--, "-o", FUZZ_TMP_DIR "/fuzz-output-cmp_tool");
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		add_argument_with_file(argv, end--, "-d", data_file);
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		add_argument_with_file(argv, end--, "-m", model_file);
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		add_argument_with_file(argv, end--, "-c", cfg_file);
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		add_argument_with_file(argv, end--, "-i", info_file);

	for (i = 0; i < end; i++) {
		size_t s = FUZZ_dataProducer_uint32Range(producer, 0, MAX_ARG_SIZE-1);

		if (s > o_args_size)
			s = o_args_size;
		memcpy(argv[i], other_arguments, s);
		argv[i][s] = '\0';
		other_arguments += s;
		o_args_size -= s;
	}

	return argc;
}



static char *get_file(FUZZ_dataProducer_t *producer, const uint8_t **src, uint32_t *size)
{
	uint32_t file_size = FUZZ_dataProducer_uint32Range(producer, 0, *size);
	char *file = FUZZ_buf_to_file(*src, file_size);
	*src += file_size;
	*size -= file_size;
	return file;
}


int LLVMFuzzerTestOneInput(const uint8_t *src, size_t size)
{
	static char **argv;
	/*
	 * Give a random portion of src data to the producer, to use for
	 * parameter generation. The rest will be used for the file content and
	 * arguments
	 */
	FUZZ_dataProducer_t *producer = FUZZ_dataProducer_create(src, size);
	uint32_t size32 = (uint32_t)FUZZ_dataProducer_reserveDataPrefix(producer);
	const char *data_file = get_file(producer, &src, &size32);
	const char *model_file = get_file(producer, &src, &size32);
	const char *info_file = get_file(producer, &src, &size32);
	const char *cfg_file = get_file(producer, &src, &size32);
	const uint8_t *other_arguments = src;

	int argc;

	FUZZ_ASSERT(size < UINT32_MAX);

	if (argv == NULL) {
		static const char program_name[] = "cmp_tool_fuzz";
		char *data;
		size_t i;

		argv = FUZZ_malloc(sizeof(*argv) * MAX_ARG_COUNT);
		data = FUZZ_malloc(sizeof(*data) * MAX_ARG_COUNT * MAX_ARG_SIZE);
		memset(data, 0, sizeof(*data) * MAX_ARG_COUNT * MAX_ARG_SIZE);
		for (i = 0; i < MAX_ARG_COUNT; i++)
			argv[i] = &data[i*MAX_ARG_SIZE];

		FUZZ_ASSERT(sizeof(program_name) <= MAX_ARG_SIZE);
		memcpy(argv[0], program_name, sizeof(program_name));
	}

	argc = gen_argv(producer, argv, data_file, model_file, cfg_file,
			info_file, other_arguments, size32);

	/* for(int i = 1; i < argc; i++) */
	/*	printf("%s\n", argv[i]); */

	testable_cmp_tool_main(argc, argv);

	FUZZ_delete_file(data_file);
	FUZZ_delete_file(model_file);
	FUZZ_delete_file(info_file);
	FUZZ_delete_file(cfg_file);

	FUZZ_dataProducer_free(producer);

	return 0;
}
