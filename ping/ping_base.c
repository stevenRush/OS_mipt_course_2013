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
 
int fd_fifo;
sem_t* synch_sem1;
sem_t* synch_sem2;
 
const char *sem1_name = "/my_sem1";
const char *sem2_name = "/my_sem2";
const char *fifo_name = "/tmp/my_pipe1";

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
	char buf[6];
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
		write(fd_fifo, ping, strlen(ping));
		sem_post(postSem);
	}
 
	for(i = 0; i < 10; ++i) 
	{
		sem_wait(waitSem);
		read(fd_fifo, buf, 6);
		printf("Got %s Writing %s\n", buf, msg);
		write(fd_fifo, msg, strlen(msg));
		sem_post(postSem);
	}
	closeResources();
	if(isMainProc)
	{
		unlinkResources();
	}
}
 
void closeResources()
{
	close(fd_fifo);
	sem_close(synch_sem2);
	sem_close(synch_sem1);
}
 
void unlinkResources()
{
	sem_unlink(sem2_name);
	sem_unlink(sem1_name);
	unlink(fifo_name);
}
 
void initResources()
{
	synch_sem1 = sem_open(sem1_name, O_RDWR | O_CREAT, 0666, 0);
	synch_sem2 = sem_open(sem2_name, O_RDWR | O_CREAT, 0666, 0);
	sem_post(synch_sem1);
	unlink(fifo_name);
	if (mkfifo(fifo_name, 0666 | O_CREAT) == -1)
		printf("Cannot create fifo\n %s\n", strerror(errno));
	fd_fifo = open(fifo_name, O_RDWR);
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
	printf("opening fifo...\n");
	fd_fifo = open(fifo_name, O_RDWR);
	printf("opened\n");
}
