#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

/* common return code */
typedef int RET;
#define RET_OK  0x00000000
#define RET_NO_DATA  0x00000001
#define RET_TIMEOUT  0x00000002
#define RET_ERR 0x80000001

/* LOG macros */
#define LOG(str, ...) {printf("\x1b[39m"); printf("[%s:%d] " str, __FILE__, __LINE__, ##__VA_ARGS__);}
#define LOG_W(str, ...) {printf("\x1b[33m"); printf("[WARNING %s:%d] " str, __FILE__, __LINE__, ##__VA_ARGS__);}
#define LOG_E(str, ...) {printf("\x1b[31m"); printf("[ERROR %s:%d] " str, __FILE__, __LINE__, ##__VA_ARGS__);}

/* definition depending on specs and HW */
#define TARGET_FPS 30
#define WIDTH 320
#define HEIGHT 240
#define FILENAME_FORMAT "IMG_%04d.jpg"

/* device related definition (todo: should be separated file) */
#define DD_ILI_9341_SPI_SPI_DEV "/dev/spidev0.0"
#define DD_ILI_9341_SPI_GPIO_DC 26
#define DD_TP_TSC_2046_SPI_SPI_DEV "/dev/spidev0.1"
#define DD_TP_TSC_2046_SPI_GPIO_IRQ 19
#define DD_CAMERA_V2L4 "/dev/video0"

#endif /* COMMON_H_ */
