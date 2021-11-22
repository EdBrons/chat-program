CFLAGS=-Wall -pedantic -g

all: chat-client chat-server

chat-client: chat-client.o utils.o
	gcc -lpthread -o chat-client chat-client.c utils.o

chat-server: chat-server.o utils.o
	gcc -lpthread -o chat-server chat-server.c utils.o

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -f chat-client chat-server chat-client.o chat-server.o utils.o
