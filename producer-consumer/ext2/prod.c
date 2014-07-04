#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SLEEP 1

const char *shm_name = "/my_shm";

typedef struct
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
} cond_var;

cond_var fill_cond;
cond_var empty_cond;
pthread_mutex_t sync_mutex;
pthread_t *tid;
volatile int *ptr;
int work;
int shm_fd;
size_t workersNumber;
size_t storageSize;

void* producer(void *args)
{
	while(work)
	{
		int product;
		pthread_mutex_lock(&empty_cond.mutex);
	
		while(*ptr == storageSize)
		{
			pthread_cond_wait(&empty_cond.cond, &empty_cond.mutex);
		}
		
		pthread_mutex_lock(&sync_mutex);
		
		product = rand() % 10000;		
		printf("I put product %d to room %d\n", product, *ptr+1);
		ptr[++*ptr] = product;
		if (*ptr == 1)
			pthread_cond_signal(&fill_cond.cond);
			
		pthread_mutex_unlock(&sync_mutex);
		sleep(SLEEP);
		
		pthread_mutex_unlock(&empty_cond.mutex);
	}
	printf("producer is done\n");
}

void* consumer(void *args)
{
	while(work)
	{
		pthread_mutex_lock(&fill_cond.mutex);
		
		while(*ptr == 0)
		{
			pthread_cond_wait(&fill_cond.cond, &fill_cond.mutex);
		}
		
		pthread_mutex_lock(&sync_mutex);
		
		printf("I got product %d from room %d\n", ptr[*ptr], *ptr);
		--*ptr;
		if (*ptr == 9)
			pthread_cond_signal(&empty_cond.cond);
			
		pthread_mutex_unlock(&sync_mutex);
		sleep(SLEEP);
				
		pthread_mutex_unlock(&fill_cond.mutex);
	}
	printf("consumer is done\n");
}

void InitResources()
{
	pthread_cond_init(&empty_cond.cond, NULL); 
	pthread_cond_init(&fill_cond.cond, NULL);	
	pthread_mutex_init(&empty_cond.mutex, NULL);
	pthread_mutex_init(&fill_cond.mutex, NULL);
	pthread_mutex_init(&sync_mutex, NULL);
	
	shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0666);
	ftruncate(shm_fd, 1024);
}

void DestroyResources()
{
	pthread_cond_destroy(&empty_cond.cond);
	pthread_cond_destroy(&fill_cond.cond);
	pthread_mutex_destroy(&empty_cond.mutex);
	pthread_mutex_destroy(&fill_cond.mutex);
	pthread_mutex_destroy(&sync_mutex);
	
	shm_unlink(shm_name);
}

void signalHandler(int signal)
{
	if (signal == SIGINT)
	{
		int i;
		work = 0;
		printf("wait while operations will be complete...\n");
		for(i = 0; i < workersNumber; ++i)
			pthread_join(tid[i], NULL);
		DestroyResources();
		exit(-1);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("Usage: prod producerNumber consumerNumber storageCapacity\n");
		return 0;
	}
	
	signal(SIGINT, &signalHandler);
	
	work = 1;
	int prodNumber, consNumber;
	size_t i;
	prodNumber = atoi(argv[1]);
	consNumber = atoi(argv[2]);
	storageSize = atoi(argv[3]);
	
	workersNumber = prodNumber + consNumber;
	
	InitResources();
	ptr = mmap(0, 512, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
	
	tid = (pthread_t *)malloc(workersNumber * sizeof(pthread_t));
	for(i = 0; i < prodNumber; ++i)
	{
		pthread_create(tid + i, NULL, &producer, NULL);
	}
	
	for(i = prodNumber; i < workersNumber; ++i)
	{
		pthread_create(tid + i, NULL, &consumer, NULL);
	}
	
	while(1)
		;
	
	return 0;
}
