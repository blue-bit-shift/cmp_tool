/**
 * @file   cmp_support.h
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at),
 * @date   2018
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
 * @brief compression/decompression helper functions
 */

#ifndef CMP_SUPPORT_H
#define CMP_SUPPORT_H

#include <stdint.h>
#include <stddef.h>


/* return code if the bitstream buffer is too small to store the whole bitstream */
#define CMP_ERROR_SMALL_BUF -2

/* return code if the value or the model is bigger than the max_used_bits
 * parameter allows
 */
#define CMP_ERROR_HIGH_VALUE -3

#define CMP_LOSSLESS	0
#define CMP_PAR_UNUNSED	0


#define MAX_MODEL_VALUE                                                        \
	16U /* the maximal model values used in the update equation for the new model */

/* valid compression parameter ranges for RDCU compression according to PLATO-UVIE-PL-UM-0001 */
#define MAX_RDCU_CMP_MODE	4U
#define MIN_RDCU_GOLOMB_PAR	1U
#define MAX_RDCU_GOLOMB_PAR	63U
#define MIN_RDCU_SPILL		2U
#define MAX_RDCU_ROUND		2U
/* for maximum spill value look at cmp_rdcu_max_spill function */

/* valid compression parameter ranges for ICU compression */
#define MIN_ICU_GOLOMB_PAR	1U
#define MAX_ICU_GOLOMB_PAR	UINT16_MAX /* the compression entity dos not allow larger values */
#define MIN_ICU_SPILL		2U
/* for maximum spill value look at cmp_icu_max_spill function */
#define MAX_ICU_ROUND		3U
#define MAX_STUFF_CMP_PAR	32U


/* default imagette RDCU compression parameters for model compression */
#define CMP_DEF_IMA_MODEL_DATA_TYPE		DATA_TYPE_IMAGETTE
#define CMP_DEF_IMA_MODEL_CMP_MODE		CMP_MODE_MODEL_MULTI
#define CMP_DEF_IMA_MODEL_MODEL_VALUE		8
#define CMP_DEF_IMA_MODEL_LOSSY_PAR		0

#define CMP_DEF_IMA_MODEL_GOLOMB_PAR		4
#define CMP_DEF_IMA_MODEL_SPILL_PAR		48
#define CMP_DEF_IMA_MODEL_AP1_GOLOMB_PAR	3
#define CMP_DEF_IMA_MODEL_AP1_SPILL_PAR		35
#define CMP_DEF_IMA_MODEL_AP2_GOLOMB_PAR	5
#define CMP_DEF_IMA_MODEL_AP2_SPILL_PAR		60

#define CMP_DEF_IMA_MODEL_RDCU_DATA_ADR		0x000000
#define CMP_DEF_IMA_MODEL_RDCU_MODEL_ADR	0x200000
#define CMP_DEF_IMA_MODEL_RDCU_UP_MODEL_ADR	0x400000
#define CMP_DEF_IMA_MODEL_RDCU_BUFFER_ADR	0x600000

/* default imagette RDCU compression parameters for 1d-differencing compression */
#define CMP_DEF_IMA_DIFF_DATA_TYPE		DATA_TYPE_IMAGETTE
#define CMP_DEF_IMA_DIFF_CMP_MODE		CMP_MODE_DIFF_ZERO
#define CMP_DEF_IMA_DIFF_MODEL_VALUE		8 /* not needed for 1d-differencing cmp_mode */
#define CMP_DEF_IMA_DIFF_LOSSY_PAR		0

#define CMP_DEF_IMA_DIFF_GOLOMB_PAR		7
#define CMP_DEF_IMA_DIFF_SPILL_PAR		60
#define CMP_DEF_IMA_DIFF_AP1_GOLOMB_PAR		6
#define CMP_DEF_IMA_DIFF_AP1_SPILL_PAR		48
#define CMP_DEF_IMA_DIFF_AP2_GOLOMB_PAR		8
#define CMP_DEF_IMA_DIFF_AP2_SPILL_PAR		72

#define CMP_DEF_IMA_DIFF_RDCU_DATA_ADR		0x000000
#define CMP_DEF_IMA_DIFF_RDCU_MODEL_ADR		0x000000 /* not needed for 1d-differencing cmp_mode */
#define CMP_DEF_IMA_DIFF_RDCU_UP_MODEL_ADR	0x000000 /* not needed for 1d-differencing cmp_mode */
#define CMP_DEF_IMA_DIFF_RDCU_BUFFER_ADR	0x600000


/* defined compression data product types */
enum cmp_data_type {
	DATA_TYPE_UNKNOWN,
	DATA_TYPE_IMAGETTE,
	DATA_TYPE_IMAGETTE_ADAPTIVE,
	DATA_TYPE_SAT_IMAGETTE,
	DATA_TYPE_SAT_IMAGETTE_ADAPTIVE,
	DATA_TYPE_OFFSET,
	DATA_TYPE_BACKGROUND,
	DATA_TYPE_SMEARING,
	DATA_TYPE_S_FX,
	DATA_TYPE_S_FX_EFX,
	DATA_TYPE_S_FX_NCOB,
	DATA_TYPE_S_FX_EFX_NCOB_ECOB,
	DATA_TYPE_L_FX,
	DATA_TYPE_L_FX_EFX,
	DATA_TYPE_L_FX_NCOB,
	DATA_TYPE_L_FX_EFX_NCOB_ECOB,
	DATA_TYPE_F_FX,
	DATA_TYPE_F_FX_EFX,
	DATA_TYPE_F_FX_NCOB,
	DATA_TYPE_F_FX_EFX_NCOB_ECOB,
	DATA_TYPE_F_CAM_IMAGETTE,
	DATA_TYPE_F_CAM_IMAGETTE_ADAPTIVE,
	DATA_TYPE_F_CAM_OFFSET,
	DATA_TYPE_F_CAM_BACKGROUND
};


/* defined compression mode */
enum cmp_mode {
	CMP_MODE_RAW,
	CMP_MODE_MODEL_ZERO,
	CMP_MODE_DIFF_ZERO,
	CMP_MODE_MODEL_MULTI,
	CMP_MODE_DIFF_MULTI,
	CMP_MODE_STUFF
};


/**
 * @brief The cmp_cfg structure can contain the complete configuration of the HW as
 *      well as the SW compressor.
 * @note the icu_output_buf will not be used for HW compression
 * @note the rdcu_***_adr parameters are ignored for SW compression
 */

struct cmp_cfg {
	void *input_buf;            /* Pointer to the data to compress buffer */
	void *model_buf;            /* Pointer to the model buffer */
	void *icu_new_model_buf;    /* Pointer to the updated model buffer (not used for RDCU compression )*/
	uint32_t *icu_output_buf;   /* Pointer to the compressed data buffer (not used for RDCU compression) */
	uint32_t samples;           /* Number of samples to compress, length of the data and model buffer
				     * (including the multi entity header by non-imagette data)
				     */
	uint32_t buffer_length;     /* Length of the compressed data buffer in number of samples */
	uint32_t rdcu_data_adr;     /* RDCU data to compress start address, the first data address in the RDCU SRAM; HW only */
	uint32_t rdcu_model_adr;    /* RDCU model start address, the first model address in the RDCU SRAM */
	uint32_t rdcu_new_model_adr;/* RDCU updated model start address, the address in the RDCU SRAM where the updated model is stored */
	uint32_t rdcu_buffer_adr;   /* RDCU compressed data start address, the first output data address in the RDCU SRAM */
	enum cmp_data_type data_type; /* Compression Data Product Types */
	uint32_t cmp_mode;          /* 0: raw mode
				     * 1: model mode with zero escape symbol mechanism
				     * 2: 1d differencing mode without input model with zero escape symbol mechanism
				     * 3: model mode with multi escape symbol mechanism
				     * 4: 1d differencing mode without input model multi escape symbol mechanism
				     */
	uint32_t model_value;       /* Model weighting parameter */
	uint32_t round;             /* lossy compression parameter */
	uint32_t golomb_par;        /* Golomb parameter for imagette data compression */
	uint32_t spill;             /* Spillover threshold parameter for imagette compression */
	uint32_t ap1_golomb_par;    /* Adaptive 1 spillover threshold for imagette data; HW only */
	uint32_t ap1_spill;         /* Adaptive 1 Golomb parameter for imagette data; HW only */
	uint32_t ap2_golomb_par;    /* Adaptive 2 spillover threshold for imagette data; HW only */
	uint32_t ap2_spill;         /* Adaptive 2 Golomb parameter; HW only */
	uint32_t cmp_par_exp_flags; /* Compression parameter for exposure flags compression */
	uint32_t spill_exp_flags;   /* Spillover threshold parameter for exposure flags compression */
	uint32_t cmp_par_fx;	    /* Compression parameter for normal flux compression */
	uint32_t spill_fx;          /* Spillover threshold parameter for normal flux compression */
	uint32_t cmp_par_ncob;      /* Compression parameter for normal center of brightness compression */
	uint32_t spill_ncob;        /* Spillover threshold parameter for normal center of brightness compression */
	uint32_t cmp_par_efx;       /* Compression parameter for extended flux compression */
	uint32_t spill_efx;         /* Spillover threshold parameter for extended flux compression */
	uint32_t cmp_par_ecob;      /* Compression parameter for executed center of brightness compression */
	uint32_t spill_ecob;        /* Spillover threshold parameter for executed center of brightness compression */
	uint32_t cmp_par_fx_cob_variance; /* Compression parameter for flux/COB variance compression */
	uint32_t spill_fx_cob_variance; /* Spillover threshold parameter for flux/COB variance compression */
	uint32_t cmp_par_mean;      /* Compression parameter for auxiliary science mean compression */
	uint32_t spill_mean;        /* Spillover threshold parameter for auxiliary science mean compression */
	uint32_t cmp_par_variance;  /* Compression parameter for auxiliary science variance compression */
	uint32_t spill_variance;    /* Spillover threshold parameter for auxiliary science variance compression */
	uint32_t cmp_par_pixels_error; /* Compression parameter for auxiliary science outlier pixels number compression */
	uint32_t spill_pixels_error; /* Spillover threshold parameter for auxiliary science outlier pixels number compression */
};


/* The cmp_status structure can contain the information of the compressor status
 * register from the RDCU, see RDCU-FRS-FN-0632.
 */

struct cmp_status {
	uint8_t cmp_ready; /* Data Compressor Ready; 0: Compressor is busy 1: Compressor is ready */
	uint8_t cmp_active; /* Data Compressor Active; 0: Compressor is on hold; 1: Compressor is active */
	uint8_t data_valid; /* Compressor Data Valid; 0: Data is invalid; 1: Data is valid */
	uint8_t cmp_interrupted; /* Data Compressor Interrupted; HW only; 0: No compressor interruption; 1: Compressor was interrupted */
	uint8_t rdcu_interrupt_en; /* RDCU Interrupt Enable; HW only; 0: Interrupt is disabled; 1: Interrupt is enabled */
};


/* The cmp_info structure can contain the information and metadata of an
 * executed RDCU compression.
 */

struct cmp_info {
	uint32_t cmp_mode_used;       /* Compression mode used */
	uint32_t spill_used;          /* Spillover threshold used */
	uint32_t golomb_par_used;     /* Golomb parameter used */
	uint32_t samples_used;        /* Number of samples (16 bit value) to be stored */
	uint32_t cmp_size;            /* Compressed data size; measured in bits */
	uint32_t ap1_cmp_size;        /* Adaptive compressed data size 1; measured in bits */
	uint32_t ap2_cmp_size;        /* Adaptive compressed data size 2; measured in bits */
	uint32_t rdcu_new_model_adr_used; /* Updated model start  address used */
	uint32_t rdcu_cmp_adr_used;   /* Compressed data start address */
	uint8_t  model_value_used;    /* Model weighting parameter used */
	uint8_t  round_used;          /* Number of noise bits to be rounded used */
	uint16_t cmp_err;             /* Compressor errors
				       * [bit 0] small_buffer_err; The length for the compressed data buffer is too small
				       * [bit 1] cmp_mode_err; The cmp_mode parameter is not set correctly
				       * [bit 2] model_value_err; The model_value parameter is not set correctly
				       * [bit 3] cmp_par_err; The spill, golomb_par combination is not set correctly
				       * [bit 4] ap1_cmp_par_err; The ap1_spill, ap1_golomb_par combination is not set correctly (only HW compression)
				       * [bit 5] ap2_cmp_par_err; The ap2_spill, ap2_golomb_par combination is not set correctly (only HW compression)
				       * [bit 6] mb_err; Multi bit error detected by the memory controller (only HW compression)
				       * [bit 7] slave_busy_err; The bus master has received the "slave busy" status (only HW compression)
				       * [bit 8] slave_blocked_err; The bus master has received the “slave blocked” status (only HW compression)
				       * [bit 9] invalid address_err; The bus master has received the “invalid address” status (only HW compression)
				       */
};


int is_a_pow_of_2(unsigned int v);
int ilog_2(uint32_t x);

unsigned int cmp_bit_to_4byte(unsigned int cmp_size_bit);

int cmp_cfg_is_valid(const struct cmp_cfg *cfg);
int cmp_cfg_icu_gen_par_is_valid(const struct cmp_cfg *cfg);
int cmp_cfg_icu_buffers_is_valid(const struct cmp_cfg *cfg);
int cmp_cfg_imagette_is_valid(const struct cmp_cfg *cfg);
int cmp_cfg_fx_cob_is_valid(const struct cmp_cfg *cfg);
int cmp_cfg_aux_is_valid(const struct cmp_cfg *cfg);
uint32_t cmp_rdcu_max_spill(unsigned int golomb_par);
uint32_t cmp_icu_max_spill(unsigned int cmp_par);

int cmp_data_type_valid(enum cmp_data_type data_type);
int rdcu_supported_data_type_is_used(enum cmp_data_type data_type);
int cmp_imagette_data_type_is_used(enum cmp_data_type data_type);
int cmp_ap_imagette_data_type_is_used(enum cmp_data_type data_type);
int cmp_fx_cob_data_type_is_used(enum cmp_data_type data_type);
int cmp_aux_data_type_is_used(enum cmp_data_type data_type);

int cmp_mode_is_supported(enum cmp_mode cmp_mode);
int model_mode_is_used(enum cmp_mode cmp_mode);
int diff_mode_is_used(enum cmp_mode cmp_mode);
int raw_mode_is_used(enum cmp_mode cmp_mode);
int rdcu_supported_cmp_mode_is_used(enum cmp_mode cmp_mode);
int zero_escape_mech_is_used(enum cmp_mode cmp_mode);
int multi_escape_mech_is_used(enum cmp_mode cmp_mode);

unsigned int round_fwd(unsigned int value, unsigned int round);
unsigned int round_inv(unsigned int value, unsigned int round);
unsigned int cmp_up_model(unsigned int data, unsigned int model,
			  unsigned int model_value, unsigned int round);


void print_cmp_cfg(const struct cmp_cfg *cfg);
void print_cmp_info(const struct cmp_info *info);

#endif /* CMP_SUPPORT_H */
