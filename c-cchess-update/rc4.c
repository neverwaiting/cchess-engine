#include "rc4.h"

void rc4_swap(struct rc4* rc4, int i, int j)
{
	int temp = rc4->state[i];
	rc4->state[i] = rc4->state[j];
	rc4->state[j] = temp;
}

void rc4_init(struct rc4* rc4)
{
	rc4->x = 0;
	rc4->y = 0;
	int i = 0;
	int j = 0;
	for (i = 0; i < 256; ++i)
	{
		rc4->state[i] = i;
	}
	for (i = 0; i < 256; ++i)
	{
		j = (j + rc4->state[i]) & 0xff;
		rc4_swap(rc4, i, j);
	}
}

uint8_t rc4_next_byte(struct rc4* rc4)
{
	rc4->x = (rc4->x + 1) & 0xff;
	rc4->y = (rc4->y + rc4->state[rc4->x]) & 0xff;
	rc4_swap(rc4, rc4->x, rc4->y);
	return rc4->state[(rc4->state[rc4->x] + rc4->state[rc4->y]) & 0xff];
}

uint32_t rc4_next_long(struct rc4* rc4)
{
	return rc4_next_byte(rc4) + (rc4_next_byte(rc4) << 8) + 
				 (rc4_next_byte(rc4) << 16) + (rc4_next_byte(rc4) << 24);
}
