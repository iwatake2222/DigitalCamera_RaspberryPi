#ifndef APP_H_
#define APP_H_

#include "common.h"
#include "ddTpTsc2046Spi.h"
#include "cameraCtrl.h"
#include "playbackCtrl.h"

class App
{
private:
	typedef enum {
		STATUS_BOOT,
		STATUS_LIVEVIEW,
		STATUS_CAPTURE,
		STATUS_PLAYBACK,
	} STATUS;
	
	typedef enum {
		INPUT_NONE,
		INPUT_CENTER,	// capture@liveview, next@playback
		INPUT_CORNER,	// mode change
	} INPUT_TYPE;

private:
	STATUS m_status;
	INPUT_TYPE m_inputType;
	uint8_t *m_bufferRGB565;
	uint8_t *m_bufferYUYV;
	DdTpTsc2046Spi *m_ddTpTsc2046Spi;
	CameraCtrl *m_cameraCtrl;
	PlaybackCtrl *m_playbackCtrl;
	uint32_t m_saveFileIndex;
	uint32_t m_loadFileIndex;

private:
	static void cbTouchPanel(float x, float y, void* arg);

private:
	void processLiveviewFrame();
	void processLiveviewMsg(INPUT_TYPE inputType);
	void processPlaybackFrame();
	void processPlaybackMsg(INPUT_TYPE inputType);
	RET getInput(INPUT_TYPE *inputType);
	void waitForFps(int fps);
	void notifyInput(INPUT_TYPE inputType);
	void startLiveview();
	void startPlayback();
	RET getSaveFilename(char* filename);
	RET getLoadFilename(char* filename);

public:
	App();
	~App();
	void run();
};

#endif /* APP_H_ */