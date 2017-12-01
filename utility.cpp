#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "utility.h"

RET saveFileBinary(const char* filename, uint8_t* data, int size)
{
	FILE *fp;
	fp = fopen(filename, "wb");
	if(fp == NULL) {
		LOG_E("fopen\n");
		return RET_ERR;
	}
	fwrite(data, sizeof(uint8_t), size, fp);
	fclose(fp);
	return RET_OK;
}

void convertRGB888To565(uint8_t *src, uint8_t *dst, uint32_t pixelSize)
{
	for(uint32_t i = 0; i < pixelSize; i++){
		uint8_t r = (*src++);
		uint8_t g = (*src++);
		uint8_t b = (*src++);
		*dst++ = (r & 0xF8) | ((g >> 5) & 0x07);
		*dst++ = ((g << 3)&0xE0) | ((b >> 3) & 0x1F);
	}
}

void convertYUYVToRGB565(uint8_t *src, uint8_t *dst, uint32_t pixelSize)
{
	/* convert YUYV (ITU-R BT.601) to RGB565 */
	for(uint32_t i = 0; i < pixelSize / 2; i++){
		uint8_t y0 = *src++;
		uint8_t u = *src++;
		uint8_t y1 = *src++;
		uint8_t v = *src++;
		uint8_t r0 = 1.164 * (y0 - 16)                   + 1.596 * (v - 128);
		uint8_t g0 = 1.164 * (y0 - 16) - 0.391 * (u-128) - 0.813 * (v - 128);
		uint8_t b0 = 1.164 * (y0 - 16) + 2.018 * (u-128);
		uint8_t r1 = 1.164 * (y1 - 16)                   + 1.596 * (v - 128);
		uint8_t g1 = 1.164 * (y1 - 16) - 0.391 * (u-128) - 0.813 * (v - 128);
		uint8_t b1 = 1.164 * (y1 - 16) + 2.018 * (u-128);
		*dst++ = (r0 & 0xF8) | ((g0 >> 5) & 0x07);
		*dst++ = ((g0 << 3)&0xE0) | ((b0 >> 3) & 0x1F);
		*dst++ = (r1 & 0xF8) | ((g1 >> 5) & 0x07);
		*dst++ = ((g1 << 3)&0xE0) | ((b1 >> 3) & 0x1F);
	}
}
