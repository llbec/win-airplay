
#if defined (WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include "videosource.h"
#include "mediaserver.h"
#include "util/platform.h"

#if (_MSC_VER >= 1900) 
extern "C" { FILE __iob_func[3] = { *stdin,*stdout,*stderr }; }
#pragma  comment(lib, "legacy_stdio_definitions.lib") 
#endif


// airplay mirror resolution support:
// 2560 * 1440 
// 1920 * 1080
// 1280 * 720
#define SCREEN_WIDTH  2560
#define SCREEN_HEIGHT 1440

#define AIR_DEVICE_NAME  "Cell Receiver"
#define HEADER_SIZE 12
#define NO_PTS UINT64_C(-1)
  
#define H264_DECODE 1

//namespace AirPlay {

HDevice::HDevice()
	: active(false), audio_volume(100), mediaWidth(0), mediaHeight(0)
{
}

HDevice::~HDevice()
{
	Stop();
}

inline void HDevice::SendToCallback(bool video, unsigned char *data,
				    size_t size, long long startTime,
				    long long stopTime, long rotation)
{
	if (!size)
		return;

	if (video)
		videoConfig.callback(videoConfig, data, size, startTime,
				     stopTime, rotation);
	else
		audioConfig.callback(audioConfig, data, size, startTime,
				     stopTime);
}

bool HDevice::SetVideoConfig(VideoConfig *config)
{
	if (!config)
		return true;
	videoConfig = *config;
	videoConfig.format = VideoFormat::H264;
	*config = videoConfig;
	return true;
}

bool HDevice::SetAudioConfig(AudioConfig* config)
{
	if (!config)
		return true;
	audioConfig = *config;
	return true;
}

Result HDevice::Start()
{
	if (active)
		return Result::InUse;
	airplay_callbacks_t *ao = new airplay_callbacks_t();
	if (!ao) {
		return Result::Error;
	}
	memset(ao, 0, sizeof(airplay_callbacks_t));
	ao->cls = this;
	ao->AirPlayPlayback_Open = AirPlayOutputFunctions::airplay_open;
	ao->AirPlayPlayback_Play = AirPlayOutputFunctions::airplay_play;
	ao->AirPlayPlayback_Pause = AirPlayOutputFunctions::airplay_pause;
	ao->AirPlayPlayback_Stop = AirPlayOutputFunctions::airplay_stop;
	ao->AirPlayPlayback_Seek = AirPlayOutputFunctions::airplay_seek;
	ao->AirPlayPlayback_SetVolume =
		AirPlayOutputFunctions::airplay_setvolume;
	ao->AirPlayPlayback_ShowPhoto =
		AirPlayOutputFunctions::airplay_showphoto;
	ao->AirPlayPlayback_GetDuration =
		AirPlayOutputFunctions::airplay_getduration;
	ao->AirPlayPlayback_GetPostion =
		AirPlayOutputFunctions::airplay_getpostion;
	ao->AirPlayPlayback_IsPlaying =
		AirPlayOutputFunctions::airplay_isplaying;
	ao->AirPlayPlayback_IsPaused = AirPlayOutputFunctions::airplay_ispaused;
	//...
	ao->AirPlayAudio_Init = AirPlayOutputFunctions::audio_init;
	ao->AirPlayAudio_Process = AirPlayOutputFunctions::audio_process;
	ao->AirPlayAudio_destroy = AirPlayOutputFunctions::audio_destory;
	ao->AirPlayAudio_SetVolume = AirPlayOutputFunctions::audio_setvolume;
	ao->AirPlayAudio_SetMetadata = AirPlayOutputFunctions::audio_setmetadata;
	ao->AirPlayAudio_SetCoverart = AirPlayOutputFunctions::audio_setcoverart;
	ao->AirPlayAudio_Flush = AirPlayOutputFunctions::audio_flush;
	//...
	ao->AirPlayMirroring_Play = AirPlayOutputFunctions::mirroring_play;
	ao->AirPlayMirroring_Process = AirPlayOutputFunctions::mirroring_process;
	ao->AirPlayMirroring_Stop = AirPlayOutputFunctions::mirroring_stop;
	ao->AirPlayMirroring_Live = AirPlayOutputFunctions::mirroring_live;
	//...
	int r = startMediaServer((char *)AIR_DEVICE_NAME, SCREEN_WIDTH,
				 SCREEN_HEIGHT, ao);
	if (0 == r) {
		active = true;
		return Result::Success;
	}
	return Result::Error;
}

void HDevice::Stop()
{
	if (active) {
		stopMediaServer();
		active = false;
	}
}

bool HDevice::CreateGraph() {
	Stop();
	return Start() == Result::Success ? true : false;
}

/**/
/*airplay*/
void HDevice::AirPlayOutputFunctions::airplay_open(void *cls, char *url,
							  float fPosition,
							  double dPosition)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(url);
	UNUSED_PARAMETER(fPosition);
	UNUSED_PARAMETER(dPosition);
}

void HDevice::AirPlayOutputFunctions::airplay_play(void *cls)
{
	UNUSED_PARAMETER(cls);
}

void HDevice::AirPlayOutputFunctions::airplay_pause(void *cls)
{
	UNUSED_PARAMETER(cls);
}

void HDevice::AirPlayOutputFunctions::airplay_stop(void *cls)
{
	UNUSED_PARAMETER(cls);
}

void HDevice::AirPlayOutputFunctions::airplay_seek(void *cls, long fPosition)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(fPosition);
}

void HDevice::AirPlayOutputFunctions::airplay_setvolume(void *cls, int volume)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(volume);
}

void HDevice::AirPlayOutputFunctions::airplay_showphoto(void *cls,
							unsigned char *data,
							long long size)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(size);
}

long HDevice::AirPlayOutputFunctions::airplay_getduration(void *cls)
{
	UNUSED_PARAMETER(cls);
	return 0;
}

long HDevice::AirPlayOutputFunctions::airplay_getpostion(void *cls)
{
	UNUSED_PARAMETER(cls);
	return 0;
}

int HDevice::AirPlayOutputFunctions::airplay_isplaying(void *cls)
{
	UNUSED_PARAMETER(cls);
	return 0;
}

int HDevice::AirPlayOutputFunctions::airplay_ispaused(void *cls)
{
	UNUSED_PARAMETER(cls);
	return 0;
}

/*audio*/
void HDevice::AirPlayOutputFunctions::audio_init(void *cls, int bits,
						 int channels, int samplerate,
						 int isaudio)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(bits);
	UNUSED_PARAMETER(channels);
	UNUSED_PARAMETER(samplerate);
	UNUSED_PARAMETER(isaudio);
}

void HDevice::AirPlayOutputFunctions::audio_process(void *cls,
						    const void *buffer,
						    int buflen,
						    uint64_t timestamp,
						    uint32_t seqnum)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(buffer);
	UNUSED_PARAMETER(buflen);
	UNUSED_PARAMETER(timestamp);
	UNUSED_PARAMETER(seqnum);
}

void HDevice::AirPlayOutputFunctions::audio_destory(void *cls)
{
	UNUSED_PARAMETER(cls);
}

void HDevice::AirPlayOutputFunctions::audio_setvolume(void *cls, int volume)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(volume);
}

void HDevice::AirPlayOutputFunctions::audio_setmetadata(void *cls,
							const void *buffer,
							int buflen)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(buffer);
	UNUSED_PARAMETER(buflen);
}

void HDevice::AirPlayOutputFunctions::audio_setcoverart(void *cls,
							const void *buffer,
							int buflen)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(buffer);
	UNUSED_PARAMETER(buflen);
}

void HDevice::AirPlayOutputFunctions::audio_flush(void *cls)
{
	UNUSED_PARAMETER(cls);
}

/*mirror*/
static uint64_t ptsOrigin = -1;
uint8_t *mirrbuff = nullptr;
void HDevice::AirPlayOutputFunctions::mirroring_play(
	void *cls, int width, int height, const void *buffer, int buflen,
	int payloadtype, uint64_t timestamp)
{
	if (!mirrbuff) {
		const int MIRROR_BUFF_SIZE = 4 * 1024 * 1024;
		mirrbuff = new uint8_t[MIRROR_BUFF_SIZE];
	}

	int spscnt;
	int spsnalsize;
	int ppscnt;
	int ppsnalsize;

	unsigned char *head = (unsigned char *)buffer;

	spscnt = head[5] & 0x1f;
	spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
	ppscnt = head[8 + spsnalsize];
	ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) |
		     ((uint32_t)head[10 + spsnalsize]);

	unsigned char *data =
		(unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);
	if (!data)
		return;

	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 1;
	memcpy(data + 4, head + 8, spsnalsize);
	data[4 + spsnalsize] = 0;
	data[5 + spsnalsize] = 0;
	data[6 + spsnalsize] = 0;
	data[7 + spsnalsize] = 1;
	memcpy(data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);

	ptsOrigin = timestamp;
	uint64_t pts = NO_PTS;
	uint32_t len = 4 + spsnalsize + 4 + ppsnalsize;
	HDevice *ptr = (HDevice *)cls;
	ptr->SendToCallback(true, data, len, timestamp, 0, 0);
	free(data);
}

void HDevice::AirPlayOutputFunctions::mirroring_process(void *cls,
							const void *buffer,
							int buflen,
							int payloadtype,
							uint64_t timestamp)
{
	if (payloadtype == 0) {
		int rLen;
		unsigned char *head;

		memcpy(mirrbuff, buffer, buflen);

		rLen = 0;
		head = (unsigned char *)mirrbuff + rLen;
		while (rLen < buflen) {
			rLen += 4;
			rLen += (((uint32_t)head[0] << 24) |
				 ((uint32_t)head[1] << 16) |
				 ((uint32_t)head[2] << 8) | (uint32_t)head[3]);

			head[0] = 0;
			head[1] = 0;
			head[2] = 0;
			head[3] = 1;
			head = (unsigned char *)mirrbuff + rLen;
		}
		uint64_t pts = timestamp - ptsOrigin;
		uint32_t len = buflen;
		HDevice *ptr = (HDevice *)cls;
		ptr->SendToCallback(true, mirrbuff, len, timestamp, 0, 0);
	} else if (payloadtype == 1) {
		int spscnt;
		int spsnalsize;
		int ppscnt;
		int ppsnalsize;

		unsigned char *head = (unsigned char *)buffer;

		spscnt = head[5] & 0x1f;
		spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
		ppscnt = head[8 + spsnalsize];
		ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) |
			     ((uint32_t)head[10 + spsnalsize]);

		unsigned char *data = (unsigned char *)malloc(4 + spsnalsize +
							      4 + ppsnalsize);

		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 1;
		memcpy(data + 4, head + 8, spsnalsize);
		data[4 + spsnalsize] = 0;
		data[5 + spsnalsize] = 0;
		data[6 + spsnalsize] = 0;
		data[7 + spsnalsize] = 1;
		memcpy(data + 8 + spsnalsize, head + 11 + spsnalsize,
		       ppsnalsize);

		ptsOrigin = timestamp;
		uint64_t pts = NO_PTS;
		uint32_t len = 4 + spsnalsize + 4 + ppsnalsize;
		HDevice *ptr = (HDevice *)cls;
		ptr->SendToCallback(true, data, len, timestamp, 0, 0);
		free(data);
	}
}

void HDevice::AirPlayOutputFunctions::mirroring_stop(void *cls)
{
	if (mirrbuff) {
		delete[] mirrbuff;
		mirrbuff = nullptr;
	}
}

void HDevice::AirPlayOutputFunctions::mirroring_live(void *cls)
{
	UNUSED_PARAMETER(cls);
}

/******/
void HDevice::AirPlayOutputFunctions::sdl_audio_callback(void *cls,
							 uint8_t *stream,
							 int len)
{
	UNUSED_PARAMETER(cls);
	UNUSED_PARAMETER(stream);
	UNUSED_PARAMETER(len);
}

//}//AirPlay namespace
