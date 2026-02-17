/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <stdlib.h>
#include <stdio.h>

#include <SDL3/SDL.h>

#include "../qcommon/q_shared.h"
#include "../client/snd_local.h"
#include "../client/client.h"

qboolean snd_inited = qfalse;

cvar_t *s_sdlBits;
cvar_t *s_sdlSpeed;
cvar_t *s_sdlChannels;
cvar_t *s_sdlDevSamps;
cvar_t *s_sdlMixSamps;

/* The audio callback. All the magic happens here. */
static int dmapos = 0;
static int dmasize = 0;

static SDL_AudioStream *sdlPlaybackStream;
static SDL_AudioDeviceID sdlPlaybackDevice;

#if defined USE_VOIP && SDL_VERSION_ATLEAST( 2, 0, 5 )
#define USE_SDL_AUDIO_CAPTURE

static SDL_AudioStream *sdlCaptureStream;
static SDL_AudioDeviceID sdlCaptureDevice;
static cvar_t *s_sdlCapture;
static float sdlMasterGain = 1.0f;
#endif

/*
===============
SNDDMA_QueueSilence
===============
*/
static void SNDDMA_QueueSilence(SDL_AudioStream *stream, int len)
{
	static Uint8 silence[4096];

	while (len > 0)
	{
		int chunk = len;
		if (chunk > (int) sizeof (silence))
			chunk = sizeof (silence);

		SDL_PutAudioStreamData(stream, silence, chunk);
		len -= chunk;
	}
}

/*
===============
SNDDMA_AudioCallback
===============
*/
static void SNDDMA_AudioCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	int len = additional_amount > 0 ? additional_amount : total_amount;
	const int bytesPerSample = dma.samplebits / 8;

	(void) userdata;

	if (len <= 0)
		return;

	if (!snd_inited || !dma.buffer || dmasize <= 0 || bytesPerSample <= 0)
	{
		SNDDMA_QueueSilence(stream, len);
		return;
	}

	while (len > 0)
	{
		int pos = dmapos * bytesPerSample;
		int len1;

		if (pos >= dmasize)
			dmapos = pos = 0;

		len1 = dmasize - pos;
		if (len1 > len)
			len1 = len;

		if (len1 <= 0)
			break;

		SDL_PutAudioStreamData(stream, dma.buffer + pos, len1);
		dmapos += (len1 / bytesPerSample);
		len -= len1;
	}

	if (len > 0)
		SNDDMA_QueueSilence(stream, len);

	if ((dmapos * bytesPerSample) >= dmasize)
		dmapos = 0;
}

static const struct
{
	Uint16	enumFormat;
	const char	*stringFormat;
} formatToStringTable[] =
{
	{ AUDIO_U8,     "AUDIO_U8" },
	{ AUDIO_S8,     "AUDIO_S8" },
	{ AUDIO_S16LSB, "AUDIO_S16LSB" },
	{ AUDIO_S16MSB, "AUDIO_S16MSB" },
	{ AUDIO_S32LSB, "AUDIO_S32LSB" },
	{ AUDIO_S32MSB, "AUDIO_S32MSB" },
	{ AUDIO_F32LSB, "AUDIO_F32LSB" },
	{ AUDIO_F32MSB, "AUDIO_F32MSB" }
};

static const int formatToStringTableSize = ARRAY_LEN( formatToStringTable );

/*
===============
SNDDMA_PrintAudiospec
===============
*/
static void SNDDMA_PrintAudiospec(const char *str, const SDL_AudioSpec *spec, int sampleFrames)
{
	int		i;
	const char	*fmt = NULL;

	Com_Printf("%s:\n", str);

	for( i = 0; i < formatToStringTableSize; i++ ) {
		if( spec->format == formatToStringTable[ i ].enumFormat ) {
			fmt = formatToStringTable[ i ].stringFormat;
		}
	}

	if( fmt ) {
		Com_Printf( "  Format:   %s\n", fmt );
	} else {
		Com_Printf( "  Format:   " S_COLOR_RED "UNKNOWN\n");
	}

	Com_Printf( "  Freq:     %d\n", (int) spec->freq );
	Com_Printf( "  Channels: %d\n", (int) spec->channels );
	if (sampleFrames > 0)
		Com_Printf( "  Frames:   %d\n", sampleFrames );
}

/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init(void)
{
	SDL_AudioSpec desired;
	SDL_AudioSpec obtained;
	int obtainedSamples = 0;
	int linked;
	int tmp;

	if (snd_inited)
		return qtrue;

	if (!s_sdlBits) {
		s_sdlBits = Cvar_Get("s_sdlBits", "16", CVAR_ARCHIVE);
		s_sdlSpeed = Cvar_Get("s_sdlSpeed", "0", CVAR_ARCHIVE);
		s_sdlChannels = Cvar_Get("s_sdlChannels", "2", CVAR_ARCHIVE);
		s_sdlDevSamps = Cvar_Get("s_sdlDevSamps", "0", CVAR_ARCHIVE);
		s_sdlMixSamps = Cvar_Get("s_sdlMixSamps", "0", CVAR_ARCHIVE);
	}

	Com_DPrintf( "SDL_Init( SDL_INIT_AUDIO )... " );

	if (!SDL_Init(SDL_INIT_AUDIO))
	{
		Com_Printf( "SDL_Init( SDL_INIT_AUDIO ) FAILED (%s)\n", SDL_GetError( ) );
		return qfalse;
	}

	Com_DPrintf( "OK\n" );
	linked = SDL_GetVersion();
	Com_Printf( "SDL version %d.%d.%d\n",
		SDL_VERSIONNUM_MAJOR( linked ),
		SDL_VERSIONNUM_MINOR( linked ),
		SDL_VERSIONNUM_MICRO( linked ) );
	Com_Printf( "SDL audio driver is \"%s\".\n", SDL_GetCurrentAudioDriver( ) );

	SDL_zero(desired);
	SDL_zero(obtained);

	tmp = ((int) s_sdlBits->value);
	if ((tmp != 16) && (tmp != 8))
		tmp = 16;

	desired.freq = (int) s_sdlSpeed->value;
	if(!desired.freq) desired.freq = 22050;
	desired.format = ((tmp == 16) ? AUDIO_S16SYS : AUDIO_U8);
	desired.channels = (int) s_sdlChannels->value;

	sdlPlaybackStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, SNDDMA_AudioCallback, NULL);
	if (sdlPlaybackStream == NULL)
	{
		Com_Printf("SDL_OpenAudioDeviceStream() failed: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return qfalse;
	}

	sdlPlaybackDevice = SDL_GetAudioStreamDevice(sdlPlaybackStream);
	if (sdlPlaybackDevice == 0 || !SDL_GetAudioDeviceFormat(sdlPlaybackDevice, &obtained, &obtainedSamples))
	{
		Com_Printf("SDL_GetAudioDeviceFormat() failed: %s\n", SDL_GetError());
		SDL_DestroyAudioStream(sdlPlaybackStream);
		sdlPlaybackStream = NULL;
		sdlPlaybackDevice = 0;
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return qfalse;
	}

	SNDDMA_PrintAudiospec("SDL_AudioSpec", &obtained, obtainedSamples);

	// dma.samples needs to be big, or id's mixer will just refuse to
	//  work at all; we need to keep it significantly bigger than the
	//  amount of SDL callback samples, and just copy a little each time
	//  the callback runs.
	// 32768 is what the OSS driver filled in here on my system. I don't
	//  know if it's a good value overall, but at least we know it's
	//  reasonable...this is why I let the user override.
	tmp = s_sdlMixSamps->value;
	if (!tmp)
		tmp = (obtainedSamples * obtained.channels) * 10;
	if (tmp <= 0)
		tmp = obtained.channels * 2048;

	// samples must be divisible by number of channels
	tmp -= tmp % obtained.channels;

	dmapos = 0;
	dma.samplebits = SDL_AUDIO_BITSIZE(obtained.format);
	dma.isfloat = SDL_AUDIO_ISFLOAT(obtained.format);
	dma.channels = obtained.channels;
	dma.samples = tmp;
	dma.fullsamples = dma.samples / dma.channels;
	dma.submission_chunk = 1;
	dma.speed = obtained.freq;
	dmasize = (dma.samples * (dma.samplebits/8));
	dma.buffer = calloc(1, dmasize);
	if (dma.buffer == NULL)
	{
		Com_Printf("Failed to allocate DMA buffer\n");
		SDL_DestroyAudioStream(sdlPlaybackStream);
		sdlPlaybackStream = NULL;
		sdlPlaybackDevice = 0;
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return qfalse;
	}

#ifdef USE_SDL_AUDIO_CAPTURE
	// !!! FIXME: some of these SDL_OpenAudioDevice() values should be cvars.
	s_sdlCapture = Cvar_Get( "s_sdlCapture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	if (!s_sdlCapture->integer)
	{
		Com_Printf("SDL audio capture support disabled by user ('+set s_sdlCapture 1' to enable)\n");
	}
#if USE_MUMBLE
	else if (cl_useMumble->integer)
	{
		Com_Printf("SDL audio capture support disabled for Mumble support\n");
	}
#endif
	else
	{
		/* !!! FIXME: list available devices and let cvar specify one, like OpenAL does */
		SDL_AudioSpec spec;
		SDL_zero(spec);
		spec.freq = 48000;
		spec.format = AUDIO_S16SYS;
		spec.channels = 1;
		sdlCaptureStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
		if (sdlCaptureStream != NULL)
		{
			sdlCaptureDevice = SDL_GetAudioStreamDevice(sdlCaptureStream);
			SDL_PauseAudioStreamDevice(sdlCaptureStream);
		}
		Com_Printf( "SDL capture device %s.\n",
				    (sdlCaptureStream == NULL) ? "failed to open" : "opened");
	}

	sdlMasterGain = 1.0f;
	if (sdlPlaybackStream != NULL)
		SDL_SetAudioStreamGain(sdlPlaybackStream, sdlMasterGain);
#endif

	Com_Printf("Starting SDL audio callback...\n");
	SDL_ResumeAudioStreamDevice(sdlPlaybackStream);
	// don't unpause the capture device; we'll do that in StartCapture.

	Com_Printf("SDL audio initialized.\n");
	snd_inited = qtrue;
	return qtrue;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int SNDDMA_GetDMAPos(void)
{
	return dmapos;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void)
{
	if (sdlPlaybackStream != NULL)
	{
		Com_Printf("Closing SDL audio playback device...\n");
		SDL_DestroyAudioStream(sdlPlaybackStream);
		Com_Printf("SDL audio playback device closed.\n");
		sdlPlaybackStream = NULL;
		sdlPlaybackDevice = 0;
	}

#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureStream != NULL)
	{
		Com_Printf("Closing SDL audio capture device...\n");
		SDL_DestroyAudioStream(sdlCaptureStream);
		Com_Printf("SDL audio capture device closed.\n");
		sdlCaptureStream = NULL;
		sdlCaptureDevice = 0;
	}
#endif

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	free(dma.buffer);
	dma.buffer = NULL;
	dmapos = dmasize = 0;
	snd_inited = qfalse;
	Com_Printf("SDL audio shut down.\n");
}

/*
===============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
	if (sdlPlaybackStream != NULL)
		SDL_UnlockAudioStream(sdlPlaybackStream);
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting (void)
{
	if (sdlPlaybackStream != NULL)
		SDL_LockAudioStream(sdlPlaybackStream);
}


#ifdef USE_VOIP
void SNDDMA_StartCapture(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureStream != NULL)
	{
		SDL_ClearAudioStream(sdlCaptureStream);
		SDL_ResumeAudioStreamDevice(sdlCaptureStream);
	}
#endif
}

int SNDDMA_AvailableCaptureSamples(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureStream != NULL)
	{
		const int bytes = SDL_GetAudioStreamAvailable(sdlCaptureStream);
		return (bytes > 0) ? (bytes / 2) : 0;
	}
	return 0;
#else
	return 0;
#endif
}

void SNDDMA_Capture(int samples, byte *data)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	// multiplied by 2 to convert from (mono16) samples to bytes.
	if (sdlCaptureStream != NULL)
	{
		const int bytes = samples * 2;
		int got = SDL_GetAudioStreamData(sdlCaptureStream, data, bytes);
		if (got < 0)
			got = 0;
		if (got < bytes)
			SDL_memset(data + got, '\0', bytes - got);
	}
	else
#endif
	{
		SDL_memset(data, '\0', samples * 2);
	}
}

void SNDDMA_StopCapture(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureStream != NULL)
		SDL_PauseAudioStreamDevice(sdlCaptureStream);
#endif
}

void SNDDMA_MasterGain( float val )
{
#ifdef USE_SDL_AUDIO_CAPTURE
	sdlMasterGain = val;
	if (sdlPlaybackStream != NULL)
		SDL_SetAudioStreamGain(sdlPlaybackStream, sdlMasterGain);
#endif
}
#endif
