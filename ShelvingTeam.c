#include "local.h"

ShelvingTeam *shelvingteam;

void createShelvingteam();
void *createEmployyesthread();
pthread_mutex_t mutex_employees[MAX_SHELVES_EMPLOYEES];
void Free();

int r1 = 0;
int main() {
   // printf("you are in process %d\n",getpid());
    createShelvingteam();
    Free();
    exit(EXIT_SUCCESS);
}

void createShelvingteam(){
    printf("entered team %d \n",shelvingteam->id);
    shelvingteam = malloc(sizeof(ShelvingTeam));
    pthread_mutex_lock(&shelvingteam->task_mutex);
    shelvingteam->id = getpid();
    shelvingteam->employee_threads = malloc(MAX_SHELVES_EMPLOYEES * sizeof(pthread_t));
    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {
        pthread_mutex_lock(&mutex_employees[i]);
        int *employee_id = malloc(sizeof(int));
        *employee_id =  generateRandomNumber(0,100) ;
        pthread_create(&shelvingteam->employee_threads[i], NULL,(void*) createEmployyesthread, (void*)&i);
        usleep(10000);
        printf("entered employee %d in team %d \n",*employee_id,shelvingteam->id);
        pthread_mutex_unlock(&mutex_employees[i]);
    }
   // pthread_create(&shelvingteam->manager_thread, NULL,(void*) shelvingteam->manager_thread, (void*) &r1);
    shelvingteam->current_task = NULL;
    pthread_mutex_unlock(&shelvingteam->task_mutex);
}

void *createEmployyesthread(void *arg) {
    int employee_id = *((int *)arg);
    // Do something with the employee_id
    //printf("Employee thread %d\n", employee_id);
    return NULL;
}

void Free(){
    free(shelvingteam->employee_threads);
    free(shelvingteam);

}