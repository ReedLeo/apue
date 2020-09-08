#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#define MAXLEN 32

int main(int argc, char** argv)
{
	struct addrinfo hint = {0};
	struct addrinfo* pres = NULL;
	struct addrinfo* pcur = NULL;
	int s = 0;
	char buf[MAXLEN] = {0};

	if (argc < 2)
	{
		fprintf(stderr, "Too few arguments.\n");
		exit(1);
	}
	
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;	
	s = getaddrinfo(argv[1], NULL, &hint, &pres);
	if (s != 0)
	{
		fprintf(stderr, "getaddrinfo() failed because of \"%s\"\n", gai_strerror(s));
		exit(1);
	}

	printf("\"%s\"\'s ip:\n", argv[1]);
	pcur = pres;
	for (pcur = pres; pcur; pcur = pcur->ai_next)
	{
		if ( s = getnameinfo(pcur->ai_addr, pcur->ai_addrlen, buf, MAXLEN, NULL, 0, NI_NUMERICHOST) != 0)
		{
			fprintf(stderr, "Unknown addr: \"%s\"\n", gai_strerror(s));
			exit(1);
		}
		printf("%s\n", buf);
	}

	freeaddrinfo(pres);
	return 0;
}
