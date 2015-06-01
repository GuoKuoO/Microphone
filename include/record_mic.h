#ifndef __RECORD_MIC_
#define __RECORD_MIC_

#define DEBUG    (1)
#define PCM_FILE  "mic.pcm"

typedef int (*capture_read)(unsigned char* buffer, int length);

typedef struct mic_property
{
	int        Sample;
	int        Channels;
	int        Precision;
}mic_property_t;

#ifdef _WIN32

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>
#include <stdlib.h>
#include <stdio.h>
#include "pthread.h"

#else

#endif

void* create_miccapture(mic_property_t *param, capture_read cb);

int start_miccapture(void *ctx);

void stop_miccapture(void *ctx);

void miccapture_join(void *ctx);

#endif