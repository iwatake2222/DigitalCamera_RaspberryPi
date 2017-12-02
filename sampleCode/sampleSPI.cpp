#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

int main()
{
	int fd;
	uint16_t spiMode     = SPI_MODE_0;
	uint8_t  bitsPerWord = 8;
	uint32_t spiSpeed    = 125.0 / 2 * 1000 * 1000;

	/* 1. デバイスファイルを開いてアクセスできるようにする */
	fd = open("/dev/spidev0.0", O_RDWR);

	/* 2. SPI通信設定 */
	ioctl(fd, SPI_IOC_WR_MODE, &spiMode);
	ioctl(fd, SPI_IOC_RD_MODE, &spiMode);
	ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bitsPerWord);
	ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bitsPerWord);
	ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spiSpeed);
	ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &spiSpeed);

	/* 3. 2バイト(0xAA, 0xBB)を送受信してみる */
	uint8_t tx[2] = {0xAA, 0xBB};
	uint8_t rx[2];
	struct spi_ioc_transfer tr = {0};
	tr.tx_buf = (uint64_t)tx;
	tr.rx_buf = (uint64_t)rx;
	tr.len = 2;
	tr.speed_hz = spiSpeed;
	tr.delay_usecs = 0;
	tr.bits_per_word = bitsPerWord;
	ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

	/* 4. 使い終わったので閉じる */
	close(fd);
}

