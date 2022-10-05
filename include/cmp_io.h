/**
 * @file   cmp_io.h
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

#include <string.h>

#include <cmp_support.h>
#include <cmp_entity.h>

#define MAX_CONFIG_LINE 256

#define DEFAULT_OUTPUT_PREFIX "OUTPUT"

#define BUFFER_LENGTH_DEF_FAKTOR 2

void print_help(const char *program_name);

int cmp_cfg_read(const char *file_name, struct cmp_cfg *cfg, int verbose_en);
int cmp_info_read(const char *file_name, struct cmp_info *info, int verbose_en);

ssize_t read_file8(const char *file_name, uint8_t *buf, uint32_t buf_size,
		   int verbose_en);
ssize_t read_file_data(const char *file_name, enum cmp_data_type data_type,
		       void *buf, uint32_t buf_size, int verbose_en);
ssize_t read_file_cmp_entity(const char *file_name, struct cmp_entity *ent,
			     uint32_t ent_size, int verbose_en);
ssize_t read_file32(const char *file_name, uint32_t *buf, uint32_t buf_size,
		    int verbose_en);

uint32_t cmp_tool_gen_version_id(const char *version);

int write_cmp_data_file(const void *buf, uint32_t buf_size, const char
			*output_prefix, const char *name_extension, int verbose);
int write_input_data_to_file(void *data, uint32_t data_size, enum cmp_data_type data_type,
			     const char *output_prefix, const char *name_extension, int verbose);
int cmp_info_to_file(const struct cmp_info *info, const char *output_prefix, int rdcu_cfg);
int cmp_cfg_fo_file(const struct cmp_cfg *cfg, const char *output_prefix, int verbose);
void cmp_cfg_print(const struct cmp_cfg *cfg);

int atoui32(const char *dep_str, const char *val_str, uint32_t *red_val);
int cmp_mode_parse(const char *cmp_mode_str, uint32_t *cmp_mode);

enum cmp_data_type string2data_type(const char *data_type_str);
const char *data_type2string(enum cmp_data_type data_type);
