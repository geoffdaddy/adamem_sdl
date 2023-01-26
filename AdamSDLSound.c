/*
 *  AdamSDLSound.c
 *  adamem
 *
 *  Created by Geoffrey Oltmans on 6/10/08.
 *  Copyright 2008 Geoffrey Oltmans. All rights reserved.
 *
 */

#include "AdamSDLSound.h"
#include "math.h"
#include "Z80.h"
#include "Coleco.h"


#define MAXFREQ         1400000    /* In cHertz, freq. divider=8            */
#define MAXINT			2^10-1


static int initialised;
static SndStruct soundState;


int soundquality=3;
static int invalidfreq[4];
static int feedback=1;
static Uint8 channel_on[4]= {1,1,1,1};
static Uint8 noise_output_channel_3=0;
static unsigned channelfreq[4];
static unsigned channelvolume[4];
static unsigned ChannelFreq[4];
static unsigned NoiseFreqs[4]= {16,32,64,0};
static Uint8 sound_on=1;
static Uint16 noiseshiftregister;
static void noiseTick(void);


// Sound callback
void soundData(void *userdata, Uint8 *stream, int len)
{
	SndStruct *soundstate = (SndStruct *)userdata;
	
	unsigned int i;

//	float phasez = 1/soundstate->sampfreq;

	float phasez0 = 0;
	float phasez1 = 0;
	float phasez2 = 0;
	float phasez3 = 0;
	float sampfreq = (float)soundstate->sampfreq;

	Sint16 *buffer = (Sint16 *)stream;
	
	
	if (soundstate->freq[0] > 0) {
		phasez0 = 1/sampfreq;
	}
	if (soundstate->freq[1] > 0) {
		phasez1 = 1/sampfreq;
	}
	if (soundstate->freq[2] > 0) {
		phasez2 = 1/sampfreq;
	}
	if (soundstate->freq[3] > 0) {
		phasez3 = 1/sampfreq;
	}
	
	
	
	for (i = 0; i < len/sizeof(Sint16) ; i++)
	{
		if (soundstate->freq[0] > 0) {
			soundstate->phase[0] = soundstate->phase[0] + phasez0;
			if (soundstate->phase[0] > (1.0/soundstate->freq[0])) {
				soundstate->phase[0] -= (float)(1.0/soundstate->freq[0]);
			}
		} else {
			soundstate->phase[0] = 0;
		}
		*buffer = *buffer + ((soundstate->phase[0] <= (1.0/soundstate->freq[0])/2 ? 1 : -1) * soundstate->amp[0]);
		
		if (soundstate->freq[1] > 0) {
			soundstate->phase[1] = soundstate->phase[1] + phasez1;
			if (soundstate->phase[1] > (1.0/soundstate->freq[1])) {
				soundstate->phase[1] -= (float)(1.0/soundstate->freq[1]);
			}
		} else {
			soundstate->phase[1] = 0;
		}
		*buffer = *buffer + ((soundstate->phase[1] <= (1.0/soundstate->freq[1])/2 ? 1 : -1) * soundstate->amp[1]);

		if (soundstate->freq[2] > 0) {
			soundstate->phase[2] = soundstate->phase[2] + phasez2;
			if (soundstate->phase[2] > (1.0/soundstate->freq[2])) {
				soundstate->phase[2] -= (float)(1.0/soundstate->freq[2]);
			}
		} else {
			soundstate->phase[2] = 0;
		}
		*buffer = *buffer + ((soundstate->phase[2] <= (1.0/soundstate->freq[2])/2 ? 1 : -1) * soundstate->amp[2]);

		if (soundstate->freq[3] > 0) {
			soundstate->phase[3] = soundstate->phase[3] + phasez3;
			if (soundstate->phase[3] > (1.0/soundstate->freq[3])) {
				soundstate->phase[3] -= (float)(1.0/soundstate->freq[3]);
				noiseTick();
			}
		} else {
			soundstate->phase[3] = 0;
		}
		*buffer = *buffer + ((((noiseshiftregister & 1) == 0 ? 1 : -1) * soundstate->amp[3]) - 3);
		
		buffer++;

	}
	

}

static void WriteVolume (int channel,int volume)
{
	if (!sound_on ||
		!channel_on[channel] || invalidfreq[channel])
		volume=0;
	volume=volume*mastervolume/15;
	SDL_LockAudio();	
	soundState.amp[channel] = volume;
	SDL_UnlockAudio();
	
}

static void WriteFreq (int channel,int freq)
{
	SDL_LockAudio();
	soundState.freq[channel] = freq;
	SDL_UnlockAudio();
}

static void ChannelOff (byte channel)
{
	WriteVolume (channel,0);
}

static void ChannelOn (byte channel)
{
	if (!sound_on)
		return;
	if ((channel==3 && channelfreq[channel]!=0) ||
		(channelfreq[channel]!=0 && channelfreq[channel]<=MAXFREQ))
	{
		invalidfreq[channel]=0;
		WriteFreq (channel,channelfreq[channel]);
		WriteVolume (channel,channelvolume[channel]);
	}
	else
	{
		WriteVolume (channel,0);
		invalidfreq[channel]=1;
	}
}

static void ToggleChannel (int channel)
{
	if (!sound_on)
		return;
	channel_on[channel]^=1;
	if (channel_on[channel])
		ChannelOn (channel);
	else
		ChannelOff (channel);
}

static void SoundOff (void)
{
	int i;
	for (i=0;i<4;++i)
		ChannelOff (i);
}

static void SoundOn (void)
{
	int i;
	if (!sound_on)
		return;
	for (i=0;i<4;++i)
		if (channel_on[i])
			ChannelOn (i);
}

static void Toggle (void)
{
	sound_on^=1;
	if (sound_on)
		SoundOn ();
	else
		SoundOff ();
}

static void SetVolume (byte channel,byte vol)
{
	const Uint16 voltabel[16]=
	{ 2550,2020,1610,1280,1010,800,640,510,400,320,250,200,160,130,100,0 };
	channelvolume[channel]=(vol>=15)? 0:voltabel[vol];
	WriteVolume (channel,channelvolume[channel]);
}

static void SetFreq (byte channel, word freq)
{
	if (!freq) freq=1024;
	ChannelFreq[channel]=freq;
	channelfreq[channel]=(freq)? (3579545/(32*(int)freq)):0;
	if (channel_on[channel])
	{
		ChannelOn (channel);
	}
	if (channel==2 && noise_output_channel_3)
		SetFreq (3,freq);
}

static void NoiseFeedback_On (void)
{
	if (feedback == 1)
		return;
	feedback = 1;
	noiseshiftregister = 0x8000;
}

static void NoiseFeedback_Off (void)
{
	if (feedback == 0)
		return;
	feedback = 0;
	noiseshiftregister = 0x8000;
}

static int Init (void)
{
 	SDL_AudioSpec *desired;
	SDL_AudioSpec *obtained;
	
	if (initialised)
		return 1;
	
	desired = malloc(sizeof(SDL_AudioSpec));
	
	if (desired == NULL) return 0;
	obtained = malloc(sizeof(SDL_AudioSpec));
	if (obtained == NULL) return 0;
	
	soundState.sampfreq = 44100;
	soundState.freq[1] = soundState.freq[2] = soundState.freq[3] = soundState.freq[0] = 0;
	soundState.phase[1] = soundState.phase[2] = soundState.phase[3] = soundState.phase[0] = 0;
	soundState.amp[0] = soundState.amp[2] = soundState.amp[3] = soundState.amp[1] = 1000;
	soundState.channels = 1;
	
	desired->freq = 44100;
	desired->format = AUDIO_S16SYS;
	desired->channels = 1;
	desired->samples = 1024;
//	desired->samples = 256;
	desired->callback = soundData;
	desired->userdata = (void *) &soundState;
	
	
	if (SDL_OpenAudio(desired,obtained) < 0) {
		if (Verbose) printf("Couldn't open audio\n");
	} else {
		if (Verbose) printf("OK\n  Opened audio...OK\n");
	}
	
	SDL_PauseAudio(0);
	
	noiseshiftregister = 0x8000; // always initialised to 0x8000 to start.
//	noiseshiftregister = 0x8000; // always initialised to 0x8000 to start.
	
	initialised = 1;
	return 1;
}

static void Reset (void)
{
}

static int parity(int val) {
    val ^= val >> 8;
    val ^= val >> 4;
    val ^= val >> 2;
    val ^= val >> 1;
    return val & 1;
};

static void noiseTick(void)
{
    noiseshiftregister=(noiseshiftregister>>1) |
	((feedback
	  ?parity(noiseshiftregister&0x09)
	  :noiseshiftregister&1)<<15);
}

static void NoiseControl (word v)
{
	if (v & 4)
		NoiseFeedback_On ();
	else
		NoiseFeedback_Off ();
	if ((v&3)==3)
	{
		if (noise_output_channel_3==0)
		{
			noise_output_channel_3=1;
			SetFreq (3,ChannelFreq[2]);
		}
	}
	else
	{
		noise_output_channel_3=0;
		SetFreq (3,NoiseFreqs[v&3]);
	}
	
}

static void WriteSoundReg (int r,int v)
{
	switch (r)
	{
		case 0:
			SetFreq (0,v);
			break;
		case 1:
			SetVolume (0,v);
			break;
		case 2:
			SetFreq (1,v);
			break;
		case 3:
			SetVolume (1,v);
			break;
		case 4:
			SetFreq (2,v);
			break;
		case 5:
			SetVolume (2,v);
			break;
		case 6:
			NoiseControl (v);
			break;
		case 7:
			SetVolume (3,v);
	}
}

static void SetMasterVolume (int newvolume)
{
	int i;
	for (i=0;i<4;++i)
		if (channel_on[i])
			ChannelOn (i);
}

static void Stop (void)
{
	SoundOff ();
}

static void Resume (void)
{
	SoundOn ();
}

struct SoundDriver_struct SB_SoundDriver=
{
	Init,
	Reset,
	WriteSoundReg,
	Toggle,
	ToggleChannel,
	SetMasterVolume,
	Stop,
	Resume
};
