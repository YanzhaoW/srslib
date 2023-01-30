#include <stdio.h>
#include <fec.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	struct Fec *fec;
	uint8_t hybrid;
	(void)argc;
	(void)argv;
	printf("srscli\n");

	fec = fec_new();

	fec_add_vmm3_hybrid(fec, 0);
	fec_add_vmm3_hybrid(fec, 1);
	fec_add_vmm3_hybrid(fec, 2);
	fec_add_vmm3_hybrid(fec, 3);

	fec_configure(fec);
	fec_open(fec, FEC_DEFAULT_IP, FEC_DEFAULT_FEC_PORT);

	fec_debug(fec, 1);

	printf("Resetting the FEC.\n");
	fec_write_reset(fec);
	sleep(1);

	fec_read_system_registers(fec);
	fec_read_link_status(fec);

	fec_write_set_mask(fec);
	fec_write_trigger_acq_constants(fec);

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

	for (hybrid = 0; hybrid < 4; ++hybrid) {
		uint32_t id[4];
		fec->hybrid_index = hybrid;
		fec_do_read_id_chip(fec, id);
		printf("hybrid id [%d] = 0x%08x:%08x:%08x:%08x\n", hybrid,
		    id[0], id[1], id[2], id[3]);
	}

	fec_write_configure_hybrid(fec);

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

	sleep(1);

	fec_write_acq_off(fec);
	fec_read_link_status(fec);

	/*
	 * does not work with all FEC firmwares.
	 * not with CLOCK_SOURCE > 1
	 */
	/* fec_do_powercycle_hybrids(fec); */
	fec_close(fec);
	fec_destroy(fec);

	return 0;
}
