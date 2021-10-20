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
 * @brief command line tool for PLATO ICU/RDCU compression/decompression
 * @see README.md
 * @see Data Compression User Manual PLATO-UVIE-PL-UM-0001
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/CUnit.h>
#include <CUnit/TestDB.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

#include "../../include/cmp_tool_lib.h"

#define PROGRAM_NAME "cmp_tool"

/* TODO */
int fd;

/* TODO */
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
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_2.txt: Warning: The file may contain more data than specified by the samples parameter.\n");
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
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_2.txt: Warning: The file may contain more data than specified by the samples parameter.\n");
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
	CU_ASSERT_STRING_EQUAL(s=read_std_err_log(), "cmp_tool: test_read_file8_2.txt: Error: The files does not contain enough data as given by the n_word parameter.\n");
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
	if ((NULL == CU_add_test(suite, "test of read_file8()", test_read_file8))) {
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
