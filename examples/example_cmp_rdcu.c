/**
 * @file   example_cmp_rdcu.c
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   Oct 2023
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
 * @brief demonstration of the use of the rdcu compressor library
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <cmp_rdcu.h>
#include <cmp_entity.h>

#define MAX_PAYLOAD_SIZE 4096
#define DATA_SAMPLES 6  /* number of 16 bit samples to compress */
#define CMP_ASW_VERSION_ID 1
#define CMP_BUF_LEN_SAMPLES DATA_SAMPLES  /* compressed buffer has the same sample size as the data buffer */
/* The start_time, end_time, model_id and counter have to be managed by the ASW
 * here we use arbitrary values for demonstration */
#define START_TIME 0
#define END_TIME 0x23
#define MODEL_ID 42
#define MODEL_COUNTER 1


/*
 * generic calls for receive and transmit RMAP packets,
 * see lib/rdcu_compress/rdcu_rmap.c for more details
 */
uint32_t rmap_rx(uint8_t *pkt);

int32_t rmap_tx(const void *hdr,  uint32_t hdr_size, const uint8_t non_crc_bytes,
		const void *data, uint32_t data_size);


int demo_rdcu_compression(void)
{
	int cnt = 0;

	/* declare configuration, status and information structure */
	struct cmp_cfg example_cfg;
	struct cmp_status example_status;
	struct cmp_info example_info;

	/* declare data buffers with some example data */
	enum cmp_data_type example_data_type = DATA_TYPE_IMAGETTE_ADAPTIVE;
	uint16_t data[DATA_SAMPLES] = {42, 23, 1, 13, 20, 1000};
	uint16_t model[DATA_SAMPLES] = {0, 22, 3, 42, 23, 16};

	/* initialise the libraries */
	rdcu_ctrl_init();
	rdcu_rmap_init(MAX_PAYLOAD_SIZE, rmap_tx, rmap_rx);

	/* set up compressor configuration */
	example_cfg = rdcu_cfg_create(example_data_type, CMP_DEF_IMA_MODEL_CMP_MODE,
				      CMP_DEF_IMA_MODEL_MODEL_VALUE, CMP_DEF_IMA_MODEL_LOSSY_PAR);
	if (example_cfg.data_type == DATA_TYPE_UNKNOWN) {
		printf("Error occurred during rdcu_cfg_create()\n");
		return -1;
	}

	if (rdcu_cfg_buffers(&example_cfg, data, DATA_SAMPLES, model, CMP_DEF_IMA_MODEL_RDCU_DATA_ADR,
			     CMP_DEF_IMA_MODEL_RDCU_MODEL_ADR, CMP_DEF_IMA_MODEL_RDCU_UP_MODEL_ADR,
			     CMP_DEF_IMA_MODEL_RDCU_BUFFER_ADR, CMP_BUF_LEN_SAMPLES)) {
		printf("Error occurred during rdcu_cfg_buffers()\n");
		return -1;
	}
	if (rdcu_cfg_imagette(&example_cfg,
			      CMP_DEF_IMA_MODEL_GOLOMB_PAR, CMP_DEF_IMA_MODEL_SPILL_PAR,
			      CMP_DEF_IMA_MODEL_AP1_GOLOMB_PAR, CMP_DEF_IMA_MODEL_AP1_SPILL_PAR,
			      CMP_DEF_IMA_MODEL_AP2_GOLOMB_PAR, CMP_DEF_IMA_MODEL_AP2_SPILL_PAR)) {
		printf("Error occurred during rdcu_cfg_imagette()\n");
		return -1;
	}

	/* start HW compression */
	if (rdcu_compress_data(&example_cfg)) {
		printf("Error occurred during rdcu_compress_data()\n");
		return -1;
	}
	/* start polling the compression status */
	/* alternatively you can wait for an interrupt from the RDCU */
	do {
		/* check compression status */
		if (rdcu_read_cmp_status(&example_status)) {
			printf("Error occurred during rdcu_read_cmp_status()");
			return -1;
		}
		cnt++;
		if (cnt > 5) {	/* wait for 5 polls */
			printf("Not waiting for compressor to become ready, will "
			       "check status and abort\n");
			/* interrupt the data compression */
			rdcu_interrupt_compression();
			/* now we may read the compression info register to get the error code */
			if (rdcu_read_cmp_info(&example_info)) {
				printf("Error occurred during rdcu_read_cmp_info()");
				return -1;
			}
			printf("Compressor error code: 0x%02X\n", example_info.cmp_err);
			return -1;
		}
	} while (!example_status.cmp_ready);

	printf("Compression took %d polling cycles\n\n", cnt);

	printf("Compressor status: ACT: %d, RDY: %d, DATA VALID: %d, INT: %d, INT_EN: %d\n",
		example_status.cmp_active, example_status.cmp_ready, example_status.data_valid,
		example_status.cmp_interrupted, example_status.rdcu_interrupt_en);

	/* now we may read the compressor registers */
	if (rdcu_read_cmp_info(&example_info)) {
		printf("Error occurred during rdcu_read_cmp_info()");
		return -1;
	}

	printf("\n\nHere's the content of the compressor registers:\n"
	       "===============================================\n");
	print_cmp_info(&example_info);

	/* check if data are valid or a compression error occurred */
	if (example_info.cmp_err != 0 || example_status.data_valid == 0) {
		printf("Compression error occurred! Compressor error code: 0x%02X\n",
		       example_info.cmp_err);
		return -1;
	}

	/* build a compression entity and put compressed data from the RDCU into it and print */
	if (1) {
		struct cmp_entity *cmp_ent;
		void *cmp_ent_data;
		size_t cmp_ent_size;
		uint32_t i, s;

		/* get the size of the compression entity */
		cmp_ent_size = cmp_ent_build(NULL, CMP_ASW_VERSION_ID,
					     START_TIME, END_TIME, MODEL_ID, MODEL_COUNTER,
					     &example_cfg, example_info.cmp_size);
		if (!cmp_ent_size) {
			printf("Error occurred during cmp_ent_build()\n");
			return -1;
		}

		/* get memory for the compression entity */
		cmp_ent = malloc(cmp_ent_size);
		if (!cmp_ent) {
			printf("Error occurred during malloc()\n");
			return -1;
		}

		/* now let us build the compression entity */
		cmp_ent_size = cmp_ent_build(cmp_ent, CMP_ASW_VERSION_ID,
					     START_TIME, END_TIME, MODEL_ID, MODEL_COUNTER,
					     &example_cfg, example_info.cmp_size);
		if (!cmp_ent_size) {
			printf("Error occurred during cmp_ent_build()\n");
			return -1;
		}

		/* get the address to store the compressed data in the
		 * compression entity */
		cmp_ent_data = cmp_ent_get_data_buf(cmp_ent);
		if (!cmp_ent_data) {
			printf("Error occurred during cmp_ent_get_data_buf()\n");
			return -1;
		}

		/* now get the compressed data form RDCU and copy it into the
		 * compression entity */
		if (rdcu_read_cmp_bitstream(&example_info, cmp_ent_data) < 0) {
			printf("Error occurred while reading in the compressed data from the RDCU\n");
			return -1;
		}

		s = cmp_ent_get_size(cmp_ent);
		printf("\n\nHere's the compressed data including the header (size %lu):\n"
		       "============================================================\n", s);
		for (i = 0; i < s; i++) {
			uint8_t *p = (uint8_t *)cmp_ent;
			printf("%02X ", p[i]);
			if (i && !((i+1) % 40))
				printf("\n");
		}
		printf("\n");

		/* now have a look into the compression entity */
		printf("\n\nParse the compression entity header:\n"
		       "====================================\n");
		cmp_ent_parse(cmp_ent);

		free(cmp_ent);
	}

	/* read updated model to some buffer and print */
	{
		int i;
		int s = rdcu_read_model(&example_info, NULL);
		uint8_t *mymodel = malloc((size_t)s);

		if (!mymodel) {
			printf("malloc failed!\n");
			return -1;
		}

		if (rdcu_read_model(&example_info, mymodel) < 0)
			printf("Error occurred while reading in the updated model\n");

		printf("\n\nHere's the updated model (size %d):\n"
		       "====================================\n", s);
		for (i = 0; i < s; i++) {
			printf("%02X ", mymodel[i]);
			if (i && !((i+1) % 40))
				printf("\n");
		}
		printf("\n");

		free(mymodel);
	}
	return 0;
}
