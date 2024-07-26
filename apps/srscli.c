#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <srs/c_impl/fec.h>
#include <srs/c_impl/util.h>
struct Config
{
	int do_start_acq;
	int do_stop_acq;
	int do_help;
	int do_test_mode;
	int verbosity;
};

struct Config *parse_args(int , char **);
void usage(const char *);
void do_start_acq(struct Fec *);
void do_stop_acq(struct Fec *);
void do_test_mode(struct Fec *);
void fec_custom_config(struct Fec *);

void
fec_custom_config(struct Fec *fec)
{
	fec->hybrid[0].vmm[0].config.channel[0].st = 0;
}

struct Config *
parse_args(int argc, char **argv)
{
	struct Config *c = (struct Config *)calloc(sizeof(struct Config), 1);
	int idx = 0;
	char *arg = NULL;

	if (argc == 1) {
		c->do_test_mode = 1;
		goto parse_ok;
	}

	ARG_INIT;
	while (argc > 0) {
		if (TEST_ARG("--acq-on", "-o")) {
			c->do_start_acq = 1;
		}
		else if (TEST_ARG("--acq-off", "-f")) {
			c->do_stop_acq = 1;
		}
		else if (TEST_ARG("--help", "-h")) {
			c->do_help = 1;
		}
		else if (TEST_ARG("--verbose", "-v")) {
			c->verbosity += 1;
		}
		else {
			printf("Unknown option '%s'\n\n", arg);
			usage(argv[0]);
			exit(-1);
		}
		ARG_ADV;
	}

parse_ok:

	return c;
}

void
usage(const char *name)
{
	printf("srscli - A minimal wrapper around libsrs\n");
	printf("\n");
	printf("  Usage: %s [options]\n", name);
	printf("\n\n");
	printf("Options:\n");
	printf("\t-o --acq-on    Start data acquisition.\n");
	printf("\t-f --acq-off   Stop data acquisition.\n");
	printf("\t-h --help      Show this help.\n");
	printf("\t-v --verbose   Increase verbosity level.\n");
	printf("\n");
}

void
do_test_mode(struct Fec *fec)
{
	uint8_t hybrid;

	printf("### TEST MODE BEGIN ###\n");
	printf("Resetting the FEC.\n");
	fec_write_reset(fec);
	usleep(300000);

	fec_read_system_registers(fec);
	fec_read_link_status(fec);

	fec_write_set_mask(fec);
	fec_write_trigger_acq_constants(fec);

	for (hybrid = 0; hybrid < fec->n_hybrids; ++hybrid) {
		fec->hybrid_index = hybrid;
		{
			uint32_t fw;
			fw = fec_do_read_hybrid_firmware(fec);
			printf("hybrid[%d] firmware = 0x%08x\n", hybrid, fw);
		}
		{
			uint32_t pos;
			pos = fec_do_read_geo_pos(fec);
			printf("hybrid[%d] geo pos = 0x%08x\n", hybrid, pos);
		}
		{
			uint32_t id[4];
			fec_do_read_id_chip(fec, id);
			printf("hybrid[%d] id = 0x%08x:%08x:%08x:%08x\n",
			    hybrid, id[0], id[1], id[2], id[3]);
		}
		fec_write_configure_hybrid(fec);
	}


	for (hybrid = 0; hybrid < 4; ++hybrid) {
		fec_debug(fec, 1);
		fec_do_send_config(fec, hybrid, 0);
		fec_do_send_config(fec, hybrid, 1);
		fec_debug(fec, 0);
		fec_read_temperature(fec, hybrid, 0);
		fec_read_temperature(fec, hybrid, 1);
	}

	fec_debug(fec, 1);

	fec_write_acq_on(fec);
	fec_read_link_status(fec);

	usleep(300000);

	fec_write_acq_off(fec);
	fec_read_link_status(fec);
	printf("### TEST MODE END ###\n");
}

void
do_start_acq(struct Fec *fec)
{
	fec_write_acq_on(fec);
}

void
do_stop_acq(struct Fec *fec)
{
	fec_write_acq_off(fec);
}

int
main(int argc, char *argv[])
{
	struct Fec *fec;
	struct Config *c;

	printf("srscli\n");

	c = parse_args(argc, argv);

	fec = fec_new();
	fec_debug(fec, c->verbosity);
	fec_configure(fec);

	fec_add_vmm3_hybrid(fec, 0);
	fec_add_vmm3_hybrid(fec, 1);
	// fec_add_vmm3_hybrid(fec, 2);
	// fec_add_vmm3_hybrid(fec, 3);

	fec_custom_config(fec);

	fec_open(fec);

	if (c->do_help) {
		usage(argv[0]);
	}
	if (c->do_test_mode) {
		do_test_mode(fec);
	} else if (c->do_start_acq) {
		do_start_acq(fec);
	} else if (c->do_stop_acq) {
		do_stop_acq(fec);
	} else {
		printf("No action requested. Exiting.\n");
	}

	/*
	 * does not work with all FEC firmwares.
	 * not with CLOCK_SOURCE > 1
	 */
	/* fec_do_powercycle_hybrids(fec); */
	fec_close(fec);
	fec_destroy(fec);

	free(c);

	return 0;
}
