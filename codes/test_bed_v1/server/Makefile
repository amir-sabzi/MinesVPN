CC = gcc

default: server

server: server.o
	$(CC) -o server server.o

server.o: tcp_server.c
	$(CC) -c tcp_server.c -o server.o

clean:
	$(RM) server *.o *~
