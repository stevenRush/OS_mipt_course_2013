#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <stdlib.h>
#include <time.h>

#include "prod.h"

/*
 Index of the last stored product is stored as 
 first number in shared memory: 
 *ptr 	- index; 
 ptr[1] - the first product
 */

#define MAX(a,b) (a > b ? a : b)
#define SLEEP 2

void OpenResources();
void CloseResources();

int main(int argc, char *argv[])
{
	int cons = argv[1][0] == 'c';	// producers executes as ./prod p
									// consumers as ./prod c
	int product;
	pid_t pid;
	srand(getpid());
	OpenResources();
	pid = getpid();
	mq_send(mq, (char*)&pid, sizeof(pid_t), 0); 
	volatile int *ptr = mmap(0, 512, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
	int a,b;
	if (cons)	// consumer
	{
		while(1)
		{
			sem_wait(sem_sync);
			sem_getvalue(sem_fillcount, &a);
			if (a == 0)
			{
				sem_post(sem_sync);
				continue;
			}
			sem_wait(sem_fillcount);
			printf("I got product %d from room %d\n", ptr[*ptr], *ptr);
			--*ptr;
			sem_post(sem_emptycount);
			sem_post(sem_sync);
			sleep(SLEEP);
		}
	}
	else	// producer
	{
		while(1)
		{
			sem_wait(sem_sync);
			sem_getvalue(sem_emptycount, &a);
			if (a == 0)
			{
				sem_post(sem_sync);
				continue;
			}
			sem_wait(sem_emptycount);
			int product = rand() % 1000;
			printf("I put product %d to room %d\n", product, *ptr+1);
			ptr[++*ptr] = product;
			sem_post(sem_fillcount);
			sem_post(sem_sync);
			sleep(SLEEP);
		}
	}
	munmap(ptr, 512);
	CloseResources();
	return 0;
}

void OpenResources()
{
	printf("start opening...\n");
	do
	{
		sem_fillcount = sem_open(sem_fillcount_name, O_RDWR);
	} while(sem_fillcount == SEM_FAILED);
	
	do
	{
		sem_emptycount = sem_open(sem_emptycount_name, O_RDWR);
	} while(sem_emptycount == SEM_FAILED);
	do
	{
		sem_sync = sem_open(sem_sync_name, O_RDWR);
	} while (sem_sync == SEM_FAILED);
	printf("sems opened\n");
	shm_fd = shm_open(shm_name, O_RDWR, 0666);
	mq = mq_open(queue_name, O_RDWR);
	if (mq == -1)
		printf("%s\n", strerror(errno));
}

void CloseResources()
{
	close(shm_fd);
	sem_close(sem_fillcount);
	sem_close(sem_emptycount);
	sem_close(sem_sync);
	mq_close(mq);
}
