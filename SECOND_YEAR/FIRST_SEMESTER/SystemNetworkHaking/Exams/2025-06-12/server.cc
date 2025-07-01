#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#ifndef PORT
#define PORT 10000
#endif

const size_t BUFSZ = 128;
const size_t KEYSZ = 8;

int subscription = 0;

class Base {
protected:
	int i;
public:
	Base(int i_): i(i_) {}
	virtual void bar() {
		printf("bar!\n");
	}
	virtual void foo() {
		printf("Base %d\n", i);
	}
};

class Derived1: public Base {
public:
	Derived1(int i_): Base(i_) {}
	virtual void bar() {
		printf("derived1 bar!\n");
	}
	virtual void foo() {
		printf("Derived1 %d\n", i);
	}
};

class Derived2: public Base {
public:
	Derived2(int i_): Base(i_) {}
	virtual void bar() {
		printf("derived2 bar!\n");
	}
	virtual void foo() {
		printf("Derived2 %d\n", i);
	}
};

char* values[8];
Base *objects[256];
char freekeys = 8;
int nextkey = 0;
void createkey(){

	char *v;
	int n;
	
	freekeys--;
	if (!subscription && freekeys < 0) {
		fprintf(stderr, "subscribe now! only 19.99$/month for unlimited keys!\n");
		return;
	}

	v = new char[KEYSZ];
	if (v == NULL) {
		fprintf(stderr, "out of memory\n");
		return;
	}
	n = read(0, v, KEYSZ);
	if (n <= 0) {
		fprintf(stderr, "Error/EOF while reading value\n");
		return;
	}
	values[nextkey] = v;
	printf("key #%d\n", nextkey);
	nextkey++;
}

// NO INTENTIONAL BUGS BELOW THIS POINT
void useobj(){
	unsigned char key;

	if (read(0, &key, 1) <= 0) {
		fprintf(stderr, "Error/EOF while reading key\n");
		return;
	}

	Base *b = objects[key];

	if (b == NULL) {
		fprintf(stderr, "no such object\n");
		return;
	}
	b->foo();
}

int readnum()
{
	char num[2];
	if (read(0, num, 2) < 2) {
		return -1;
	}
	if (num[0] < '0' || num[0] > '9' || num[1] < '0' || num[1] > '9') {
		return -1;
	}
	return (num[0] - '0') * 10 + (num[1] - '0');
}

void createobj()
{
	char type;
	unsigned char key;

	if (read(0, &key, 1) <= 0) {
		fprintf(stderr, "Error/EOF while reading key\n");
		return;
	}

	if (objects[key]) {
		fprintf(stderr, "key already used\n");
		return;
	}

	if (read(0, &type, 1) < 1) {
		fprintf(stderr, "error reading type\n");
		return;
	}

	int n = readnum();
	if (n < 0) {
		fprintf(stderr, "error reading object data\n");
		return;
	}

	switch (type) {
	case 'b':
		objects[key] = new Base(n);
		break;
	case '1':
		objects[key] = new Derived1(n);
		break;
	case '2':
		objects[key] = new Derived2(n);
		break;
	default:
		fprintf(stderr, "unknown type: %c\n", type);
		break;
	}
	if (objects[key] == NULL) {
		fprintf(stderr, "out of memory\n");
	}
}

extern "C" void printflag();
void child()
{
	char cmd;
	if (subscription)
		printflag();
	while (read(0, &cmd, 1) > 0) {
		if (index("cdou", cmd) == NULL) {
			if (cmd != '\n')
				fprintf(stderr, "Unknown command: '%c'\n", cmd);
			continue;
		}
		if (cmd == 'q')
			break;
		switch (cmd) {
		case 'c':
			createkey();
			break;
		case 'o':
			createobj();
			break;
		case 'u':
			useobj();
			break;
		default:
			break;
		}
	}
}

static void cleanup()
{
	char buf[4096];

	memset(buf, 0, 4096);
}

int main()
{
	int lstn;
	int enable;
	struct sockaddr_in lstn_addr;

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	lstn = socket(AF_INET, SOCK_STREAM, 0);
	if (lstn < 0) {
		perror("socket");
		return 1;
	}
	enable = 1;
	if (setsockopt(lstn, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
		perror("setsockopt");
		return 1;
	}
	bzero(&lstn_addr, sizeof(lstn_addr));

	lstn_addr.sin_family = AF_INET;
	lstn_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	lstn_addr.sin_port = htons(PORT);

	if (bind(lstn, (struct sockaddr *)&lstn_addr, sizeof(lstn_addr)) < 0) {
		perror("bind");
		return 1;
	}

	if (listen(lstn, 10) < 0) {
		perror("listen");
		return 1;
	}
	printf("Listening on port %d\n", PORT);

	signal(SIGCHLD, SIG_IGN);

	for (;;) {
		int con = accept(lstn, NULL, NULL);
		if (con < 0) {
			perror("accept");
			return 1;
		}

		switch (fork()) {
		case -1:
			perror("fork");
			return 1;
		case 0:
			printf("New connection, child %d\n", getpid());

			close(0);
			dup(con);
			close(1);
			dup(con);
			close(2);
			dup(con);
			close(con);
			cleanup();
			child();
			exit(0);
			break;
		default:
			close(con);
			break;
		}
	}
	return 0;
}
