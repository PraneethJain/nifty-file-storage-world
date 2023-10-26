CC = gcc
CFLAGS = -Wall -Wextra -Werror

all:
	$(CC) $(CFLAGS) -o client.out client/main.c
	$(CC) $(CFLAGS) -o storage_server.out storage_server/main.c
	$(CC) $(CFLAGS) -o naming_server.out naming_server/main.c
	
clean:
	rm *.out