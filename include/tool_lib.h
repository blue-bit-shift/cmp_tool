/**
 * @file   tool_lib.h
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

#include "cmp_support.h"
#include "rdcu_pkt_to_file.h"

#define PROGRAM_NAME "cmp_tool"
#define MAX_CONFIG_LINE 256

#define DEFAULT_OUTPUT_PREFIX "OUTPUT"

void Print_Help(const char *argv);

int read_cmp_cfg(const char *file_name, struct cmp_cfg *cfg, int verbose_en);
int read_cmp_info(const char *file_name, struct cmp_info *info, int verbose_en);

uint16_t * read_file16(const char *file_name, uint32_t samples, int verbose_en);
uint32_t * read_file32(const char *file_name, uint32_t buf_len, int verbose_en);

int write_cmp_data_file(const void *buf, uint32_t buf_size, const char
			*output_prefix, const char *name_extension, int verbose);
int write_to_file16(const uint16_t *buf, uint32_t buf_len, const char
		    *output_prefix, const char *name_extension, int verbose);

void print_cfg(const struct cmp_cfg *cfg, int rdcu_cfg);

int write_info(const struct cmp_info *info, const char *output_prefix,
	       int machine_cfg);
