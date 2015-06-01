#include "record_mic.h"

#ifdef _WIN32

#define sleep_ms(x) Sleep(x)

extern FILE* fp;

typedef struct capture_mic
{
	LPDIRECTSOUNDCAPTURE8 lpCapure;

	WAVEFORMATEX  Wavefmt;
	DSCBUFFERDESC BufferDesc;
	LPDIRECTSOUNDCAPTUREBUFFER pDSCaptureBuffer;

	capture_read  Cb;

	int Capture_flag;
	pthread_t Thread;
}capture_mic_t;

#else

#endif

void *create_miccapture(mic_property_t *param, capture_read cb);
int  start_miccapture(void *ctx);
void stop_miccapture(void *ctx);
void miccapture_join(void *ctx);

void* create_miccapture(mic_property_t *param, capture_read cb)
{
	HRESULT hr = S_OK;
	capture_mic_t *mic_ctx = NULL;

	mic_ctx = (capture_mic_t *)malloc((sizeof(capture_mic_t)));
	if(mic_ctx == NULL)
	{
		printf("[Capture microphone]: malloc object failed. \n");
		goto fail;
	}
	memset(mic_ctx, 0, sizeof(capture_mic_t));
	mic_ctx->Capture_flag = 0;
	mic_ctx->Cb = cb;

	hr = DirectSoundCaptureCreate8(NULL, &mic_ctx->lpCapure, NULL);
	if(S_OK != hr)
	{
		printf("[Capture microphone]: create object failed. \n");
		goto fail;
	}

	WAVEFORMATEX *pWavefmt = &(mic_ctx->Wavefmt);
	memset(pWavefmt, 0, sizeof(WAVEFORMATEX));
	pWavefmt->cbSize = sizeof(WAVEFORMATEX);
	pWavefmt->wFormatTag = WAVE_FORMAT_PCM;
	pWavefmt->nSamplesPerSec  = param->Sample;
	pWavefmt->wBitsPerSample = param	->Precision;
	pWavefmt->nChannels = param->Channels;
	pWavefmt->nBlockAlign = pWavefmt->nChannels * (pWavefmt->wBitsPerSample / 8);
	pWavefmt->nAvgBytesPerSec = pWavefmt->nBlockAlign * pWavefmt->nSamplesPerSec;
	
	DSCBUFFERDESC *pBufferDes = &(mic_ctx->BufferDesc);
	memset(pBufferDes, 0, sizeof(DSCBUFFERDESC));
	pBufferDes->dwSize = sizeof(DSCBUFFERDESC);
	pBufferDes->dwBufferBytes = pWavefmt->nAvgBytesPerSec;
	pBufferDes->lpwfxFormat = pWavefmt;

	hr = mic_ctx->lpCapure->CreateCaptureBuffer(pBufferDes, &mic_ctx->pDSCaptureBuffer, NULL);
	if(S_OK != hr)
	{
		printf("[Capture microphone]: create capture buffer failed. \n");
		goto fail;
	}

#if DEBUG
	fp = fopen(PCM_FILE, "wb");
	if(NULL == fp)
	{
		printf("[Capture microphone]: create mic-capture file failed. \n");
	}
#endif

	return (void*)mic_ctx;

fail:
	stop_miccapture(mic_ctx);
	miccapture_join(mic_ctx);
	return NULL;
}

void stop_miccapture(void *mic)
{
	capture_mic_t *mic_ctx = (capture_mic_t*)mic;
	if(NULL == mic_ctx)
	{
		return;
	}

	if(mic_ctx->Capture_flag == 1)
	{
		mic_ctx->pDSCaptureBuffer->Stop();
		mic_ctx->Capture_flag = 0;
	}

	return ;
}

void miccapture_join(void *mic)
{
	capture_mic_t *mic_ctx = (capture_mic_t*)mic;
	if(NULL == mic_ctx)
	{
		return;
	}

    pthread_join(mic_ctx->Thread, (void **)mic_ctx);

#if DEBUG
	if(fp)
	{
		fclose(fp);
	}
#endif

	return ;
}

static void *capture_thread(void *ctx)
{
	capture_mic_t *mic_ctx = (capture_mic_t *)ctx;
	HRESULT hr = S_OK;
	DWORD   dwReadPos = 0;
	DWORD   dwCapturePos = 0;
	DWORD   dwLastReadPos = 0;
	void    *pbCaptureData = NULL;
	DWORD   dwCaptureLength = 0;
	void    *pbCaptureData2 = NULL;
	DWORD   dwCaptureLength2 = 0;
	DWORD   dwSize = 0;

	while(mic_ctx->Capture_flag)
	{
		hr = mic_ctx->pDSCaptureBuffer->GetCurrentPosition(&dwCapturePos, &dwReadPos);
		if(dwReadPos <= 0)
		{
			continue;
			sleep_ms(1);
		}

		if(dwReadPos < dwLastReadPos)
		{
			dwSize = dwReadPos + mic_ctx->BufferDesc.dwBufferBytes - dwLastReadPos;
		}
		else
		{
			dwSize = dwReadPos - dwLastReadPos;
		}

		hr = mic_ctx->pDSCaptureBuffer->Lock(dwLastReadPos, dwSize,
			&pbCaptureData, &dwCaptureLength,
			&pbCaptureData2, &dwCaptureLength2, 0L
			);

	   if(pbCaptureData && dwCaptureLength && mic_ctx->Cb)
	   {
		   mic_ctx->Cb((unsigned char*)pbCaptureData, dwCaptureLength);
	   }

#if DEBUG
	  if(fp)
	  {
		  fwrite(pbCaptureData, 1, dwCaptureLength, fp);
		  fflush(fp);
	  }
#endif

	   hr = mic_ctx->pDSCaptureBuffer->Unlock(pbCaptureData, dwCaptureLength,
			pbCaptureData2, dwCaptureLength2
			);

	   dwLastReadPos += dwSize;
	   dwLastReadPos %= (mic_ctx->BufferDesc.dwBufferBytes);

	   sleep_ms(1);
	}

	return NULL;
}

int start_miccapture(void *ctx)
{
	capture_mic_t *mic_ctx = (capture_mic_t *)ctx;
	HRESULT hr = S_OK;

	hr = mic_ctx->pDSCaptureBuffer->Start(DSCBSTART_LOOPING);
	if(S_OK == hr)
	{
		mic_ctx->Capture_flag = 1;
		pthread_create(&mic_ctx->Thread, NULL, capture_thread, ctx);
		sleep_ms(5);
	}
	else
	{
		printf("[Capture microphone]: capture buffer start failed. \n");
		return -1;
	}

	return 0;
}
