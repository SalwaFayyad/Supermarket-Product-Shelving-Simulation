## Supermarket Product Shelving Simulation

The Supermarket Product Shelving Simulation project aims to create a dynamic simulation of a supermarket environment, where employees are responsible for restocking products on shelves while customers arrive and shop. The system operates as follows:

- The supermarket sells a user-defined number of products, with a specified initial amount placed on the shelves, and the remaining kept in storage.
- Shelving teams, each consisting of a team manager and a user-defined number of employees, are employed to restock shelves.
- When the amount of a product on the shelves drops below a threshold, a team manager retrieves the necessary amount from storage and places it on a rolling cart. The employees then restock the shelves from this cart.
- Each shelving team can only work on one product item at a time, and they cannot simultaneously restock multiple items.
- Customers arrive randomly, with their arrival rate falling within a user-defined range. They select random items and quantities to purchase, with out-of-stock items unavailable for selection.
- The simulation continues until either the storage area runs out of stock or a user-defined time limit is reached.

## Implementation Details:
The project will be implemented on Linux machines using a combined multi-processing and multi-threading approach. Customers will be represented as individual processes, while shelving teams will also be processes. Each shelving team will consist of a team manager and team employees, with these roles implemented as threads within the shelving process.
## Dependencies
- UNIX/Linux Operating System
- GCC Compiler
- POSIX Threads Library (pthread)
- OpenGL Utility Toolkit (GLUT): For the graphical user interface.
- OpenGL: Required for rendering in the GUI.
- Standard Math Library (typically included in standard development environments)
## Objective:
The main objective of this project is to create an interactive and realistic simulation of a supermarket environment, demonstrating the interplay between product shelving activities and customer shopping behavior. By leveraging multi-processing and multi-threading techniques, the simulation aims to accurately replicate real-world scenarios and provide insights into optimizing supermarket operations and customer satisfaction.

 ![image](https://github.com/saharfayyad02/realtime_project2/assets/104863637/73df172c-3d25-4c52-b048-4eca8b68f5dc)
 ![image](https://github.com/saharfayyad02/realtime_project2/assets/104863637/1ca833f6-3e5d-4fb2-92d8-97f47f153e24)

