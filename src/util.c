#define _POSIX_C_SOURCE 199309L
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#include <util.h>

uint32_t *
util_fill_array32(size_t size, ...)
{
	size_t i = 0;
	uint32_t *array;
	va_list ap;

	/* restrict to reasonable size */
	assert(size < 1024);
	array = (uint32_t *)calloc(sizeof(uint32_t), size);
	assert(array != NULL);

	va_start(ap, size);
	while (i < size) {
		array[i++] = va_arg(ap, uint32_t);
	}
	va_end(ap);
	return array;
}

void
print_hex(uint8_t *d, size_t len, const char *name, const char *prefix)
{
#if 1
	size_t i;
	printf("  |%s dump (%s, %lu) ----------------\n  |", prefix, name, len);
	for (i = 0; i < len; ++i) {
		printf("%02x", d[i]);
		if ((i + 1) % 4 == 0) {
			printf(" ");
		}
		if ((i + 1) % 32 == 0 && i != len - 1) {
			printf("\n  |");
		}
	}
	printf("\n  |-----------------------------------------------\n");
#else
	(void)d;
	(void)len;
	(void)name;
#endif
}

double
time_double()
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (double)tp.tv_sec + ((double)tp.tv_nsec) * 1e-9;
}

uint32_t
gray2bin32(uint32_t val) {
    val = val ^ (val >> 16);
    val = val ^ (val >> 8);
    val = val ^ (val >> 4);
    val = val ^ (val >> 2);
    val = val ^ (val >> 1);
    return val;
}
