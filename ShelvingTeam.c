#include "local.h"

/************************************** FUNCTIONS & GLOBAL VARIABLES **********************************************/

ShelvingTeam *shared_shelvingTeams;
Product *shared_products;
int shm_id, num_of_products, product_threshold, simulation_threshold, num_of_product_on_shelves, ind, shm_id_for_shelvingTeam;
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task_available = PTHREAD_COND_INITIALIZER;

void getRequiredSharedMemories();

void teamForming();

void *managerThread();

void *employeeThreads();

void clean_up();

/*****************************************************************************************************************/

int main(int argc, char *argv[]) {
    printf("%d created shelving team\n", getpid());
    if (argc != 5) {
        printf("Usage: %s <num_of_product> <product_threshold> <simulation_threshold> <num_of_product_on_shelves>\n",
               argv[0]);
        exit(EXIT_FAILURE);
    }
    num_of_products = atoi(argv[1]);
    product_threshold = atoi(argv[2]);
    simulation_threshold = atoi(argv[3]);
    num_of_product_on_shelves = atoi(argv[4]);

    getRequiredSharedMemories();
    teamForming();
    clean_up();
    exit(EXIT_SUCCESS);
}

/* function to get the shared memories for shelving teams and products */
void getRequiredSharedMemories() {
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
    /* Get the shelving teams shared memory to put the shelving team on it */
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

/* put the team on shared memory, initialize its values, and create manager and employees threads */
void teamForming() {

    /* Put the shelving team in the first available space founded */
    for (ind = 0; ind < SHELVING_TEAMS_NUMBER; ++ind) {
        /* if the shelving team id in this index is -1 => available space */
        if (shared_shelvingTeams[ind].id == -1) {
            shared_shelvingTeams[ind].id = getpid();
            shared_shelvingTeams[ind].current_product_index = -1; /* the team is currently available */
            shared_shelvingTeams[ind].rolling_cart_qnt = 0;
            shared_shelvingTeams[ind].manager_status = -1; /* manager is not working */
            shared_shelvingTeams[ind].employee_status = -1; /* employees are not working */
            break;
        }
    }

    /* create the manager thread */
    pthread_create(&shared_shelvingTeams[ind].manager_thread, NULL, (void *) managerThread, NULL);

    /* create the employees threads */
    for (int i = 0; i < MAX_SHELVES_EMPLOYEES-1; i++) {
        pthread_create(&shared_shelvingTeams[ind].employee_threads[i], NULL, (void *) employeeThreads, NULL);
    }

    /* wait for threads to finish */
    pthread_join(shared_shelvingTeams[ind].manager_thread, NULL);  /* wait for manager thread */
    for (int i = 0; i < MAX_SHELVES_EMPLOYEES-1; i++) {
        pthread_join(shared_shelvingTeams[ind].employee_threads[i], NULL);  /* wait for employee thread */
    }
}

/* the manager thread function */
void *managerThread() {
    int mq_key = getpid();
    int msg_queue_id = msgget(mq_key, 0666 | IPC_CREAT);
    Message msg;
    /* Wait for messages from main parent to know if it is time to restock */
    printf("%d waiting...\n", getpid());
    while (msgrcv(msg_queue_id, &msg, sizeof(msg), 0, 0) > 0) {
        /* the message received */
        if (msg.receiver_id == getpid() && msg.type == 1) {
            printf("team %d received message from main process\n", getpid());
            int product_index = msg.product_index;
            shared_shelvingTeams[ind].current_product_index = product_index;
            /* manager start working */
            shared_shelvingTeams[ind].manager_status = 1;
            /* get the necessary amount from the storage */
            printf("********************************BEFORE********************************\n");
            printf("(inside team %d) %s shelve %d storage %d\n",
                   getpid(),
                   shared_products[product_index].name,
                   shared_products[product_index].quantity_on_shelves,
                   shared_products[product_index].quantity_in_storage
            );
            /* to make sure no threads can work before the manager finished */
            pthread_mutex_lock(&task_mutex);
            sleep(2);
            lock(getppid(), product_index, "ShelvingTeam.c");
            /* if the quantity in the storage and shelves is less than the maximum number of quantity on shelve,
             * then put the whole storage quantity in the rolling cart */
            if (shared_products[product_index].quantity_in_storage +
                shared_products[product_index].quantity_on_shelves <= num_of_product_on_shelves) {
                shared_shelvingTeams[ind].rolling_cart_qnt = shared_products[product_index].quantity_in_storage;
                shared_products[product_index].quantity_in_storage = 0;
            } else {
                /* else, put in the rolling cart, the maximum number of quantity to make the shelve full */
                shared_shelvingTeams[ind].rolling_cart_qnt = num_of_product_on_shelves - shared_products[product_index].quantity_on_shelves;
                shared_products[product_index].quantity_in_storage -=
                        num_of_product_on_shelves - shared_products[product_index].quantity_on_shelves;
            }
            printf("team %d rolling cart: %d for product %s\n", getpid(), shared_shelvingTeams[ind].rolling_cart_qnt,
                   shared_products[product_index].name);

            /* manager has done his work */
            shared_shelvingTeams[ind].manager_status = -1;
            unlock(getppid(), product_index, "ShelvingTeam.c");

            /* signal employee threads to do the work */
            shared_shelvingTeams[ind].employee_status = 1;
            for (int i = 0; i < shared_shelvingTeams[ind].rolling_cart_qnt; ++i) {
                pthread_cond_signal(&task_available);
            }
            /* manager released the mutex lock */
            pthread_mutex_unlock(&task_mutex);
            sleep(2);
        }
    }
    return NULL;
}

/* the employee thread function */
void *employeeThreads() {
    while (1) {
        /* to avoid race condition between threads */
        pthread_mutex_lock(&task_mutex);

        /* wait until the rolling cart has items */
        while (shared_shelvingTeams[ind].rolling_cart_qnt == 0) {
            /* wait for the signal from manager */
            pthread_cond_wait(&task_available, &task_mutex);
        }

        /* decrease the rolling cart quantity by 1 */
        shared_shelvingTeams[ind].rolling_cart_qnt -= 1;

        int product_index = shared_shelvingTeams[ind].current_product_index;
        lock(getppid(), product_index, "ShelvingTeam.c");

        /* increase the quantity on the shelve by 1 */
        usleep(100000);
        shared_products[product_index].quantity_on_shelves += 1;
        usleep(100000);

        printf("********************************AFTER***************************\n");
        printf("(inside team %d) %s shelve %d quantity %d storage %d\n",
               getpid(),
               shared_products[product_index].name,
               shared_products[product_index].quantity_on_shelves,
               shared_shelvingTeams[ind].rolling_cart_qnt,
               shared_products[product_index].quantity_in_storage
        );

        /* if the last item was put, then make the team available again */
        if (shared_shelvingTeams[ind].rolling_cart_qnt == 0) {
            shared_shelvingTeams[ind].employee_status = -1;
            shared_shelvingTeams[ind].current_product_index = -1;
        }
        unlock(getppid(), product_index, "ShelvingTeam.c");
        /* release the mutex lock */
        pthread_mutex_unlock(&task_mutex);
    }
}

/* detach shard memories and destroy threads mutex and condition*/
void clean_up() {
    shmdt(shared_products);
    shmdt(shared_shelvingTeams);
    pthread_mutex_destroy(&task_mutex);
    pthread_cond_destroy(&task_available);
}