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

#ifndef FUZZ_HELPERS_H
#define FUZZ_HELPERS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


#define FUZZ_QUOTE_IMPL(str) #str
#define FUZZ_QUOTE(str) FUZZ_QUOTE_IMPL(str)

/**
 * Asserts for fuzzing that are always enabled.
 */
#define FUZZ_ASSERT_MSG(cond, msg)                                             \
  ((cond) ? (void)0                                                            \
          : (fprintf(stderr, "%s: %u: Assertion: `%s' failed. %s\n", __FILE__, \
                     __LINE__, FUZZ_QUOTE(cond), (msg)),                       \
             abort()))
#define FUZZ_ASSERT(cond) FUZZ_ASSERT_MSG((cond), "");

void* FUZZ_malloc(size_t size);

#ifdef __cplusplus
}
#endif

#endif
