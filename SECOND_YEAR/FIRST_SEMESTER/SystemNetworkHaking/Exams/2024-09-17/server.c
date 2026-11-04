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

int printflag(void);

#define CMDSZ 130

unsigned int key = 0;

void login(char *password)
{
	// TODO: check password and then set key
}

void child()
{
	char buf[CMDSZ];

	while (fgets(buf, CMDSZ, stdin)) {
		switch (buf[0]) {
		case '\0':
		case '%': // comment char
			break;
		case 'l':
			login(buf + 1);
			break;
		case 'r':
			if (key != 0xfab4) {
				printf("access denied\n");
			} else {
				printflag();
			}
			break;
		default:
			printf("ignored line:\n");
			printf(buf);
			break;
		}
		fflush(stdout);
	}
}

// NO INTENTIONAL BUGS BELOW THIS POINT
int main()
{
	int lstn;
	int enable;
	struct sockaddr_in lstn_addr;

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

			fflush(stdout);

			close(0);
			dup(con);
			close(1);
			dup(con);
			close(2);
			dup(con);
			close(con);
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
#include <stdio.h>

#define BUFSZ 1024

int printflag()
{
	char buf[BUFSZ], *scan = buf;

	FILE *flag = fopen("flag.txt", "r");
	if (flag == NULL) {
		perror("flag.txt");
		return -1;
	}

	if (fgets(buf, BUFSZ, flag) == NULL) {
		perror("flag.txt");
		return -1;
	}

	printf("Here is the flag:\n");
	while (*scan)	
		printf("%c", *scan++);

	return 0;
}
