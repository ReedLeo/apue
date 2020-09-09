#ifndef __PROTO_H__
#define __PROTO_H__

#define RECVPORT "1989"
#define NAMELEN 11

struct msg_st
{
	uint8_t name[NAMELEN];
	uint32_t math;
	uint32_t chinese;
} __attribute__((packed));

#endif
