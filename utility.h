#include <common.h>

#define SWAP16BIT(data) (((data&0xFF) << 8) | ((data >> 8) & 0xFF))

RET saveFileBinary(const char* filename, uint8_t* data, int size);
void convertRGB888To565(uint8_t *src, uint8_t *dst, uint32_t pixelSize);
void convertYUYVToRGB565(uint8_t *src, uint8_t *dst, uint32_t pixelSize);

