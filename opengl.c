#include "local.h"
#define PI 3.1415926535898

int shm_id ,Products_count, nShelvingTeams = 5,customer_shm_id,shelving_shm_id,items_sem_id,shm_id_for_shelvingTeam;
Product *shared_products;
ShelvingTeam *sharedMemory_shelvingteam;
Customer *customers_shared_memory;

void getSharedMemorys() {
    /* Create a shared memory segment */
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

    shm_id_for_shelvingTeam = shmget((int) SHELVING_KEY, 0, 0);
    if (shm_id_for_shelvingTeam == -1) {
        perror("Error accessing shared memory in customer.c");
        exit(EXIT_FAILURE);
    }
    sharedMemory_shelvingteam = (ShelvingTeam *) shmat((int) shm_id_for_shelvingTeam, NULL, 0);
    if (sharedMemory_shelvingteam == (void *) -1) {
        perror("Error attaching shared memory in customer.c");
        exit(EXIT_FAILURE);
    }
}
void drawText(float x, float y, const char *text) {
    glColor3f(0.0, 0.0, 0.0); // Set text color to black

    // Set the position for the text (centered)
    float textX = x - glutBitmapLength(GLUT_BITMAP_8_BY_13, (const unsigned char *)text) / 2;
    float textY = y;

    glRasterPos2f(textX, textY);

    // Display each character of the string
    for (int i = 0; text[i] != '\0'; i++) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, text[i]);
    }
}

void drawCustomers() {

    // Assuming you have information about the number of customers and their positions
    int numCustomers = 5; // Replace with the actual number of customers
    float spacing = 70.0; // Adjust the spacing between customers

    for (int i = 0; i < numCustomers; i++) {
        float x = i * spacing + 100; // Adjust the starting position
        float y = 500; // Adjust the y-coordinate
        glColor3f(1.0, 1.0, 1.0);

        // Draw a square for each customer
        glBegin(GL_QUADS);
        glVertex2f(x - 25, y - 25); // Top left
        glVertex2f(x + 25, y - 25); // Top right
        glVertex2f(x + 25, y + 25); // Bottom right
        glVertex2f(x - 25, y + 25); // Bottom left
        glEnd();

        glColor3f(0.0f, 0.0f, 0.0f); // Black text
        glRasterPos2f(x - 12.0f, y );

        char CustomerId[6]; /* Convert ID to string */
        sprintf(CustomerId, "%d", customers_shared_memory[i].id);
        for (int i = 0; CustomerId[i] != '\0'; ++i) {
            /* Draw a character using the GLUT_BITMAP_HELVETICA_12 font at the current raster position */
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, CustomerId[i]);
        }
    }

}


void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw customers
    drawCustomers();

    glFlush();
}

void reshape(int width, int height) {
    glViewport(0,0,(GLsizei)width,(GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1500.0, -300.0, 600.0);
    glMatrixMode(GL_MODELVIEW);
}


int main(int argc, char *argv[]) {
    getSharedMemorys();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(1500, 1000);
    glutInitWindowPosition(10, 10);
    glutCreateWindow("Circle Example");

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);

    glutMainLoop();

    return 0;
}