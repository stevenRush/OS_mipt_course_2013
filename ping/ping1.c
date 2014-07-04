#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

typedef struct 
{
	char *message;
	char *dest;
	unsigned mod;
} ThreadInfo, *pThreadInfo;

pthread_t tid[2];
int counter;
pthread_mutex_t lock;

void * Proc(void * args)
{
	ThreadInfo ti = * (pThreadInfo)args;
	int i, j;
	for(i = 0; i < 10 + ti.mod; ++i)
	{
		while(counter % 2 != ti.mod)
			;
		pthread_mutex_lock(&lock);
		counter += 1;
		printf("Got %s Writing %s\n", ti.dest, ti.message);
		strcpy(ti.dest, ti.message);
		pthread_mutex_unlock(&lock);
	}
}
 
int main(int argc, char** argv)
{
	int i;
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("Cannot init mutex!\n");
		return 1;
	}
	
	ThreadInfo ti1, ti2;
	counter = 1;
	char *dest = (char*)malloc(sizeof(char) * 10);
	strcpy(dest, "ping");
	
	ti1.mod = 0;
	ti1.message = (char*)malloc(sizeof(char) * 10);
	strcpy(ti1.message, "ping");
	ti1.dest = dest;
	printf("creating first thread...\n");
	pthread_create(tid, NULL, &Proc, (void*) &ti1);
	
	ti2.mod = 1;
	ti2.message = (char*)malloc(sizeof(char) * 10);
	strcpy(ti2.message, "pong");
	ti2.dest = dest;
	printf("creating second thread...\n");
	pthread_create(tid + 1, NULL, &Proc, (void*) &ti2);
	
	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	pthread_mutex_destroy(&lock);
	free(dest);
	free(ti1.message);
	free(ti2.message);
	return 0;	
}
