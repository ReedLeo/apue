#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	char* str = NULL;
	size_t file_size = 0;
	struct stat stat_res = {0};	
	int fd = -1;
	long long count = 0;
	
	if (argc < 2)
	{
		fprintf(stderr, "Too few arguments\n");
		exit(EXIT_FAILURE);
	}
	
	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("open()");
		exit(EXIT_FAILURE);
	}

	if (fstat(fd, &stat_res) < 0)
	{
		perror("fstat()");
		exit(EXIT_FAILURE);
	}

	str = mmap(NULL, stat_res.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (str == NULL)
	{
		perror("mmap()");
		exit(EXIT_FAILURE);
	}
	
	for (off_t i = 0; i < stat_res.st_size; ++i)
	{
		if (str[i] == 'a')
			count++;
	}
	
	printf("%s contains %lld \'a\'\n", argv[1], count);
	munmap(str, stat_res.st_size);
	close(fd);
	return 0;
}
