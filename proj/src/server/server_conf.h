#ifndef SERV_CONF_H__
#define SERV_CONF_H__

#define DEFAULT_MEDIA_DIR   "/var/media/"
#define DEFAULT_IFNAME      "eth0"

enum
{
    RUN_BACKGROUND = 1,
    RUN_FOREGROUND
};

struct serv_conf_st
{
    char* mgroup;
    char* rcvport;
    char* media_dir;
    char* ifname;
    char  run_mode;
};

extern struct serv_conf_st g_svconf;

#endif