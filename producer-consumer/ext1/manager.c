#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>

#include "prod.h"

pid_t *entries;
int entries_count;
int storage_size;

void UnlinkResources();
void InitResources();
void KillEntries();
void CloseResources();

void sighandler(int signum)
{
	if (signum == SIGUSR1)
	{
		printf("Killing all consumers and producers...\n");
		KillEntries();
		CloseResources();
		UnlinkResources();
		exit(-1);
	}
}

void KillEntries()
{
	int i;
	for(i = 0; i < entries_count; ++i)
		kill(entries[i], SIGKILL);
}

void InitResources()
{
	UnlinkResources();
	sem_fillcount = sem_open(sem_fillcount_name, O_RDWR | O_CREAT, 0666, 0);		
	sem_emptycount = sem_open(sem_emptycount_name, O_RDWR | O_CREAT, 0666, storage_size);
	sem_sync = sem_open(sem_sync_name, O_RDWR | O_CREAT, 0666, 0);
	
	shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0666);
	ftruncate(shm_fd, 1024);
	
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = sizeof(pid_t);
	attr.mq_curmsgs = 0;
	
	mq = mq_open(queue_name, O_CREAT | O_RDWR, 0666, &attr);
	if (mq == -1)
		printf("%s\n", strerror(errno));
}

void UnlinkResources()
{
	shm_unlink(shm_name);
	sem_unlink(sem_fillcount_name);
	sem_unlink(sem_emptycount_name);
	sem_unlink(sem_sync_name);
	mq_unlink(queue_name);
}

void CloseResources()
{
	sem_close(sem_sync);
	sem_close(sem_emptycount);
	sem_close(sem_fillcount);
	mq_close(mq);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Usage manager entries_count storage_size\nentries_count - number of producers and consumers, storage_size - size of storage\n");
		return 0;
	}
	__sighandler_t prev;
	char buf[10];
	int i;
	pid_t pid;
	
	prev = signal(SIGUSR1, &sighandler);
	entries_count = atoi(argv[1]);
	entries = (pid_t*) malloc(sizeof(pid_t) * entries_count);
	storage_size = atoi(argv[2]);
	
	InitResources();
	for(i = 0; i < entries_count; ++i)
	{
		mq_receive(mq, (char*)&pid, sizeof(pid_t), NULL);
		printf("Got pid = %d\n", pid);
		entries[i] = pid;
	}
	int *ptr = mmap(0, 512, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
	*ptr = 0;
	sem_post(sem_sync);
	while(1)
		;
	munmap(ptr, 512);
	signal(SIGUSR1, prev);
	return 0;
}
