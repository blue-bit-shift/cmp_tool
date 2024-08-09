/**
 * @file   cmp_guess.h
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
 * @brief helps the user to find a good compression parameters for a given
 *	dataset
 */

#ifndef CMP_GUESS_H
#define CMP_GUESS_H

#include <cmp_support.h>


#define DEFAULT_GUESS_LEVEL 2

#define CMP_GUESS_DEF_MODE_DIFF		CMP_MODE_DIFF_ZERO
#define CMP_GUESS_DEF_MODE_MODEL	CMP_MODE_MODEL_MULTI

/* good guess for the spill parameter using the MODE_DIFF_MULTI */
#define CMP_GOOD_SPILL_DIFF_MULTI 2U
/* how often the model is updated before it is reset default value */
#define CMP_GUESS_N_MODEL_UPDATE_DEF	8

uint32_t cmp_guess(struct rdcu_cfg *rcfg, int level);
void cmp_guess_set_model_updates(int n_model_updates);

uint32_t cmp_rdcu_get_good_spill(unsigned int golomb_par, enum cmp_mode cmp_mode);

uint16_t cmp_guess_model_value(int n_model_updates);

#endif /* CMP_GUESS_H */
