#ifndef CAMERA_CTRL_H_
#define CAMERA_CTRL_H_

#include "common.h"
#include "ddIli9341Spi.h"
#include "ddCamera.h"

class CameraCtrl
{
private:
	DdIli9341Spi *m_ddIli9341Spi;
	DdCamera *m_ddCamera;
	uint8_t *m_bufferRGB888;
	uint8_t *m_bufferRGB565;
	uint32_t m_width;
	uint32_t m_height;

public:
	CameraCtrl(uint32_t width, uint32_t height);
	~CameraCtrl();
	RET liveviewStart();
	RET liveviewStop();
	RET liveviewFrame();
	RET captureJpeg(const char* filename);
};

#endif /* CAMERA_CTRL_H_ */

