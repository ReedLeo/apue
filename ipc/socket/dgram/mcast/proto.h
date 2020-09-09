#ifndef __PROTO_H__
#define __PROTO_H__

#define MGROUP		"224.2.2.1"
#define RECVPORT 	"1989"
#define NAMEMAX 	(512 - 8 - 8)

struct msg_st
{
	uint32_t math;
	uint32_t chinese;
	uint8_t name[1];
} __attribute__((packed));

#endif
