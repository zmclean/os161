#include "stdio.h"
#include "unistd.h"

int main() {
	pid_t pid = fork();
	printf("Fork PID: %d\n", pid);

	pid = getpid();
	printf("getpid PID: %d\n", pid);
	
	int x;
	
	pid = waitpid(pid, &x, 0);
	printf("waitpid PID: %d\n", pid);
	return 0;
}
