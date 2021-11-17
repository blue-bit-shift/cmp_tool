/**
 * @file   cmp_guess.c
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
 *
 * @brief helps the user to find a good compression parameters for a given
 *	dataset
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/cmp_icu.h"
#include "../include/cmp_guess.h"


/* how often the model is updated before it is rested */
static int num_model_updates = CMP_GUESS_N_MODEL_UPDATE_DEF;


/**
 * @brief sets how often the model is updated before model reset for the
 * cmp_guess function
 * @note the default value is CMP_GUESS_N_MODEL_UPDATE_DEF
 * @note this is needed to guess a good model_value
 *
 * @param n_model_updates number of model updates
 */

void cmp_guess_set_model_updates(int n_model_updates)
{
	num_model_updates = n_model_updates;
}


/**
 * @brief guess a good model value
 *
 * @param n_model_updates number of model updates
 *
 * @returns guessed model_value
 */

uint16_t cmp_guess_model_value(int n_model_updates)
{
	if (n_model_updates <= 2)
		return 8;
	if (n_model_updates <= 5)
		return 10;
	if (n_model_updates <= 11)
		return 11;
	if (n_model_updates <= 21)
		return 12;

	return 13;
}


/**
 * @brief guess a good configuration with pre_cal_method
 *
 * @param cfg compression configuration structure
 *
 * @returns the size in bits of the compressed data of the guessed
 * configuration; 0 on error
 */

static uint32_t pre_cal_method(struct cmp_cfg *cfg)
{
	uint32_t g;
	uint32_t cmp_size;
	uint32_t cmp_size_best = ~0U;
	uint32_t golomb_par_best = 0;
	uint32_t spill_best = 0;

	for (g = MIN_RDCU_GOLOMB_PAR; g < MAX_RDCU_GOLOMB_PAR; g++) {
		uint32_t s = cmp_get_good_spill(g, cfg->cmp_mode);

		cfg->golomb_par = g;
		cfg->spill = s;
		cmp_size = cmp_encode_data(cfg);
		if (cmp_size == 0) {
			return 0;
		} else if (cmp_size < cmp_size_best) {
			cmp_size_best = cmp_size;
			golomb_par_best = g;
			spill_best = s;
		}
	}
	cfg->golomb_par = golomb_par_best;
	cfg->spill = spill_best;

	return cmp_size_best;
}


/**
 * @brief guess a good configuration with brute force method
 *
 * @param cfg compression configuration structure
 *
 * @returns the size in bits of the compressed data of the guessed
 * configuration; 0 on error
 */

#define CMP_GUESS_MAX_CAL_STEPS 20274
static uint32_t brute_force(struct cmp_cfg *cfg)
{
	uint32_t g, s;
	uint32_t n_cal_steps = 0, last = 0;
	const uint32_t max_cal_steps = CMP_GUESS_MAX_CAL_STEPS;
	uint32_t cmp_size;
	uint32_t cmp_size_best = ~0U;
	uint32_t golomb_par_best = 0;
	uint32_t spill_best = 0;
	uint32_t percent;

	/* short cut for zero escape mechanism */
	if (zero_escape_mech_is_used(cfg->cmp_mode))
		return pre_cal_method(cfg);

	printf("0%%... ");
	fflush(stdout);

	for (g = MIN_RDCU_GOLOMB_PAR; g < MAX_RDCU_GOLOMB_PAR; g++) {
		for (s = MIN_RDCU_SPILL; s < get_max_spill(g, cfg->cmp_mode); s++) {
			cfg->golomb_par = g;
			cfg->spill = s;

			cmp_size = cmp_encode_data(cfg);
			if (cmp_size == 0) {
				return 0;
			} else if (cmp_size < cmp_size_best) {
				cmp_size_best = cmp_size;
				golomb_par_best = g;
				spill_best = s;
			}
		}
		n_cal_steps += s;

		percent = n_cal_steps*100/max_cal_steps;
		if (percent > 5+last && percent < 100) {
			last = percent;
			printf("%u%%... ", percent);
			fflush(stdout);
		}
	}
	printf("100%% ");
	cfg->golomb_par = golomb_par_best;
	cfg->spill = spill_best;

	return cmp_size_best;
}


/**
 * @brief guessed rdcu specific adaptive parameters
 *
 * @param cfg compression configuration structure
 * @note internal use only
 */

static void add_rdcu_pars_internal(struct cmp_cfg *cfg)
{
	if (cfg->golomb_par == MIN_RDCU_GOLOMB_PAR) {
		cfg->ap1_golomb_par = cfg->golomb_par + 1;
		cfg->ap2_golomb_par = cfg->golomb_par + 2;

	} else if (cfg->golomb_par == MAX_RDCU_GOLOMB_PAR) {
		cfg->ap1_golomb_par = cfg->golomb_par - 2;
		cfg->ap2_golomb_par = cfg->golomb_par - 1;
	} else {
		cfg->ap1_golomb_par = cfg->golomb_par - 1;
		cfg->ap2_golomb_par = cfg->golomb_par + 1;
	}

	cfg->ap1_spill = cmp_get_good_spill(cfg->ap1_golomb_par, cfg->cmp_mode);
	cfg->ap2_spill = cmp_get_good_spill(cfg->ap2_golomb_par, cfg->cmp_mode);

	if (model_mode_is_used(cfg->cmp_mode)) {
		cfg->rdcu_data_adr = DEFAULT_CFG_MODEL.rdcu_data_adr;
		cfg->rdcu_model_adr = DEFAULT_CFG_MODEL.rdcu_model_adr;
		cfg->rdcu_new_model_adr = DEFAULT_CFG_MODEL.rdcu_new_model_adr;
		cfg->rdcu_buffer_adr = DEFAULT_CFG_MODEL.rdcu_buffer_adr;
	} else {
		cfg->rdcu_data_adr = DEFAULT_CFG_DIFF.rdcu_data_adr;
		cfg->rdcu_model_adr = DEFAULT_CFG_DIFF.rdcu_model_adr;
		cfg->rdcu_new_model_adr = DEFAULT_CFG_DIFF.rdcu_new_model_adr;
		cfg->rdcu_buffer_adr = DEFAULT_CFG_DIFF.rdcu_buffer_adr;
	}
}


/**
 * @brief guess a good compression configuration
 * @details use the referenced in the cfg struct (samples, input_buf, model_buf
 * (optional)) and the cmp_mode to find a good set of compression parameters
 * @note compression parameters in the cfg struct (golomb_par, spill, model_value,
 * ap1_.., ap2_.., buffer_length, ...) are overwritten by this function
 *
 * @param cfg compression configuration structure
 * @param level guess_level 1 -> fast; 2 -> default; 3 -> slow(brute force)
 *
 * @returns the size in bits of the compressed data of the guessed
 * configuration; 0 on error
 */

uint32_t cmp_guess(struct cmp_cfg *cfg, int level)
{
	size_t size;
	struct cmp_cfg work_cfg;
	int err;
	uint32_t cmp_size = 0;

	if (!cfg)
		return 0;

	if (!cfg->input_buf)
		return 0;

	if (!rdcu_supported_mode_is_used(cfg->cmp_mode)) {
		printf("This compression mode is not implied yet.\n");
		return 0;
	}
	/* make a working copy of the input data (and model) because the
	 * following function works inplace */
	work_cfg = *cfg;
	work_cfg.input_buf = NULL;
	work_cfg.model_buf = NULL;
	work_cfg.icu_new_model_buf = NULL;
	work_cfg.icu_output_buf = NULL;
	work_cfg.buffer_length = 0;

	size = cfg->samples * size_of_a_sample(work_cfg.cmp_mode);
	if (size == 0)
		goto error;
	work_cfg.input_buf = malloc(size);
	if (!work_cfg.input_buf) {
		printf("malloc() failed!\n");
		goto error;
	}
	memcpy(work_cfg.input_buf, cfg->input_buf, size);

	if (cfg->model_buf && model_mode_is_used(cfg->cmp_mode)) {
		work_cfg.model_buf = malloc(size);
		if (!work_cfg.model_buf) {
			printf("malloc() failed!\n");
			goto error;
		}
		memcpy(work_cfg.model_buf, cfg->model_buf, size);

		work_cfg.icu_new_model_buf = malloc(size);
		if (!work_cfg.icu_new_model_buf) {
			printf("malloc() failed!\n");
			goto error;
		}
	}

	err = cmp_pre_process(&work_cfg);
	if (err)
		goto error;

	err = cmp_map_to_pos(&work_cfg);
	if (err)
		goto error;

	/* find the best parameters */
	switch (level) {
	case 3:
		cmp_size = brute_force(&work_cfg);
		break;
	case 1:
		printf("guess level 1 not implied yet use guess level 2\n");
		/* fall through */
	case 2:
		cmp_size = pre_cal_method(&work_cfg);
		break;
	default:
		fprintf(stderr, "cmp_tool: guess level not supported!\n");
		goto error;
		break;
	}
	if (!cmp_size)
		goto error;

	free(work_cfg.input_buf);
	free(work_cfg.model_buf);
	free(work_cfg.icu_new_model_buf);

	cfg->golomb_par = work_cfg.golomb_par;
	cfg->spill = work_cfg.spill;

	cfg->model_value = cmp_guess_model_value(num_model_updates);

	if (rdcu_supported_mode_is_used(cfg->cmp_mode))
		add_rdcu_pars_internal(cfg);

	cfg->buffer_length = ((cmp_size + 32)&~0x1FU)/(size_of_a_sample(work_cfg.cmp_mode)*8);

	return cmp_size;

error:
	free(work_cfg.input_buf);
	free(work_cfg.model_buf);
	free(work_cfg.icu_new_model_buf);
	return 0;
}

