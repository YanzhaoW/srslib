#include <stdio.h>
#include <fec.h>

int
main(int argc, char *argv[])
{
	struct Fec *fec;
	(void)argc;
	(void)argv;
	printf("srscli\n");

	fec = fec_new();
	fec_open(fec, FEC_DEFAULT_IP, FEC_DEFAULT_FEC_PORT);
	fec_read_system_registers(fec);
	fec_read_link_status(fec);
	fec_close(fec);
	fec_destroy(fec);

	return 0;
}
