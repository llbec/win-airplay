#pragma once

#include "airplaycapture.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <string>

#define CHROMA "YUYV"

#define __STDC_CONSTANT_MACROS


#define MAX_CACHED_AFRAME_NUM 200
#define MAX_CACHED_VFRAME_NUM 10

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)
#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0
#define AUDIO_DIFF_AVG_NB 20
#define VIDEO_PICTURE_QUEUE_SIZE 1

//namespace AirPlay {

struct AirStream;

struct HDevice {
	unsigned long long sockSvr;
	unsigned long long sockAcp;
	unsigned int mediaWidth;
	unsigned int mediaHeight;
	int audio_volume;
	bool active;

	void * stream;
	VideoConfig videoConfig;
	AudioConfig audioConfig;

	struct AirPlayOutputFunctions {
		static void airplay_open(void *cls, char *url, float fPosition,
					 double dPosition);
		static void airplay_play(void *cls);
		static void airplay_pause(void *cls);
		static void airplay_stop(void *cls);
		static void airplay_seek(void *cls, long fPosition);
		static void airplay_setvolume(void *cls, int volume);
		static void airplay_showphoto(void *cls, unsigned char *data,
					      long long size);
		static long airplay_getduration(void *cls);
		static long airplay_getpostion(void *cls);
		static int airplay_isplaying(void *cls);
		static int airplay_ispaused(void *cls);
		//...
		static void audio_init(void *cls, int bits, int channels,
				       int samplerate, int isaudio);
		static void audio_process(void *cls, const void *buffer,
					  int buflen, uint64_t timestamp,
					  uint32_t seqnum);
		static void audio_destory(void *cls);
		static void audio_setvolume(void *cls, int volume); //1-100
		static void audio_setmetadata(void *cls, const void *buffer,
					      int buflen);
		static void audio_setcoverart(void *cls, const void *buffer,
					      int buflen);
		static void audio_flush(void *cls);
		//...
		static void mirroring_play(void *cls, int width, int height,
					   const void *buffer, int buflen,
					   int payloadtype, uint64_t timestamp);
		static void mirroring_process(void *cls, const void *buffer,
					      int buflen, int payloadtype,
					      uint64_t timestamp);
		static void mirroring_stop(void *cls);
		static void mirroring_live(void *cls);
		//...
		static void sdl_audio_callback(void *cls, uint8_t *stream,
					       int len);
	};

	HDevice();
	~HDevice();

	/*inline void SendToCallback(bool video, unsigned char *data, size_t size,
				   long long startTime, long long stopTime,
				   long rotation);*/

	bool SetVideoConfig(VideoConfig *config);
	bool SetAudioConfig(AudioConfig *config);
	bool CreateGraph();
	bool StartNetwork(int port);
	Result Start();
	void Stop();
};

//}//AirPlay namespace

#if 0
class AirScreen
{
public:
	AirScreen();
	~AirScreen();

public:
	void start_airplay();
	void stop_airplay();
	volatile bool audio_quit;

	int audio_volume;

	void toggle_full_screen();

public:
	void *pixelData;

	unsigned int mediaWidth;
	unsigned int mediaHeight;

	// should only be set inside texture lock
	bool isRendering;
	int remainingVideos;

public:
	class AirPlayOutputFunctions
	{
	public:
		static void airplay_open(void *cls, char *url, float fPosition, double dPosition);
		static void airplay_play(void *cls);
		static void airplay_pause(void *cls);
		static void airplay_stop(void *cls);
		static void airplay_seek(void *cls, long fPosition);
		static void airplay_setvolume(void *cls, int volume);
		static void airplay_showphoto(void *cls, unsigned char *data, long long size);
		static long airplay_getduration(void *cls);
		static long airplay_getpostion(void *cls);
		static int  airplay_isplaying(void *cls);
		static int  airplay_ispaused(void *cls);
        //...
		static void audio_init(void *cls, int bits, int channels, int samplerate, int isaudio);
		static void audio_process(void *cls, const void *buffer, int buflen, uint64_t timestamp, uint32_t seqnum);
		static void audio_destory(void *cls);
		static void audio_setvolume(void *cls, int volume);//1-100
		static void audio_setmetadata(void *cls, const void *buffer, int buflen);
		static void audio_setcoverart(void *cls, const void *buffer, int buflen);
		static void audio_flush(void *cls);
        //...
		static void mirroring_play(void *cls, int width, int height, const void *buffer, int buflen, int payloadtype, uint64_t timestamp);
		static void mirroring_process(void *cls, const void *buffer, int buflen, int payloadtype, uint64_t timestamp);
		static void mirroring_stop(void *cls);
        //...
		static void sdl_audio_callback(void *cls, uint8_t *stream, int len);
	};
};
#endif
