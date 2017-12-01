#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "utility.h"
#include "app.h"
#include "playbackCtrl.h"
#include "cameraCtrl.h"
#include "ddTpTsc2046Spi.h"

App::App()
{
	m_status = STATUS_BOOT;
	m_inputType = INPUT_NONE;
	m_saveFileIndex = 0;
	m_loadFileIndex = 0;
	m_ddTpTsc2046Spi = new DdTpTsc2046Spi(DD_TP_TSC_2046_SPI_SPI_DEV, DD_TP_TSC_2046_SPI_GPIO_IRQ, cbTouchPanel, this);
	
	/* start liveview by default */
	startLiveview();
}

App::~App()
{
	delete m_ddTpTsc2046Spi;
	if(m_cameraCtrl) delete m_cameraCtrl;
	if(m_playbackCtrl) delete m_playbackCtrl;
}

void App::run()
{
	while(1) {
		INPUT_TYPE inputType;
		RET ret = getInput(&inputType);

		if(ret == RET_OK) {
			/* process for the received message */
			switch(m_status) {
			case STATUS_LIVEVIEW:
				processLiveviewMsg(inputType);
				break;
			case STATUS_PLAYBACK:
				processPlaybackMsg(inputType);
				break;
			default:
				break;
			}
		}

		/* process for every frame */
		switch(m_status) {
		case STATUS_LIVEVIEW:
			processLiveviewFrame();
			break;
		case STATUS_PLAYBACK:
			processPlaybackFrame();
			break;
		default:
			break;
		}
		waitForFps(TARGET_FPS);
	}
}

void App::processLiveviewMsg(INPUT_TYPE inputType)
{
	char filename[64];
	switch (inputType) {
	case INPUT_CENTER:
		m_status = STATUS_CAPTURE;
		getSaveFilename(filename);
		m_cameraCtrl->captureJpeg(filename);
		LOG("CAPTURE: %s\n", filename);
		m_status = STATUS_LIVEVIEW;
		break;
	case INPUT_CORNER:
		startPlayback();
		break;
	default:
		/* do nothing */
		break;
	}

}

void App::processLiveviewFrame()
{
	m_cameraCtrl->liveviewFrame();
}

void App::processPlaybackMsg(INPUT_TYPE inputType)
{
	char filename[64];
	switch (inputType) {
	case INPUT_CENTER:
		if(getLoadFilename(filename) == RET_OK) {
			m_playbackCtrl->play(filename);
		}
		break;
	case INPUT_CORNER:
		startLiveview();
		break;
	default:
		/* do nothing */
		break;
	}
}

void App::processPlaybackFrame()
{
	/* do nothing */
}

void App::startLiveview()
{
	switch(m_status) {
	case STATUS_LIVEVIEW:
		/* do nothing */
		break;
	case STATUS_PLAYBACK:
		/* exit playback */
		delete m_playbackCtrl;
		m_playbackCtrl = NULL;
		/* then, start liveview */
	default:
		m_cameraCtrl = new CameraCtrl(WIDTH, HEIGHT);
		m_cameraCtrl->liveviewStart();
		m_status = STATUS_LIVEVIEW;
	}
}

void App::startPlayback()
{
	char filename[64];
	switch(m_status) {
	case STATUS_PLAYBACK:
		/* do nothing */
		break;
	case STATUS_LIVEVIEW:
		/* exit liveview */
		m_cameraCtrl->liveviewStop();
		delete m_cameraCtrl;
		m_cameraCtrl = NULL;
		/* then, start playback */
	default:
		m_loadFileIndex = 0;
		m_playbackCtrl = new PlaybackCtrl(WIDTH, HEIGHT);
		if(getLoadFilename(filename) == RET_OK) {
			m_playbackCtrl->play(filename);
		}
		m_status = STATUS_PLAYBACK;
	}
}

void App::waitForFps(int fps)
{
	static clock_t previousTime = clock();
	double interval;
	while(1) {
		interval = (double)(clock() - previousTime)/ CLOCKS_PER_SEC;
		if(interval > 1.0/(double)fps) break;
		usleep(100);
	}
	// LOG("FPS = %.2f\n", 1.0/interval);
	previousTime = clock();
}


void App::notifyInput(INPUT_TYPE inputType)
{
	/* todo: mutex guard */
	m_inputType = inputType;
	/* todo: mutex guard */
}

RET App::getInput(INPUT_TYPE *inputType)
{
	/* todo: mutex guard */
	*inputType = m_inputType;
	m_inputType = INPUT_NONE;
	/* todo: mutex guard */
	if(*inputType == INPUT_NONE) return RET_NO_DATA;
	return RET_OK;
}

void App::cbTouchPanel(float x, float y, void *arg)
{
	LOG("Touch:%f %f\n", x, y);
	App *p = (App*)arg;
	if(x < 0.8 && y < 0.8) {
		p->notifyInput(INPUT_CENTER);
	} else {
		p->notifyInput(INPUT_CORNER);
	} 
}

RET App::getSaveFilename(char* filename)
{
	FILE *fp;
	for(int i = 0; i < 999; i++) {
		sprintf(filename, FILENAME_FORMAT, m_saveFileIndex++);
		if ((fp = fopen(filename, "r")) == NULL) {
			return RET_OK;
		}
		fclose(fp);
	}
	return RET_NO_DATA;
}

RET App::getLoadFilename(char* filename)
{
	FILE *fp;
	for(int i = 0; i < 999; i++) {
		sprintf(filename, FILENAME_FORMAT, m_loadFileIndex++);
		if ((fp = fopen(filename, "r")) != NULL) {
			fclose(fp);
			return RET_OK;
		}
	}
	m_loadFileIndex = 0;
	return RET_NO_DATA;
}
