#pragma once
uint32_t *util_fill_array32(size_t, ...);
void print_hex(uint8_t *, size_t, const char *, const char *);
double time_double(void);
uint32_t gray2bin32(uint32_t);

/* for command line parsing */
#define TEST_ARG(name, brief) (strncmp(arg, name, strlen(name)) == 0 \
    || strncmp(arg, brief, strlen(brief)) == 0)
#define ARG_ADV do { idx++; argc--; arg=argv[idx]; } while(0)
#define ARG_INIT ARG_ADV
