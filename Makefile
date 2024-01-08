CC = gcc -g
CFLAGS = -Wall -Wextra
LIBS = -lGL -lGLU -lglut -lm

all: main customer ShelvingTeam

main: main.c
	$(CC) $(CFLAGS) -o main main.c $(LIBS)

customer: customer.c
	$(CC) $(CFLAGS) -o customer customer.c

cashier: ShelvingTeam.c
	$(CC) $(CFLAGS) -o ShelvingTeam ShelvingTeam.c

clean:
	rm -f main customer ShelvingTeam