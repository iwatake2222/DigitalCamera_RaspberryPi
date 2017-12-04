#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <jpeglib.h>
#include "common.h"
#include "utility.h"
#include "playbackCtrl.h"
#include "ddIli9341Spi.h"

PlaybackCtrl::PlaybackCtrl(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	m_ddIli9341Spi = new DdIli9341Spi(DD_ILI_9341_SPI_SPI_DEV, DD_ILI_9341_SPI_GPIO_DC);
}

PlaybackCtrl::~PlaybackCtrl()
{
	delete m_ddIli9341Spi;
}

RET PlaybackCtrl::play(const char* filename)
{
	uint8_t* buffRGB565 = new uint8_t[m_width * m_height * 2];
	FILE *fp;
	fp = fopen(filename, "rb");
	decodeJpeg(fp, m_width, m_height, buffRGB565);
	m_ddIli9341Spi->drawBuffer((uint16_t*)buffRGB565);

	fclose(fp);
	delete[] buffRGB565;

	return RET_OK;
}

void PlaybackCtrl::libjpeg_output_message(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);
	printf("%s\n", buffer);
}

RET PlaybackCtrl::calcJpegOutputSize(struct jpeg_decompress_struct* cinfo, uint32_t maxWidth, uint32_t maxHeight)
{
	RET ret;
	if( (cinfo->image_width == maxWidth) && (cinfo->image_height == maxHeight) ) return RET_OK;

	uint32_t scaleX = 8, scaleY = 8;    // real scale = scale / 8
	if(cinfo->image_width <= maxWidth) {
		scaleX = 8;
	} else if(cinfo->image_width/2 <= maxWidth) {
		scaleX = 4;
	} else if(cinfo->image_width/4 <= maxWidth) {
		scaleX = 2;
	} else if(cinfo->image_width/8 <= maxWidth) {
		scaleX = 1;
	} else {
		return RET_ERR;
	}
	scaleY = scaleX;
	if ((cinfo->image_height * scaleY) / 8 > maxHeight){
		if(cinfo->image_height <= maxHeight) {
			scaleY = 8;
		} else if(cinfo->image_height/2 <= maxHeight) {
			scaleY = 4;
		} else if(cinfo->image_height/4 <= maxHeight) {
			scaleY = 2;
		} else if(cinfo->image_height/8 <= maxHeight) {
			scaleY = 1;
		} else {
			return RET_ERR;
		}
	}
	scaleX = scaleY;
	cinfo->scale_num = scaleX;
	cinfo->scale_denom = 8;

	jpeg_calc_output_dimensions(cinfo);

	ret = m_ddIli9341Spi->setArea( (maxWidth - cinfo->output_width) / 2, (maxHeight - cinfo->output_height) / 2,
							(maxWidth + cinfo->output_width) / 2 - 1, (maxHeight + cinfo->output_height) / 2 - 1);
	return ret;
}


RET PlaybackCtrl::decodeJpeg(FILE *file, uint32_t maxWidth, uint32_t maxHeight, uint8_t *bufferRGB565)
{
	int ret = 0;

	/*** alloc memory ***/
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	uint8_t* lineBuffRGB888 = new uint8_t[320*3];
	JSAMPROW buffer[2] = {0};

	/*** prepare libjpeg ***/
	buffer[0] = lineBuffRGB888;
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->output_message = libjpeg_output_message;  // over-write error output function
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, file);

	/* get jpeg info to resize appropriate size */
	ret = jpeg_read_header(&cinfo, TRUE);
	if(ret != JPEG_HEADER_OK) {
		LOG_E("%d\n", ret);
		jpeg_destroy_decompress(&cinfo);
		delete[] lineBuffRGB888;
		return RET_ERR;
	}

	/* calculate output size */
	ret = calcJpegOutputSize(&cinfo, maxWidth, maxHeight);
	if(ret != RET_OK) {
		LOG_E("unsupported size %d %d\n", cinfo.image_width, cinfo.image_height);
		jpeg_destroy_decompress(&cinfo);
		delete[] lineBuffRGB888;
		return RET_ERR;
	}

	/* jpeg decode setting */
	cinfo.dct_method = JDCT_ISLOW;
	cinfo.dither_mode = JDITHER_ORDERED;
	cinfo.do_fancy_upsampling = TRUE;
	ret = jpeg_start_decompress(&cinfo);
	if(ret != 1) {
		LOG_E("%d\n", ret);
		jpeg_destroy_decompress(&cinfo);
		delete[] lineBuffRGB888;
		return RET_ERR;
	}

	/*** decode jpeg and display it line by line ***/
	while( cinfo.output_scanline < cinfo.output_height ) {
		if (jpeg_read_scanlines(&cinfo, buffer, 1) != 1) {
			LOG_E("Decode Stop at line %d\n", cinfo.output_scanline);
			break;
		}
		convertRGB888To565(lineBuffRGB888, bufferRGB565, cinfo.output_width);
		bufferRGB565 += cinfo.output_width * 2;
	}

	ret = jpeg_finish_decompress(&cinfo);
	if(ret != 1) {
		LOG_E("%d\n", ret);
	}
	jpeg_destroy_decompress(&cinfo);
	delete[] lineBuffRGB888;

	return RET_OK;
}

