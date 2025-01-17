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

/* #ifndef TEST */
/* #define MAIN main */
/* #else */
/* #endif */
/* #include "../../programs/cmp_tool.c" */







#include <stdio.h>




#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* #include "fuzz_utils.h" */

int ignore_stdout_and_stderr(void)
{
	int fd = open("/dev/null", O_WRONLY);
	int ret = 0;

	if (fd == -1) {
		warn("open(\"/dev/null\") failed");
		return -1;
	}

	if (dup2(fd, STDOUT_FILENO) == -1) {
		warn("failed to redirect stdout to /dev/null\n");
		ret = -1;
	}

#if 1
	if (dup2(fd, STDERR_FILENO) == -1) {
		warn("failed to redirect stderr to /dev/null\n");
		ret = -1;
	}
#endif

	if (close(fd) == -1) {
		warn("close");
		ret = -1;
	}

	return ret;
}

int delete_file(const char *pathname)
{
	int ret = unlink(pathname);

	if (ret == -1) {
		warn("failed to delete \"%s\"", pathname);
	}

	free((void *)pathname);

	return ret;
}

char *buf_to_file(const uint8_t *buf, size_t size)
{
	int fd;
	size_t pos = 0;

	char *pathname = strdup("/tmp/fuzz-XXXXXX");

	if (pathname == NULL) {
		return NULL;
	}

	fd = mkstemp(pathname);
	if (fd == -1) {
		warn("mkstemp(\"%s\")", pathname);
		free(pathname);
		return NULL;
	}

	while (pos < size) {
		int nbytes = write(fd, &buf[pos], size - pos);

		if (nbytes <= 0) {
			if (nbytes == -1 && errno == EINTR) {
				continue;
			}
			warn("write");
			goto err;
		}
		pos += nbytes;
	}

	if (close(fd) == -1) {
		warn("close");
		goto err;
	}

	return pathname;

err:
	delete_file(pathname);
	FUZZ_ASSERT(1);
	return NULL;
}

#include <unistd.h>
#include <limits.h>

#define FILE_ARG_SIZE 50

static void add_argument_with_file(char **argv, int index, const char *flag, const char *file)
{
	if (index >= 0) {
		argv[index] = FUZZ_malloc(FILE_ARG_SIZE);
		memcpy(argv[index], flag, strlen(flag) + 1);
		strcat(argv[index], file);
	}
}

char **gen_argv(FUZZ_dataProducer_t *producer, int *argc, char *data_file,
		char *model_file, char *cfg_file, char *info_file,
		const uint8_t *other_arguments, size_t o_args_size)
{
	char **argv;
	long max_arg = sysconf(_SC_ARG_MAX);
	long line_max = sysconf(_SC_LINE_MAX) - 1;
	int i = 0;

	FUZZ_ASSERT(max_arg > 1);
	FUZZ_ASSERT(max_arg <= INT_MAX);
	FUZZ_ASSERT(line_max > 0);
	FUZZ_ASSERT(line_max <= UINT32_MAX);

	*argc = FUZZ_dataProducer_int32Range(producer, 1, (int32_t)max_arg);
	argv = FUZZ_malloc((size_t)(*argc) * sizeof(argv));

	/* set program name */
	add_argument_with_file(argv, 0, "cmp_tool_fuzz", "");
	/* set the file at thend the have a higher priotirty */
	i = *argc-1;
	add_argument_with_file(argv, i--, "-o /tmp/fuzz-output-cmp_tool", "");
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		add_argument_with_file(argv, i--, "-d", data_file);
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		add_argument_with_file(argv, i--, "-m", model_file);
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		add_argument_with_file(argv, i--, "-c", cfg_file);
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		add_argument_with_file(argv, i--, "-i", info_file);


	while (i > 0) {
		size_t s = FUZZ_dataProducer_uint32Range(producer, 0, (uint32_t)line_max);

		if (o_args_size < s)
			s = o_args_size;
		argv[i] = FUZZ_malloc(s+1);
		memcpy(argv[i], other_arguments, s);
		argv[i--][s] = '\0';
		other_arguments += s;
		o_args_size -= s;
	}

	return argv;
}


void free_argv(int argc, char **argv)
{
	while (argc--) {
		free(argv[argc]);
		argv[argc] = NULL;
	}
	free(argv);
}



int testable_main(int argc, char **argv);
#if 0
int main(void)
{
	int argc = 0;
	char **argv = NULL;
	FUZZ_dataProducer_t *producer = FUZZ_dataProducer_create(NULL, 0);
	char data_file[] = "data-file";
	char model_file[] = "model-file";
	char cfg_file[] = "cfg-file";
	uint8_t other_arguments[] = "other-stuf";

	argv = gen_argv(producer, &argc, data_file, model_file, cfg_file,
			other_arguments, sizeof(other_arguments));
	FUZZ_ASSERT(argv != NULL);
	FUZZ_ASSERT(argc == 11);
	FUZZ_ASSERT(!strcmp(argv[0], "cmp_tool_fuzz"));
	FUZZ_ASSERT(!strcmp(argv[10], "-o /tmp/fuzz-output-cmp_tool"));
	FUZZ_ASSERT(!strcmp(argv[9], "-ddata-file"));
	FUZZ_ASSERT(!strcmp(argv[8], "-mmodel-file"));
	FUZZ_ASSERT(!strcmp(argv[7], "-ccfg-file"));
	FUZZ_ASSERT(!strcmp(argv[6], "oth"));
	FUZZ_ASSERT(!strcmp(argv[5], "er-"));
	FUZZ_ASSERT(!strcmp(argv[4], "stu"));
	FUZZ_ASSERT(!strcmp(argv[3], "f"));
	FUZZ_ASSERT(!strcmp(argv[2], ""));
	FUZZ_ASSERT(!strcmp(argv[1], ""));

	/* testable_main(argc, argv); */
	free_argv(argc, argv);
	FUZZ_dataProducer_free(producer);
	return 0;
}
#endif

char *get_file(FUZZ_dataProducer_t *producer, const uint8_t **src, uint32_t *size)
{
	uint32_t file_size = FUZZ_dataProducer_uint32Range(producer, 0, *size);
	char *file = buf_to_file(*src, file_size);
	*src += file_size;
	*size -= file_size;
	return file;
}


int LLVMFuzzerTestOneInput(const uint8_t *src, size_t size)
{
	FUZZ_dataProducer_t *producer = FUZZ_dataProducer_create(src, size);

	size = FUZZ_dataProducer_reserveDataPrefix(producer);
	/* Create an array to hold the input arguments */
	int argc = 0;
	char **argv = NULL;
	uint32_t size32  = (uint32_t)size;
	char *data_file = get_file(producer, &src, &size32);
	char *model_file = get_file(producer, &src, &size32);
	char *info_file = get_file(producer, &src, &size32);
	char *cfg_file = get_file(producer, &src, &size32);
	const uint8_t *other_arguments = src;

	FUZZ_ASSERT(size < UINT32_MAX);

	argv = gen_argv(producer, &argc, data_file, model_file, cfg_file, info_file,
			other_arguments, size32);


#if 0
	/* Give a random portion of src data to the producer, to use for
	   parameter generation. The rest will be used for data/model */
	FUZZ_dataProducer_t *producer = FUZZ_dataProducer_create(src, size);

	size = FUZZ_dataProducer_reserveDataPrefix(producer);
	FUZZ_ASSERT(size <= UINT32_MAX);

	/* spilt data to compressed data and model data */
	ent_size = FUZZ_dataProducer_uint32Range(producer, 0, (uint32_t)size);
	model_of_data_size = FUZZ_dataProducer_uint32Range(producer, 0, (uint32_t)size-ent_size);

	if (ent_size)
		ent = (const struct cmp_entity *)src;
	if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
		model_of_data = src + ent_size;
	else
		model_of_data = NULL;


#endif
	/* ignore_stdout_and_stderr(); */
	/* for(int i = 1; i < argc; i++) */
	/*	printf("%s\n", argv[i]); */

	testable_main(argc, argv);

	delete_file(data_file);
	delete_file(model_file);
	delete_file(info_file);
	delete_file(cfg_file);

	free_argv(argc, argv);

	FUZZ_dataProducer_free(producer);

	return 0;
}

#if 0
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

extern int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size);
__attribute__((weak)) extern int LLVMFuzzerInitialize(int *argc, char ***argv);
int main(int argc, char **argv)
{
  fprintf(stderr, "StandaloneFuzzTargetMain: running %d inputs\n", argc - 1);
  if (LLVMFuzzerInitialize)
    LLVMFuzzerInitialize(&argc, &argv);
  for (int i = 1; i < argc; i++) {
    fprintf(stderr, "Running: %s\n", argv[i]);
    FILE *f = fopen(argv[i], "r");

    assert(f);
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);

    fseek(f, 0, SEEK_SET);
    unsigned char *buf = (unsigned char *)malloc(len);
    size_t n_read = fread(buf, 1, len, f);

    fclose(f);
    assert(n_read == len);
    LLVMFuzzerTestOneInput(buf, len);
    free(buf);
    fprintf(stderr, "Done:    %s: (%zd bytes)\n", argv[i], n_read);
  }
}
#endif
