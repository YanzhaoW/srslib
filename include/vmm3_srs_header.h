#pragma once

struct VMM3SRSHeader
{
	uint32_t frame_counter;
	uint32_t data_id;
	uint32_t udp_timestamp;
	uint32_t offset_overflow;
};

#define VMM3SRSHeaderSize (sizeof(struct VMM3SRSHeader))
#define VMM3SRSMinimumPayload (VMM3SRSHeaderSize + VMM3SRSHitSize)
#define JUMBO_FRAME_SIZE 8950
#define JUMBO_FRAME_SIZE_SRS 9000
