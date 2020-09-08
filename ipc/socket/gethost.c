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
	
	s = getaddrinfo(argv[1], "80", &hint, &pres);
	if (s != 0)
	{
		fprintf(stderr, "getaddrinfo() failed because of \"%s\"\n", gai_strerror(s));
		exit(1);
	}

	printf("\"%s\"\'s ip:\n", argv[1]);
	pcur = pres;
	for (pcur = pres; pcur; pcur = pcur->ai_next)
	{
		if (inet_ntop(pcur->ai_family, &pcur->ai_addr, buf, MAXLEN) == NULL)
		{
			fprintf(stderr, "Unknown addr: \"%s\"\n", strerror(errno));
			exit(1);
		}
		printf("%s\n", buf);
	}

	freeaddrinfo(pres);
	return 0;
}
