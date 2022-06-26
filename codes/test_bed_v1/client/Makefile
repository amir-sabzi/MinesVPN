CC = gcc

default: client

client: client.o
	$(CC) -o client client.o

client.o: tcp_client.c
	$(CC) -c tcp_client.c -o client.o

clean:
	$(RM) client *.o *~
