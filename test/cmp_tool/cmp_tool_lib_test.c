/**
 * @file   test_cmp_tool_lib.c
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
 * @brief unit tests for cmp_tool_lib.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/CUnit.h>
#include <CUnit/TestDB.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

#include "../../include/cmp_tool_lib.h"
#include "../../include/cmp_guess.h"

#define PROGRAM_NAME "cmp_tool"

/* used to redirect stdout to file */
int fd;

/* used to redirect stdout to file */
fpos_t pos;

/* @brief The init cmp_tool test suite cleanup functionr. Redirect stdout to
 *	file
 * @see: http://c-faq.com/stdio/undofreopen.html
 * @returns zero on success, non-zero otherwise.
 */
static int init_cmp_tool(void)
{
	fflush(stderr);
	fgetpos(stderr, &pos);
	fd = dup(fileno(stderr));
	if (freopen("tmp_stderr.log", "w", stderr) == NULL) {
		perror("freopen() failed");
		return -1;
	}

	return 0;
}


/* @brief The cmp_tool test suite cleanup function. Closes the temporary file
 *	used by the tests.
 * @returns zero on success, non-zero otherwise.
 */
static int clean_cmp_tool(void)
{
	fflush(stderr);
	dup2(fd, fileno(stderr));
	close(fd);
	clearerr(stderr);
	fsetpos(stderr, &pos);        /* for C9X */

	return 0;
}


/* returnd memory must be freed after use */
char *read_std_err_log(void)
{
	char *response = NULL;
	size_t len = 0;
	FILE *fp = fopen("tmp_stderr.log", "r");
	static int n_lines_read =0;
	int i;

	if (!fp) {
		puts("File opening failed");
		abort();
	}

	fflush(stderr);
	for (i=0; getline(&response, &len, fp) != -1; i++) {
		if (i == n_lines_read) {
			n_lines_read++;
			/* printf("%zu %s\n",len, response); */
			fclose(fp);
			return response;
		} else {
			free(response);
			response = NULL;
		}
	}
	free(response);
	response = malloc(1);
	if (!response)
		abort();
	response[0] = '\0';

	fclose(fp);
	return response;
}


/* tests */

/**
 * @test cmp_tool
 */

void test_read_file8(void)
{
	void *file_name;
	uint8_t *buf;
	uint32_t n_word;
	ssize_t i, size;
	uint8_t array[33] = {0};
	char *s;

	/* read in a normal data file */
	memset(array, 0, sizeof(array));
	file_name = "test_read_file8_1.txt";
	buf = array;
	n_word = 33;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, 33);
	/* no output on stderr */
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "");
	free(s);

	/* tests counting the size of a file */
	file_name = "test_read_file8_2.txt";
	buf = NULL;
	n_word = 0;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, 33);
	/* no output on stderr */
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "");
	free(s);

	/* test data read in counting comments */
	file_name = "test_read_file8_2.txt";
	buf = array;
	n_word = size;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, 33);
	for (i = 0; i < size; ++i) {
		CU_ASSERT_EQUAL(buf[i], i);
	}
	/* no output on stderr */
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "");
	free(s);

	/* tests partially read in */
	memset(array, 0, sizeof(array));
	file_name = "test_read_file8_2.txt";
	buf = array;
	n_word = 32;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, 32);
	for (i = 0; i < 33; ++i) {
		if (i < 32) {
			CU_ASSERT_EQUAL(buf[i], i);
		} else {
			CU_ASSERT_EQUAL(buf[i], 0);
		}
	}
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_2.txt: Warning: The file may contain more data than specified by the samples or cmp_size parameter.\n");
	free(s);

	/* tests read 0 words in */
	memset(array, 0, sizeof(array));
	file_name = "test_read_file8_2.txt";
	buf = array;
	n_word = 0;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, 0);
	for (i = 0; i < 33; ++i) {
		CU_ASSERT_EQUAL(buf[i], 0);
	}
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_2.txt: Warning: The file may contain more data than specified by the samples or cmp_size parameter.\n");
	free(s);

	/* TODO; tests read in 0 empty file */

	/* Error cases */
	/***************/

	/* file does not contain enough data */
	memset(array, 0xFF, sizeof(array));
	file_name = "test_read_file8_2.txt";
	buf = array;
	n_word = 34;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, -1);
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_2.txt: Error: The files do not contain enough data as requested.\n");
	free(s);
	memset(array, 0xFF, sizeof(array));

	/* no file_name */
	file_name = NULL;
	buf = array;
	n_word = 33;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, -1);
	/* no output on stderr */
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "");
	free(s);

	/* wrong file_name */
	file_name = "file_not_exist.txt";
	buf = array;
	n_word = 33;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, -1);
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: file_not_exist.txt: No such file or directory\n");
	free(s);

	/* wrong data format part 1/2 */
	file_name = "test_read_file8_3.txt";
	buf = array;
	n_word = 33;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, -1);
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_3.txt: Error: The data are not correct formatted. Expected format is like: 12 AB 23 CD .. ..\n");
	free(s);

	/* wrong data format part 2/2 */
	file_name = "test_read_file8_4.txt";
	buf = array;
	n_word = 33;
	size = read_file8(file_name, buf, n_word, 0);
	CU_ASSERT_EQUAL(size, -1);
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_4.txt: Error: The data are not correct formatted. Expected format is like: 12 AB 23 CD .. ..\n");
	free(s);

	/* file error */
	/* TODO: how to trigger a file error indicator error */
	if (1) {
		/* static char fname[] = "test_read_file8_5.txt"; */
		/* FILE *f = fopen(fname, "wb"); */
		/* fputs("\xff\xff\n", f); // not a valid UTF-8 character sequence */
		/* fclose(f); */
		buf = array;
		n_word = 33;
		size = read_file8("test_read_file8_5.txt", buf, n_word, 0);
		CU_ASSERT_EQUAL(size, -1);
		CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_5.txt: Error: File error indicator set.\n");
		free(s);
	}
}


#define CMP_TEST_SAMPLES 5
void test_cmp_guess(void)
{
	struct cmp_cfg cfg = {0};
	uint16_t data[CMP_TEST_SAMPLES] = {2,4,6,8,10};
	uint16_t data_exp[CMP_TEST_SAMPLES] = {2,4,6,8,10};
	uint16_t model[CMP_TEST_SAMPLES] = {4,6,8,10,12};
	uint16_t model_exp[CMP_TEST_SAMPLES] = {4,6,8,10,12};
	uint32_t cmp_size;
	int level;


	/* test 1d-diff mode */
	cfg.input_buf = data;
	cfg.model_buf = NULL;
	cfg.samples = CMP_TEST_SAMPLES;
	cfg.cmp_mode = MODE_DIFF_MULTI;
	level = 2;
	cmp_size = cmp_guess(&cfg, level);
	CU_ASSERT_TRUE(cmp_size);

	CU_ASSERT_EQUAL(cmp_size, 25);
	CU_ASSERT_EQUAL(cfg.cmp_mode, MODE_DIFF_MULTI);
	CU_ASSERT_EQUAL(cfg.golomb_par, 1);
	CU_ASSERT_EQUAL(cfg.spill, 2);
	/* CU_ASSERT_EQUAL(cfg.model_value, ); model_value is not needed */
	CU_ASSERT_EQUAL(cfg.round, 0);
	CU_ASSERT_EQUAL(cfg.ap1_golomb_par, 2);
	CU_ASSERT_EQUAL(cfg.ap1_spill, 2);
	CU_ASSERT_EQUAL(cfg.ap2_golomb_par, 3);
	CU_ASSERT_EQUAL(cfg.ap2_spill, 2);
	CU_ASSERT_NSTRING_EQUAL(cfg.input_buf, data_exp, CMP_TEST_SAMPLES);
	/* CU_ASSERT_NSTRING_EQUAL(cfg.model_buf, model_exp, CMP_TEST_SAMPLES); model is
	 * not used*/
	CU_ASSERT_EQUAL(cfg.samples, CMP_TEST_SAMPLES);
	CU_ASSERT_EQUAL(cfg.buffer_length, 2);


	/* test model mode */
	memset(&cfg, 0, sizeof(struct cmp_cfg));
	cfg.input_buf = data;
	cfg.model_buf = model;
	cfg.samples = CMP_TEST_SAMPLES;
	cfg.cmp_mode = MODE_MODEL_ZERO;
	level =3;

	cmp_guess_set_model_updates(12);
	cmp_size = cmp_guess(&cfg, level);
	CU_ASSERT_TRUE(cmp_size);

	CU_ASSERT_EQUAL(cmp_size, 20);
	CU_ASSERT_EQUAL(cfg.cmp_mode, 1);
	CU_ASSERT_EQUAL(cfg.golomb_par, 2);
	CU_ASSERT_EQUAL(cfg.spill, 22);
	CU_ASSERT_EQUAL(cfg.model_value, 12);
	CU_ASSERT_EQUAL(cfg.round, 0);
	CU_ASSERT_EQUAL(cfg.ap1_golomb_par, 1);
	CU_ASSERT_EQUAL(cfg.ap1_spill, 8);
	CU_ASSERT_EQUAL(cfg.ap2_golomb_par, 3);
	CU_ASSERT_EQUAL(cfg.ap2_spill, 35);
	CU_ASSERT_NSTRING_EQUAL(cfg.input_buf, data_exp, CMP_TEST_SAMPLES);
	CU_ASSERT_NSTRING_EQUAL(cfg.model_buf, model_exp, CMP_TEST_SAMPLES);
	CU_ASSERT_EQUAL(cfg.samples, CMP_TEST_SAMPLES);
	CU_ASSERT_EQUAL(cfg.buffer_length, 2);


	/* test diff mode without model buffer*/
	cmp_guess_set_model_updates(CMP_GUESS_N_MODEL_UPDATE_DEF);
	memset(&cfg, 0, sizeof(struct cmp_cfg));
	cfg.input_buf = data;
	cfg.model_buf = model;
	cfg.samples = CMP_TEST_SAMPLES;
	cfg.cmp_mode = MODE_DIFF_MULTI;
	level = 3;

	cmp_size = cmp_guess(&cfg, level);
	CU_ASSERT_TRUE(cmp_size);

	CU_ASSERT_EQUAL(cmp_size, 20);
	CU_ASSERT_EQUAL(cfg.cmp_mode, MODE_DIFF_MULTI);
	CU_ASSERT_EQUAL(cfg.golomb_par, 2);
	CU_ASSERT_EQUAL(cfg.spill, 5);
	CU_ASSERT_EQUAL(cfg.model_value, 11);
	CU_ASSERT_EQUAL(cfg.round, 0);
	CU_ASSERT_EQUAL(cfg.ap1_golomb_par, 1);
	CU_ASSERT_EQUAL(cfg.ap1_spill, 2);
	CU_ASSERT_EQUAL(cfg.ap2_golomb_par, 3);
	CU_ASSERT_EQUAL(cfg.ap2_spill, 2);
	CU_ASSERT_NSTRING_EQUAL(cfg.input_buf, data_exp, CMP_TEST_SAMPLES);
	CU_ASSERT_NSTRING_EQUAL(cfg.model_buf, model_exp, CMP_TEST_SAMPLES);
	CU_ASSERT_EQUAL(cfg.samples, CMP_TEST_SAMPLES);
	CU_ASSERT_EQUAL(cfg.buffer_length, 2);


	/* error test cfg = NULL */
	level = 2;
	cmp_size = cmp_guess(NULL, level);
	CU_ASSERT_FALSE(cmp_size);


	/* error test unknown guess_level */
	level = 4;
	cmp_size = cmp_guess(&cfg, level);
	CU_ASSERT_FALSE(cmp_size);


	/* error test model mode without model buffer*/
	memset(&cfg, 0, sizeof(struct cmp_cfg));
	cfg.input_buf = NULL;
	cfg.model_buf = NULL;
	cfg.samples = CMP_TEST_SAMPLES;
	cfg.cmp_mode = MODE_DIFF_MULTI;
	level = 2;

	cmp_size = cmp_guess(&cfg, level);
	CU_ASSERT_FALSE(cmp_size);


	/* error test model mode without model buffer*/
	memset(&cfg, 0, sizeof(struct cmp_cfg));
	cfg.input_buf = data;
	cfg.model_buf = NULL;
	cfg.samples = CMP_TEST_SAMPLES;
	cfg.cmp_mode = MODE_MODEL_MULTI;
	level = 2;

	cmp_size = cmp_guess(&cfg, level);
	CU_ASSERT_FALSE(cmp_size);


	/* error test not supported cmp_mode */
	memset(&cfg, 0, sizeof(struct cmp_cfg));
	cfg.input_buf = data;
	cfg.model_buf = model;
	cfg.samples = CMP_TEST_SAMPLES;
	cfg.cmp_mode = 5;
	level = 2;

	cmp_size = cmp_guess(&cfg, level);
	CU_ASSERT_FALSE(cmp_size);


	/* error test samples = 0 */
	memset(&cfg, 0, sizeof(struct cmp_cfg));
	cfg.input_buf = data;
	cfg.model_buf = model;
	cfg.samples = 0;
	cfg.cmp_mode = MODE_MODEL_MULTI;
	level = 2;

	cmp_size = cmp_guess(&cfg, level);
	CU_ASSERT_FALSE(cmp_size);
}

void test_cmp_guess_model_value(void)
{
	uint16_t model_value;

	model_value = cmp_guess_model_value(0);
	CU_ASSERT_EQUAL(model_value, 8);
	model_value = cmp_guess_model_value(1);
	CU_ASSERT_EQUAL(model_value, 8);
	model_value = cmp_guess_model_value(2);
	CU_ASSERT_EQUAL(model_value, 8);
	model_value = cmp_guess_model_value(3);
	CU_ASSERT_EQUAL(model_value, 10);
	model_value = cmp_guess_model_value(4);
	CU_ASSERT_EQUAL(model_value, 10);
	model_value = cmp_guess_model_value(5);
	CU_ASSERT_EQUAL(model_value, 10);
	model_value = cmp_guess_model_value(6);
	CU_ASSERT_EQUAL(model_value, 11);
	model_value = cmp_guess_model_value(10);
	CU_ASSERT_EQUAL(model_value, 11);
	model_value = cmp_guess_model_value(11);
	CU_ASSERT_EQUAL(model_value, 11);
	model_value = cmp_guess_model_value(12);
	CU_ASSERT_EQUAL(model_value, 12);
	model_value = cmp_guess_model_value(20);
	CU_ASSERT_EQUAL(model_value, 12);
	model_value = cmp_guess_model_value(21);
	CU_ASSERT_EQUAL(model_value, 12);
	model_value = cmp_guess_model_value(22);
	CU_ASSERT_EQUAL(model_value, 13);
}


void test_cmp_mode_parse(void)
{
	uint32_t cmp_mode = ~0;
	int err;

	/* error cases */
	err = cmp_mode_parse(NULL, NULL);
	CU_ASSERT_TRUE(err);

	err = cmp_mode_parse(NULL, &cmp_mode);
	CU_ASSERT_TRUE(err);

	err = cmp_mode_parse("MODE_RAW", NULL);
	CU_ASSERT_TRUE(err);

	err = cmp_mode_parse("UNKNOWN", &cmp_mode);
	CU_ASSERT_TRUE(err);

	err = cmp_mode_parse("9999999999999999999", &cmp_mode);
	CU_ASSERT_TRUE(err);

	/* mode not defined*/
	err = cmp_mode_parse("424212", &cmp_mode);
	CU_ASSERT_TRUE(err);

	/* normal operation */
	err = cmp_mode_parse("MODE_RAW", &cmp_mode);
	CU_ASSERT_FALSE(err);
	CU_ASSERT_EQUAL(cmp_mode, MODE_RAW);

	err = cmp_mode_parse("0", &cmp_mode);
	CU_ASSERT_FALSE(err);
	CU_ASSERT_EQUAL(cmp_mode, MODE_RAW);

	err = cmp_mode_parse("0 \n", &cmp_mode);
	CU_ASSERT_FALSE(err);
	CU_ASSERT_EQUAL(cmp_mode, MODE_RAW);

	err = cmp_mode_parse(" 2 ", &cmp_mode);
	CU_ASSERT_FALSE(err);
	CU_ASSERT_EQUAL(cmp_mode, 2);

	err = cmp_mode_parse("MODE_DIFF_MULTI", &cmp_mode);
	CU_ASSERT_FALSE(err);
	CU_ASSERT_EQUAL(cmp_mode, MODE_DIFF_MULTI);
}

CU_ErrorCode cmp_tool_add_tests(CU_pSuite suite)
{
	CU_pTest  __attribute__((unused)) test;

	/* add a suite to the registry */
	suite = CU_add_suite("cmp_tool", init_cmp_tool, clean_cmp_tool);
	if (suite == NULL) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if ((NULL == CU_add_test(suite, "test of read_file8()", test_read_file8)) ||
	    (NULL == CU_add_test(suite, "test of cmp_mode_parse()", test_cmp_mode_parse))||
	    (NULL == CU_add_test(suite, "test of cmp_guess_model_value()", test_cmp_guess_model_value))||
	    (NULL == CU_add_test(suite, "test of cmp_guess()", test_cmp_guess))) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	return CUE_SUCCESS;
}


#if (__MAIN__)
int main(void)
{
	CU_pSuite suite = NULL;

	/* initialize the CUnit test registry */
	if (CU_initialize_registry() != CUE_SUCCESS)
		return CU_get_error();

	cmp_tool_add_tests(suite);

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();

	printf("\n\n\n");

	CU_basic_show_failures(CU_get_failure_list());

	CU_cleanup_registry();

	return CU_get_error();
}
#endif
