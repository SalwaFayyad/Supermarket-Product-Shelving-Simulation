#include "local.h"

ShelvingTeam *shelvingteam;

void createShelvingteam();
void *createthreadsid();
pthread_mutex_t mutex_employees[MAX_SHELVES_EMPLOYEES];
void Free();

int managerThread = 0;
int main() {
   // printf("you are in process %d\n",getpid());
    createShelvingteam();
    Free();
    exit(EXIT_SUCCESS);
}

void createShelvingteam(){
    shelvingteam = malloc(sizeof(ShelvingTeam));
    pthread_mutex_lock(&shelvingteam->task_mutex);
    shelvingteam->id = getpid();
    //printf("entered team %d \n",shelvingteam->id);
    shelvingteam->employee_threads = malloc(MAX_SHELVES_EMPLOYEES * sizeof(pthread_t));
    for (int i = 1; i < MAX_SHELVES_EMPLOYEES; i++) {
        pthread_mutex_lock(&mutex_employees[i]);
        srand(time(NULL) % getpid());
        int *employee_id = malloc(sizeof(int));
        *employee_id =  i;
        pthread_create(&shelvingteam->employee_threads[i], NULL,(void*) createthreadsid, (void*)&i);

        pthread_mutex_unlock(&mutex_employees[i]);
      //  printf("entered employee %d in team %d  \n",*employee_id,shelvingteam->id);
    }
    pthread_create(&shelvingteam->manager_thread, NULL,(void*) createthreadsid, (void*)&managerThread);
    shelvingteam->current_task = NULL;
    pthread_mutex_unlock(&shelvingteam->task_mutex);
}

void *createthreadsid(void *arg) {
    int id = *((int *)arg);
    // Do something with the employee_id
    //printf("Employee thread %d\n", employee_id);
    return NULL;
}

void Free(){
    free(shelvingteam->employee_threads);
    free(shelvingteam);
    printf("finished %d\n",getpid());
}