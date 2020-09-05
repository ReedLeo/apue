#ifndef __PROTO_H__
#define __PROTO_H__

#define KEYPATH 	"/etc/services"
#define KEYPROJ		'g'
#define NAMESIZE 	128

struct msg_st
{
	long mtype;
	char name[NAMESIZE];
	int math;
	int chinese;
};
#endif
