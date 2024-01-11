#include "local.h"

ShelvingTeam *shared_shelvingTeams;
Product *shared_products;

void getRequiredSharedMemories();

void teamForming();

void *managerThread();

void *employeeThreads();

void clean_up();

int shm_id, num_of_products, product_threshold, simulation_threshold, num_of_product_on_shelves, ind, shm_id_for_shelvingTeam;

pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task_available = PTHREAD_COND_INITIALIZER;

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
            shared_shelvingTeams[ind].rolling_cart_qnt = 0;
            shared_shelvingTeams[ind].manager_status = -1;
            shared_shelvingTeams[ind].employee_status = -1;
            break;
        }

    }
    /* Reaching the end of shared memory without finding available space */
//    if (ind == SHELVING_TEAMS_NUMBER) {
//        perror("Maximum size of shelving teams shared memory in ShelvingTeam.c");
//        exit(EXIT_FAILURE);
//    }

    /* Create manager thread at the chosen random index */
    pthread_create(&shared_shelvingTeams[ind].manager_thread, NULL, (void *) managerThread, NULL);

    for (int i = 0; i < MAX_SHELVES_EMPLOYEES-1; i++) {
        /* Create regular employee thread */
        pthread_create(&shared_shelvingTeams[ind].employee_threads[i], NULL, (void *) employeeThreads, NULL);
    }

    // Wait for individual threads to finish
    pthread_join(shared_shelvingTeams[ind].manager_thread, NULL);  // Wait for manager thread
    for (int i = 0; i < MAX_SHELVES_EMPLOYEES-1; i++) {
        pthread_join(shared_shelvingTeams[ind].employee_threads[i], NULL);  // Wait for employee thread
    }
}

void *managerThread() {

    int mq_key = getpid();
    int msg_queue_id = msgget(mq_key, 0666 | IPC_CREAT);
    Message msg;
    /* Wait for messages from main parent to know if it is time to restock */
    printf("%d waiting........\n", getpid());
    while (msgrcv(msg_queue_id, &msg, sizeof(msg), 0, 0) > 0) {
        /* Check if message indicates arrival */
        if (msg.receiver_id == getpid() && msg.type == 1) {
            printf("team %d received message from main process\n", getpid());
            int product_index = msg.product_index;
            shared_shelvingTeams[ind].current_product_index = product_index;
            /* get the necessary amount from the storage */
            shared_shelvingTeams[ind].manager_status = 1;
            shared_shelvingTeams[ind].employee_status = 1;

            lock(getppid(), product_index, "ShelvingTeam.c");
            printf("********************************BEFORE\n");
            printf("******* (inside team %d) %s shelve %d storage %d\n",
                   getpid(),
                   shared_products[product_index].name,
                   shared_products[product_index].quantity_on_shelves,
                   shared_products[product_index].quantity_in_storage
            );

            sleep(2);
            pthread_mutex_lock(&task_mutex);

            if (shared_products[product_index].quantity_in_storage +
                shared_products[product_index].quantity_on_shelves <= num_of_product_on_shelves) {

                shared_shelvingTeams[ind].rolling_cart_qnt = shared_products[product_index].quantity_in_storage;
                shared_products[product_index].quantity_in_storage = 0;

                shared_shelvingTeams[ind].manager_status = -1;

                // sleep(1);
            } else {
                shared_shelvingTeams[ind].manager_status = -1;

                shared_shelvingTeams[ind].rolling_cart_qnt = num_of_product_on_shelves - shared_products[product_index].quantity_on_shelves;
                shared_products[product_index].quantity_in_storage -=
                        num_of_product_on_shelves - shared_products[product_index].quantity_on_shelves;
                //sleep(1);
            }
            printf("team %d rolling cart: %d for product %s\n", getpid(), shared_shelvingTeams[ind].rolling_cart_qnt,
                   shared_products[product_index].name);
            unlock(getppid(), product_index, "ShelvingTeam.c");

            /* Signal employee threads to do the work */
            for (int i = 0; i < shared_shelvingTeams[ind].rolling_cart_qnt; ++i) {
                pthread_cond_signal(&task_available);
            }
            pthread_mutex_unlock(&task_mutex);
        }
    }
    return NULL;
}

void *employeeThreads() {
    while (1) {
        // to avoid race condition between employees threads
        sleep(2);
        pthread_mutex_lock(&task_mutex);

        while (shared_shelvingTeams[ind].rolling_cart_qnt == 0) {
            pthread_cond_wait(&task_available, &task_mutex);
        }

        shared_shelvingTeams[ind].rolling_cart_qnt -= 1;

        int product_index = shared_shelvingTeams[ind].current_product_index;
        lock(getppid(), product_index, "ShelvingTeam.c");

        usleep(100000);
        shared_products[product_index].quantity_on_shelves += 1;
        usleep(100000);

        printf("********************************AFTER\n");
        printf("******* (inside team %d) %s shelve %d quantity %d storage %d\n",
               getpid(),
               shared_products[product_index].name,
               shared_products[product_index].quantity_on_shelves,
               shared_shelvingTeams[ind].rolling_cart_qnt,
               shared_products[product_index].quantity_in_storage
        );

        if (shared_shelvingTeams[ind].rolling_cart_qnt == 0) {
            shared_shelvingTeams[ind].current_product_index = -1;
            shared_shelvingTeams[ind].employee_status = -1;
        }

        unlock(getppid(), product_index, "ShelvingTeam.c");
        pthread_mutex_unlock(&task_mutex);
    }
}

void clean_up() {
    shmdt(shared_products);
    shmdt(shared_shelvingTeams);
    pthread_mutex_destroy(&task_mutex);
    pthread_cond_destroy(&task_available);
    printf("finished %d\n", getpid());
}