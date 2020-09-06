#ifndef __PROTO_H__
#define __PROTO_H__

#define KEYPATH 	"/etc/services"
#define KEYPROJ		'a'

#define PATHMAX		1024
#define DATAMAX		1024

enum 
{
	MSG_PATH = 1,
	MSG_DATA,
	MSG_EOT
};

typedef strcut msg_path_st
{
	long mtype;				/* Must be MSG_PATH */
	char path[PATHMAX];
} msg_path_t;

typedef struct msg_data_st
{
	long mtype;				/* Must be MSG_DATA */
	char data[DATAMAX];
	int datalen;
} msg_data_t;

typedef struct msg_eot_st
{
	long mtype;				/* Must be MSG_EOT */
} msg_eot_t;

union msg_s2c_un
{
	long mtype;
	msg_data_t datamsg;
	msg_eot_t eotmsg;
};

#endif
