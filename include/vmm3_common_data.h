#pragma once

struct VMM3CommonData
{
	uint32_t data_id;
	uint8_t fec_id;
	uint32_t last_frame_counter;
	uint8_t fc_is_initialised;
	uint32_t frame_counter;
	uint32_t udp_timestamp;
	uint32_t offset_overflow;
};
