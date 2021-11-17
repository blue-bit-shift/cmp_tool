/**
 * @file   cmp_guess.h
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
 * @brief helps the user to find a good compression parameters for a given
 *	dataset
 */

#ifndef CMP_GUESS_H
#define CMP_GUESS_H

#include "cmp_support.h"

#define DEFAULT_GUESS_LEVEL 2

#define CMP_GUESS_DEF_MODE_DIFF		MODE_DIFF_ZERO
#define CMP_GUESS_DEF_MODE_MODEL	MODE_MODEL_MULTI

/* how often the model is updated before it is reset default value */
#define CMP_GUESS_N_MODEL_UPDATE_DEF	8

uint32_t cmp_guess(struct cmp_cfg *cfg, int level);
void cmp_guess_set_model_updates(int n_model_updates);

uint16_t cmp_guess_model_value(int n_model_updates);

#endif /* CMP_GUESS_H */
