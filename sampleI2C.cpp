#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int main()
{
	int fd;
	uint8_t slaveAddress = 0x18;
	uint8_t registerAddress = 0x0F;
	uint8_t readData;

	/* 1. デバイスファイルを開いてアクセスできるようにする */
	fd = open("/dev/i2c-1", O_RDWR);

	/* 2. スレーブアドレスの設定 */
	ioctl(fd, I2C_SLAVE, slaveAddress);

	/* 3. "0x0F"を送信してみる。(たいていの場合、これはデバイス内のレジスタアドレスを指定する) */
	write(fd, &registerAddress, 1);

	/* 4. 1Byte Readしてみる。(たいていの場合、先ほど指定したアドレス(0x0F)の値が読み出される) */
	read(fd, &readData, 1);
	printf("%02X\n", readData);

	/* 4. 使い終わったので閉じる */
	close(fd);
}
