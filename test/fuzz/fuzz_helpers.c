/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE.BSD-3.Zstandard file in the 3rdparty_licenses directory) and the GPLv2
 * (found in the LICENSE.GPL-2 file in the 3rdparty_licenses directory).
 * You may select, at your option, one of the above-listed licenses.
 */

/**
 * Helper functions for fuzzing.
 */

#include <stdlib.h>

#include "fuzz_helpers.h"


void *FUZZ_malloc(size_t size)
{
	if (size > 0) {
		void *const mem = malloc(size);

		FUZZ_ASSERT(mem);
		return mem;
	}
	return NULL;
}
