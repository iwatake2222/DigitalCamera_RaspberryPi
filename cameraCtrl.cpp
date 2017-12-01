#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "utility.h"
#include "cameraCtrl.h"
#include "ddIli9341Spi.h"
#include "ddCamera.h"


CameraCtrl::CameraCtrl(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	m_ddCamera = NULL;
	m_ddIli9341Spi = new DdIli9341Spi(DD_ILI_9341_SPI_SPI_DEV, DD_ILI_9341_SPI_GPIO_DC);
}

CameraCtrl::~CameraCtrl()
{
	if(m_ddCamera) liveviewStop();	// in case liveview was not stopped
	delete m_ddIli9341Spi;
}

RET CameraCtrl::liveviewStart()
{	
	if(m_ddCamera) return RET_ERR;
	m_bufferRGB565 = new uint8_t[m_width * m_height * 2];
	m_bufferRGB888 = new uint8_t[m_width * m_height * 3];
	m_ddCamera = new DdCamera(m_width, m_height, DdCamera::CAPTURE_FORMAT_RGB888);
	m_ddCamera->captureStart();
	return RET_OK;
}

RET CameraCtrl::liveviewStop()
{	
	if(!m_ddCamera) return RET_ERR;
	m_ddCamera->captureStop();
	delete m_ddCamera;
	delete m_bufferRGB565;
	delete m_bufferRGB888;
	m_ddCamera = NULL;
	return RET_OK;
}


RET CameraCtrl::liveviewFrame()
{	
	uint32_t size;

	m_ddCamera->captureCopyBuffer(m_bufferRGB888, &size);
	if (size != m_width * m_height * 3) {
		LOG_E("size must be %d, but it was %d\n", m_width * m_height * 3, size);
		return RET_ERR;
	}

	convertRGB888To565(m_bufferRGB888, m_bufferRGB565, m_width * m_height);	

	m_ddIli9341Spi->drawBuffer((uint16_t*)m_bufferRGB565);
	
	return RET_OK;
}


RET CameraCtrl::captureJpeg(const char* filename)
{	
	if(m_ddCamera) liveviewStop();
	uint8_t *bufferJpeg = new uint8_t[m_width * m_height * 2];
	m_ddCamera = new DdCamera(m_width, m_height, DdCamera::CAPTURE_FORMAT_JPG);
	m_ddCamera->captureStart();

	uint32_t size;
	m_ddCamera->captureCopyBuffer(bufferJpeg, &size);
	saveFileBinary(filename, bufferJpeg, size);

	m_ddCamera->captureStop();
	delete m_ddCamera;
	delete bufferJpeg;
	m_ddCamera = NULL;

	liveviewStart();	// restart liveview
	return RET_OK;
}

