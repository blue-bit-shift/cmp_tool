/**
 * @file   cmp_rdcu_extended.h
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at),
 * @date   2021
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


#ifndef _CMP_RDCU_EXTENDED_H_
#define _CMP_RDCU_EXTENDED_H_

#include "../include/cmp_rdcu.h"

int rdcu_start_compression(void);

int rdcu_set_compression_register(const struct cmp_cfg *cfg);

int rdcu_compress_data_parallel(const struct cmp_cfg *cfg,
				const struct cmp_info *last_info);

#endif /* _CMP_RDCU_EXTENDED_H_ */
