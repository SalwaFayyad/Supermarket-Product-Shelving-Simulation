
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
#include <sys/mman.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdbool.h>
#include "sys/msg.h"
#include <pthread.h>
#include <stdatomic.h>


#define MAX_SIZE  200
#define MAX_SHELVES_EMPLOYEES  6
#define MAX_CUSTOMERS 500
#define CUSTOMERS_KEY 1001

typedef struct{
    char name[100];
    int quantity_on_shelves;
    int quantity_in_storage;
    int threshold;
    int is_claimed;
    pthread_mutex_t task_mutex;
    //  int last_item_flag;
}Product;

typedef struct {
    int id;
    int arrival_time;
    /* [i][0] -> index on the items shared memory, [i][1] -> # of quantity of the items */
    int shopping_list[MAX_SIZE][2];
} Customer;

typedef struct {
    int id;
    pthread_t manager_thread;
    pthread_t *employee_threads;
    Product current_task;
    pthread_mutex_t task_mutex;
    pthread_cond_t task_available;
} ShelvingTeam;

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



#endif //REALTIME_PROJECT1_LOCAL_H
