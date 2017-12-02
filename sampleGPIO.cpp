#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

int main()
{
	int fd;

	/* 1. GPIO26を使用可能にする */
	fd = open("/sys/class/gpio/export", O_WRONLY);
	write(fd, "26", 2);
	close(fd);

	/* 2. GPIO26を出力設定する */
	fd = open("/sys/class/gpio/gpio26/direction", O_WRONLY);
	write(fd, "out", 3);
	close(fd);

	/* 3. GPIO26に1(High)を出力する */
	fd = open("/sys/class/gpio/gpio26/value", O_WRONLY);
	write(fd, "1", 1);

	/* 4. 使い終わったので閉じる */
	close(fd);
}

