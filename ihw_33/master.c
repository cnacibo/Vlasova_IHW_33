#include "defs.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s \"TEXT TO ENCRYPT\" <NUM_PROCESSES>\n", argv[0]);
        return 1;
    }

    char *input = argv[1];
    int num_processes = atoi(argv[2]);

    if (num_processes <= 0 || num_processes > MAX_PROCESSES) {
        fprintf(stderr, "Invalid number of processes.\n");
        return 1;
    }

    int shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    SharedData *shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    shared_data->count = num_processes; // Сохраняем количество процессов

    int sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);

    int msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);

    // Разделим текст на фрагменты
    int len = strlen(input);
    int chunk_size = len / num_processes;
    if (len % num_processes != 0) chunk_size++;

    for (int i = 0; i < num_processes; i++) {
        struct msgbuf msg;
        msg.mtype = 1;
        msg.data.index = i;
        int start = i * chunk_size;
        int size = chunk_size;
        if (start + size > len) size = len - start;
        strncpy(msg.data.text, input + start, size);
        msg.data.text[size] = '\0';
        msgsnd(msg_id, &msg, sizeof(msg.data), 0);
    }

    // Запуск процессов
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char idx[8];
            sprintf(idx, "%d", i);
            execl("./worker", "worker", idx, NULL);
            perror("execl");
            exit(1);
        }
    }

    // Ожидание завершения всех worker'ов
    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }

    printf("\n[Master] Final encoded message:\n");
    // Выводим фрагменты в правильном порядке
    for (int i = 0; i < num_processes; i++) {
        printf("%s", shared_data->fragments[i].encrypted);
    }
    printf("\n");

    // Очистка
    shmdt(shared_data);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    msgctl(msg_id, IPC_RMID, NULL);
    printf("[Master] Cleaned up resources.\n");

    return 0;
}