/**
 * @file   rdcu_pkt_to_file.h
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
 */

#ifndef _RDCU_PKT_TO_FILE_H_
#define _RDCU_PKT_TO_FILE_H_

#include <cmp_support.h>

/* directory where the tc files are stored, when --rdcu_pkt option is set */
#define TC_DIR "TC_FILES"

#define RDCU_DEST_KEY	0x0

#define MAX_TC_FOLDER_DIR_LEN 256

/* default values when no .rdcu_pkt_mode_cfg file is available */
#define DEF_ICU_ADDR 0xA7
#define DEF_RDCU_ADDR 0xFE
#define DEF_MTU 4224

int init_rmap_pkt_to_file(void);

void set_tc_folder_dir(const char *dir_name);

int gen_write_rdcu_pkts(const struct cmp_cfg *cfg);
int gen_read_rdcu_pkts(const struct cmp_info *info);
int gen_rdcu_parallel_pkts(const struct cmp_cfg *cfg,
			   const struct cmp_info *last_info);

#endif /* _RDCU_PKT_TO_FILE_H_ */
