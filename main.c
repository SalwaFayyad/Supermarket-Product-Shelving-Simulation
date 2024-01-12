#include "local.h"
/*
 * Sahar Fayyad 1190119
 * Maysam Khatib 1190207
 * Majd Abubaha 1190069
 * Salwa Fayyad 1200430
 * */

/************************************** FUNCTIONS & GLOBAL VARIABLES **********************************************/

int num_of_products;
int num_of_product_on_shelves;
int product_threshold;
int arrival_rate_min;
int arrival_rate_max;
int simulation_threshold;

void readArguments(char *file_name);

void createSharedMemories();

void createSemaphoresForProducts();

void createMsgQueue();

void readProducts();

void generateShelvingTeams();

void *productsCheck();

void *customersGeneration();

void cleanup();

void generateCustomers();

void killChildProcesses();

Product *shared_products;
ShelvingTeam *shared_shelvingTeams;
Customer *shared_customers;

pthread_t products_check_thread, Customer_check_thread;

pid_t childProcesses[1000];

int product_shm_id, products_count, nShelvingTeams = SHELVING_TEAMS_NUMBER, customers_shm_id,
        shelving_shm_id, items_sem_id, message_queue_id, childCounter = 0, random_team_index, storage_finished = 0;

time_t start_time;
/*********************************** FUNCTIONS & GLOBAL VARIABLES of opengl****************************************/

int numCustomers = 0, minutes = 0, elapsed_time = 0;
float space = 50.0f, squareSize = 55.0f, managerX, managerY[10];
char *simulation_end_statement = "";

void drawCustomers();

void drawProductSquare(float x, float y, float size, int quantity, char name[20]);

void drawProducts();

void drawManager(int x, int y, float colorR, float colorG, float colorB);

void drawEmployee(int x, int y, float colorR, float colorG, float colorB);

void displayTeams();

void initializeManager();

void drawTime();

void display();

void timer(int);

void reshape(int width, int height);

void startOpengl();

void terminateProgram();

/*****************************************************************************************************************/

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
    initializeManager();
    glutInit(&argc, argv);

    /* Create the threshold check thread */
    if (pthread_create(&products_check_thread, NULL, productsCheck, NULL) != 0) {
        perror("Error creating products check thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&Customer_check_thread, NULL, customersGeneration, NULL) != 0) {
        perror("Error creating products check thread");
        exit(EXIT_FAILURE);
    }

    startOpengl();
//    killChildProcesses();
//    cleanup();
    return 0;
}

/* function to read arguments file */
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

/* function to create shared memories for products, customers, and teams */
void createSharedMemories() {
    /* Create a shared memory segment for products */
    product_shm_id = shmget(getpid(), sizeof(Product) * MAX_SIZE, IPC_CREAT | 0666);
    if (product_shm_id == -1) {
        perror("Error creating shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Attach the shared memory segment for products */
    shared_products = (Product *) shmat(product_shm_id, NULL, 0);
    if (shared_products == (void *) -1) {
        perror("Error attaching shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Create a shared memory segment for customers */
    customers_shm_id = shmget(CUSTOMERS_KEY, sizeof(Customer) * MAX_CUSTOMERS, IPC_CREAT | 0666);
    if (customers_shm_id == -1) {
        perror("Error creating customers shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Attach the shared memory segment for customers */
    shared_customers = (Customer *) shmat(customers_shm_id, NULL, 0);
    if (shared_customers == (void *) -1) {
        perror("Error attaching customers shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Set the initial customer id to -1 */
    for (int i = 0; i < MAX_CUSTOMERS; ++i) {
        shared_customers[i].id = -1;
    }

    /* Create a shared memory segment for teams */
    shelving_shm_id = shmget(SHELVING_KEY, sizeof(ShelvingTeam) * nShelvingTeams, IPC_CREAT | 0666);
    if (shelving_shm_id == -1) {
        perror("Error creating shelving team shared memory in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Attach the shared memory segment for teams */
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

/* function to create semaphores for product */
void createSemaphoresForProducts() {
    /* Create semaphore for available products */
    items_sem_id = semget(getpid(), num_of_products, IPC_CREAT | 0666);
    if (items_sem_id == -1) {
        perror("Error creating semaphores in the parent process");
        exit(EXIT_FAILURE);
    }

    /* Initialize each semaphore to its product */
    for (int i = 0; i < num_of_products; ++i) {
        semctl(items_sem_id, i, SETVAL, 1);
    }
}

/* function to create massage queue*/
void createMsgQueue() {
    /* Create message queue */
    message_queue_id = msgget(getpid(), IPC_CREAT | 0666);
    if (message_queue_id == -1) {
        perror("Error creating message queue in thr parent process");
        exit(EXIT_FAILURE);
    }
}

/* function to real products file and saved it in the shared memory for products */
void readProducts() {
    /* Read from the products file */
    FILE *file = fopen("products.txt", "r");
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

        /* Each line split to (name, quantity) */
        for (int i = 0; i < 2; ++i) {

            /* Save the product name to the items shared memory */
            if (i == 0) {
                shared_products[products_count].name[0] = '\0';
                strncat(shared_products[products_count].name, token,
                        sizeof(shared_products[products_count].name) - strlen(shared_products[products_count].name) -
                        1);
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
            token = strtok(NULL, ","); /* Move to the next token */
        }
        products_count++;
        if (products_count >= num_of_products) { /* Break out of the loop if the maximum size is reached */
            break;
        }
    }
    /* Set the last_item_flag of the last item to 1 (indicating it's the last item) */
    fclose(file); /* Close the file */

}

/* function to generate each team */
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

            /* Execute the ShelvingTeam process with command-line arguments */
            execlp("./shelving_team", "./shelving_team", num_of_product_str, product_threshold_str,
                   simulation_threshold_str, num_of_product_on_shelves_str, (char *) NULL);

            /* If execlp fails */
            perror("Error executing customer process");
            exit(EXIT_FAILURE);


        } else {
            /* Parent process */
            childProcesses[childCounter++] = shelving_pid;
        }
    }
}

/* function to generate each customer */
void generateCustomers() {

    char num_of_product_str[20];
    /* Convert integer values to strings */
    sprintf(num_of_product_str, "%d", num_of_products);

    /* Fork a new Customer Process */
    sleep(1);
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        /* Execute the customer process with command-line arguments */
        execlp("./customer", "./customer", num_of_product_str, (char *) NULL);
        /* If execlp fails */
        perror("Error executing customer process");
        exit(EXIT_FAILURE);
    } else {
        /* Parent process */
        childProcesses[childCounter++] = pid;/* Add the child process ID to the array*/
    }
}

/* create thread to check if products reach the min threshold value and send message queue to a chosen manager */
void *productsCheck() {
    int out_of_stock;
    int total_time = 0;

    while (1) {
        out_of_stock = 0;
        for (int i = 0; i < num_of_products; ++i) {

            /* Check if the quantity is zero */
            if (shared_products[i].quantity_in_storage == 0) {
                out_of_stock++;
                continue;
            }

            /* Check if the quantity is below the threshold */
            if (shared_products[i].quantity_on_shelves <= product_threshold) {
                printf("Product %s reached or fell below the threshold! -> %d\n", shared_products[i].name,
                       shared_products[i].quantity_on_shelves);

                /* check if team manager is busy */
                int assigned_before = 0;
                for (int j = 0; j < nShelvingTeams; ++j) {
                    if (shared_shelvingTeams[j].current_product_index == i) {
                        assigned_before = 1;
                        break;
                    }
                }

                if (assigned_before == 1) {
                    continue;
                }

                /** Send a message to a random team to refill the shelve **/

                /* Choose the random team and make sure it is available and not doing anything*/
                random_team_index = generateRandomNumber(0, nShelvingTeams - 1);
                while (shared_shelvingTeams[random_team_index].current_product_index != -1) {
                    random_team_index = generateRandomNumber(0, nShelvingTeams - 1);
                }
                /* Creating the message: type 1 -> refill the shelve*/
                Message msg;
                msg.sender_id = getpid();
                msg.receiver_id = shared_shelvingTeams[random_team_index].id;
                msg.type = 1;
                msg.product_index = i;

                /* Send the message */
                int mq_key = msg.receiver_id;
                int msg_queue_id = msgget(mq_key, 0666 | IPC_CREAT);
                if (msgsnd(msg_queue_id, &msg, sizeof(msg), 0) == -1) {
                    perror("Error sending refill shelves message in main.c");
                    exit(EXIT_FAILURE);
                }
                printf("\n\nmessage sent to %d to restock %s\n\n", random_team_index, shared_products[i].name);
                usleep(100000);
            }
        }

        /* Sleep for a period before checking again */
        sleep(1);
        total_time += 5;

        /* check if the storage out of stock */
        if (out_of_stock == products_count) {
            storage_finished = 1;
            break;
        }
    }
    return NULL;
}

/* thread to create customer randomly */
void *customersGeneration() {

    float current_time = 0.0f;

    /* assign initial random number of customers */
    numCustomers = generateRandomNumber(arrival_rate_min, arrival_rate_max);
    int new_random = numCustomers;
    while (1) {
        float delay = 4.0f;

        /* check if the number of customers created less than the max number of customers, and check the interval time */
        if (numCustomers < MAX_CUSTOMERS && current_time >= delay) {
            current_time = 0.0f;

            for (int i = 0; i < new_random; i++) {
                generateCustomers();
            }

            /* create another random number of customers */
            new_random = generateRandomNumber(arrival_rate_min, arrival_rate_max);
            numCustomers += new_random; /* increase the number of created customers */
        }
        current_time += 1.0f / 60.0f;
        usleep(16777);
    }

}

/* Function to kill all active child processes */
void killChildProcesses() {
    for (int i = 0; i < childCounter; ++i) {
        printf("Killing child process with PID: %d\n", childProcesses[i]);
        kill(childProcesses[i], SIGTERM);
    }
}

void cleanup() {
    /* Detach All shared memory and Semaphores created */
    pthread_cancel(products_check_thread);
    pthread_cancel(Customer_check_thread);
    shmdt(shared_customers);
    shmdt(shared_products);
    shmctl(product_shm_id, IPC_RMID, (struct shmid_ds *) 0);
    semctl(items_sem_id, 0, IPC_RMID);
    msgctl(message_queue_id, IPC_RMID, (struct msqid_ds *) 0);
    printf("Main died\n");
}


/**********************************************OPENGL********************************************************/

/* function to draw the time */
void drawTime() {
    glBegin(GL_QUADS); /* Start drawing quads (for the square) */
    glColor3f(0, 0, 0); /* Set the color for the square */

    /* Define the vertices of the square */
    glVertex2f(1400, 960); /* Top-left corner */
    glVertex2f(1400, 930); /* Bottom-left corner */
    glVertex2f(1450, 930); /* Bottom-right corner */
    glVertex2f(1450, 960); /* Top-right corner */

    glEnd(); /* End drawing the square */

    glColor3f(0.1, 0.5, 0.7); /* Set the color for the text */
    glRasterPos2f(1450, 970); /* Adjust position for drawing text within the square */

    int seconds = elapsed_time;
    char secondsStr[5];
    sprintf(secondsStr, "%d", seconds);

    for (int i = 0; secondsStr[i] != '\0'; ++i) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_18 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, secondsStr[i]);
    }

    glColor3f(0.1, 0.5, 0.7);
    glRasterPos2f(1370, 970);
    /* Draw the second word */
    char *secondStr = "SECONDS";
    while (*secondStr) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_24 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *secondStr);
        secondStr++;
    }

    glColor3f(0, 0, 0); /* Set the color for the square */
    glBegin(GL_QUADS);
    /* Define the vertices of the square */
    glVertex2f(1400, 960); /* Top-left corner */
    glVertex2f(1400, 930); /* Bottom-left corner */
    glVertex2f(1450, 930); /* Bottom-right corner */
    glVertex2f(1450, 960); /* Top-right corner */

    glEnd(); /* End drawing the square */
    glColor3f(0.1, 0.5, 0.7); /* Set the color for the square */
    glRasterPos2f(1450, 940); /* Adjust position for drawing text within the square */
    int min = minutes;
    char minStr[5];
    sprintf(minStr, "%d", min);
    for (int i = 0; minStr[i] != '\0'; ++i) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_18 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, minStr[i]);
    }

    /* Draw the minutes word */
    glColor3f(0.1, 0.5, 0.7);
    glRasterPos2f(1370, 940);

    char *minutesStr = "MINUTES";
    while (*minutesStr) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_24 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *minutesStr);
        minutesStr++;
    }
}

/* function to draw the customers */
void drawCustomers() {

    float spacing = 60.0; /* Adjust the space between customers */
    for (int i = 0; i < numCustomers; i++) {
        float x = i * spacing + 30.0; /* Adjust the starting position */
        float y = 980; /* Adjust the y-coordinate */
        if (shared_customers[i].id == -1) {
            glColor3f(0.0, 0.0, 0.0);
        } else {
            glColor3f(0.6, 0.2, 1.0);
        }

        /* Draw a square for each customer */
        glBegin(GL_QUADS);
        glVertex2f(x - 20, y - 20); /* Top left */
        glVertex2f(x + 20, y - 20); /* Top right */
        glVertex2f(x + 20, y + 20); /* Bottom right */
        glVertex2f(x - 20, y + 20); /* Bottom left */
        glEnd();

    }
}

/* function to draw square to represent each square */
void drawProductSquare(float x, float y, float size, int quantity, char name[20]) {

    /* Set drawing color based on quantity */
    if (quantity <= 0) {
        glColor3f(1.0f, 0.0f, 0.0f);
    } else {
        glColor3f(0.5f, 0.5f, 0.5f);
    }

    /* Draw square to represent product*/
    glBegin(GL_QUADS);
    glVertex2f(x - size / 2, y - size / 2);
    glVertex2f(x + size / 2, y - size / 2);
    glVertex2f(x + size / 2, y + size / 2);
    glVertex2f(x - size / 2, y + size / 2);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f); /* set the color */
    glRasterPos2f(x - size / 2 + 10, y + size / 2 - 20); /* set the position */

    /* Draw text with quantity and price */
    char quantityStr[4]; /* Convert quantity to string */
    sprintf(quantityStr, "%d", quantity);
    for (int i = 0; quantityStr[i] != '\0'; ++i) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, quantityStr[i]);
    }
    glColor3f(0.0f, 0.0f, 0.0f);

    glRasterPos2f(x - size / 2, y + size / 2 - 40);
    /* Draw text with quantity */
    char *nameStr = name; /* Convert quantity to string */
    while (*nameStr) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *nameStr);
        nameStr++;
    }
}

/* function used to draw products */
void drawProducts() {
    int itemsPerRow = (int) floor((1500.0f - space) / (space * 2 + 50.0f)) -
                      4;  /* Calculate items per row based on window width */
    int totalRows = (int) ceil((float) num_of_products / (float) itemsPerRow); /* calculate the total number of rows */
    float yOffset =
            1500.0f - (totalRows * 50.0f);  /* Start from the bottom of the window, assuming a window height of 1000 */

    glColor3f(1.0f, 0.0f, 0.0f);

    glRasterPos2f(180, yOffset - 250);
    /* Draw text with quantity */
    char *QuantityonShelvesStr = "Quantity on Shelves"; /* Convert quantity to string */
    while (*QuantityonShelvesStr) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *QuantityonShelvesStr);
        QuantityonShelvesStr++;
    }

    glColor3f(1.0f, 0.0f, 0.0f);
    glRasterPos2f(1150, yOffset - 250);

    /* Draw text with quantity */
    char *QuantityonStorgeStr = "Quantity on Storage"; /* Convert quantity to string */
    while (*QuantityonStorgeStr) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *QuantityonStorgeStr);
        QuantityonStorgeStr++;
    }

    /* draw products in row and columns */
    for (int i = 0; i < totalRows; ++i) {
        for (int j = 0; j < itemsPerRow && (i * itemsPerRow + j) < num_of_products; ++j) {
            float x = j * (space * 2 + 50.0f);
            x = ((j + 0.5f) * (squareSize + space)) - 1.0f + space;
            yOffset = ((totalRows - 2 - i) * (squareSize + space)) - 1.0f + space;

            /* set the positions of product in product shared memory */
            shared_products[i * itemsPerRow + j].x_position_on_shelves = x;
            shared_products[i * itemsPerRow + j].y_position_on_shelves = yOffset + 100;
            shared_products[i * itemsPerRow + j].x_position_on_storage = x + 900;
            shared_products[i * itemsPerRow + j].y_position_on_storage = yOffset + 100;
            drawProductSquare(x, yOffset + 100, squareSize, shared_products[i * itemsPerRow + j].quantity_on_shelves,
                              shared_products[i * itemsPerRow + j].name);

            /* draw each product */
            drawProductSquare(x + 900, yOffset + 100, squareSize,
                              shared_products[i * itemsPerRow + j].quantity_in_storage,
                              shared_products[i * itemsPerRow + j].name);

        }
    }
}

/* function to draw the manager of each team as a triangle, each manager have different color as team color */
void drawManager(int x, int y, float colorR, float colorG, float colorB) {
    glColor3f(colorR, colorG, colorB); /* Set the color based on parameters */
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y + 15); /* Top vertex */
    glVertex2f(x - 15, y - 15); /* Bottom left vertex */
    glVertex2f(x + 15, y - 15); /* Bottom right vertex */
    glEnd();
}

/* draw employees for each team as circle, each employee team has different color */
void drawEmployee(int x, int y, float colorR, float colorG, float colorB) {
    glColor3f(colorR, colorG, colorB); /* Set the color based on parameters */
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y); /* Center of the circle */

    int numSegments = 50;
    float radius = 10.0f;

    /* start draw each employee as circle */
    for (int i = 0; i <= numSegments; i++) {
        float theta = (float) i / (float) numSegments * 2.0f * 3.1415926535898f;
        float ex = x + radius * cos(theta);
        float ey = y + radius * sin(theta);
        glVertex2f(ex, ey);
    }

    glEnd();
}

/* function to draw the teams, employee and manager */
void displayTeams() {

    for (int j = 0; j < nShelvingTeams; j++) {
        /* Define different colors for each team */
        float teamColorR = (j) % nShelvingTeams;
        float teamColorG = (j + 2) % nShelvingTeams;
        float teamColorB = (j + 3) % nShelvingTeams;
        if (nShelvingTeams - 1 == j) {
            teamColorR = 0;
            teamColorG = 0;
            teamColorB = 1;
        }

        int numEmployees = MAX_SHELVES_EMPLOYEES - 1;

        /* draw each manager */
        drawManager(shared_shelvingTeams[j].x_position_manager, shared_shelvingTeams[j].y_position_manager, teamColorR,
                    teamColorG, teamColorB);

        /* set the radius of the circle that employees create around the manager*/
        float circleRadius = 50.0f;

        for (int i = 0; i < numEmployees; ++i) {
            /* Calculate employee position using polar coordinates */
            float angle = i * (2.0f * 3.1415926535898f) / numEmployees;
            float employeeX = shared_shelvingTeams[j].x_position_employee + circleRadius * cos(angle);
            float employeeY = shared_shelvingTeams[j].y_position_employee + circleRadius * sin(angle);

            /* Draw the employee at the calculated position with the team's color */
            drawEmployee(employeeX, employeeY, teamColorR, teamColorG, teamColorB);
        }
    }
}

/* function that initialize the position of manager and employees */
void initializeManager() {

    /* set the initial positions of managers and employees */
    for (int j = 0; j < nShelvingTeams; j++) {
        managerX = 700.0f;
        shared_shelvingTeams[j].x_position_manager = managerX;
        shared_shelvingTeams[j].x_position_employee = managerX;
        managerY[j] = 900 - (j * 200);
        shared_shelvingTeams[j].y_position_manager = managerY[j];
        shared_shelvingTeams[j].y_position_employee = managerY[j];
    }
}

/* function that display the drawing in opengl */
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawTime();
    drawProducts();
    displayTeams();
    drawCustomers();
    glutSwapBuffers();
}

/* function that uses GLUT to schedule periodic updates for an OpenGL program */
void timer(int) {
    time_t current_time = time(NULL); /* check the time */
    elapsed_time = difftime(current_time, start_time); /* calculate the seconds from start the program*/

    if (elapsed_time >= (simulation_threshold * 60.0)) {
        simulation_end_statement = " SIMULATION TIME FINISH ";
    } else {
        simulation_end_statement = " STORAGE FINISH ";
    }
    for (int i = 1; i <= simulation_threshold  * 60 ; i++) {
        if (elapsed_time == i) {
            minutes += 1;
        }
    }
    minutes = elapsed_time / 60;

    /* check the simulation threshold or number of products in storage to finish the program */
    if (elapsed_time >= (simulation_threshold * 60.0) || storage_finished == 1) {
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(0.1, 0.5, 0.7);
        glRasterPos2f(700, 500);

        char *simulation_end_statementStr = simulation_end_statement;
        while (*simulation_end_statementStr) {
            /* Draw a character using the GLUT_BITMAP_HELVETICA_24 font at the current raster position */
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *simulation_end_statementStr);
            simulation_end_statementStr++;
        }
        glFlush();
        sleep(5);
        terminateProgram();
    } else {
        /* check the status of employees, and manager of each team */
        for (int i = 0; i < nShelvingTeams; ++i) {
            if (shared_shelvingTeams[i].manager_status == 1 && shared_shelvingTeams[i].current_product_index != -1) {
                shared_shelvingTeams[i].x_position_manager = shared_products[shared_shelvingTeams[i].current_product_index].x_position_on_storage;
                shared_shelvingTeams[i].y_position_manager = shared_products[shared_shelvingTeams[i].current_product_index].y_position_on_storage;
            } else {
                shared_shelvingTeams[i].x_position_manager = managerX;
                shared_shelvingTeams[i].y_position_manager = managerY[i];
            }
            if (shared_shelvingTeams[i].employee_status == 1 && shared_shelvingTeams[i].current_product_index != -1) {
                shared_shelvingTeams[i].x_position_employee = shared_products[shared_shelvingTeams[i].current_product_index].x_position_on_shelves;
                shared_shelvingTeams[i].y_position_employee = shared_products[shared_shelvingTeams[i].current_product_index].y_position_on_shelves;
            } else {
                shared_shelvingTeams[i].x_position_employee = managerX;
                shared_shelvingTeams[i].y_position_employee = managerY[i];
            }
        }

        glutPostRedisplay(); /* triggers a window redisplay */
        glutTimerFunc(1000 / 60, timer, 0); /* sets up a timer to call itself every 16.67 milliseconds */
    }

}

void reshape(int width, int height) {
    glViewport(0, 0, (GLsizei) width,
               (GLsizei) height); /* Set the viewport of the window to cover the entire window area */
    glMatrixMode(GL_PROJECTION); /* Switch to the projection matrix mode */
    glLoadIdentity(); /* Load the identity matrix into the projection matrix */
    gluOrtho2D(0.0, 1500.0, 0.0, 1000.0); /* Adjust orthographic projection based on window size */
    glMatrixMode(GL_MODELVIEW); /* Switch back to the modelview matrix mode */
}

void startOpengl() {
    start_time = time(NULL); /* used for start calculations of time in the program. */
    /* Set the display mode for the window
    GLUT_SINGLE: Use a single buffer for drawing
    GLUT_RGB: Use the RGB color model for the window */
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1500, 1000); /* Set the initial size of the window */
    glutInitWindowPosition(10, 10); /* Set the initial position of the window on the screen */
    glutCreateWindow("2S2M Supermarket Product Shelving"); /* Create a window with the specified title */
    glutReshapeFunc(reshape); /* Register the reshape function */
    glutDisplayFunc(display); /* Register the display function */
    glutTimerFunc(0, timer, 0); /* Register the timer function */
    glutMainLoop(); /* Enter the GLUT event loop */
}

void terminateProgram() {

    killChildProcesses(); /* End of simulation -> killing all child processes and remove shared memories, semaphores and message queue */
    cleanup();  /* Clean the code and deattached all shared memory and semaphores */
    glutLeaveMainLoop(); /* Register the closeWindow function to be called on window closure */

    exit(EXIT_SUCCESS);
}