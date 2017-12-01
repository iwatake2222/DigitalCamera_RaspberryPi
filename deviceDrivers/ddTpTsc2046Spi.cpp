#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <linux/spi/spidev.h>
#include "ddTpTsc2046Spi.h"

/*
 * spiDevice: "/dev/spidev0.0"
 * gpioPinDc: 26
 */
DdTpTsc2046Spi::DdTpTsc2046Spi(const char* spiDevice, int gpioPinIrq, CALLBACK cb, void *arg)
{
	m_isThreadExit = false;
	m_cbTouch = cb;
	m_cbArg = arg;
	m_spiMode= SPI_MODE_0;
	m_bitsPerWord = 8;
	m_spiSpeed = 125 * 1000;
	sprintf(m_gpioPinIrq, "%d", gpioPinIrq);

	initializeSPI(spiDevice);
	while(initializeGPIO() != RET_OK) usleep(1000);

	pthread_attr_t tattr;
	pthread_attr_init(&tattr);
	pthread_create(&m_thread, &tattr, &threadFunc, this);
}

DdTpTsc2046Spi::~DdTpTsc2046Spi()
{
	LOG("enter destructor\n");
	m_isThreadExit = true;
	pthread_join(m_thread, NULL);
	close(m_fdSpi);
	openWriteFile("/sys/class/gpio/unexport", m_gpioPinIrq);
	LOG("finish destructor\n");
}

void* DdTpTsc2046Spi::threadFunc(void *pInstance)
{
	DdTpTsc2046Spi* p = (DdTpTsc2046Spi*)pInstance;
	int fd;
	char str[64];
	struct pollfd pfd;
	sprintf(str, "/sys/class/gpio/gpio%s/value", p->m_gpioPinIrq);
	fd = open(str, O_RDONLY);
	if (fd < 0) {
		LOG_E("gpio fd = %d\n", fd);
	}
	pfd.fd = fd;
	pfd.events = POLLPRI;

	float x, y;
	bool pressed;
	while(!p->m_isThreadExit) {
		lseek(fd, 0, SEEK_SET);
		if(poll(&pfd, 1, 1000) != 0) {
			pressed = p->getPosition(&x, &y);
			if(pressed) {
				p->m_cbTouch(x, y, p->m_cbArg);
				usleep(p->INTERRUPT_INTERVAL_MS * 1000);
			}
			char c;
			read(fd, &c, 1);	// need to read one to reset irq status?
		}
	}

	close(fd);
	return 0;
}


bool DdTpTsc2046Spi::getPosition(float *x, float *y)
{
	int temp;
	uint8_t data0, data1;

	// measure X
	sendCommand(createCommand(1, 0, 0), &data0, &data1);
	temp = ((data1 << 4) & 0xFF0) | ((data0 >> 4) & 0x0F);
	*x = temp / 2048.0;

	// measure Y
	sendCommand(createCommand(5, 0, 0), &data0, &data1);
	temp = ((data1 << 4) & 0xFF0) | ((data0 >> 4) & 0x0F);
	*y = temp / 2048.0;

	// measure pressure
	sendCommand(createCommand(3, 0, 0), &data0, &data1);
	temp = ((data1 << 4) & 0xFF0) | ((data0 >> 4) & 0x0F);
	
	// printf("X = %f, Y = %f, Press = %d\n", *x, *y, temp);
	if (temp < 10) {
		return false;
	} else {
		return true;
	}
}

uint8_t DdTpTsc2046Spi::createCommand(uint8_t A, uint8_t mode, uint8_t SER)
{
	return 0x80 | (A << 4) | (mode << 3) | (SER << 2) | (0 << 0);
}

void DdTpTsc2046Spi::sendCommand(uint8_t cmd, uint8_t *data0, uint8_t *data1)
{
	/* the first sent byte is command */
	/* the last two sent bytes are dummy data to read (MSB must be 0, which means 'S' bit is false) */
	uint8_t tx[3] = {cmd, 0x00, 0x00};
	uint8_t rx[3];
	struct spi_ioc_transfer tr = {0};
	tr.tx_buf = (uint64_t)tx;
	tr.rx_buf = (uint64_t)rx;
	tr.len = 3;
	tr.speed_hz = m_spiSpeed;
	tr.delay_usecs = 0;
	tr.bits_per_word = m_bitsPerWord;
	ioctl(m_fdSpi, SPI_IOC_MESSAGE(1), &tr);
	*data0 = rx[0];
	*data1 = rx[1];
}

RET DdTpTsc2046Spi::openWriteFile(const char* filePath, const char* str)
{
	int fd;
	fd = open(filePath, O_WRONLY);
	if (fd < 0) {
		LOG_E("Open %s, fd = %d\n", filePath, fd);
		return RET_ERR;
	}
	if (write(fd, str, strlen(str)) != (int)strlen(str)) {
		LOG_E("Write %s\n", str);
		close(fd);
		return RET_ERR;
	}
	close(fd);
	return RET_OK;
}

RET DdTpTsc2046Spi::initializeGPIO()
{
	RET ret;
	struct stat st;
	char str[64];
	sprintf(str, "/sys/class/gpio/gpio%s/direction", m_gpioPinIrq);


	if (stat(str, &st) != 0) {
		/* 1. start using the specified GPIO pin, if it's not ready */
		ret = openWriteFile("/sys/class/gpio/export", m_gpioPinIrq);
		// do not check error for export because an error may occur when the port is already exoprted
	}

	/* 2. wait until file is ready */
	while(stat(str, &st) != 0) usleep(100);
		
	/* 3. set GPIO direction as in */
	ret = openWriteFile(str, "in");
	if (ret != RET_OK) return RET_ERR;

	/* 4. detect falling edge */
	sprintf(str, "/sys/class/gpio/gpio%s/edge", m_gpioPinIrq);
	ret = openWriteFile(str, "falling");
	if (ret != RET_OK) return RET_ERR;

	return RET_OK;
}

RET DdTpTsc2046Spi::initializeSPI(const char* spiDevice)
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


