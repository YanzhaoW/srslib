#include <stdio.h>
#include <fec.h>
#include <udp_socket.h>

int
main(int argc, char *argv[])
{
	struct Fec *fec;
	(void)argc;
	(void)argv;
	printf("srscli\n");

	fec = fec_new();
	fec_configure(fec);
	fec_open(fec, FEC_DEFAULT_IP, FEC_DEFAULT_FEC_PORT);

	fec_write_reset(fec);
	fec_read_system_registers(fec);
	fec_read_link_status(fec);
	fec_write_trigger_acq_constants(fec);

#if 0
	/* this can disturb comms */
	fec_write_acq_on(fec);
	fec_read_link_status(fec);
	fec_write_acq_off(fec);
	fec_read_link_status(fec);
#endif

	fec_write_set_mask(fec);

	{
		uint32_t fw;
		fw = fec_do_read_hybrid_firmware(fec);
		printf("hybrid firmware = 0x%08x\n", fw);
	}
	{
		uint32_t pos;
		pos = fec_do_read_geo_pos(fec);
		printf("hybrid geo pos = 0x%08x\n", pos);
	}
	{
		uint32_t id[4];
		fec->hybrid_index = 0;
		fec_do_read_id_chip(fec, id);
		printf("hybrid id = 0x%08x:%08x:%08x:%08x\n",
		    id[0], id[1], id[2], id[3]);
	}

	/*
	 * fec->config.debug = 1;
	 * fec->socket->config.debug = 1;
	 */
	fec_write_configure_hybrid(fec);

	fec_do_read_adc(fec, 2);

	/*
	 * does not work with all FEC firmwares.
	 * not with CLOCK_SOURCE > 1
	 */
	/* fec_do_powercycle_hybrids(fec); */
	fec_close(fec);
	fec_destroy(fec);

	return 0;
}
