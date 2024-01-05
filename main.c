#include <stdio.h>
#include "local.h"

int num_of_product;
int num_of_product_on_shelves;
int product_threshold ;
int arrival_rate_min ;
int arrival_rate_max ;
int simulation_threshold ;

void readArguments(char *file_name);
void createSharedMemoryForItems();
void readProducts();


int shm_id ,Products_count;
Products *shared_Products;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uh oh! Something went wrong.\n");
        printf("You didn't provide the arguments file I need.\n");
        printf("Please try again with: %s arguments.txt\n", argv[0]);
        exit(-1);
    }

    readArguments(argv[1]);
    readProducts();

    return 0;
}

void readArguments(char *file_name) {

    /* Read from the arguments file */
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];

    /* Read data from the file line by line using fgets */
    while (fgets(buffer, sizeof(buffer), file)) {
        int p, ma, p_th, ar_min, ar_max, sim_th;
        /* Attempt to parse the buffer to extract the values */
        if (sscanf(buffer, "num_of_product = %d", &p) == 1) {
            num_of_product = p; /* If successful, update the values */
        } else if (sscanf(buffer, "num_of_product_on_shelves = %d", &ma) == 1) {
            num_of_product_on_shelves = ma;
        } else if (sscanf(buffer, "product_threshold = %d", &p_th) == 1) {
            product_threshold = p_th;
        } else if (sscanf(buffer, "arrival_rate_min = %d", &ar_min) == 1) {
            arrival_rate_min = ar_min;
        } else if (sscanf(buffer, "arrival_rate_max = %d", &ar_max) == 1) {
            arrival_rate_max = ar_max;
        } else if (sscanf(buffer, "simulation_threshold = %d", &sim_th) == 1) {
            simulation_threshold = sim_th;
        }
    }
    fclose(file);
}

//void createSharedMemoryForItems() {
//    /* Create a shared memory segment */
//    shm_id = shmget(getpid(), sizeof(Products) * MAX_SIZE, IPC_CREAT | 0666);
//    if (shm_id == -1) {
//        perror("Error creating shared memory in the parent process");
//        exit(EXIT_FAILURE);
//    }
//
//    /* Attach the shared memory segment */
//    shared_Products = (Products *) shmat(shm_id, NULL, 0);
//    if (shared_Products == (void *) -1) {
//        perror("Error attaching shared memory in the parent process");
//        exit(EXIT_FAILURE);
//    }
//}


void readProducts() {
    /* Read from the items_list file */
    FILE *file = fopen("Products.txt", "r");
    if (file == NULL) {
        perror("Error opening items_list.txt file");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    Products_count = 0;

    /* Read data from the file line by line using fgets */
    while (fgets(buffer, sizeof(buffer), file)) {

        /* split each line using ',' as the delimiter */
        char *token = strtok(buffer, ",");

        /* Each line split to (name, quantity, price) */
        for (int i = 0; i < 2; ++i) {

            /* Enter the items name to the items shred memory */
            if (i == 0) {
                shared_Products[Products_count].name[0] = '\0';
                strncat(shared_Products[Products_count].name, token,
                        sizeof(shared_Products[Products_count].name) - strlen(shared_Products[Products_count].name) - 1);
                shared_Products[Products_count].name[sizeof(shared_Products[Products_count].name) - 1] = '\0';
            }else {    /* Enter the items price to the items shred memory and convert it to a float */
                shared_Products[Products_count].quantity_on_shelves = num_of_product_on_shelves;
                shared_Products[Products_count].quantity_in_storage =  atof(token) - num_of_product_on_shelves;
            }
            shared_Products[Products_count].threshold = product_threshold;
            shared_Products[Products_count].last_item_flag = 0;
            token = strtok(NULL, ","); /* Move to the next token */
        }
        Products_count++;
        if (Products_count >= MAX_SIZE) { /* Break out of the loop if the maximum size is reached */
            break;
        }
    }
    /* Set the last_item_flag of the last item to 1 (indicating it's the last item) */
    shared_Products[Products_count - 1].last_item_flag = 1;
    fclose(file); /* Close the file */
}