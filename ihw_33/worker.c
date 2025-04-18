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

    // Получаем сообщение с нужным индексом
    struct msgbuf msg;
    while (1) {
        if (msgrcv(msg_id, &msg, sizeof(msg.data), 1, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        if (msg.data.index == index) break;
        // Если сообщение не для этого worker'а, возвращаем его в очередь
        msgsnd(msg_id, &msg, sizeof(msg.data), 0);
    }

    char encrypted[1024];
    encode_text(msg.data.text, encrypted);

    // Блокируем семафор перед доступом к shared_data
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);

    // Записываем в свой индекс
    strcpy(shared_data->fragments[index].original, msg.data.text);
    strcpy(shared_data->fragments[index].encrypted, encrypted);
    shared_data->fragments[index].index = index;

    // Разблокируем семафор
    sb.sem_op = 1;
    semop(sem_id, &sb, 1);

    printf("[Worker-%d] Encrypted: %s -> %s\n", index, msg.data.text, encrypted);

    shmdt(shared_data);
    return 0;
}