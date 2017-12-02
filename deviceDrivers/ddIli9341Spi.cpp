#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/spi/spidev.h>
#include "utility.h"
#include "ddIli9341Spi.h"

/*
 * spiDevice: "/dev/spidev0.0"
 * gpioPinDc: 26
 */
DdIli9341Spi::DdIli9341Spi(const char* spiDevice, int gpioPinDc)
{
	m_spiMode= SPI_MODE_0;
	m_bitsPerWord = 8;
	m_spiSpeed = 125.0 / 2 * 1000 * 1000;
	sprintf(m_gpioPinDc, "%d", gpioPinDc);

	initializeSPI(spiDevice);
	while(initializeGPIO() != RET_OK) usleep(1000);
	initializeDevice();

	drawRect(0x0000);
	// colorTest();
}

DdIli9341Spi::~DdIli9341Spi()
{
	close(m_fdSpi);

	close(m_fdGpioDc);
	int fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (fd < 0) {
		LOG_E("gpio fd = %d\n", fd);
	}
	if (write(fd, m_gpioPinDc, strlen(m_gpioPinDc)) != (int)strlen(m_gpioPinDc)) {
		LOG_E("gpio unexport\n");
	}
	close(fd);
}

RET DdIli9341Spi::setArea(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
	uint8_t data[4];
	writeCommand(0x2A);
	data[0] = (x0 >> 8) & 0xFF;
	data[1] = x0 & 0xFF;
	data[2] = (x1 >> 8) & 0xFF;
	data[3] = x1 & 0xFF;
	writeData(data, 4);
	writeCommand(0x2B);
	data[0] = (y0 >> 8) & 0xFF;
	data[1] = y0 & 0xFF;
	data[2] = (y1 >> 8) & 0xFF;
	data[3] = y1 & 0xFF;
	writeData(data, 4);
	writeCommand(0x2C);
	return RET_OK;
}

RET DdIli9341Spi::drawRect(uint16_t color, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
	int width = x1 - x0 + 1;
	int height = y1 - y0 + 1;
	uint16_t *buffer = new uint16_t[width*height];
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			buffer[y*width + x] = color;
		}
	}
	drawBuffer(buffer, x0, y0, x1, y1);
	return RET_OK;
}

RET DdIli9341Spi::drawBuffer(uint16_t *buffer, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
	setArea(x0, y0, x1, y1);
	int width = x1 - x0 + 1;
	int height = y1 - y0 + 1;
	for(int y = 0; y < height; y++) {
		writeData((uint8_t*)(buffer+y*width), width * 2);
	}
	return RET_OK;
}

RET DdIli9341Spi::initializeDevice()
{
	writeCommand(0x01);	//software reset
	usleep(10*1000);
	writeCommand(0x11);	//exit sleep
	usleep(10*1000);
	writeCommand(0xB6);
	writeData(0x0A);
	writeData(0xC2);
	writeCommand(0x36);	// memory access control
	writeData(0x80 | 0x20 | 0x08);	// RGB order, Row/Col exchange, Y flip
	writeCommand(0x3A);	// pixel format
	writeData(0x55);	//RGB565 (16bit)
	setArea(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
	writeCommand(0x29);
	writeCommand(0x2C);
	return RET_OK;
}

void DdIli9341Spi::writeCommand(uint8_t cmd)
{
	write(m_fdGpioDc, "0", 1);

	struct spi_ioc_transfer tr = {0};
	tr.tx_buf = (uint64_t)&cmd;
	tr.rx_buf = 0;
	tr.len = 1;
	ioctl(m_fdSpi, SPI_IOC_MESSAGE(1), &tr);
}

void DdIli9341Spi::writeData(uint8_t data)
{
	write(m_fdGpioDc, "1", 1);

	struct spi_ioc_transfer tr = {0};
	tr.tx_buf = (uint64_t)&data;
	tr.rx_buf = 0;
	tr.len = 1;
	ioctl(m_fdSpi, SPI_IOC_MESSAGE(1), &tr);
}

void DdIli9341Spi::writeData(uint8_t data[], uint32_t size)
{
	write(m_fdGpioDc, "1", 1);

	struct spi_ioc_transfer tr = {0};
	tr.tx_buf = (uint64_t)data;
	tr.rx_buf = 0;
	tr.len = size;
	ioctl(m_fdSpi, SPI_IOC_MESSAGE(1), &tr);
}

RET DdIli9341Spi::initializeGPIO()
{
	int fd;
	struct stat st;
	char str[64];
	sprintf(str, "/sys/class/gpio/gpio%s/direction", m_gpioPinDc);

	if (stat(str, &st) != 0) {
		/* 1. start using the specified GPIO pin, if it's not ready */
		fd = open("/sys/class/gpio/export", O_WRONLY);
		if (fd < 0) {
			LOG_E("gpio fd = %d\n", fd);
			return RET_ERR;
		}
		if (write(fd, m_gpioPinDc, strlen(m_gpioPinDc)) != (int)strlen(m_gpioPinDc)) {
			LOG_E("gpio export\n");
			// close(fd);
			// return RET_ERR;
		}
		close(fd);
	}

	/* 2. wait until file is ready */
	while(stat(str, &st) != 0) usleep(100);

	/* 3. set GPIO direction as out */
	fd = open(str, O_WRONLY);
	if (fd < 0) {
		LOG_E("gpio fd = %d\n", fd);
		return RET_ERR;
	}
	if (write(fd, "out", 3) != 3) {
		LOG_E("gpio direction\n");
		close(fd);
		return RET_ERR;
	}
	close(fd);

	/* 4. keep fd to access GPIO value */
	sprintf(str, "/sys/class/gpio/gpio%s/value", m_gpioPinDc);
	m_fdGpioDc = open(str, O_WRONLY);
	if (m_fdGpioDc < 0) {
		LOG_E("gpio fd = %d\n", m_fdGpioDc);
		return RET_ERR;
	}

	return RET_OK;
}

RET DdIli9341Spi::initializeSPI(const char* spiDevice)
{
	int ret = 0;
	m_fdSpi = open(spiDevice, O_RDWR);
	if (m_fdSpi < 0) {
		LOG_E("spi fd = %d\n", m_fdSpi);
		return RET_ERR;
	}

	ret |= ioctl(m_fdSpi, SPI_IOC_WR_MODE, &m_spiMode);
	ret |= ioctl(m_fdSpi, SPI_IOC_RD_MODE, &m_spiMode);
	if (ret != 0) {
		LOG_E("failed to set SPI mode. errno = %d, ret = %d\n", errno, ret);
		return RET_ERR;
	}

	ret |= ioctl(m_fdSpi, SPI_IOC_WR_BITS_PER_WORD, &m_bitsPerWord);
	ret |= ioctl(m_fdSpi, SPI_IOC_RD_BITS_PER_WORD, &m_bitsPerWord);
	ret |= ioctl(m_fdSpi, SPI_IOC_WR_MAX_SPEED_HZ, &m_spiSpeed);
	ret |= ioctl(m_fdSpi, SPI_IOC_RD_MAX_SPEED_HZ, &m_spiSpeed);
	if (ret != 0) {
		LOG_E("failed to set bits per word or speed. errno = %d, ret = %d\n", errno, ret);
		return RET_ERR;
	}
	
	return RET_OK;
}


void DdIli9341Spi::loopSpiTest()
{
	uint8_t tx[] = {
		0x11, 0x22, 0x33, 0x44, 0x55,
	};
	uint8_t rx[sizeof(tx)] = {0};
	struct spi_ioc_transfer tr = {0};
	tr.tx_buf = (uint64_t)tx;
	tr.rx_buf = (uint64_t)rx;
	tr.len = sizeof(tx);
	tr.speed_hz = m_spiSpeed;
	tr.delay_usecs = 0;
	tr.bits_per_word = m_bitsPerWord;

	ioctl(m_fdSpi, SPI_IOC_MESSAGE(1), &tr);

	for (unsigned int i = 0; i < sizeof(rx); i++) {
		printf("%02X ", rx[i]);
	}
	printf("\n");
}

void DdIli9341Spi::colorTest()
{
	usleep(2 * 1000 * 1000);
	drawRect(SWAP16BIT(0xF800));	// R
	usleep(2 * 1000 * 1000);
	drawRect(SWAP16BIT(0x07E0));	// G
	usleep(2 * 1000 * 1000);
	drawRect(SWAP16BIT(0x001F));	// B
}
