#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

void startCapture();
void copyBuffer(uint8_t *dstBuffer, uint32_t *size);
void stopCapture();
int saveFileBinary(const char* filename, uint8_t* data, int size);

int main()
{
	uint8_t* buff = new uint8_t[320*240];
	uint32_t size;
	startCapture();
	copyBuffer(buff, &size);
	stopCapture();
	saveFileBinary("aaa.jpg", buff, size);
	delete[] buff;
	return 0;
}


int fd;
const int v4l2BufferNum = 2;
void *v4l2Buffer[v4l2BufferNum];
uint32_t v4l2BufferSize[v4l2BufferNum];

void startCapture()
{
	fd = open("/dev/video0", O_RDWR);

	/* 1. フォーマット指定。320 x 240のJPEG形式でキャプチャしてください */
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 320;
	fmt.fmt.pix.height = 240;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
	ioctl(fd, VIDIOC_S_FMT, &fmt);

	/* 2. バッファリクエスト。バッファを2面確保してください */
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count = v4l2BufferNum;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	ioctl(fd, VIDIOC_REQBUFS, &req);

	/* 3. 確保されたバッファをユーザプログラムからアクセスできるようにmmapする */
	struct v4l2_buffer buf;
	for (uint32_t i = 0; i < v4l2BufferNum; i++) {
		/* 3.1 確保したバッファ情報を教えてください */
		memset(&buf, 0, sizeof(buf));
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;
		ioctl(fd, VIDIOC_QUERYBUF, &buf);

		/* 3.2 取得したバッファ情報を基にmmapして、後でアクセスできるようにアドレスを保持っておく */
		v4l2Buffer[i] = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, fd, buf.m.offset);
		v4l2BufferSize[i] = buf.length;
	}

	/* 4. バッファのエンキュー。指定するバッファをキャプチャするときに使ってください */
	for (uint32_t i = 0; i < v4l2BufferNum; i++) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		ioctl(fd, VIDIOC_QBUF, &buf);
	}

	/* 5. ストリーミング開始。キャプチャを開始してください */
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMON, &type);

	/* この例だと2面しかないので、2フレームのキャプチャ(1/30*2秒?)が終わった後、新たにバッファがエンキューされるまでバッファへの書き込みは行われない */
}

void copyBuffer(uint8_t *dstBuffer, uint32_t *size)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	/* 6. バッファに画データが書き込まれるまで待つ */
	while(select(fd + 1, &fds, NULL, NULL, NULL) < 0);

	if (FD_ISSET(fd, &fds)) {
		/* 7. バッファのデキュー。もっとも古くキャプチャされたバッファをデキューして、そのインデックス番号を教えてください */
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		ioctl(fd, VIDIOC_DQBUF, &buf);

		/* 8. デキューされたバッファのインデックス(buf.index)と書き込まれたサイズ(buf.byteused)が返ってくる */
		*size = buf.bytesused;

		/* 9. ユーザプログラムで使いやすいように、別途バッファにコピーしておく */
		memcpy(dstBuffer, v4l2Buffer[buf.index], buf.bytesused);

		/* 10. 先ほどデキューしたバッファを、再度エンキューする。カメラデバイスはこのバッファに対して再びキャプチャした画を書き込む */
		ioctl(fd, VIDIOC_QBUF, &buf);
	}
}

void stopCapture()
{
	/* 11. ストリーミング終了。キャプチャを停止してください */
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMOFF, &type);

	/* 12. リソース解放 */
	for (uint32_t i = 0; i < v4l2BufferNum; i++) munmap(v4l2Buffer[i], v4l2BufferSize[i]);

	/* 13. デバイスファイルを閉じる */
	close(fd);
}

int saveFileBinary(const char* filename, uint8_t* data, int size)
{
	FILE *fp;
	fp = fopen(filename, "wb");
	if(fp == NULL) {
		return -1;
	}
	fwrite(data, sizeof(uint8_t), size, fp);
	fclose(fp);
	return 0;
}
