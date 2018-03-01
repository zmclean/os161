#include "stdio.h"
#include "unistd.h"
int main() {
	time_t seconds;
	__time(&seconds, NULL);

	int minutes = ((int)seconds % 3600) / 60;
	int hours = (((int)seconds % 86400) / 3600) - 8;
	seconds %= 60;
	printf("Hello World\nTime: %d:%d:%d\n", hours, minutes, (int)seconds);
	return 0;
}
