/**
 * @file   rdcu_pkt_to_file.c
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
 * @brief RDCU packets to file library
 *
 * This library provided a rmap_rx and rmap_tx function for the rdcu_rmap
 * library to write generated packets into text files.
 * @warning this part of the software is not intended to run on-board on the ICU.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#include <cmp_support.h>
#include <rdcu_pkt_to_file.h>
#include <cmp_rdcu.h>
#include <rdcu_rmap.h>
#include <rdcu_ctrl.h>
#include <rdcu_cmd.h>


int rdcu_compress_data_parallel(const struct cmp_cfg *cfg, const struct cmp_info *last_info);


/* Name of directory were the RMAP packages are stored */
static char tc_folder_dir[MAX_TC_FOLDER_DIR_LEN] = "TC_FILES";


/**
 * @brief set the directory name were the RMAP packages are stored
 *
 * @param dir_name	Name of directory were the RMAP packages are stored
 */

void set_tc_folder_dir(const char *dir_name)
{
	if (!dir_name)
		return;

	strncpy(tc_folder_dir, dir_name, sizeof(tc_folder_dir));
	/*  Ensure null-termination. */
	tc_folder_dir[sizeof(tc_folder_dir) - 1] = '\0';
}


/**
 * @brief return a file to write with the name format dir_name/XXX.tc, where XXX
 *	is n_tc in decimal representation
 *
 * @param dir_name	name of directory were the RMAP packages are stored
 * @param n_tc		number of TC commands
 *
 * @return	pointer to the new file stream
 * @see https://developers.redhat.com/blog/2018/05/24/detecting-string-truncation-with-gcc-8/
 */

static FILE *open_file(const char *dir_name, int n_tc)
{
	char *pathname;
	FILE *fp;
	int n;

	errno = 0;
	n = snprintf(NULL, 0, "%s/%04d.tc", dir_name, n_tc);
	if (n < 0) {
		perror("snprintf failed");
		abort();
	}

	errno = 0;
	pathname = (char *)malloc((size_t)n + 1);
	if (!pathname) {
		perror("malloc failed");
		abort();
	}

	errno = 0;
	n = snprintf(pathname, (size_t)n + 1, "%s/%04d.tc", dir_name, n_tc);
	if (n < 0) {
		perror("snprintf failed");
		abort();
	}

	fp = fopen(pathname, "w");
	free(pathname);
	return fp;
}


/**
 * @brief Implementation of the rmap_rx function for the rdcu_rmap lib. All
 *	generated packages are write to a file.
 */

static int32_t rmap_tx_to_file(const void *hdr, uint32_t hdr_size,
			       const uint8_t non_crc_bytes, const void *data,
			       uint32_t data_size)
{
	static int n_pkt = 1; /* number of packets */
	static char tc_folder_dir_old[MAX_TC_FOLDER_DIR_LEN] = {0};
	uint8_t *blob = NULL;
	int n, i;
	FILE *fp;

	if (hdr == NULL)
		return 0;

	/* make sure that string is null-terminated */
	tc_folder_dir[MAX_TC_FOLDER_DIR_LEN - 1] = '\0';

	if (strcmp(tc_folder_dir, tc_folder_dir_old) != 0) {
		struct stat st = { 0 };

		n_pkt = 1;

		/* creating a directory if that directory does not exist */
		if (stat(tc_folder_dir, &st) == -1) {
			int err;
			#if defined(_WIN32) || defined(_WIN64)
				err = mkdir(tc_folder_dir);
			#else
				err = mkdir(tc_folder_dir, 0700);
			#endif
			if (err)
				return -1;
		}

		strncpy(tc_folder_dir_old, tc_folder_dir,
			sizeof(tc_folder_dir_old));
		tc_folder_dir_old[sizeof(tc_folder_dir_old) - 1] = '\0';
	}

	n = rdcu_package(NULL, hdr, hdr_size, non_crc_bytes, data, data_size);
	if (n <= 0)
		return -1;
	blob = malloc(n);
	if (!blob) {
		printf("malloc for tx_pkt faild\n");
		return -1;
	}

	n = rdcu_package(blob, hdr, hdr_size, non_crc_bytes, data, data_size);
	if (n <= 0) {
		free(blob);
		return -1;
	}

	fp = open_file(tc_folder_dir, n_pkt);

	if (fp == NULL) {
		perror("fopen()");
		free(blob);
		return -1;
	}

	for (i = 0; i < n; i++) {
		/* printf("%02X ", blob[i]); */
		fprintf(fp, "%02X ", blob[i]);
		/* if (i && !((i + 1) % 40)) */
		/* printf("\n"); */
	}

	/* printf("\n"); */
	fprintf(fp, "\n");
	fclose(fp);

	free(blob);

	n_pkt++;
	return 0;
}


/**
 * @brief Dummy implementation of the rmap_rx function for the rdcu_rmap lib. We
 * do not want to receive any packages.
 */

static uint32_t rmap_rx_dummy(uint8_t *pkt)
{
	(void)(pkt);
	return 0;
}


/**
 * @brief read out .rdcu_pkt_mode_cfg configure file
 *
 * @param icu_addr	RMAP source logical address
 * @param rdcu_addr	RMAP destination logical address
 * @param mtu		the maximum data transfer size per unit
 *
 * @returns 0 on success, otherwise error
 */

static int read_rdcu_pkt_mode_cfg(uint8_t *icu_addr, uint8_t *rdcu_addr,
				  int *mtu)
{
	/* TODO: Build string" %s/.rdcu_pkt_mode_cfg", RDCU_PKT_MODE_DIR */
	char line[256];
	char *end;
	unsigned int read_all = 0;
	FILE *fp = fopen(".rdcu_pkt_mode_cfg", "r");

	*icu_addr = 0;
	*rdcu_addr = 0;
	*mtu = 0;

	if (fp == NULL) {
		perror("fopen()");
		return -1;
	}

	while (fgets(line, sizeof(line), fp)) {
		size_t l;
		long i;
		char *p;

		p = strchr(line, '\n');
		if (p) {
			*p = '\0';
		} else {
			fprintf(stderr, "Error read in line to long.\n");
			fclose(fp);
			return -1;
		}

		if (line[0] == ' ' || line[0] == '\t' || line[0] == '#')
			continue;
		if (!strncmp(line, "ICU_ADDR", l = strlen("ICU_ADDR"))) {
			end = NULL;
			errno = 0;
			i = strtol(line + l, &end, 0);
			if (end == line + l || errno == ERANGE || i < 0 ||
			    i > 0xFF) {
				fprintf(stderr, "Error reading ICU_ADDR.\n");
				errno = 0;
				fclose(fp);
				return -1;
			}
			*icu_addr = (uint8_t)i;
			read_all |= 1UL << 0;
			continue;
		}
		if (!strncmp(line, "RDCU_ADDR", l = strlen("RDCU_ADDR"))) {
			end = NULL;
			errno = 0;
			i = strtol(line + l, &end, 0);
			if (end == line + l || errno == ERANGE || i < 0
			    || i > 0xFF) {
				fprintf(stderr, "Error reading RDCU_ADDR.\n");
				errno = 0;
				fclose(fp);
				return -1;
			}
			*rdcu_addr = (uint8_t)i;
			read_all |= 1UL << 1;
			continue;
		}
		if (!strncmp(line, "MTU", l = strlen("MTU"))) {
			end = NULL;
			errno = 0;
			i = strtol(line + l, &end, 0);
			if (end == line + l || errno == ERANGE || i < 0 ||
			    i > INT_MAX) {
				fprintf(stderr, "Error reading MTU.\n");
				errno = 0;
				fclose(fp);
				return -1;
			}
			*mtu = (int)i;
			read_all |= 1UL << 2;
			continue;
		}
	}
	fclose(fp);

	/* all keywords read? */
	if (read_all < 0x7)
		return -1;

	printf("Use ICU_ADDR = %#02X, RDCU_ADDR = %#02X and MTU = %d for the RAMP packets.\n", *icu_addr, *rdcu_addr, *mtu);

	return 0;
}


/**
 * @brief initialise the RDCU packets to file library
 *
 * @note use the .rdcu_pkt_mode_cfg file to read the icu_addr, rdcu_addr and mtu
 *	parameters
 *
 * @returns 0 on success, otherwise error
 */

int init_rmap_pkt_to_file(void)
{
	uint8_t icu_addr, rdcu_addr;
	int mtu;

	if (read_rdcu_pkt_mode_cfg(&icu_addr, &rdcu_addr, &mtu)) {
		icu_addr = DEF_ICU_ADDR;
		rdcu_addr = DEF_RDCU_ADDR;
		mtu = DEF_MTU;
	}
	rdcu_ctrl_init();
	rdcu_set_source_logical_address(icu_addr);
	rdcu_set_destination_logical_address(rdcu_addr);
	rdcu_set_destination_key(RDCU_DEST_KEY);
	rdcu_rmap_init(mtu, rmap_tx_to_file, rmap_rx_dummy);
	return 0;
}


/**
 * @brief generate the rmap packets to set up an RDCU compression
 * @note note that the initialization function init_rmap_pkt_to_file() must be
 *	executed before
 * @note the configuration of the ICU_ADDR, RDCU_ADDR, MTU settings are in the
 *	.rdcu_pkt_mode_cfg file
 *
 * @param cfg	compressor configuration contains all parameters required for
 *		compression
 *
 * @returns 0 on success, error otherwise
 */

int gen_write_rdcu_pkts(const struct cmp_cfg *cfg)
{
	struct stat st = { 0 };

	if (!cfg)
		return -1;

	/* creating TC_DIR directory if that directory does not exist */
	if (stat(TC_DIR, &st) == -1) {
		int err;
		#if defined(_WIN32) || defined(_WIN64)
			err = mkdir(TC_DIR);
		#else
			err = mkdir(TC_DIR, 0700);
		#endif
		if (err)
			return -1;
	}

	set_tc_folder_dir(TC_DIR "/compress_data");
	if (rdcu_compress_data(cfg))
		return -1;

	return 0;
}


/**
 * @brief generate the rmap packets to read the result of an RDCU compression
 * @note note that the initialization function init_rmap_pkt_to_file() must be
 *	executed before
 * @note the configuration of the ICU_ADDR, RDCU_ADDR, MTU settings are in the
 *	.rdcu_pkt_mode_cfg file
 *
 * @param info	 compressor information contains information of an executed
 *		 compression
 *
 * @returns 0 on success, error otherwise
 */

int gen_read_rdcu_pkts(const struct cmp_info *info)
{
	int s;
	void *tmp_buf;
	struct stat st = { 0 };

	if (!info)
		return -1;

	/* creating TC_DIR directory if that directory does not exist */
	if (stat(TC_DIR, &st) == -1) {
		int err;
		#if defined(_WIN32) || defined(_WIN64)
			err = mkdir(TC_DIR);
		#else
			err = mkdir(TC_DIR, 0700);
		#endif
		if (err)
			return -1;
	}

	set_tc_folder_dir(TC_DIR "/read_status");
	if (rdcu_read_cmp_status(NULL))
		return -1;

	set_tc_folder_dir(TC_DIR "/read_info");
	if (rdcu_read_cmp_info(NULL))
		return -1;

	set_tc_folder_dir(TC_DIR "/read_cmp_data");
	s = rdcu_read_cmp_bitstream(info, NULL);
	if (s < 0)
		return -1;
	tmp_buf = malloc((size_t)s);
	if (!tmp_buf)
		return -1;
	s = rdcu_read_cmp_bitstream(info, tmp_buf);
	free(tmp_buf);
	if (s < 0)
		return -1;

	if (model_mode_is_used(info->cmp_mode_used)) {
		set_tc_folder_dir(TC_DIR "/read_upmodel");
		s = rdcu_read_model(info, NULL);
		if (s < 0)
			return -1;
		tmp_buf = malloc((size_t)s);
		if (!tmp_buf)
			return -1;
		s = rdcu_read_model(info, tmp_buf);
		free(tmp_buf);
		if (s < 0)
			return -1;
	}
	return 0;
}


/**
 * @brief generate the rmap packets to set up an RDCU compression, read the
 *	bitstream and the updated model in parallel to write the data to compressed
 *	and the model and start the compression
 * @note the compressed data are read from cfg->rdcu_buffer_adr with the length
 *	of last_cmp_size
 * @note note that the initialization function init_rmap_pkt_to_file() must be
 *	executed before
 * @note the configuration of the ICU_ADDR, RDCU_ADDR, MTU settings are in the
 *	.rdcu_pkt_mode_cfg file
 *
 * @param cfg		compressor configuration contains all parameters required for
 *			compression
 * @param last_info	compression information from the last executed
 *			compression
 *
 * @returns 0 on success, error otherwise
 */

int gen_rdcu_parallel_pkts(const struct cmp_cfg *cfg,
			   const struct cmp_info *last_info)
{
	struct stat st = { 0 };

	if (!cfg)
		return -1;

	/* creating TC_DIR directory if that directory does not exist */
	if (stat(TC_DIR, &st) == -1) {
		int err;
		#if defined(_WIN32) || defined(_WIN64)
			err = mkdir(TC_DIR);
		#else
			err = mkdir(TC_DIR, 0700);
		#endif
		if (err)
			return -1;
	}

	set_tc_folder_dir(TC_DIR "/compress_data_parallel");
	if (rdcu_compress_data_parallel(cfg, last_info))
		return -1;

	return 0;
}
