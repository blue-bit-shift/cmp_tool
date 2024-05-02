/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE.BSD-3.Zstandard file in the 3rdparty_licenses directory) and the GPLv2
 * (found in the LICENSE.GPL-2 file in the 3rdparty_licenses directory).
 * You may select, at your option, one of the above-listed licenses.
 */

#include "fuzz_helpers.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void* FUZZ_malloc(size_t size)
{
    if (size > 0) {
        void* const mem = malloc(size);
        FUZZ_ASSERT(mem);
        return mem;
    }
    return NULL;
}

void* FUZZ_malloc_rand(size_t size, FUZZ_dataProducer_t *producer)
{
    if (size > 0) {
        void* const mem = malloc(size);
        FUZZ_ASSERT(mem);
        return mem;
    } else {
        uintptr_t ptr = 0;
        /* Add +- 1M 50% of the time */
        if (FUZZ_dataProducer_uint32Range(producer, 0, 1))
            FUZZ_dataProducer_int32Range(producer, -1000000, 1000000);
        return (void*)ptr;
    }

}

int FUZZ_memcmp(void const* lhs, void const* rhs, int32_t size)
{
    if (size <= 0) {
        return 0;
    }
    return memcmp(lhs, rhs, (size_t)size);
}
