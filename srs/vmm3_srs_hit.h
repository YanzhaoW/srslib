#pragma once

struct VMM3SRSHit
{
	uint32_t d32;
	uint16_t d16;
} __attribute__((__packed__));

#define VMM3SRSHitSize (sizeof(struct VMM3SRSHit))
