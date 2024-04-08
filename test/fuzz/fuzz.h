/**
 * Fuzz target interface.
 */

#ifndef FUZZ_H
#define FUZZ_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int LLVMFuzzerTestOneInput(const uint8_t *src, size_t size);

#ifdef __cplusplus
}
#endif

#endif
