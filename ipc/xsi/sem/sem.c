#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define NUM_PROC_MAX 20
#define FNAME "/tmp/out"

static int fd = -1;
static int semid = 0;
static FILE* fp;

void P()
{
	struct sembuf sop = {0};
	
	sop.sem_num = 0;
	sop.sem_op = -1;
	sop.sem_flg = SEM_UNDO;

	if (semop(semid, &sop, 1) < 0)
	{
		perror("semop()");
		exit(1);
	}
}

void V()
{
	struct sembuf sop = {0};
	
	sop.sem_num = 0;
	sop.sem_op = 1;
	sop.sem_flg = SEM_UNDO;

	if (semop(semid, &sop, 1) < 0)
	{
		perror("semop()");
		exit(1);
	}
}

static void* sub_proc()
{
	long cnt = 0;
	
	P();	

	fseek(fp, 0, SEEK_SET);
	fscanf(fp, "%ld", &cnt);
	++cnt;
	
	fseek(fp, 0, SEEK_SET);
	fprintf(fp, "%ld", cnt);
	fflush(fp);

err_exit:
	V();
	exit(0);
}

int main(int argc, char** argv)
{
	int ret = 0;
	pid_t pid;
	
	semid = semget(IPC_PRIVATE, 1, 0600);
	if (semid < 0)
	{
		perror("semget()");
		exit(1);
	}

	if (semctl(semid, 0, SETVAL, 1) < 0)
	{
		perror("semctl()");
		exit(1);
	}

	P();

	fp = fopen(FNAME, "w+");
	fd = fileno(fp);

	fprintf(fp, "1");
	fflush(fp);

	V();
	
	for (int i = 0; i < NUM_PROC_MAX; ++i)
	{
		pid = fork();
		if (pid < 0)
		{
			perror("fork()");
			exit(1);
		}
		else if (pid == 0)
		{
			sub_proc();
			exit(0);			
		}
	}

	for (int i = 0; i < NUM_PROC_MAX; ++i)
	{
		if (wait(NULL) < 0)
		{
			perror("wait()");
			exit(1);
		}
	}

	fclose(fp);

	if (semctl(semid, 0, IPC_RMID) < 0)
	{
		perror("semctl()");
		exit(1);
	}
	return 0;
}


