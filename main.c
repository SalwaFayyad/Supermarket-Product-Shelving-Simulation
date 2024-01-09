#include <stdio.h>
#include "local.h"

int num_of_products;
int num_of_product_on_shelves;
int product_threshold ;
int arrival_rate_min ;
int arrival_rate_max ;
int simulation_threshold ;

void readArguments(char *file_name);
void createSharedMemories();
void createSemaphoresForProducts();
void createMsgQueue();
void readProducts();
void generateShelvingTeams();
void *productsCheck(void *arg);
void cleanup();
void generateCustomers();
/* PUT THE SHELVING TEAM IN SHARED MEMORY*/

int product_shm_id, products_count, nShelvingTeams = 5, customers_shm_id, shelving_shm_id, items_sem_id, message_queue_id;
Product *shared_products;
ShelvingTeam *shared_shelvingTeams;
Customer *shared_customers;

pthread_t products_check_thread;

//void drawText(float x, float y, const char *text) {
//    glColor3f(0.0, 0.0, 0.0); // Set text color to black
//
//    // Set the position for the text (centered)
//    float textX = x - glutBitmapLength(GLUT_BITMAP_8_BY_13, (const unsigned char *)text) / 2;
//    float textY = y;
//
//    glRasterPos2f(textX, textY);
//
//    // Display each character of the string
//    for (int i = 0; text[i] != '\0'; i++) {
//        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, text[i]);
//    }
//}
//
//void drawCustomers() {
//
//    // Assuming you have information about the number of customers and their positions
//    int numCustomers = 5; // Replace with the actual number of customers
//    float spacing = 70.0; // Adjust the spacing between customers
//
//    for (int i = 0; i < numCustomers; i++) {
//        float x = i * spacing + 100; // Adjust the starting position
//        float y = 500; // Adjust the y-coordinate
//        glColor3f(1.0, 1.0, 1.0);
//
//        // Draw a square for each customer
//        glBegin(GL_QUADS);
//        glVertex2f(x - 25, y - 25); // Top left
//        glVertex2f(x + 25, y - 25); // Top right
//        glVertex2f(x + 25, y + 25); // Bottom right
//        glVertex2f(x - 25, y + 25); // Bottom left
//        glEnd();
//
//        glColor3f(0.0f, 0.0f, 0.0f); // Black text
//        glRasterPos2f(x - 12.0f, y );
//
//        char CustomerId[6]; /* Convert ID to string */
//        sprintf(CustomerId, "%d", shared_customers->id);
//        for (int i = 0; CustomerId[i] != '\0'; ++i) {
//            /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
//            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, CustomerId[i]);
//        }
//    }
//
//}
//
//
//void display() {
//    glClear(GL_COLOR_BUFFER_BIT);
//
//    // Draw customers
//    drawCustomers();
//
//    glFlush();
//}
//
//void reshape(int width, int height) {
//    glViewport(0,0,(GLsizei)width,(GLsizei)height);
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//    gluOrtho2D(0.0, 1500.0, -300.0, 600.0);
//    glMatrixMode(GL_MODELVIEW);
//}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uh oh! Something went wrong.\n");
        printf("You didn't provide the arguments file I need.\n");
        printf("Please try again with: %s arguments.txt\n", argv[0]);
        exit(-1);
    }

    readArguments(argv[1]);
    createSharedMemories();
    createSemaphoresForProducts();
    createMsgQueue();
    readProducts();
    generateShelvingTeams();

    /* Create the threshold check thread */
    if (pthread_create(&products_check_thread, NULL, productsCheck, NULL) != 0) {
        perror("Error creating products check thread");
        exit(EXIT_FAILURE);
    }
//    generateMultipleShelvingTeams();


//    pid_t opengl = fork();
//    if (opengl == -1) {
//        perror("Error forking opengl process");
//        exit(EXIT_FAILURE);
//    } else if (opengl == 0) { // This is the child process
//        execlp("./opengl", "./opengl", (char *)NULL);
//        perror("Error executing opengl");
//        exit(EXIT_FAILURE);
//    } else {
//        close(STDOUT_FILENO);
//        close(STDERR_FILENO);
//    }

//    glutInit(&argc, argv);
//    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
//    glutInitWindowSize(1500, 1000);
//    glutInitWindowPosition(10, 10);
//    glutCreateWindow("Circle Example");
//    glutReshapeFunc(reshape);
//    glutDisplayFunc(display);
//    glutMainLoop();

    for(int i = 0 ;i < 5 ;i++){
        generateCustomers();
        sleep(5);
    }



//    for (int  i = 0 ;i < Products_count ;i++){
//        printf("name %s\n",shared_Products[i].name);
//        printf("quantity_on_shelves %d\n",shared_Products[i].quantity_on_shelves);
//        printf("quantity_in_storage %d\n",shared_Products[i].quantity_in_storage);
//        printf("threshold %d\n",shared_Products[i].threshold);
//    }
    sleep(10);
    cleanup();
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
        if (sscanf(buffer, "num_of_products = %d", &p) == 1) {
            num_of_products = p; /* If successful, update the values */
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

void createSharedMemories() {
    /* Create a shared memory segment */
    product_shm_id = shmget(getpid(), sizeof(Product) * MAX_SIZE, IPC_CREAT | 0666);
    if (product_shm_id == -1) {
        perror("Error creating shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Attach the shared memory segment */
    shared_products = (Product *) shmat(product_shm_id, NULL, 0);
    if (shared_products == (void *) -1) {
        perror("Error attaching shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Create a shared memory segment */
    customers_shm_id = shmget(CUSTOMERS_KEY, sizeof(Customer) * MAX_CUSTOMERS, IPC_CREAT | 0666);
    if (customers_shm_id == -1) {
        perror("Error creating customers shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Attach the shared memory segment */
    shared_customers = (Customer *) shmat(customers_shm_id, NULL, 0);
    if (shared_customers == (void *) -1) {
        perror("Error attaching customers shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Set the initial customer id to -1 */
    for (int i = 0; i < MAX_CUSTOMERS; ++i) {
        shared_customers[i].id = -1;
    }

    shelving_shm_id = shmget(SHELVING_KEY, sizeof(ShelvingTeam) * nShelvingTeams, IPC_CREAT | 0666);
    if (shelving_shm_id == -1) {
        perror("Error creating shelving team shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Attach the shared memory segment */
    shared_shelvingTeams = (ShelvingTeam *) shmat(shelving_shm_id, NULL, 0);
    if (shared_shelvingTeams == (void *) -1) {
        perror("Error attaching shelving team shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Set the initial shelving id to -1 */
    for (int i = 0; i < nShelvingTeams; ++i) {
        shared_shelvingTeams[i].id = -1;
    }
}

void createSemaphoresForProducts() {
    /* Create semaphore for available items */
    printf("pid: %d, products n: %d", getpid(), num_of_products);
    items_sem_id = semget(getpid(), num_of_products, IPC_CREAT | 0666);
    if (items_sem_id == -1) {
        perror("Error creating semaphores in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Initialize each semaphore to its item */
    for (int i = 0; i < num_of_products; ++i) {
        semctl(items_sem_id, i, SETVAL, 1);
    }
}

void createMsgQueue() {
    /* Create message queue */
    message_queue_id = msgget(getpid(), IPC_CREAT | 0666);
    if (message_queue_id == -1) {
        perror("Error creating message queue in thr parent process");
        exit(EXIT_FAILURE);
    }
}

void readProducts() {
    /* Read from the items_list file */
    FILE *file = fopen("Products.txt", "r");
    if (file == NULL) {
        perror("Error opening items_list.txt file");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    products_count = 0;

    /* Read data from the file line by line using fgets */
    while (fgets(buffer, sizeof(buffer), file)) {

        /* split each line using ',' as the delimiter */
        char *token = strtok(buffer, ",");

        /* Each line split to (name, quantity, price) */
        for (int i = 0; i < 2; ++i) {

            /* Save the items name to the items shared memory */
            if (i == 0) {
                shared_products[products_count].name[0] = '\0';
                strncat(shared_products[products_count].name, token,
                        sizeof(shared_products[products_count].name) - strlen(shared_products[products_count].name) - 1);
                shared_products[products_count].name[sizeof(shared_products[products_count].name) - 1] = '\0';
            } else {
                /* Save the items quantity to the items shared memory */
                int all_quantity = atoi(token);
                if (all_quantity <= num_of_product_on_shelves) {
                    shared_products[products_count].quantity_on_shelves = all_quantity;
                    shared_products[products_count].quantity_in_storage = 0;
                } else {
                    shared_products[products_count].quantity_on_shelves = num_of_product_on_shelves;
                    shared_products[products_count].quantity_in_storage = all_quantity - num_of_product_on_shelves;
                }
            }
            shared_products[products_count].is_claimed = 0;
            token = strtok(NULL, ","); /* Move to the next token */
        }
        products_count++;
        if (products_count >= num_of_products) { /* Break out of the loop if the maximum size is reached */
            break;
        }
    }
    /* Set the last_item_flag of the last item to 1 (indicating it's the last item) */
   // shared_Products[Products_count - 1].last_item_flag = 1;
    fclose(file); /* Close the file */
//    for (int i = 0; i < products_count; ++i) {
//        printf("name %s\n on shelves: %d\n on storage: %d\n", shared_products[i].name, shared_products[i].quantity_on_shelves, shared_products[i].quantity_in_storage);
//    }
}

void generateShelvingTeams() {
    char num_of_product_str[20];
    char product_threshold_str[20];
    char simulation_threshold_str[20];
    char num_of_product_on_shelves_str[20];

    /* Convert integer values to strings */
    sprintf(num_of_product_str, "%d", num_of_products);
    sprintf(product_threshold_str, "%d", product_threshold);
    sprintf(simulation_threshold_str, "%d", simulation_threshold);
    sprintf(num_of_product_on_shelves_str, "%d", num_of_product_on_shelves);

    srand(time(NULL) % getpid());
    for (int i = 0; i < nShelvingTeams; i++) {
        pid_t shelving_pid = fork(); /* Fork a child process */
        if (shelving_pid == -1) {
            perror("Error forking cashier process"); /* Error while forking */
            exit(EXIT_FAILURE);
        } else if (shelving_pid == 0) {
            /* Execute the customer process with command-line arguments */
            execlp("./ShelvingTeam", "./ShelvingTeam",num_of_product_str,product_threshold_str,simulation_threshold_str,num_of_product_on_shelves_str, (char *) NULL);

            /* If execlp fails */
            perror("Error executing customer process");
            exit(EXIT_FAILURE);

        }
//        } else {
//            /* Parent process */
//            childProcesses[childCounter++] = cash_pid;
//        }
    }
}

void generateCustomers() {

    char num_of_product_str[20];

    /* Convert integer values to strings */
    sprintf(num_of_product_str, "%d", num_of_products);

    sleep(1);
    pid_t pid = fork(); /* Fork a new Customer Process */

    if (pid == -1) {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {

        /* Execute the customer process with command-line arguments */
        execlp("./customer", "./customer",num_of_product_str, (char *) NULL);

        /* If execlp fails */
        perror("Error executing customer process");
        exit(EXIT_FAILURE);    }
//    } else {
//        /* Parent process */
//        childProcesses[childCounter++] = pid;/* Add the child process ID to the array*/
//    }

}

void *productsCheck(void *arg) {
    int out_of_stock;
    while (1) {
        out_of_stock = 0;
        for (int i = 0; i < num_of_products; ++i) {

            /* check if the quantity is zero */
            if(shared_products[i].quantity_in_storage == 0) {
                out_of_stock++;
                continue;
            }

            /* check if the quantity is below the threshold */
            if (shared_products[i].quantity_on_shelves <= product_threshold) {
                printf("Product %s reached or fell below the threshold! -> %d\n", shared_products[i].name, shared_products[i].quantity_on_shelves);
                // send a message to a random team about this product
            }

        }

        /* Sleep for a period before checking again */
        sleep(1);

        if (out_of_stock == products_count) {
            break;
        }
    }
    return NULL;
}



void cleanup() {
    /* Detach All shared memory and Semaphores created */
    shmdt(shared_customers);
    shmdt(shared_products);
    shmctl(product_shm_id, IPC_RMID, (struct shmid_ds *) 0);
    semctl(items_sem_id, 0, IPC_RMID);
    msgctl(message_queue_id, IPC_RMID, (struct msqid_ds *) 0);
    printf("Main died\n");
}
