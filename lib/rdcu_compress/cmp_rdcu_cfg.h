/**
 * @file   cmp_rdcu_cfg.h
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
 * @brief hardware compressor configuration library
 */

#ifndef CMP_RDCU_CFG_H
#define CMP_RDCU_CFG_H

#include <stdint.h>

#include "../common/cmp_support.h"


int rdcu_cfg_create(struct rdcu_cfg *rcfg, enum cmp_mode cmp_mode,
		    uint32_t model_value, uint32_t round);

int rdcu_cfg_buffers(struct rdcu_cfg *rcfg, uint16_t *data_to_compress,
		     uint32_t data_samples, uint16_t *model_of_data,
		     uint32_t rdcu_data_adr, uint32_t rdcu_model_adr,
		     uint32_t rdcu_new_model_adr, uint32_t rdcu_buffer_adr,
		     uint32_t rdcu_buffer_lenght);

int rdcu_cfg_imagette(struct rdcu_cfg *rcfg,
		      uint32_t golomb_par, uint32_t spillover_par,
		      uint32_t ap1_golomb_par, uint32_t ap1_spillover_par,
		      uint32_t ap2_golomb_par, uint32_t ap2_spillover_par);
int rdcu_cfg_imagette_default(struct rdcu_cfg *rcfg);

int rdcu_cmp_cfg_is_invalid(const struct rdcu_cfg *rcfg);

#endif /* CMP_RDCU_CFG_H */
