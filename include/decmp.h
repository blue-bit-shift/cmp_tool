/**
 * @file   decmp.h
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
 * @brief software decompression library
 */

#ifndef DECMP_H_
#define DECMP_H_

#include "../include/cmp_support.h"

void *malloc_decompressed_data(const struct cmp_info *info);

int decompress_data(const void *compressed_data, void *de_model_buf,
		    const struct cmp_info *info, void *decompressed_data);

double get_compression_ratio(const struct cmp_info *info);

#endif /* DECMP_H_ */
