/**
 * @file   cmp_guess.c
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
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
 * @brief helps the user find good compression parameters for a given dataset
 * @warning this part of the software is not intended to run on-board on the ICU.
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmp_error.h>
#include <cmp_debug.h>
#include <leon_inttypes.h>
#include <cmp_data_types.h>
#include <cmp_support.h>
#include <cmp_icu.h>
#include <cmp_chunk.h>
#include <cmp_chunk_type.h>
#include <cmp_guess.h>

#define CMP_GUESS_MAX_CAL_STEPS 20274


/* how often the model is updated before it is rested */
static int num_model_updates = CMP_GUESS_N_MODEL_UPDATE_DEF;


/**
 * @brief sets how often the model is updated before the model reset;
 * @note the default value is CMP_GUESS_N_MODEL_UPDATE_DEF
 * @note this is needed to guess a good model_value
 *
 * @param n_model_updates	number of model updates
 */

void cmp_guess_set_model_updates(int n_model_updates)
{
	num_model_updates = n_model_updates;
}


/**
 * @brief guess a good model value
 *
 * @param n_model_updates	number of model updates
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
 * @brief get a good spill threshold parameter for the selected Golomb parameter
 *	and compression mode
 *
 * @param golomb_par	Golomb parameter
 * @param cmp_mode	compression mode
 *
 * @returns a good spill parameter (optimal for zero escape mechanism)
 */

uint32_t cmp_rdcu_get_good_spill(unsigned int golomb_par, enum cmp_mode cmp_mode)
{
	const uint32_t LUT_IMA_MULIT[MAX_IMA_GOLOMB_PAR+1] = {0, 8, 16, 23,
		30, 36, 44, 51, 58, 64, 71, 77, 84, 90, 97, 108, 115, 121, 128,
		135, 141, 148, 155, 161, 168, 175, 181, 188, 194, 201, 207, 214,
		229, 236, 242, 250, 256, 263, 269, 276, 283, 290, 296, 303, 310,
		317, 324, 330, 336, 344, 351, 358, 363, 370, 377, 383, 391, 397,
		405, 411, 418, 424, 431, 452 };

	if (zero_escape_mech_is_used(cmp_mode))
		return cmp_ima_max_spill(golomb_par);

	if (cmp_mode == CMP_MODE_MODEL_MULTI) {
		if (golomb_par > MAX_IMA_GOLOMB_PAR)
			return 0;
		else
			return LUT_IMA_MULIT[golomb_par];
	}

	if (cmp_mode == CMP_MODE_DIFF_MULTI)
		return CMP_GOOD_SPILL_DIFF_MULTI;

	return 0;
}


/**
 * @brief guess a good configuration with pre_cal_method
 *
 * @param rcfg	RDCU compression configuration structure
 *
 * @returns the size in bits of the compressed data of the guessed
 * configuration; 0 on error
 */

static uint32_t pre_cal_method(struct rdcu_cfg *rcfg)
{
	uint32_t g;
	uint32_t cmp_size, cmp_size_best = INT_MAX;
	uint32_t golomb_par_best = 0;
	uint32_t spill_best = 0;

	for (g = MIN_IMA_GOLOMB_PAR; g < MAX_IMA_GOLOMB_PAR; g++) {
		uint32_t s = cmp_rdcu_get_good_spill(g, rcfg->cmp_mode);

		rcfg->golomb_par = g;
		rcfg->spill = s;
		cmp_size = compress_like_rdcu(rcfg, NULL);
		if (cmp_is_error(cmp_size)) {
			return 0;
		} else if (cmp_size < cmp_size_best) {
			cmp_size_best = cmp_size;
			golomb_par_best = g;
			spill_best = s;
		}
	}
	rcfg->golomb_par = golomb_par_best;
	rcfg->spill = spill_best;

	return (uint32_t)cmp_size_best;
}


/**
 * @brief guess a good configuration with brute force method
 *
 * @param rcfg	RDCU compression configuration structure
 *
 * @returns the size in bits of the compressed data of the guessed
 * configuration; 0 on error
 */

static uint32_t brute_force(struct rdcu_cfg *rcfg)
{
	uint32_t g, s;
	uint32_t n_cal_steps = 0, last = 0;
	const uint32_t max_cal_steps = CMP_GUESS_MAX_CAL_STEPS;
	uint32_t cmp_size, cmp_size_best = INT_MAX;
	uint32_t golomb_par_best = 0;
	uint32_t spill_best = 0;
	uint32_t percent;

	/* shortcut for zero escape mechanism */
	if (zero_escape_mech_is_used(rcfg->cmp_mode))
		return pre_cal_method(rcfg);

	printf("0%%... ");
	fflush(stdout);

	for (g = MIN_IMA_GOLOMB_PAR; g < MAX_IMA_GOLOMB_PAR; g++) {
		for (s = MIN_IMA_SPILL; s < cmp_ima_max_spill(g); s++) {
			rcfg->golomb_par = g;
			rcfg->spill = s;

			cmp_size = compress_like_rdcu(rcfg, NULL);
			if (cmp_is_error(cmp_size)) {
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
			printf("%" PRIu32 "%%... ", percent);
			fflush(stdout);
		}
	}
	printf("100%% ");
	rcfg->golomb_par = golomb_par_best;
	rcfg->spill = spill_best;

	return (uint32_t)cmp_size_best;
}


/**
 * @brief guessed rdcu specific adaptive parameters
 *
 * @param rcfg	RDCU compression configuration structure
 * @note internal use only
 */

static void add_rdcu_pars_internal(struct rdcu_cfg *rcfg)
{
	if (rcfg->golomb_par == MIN_IMA_GOLOMB_PAR) {
		rcfg->ap1_golomb_par = rcfg->golomb_par + 1;
		rcfg->ap2_golomb_par = rcfg->golomb_par + 2;

	} else if (rcfg->golomb_par == MAX_IMA_GOLOMB_PAR) {
		rcfg->ap1_golomb_par = rcfg->golomb_par - 2;
		rcfg->ap2_golomb_par = rcfg->golomb_par - 1;
	} else {
		rcfg->ap1_golomb_par = rcfg->golomb_par - 1;
		rcfg->ap2_golomb_par = rcfg->golomb_par + 1;
	}

	rcfg->ap1_spill = cmp_rdcu_get_good_spill(rcfg->ap1_golomb_par, rcfg->cmp_mode);
	rcfg->ap2_spill = cmp_rdcu_get_good_spill(rcfg->ap2_golomb_par, rcfg->cmp_mode);

	if (model_mode_is_used(rcfg->cmp_mode)) {
		rcfg->rdcu_data_adr = CMP_DEF_IMA_MODEL_RDCU_DATA_ADR;
		rcfg->rdcu_model_adr = CMP_DEF_IMA_MODEL_RDCU_MODEL_ADR;
		rcfg->rdcu_new_model_adr = CMP_DEF_IMA_MODEL_RDCU_UP_MODEL_ADR;
		rcfg->rdcu_buffer_adr = CMP_DEF_IMA_MODEL_RDCU_BUFFER_ADR;
	} else {
		rcfg->rdcu_data_adr = CMP_DEF_IMA_DIFF_RDCU_DATA_ADR;
		rcfg->rdcu_model_adr = CMP_DEF_IMA_DIFF_RDCU_MODEL_ADR;
		rcfg->rdcu_new_model_adr = CMP_DEF_IMA_DIFF_RDCU_UP_MODEL_ADR;
		rcfg->rdcu_buffer_adr = CMP_DEF_IMA_DIFF_RDCU_BUFFER_ADR;
	}
}


/**
 * @brief guess a good compression configuration
 * @details use the samples, input_buf, model_buf and the cmp_mode in rcfg to
 *	find a good set of compression parameters
 * @note compression parameters in the rcfg struct (golomb_par, spill, model_value,
 *	ap1_.., ap2_.., buffer_length, ...) are overwritten by this function
 *
 * @param rcfg	RDCU compression configuration structure
 * @param level	guess_level 1 -> fast; 2 -> default; 3 -> slow(brute force)
 *
 * @returns the size in bits of the compressed data of the guessed
 * configuration; 0 on error
 */

uint32_t cmp_guess(struct rdcu_cfg *rcfg, int level)
{
	struct rdcu_cfg work_rcfg;
	uint32_t cmp_size = 0;

	if (!rcfg)
		return 0;

	if (!rcfg->input_buf)
		return 0;
	if (model_mode_is_used(rcfg->cmp_mode))
		if (!rcfg->model_buf)
			return 0;

	if (!cmp_mode_is_supported(rcfg->cmp_mode)) {
		printf("This compression mode is not implied yet.\n");
		return 0;
	}
	/* make a working copy of the input data (and model) because the
	 * following function works in-place
	 */
	work_rcfg = *rcfg;
	work_rcfg.icu_new_model_buf = NULL;
	work_rcfg.icu_output_buf = NULL;
	work_rcfg.buffer_length = 0;

	if (model_mode_is_used(rcfg->cmp_mode)) {
		work_rcfg.icu_new_model_buf = malloc(rcfg->samples * sizeof(uint16_t));
		if (!work_rcfg.icu_new_model_buf) {
			printf("malloc() failed!\n");
			goto error;
		}
	}

	/* find the best parameters */
	switch (level) {
	case 3:
		cmp_size = brute_force(&work_rcfg);
		break;
	case 1:
		printf("guess level 1 not implied for RDCU data, I use guess level 2\n");
		/* fall through */
	case 2:
		cmp_size = pre_cal_method(&work_rcfg);
		break;
	default:
		fprintf(stderr, "cmp_tool: guess level not supported for RDCU guess mode!\n");
		goto error;
	}
	if (!cmp_size)
		goto error;

	free(work_rcfg.icu_new_model_buf);

	rcfg->golomb_par = work_rcfg.golomb_par;
	rcfg->spill = work_rcfg.spill;

	rcfg->model_value = cmp_guess_model_value(num_model_updates);

	add_rdcu_pars_internal(rcfg);

	rcfg->buffer_length = ((cmp_size + 32)&~0x1FU)/(size_of_a_sample(DATA_TYPE_IMAGETTE)*8);

	return cmp_size;

error:
	free(work_rcfg.icu_new_model_buf);
	return 0;
}


/**
 * @brief get the next Golomb parameter value to try based on the guess level
 *
 * @param cur_g		current Golomb parameter value
 * @param guess_level	determines the granularity of the parameter search
 *			higher values decrease step size (finer search)
 *			lower/negative values increase step size (coarser search)
 *			range: [-31, 31], default: 2
 *
 * @returns next Golomb parameter value to try
 */

static uint32_t get_next_g_par(uint32_t cur_g, int guess_level)
{
	uint32_t result = cur_g;

	guess_level--; /* use a better guess level */

	if (guess_level > 31)
		guess_level = 31;

	if (guess_level < -31)
		guess_level = -31;


	if (guess_level >= 0)
		result += (1U << ilog_2(cur_g)) >> guess_level;
	else
		result = cur_g << -guess_level;

	if (result == cur_g)
		result++;

	return result;
}


/**
 * @brief estimate the optimal specific compression parameter set for a given chunk type
 *
 * @param chunk		pointer to the chunk data to analyse
 * @param chunk_size	size of the chunk in bytes
 * @param chunk_model	pointer to the model data (can be NULL)
 * @param cmp_par	pointer to where to store the optimized compression parameters
 * @param guess_level	controls the granularity of the parameter search; 2 is the default
 *
 * @returns the size of the compressed data with the estimated parameters; error
 *	code on failure
 */

static uint32_t cmp_guess_chunk_par(const void *chunk, uint32_t chunk_size,
				    const void *chunk_model, struct cmp_par *cmp_par,
				    int guess_level)
{
	uint32_t *param_ptrs[7] = {0};
	uint32_t cmp_size_best = ~0U;
	int i;

	if (cmp_par->lossy_par)
		debug_print("Warning: lossy compression is not supported for chunk compression, lossy_par will be ignored.");
	cmp_par->lossy_par = 0;
	cmp_par->model_value = cmp_guess_model_value(num_model_updates);

	switch (cmp_col_get_chunk_type(chunk)) {
	case CHUNK_TYPE_NCAM_IMAGETTE:
		param_ptrs[0] = &cmp_par->nc_imagette;
		break;
	case CHUNK_TYPE_SAT_IMAGETTE:
		param_ptrs[0] = &cmp_par->saturated_imagette;
		break;
	case CHUNK_TYPE_SHORT_CADENCE:
		param_ptrs[0] = &cmp_par->s_exp_flags;
		param_ptrs[1] = &cmp_par->s_fx;
		param_ptrs[2] = &cmp_par->s_ncob;
		param_ptrs[3] = &cmp_par->s_efx;
		param_ptrs[4] = &cmp_par->s_ecob;
		break;
	case CHUNK_TYPE_LONG_CADENCE:
		param_ptrs[0] = &cmp_par->l_exp_flags;
		param_ptrs[1] = &cmp_par->l_fx;
		param_ptrs[2] = &cmp_par->l_ncob;
		param_ptrs[3] = &cmp_par->l_efx;
		param_ptrs[4] = &cmp_par->l_ecob;
		param_ptrs[5] = &cmp_par->l_fx_cob_variance;
		break;
	case CHUNK_TYPE_OFFSET_BACKGROUND:
		param_ptrs[0] = &cmp_par->nc_offset_mean;
		param_ptrs[1] = &cmp_par->nc_offset_variance;
		param_ptrs[2] = &cmp_par->nc_background_mean;
		param_ptrs[3] = &cmp_par->nc_background_variance;
		param_ptrs[4] = &cmp_par->nc_background_outlier_pixels;
		break;
	case CHUNK_TYPE_SMEARING:
		param_ptrs[0] = &cmp_par->smearing_mean;
		param_ptrs[1] = &cmp_par->smearing_variance_mean;
		param_ptrs[2] = &cmp_par->smearing_outlier_pixels;
		break;
	case CHUNK_TYPE_F_CHAIN:
		param_ptrs[0] = &cmp_par->fc_imagette;
		param_ptrs[1] = &cmp_par->fc_offset_mean;
		param_ptrs[2] = &cmp_par->fc_offset_variance;
		param_ptrs[3] = &cmp_par->fc_background_mean;
		param_ptrs[4] = &cmp_par->fc_background_variance;
		param_ptrs[5] = &cmp_par->fc_background_outlier_pixels;
		break;
	case CHUNK_TYPE_UNKNOWN:
	default: /*
		  * default case never reached because cmp_col_get_chunk_type
		  * returns CHUNK_TYPE_UNKNOWN if the type is unknown
		  */
		break;
	}

	/* init */
	for (i = 0; param_ptrs[i] != NULL; i++)
		*param_ptrs[i] = 1;

	for (i = 0; param_ptrs[i] != NULL; i++) {
		uint32_t best_g = *param_ptrs[i];
		uint32_t g;

		for (g = MIN_NON_IMA_GOLOMB_PAR; g < MAX_NON_IMA_GOLOMB_PAR; g =  get_next_g_par(g, guess_level)) {
			uint32_t cmp_size;

			*param_ptrs[i] = g;
			cmp_size = compress_chunk(chunk, chunk_size, chunk_model,
						  NULL, NULL, 0, cmp_par);
			FORWARD_IF_ERROR(cmp_size, "");
			if (cmp_size < cmp_size_best) {
				cmp_size_best = cmp_size;
				best_g = g;
			}
		}
		*param_ptrs[i] = best_g;
	}

	return cmp_size_best;
}


/**
 * @brief estimate an optimal compression parameters for the given chunk
 *
 * @param chunk		pointer to the chunk data to analyse
 * @param chunk_size	size of the chunk in bytes
 * @param chunk_model	pointer to the model data (can be NULL)
 * @param cmp_par	pointer to where to store the optimized compression parameters
 * @param guess_level	controls the granularity of the parameter search; 2 is
 *			the default
 *
 * @returns the size of the compressed data with the estimated parameters; error
 *	code on failure
 */

uint32_t cmp_guess_chunk(const void *chunk, uint32_t chunk_size,
			 const void *chunk_model, struct cmp_par *cmp_par,
			 int guess_level)
{
	uint32_t cmp_size_zero, cmp_size_multi;
	struct cmp_par cmp_par_zero;
	struct cmp_par cmp_par_multi;

	memset(&cmp_par_zero, 0, sizeof(cmp_par_zero));
	memset(&cmp_par_multi, 0, sizeof(cmp_par_multi));

	if (chunk_model) {
		cmp_par_zero.cmp_mode = CMP_MODE_DIFF_ZERO;
		cmp_par_multi.cmp_mode = CMP_MODE_MODEL_MULTI;
	} else {
		cmp_par_zero.cmp_mode = CMP_MODE_DIFF_ZERO;
		cmp_par_multi.cmp_mode = CMP_MODE_DIFF_MULTI;
	}
	cmp_size_zero = cmp_guess_chunk_par(chunk, chunk_size, chunk_model,
					    &cmp_par_zero, guess_level);
	FORWARD_IF_ERROR(cmp_size_zero, "");

	cmp_size_multi = cmp_guess_chunk_par(chunk, chunk_size, chunk_model,
					     &cmp_par_multi, guess_level);
	FORWARD_IF_ERROR(cmp_size_multi, "");

	if (cmp_size_zero <= cmp_size_multi) {
		*cmp_par = cmp_par_zero;
		return cmp_size_zero;
	}

	*cmp_par = cmp_par_multi;
	return cmp_size_multi;
}
