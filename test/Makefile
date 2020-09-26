
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -lrt -lpthread

all: proj2

proj2: proj2.o 
	gcc $(CFLAGS) $(LDFLAGS) -o proj2 proj2.o

proj2.o: proj2.c
	gcc $(CFLAGS) $(LDFLAGS) -c proj2.c -o proj2.o

clean:
	rm proj2.o proj2