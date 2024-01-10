#include "local.h"

ShelvingTeam *team, *shared_shelvingTeams;
Product *shared_products;

void teamForming();

void *managerThread(void *arg);
void *employeeThreads(void *arg) ;

void getRequiredSharedMemories();
void  do_wrap_up(int i,char *name,int j);
void do_wrap_up2();
int can_claim_task(Product *product);

int shm_id,num_of_products,product_threshold,simulation_threshold,num_of_product_on_shelves,rollingCart = 0,ind, manager_index, shm_id_for_shelvingTeam;
atomic_flag task_claimed  ;

pthread_mutex_t task_mutex= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task_available= PTHREAD_COND_INITIALIZER;


pthread_mutex_t mutex_employees[MAX_SHELVES_EMPLOYEES];
void Free();

void createMsgQueue();

int main(int argc, char *argv[]) {
    printf("%d created shelving team\n",getpid());
    if (argc != 5) {
        printf("Usage: %s <num_of_product> <product_threshold> <simulation_threshold> <num_of_product_on_shelves>\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    num_of_products= atoi(argv[1]);
    product_threshold = atoi(argv[2]);
    simulation_threshold =atoi(argv[3]);
    num_of_product_on_shelves = atoi(argv[4]);
 //  printf("num_of_product_on_shelves %d\n",num_of_product_on_shelves);
//    printf("product_threshold %d\n",product_threshold);
//    printf("simulation_threshold %d\n",simulation_threshold);
    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {
        pthread_mutex_init(&mutex_employees[i], NULL);
    }

    getRequiredSharedMemories();
    teamForming();
    Free();
    exit(EXIT_SUCCESS);
}

void getRequiredSharedMemories(){
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

    shm_id_for_shelvingTeam = shmget((int) SHELVING_KEY, 0, 0);
    if (shm_id_for_shelvingTeam == -1) {
        perror("Error accessing shared memory in customer.c");
        exit(EXIT_FAILURE);
    }
    shared_shelvingTeams = (ShelvingTeam *) shmat((int) shm_id_for_shelvingTeam, NULL, 0);
    if (shared_shelvingTeams == (void *) -1) {
        perror("Error attaching shared memory in customer.c");
        exit(EXIT_FAILURE);
    }
}

void teamForming() {

    /* Put the shelving team in the first available space founded */
    for (ind = 0; ind < SHELVING_TEAMS_NUMBER; ++ind) {

        /* if the customer id in this index is -1 => available space */
        if (shared_shelvingTeams[ind].id == -1) {
            ////////////////////make sure it doesn't cause any problem

            shared_shelvingTeams[ind].id = getpid();
            shared_shelvingTeams[ind].current_product_index = -1; // means they are not working

            break;
        }

    }
    /* Reaching the end of shared memory without finding available space */
    if (ind == SHELVING_TEAMS_NUMBER) {
        perror("Maximum size of shelving teams shared memory in ShelvingTeam.c");
        exit(EXIT_FAILURE);
    }

    // Generate a random manager index within the employee range
    manager_index = generateRandomNumber(0,MAX_SHELVES_EMPLOYEES - 1) ;

    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {

        if (i == manager_index) {
            /* Create manager thread at the chosen random index */
            pthread_create(&shared_shelvingTeams[ind].manager_thread, NULL, (void *) managerThread, (void *) &i);
        } else {
            /* Create regular employee thread */
            pthread_create(&shared_shelvingTeams[ind].employee_threads[i], NULL, (void *) employeeThreads, (void*) &i);
        }

    }
    // Wait for individual threads to finish
    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {
        if (i == manager_index) {
            pthread_join(shared_shelvingTeams[ind].manager_thread, NULL);  // Wait for manager thread
        } else {
            pthread_join(shared_shelvingTeams[ind].employee_threads[i], NULL);  // Wait for employee thread
        }
    }
}


//void putShelvingTeamInSharedMemoryAndChooseLeaders() {
//
//    shared_shelvingTeams = malloc(sizeof(ShelvingTeam));
//    pthread_mutex_init(&shared_shelvingTeams->task_mutex, NULL);
//
//    pthread_mutex_lock(&shared_shelvingTeams->task_mutex);
//    shared_shelvingTeams->id = getpid();
//    printf("team %d created \n",getpid());
//
//    shared_shelvingTeams->employee_threads = malloc(MAX_SHELVES_EMPLOYEES * sizeof(pthread_t));
//
//    // Generate a random manager index within the employee range (including 0)
//     manager_index = generateRandomNumber(0,MAX_SHELVES_EMPLOYEES - 1) ;
//    //printf("the manager_index = %d\n",manager_index);
//    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {  // Start loop from 0
//        pthread_mutex_lock(&mutex_employees[i]);
//
//        // Create manager thread at the chosen index
//        if (i == manager_index) {
//            pthread_create(&shared_shelvingTeams->manager_thread, NULL, (void *) managerThread, (void *) &i);
//        } else {
//            // Create regular employee thread
//            pthread_create(&shared_shelvingTeams->employee_threads[i], NULL, (void*) employeeThreads, (void*)&i);
//        }
//        pthread_mutex_unlock(&mutex_employees[i]);
//    }
//
//
//    for (int i = 0; i < MAX_SHELVES_EMPLOYEES; i++) {
//        char *name;
//        if (i == manager_index) {
//            name = "manger";
//            pthread_join(shared_shelvingTeams->manager_thread, NULL);
//        } else {
//            name="employee";
//            pthread_join(shared_shelvingTeams->employee_threads[i], NULL);
//        }
//      //  do_wrap_up(i,name,getpid());
//    }
//
//    printf("thread %d have completed.\n",getpid());
//    pthread_mutex_unlock(task_mutex);
//}

void *managerThread(void *arg) {

    int mq_key = getpid();
    int msg_queue_id = msgget(mq_key, 0666 | IPC_CREAT);
    Message msg;

    while(1) {
        /* Wait for messages from main parent to know if it is time to restock */
        printf("%d waiting........\n", getpid());
        while (msgrcv(msg_queue_id, &msg, sizeof(msg), 0, 0) > 0) {
            //printf("**** cashier queue size: %d *****\n", cashiers_shared_memory[cashier_index].queue_size);
            /* Check if message indicates arrival */
            if (msg.receiver_id == getpid() && msg.type == 1) {
                printf("team %d received message from main process\n", getpid());
                int product_index = msg.product_index;
                shared_shelvingTeams[ind].current_product_index = product_index;
                /* get the necessary amount from the storage */

                lock(getppid(), product_index, "ShelvingTeam.c");
                printf("********************************BEFORE\n");
                printf("******* (inside team) %s shelve %d storage %d\n",
                       shared_products[product_index].name,
                       shared_products[product_index].quantity_on_shelves,
                       shared_products[product_index].quantity_in_storage
                );

                if (shared_products[product_index].quantity_in_storage + shared_products[product_index].quantity_on_shelves <= num_of_product_on_shelves) {

                    shared_shelvingTeams[ind].rolling_cart_qnt = shared_products[product_index].quantity_in_storage;
                    shared_products[product_index].quantity_in_storage = 0;
                } else {
                    shared_shelvingTeams[ind].rolling_cart_qnt = num_of_product_on_shelves - shared_products[product_index].quantity_on_shelves;
                    shared_products[product_index].quantity_in_storage -= num_of_product_on_shelves - shared_products[product_index].quantity_on_shelves;
                }
                printf("team %d rolling cart: %d for product %s\n",  getpid(),shared_shelvingTeams[ind].rolling_cart_qnt, shared_products[product_index].name);
                unlock(getppid(), product_index, "ShelvingTeam.c");

                /* Signal employee threads to do the work */
                pthread_cond_signal(&task_available);
            }
        }
    }
}


void *employeeThreads(void *arg) {
//    int id = *((int *)arg);
//    pthread_exit(NULL);
    ShelvingTeam *this_team = &shared_shelvingTeams[ind];
    while(1) {
        // to avoid race condition between employees threads
        pthread_mutex_lock(&task_mutex);
        pthread_cond_wait(&task_available, &task_mutex);

        if (this_team->current_product_index == -1) {
            pthread_mutex_unlock(&task_mutex);
            continue;
        }
        int product_index = this_team->current_product_index;
        lock(getppid(), product_index, "ShelvingTeam.c");

        shared_products[product_index].quantity_on_shelves += this_team->rolling_cart_qnt;
        printf("********************************AFTER\n");
        printf("******* (inside team) %s shelve %d storage %d\n",
               shared_products[product_index].name,
               shared_products[product_index].quantity_on_shelves,
               shared_products[product_index].quantity_in_storage
        );

        unlock(getppid(), product_index, "ShelvingTeam.c");

        pthread_mutex_unlock(&task_mutex);
    }
}

//int can_claim_task(Product *product) {
//    pthread_mutex_lock(&product->task_mutex);
//    int can_claim = 0;
//    if (!product->is_claimed) {
//        product->is_claimed = 1;
//        can_claim = 1;
//    }
//    pthread_mutex_unlock(&product->task_mutex);
//    return can_claim;
//}

//void mangerThreads(void *arg){
//    int chosen_index;
//    for (int i = 0 ; i < num_of_product ;i++) {
//        if (shared_products[i].quantity_on_shelves <= product_threshold) {
//           // printf("the product %s with quantity %d\n",shared_products[i].name,shared_products[i].quantity_on_shelves );
//            chosen_index = i ;
//           pthread_mutex_lock(&shared_products[i].task_mutex);
//            sharedMemory_shelvingteam[chosen_index].current_task = &shared_products[i];
//            printf("product %s in team %d\n",sharedMemory_shelvingteam[chosen_index].current_task->name,getpid());
//
////
////            if(shared_products[i].quantity_in_storage < num_of_product_on_shelves - 2){
////                rollingCart = shared_products[i].quantity_in_storage;
////            }else{
////                rollingCart = num_of_product_on_shelves - 2;
////            }
////            printf("rollingCart %d product %s in team %d\n",rollingCart,sharedMemory_shelvingteam[chosen_index].current_task->name,getpid());
////         //   pthread_cond_broadcast(&sharedMemory_shelvingteam->task_available);
////            pthread_mutex_unlock(&shared_products[i].task_mutex);
//////
////            break;
//        }
//    }
//}

void mangerThreads(void *arg) {
   // pthread_mutex_lock(&sharedMemory_shelvingteam->task_mutex);
        int task_found = 0;
        for (int i = 0; i < num_of_products; i++) {
       //     pthread_mutex_lock(&shared_products[i].task_mutex);
            if (shared_products[i].quantity_on_shelves <= product_threshold) {
//                printf("product %s in team %d with the manger %d\n", shared_shelvingTeams->current_task.name, getpid(), manager_index);

//                if (can_claim_task(&shared_products[i])) {
//                    printf("in the if 2\n");
//
//                    sharedMemory_shelvingteam->current_task = shared_products[i];
//                    if(shared_products[i].quantity_in_storage < num_of_product_on_shelves - 2){
//                        rollingCart = shared_products[i].quantity_in_storage;
//                      }else{
//                        rollingCart = num_of_product_on_shelves - 2;
//                      }
//                        printf("rollingCart %d product %s in team %d\n",rollingCart,sharedMemory_shelvingteam->current_task.name,getpid());
//                    task_found = 1;
//                    break;
//                }
            }
        //    pthread_mutex_unlock(&shared_products[i].task_mutex);
        }

        if (task_found) {
            // Signal employee threads about the new task
//            pthread_cond_broadcast(&shared_shelvingTeams->task_available);
        } else {
            // Wait for a new task or simulation end
           // pthread_cond_wait(&sharedMemory_shelvingteam->new_task_signal, &sharedMemory_shelvingteam->task_mutex);
        }

   // pthread_mutex_unlock(&sharedMemory_shelvingteam->task_mutex);
}

void do_wrap_up(int i,char *name,int j){
    printf("the id %d for %s in team %d\n", i ,name,j);
}
void do_wrap_up2(){
    printf("rollingCart %d for item %d in team %d\n", rollingCart,ind, getpid());

}

void Free(){
    free(shared_shelvingTeams->employee_threads);
    free(shared_shelvingTeams);
    pthread_mutex_destroy(&task_mutex);
    for(int i = 0 ; i < MAX_SHELVES_EMPLOYEES ;i++){
        pthread_mutex_destroy(&mutex_employees[i]);
    }

    printf("finished %d\n",getpid());
}