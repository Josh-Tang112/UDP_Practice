CC=gcc
CFLAGS=-Wall -Wextra -Iincludes
LDLIBS=-lm
VPATH=src

all: server client

server: server.o optparser.o 

client: client.o optparser.o

clean:
	rm -f *~ *.o client server
