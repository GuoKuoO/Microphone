#include "record_mic.h"
#include <signal.h>

#if DEBUG
FILE    *fp = NULL;
#endif

static int b_app_start = 0;

static void *mic_ctx = NULL;

static void SigIntHandler(int const a)
{
	signal(SIGINT, SigIntHandler);
	if(!b_app_start)
	{
		exit(0);
	}

	stop_miccapture(mic_ctx);
}

int main()
{
    mic_property_t cap_param;
	cap_param.Channels  =  2;
	cap_param.Precision =  16;
	cap_param.Sample    =  44100;

	signal(SIGINT, SigIntHandler);

#if defined(WIN32)
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

	mic_ctx = create_miccapture(&cap_param, NULL);
	if(mic_ctx)
	{
		start_miccapture(mic_ctx);
		b_app_start = 1;
	}

	miccapture_join(mic_ctx);

	return 0;
}
