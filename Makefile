CFLAGS=-Wall -pedantic -g

all: chat-client chat-server

chat-client: chat-client.o
	gcc -o chat-client chat-client.c

chat-server: chat-server.o
	gcc -lpthread -o chat-server chat-server.c

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -f chat-client chat-server chat-client.o chat-server.o
