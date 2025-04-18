#ifndef DEFS_H
#define DEFS_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define SHM_KEY 1234
#define SEM_KEY 5678
#define MSG_KEY 9012

#define MAX_TEXT_LEN 1024
#define MAX_WORKERS 10
#define MAX_FRAGMENT_SIZE 256

struct msgbuf {
    long mtype;
    struct {
        int index;
        char text[MAX_FRAGMENT_SIZE];
    } data;
};

typedef struct {
    int index;
    char original[MAX_FRAGMENT_SIZE];
    char encrypted[MAX_FRAGMENT_SIZE];
} Fragment;

typedef struct {
    Fragment fragments[MAX_WORKERS];
    int count;
} SharedData;

#endif