#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
 
int shm_fd;
sem_t* synch_sem1;
sem_t* synch_sem2;
 
const char *sem1_name = "/my_sem1";
const char *sem2_name = "/my_sem2";
const char *shm_name = "/my_shm";

const char *ping = "ping";
const char *pong = "pong";
 
void initResources();
void openResources();
void closeResources();
void unlinkResources();
 
int main(int argc, char** argv)
{
	int i;
	bool isMainProc = (argc >= 2);
	if(isMainProc) 
	{
		printf("main proc\n");
		initResources();
	} 
	else
	{
		printf("arbitr. proc\n");
		openResources();
	}
	char *ptr = mmap(0, 512, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
 
	sem_t* waitSem;
	sem_t* postSem;
	char *msg;
	if(isMainProc) 
	{
		waitSem = synch_sem2;
		postSem = synch_sem1;
		msg = ping;
	} 
	else 
	{
		waitSem = synch_sem1;
		postSem = synch_sem2;
		msg = pong;
	}
	
	if(isMainProc) 
	{
		strcpy(ptr, ping);
		sem_post(postSem);
	}
 
	for(i = 0; i < 10; ++i) 
	{
		sem_wait(waitSem);
		printf("Got %s Writing %s\n", ptr, msg);
		strcpy(ptr, msg);
		sem_post(postSem);
	}
	munmap(ptr, 512);
	closeResources();
	if(isMainProc)
	{
		unlinkResources();
	}
}
 
void closeResources()
{
	close(shm_fd);
	sem_close(synch_sem2);
	sem_close(synch_sem1);
}
 
void unlinkResources()
{
	shm_unlink(shm_name);
	sem_unlink(sem2_name);
	sem_unlink(sem1_name);
}
 
void initResources()
{
	synch_sem1 = sem_open(sem1_name, O_RDWR | O_CREAT, 0666, 0);
	synch_sem2 = sem_open(sem2_name, O_RDWR | O_CREAT, 0666, 0);
	shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0666);
	sem_post(synch_sem1);
	ftruncate(shm_fd, 1024);
}
 
void openResources()
{
	do {
		printf("opening sem\n");
		synch_sem1 = sem_open(sem1_name, O_RDWR);
	} while(synch_sem1 == SEM_FAILED);
 
	do {
		printf("opening sem\n");
		synch_sem2 = sem_open(sem2_name, O_RDWR);
	} while(synch_sem2 == SEM_FAILED);
 
	printf("waiting...\n");
	sem_wait(synch_sem1);
	printf("opening mem\n");
	shm_fd = shm_open(shm_name, O_RDWR, 0666);
	printf("opened\n");
}
