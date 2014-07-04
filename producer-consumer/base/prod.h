#pragma once

int shm_fd;
sem_t *sem_fillcount;
sem_t *sem_emptycount;
sem_t *sem_sync;
mqd_t mq;

const char *shm_name = "/my_shm";
const char *sem_fillcount_name = "/sem_fillcount";
const char *sem_emptycount_name = "/sem_emptycount";
const char *sem_sync_name = "/sem_sync";
const char *queue_name = "/my_queue4";
