#include <stdio.h>

#include "test_common.h"

int main()
{
	cmp_rand_seed(1);

	for (int i = 0; i < 60; ++i) {
		uint32_t x = cmp_rand32();
		printf("%x\n", x);
	}

	return 0;
}
