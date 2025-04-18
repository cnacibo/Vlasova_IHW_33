#include "defs.h"

char encode_char(char c) {
    if (c >= 'A' && c <= 'Z') return 'A' + ((c - 'A' + 3) % 26);
    if (c >= 'a' && c <= 'z') return 'a' + ((c - 'a' + 3) % 26);
    return c;
}

void encode_text(char *src, char *dest) {
    int i;
    for (i = 0; src[i]; ++i) {
        dest[i] = encode_char(src[i]);
    }
    dest[i] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s index\n", argv[0]);
        return 1;
    }

    int index = atoi(argv[1]);

    int shm_id = shmget(SHM_KEY, sizeof(SharedData), 0666);
    SharedData *shared_data = (SharedData *)shmat(shm_id, NULL, 0);

    int sem_id = semget(SEM_KEY, 1, 0666);
    int msg_id = msgget(MSG_KEY, 0666);

    struct msgbuf msg;
    msgrcv(msg_id, &msg, sizeof(msg.data), 1, 0);

    char encrypted[MAX_FRAGMENT_SIZE];
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
