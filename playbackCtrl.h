#ifndef PLAYBACK_CTRL_H_
#define PLAYBACK_CTRL_H_

#include <jpeglib.h>
#include "common.h"
#include "ddIli9341Spi.h"

class PlaybackCtrl
{
private:
	DdIli9341Spi *m_ddIli9341Spi;
	uint32_t m_width;
	uint32_t m_height;

private:
	static void libjpeg_output_message(j_common_ptr cinfo);

private:
	RET decodeJpeg(FILE *file, uint32_t maxWidth, uint32_t maxHeight, uint8_t *bufferRGB565);
	RET calcJpegOutputSize(struct jpeg_decompress_struct* cinfo, uint32_t maxWidth, uint32_t maxHeight);

public:
	PlaybackCtrl(uint32_t width, uint32_t height);
	~PlaybackCtrl();
	RET play(const char* filename);
};

#endif /* PLAYBACK_CTRL_H_ */

