#include <stdio.h>
#include <fec.h>

int read_loop(struct Fec *);
void handle_data(struct Fec *);

int
main(int argc, char *argv[])
{
	struct Fec *fec;
	(void)argc;
	(void)argv;
	printf("srsread\n");

	fec = fec_new();

	fec_configure(fec);
	fec_open(fec, FEC_DEFAULT_IP, fec->daq.port);

	while (1) {
		read_loop(fec);
	}

	fec_close(fec);
	fec_destroy(fec);
	return 0;
}

void
handle_data(struct Fec *fec)
{
	(void)fec;
	printf("handle_data.\n");
}

int
read_loop(struct Fec *fec)
{
	int rc;
	rc = fec_rw(fec, 0, NULL, handle_data);
	if (rc == FEC_RW_NO_DATA) {
		printf("No data from FEC. Acquisition on and signals coming?\n");
	} else {
		printf("Received data from FEC.\n");
	}
	return 0;
}
