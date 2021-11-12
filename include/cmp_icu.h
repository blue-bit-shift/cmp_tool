/**
 * @file   cmp_icu.h
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
 *
 *
 * @brief software compression library
 * @see Data Compression User Manual PLATO-UVIE-PL-UM-0001
 */

#ifndef _CMP_ICU_H_
#define _CMP_ICU_H_

#include "../include/cmp_support.h"

int icu_compress_data(struct cmp_cfg *cfg, struct cmp_info *info);


int cmp_pre_process(struct cmp_cfg *cfg);
int cmp_map_to_pos(struct cmp_cfg *cfg);
uint32_t cmp_encode_data(struct cmp_cfg *cfg);

#endif /* _CMP_ICU_H_ */
