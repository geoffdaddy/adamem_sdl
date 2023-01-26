/*
 *  AdamSDLSound_2.c
 *  adamem
 *
 *  Created by Geoffrey Oltmans on 6/10/08.
 *  Copyright 2008 Geoffrey Oltmans. All rights reserved.
 *
 */

#include "AdamSDLSound_2.h"
#include "math.h"
#include "Z80.h"
#include "Coleco.h"


//#define MAXINT			2^10-1
#define MAXINT			0x3ff
#define CLOCK_FREQ		3579545
#define PSG_CLOCK_FREQ		1789772

static int initialised;
static SndStruct soundState;


int soundquality=3;
static int invalidfreq[4];
static int feedback=1;
static Uint8 channel_on[4]= {1,1,1,1};
static Uint8 noise_output_channel_3=0;
static unsigned channelvolume[4];
static unsigned ChannelFreq[4];
static unsigned NoiseFreqs[4]= {32,64,128,0};
static Uint8 sound_on=1;
static void noiseTick(void);
#ifdef SOUND_PSG
static void PSG_noiseTick(void);
extern byte PSGReg[16];

//The numbers in this table correspond to the volume for the portion of the
// wave. The wave table encompasses two periods of each waveform (to make it 
// easier to repeat if necessary...

const Uint8 PSG_envelopeWaves[16][32] =
{
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15},
	{15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15}, 
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  
};

const Uint16 PSG_voltable[16]=
{ 0, 100, 130, 160, 200, 250, 320, 400, 510, 640, 800, 1010, 1280, 1610,2020,2550};

Uint8 PSG_getEnvelope(void)
{
	//PSG_Shape comes straight from register 13 of the PSG. Using the previous state and the
	// above table, we will return the new state of the waveform to be amplified by the clockSound()
	// code.
	return PSG_envelopeWaves[soundState.PSG_envelopeShape][soundState.PSG_envelopeCounter];
}

static void PSG_EnvelopeTick(void)
{
	Uint8 PSG_Continue = soundState.PSG_envelopeShape & 0x08;
	Uint8 PSG_Hold = soundState.PSG_envelopeShape & 0x01;
	
	if (soundState.PSG_envelopeCounter == 31)
	{
		if (PSG_Continue && !PSG_Hold)
		{
			soundState.PSG_envelopeCounter = 0;
		}		
	} else {
		soundState.PSG_envelopeCounter++;
	}
}

#endif

static void clockSound(void)
{
	int i;
	soundState.sampNum++;
	
	for (i = 0; i<4; i++)
	{
		if ((soundState.timer[i] -= soundState.samplesBeforeTick) <= 0)
//		if ((soundState.timer[i]-- ) <= 0)
		{
				if (soundState.timerReloadVal[i] <= 0) {
					soundState.flipFlopState[i] = 1;
					soundState.timer[i] = 0;
				} else {
					if (soundState.timerReloadVal[i] < soundState.samplesBeforeTick) {
						soundState.timer[i] = soundState.timerReloadVal[i];
					} else {
						soundState.timer[i] += soundState.timerReloadVal[i];
						soundState.flipFlopState[i] = soundState.flipFlopState[i] == 1 ? 0 : 1;
						if (i == 3 && soundState.flipFlopState[i]) {
							noiseTick();
						}
					}

				}
				
		}
	}
	//now the PSG...
#ifdef SOUND_PSG
	for (i = 0; i<4; i++)
	{
		if ((soundState.PSG_timer[i] -= soundState.PSG_samplesBeforeTick) <= 0)
		{
			if (soundState.PSG_timerReloadVal[i] <= 0) {
				soundState.PSG_flipFlopState[i] = 1;
				soundState.PSG_timer[i] = 0;
			} else {
				if (soundState.PSG_timerReloadVal[i] < soundState.PSG_samplesBeforeTick) {
					soundState.PSG_timer[i] = soundState.PSG_timerReloadVal[i];
				} else {
					soundState.PSG_timer[i] += soundState.PSG_timerReloadVal[i];
					soundState.PSG_flipFlopState[i] = soundState.PSG_flipFlopState[i] ? 0 : 1;
					if (i == 3 && soundState.PSG_flipFlopState[i]) {
						PSG_noiseTick();
					}
				}
				
			}
			
		}
	}

	if ((soundState.PSG_envelopeTimer -= soundState.PSG_samplesBeforeTick) <= 0)
	{
		if (soundState.PSG_envelopePeriod <= 0) {
			soundState.PSG_envelopeTimer = 0;
		} else {
			if (soundState.PSG_envelopePeriod < soundState.PSG_samplesBeforeTick) {
				soundState.PSG_envelopeTimer = soundState.PSG_envelopePeriod;
			} else {
				soundState.PSG_envelopeTimer += soundState.PSG_envelopePeriod;
				PSG_EnvelopeTick();
			}
			
		}
		
	}
	
#endif	
	
	
}

// Sound callback
void soundData(void *userdata, Uint8 *stream, int len)
{
	SndStruct *soundstate = (SndStruct *)userdata;
    unsigned int i,j,k;
	Sint16 sample = 0;

	Sint16 *buffer = (Sint16 *)stream;

	for (i = 0; i < len/sizeof(Sint16) ; i++)
	{
		clockSound();
		sample = 0;
        for ( j = 0; j < 4; j++)
        {
			if ( j != 3) {
				if (ChannelFreq[j] != 0) {
					sample += ((soundstate->flipFlopState[j] ? -1 : 1) * soundstate->amp[j]);
				} else {
					sample += soundstate->amp[j] ? soundstate->amp[j] : 0;
				}

			}
			else
				sample += (((soundstate->noiseshiftregister & 1) == 0 ? -1 : 1) * soundstate->amp[j]);
        }

#ifdef SOUND_PSG
		for ( j = 0; j < 3; j++)
        {
				if (soundstate->PSG_channelMode[j]) //envelope
				{
					k = PSG_getEnvelope();
					sample += ( ((soundstate->PSG_flipFlopState[j] && soundstate->PSG_toneEnable[j]) ||
							    ((soundstate->PSG_noiseshiftregister & 1) == 0 ? 0 : 1) && soundstate->PSG_noiseEnable[j])) * PSG_voltable[k];
				} else { //use normal volume...
					sample += (((soundstate->PSG_flipFlopState[j] && soundstate->PSG_toneEnable[j]) ||
								((soundstate->PSG_noiseshiftregister & 1) == 0 ? 0 : 1) && soundstate->PSG_noiseEnable[j])) * PSG_voltable[soundstate->PSG_amp[j]];
				}
		}
#endif
		
		*buffer++ = sample;

	}


}

static void WriteVolume (int channel,int volume)
{
	if (!sound_on ||
		!channel_on[channel] || invalidfreq[channel])
		volume=0;
	volume=volume*mastervolume/15;
	soundState.amp[channel] = volume;
}

static void WriteFreq (int channel,int freq)
{
	soundState.timerReloadVal[channel] = freq;
}

static void ChannelOff (byte channel)
{
	WriteVolume (channel,0);
}

static void ChannelOn (byte channel)
{
	if (!sound_on)
		return;
	if (((channel==3) && (ChannelFreq[channel]!=0)) ||
        (ChannelFreq[channel]<=MAXINT))
	{
		invalidfreq[channel]=0;
		WriteFreq (channel,ChannelFreq[channel]);
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
	ChannelFreq[channel]=freq;
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
}

static void NoiseFeedback_Off (void)
{
	if (feedback == 0)
		return;
	feedback = 0;
}

static int Init (void)
{
 	SDL_AudioSpec *desired;
	SDL_AudioSpec *obtained;
	SDL_AudioDeviceID dev;


	if (initialised)
		return 1;

	desired = malloc(sizeof(SDL_AudioSpec));

	if (desired == NULL) return 0;
	obtained = malloc(sizeof(SDL_AudioSpec));
	if (obtained == NULL) return 0;

	soundState.sampfreq = 44100;
	soundState.flipFlopState[0] = soundState.flipFlopState[1] = soundState.flipFlopState[2] = soundState.flipFlopState[3] = 1;
	soundState.amp[0] = soundState.amp[1] = soundState.amp[2] = soundState.amp[3] = 1000;
	soundState.channels = 1;
	soundState.sampNum = 0;
	soundState.samplesBeforeTick = CLOCK_FREQ / 16 / 44100; // Should be about 5
	soundState.timer[0] = soundState.timer[1] = soundState.timer[2] = soundState.timer[3] = 0;
	soundState.timerReloadVal[0] = soundState.timerReloadVal[1] = soundState.timerReloadVal[2] = soundState.timerReloadVal[3] = 0;
    soundState.noiseshiftregister = 0x4000; // always initialised to 0x4000 to start.
	soundState.daVolume = 0;
	soundState.ctrlVolume = 0;
	soundState.oldSampleVolume = 0;
	
#ifdef SOUND_PSG
	soundState.PSG_flipFlopState[0] = soundState.PSG_flipFlopState[1] = soundState.PSG_flipFlopState[2] = soundState.PSG_flipFlopState[3] = 1;
	soundState.PSG_amp[0] = soundState.PSG_amp[1] = soundState.PSG_amp[2] = soundState.PSG_amp[3] = 0;
	soundState.PSG_timer[0] = soundState.PSG_timer[1] = soundState.PSG_timer[2] = soundState.PSG_timer[3] = 0;
	soundState.PSG_timerReloadVal[0] = soundState.PSG_timerReloadVal[1] = soundState.PSG_timerReloadVal[2] = soundState.PSG_timerReloadVal[3] = 0;
	soundState.PSG_noiseshiftregister = 0x8000; // always initialised to 0x8000 to start.
	soundState.PSG_envelopeCounter = 0;
	soundState.PSG_channelMode[0] = soundState.PSG_channelMode[1] = soundState.PSG_channelMode[2] = 0;
	soundState.PSG_samplesBeforeTick = PSG_CLOCK_FREQ / 8 / 44100; // Should be about 5
#endif

	desired->freq = 44100;
	desired->format = AUDIO_S16SYS;
	desired->channels = 1;
	desired->samples = 512;
	desired->callback = soundData;
	desired->userdata = (void *) &soundState;

	if(!(dev = SDL_OpenAudioDevice(NULL, 0, desired, obtained, 0/*SDL_AUDIO_ALLOW_FORMAT_CHANGE*/))) {
		if (Verbose) printf("Couldn't open audio\n");
	} else {
		if (Verbose) printf("OK\n  Opened audio...OK\n");
	}

	SDL_PauseAudioDevice(dev, 0); /* start audio playing. */

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
	soundState.noiseshiftregister=(soundState.noiseshiftregister>>1) |
		((feedback
		?parity(soundState.noiseshiftregister&0x03)
		:soundState.noiseshiftregister&1)<<14);
}

#ifdef SOUND_PSG
static int PSG_parity(int val) {
    val ^= val >> 17;
    val ^= val >> 6;
    val ^= val & 1;
    return val & 1;
};

static void PSG_noiseTick(void)
{
	soundState.PSG_noiseshiftregister=(soundState.PSG_noiseshiftregister>>1) |
	((parity(soundState.PSG_noiseshiftregister&0x09) << 15));
}


static void PsgSetTonePeriodFine (int channel, Uint8 val)
{
	int temp = soundState.PSG_timerReloadVal[channel] & 0xf00;
	temp |= val;
	
	soundState.PSG_timerReloadVal[channel] = temp;
}

static void PsgSetTonePeriodCoarse (int channel, Uint8 val)
{
	int temp = soundState.PSG_timerReloadVal[channel] & 0xff;
	temp |= (val&0x0f)<<8;
	
	soundState.PSG_timerReloadVal[channel] = temp;
}

static void PsgSetNoisePeriod (Uint8 val)
{
	soundState.PSG_timerReloadVal[3] = val;
}

static void PsgMixerControl (Uint8 val)
{
	soundState.PSG_toneEnable[0] = val&0x1 ? 0 : 1;
	soundState.PSG_toneEnable[1] = val&0x2 ? 0 : 1;
	soundState.PSG_toneEnable[2] = val&0x4 ? 0 : 1;

	soundState.PSG_noiseEnable[0] = val&0x8 ? 0 : 1;
	soundState.PSG_noiseEnable[1] = val&0x10 ? 0 : 1;
	soundState.PSG_noiseEnable[2] = val&0x20 ? 0 : 1;

}

static void PSGSetVolume (int channel, Uint8 val)
{
	soundState.PSG_amp[channel] = val&0x0f;
	soundState.PSG_channelMode[channel] = val&0x10 ? 1 : 0;
}

static void PSGEnvelopePeriodFine (Uint8 val)
{
	int temp = soundState.PSG_envelopePeriod & 0xf00;
	temp |= val;
	
	soundState.PSG_envelopePeriod = temp/16;
	
}

static void PSGEnvelopePeriodCoarse (Uint8 val)
{
	int temp = soundState.PSG_envelopePeriod & 0xff;
	temp |= val<<8;
	
	soundState.PSG_envelopePeriod = temp/16;
	
	
}

static void PSGEnvelopeShapeControl (Uint8 val)
{
	soundState.PSG_envelopeShape = (val&0x0f);
}




#endif

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
	soundState.noiseshiftregister = 0x4000;
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

static void PSGWriteSoundReg (int r,int v)
{
	switch (r)
	{
		case 0:
			PsgSetTonePeriodFine (0,v);
			break;
		case 1:
			PsgSetTonePeriodCoarse (0,v);
			break;
		case 2:
			PsgSetTonePeriodFine (1,v);
			break;
		case 3:
			PsgSetTonePeriodCoarse (1,v);
			break;
		case 4:
			PsgSetTonePeriodFine (2,v);
			break;
		case 5:
			PsgSetTonePeriodCoarse (2,v);
			break;
		case 6:
			PsgSetNoisePeriod (v);
			break;
		case 7:
			PsgMixerControl (v);
			break;
		case 8:
			PSGSetVolume (0,v);
			break;
		case 9:
			PSGSetVolume (1,v);
			break;
		case 10:
			PSGSetVolume (2,v);
			break;
		case 11:
			PSGEnvelopePeriodFine (v);
			break;
		case 12:
			PSGEnvelopePeriodCoarse (v);
			break;
		case 13:
			PSGEnvelopeShapeControl (v);
			break;
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
	Resume,
	PSGWriteSoundReg
};

