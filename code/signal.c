#include <stdio.h>
#include <signal.h>
#include <unistd.h>


void sig_callback(int signum) {
	switch (signum) {
		case SIGINT:
			printf("Get signal SIGINT\n");
			break;
		case SIGTERM:
			printf("Get signal SIGTERM\n");
			break;
		default:
			printf("Unknown signal %d\n", signum);
			break;
	}
}

int main(int argc, char **argv) {
	printf("Register SIGINT(%u) and SIGTERM(%u) Signal Action\n", SIGINT, SIGTERM);

	signal(SIGINT, sig_callback);
	signal(SIGTERM, sig_callback);

	printf("Waiting for Signal...\n");

	for(;;)
		;

	return 0;
}
