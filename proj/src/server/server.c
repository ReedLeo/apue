#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <proto.h>
#include "server_conf.h"
#include "medialib.h"
#include "thr_list.h"
#include "thr_channel.h"

struct serv_conf_st g_svconf = {
    .rcvport    = DEFAULT_PORT,
    .mgroup     = DEFAULT_MGROUP,
    .media_dir  = DEFAULT_MEDIA_DIR,
    .ifname     = DEFAULT_IFNAME,
    .run_mode   = RUN_BACKGROUND
};

static void print_help(char** argv)
{
    fprintf(stderr, "Usage: %s\n", argv[0]);
    fprintf(stderr, "-M:  specify multi-cast group.\n");
    fprintf(stderr, "-P:  specify recvieve port number.");
    fprintf(stderr, "-F:  running foregound.");
    fprintf(stderr, "-D:  specify the medialib path.");
    fprintf(stderr, "-I:  specify networking interface name.");
    fprintf(stderr, "-H:  help");
    fflush(stderr);
}

/**
 * -M:  specify multi-cast group.
 * -P:  specify recvieve port number.
 * -F:  running foregound.
 * -D:  specify the medialib path.
 * -I:  specify networking interface name.
 * -H:  help
*/
static void parse_cmdline(int argc, char** argv)
{
    static struct option argarr[] = {
		{"port",        required_argument,  NULL, 'P'},
		{"mgroup",      required_argument,  NULL, 'M'},
        {"media_dir",   required_argument,  NULL, 'D'},
        {"ifname",      required_argument,  NULL, 'I'},
		{"foreground",  no_argument,        NULL, 'F'},
		{"help",        no_argument,        NULL, 'H'},
		{NULL,          0,                  NULL, 0}
	};
    
    int c;

    while (1)
	{
		int index = 0;
		c = getopt_long(argc, argv, "P:M:I:D:FH", argarr, &index);
		if (c < 0)
			break;
		switch (c)
		{
		case 'P':
			g_svconf.rcvport = optarg; 	
			break;

		case 'M':
			g_svconf.mgroup = optarg;
			break;

		case 'F':
			g_svconf.run_mode = RUN_FOREGROUND;
			break;

        case 'I':
            g_svconf.ifname = optarg;
            break;
        
        case 'D':
            g_svconf.media_dir = optarg;
            break;

		case 'H':
			print_help(argv);
			exit(0);
			break;

		default:
            syslog(LOG_ERR, "Parse command line failed. Unknown option: %c", c);
			abort();
			break;
		}
	}
}

/* Bit-mask values for 'flags' argument of becomeDaemon() */

#define BD_NO_CHDIR           01    /* Don't chdir("/") */
#define BD_NO_CLOSE_FILES     02    /* Don't close all open files */
#define BD_NO_REOPEN_STD_FDS  04    /* Don't reopen stdin, stdout, and
                                       stderr to /dev/null */
#define BD_NO_UMASK0         010    /* Don't do a umask(0) */

#define BD_MAX_CLOSE  8192          /* Maximum file descriptors to close if
                                       sysconf(_SC_OPEN_MAX) is indeterminate */

int become_daemon(int flags)
{
    int maxfd, fd;

    switch (fork()) {                   /* Become background process */
    case -1: return -1;
    case 0:  break;                     /* Child falls through... */
    default: _exit(EXIT_SUCCESS);       /* while parent terminates */
    }

    if (setsid() == -1)                 /* Become leader of new session */
        return -1;

    switch (fork()) {                   /* Ensure we are not session leader */
    case -1: return -1;
    case 0:  break;
    default: _exit(EXIT_SUCCESS);
    }

    if (!(flags & BD_NO_UMASK0))
        umask(0);                       /* Clear file mode creation mask */

    if (!(flags & BD_NO_CHDIR))
        chdir("/");                     /* Change to root directory */

    if (!(flags & BD_NO_CLOSE_FILES)) { /* Close all open files */
        maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd == -1)                /* Limit is indeterminate... */
            maxfd = BD_MAX_CLOSE;       /* so take a guess */

        for (fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if (!(flags & BD_NO_REOPEN_STD_FDS)) {
        close(STDIN_FILENO);            /* Reopen standard fd's to /dev/null */

        fd = open("/dev/null", O_RDWR);

        if (fd != STDIN_FILENO)         /* 'fd' should be 0 */
            return -1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }

    return 0;
}

static void daemon_exit(int signum)
{
    closelog();
    exit(EXIT_SUCCESS);
}

static void init()
{
    struct sigaction sa = {0};
    sa.sa_handler = daemon_exit;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGQUIT);

    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

}

static int init_socket()
{
    int sfd = -1;
    struct ip_mreqn mreq = {0};

    sfd = socket(AF_INET, SOCK_DGRAM, 0);
    // bind() could be omitted by server.

    inet_pton(AF_INET, g_svconf.mgroup, &mreq.imr_multiaddr);
    // it's equivalent to inet_pton(AF_INET, "0.0.0.0", &mreq.imr_addresss);
    mreq.imr_address.s_addr = INADDR_ANY;
    mreq.imr_ifindex = if_nametoindex(g_svconf.ifname);

    if (setsockopt(sfd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) {
        syslog(LOG_ERR, "setsocketopt: join to multicast group failed. %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return sfd;
}

int main(int argc, char** argv)
{
    int sfd;

    init();

    openlog(argv[0], LOG_PID, LOG_USER);

    /* Command line analyse. */
    parse_cmdline(argc, argv);

    /* Run server as a daemon. */
    if (RUN_BACKGROUND == g_svconf.run_mode)
    {
        if (become_daemon(0) < 0)
        {   
            /* No panic: stdio may be closed */
            syslog(LOG_ERR, "Daemonize failed. %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    else if (RUN_FOREGROUND == g_svconf.run_mode) 
    { 
        /* do nonthing. */
        /* foreground for debug. */
    }
    else 
    {
        // fprintf(stderr, "Unknown running mode.\n");
        syslog(LOG_ERR, "Unknown running mode.");
        exit(EXIT_FAILURE);
    }
    
    /**
    * Initlaize socket
    */
   sfd = init_socket();

    /**
     * Get channel information.
    */
    struct mlib_listentry_st* p_list = NULL;
    int list_size = 0;
    int err = 0;
    err = mlib_getchnlist(&p_list, &list_size);
    if (err) 
    {
        // error handling
    }

   /**
    * Create threads of radio program list.
   */
    err = thr_list_create(p_list, list_size);
    if (err)
    {
        // error handling
    }

    /**
     * Create threads of channel.
    */
   int chn_created;
   for (chn_created = 0; chn_created < list_size; ++chn_created) 
   {
       err = thr_channel_create(p_list + chn_created);
       // error handling
   }

   syslog(LOG_DEBUG, "%d channel(s) has been created.", chn_created);

	while (1)
	{
		pause();
	}
    
    return 0;
}