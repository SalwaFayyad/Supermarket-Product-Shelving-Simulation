
#ifndef REALTIME_PROJECT1_LOCAL_H
#define REALTIME_PROJECT1_LOCAL_H

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/mman.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#include "sys/msg.h"
#include <pthread.h>
#include <stdatomic.h>
#include <sys/sem.h>



#define MAX_SIZE  200
#define MAX_SHELVES_EMPLOYEES  6
#define SHELVING_TEAMS_NUMBER 5
#define MAX_CUSTOMERS 500
#define CUSTOMERS_KEY 1002
#define SHELVING_KEY 5000

typedef struct{
    char name[100];
    int quantity_on_shelves;
    int quantity_in_storage;
    float x_position_on_storage;
    float y_position_on_storage;
    float x_position_on_shelves;
    float y_position_on_shelves;
}Product;

typedef struct {
    int id;
    /* [i][0] -> index on the items shared memory, [i][1] -> # of quantity of the items */
    int shopping_list[MAX_SIZE][2];
} Customer;

typedef struct {
    int id;
    pthread_t manager_thread;
    pthread_t employee_threads[MAX_SHELVES_EMPLOYEES-1];
    int current_product_index;
    int rolling_cart_qnt;
    float x_position_manager;
    float y_position_manager;
    float x_position_employee;
    float y_position_employee;
    int manager_status;
    int employee_status;
} ShelvingTeam;

typedef struct {
    int sender_id;
    int receiver_id;
    long type;
    int product_index;
} Message;

/* Function to generate random numbers */
int generateRandomNumber(int min, int max) {
    /* Different seed for each process */
    srand(time(NULL) % getpid());
    if (max < min) {
        printf("Error: Invalid range\n");
        exit(EXIT_FAILURE);
    } else if (max == min) {
        return max;
    }
    int range = max - min + 1;
    int random = rand() % range;
    int randomNumber = min + random;
    return randomNumber;
}

void lock(int key, int index, char class_name[]) {
    int sem_id = semget(key, 0, 0);
    struct sembuf acquire_element = {index, -1, 0};
    if (semop(sem_id, &acquire_element, 1) == -1) {
        char error_message[100];  // Adjust the size as needed
        snprintf(error_message, sizeof(error_message), "Error acquiring semaphore in %s", class_name);
        perror(error_message);
        exit(EXIT_FAILURE);
    }
}

void unlock(int key, int index, char class_name[]) {
    int sem_id = semget(key, 0, 0);
    struct sembuf release_element = {index, 1, 0};
    if (semop(sem_id, &release_element, 1) == -1) {
        char error_message[100];  // Adjust the size as needed
        snprintf(error_message, sizeof(error_message), "Error releasing semaphore in %s", class_name);
        perror(error_message);
        exit(EXIT_FAILURE);
    }
}

#endif //REALTIME_PROJECT1_LOCAL_H
