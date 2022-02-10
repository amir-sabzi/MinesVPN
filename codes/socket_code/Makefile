

CC = gcc
# CFLAGS = 

default: simple_ts

simple_ts: simple_ts.o
	$(CC) -o simple_ts simple_ts.o

simple_ts.o: socket_timestamping.c
	$(CC) -c socket_timestamping.c -o simple_ts.o

clean: 
	$(RM) simple_ts *.o *~
