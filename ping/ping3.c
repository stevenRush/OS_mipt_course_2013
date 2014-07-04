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
#include <mqueue.h>

#define MAX_SIZE 6
 
mqd_t mq;
sem_t* synch_sem1; 
sem_t* synch_sem2;
 
const char *sem1_name = "/my_sem1";
const char *sem2_name = "/my_sem2";
const char *queue_name = "/my_queue1";

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
		mq_send(mq, ping, MAX_SIZE, 0);
		sem_post(postSem);
	}
 
	for(i = 0; i < 10; ++i) 
	{
		sem_wait(waitSem);
		mq_receive(mq, buf, MAX_SIZE, NULL);
		printf("Got %s Writing %s\n", buf, msg);
		mq_send(mq, msg, MAX_SIZE, 0);
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
	mq_close(mq);
	sem_close(synch_sem2);
	sem_close(synch_sem1);
}

void unlinkResources()
{
	sem_unlink(sem2_name);
	sem_unlink(sem1_name);
	mq_unlink(queue_name);
}

void initResources()
{
	synch_sem1 = sem_open(sem1_name, O_RDWR | O_CREAT, 0666, 0);
	synch_sem2 = sem_open(sem2_name, O_RDWR | O_CREAT, 0666, 0);
	sem_post(synch_sem1);
	
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MAX_SIZE;
	attr.mq_curmsgs = 0;
	unlink(queue_name);
	mq = mq_open(queue_name, O_CREAT | O_RDWR, 0666, &attr);
	if ((mqd_t)-1 == mq)
	{
		printf("Error! %s\n", strerror(errno));
	}
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
	printf("opening queue...\n");
	mq = mq_open(queue_name, O_RDWR);
	if ((mqd_t)-1 == mq)
	{
		printf("Error! %s\n", strerror(errno));
	}
	printf("opened\n");
}
