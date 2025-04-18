#include "defs.h"

#define ALPHABET_SIZE 26
int code_table[ALPHABET_SIZE] = {
    10, 42, 12, 21, 7, 45, 67, 88, 23, 90,
    34, 56, 78, 11, 33, 55, 77, 99, 22, 44,
    66, 8, 9, 15, 19, 25
};

void encode_text(const char *src, char *dest) {
    char buffer[1024] = {0};
    for (int i = 0; src[i]; ++i) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') {
            char num[8];
            sprintf(num, "%d ", code_table[c - 'A']);
            strcat(buffer, num);
        } else if (c >= 'a' && c <= 'z') {
            char num[8];
            sprintf(num, "%d ", code_table[c - 'a']);
            strcat(buffer, num);
        } else if (c == ' ') {
            strcat(buffer, "| ");
        } else {
            strcat(buffer, "? ");
        }
    }
    strcpy(dest, buffer);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fragment_index>\n", argv[0]);
        exit(1);
    }

    int index = atoi(argv[1]);

    int shm_id = shmget(SHM_KEY, sizeof(SharedData), 0666);
    SharedData *shared_data = (SharedData *)shmat(shm_id, NULL, 0);

    int sem_id = semget(SEM_KEY, 1, 0666);
    int msg_id = msgget(MSG_KEY, 0666);

    struct msgbuf msg;
    msgrcv(msg_id, &msg, sizeof(msg.data), 1, 0);

    char encrypted[1024];
    encode_text(msg.data.text, encrypted);

    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);

    strcpy(shared_data->fragments[index].original, msg.data.text);
    strcpy(shared_data->fragments[index].encrypted, encrypted);
    shared_data->fragments[index].index = index;

    sb.sem_op = 1;
    semop(sem_id, &sb, 1);

    printf("[Worker-%d] Encrypted: %s -> %s\n", index, msg.data.text, encrypted);

    return 0;
}
