#ifndef DD_ILI_9341_SPI_H_
#define DD_ILI_9341_SPI_H_

#include "common.h"

class DdIli9341Spi
{
private:
	const static uint32_t LCD_WIDTH = 320;
	const static uint32_t LCD_HEIGHT = 240;

private:
	/* variables for SPI */
	int m_fdSpi;	// /dev/spidev0.0
	uint16_t m_spiMode;
	uint8_t m_bitsPerWord;
	uint32_t m_spiSpeed;

	/* variables for GPIO(DC) */
	int m_fdGpioDc;	// /sys/class/gpio/gpio26/value
	char m_gpioPinDc[4];

private:
	RET initializeSPI(const char* spiDevice);
	RET initializeGPIO();
	RET initializeDevice();
	void writeCommand(uint8_t cmd);
	void writeData(uint8_t data);
	void writeData(uint8_t data[], uint32_t size);
	void loopSpiTest();
	void colorTest();

public:
	DdIli9341Spi(const char* spiDevice, int gpioPinDc);
	~DdIli9341Spi();

	// note: not thread safe
	RET setArea(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
	RET drawBuffer(uint16_t *buffer, uint32_t x0 = 0, uint32_t y0 = 0, uint32_t x1 = LCD_WIDTH - 1, uint32_t y1 = LCD_HEIGHT - 1);
	RET drawRect(uint16_t color, uint32_t x0 = 0, uint32_t y0 = 0, uint32_t x1 = LCD_WIDTH - 1, uint32_t y1 = LCD_HEIGHT - 1);

};

#endif /* DD_ILI_9341_SPI_H_ */