/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE.BSD-3.Zstandard file in the 3rdparty_licenses directory) and the GPLv2
 * (found in the LICENSE.GPL-2 file in the 3rdparty_licenses directory).
 * You may select, at your option, one of the above-listed licenses.
 */

/*
 * Modifications made by
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date 2025
 *
 * - add FUZZ_buf_to_file
 * - add FUZZ_delete_file
 *
 * Modifications are also licensed under the same license for consistency
 */

/**
 * Helper functions for fuzzing.
 */

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "fuzz_helpers.h"
#include "fuzz_data_producer.h"


void *FUZZ_malloc(size_t size)
{
	if (size > 0) {
		void *const mem = malloc(size);

		FUZZ_ASSERT(mem);
		return mem;
	}
	return NULL;
}


int FUZZ_delete_file(const char *path_name)
{
	int const ret = unlink(path_name);

	FUZZ_ASSERT(ret != -1);

	free((void *)path_name);

	return ret;
}


char *FUZZ_buf_to_file(const uint8_t *buf, size_t size)
{
	int fd, ret_close;
	size_t pos = 0;

	char *path_name = strdup(FUZZ_TMP_DIR "/fuzz-XXXXXX");

	FUZZ_ASSERT(path_name != NULL);

	fd = mkstemp(path_name);
	FUZZ_ASSERT_MSG(fd != -1, path_name);

	while (pos < size) {
		ssize_t bytes_written = write(fd, &buf[pos], size - pos);

		if (bytes_written == -1 && errno == EINTR)
			continue;

		FUZZ_ASSERT(bytes_written != -1);

		pos += (size_t)bytes_written;
	}

	ret_close = close(fd);
	FUZZ_ASSERT(ret_close != -1);

	return path_name;
}
