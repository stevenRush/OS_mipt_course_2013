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

void OpenResources();
void CloseResources();

int main(int argc, char *argv[])
{
	int cons = argv[1][0] == 'c';	// producers executes as ./prod p
									// consumers as ./prod c
	int product;
	pid_t pid;
	srand(time(NULL));
	OpenResources();
	pid = getpid();
	mq_send(mq, (char*)&pid, sizeof(pid_t), 0); 
	int *ptr = mmap(0, 512, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
	sem_wait(sem_sync);
	if (cons)
	{
		while(1)
		{
			sem_wait(sem_fillcount);
			printf("I got product %d\n", ptr[*ptr]);
			--*ptr;
			sleep(1);
			sem_post(sem_emptycount);
		}
	}
	else
	{
		while(1)
		{
			sem_wait(sem_emptycount);
			product = rand() % 10000;
			printf("I put product %d to room %d\n", product, *ptr+1);
			ptr[*ptr+1] = product;
			++*ptr;
			sleep(1);
			sem_post(sem_fillcount);
		}
	}
	munmap(ptr, 512);
	CloseResources();
	return 0;
}

void OpenResources()
{
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
	shm_fd = shm_open(shm_name, O_RDWR, 0666);
	mq = mq_open(queue_name, O_RDWR);
}

void CloseResources()
{
	close(shm_fd);
	sem_close(sem_fillcount);
	sem_close(sem_emptycount);
	sem_close(sem_sync);
	mq_close(mq);
}
