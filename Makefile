# Makefile for CSN6214 Assignment
CC = gcc
CFLAGS = -pthread -Wall

all: server client

server: server.c game.h
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client game.log scores.txt