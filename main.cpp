#include <stdio.h>
#include "common.h"
#include "app.h"

int main()
{
	/* to ensure log file is written by redirect */
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	printf("Hello World\n");

	App *app = new App();

	app->run();

	/* program never reaches here */
	delete app;

	return 0;
}
