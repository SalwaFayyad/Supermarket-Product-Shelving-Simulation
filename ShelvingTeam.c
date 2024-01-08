#include "local.h"

ShelvingTeam *shelvingteam;
Product *shared_products;

void createShelvingteam();
void *employeeThreads(void *arg) ;
void mangerThreads(void *arg);
void getSharedProducts();
void  do_wrap_up(int i,char *name,int j);
void do_wrap_up2();

int shm_id,num_of_product,product_threshold,simulation_threshold,num_of_product_on_shelves,rollingCart = 0,ind;

pthread_mutex_t mutex_employees[MAX_SHELVES_EMPLOYEES];
void Free();

int main(int argc, char *argv[]) {
   // printf("you are in process %d\n",getpid());
    if (argc != 5) {
        printf("Usage: %s <num_of_product> <product_threshold> <simulation_threshold> <num_of_product_on_shelves>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    num_of_product= atoi(argv[1]);
    product_threshold = atoi(argv[2]);
    simulation_threshold =atoi(argv[3]);
    num_of_product_on_shelves = atoi(argv[4]);
 //  printf("num_of_product_on_shelves %d\n",num_of_product_on_shelves);
//    printf("product_threshold %d\n",product_threshold);
//    printf("simulation_threshold %d\n",simulation_threshold);
    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {
        pthread_mutex_init(&mutex_employees[i], NULL);
    }

    getSharedProducts();
    createShelvingteam();
    Free();
    exit(EXIT_SUCCESS);
}

void createShelvingteam() {
    shelvingteam = malloc(sizeof(ShelvingTeam));
    pthread_mutex_init(&shelvingteam->task_mutex, NULL);

    pthread_mutex_lock(&shelvingteam->task_mutex);
    shelvingteam->id = getpid();
    printf("team %d created \n",getpid());

    shelvingteam->employee_threads = malloc(MAX_SHELVES_EMPLOYEES * sizeof(pthread_t));

    // Generate a random manager index within the employee range (including 0)
    int manager_index = generateRandomNumber(0,MAX_SHELVES_EMPLOYEES - 1) ;
    //printf("the manager_index = %d\n",manager_index);
    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {  // Start loop from 0
        pthread_mutex_lock(&mutex_employees[i]);
        srand(time(NULL) % getpid());  // Seed for each thread creation

        // Create manager thread at the chosen index
        if (i == manager_index) {
            pthread_create(&shelvingteam->manager_thread, NULL, (void*)mangerThreads, (void*)&i);
        } else {
            // Create regular employee thread
            pthread_create(&shelvingteam->employee_threads[i], NULL, (void*) employeeThreads, (void*)&i);
        }
        pthread_mutex_unlock(&mutex_employees[i]);
    }


    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {
        if (i == manager_index) {
            pthread_join(shelvingteam->manager_thread, NULL);
        } else {
            pthread_join(shelvingteam->employee_threads[i], NULL);
        }
    }

    printf("thread %d have completed.\n",getpid());
    pthread_mutex_unlock(&shelvingteam->task_mutex);
}


void *employeeThreads(void *arg) {
    int id = *((int *)arg);
    pthread_exit(NULL);
}

void mangerThreads(void *arg){
   // pthread_mutex_lock(&shelvingteam->task_mutex);
    for (int i = 0 ; i < num_of_product ;i++) {
        if (shared_products[i].quantity_on_shelves <= product_threshold) {
            printf("the product %s with quantity %d\n",shared_products[i].name,shared_products[i].quantity_on_shelves );
            pthread_mutex_lock(&shared_products[i].task_mutex);
            shelvingteam->current_task = shared_products[i];
            if(shared_products[i].quantity_in_storage < num_of_product_on_shelves - 2){
                rollingCart = shared_products[i].quantity_in_storage;
            }else{
                rollingCart = num_of_product_on_shelves - 2;
            }
            ind = i ;
            printf("the rollingCart %d from product %s in team %d\n",rollingCart,shared_products[i].name,getpid());
            pthread_cond_broadcast(&shelvingteam->task_available);
            pthread_mutex_unlock(&shared_products[i].task_mutex);

            //   pthread_mutex_unlock(&shelvingteam->task_mutex);
            break;
        }
    }
}



void getSharedProducts(){
    /* Get the items shared memory to choose items from it */
    shm_id = shmget((int) getppid(), 0, 0);
    if (shm_id == -1) {
        perror("Error accessing shared memory in customer.c");
        exit(EXIT_FAILURE);
    }
    shared_products = (Product *) shmat((int) shm_id, NULL, 0);
    if (shared_products == (void *) -1) {
        perror("Error attaching shared memory in customer.c");
        exit(EXIT_FAILURE);
    }
}

void do_wrap_up(int i,char *name,int j){
    printf("the id %d for %s in team %d\n", i ,name,j);
}
void do_wrap_up2(){
    printf("rollingCart %d for item %d in team %d\n", rollingCart,ind, getpid());

}

void Free(){
    free(shelvingteam->employee_threads);
    free(shelvingteam);
    pthread_mutex_destroy(&shelvingteam->task_mutex);
    for(int i = 0 ; i < MAX_SHELVES_EMPLOYEES ;i++){
        pthread_mutex_destroy(&mutex_employees[i]);
    }

    printf("finished %d\n",getpid());
}