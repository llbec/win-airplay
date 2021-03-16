#include <objbase.h>

#include <obs-module.h>
#include <obs.hpp>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/windows/WinHandle.hpp>
#include <util/threading.h>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/windows/WinHandle.hpp>
#include <util/threading.h>
//#include <WinSock2.h>
#include "videosource.hpp"
#include "mediaserver.h"
#include "buffer_util.h"

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
#define IPV4_LOCALHOST 0x7F000001
#define LOCALPORT 2727

static SOCKET Connect_Socket(int port) {
	int trycount = 3;
	do {
		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET)
			continue;
		SOCKADDR_IN sin;
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(IPV4_LOCALHOST);
		sin.sin_port = htons(port);
		if (connect(sock, (SOCKADDR *)&sin, sizeof(sin)) ==
		    SOCKET_ERROR) {
			closesocket(sock);
			continue;
		}
		if (sock != INVALID_SOCKET)
			return sock;
	} while (--trycount > 0);
	return INVALID_SOCKET;
}

//namespace AirPlay {

static DWORD CALLBACK AStreamThread(LPVOID ptr);

struct AirStream {
	WinHandle thread;
	SOCKET socket;
	VideoConfig videoConfig;
	AirStream()
	{
		// init socket
		int port = LOCALPORT;
		socket = Connect_Socket(port);
		if (socket == INVALID_SOCKET)
			throw "AirStream: Failed to create socket";
		thread = CreateThread(nullptr, 0, AStreamThread, this, 0,
				      nullptr);
		if (!thread)
			throw "AirStream: Failed to create thread";
	}
	~AirStream() {
		closesocket(socket);
		WaitForSingleObject(thread, INFINITE);
	}
	inline void SendToCallback(void *data, size_t size,
				   long long startTime, long long stopTime,
				   long rotation)
	{
		if (!size)
			return;
		videoConfig.callback(videoConfig, (unsigned char *)data, size, startTime,
				     stopTime, rotation);
	}
	bool SetVideoConfig(VideoConfig *config) {
		if (!config)
			return true;
		videoConfig = *config;
		videoConfig.format = VideoFormat::H264;
		*config = videoConfig;
		return true;
	}
	void StreamLoop()
	{
		while (true) {
			uint8_t header[HEADER_SIZE];
			int r = recv(socket, (char *)header, HEADER_SIZE, 0);
			if (r < HEADER_SIZE)
				break;
			uint64_t pts = buffer_read64be(header);
			uint32_t len = buffer_read32be(&header[8]);
			if (pts != NO_PTS && (pts & 0x8000000000000000) != 0)
				throw "Invalid pts!";
			if (len == 0)
				throw "Invalid length!";
			char *buf = (char *)malloc(len);
			if (!buf)
				throw "malloc failed!";
			r = recv(socket, buf, len, 0);
			if (r != len)
				break;
			SendToCallback(buf, len, pts, 0, 0);
			free(buf);
			buf = NULL;
		}
	}
};

static DWORD CALLBACK AStreamThread(LPVOID ptr)
{
	AirStream *stream = (AirStream *)ptr;

	os_set_thread_name("win-airplay: AStreamThread");

	CoInitialize(nullptr);
	stream->StreamLoop();
	CoUninitialize();
	return 0;
}


HDevice::HDevice()
	: active(false), audio_volume(100), mediaWidth(0), mediaHeight(0)
{
	stream = NULL;
}

HDevice::~HDevice()
{
	Stop();
	if (stream)
		delete ((AirStream *)stream);
}


bool HDevice::SetVideoConfig(VideoConfig *config)
{
	/*if (!config)
		return true;
	videoConfig = *config;
	videoConfig.format = VideoFormat::H264;
	*config = videoConfig;
	return true;*/
	return ((AirStream*)stream)->SetVideoConfig(config);
}

bool HDevice::SetAudioConfig(AudioConfig* config)
{
	if (!config)
		return true;
	audioConfig = *config;
	return true;
}

bool HDevice::StartNetwork(int port) {
	sockSvr = socket(AF_INET, SOCK_STREAM, 0);
	if (sockSvr == INVALID_SOCKET)
		return false;
	int reuse = 1;
	if (setsockopt(sockSvr, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
		       sizeof(reuse)) == -1)
		return false;

	SOCKADDR_IN sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr =
		htonl(IPV4_LOCALHOST); // htonl() harmless on INADDR_ANY
	sin.sin_port = htons(port);
	if (bind(sockSvr, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR) {
		closesocket(sockSvr);
		return false;
	}
	if (listen(sockSvr, 1) == SOCKET_ERROR) {
		closesocket(sockSvr);
		return false;
	}
	return true;
}

Result HDevice::Start()
{
	if (active)
		return Result::InUse;

	if (!StartNetwork(LOCALPORT))
		return Result::Error;

	stream = new AirStream();

	airplay_callbacks_t *ao = new airplay_callbacks_t();
	if (!ao) {
		closesocket(sockSvr);
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
	} else if (r < 0) {
		throw "start media server error code : -1";
	} else {
		throw "start media server error code : 1";
	}
	closesocket(sockSvr);
	return Result::Error;
}

void HDevice::Stop()
{
	if (active) {
		closesocket(sockSvr);
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

int net_send_all(SOCKET socket, const void *buf, size_t len)
{
	int w = 0;
	while (len > 0) {
		w = send(socket, (char *)buf, len, 0);
		if (w == -1) {
			return -1;
		}
		len -= w;
		buf = (char *)buf + w;
	}
	return w;
}

void HDevice::AirPlayOutputFunctions::mirroring_play(
	void *cls, int width, int height, const void *buffer, int buflen,
	int payloadtype, uint64_t timestamp)
{
	HDevice *ptr = (HDevice *)cls;
	if (!ptr)
		return;
	if (!mirrbuff) {
		const int MIRROR_BUFF_SIZE = 4 * 1024 * 1024;
		mirrbuff = new uint8_t[MIRROR_BUFF_SIZE];
	}

	SOCKADDR_IN csin;
	int sinsize = sizeof(csin);
	ptr->sockAcp = accept(ptr->sockSvr, (SOCKADDR *)&csin, &sinsize);

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

	uint8_t hdr[12];
	buffer_write64be(&hdr[0], pts);
	buffer_write32be(&hdr[8], len);
	net_send_all(ptr->sockAcp, hdr, 12);
	net_send_all(ptr->sockAcp, data, len);
	//ptr->SendToCallback(true, data, len, timestamp, 0, 0);
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
		//ptr->SendToCallback(true, mirrbuff, len, timestamp, 0, 0);
		uint8_t hdr[12];
		buffer_write64be(&hdr[0], pts);
		buffer_write32be(&hdr[8], len);
		net_send_all(ptr->sockAcp, hdr, 12);
		net_send_all(ptr->sockAcp, mirrbuff, len);
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
		//ptr->SendToCallback(true, data, len, timestamp, 0, 0);
		uint8_t hdr[12];
		buffer_write64be(&hdr[0], pts);
		buffer_write32be(&hdr[8], len);
		net_send_all(ptr->sockAcp, hdr, 12);
		net_send_all(ptr->sockAcp, data, len);
		free(data);
	}
}

void HDevice::AirPlayOutputFunctions::mirroring_stop(void *cls)
{
	HDevice *ptr = (HDevice *)cls;
	closesocket(ptr->sockAcp);
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
