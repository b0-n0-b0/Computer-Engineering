CFLAGS=-pie -g -z noexecstack -fstack-protector -z relro -z now -Wno-stringop-overflow -Wno-format-security
all: server

server: server.c
	gcc $(CFLAGS) -o server server.c

.PHONY: clean
clean:
	rm -f server
