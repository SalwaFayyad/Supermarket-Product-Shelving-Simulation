
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
#include <sys/sem.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#include "sys/msg.h"

#define MAX_SIZE  200
#define MAX_SHELVES_TEAM  10
#define MAX_EMPLOYEE_TEAM  10
#define MAX_CUSTOMERS 500


typedef struct {
    int id;
    int arrival_time;
    /* [i][0] -> index on the items shared memory, [i][1] -> # of quantity of the items */
    int shopping_list[MAX_SIZE][2];
} Customer;

typedef struct{
    char name[100];
    int quantity_on_shelves;
    int quantity_in_storage;
    int threshold;
    int last_item_flag;
}Products;

typedef struct{
    int id;
//    int manager_thread;
//    int employee_threads;
//    int current_task;

}Shelving_teams;



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
