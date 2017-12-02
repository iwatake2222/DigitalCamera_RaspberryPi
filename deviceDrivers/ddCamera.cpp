#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "ddCamera.h"

/* sudo modprobe bcm2835-v4l2 */
/*  v4l2-ctl -d /dev/video0 --list-formats-ext */
DdCamera::DdCamera(uint32_t width, uint32_t height, CAPTURE_FORMAT pixelFormat)
{
	init(width, height, pixelFormat);
}

DdCamera::~DdCamera()
{
	finalize();
}


RET DdCamera::init(uint32_t width, uint32_t height, CAPTURE_FORMAT pixelFormat)
{
	int ret;

	m_width = width;
	m_height = height;
	uint32_t v4l2pixelFormat;
	switch(pixelFormat) {
	case CAPTURE_FORMAT_YUYV:
		v4l2pixelFormat = V4L2_PIX_FMT_YUYV; break;
	case CAPTURE_FORMAT_YUV420:
		v4l2pixelFormat = V4L2_PIX_FMT_YUV420; break;
	case CAPTURE_FORMAT_RGB888:
		v4l2pixelFormat = V4L2_PIX_FMT_RGB24; break;
	case CAPTURE_FORMAT_JPG:
		v4l2pixelFormat = V4L2_PIX_FMT_JPEG; break;
	default:
		LOG_E("parameter error %d\n", pixelFormat);	return RET_ERR;
	}

	/* open video device file */
	m_fd = open(DD_CAMERA_V2L4, O_RDWR);
	if(m_fd < 0) {
		LOG_E("open: errno = %d, fd = %d\n", errno, m_fd);
		return RET_ERR;
	}

	/* set pixel format and capture size */
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = m_width;
	fmt.fmt.pix.height = m_height;
	fmt.fmt.pix.pixelformat = v4l2pixelFormat;
	fmt.fmt.pix.field = V4L2_FIELD_ANY;
	// fmt.fmt.pix.colorspace = V4L2_COLORSPACE_DEFAULT;
	// fmt.fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	if ((ret = ioctl(m_fd, VIDIOC_S_FMT, &fmt)) < 0) {
		LOG_E("ioctl(VIDIOC_S_FMT): errno = %d, ret = %d\n", errno, ret);
		return RET_ERR;
	}
	if ( (fmt.fmt.pix.pixelformat != v4l2pixelFormat) || (fmt.fmt.pix.width != m_width) || (fmt.fmt.pix.height != m_height) ){
		LOG_E("PixFmt: %08X / %08X, Width:%d / %d, Height:%d / %d\n", fmt.fmt.pix.pixelformat, v4l2pixelFormat, fmt.fmt.pix.width, m_width, fmt.fmt.pix.height, m_height);
		return RET_ERR;
	}

	/* request buffers (device allocates buffers?) */
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count = BUFFER_NUM;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if ((ret = ioctl(m_fd, VIDIOC_REQBUFS, &req)) < 0) {
		LOG_E("ioctl(VIDIOC_REQBUFS): errno = %d, ret = %d\n", errno, ret);
		return RET_ERR;
	}
	if(m_bufferNum != BUFFER_NUM) LOG_W("Assigned buffer num = %d, but allocated buffer num = %d\n", BUFFER_NUM, req.count);
	/* memo: use single or double buffer to avoid liveview delay */
	// m_bufferNum = req.count;
	m_bufferNum = BUFFER_NUM;

	/* mmap the buffer memory allocated by VIDIOC_REQBUFS, so that I can access the buffers */
	struct v4l2_buffer buf;
	for (uint32_t i = 0; i < m_bufferNum; i++) {
		/* get the buffer information */
		memset(&buf, 0, sizeof(buf));
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;
		if ((ret = ioctl(m_fd, VIDIOC_QUERYBUF, &buf)) < 0) {	
			LOG_E("ioctl(VIDIOC_QUERYBUF): errno = %d, ret = %d\n", errno, ret);
			return RET_ERR;
		}

		/* mmap addres according to the information retrieved from device(VIDIOC_QUERYBUF) */
		m_buffer[i] = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, m_fd, buf.m.offset);
		if (m_buffer[i] == MAP_FAILED) {
			LOG_E("mmap: %d/%d", i, m_bufferNum);
			return RET_ERR;
		}
		m_bufferSize[i] = buf.length;
		LOG("m_buffer[%d] size is %d\n", i, buf.length);
	}

	/* enqueue the allocated buffers for stream capture */
	for (uint32_t i = 0; i < m_bufferNum; i++) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if ((ret = ioctl(m_fd, VIDIOC_QBUF, &buf)) < 0) {
			LOG_E("ioctl(VIDIOC_QBUF): errno = %d, ret = %d\n", errno, ret);
			return RET_ERR;
		}
	}

	return RET_OK;
}

RET DdCamera::finalize()
{
	/* release resources */
	for (uint32_t i = 0; i < m_bufferNum; i++) munmap(m_buffer[i], m_bufferSize[i]);
	close(m_fd);
	return RET_OK;
}

RET DdCamera::captureStart()
{
	/* start camera capture(stream) */
	int ret;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if ((ret = ioctl(m_fd, VIDIOC_STREAMON, &type)) < 0) {
		LOG_E("ioctl(VIDIOC_STREAMON): errno = %d, ret = %d\n", errno, ret);
		return RET_ERR;
	}
	return RET_OK;
}

RET DdCamera::captureStop()
{
	/* stop camera capture(stream) */
	int ret;
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if ((ret = ioctl(m_fd, VIDIOC_STREAMOFF, &type)) < 0) {
		LOG_E("ioctl(VIDIOC_STREAMOFF): errno = %d, ret = %d\n", errno, ret);
		return RET_ERR;
	}
	return RET_OK;
}

/* copy the oldest captured buffer and captured size */
/* returnd buffer index looks like 0, 1, 2, 0, 1, 2, 0, 1,,, (at buffer num = 3)*/
RET DdCamera::captureCopyBuffer(uint8_t *dstBuffer, uint32_t *size)
{
	int ret;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(m_fd, &fds);

	// wait until any of the buffers is ready
	while(1) {
		if ((ret = select(m_fd + 1, &fds, NULL, NULL, NULL)) < 0) {
			if (errno == EINTR) continue;
			LOG_E("select: errno = %d, ret = %d\n", errno, ret);
			return RET_ERR;;
		}
		break;
	}

	if (FD_ISSET(m_fd, &fds)) {	// check if the fd exists. (must be true)
		/* dequeue a filled (captured) buffer from the driver's outgoint queue */
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(m_fd, VIDIOC_DQBUF, &buf);
		if (ret < 0) {
			LOG_E("ioctl(VIDOC_DQBUF): errno = %d, ret = %d\n", errno, ret);
			return RET_ERR;
		}
		*size = buf.bytesused;

		/* copy the buffer (in the driver) to destination buffer */
		memcpy(dstBuffer, m_buffer[buf.index], buf.bytesused);

		/* enqueue(reuse) a buffer in the driver's incoming queue */
		ret = ioctl(m_fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			LOG_E("ioctl(VIDIOC_QBUF): errno = %d, ret = %d\n", errno, ret);
			return RET_ERR;
		}
	} else {
		LOG_E("FD_ISSET\n");
		return RET_ERR;
	}
	return RET_OK;
}
