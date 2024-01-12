CC = gcc -g
CFLAGS = -Wall -Wextra
LIBS = -lGL -lGLU -lglut -lm

all: main customer shelving_team

main: main.c
	$(CC) $(CFLAGS) -o main main.c $(LIBS)

customer: customer.c
	$(CC) $(CFLAGS) -o customer customer.c

ShelvingTeam: shelving_team.c
	$(CC) $(CFLAGS) -o shelving_team shelving_team.c

clean:
	rm -f main customer shelving_team