#include <stdio.h>
#include "local.h"

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

void *productsCheck(void *arg);

void cleanup();

void generateCustomers();

/* PUT THE SHELVING TEAM IN SHARED MEMORY*/

int product_shm_id, products_count, nShelvingTeams = SHELVING_TEAMS_NUMBER, customers_shm_id, shelving_shm_id, items_sem_id, message_queue_id;
Product *shared_products;
ShelvingTeam *shared_shelvingTeams;
Customer *shared_customers;

pthread_t products_check_thread;

int product_index;
int random_team_index;
void drawCustomers(int i) {

    // Assuming you have information about the number of customers and their positions
   // int numCustomers = 5; // Replace with the actual number of customers
    float spacing = 70.0; // Adjust the spacing between customers
    float x = i * spacing + 100; // Adjust the starting position
    float y = 950; // Adjust the y-coordinate
    glColor3f(1.0, 1.0, 1.0);

    // Draw a square for each customer
    glBegin(GL_QUADS);
    glVertex2f(x - 25, y - 25); // Top left
    glVertex2f(x + 25, y - 25); // Top right
    glVertex2f(x + 25, y + 25); // Bottom right
    glVertex2f(x - 25, y + 25); // Bottom left
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f); // Black text
    glRasterPos2f(x - 12.0f, y);

    char CustomerId[6]; /* Convert ID to string */
    sprintf(CustomerId, "%d", shared_customers[i].id);
    for (int i = 0; CustomerId[i] != '\0'; ++i) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, CustomerId[i]);
    }

}

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

    glColor3f(0.0f, 0.0f, 0.0f);

    glRasterPos2f(x - size / 2 + 10, y + size / 2 - 20);
    /* Draw text with quantity and price */
    char quantityStr[4]; /* Convert quantity to string */
    sprintf(quantityStr, "%d", quantity);
    for (int i = 0; quantityStr[i] != '\0'; ++i) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, quantityStr[i]);
    }
    glColor3f(0.0f, 0.0f, 0.0f);

    glRasterPos2f(x - size / 2 , y + size / 2 - 40);
    /* Draw text with quantity and price */
    char *nameStr = name; /* Convert quantity to string */
    while (*nameStr) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *nameStr);
        nameStr++;
    }
}

float spacing = 50.0f;
float squareSize = 55.0f;

void drawProducts() {
    int itemsPerRow = (int)floor((1500.0f - spacing) / (spacing * 2 + 50.0f)) - 4;  // Calculate items per row based on window width
    int totalRows = (int)ceil((float)num_of_products / (float)itemsPerRow);
    float yOffset = 1500.0f - (totalRows * 50.0f);  // Start from the bottom of the window, assuming a window height of 1000

    glColor3f(1.0f, 0.0f, 0.0f);

    glRasterPos2f(180 , yOffset - 250);
    /* Draw text with quantity and price */
    char *QuantityonShelvesStr = "Quantity on Shelves"; /* Convert quantity to string */
    while (*QuantityonShelvesStr) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *QuantityonShelvesStr);
        QuantityonShelvesStr++;
    }

    glColor3f(1.0f, 0.0f, 0.0f);

    glRasterPos2f(1150 , yOffset - 250);
    /* Draw text with quantity and price */
    char *QuantityonStorgeStr = "Quantity on Storage"; /* Convert quantity to string */
    while (*QuantityonStorgeStr) {
        /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *QuantityonStorgeStr);
        QuantityonStorgeStr++;
    }

    for (int i = 0; i < totalRows; ++i) {
        for (int j = 0; j < itemsPerRow && (i * itemsPerRow + j) < num_of_products; ++j) {
            float x = j * (spacing * 2 + 50.0f);
            x = ((j + 0.5f) * (squareSize + spacing)) - 1.0f + spacing;
            yOffset = ((totalRows - 2 - i) * (squareSize + spacing)) - 1.0f + spacing;

            shared_products[i * itemsPerRow + j].x_position_on_shelves = x;
            shared_products[i * itemsPerRow + j].y_position_on_shelves = yOffset + 100;
            shared_products[i * itemsPerRow + j].x_position_on_storage = x + 900;
            shared_products[i * itemsPerRow + j].y_position_on_storage = yOffset + 100;
//            printf("shared_products[i * itemsPerRow + j].x_position_on_storage %f\n",shared_products[i * itemsPerRow + j].x_position_on_storage);
//            printf("shared_products[i * itemsPerRow + j].y_position_on_storage %f\n",shared_products[i * itemsPerRow + j].y_position_on_storage);

            drawProductSquare(x, yOffset + 100, squareSize, shared_products[i * itemsPerRow + j].quantity_on_shelves, shared_products[i * itemsPerRow + j].name);
            drawProductSquare(x + 900, yOffset + 100, squareSize, shared_products[i * itemsPerRow + j].quantity_in_storage, shared_products[i * itemsPerRow + j].name);

        }
    }
}

void drawManager(int x, int y, float color) {
    glColor3f(0.0f, color, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y + 15); // Top vertex
    glVertex2f(x - 15, y - 15); // Bottom left vertex
    glVertex2f(x + 15, y - 15); // Bottom right vertex
    glEnd();
}

void drawEmployee(int x, int y, float colorR, float colorG, float colorB) {
    glColor3f(colorR, colorG, colorB); // Set the color based on parameters
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y); // Center of the circle

    int numSegments = 50;
    float radius = 10.0f;

    for (int i = 0; i <= numSegments; i++) {
        float theta = (float)i / (float)numSegments * 2.0f * 3.1415926535898f;
        float ex = x + radius * cos(theta);
        float ey = y + radius * sin(theta);
        glVertex2f(ex, ey);
    }

    glEnd();
}

void displayTeams(float managerX,float managerY) {
    // glClear(GL_COLOR_BUFFER_BIT);

    for (int j = 0; j < nShelvingTeams; j++) {
        // Define different colors for each team
        float teamColorR = (j % 2 == 0) ? 0.0f : 1.0f;      // Alternate red and white
        float teamColorG = (j % 2 == 0) ? 0.0f : 1.0f;
        float teamColorB = (j % 2 == 0) ? 1.0f : 1.0f;
        // Draw employees in a circle around the manager
        int numEmployees = MAX_SHELVES_EMPLOYEES - 1;
//        float managerX = 700.0f;
//        shared_shelvingTeams[j].x_position =  managerX;
//        //  printf(" shared_shelvingTeams[j].x_position %f\n", shared_shelvingTeams[j].x_position);
//        float managerY = 900 - (j * 200);
//        shared_shelvingTeams[j].y_position =  managerY;
//        // Draw the manager in the center

        if(shared_shelvingTeams[random_team_index].manager_status == 1 && j == random_team_index){
            drawManager(shared_shelvingTeams[random_team_index].x_position, shared_shelvingTeams[random_team_index].y_position,1.0f);
            drawManager(managerX, managerY,0.0f);
        }else if(shared_shelvingTeams[random_team_index].manager_status == 1 && j == random_team_index){
            drawManager(shared_shelvingTeams[random_team_index].x_position, shared_shelvingTeams[random_team_index].y_position,0.0f);
            drawManager(managerX, managerY,1.0f);

        }
        else {
            drawManager(shared_shelvingTeams[j].x_position, shared_shelvingTeams[j].y_position, 1.0f);
        }

        float circleRadius = 50.0f;

        for (int i = 0; i < numEmployees; ++i) {
            // Calculate employee position using polar coordinates
            float angle = i * (2.0f * 3.1415926535898f) / numEmployees;
            float employeeX = managerX + circleRadius * cos(angle);
            float employeeY = managerY + circleRadius * sin(angle);

            // Draw the employee at the calculated position with the team's color
            drawEmployee(employeeX, employeeY, teamColorR, teamColorG, teamColorB);
        }
    }
    // glutSwapBuffers();
}

float current_time = 0.0f;
int j = 0;

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawProducts();
    float managerX;
    float managerY [10];
    for(int j = 0 ; j < nShelvingTeams ;j++) {
         managerX = 700.0f;
        shared_shelvingTeams[j].x_position = managerX;
        //  printf(" shared_shelvingTeams[j].x_position %f\n", shared_shelvingTeams[j].x_position);
         managerY[j] = 900 - (j * 200);
        shared_shelvingTeams[j].y_position = managerY[j];
    }
    while(1){
        drawProducts();
        for(int j = 0 ; j < nShelvingTeams ;j++) {
            displayTeams(managerX,managerY[j]);
        }
        float delay = 5.0f;
        for(int i = 0 ; i < j ;i++){
            drawCustomers(i);
        }
        if(current_time >= delay && 7 != j){
            generateCustomers();
            current_time = 0.0f;
            sleep(1);
            j++;
        }

       // for(int i = 0 ; i < nShelvingTeams ;i++){
            if(shared_shelvingTeams[random_team_index].manager_status == 1){
             //   printf("inside the if \n");
                shared_shelvingTeams[random_team_index].x_position = shared_products[product_index].x_position_on_storage;
                shared_shelvingTeams[random_team_index].y_position = shared_products[product_index].y_position_on_storage - 50.0f;
            }else if(shared_shelvingTeams[random_team_index].manager_status == 2){
                shared_shelvingTeams[random_team_index].x_position = managerX;
                shared_shelvingTeams[random_team_index].y_position = managerY[product_index];

            }
       // }

        current_time += 1.0f / 60.0f;
        glutSwapBuffers();
        usleep(16667);
    }
}

void timer(int) {
    glutPostRedisplay();
    glutTimerFunc(1000 / 60, timer, 0);
//    for(int i = 0 ; i < nShelvingTeams ;i++){
//        if(shared_shelvingTeams[i].manager_status == 1){
//            printf("inside the timer to check\n");
//            shared_shelvingTeams[i].x_position = shared_products[product_index].x_position_on_storage;
//            printf("shared_shelvingTeams[i].x_position %f ,shared_products[product_index].x_position_on_storage %f\n",shared_shelvingTeams[i].x_position,shared_products[product_index].x_position_on_storage);
//            shared_shelvingTeams[i].y_position = shared_products[product_index].y_position_on_storage - 2.0;
//        }
//    }
}

void reshape(int width, int height) {
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1500.0, 0.0, 1000.0);
    // Adjust orthographic projection based on window size
    glMatrixMode(GL_MODELVIEW);
}


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

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1500, 1000);
    glutInitWindowPosition(10, 10);
    glutCreateWindow("Items Display");
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();

//    generateShelvingTeams();
//



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



//    for (int i = 0; i < 5; i++) {
//        generateCustomers();
//        sleep(5);
//    }



//    for (int  i = 0 ;i < Products_count ;i++){
//        printf("name %s\n",shared_Products[i].name);
//        printf("quantity_on_shelves %d\n",shared_Products[i].quantity_on_shelves);
//        printf("quantity_in_storage %d\n",shared_Products[i].quantity_in_storage);
//        printf("threshold %d\n",shared_Products[i].threshold);
//    }
    while (1) {
        sleep(5);
        printf("-------------------------------------------------\n");
        for (int i = 0; i < num_of_products; ++i) {
            printf("%s shelve %d storage %d\n", shared_products[i].name, shared_products[i].quantity_on_shelves,
                   shared_products[i].quantity_in_storage);
        }
        printf("-------------------------------------------------\n");
    }
    sleep(60);
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
            execlp("./ShelvingTeam", "./ShelvingTeam", num_of_product_str, product_threshold_str,
                   simulation_threshold_str, num_of_product_on_shelves_str, (char *) NULL);

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
    }
    else {
        sleep(2);
        /* Parent process */
        // childProcesses[childCounter++] = pid;/* Add the child process ID to the array*/
    }

}

void *productsCheck(void *arg) {
    int out_of_stock;
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
                product_index = i;
                /* Send the message */
                int mq_key = msg.receiver_id;
                int msg_queue_id = msgget(mq_key, 0666 | IPC_CREAT);
                if (msgsnd(msg_queue_id, &msg, sizeof(msg), 0) == -1) {
                    perror("Error sending refill shelves message in main.c");
                    exit(EXIT_FAILURE);
                }
                printf("\n\nmessage sent to %d\n\n", msg.receiver_id);
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