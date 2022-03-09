

CC = gcc
# CFLAGS = 

default: simple_ts

simple_ts: simple_ts.o
	$(CC) -o simple_ts simple_ts.o -lm

simple_ts.o: socket_timestamping_opt.c
	$(CC) -c socket_timestamping_opt.c -o simple_ts.o

clean: 
	$(RM) simple_ts *.o *~
