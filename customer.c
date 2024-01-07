#include "local.h"

int num_of_product,customer_index, customer_shm_id,shm_id,nItems;
Customer *customer ,*customers_shared_memory;
Product *shared_products;

void getSharedMemories();
void createCustomer();
void cleanup();
void chooseItems();

int main(int argc, char *argv[]) {
    printf("%d customer created\n", getpid());
    if (argc != 2) {
        printf("Usage: %s <num_of_product>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Get variables from argv */
    num_of_product = atoi(argv[1]);
    getSharedMemories();
    createCustomer();
    sleep(3);
    printf("chooseItems 1\n");
    chooseItems();
    printf("chooseItems 2\n");

    //cleanup();

    /* Exit the child process */
    exit(EXIT_SUCCESS);
}

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
void createCustomer() {

    /* Allocate memory for a new Customer structure */
    customer = malloc(sizeof(Customer));

    /* Assign customer values */
    customer->id = getpid();

    /* Put the customer in the first available space founded */
    for (customer_index = 0; customer_index < MAX_CUSTOMERS; ++customer_index) {

        /* if the customer id in this index is -1 => available space */
        if (customers_shared_memory[customer_index].id == -1) {
            customers_shared_memory[customer_index] = *customer;
            break;
        }

    }

}

void chooseItems() {
    int shop_time = 0,shopping_list_index = 0;
    printf("out the for");
    for (int i = 0; i < num_of_product; ++i) {
        printf("inside the for");
        pthread_mutex_lock(&shared_products[i].task_mutex);
        printf("inside the for");
        if (shared_products[i].quantity_on_shelves != 0 && rand() % 3 == 0) {
            int qnt = 1;  // Choose only one quantity
            printf("inside the first if");

            // Check if there is enough quantity on shelves
            if (qnt <= shared_products[i].quantity_on_shelves) {
                printf("inside the second if");

                // Update the chosen item's quantity in the shared memory and put it in the customer bucket
                shared_products[i].quantity_on_shelves -= qnt;

                customer->shopping_list[shopping_list_index].quantity_on_shelves = qnt;
                // You may want to copy the name as well
                printf("chosen products %s for customer %d \n",customer->shopping_list[shopping_list_index].name,getpid());

                shopping_list_index++;

                sleep(1);
            }
        }
        pthread_mutex_unlock(&shared_products[i].task_mutex);

    }
    sleep(4);
}


void cleanup() {
    shmdt(customers_shared_memory);
    shmdt(shared_products);
}