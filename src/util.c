#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <util.h>

void
print_hex(uint8_t *d, size_t len, const char *name)
{
#if 1
	size_t i;
	printf("  |--- dump (%s, %lu) ----------------\n  |", name, len);
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

