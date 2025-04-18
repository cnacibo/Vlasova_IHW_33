#include "defs.h"
int shm_id, sem_id, msg_id;
SharedData *shared_data;

void cleanup(int sig) {
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    msgctl(msg_id, IPC_RMID, NULL);
    printf("\n[Master] Cleaned up resources.\n");
    exit(0);
}

void sem_op(int op) {
    struct sembuf sb = {0, op, 0};
    semop(sem_id, &sb, 1);
}

void split_and_send(const char *text, int n_workers) {
    int len = strlen(text);
    int chunk = len / n_workers;

    for (int i = 0; i < n_workers; ++i) {
        struct msgbuf msg;
        msg.mtype = 1;
        msg.data.index = i;
        strncpy(msg.data.text, text + i * chunk, chunk);
        msg.data.text[chunk] = '\0';
        msgsnd(msg_id, &msg, sizeof(msg.data), 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s \"TEXT\" num_workers\n", argv[0]);
        return 1;
    }

    signal(SIGINT, cleanup);

    const char *text = argv[1];
    int workers = atoi(argv[2]);

    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    shared_data->count = 0;

    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);

    msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);

    split_and_send(text, workers);

    for (int i = 0; i < workers; ++i) {
        if (fork() == 0) {
            char index_str[10];
            sprintf(index_str, "%d", i);
            execl("./worker", "./worker", index_str, NULL);
            perror("execl");
            exit(1);
        }
    }

    for (int i = 0; i < workers; ++i) {
        wait(NULL);
    }

    printf("\n[Master] Final encoded message:\n");
    for (int i = 0; i < workers; ++i) {
        printf("%s", shared_data->fragments[i].encrypted);
    }
    printf("\n");
    cleanup(0);
    return 0;
}