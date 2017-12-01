#ifndef DD_CAMERA_H_
#define DD_CAMERA_H_

#include "common.h"

class DdCamera
{
public:
	typedef enum {
		CAPTURE_FORMAT_YUYV,
		CAPTURE_FORMAT_YUV420,
		CAPTURE_FORMAT_RGB888,
		CAPTURE_FORMAT_JPG,
	} CAPTURE_FORMAT;

private:
	static const uint32_t BUFFER_NUM = 1;	// use only one buffer to decrease display delay

private:
	int m_fd;
	void *m_buffer[BUFFER_NUM];
	uint32_t m_bufferSize[BUFFER_NUM];
	uint32_t m_bufferNum;	// actually allocated buffer num (can be bigger/smaller than BUFFER_NUM)
	uint32_t m_width;
	uint32_t m_height;

private:
	RET init(uint32_t width, uint32_t height, CAPTURE_FORMAT pixelFormat);
	RET finalize();

public:
	DdCamera(uint32_t width, uint32_t height, CAPTURE_FORMAT pixelFormat);
	~DdCamera();
	RET captureStart();
	RET captureStop();
	RET captureCopyBuffer(uint8_t *dstBuffer, uint32_t *size);
};

#endif /* DD_CAMERA_H_ */