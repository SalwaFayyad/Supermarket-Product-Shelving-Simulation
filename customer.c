#include "local.h"

/************************************** FUNCTIONS & GLOBAL VARIABLES **********************************************/

int num_of_products, customer_index, customer_shm_id, shm_id;
Customer *customers_shared_memory;
Product *shared_products;

void getSharedMemories();

void putCustomerOnSharedMemory();

void chooseItems();

void cleanup();

/*****************************************************************************************************************/

int main(int argc, char *argv[]) {
    printf("%d customer created\n", getpid());
    if (argc != 2) {
        printf("Usage: %s <num_of_product>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Get variables from argv */
    num_of_products = atoi(argv[1]);

    getSharedMemories();
    putCustomerOnSharedMemory();
    chooseItems();
    cleanup();

    /* Exit the child process */
    exit(EXIT_SUCCESS);
}

/* function to get the shared memories for customers and products */
void getSharedMemories() {
    /* get the customers shared memory to put the customer in */
    customer_shm_id = shmget(CUSTOMERS_KEY, 0, 0);
    if (customer_shm_id == -1) {
        perror("Error accessing customers shared memory in customer.c");
        exit(EXIT_FAILURE);
    }

    customers_shared_memory = (Customer *) shmat((int) customer_shm_id, NULL, 0);
    if (customers_shared_memory == (void *) -1) {
        perror("Error attaching customers shared memory in customer.c");
        exit(EXIT_FAILURE);
    }

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

/* Create the customer and put it in the shared memory */
void putCustomerOnSharedMemory() {
    /* Put the customer in the first available space founded */
    for (customer_index = 0; customer_index < MAX_CUSTOMERS; ++customer_index) {
        /* if the customer id in this index is -1 => available space */
        if (customers_shared_memory[customer_index].id == -1) {
            customers_shared_memory[customer_index].id = getpid();
            break;
        }
    }
}

/* choose products from the shelves */
void chooseItems() {
    float shop_time = 0.0f;
    int shopping_list_index = 0;
    for (int i = 0; i < num_of_products; ++i) {
        /* if the shopping time exceeded 0.5 minute then leave */
        if (shop_time >= 0.5) {
            break;
        }
        /* for buying products randomly */
        int random = generateRandomNumber(0, 3);
        sleep(1);

        if (shared_products[i].quantity_on_shelves != 0 && random % 3 == 0) {
            /* decide the quantity to take from the product */
            int qnt;
            if (shared_products[i].quantity_on_shelves < 5) {
                qnt = generateRandomNumber(1, shared_products[i].quantity_on_shelves);
            } else {
                qnt = generateRandomNumber(1, 5);
            }
            /* take the quantity from the shelve */
            lock(getppid(), i, "customer.c");
            shared_products[i].quantity_on_shelves -= qnt;
            unlock(getppid(), i, "customer.c");
            /* put it in the shopping list */
            customers_shared_memory[customer_index].shopping_list[shopping_list_index][1] = qnt;
            printf("customer %d bought %d from %s \n", getpid(), qnt, shared_products[i].name);
            customers_shared_memory[customer_index].shopping_list[shopping_list_index][0] = i;
            /* increase shopping time */
            shop_time += 1.0f / 60;
            shopping_list_index++;
        }
    }
}

/* remove the customer from the shared memory and detach shared memories */
void cleanup() {
    customers_shared_memory[customer_index].id = -1;
    shmdt(customers_shared_memory);
    shmdt(shared_products);
}