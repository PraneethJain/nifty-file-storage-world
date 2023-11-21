CC = gcc
CFLAGS = -Wall -Wextra -Werror

all:
	$(CC) $(CFLAGS) -o client.out client/main.c common/network.c
	$(CC) $(CFLAGS) -o storage_server.out storage_server/main.c storage_server/ss_to_nm.c storage_server/ss_to_client.c common/network.c common/tree.c
	$(CC) $(CFLAGS) -o naming_server.out naming_server/main.c naming_server/nm_to_ss.c naming_server/nm_to_client.c common/network.c common/tree.c
	
clean:
	rm *.out *.log
