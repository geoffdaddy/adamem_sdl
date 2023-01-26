#ifndef _ADAMSDLSOUND_H
#define _ADAMSDLSOUND_H

#include "Sound.h"
#include "SDL.h"
/*
 *  AdamSDLSound_2.h
 *  adamem
 *
 *  Created by Geoffrey Oltmans on 6/10/08.
 *  Copyright 2008 Geoffrey Oltmans. All rights reserved.
 *
 */

typedef struct _sndstruct {
		unsigned int sampfreq;
		unsigned int channels;
		Sint16 flipFlopState[4];
		unsigned int sampNum;
		int samplesBeforeTick;
		int timer[4];
		int timerReloadVal[4];
		unsigned int amp[4];
		Uint16 noiseshiftregister;
		Sint16 prevFlipFlopStateChan2;
	
#ifdef SOUND_PSG
		unsigned int PSG_sampNum;
		int PSG_samplesBeforeTick;
		Sint16 PSG_flipFlopState[4];
		Uint16 PSG_noiseshiftregister;
		unsigned int PSG_envelopeCounter;
		int PSG_timer[4];
		int PSG_timerReloadVal[4];
		int PSG_amp[4];
		Uint8 PSG_channelMode[3];
		Uint8 PSG_noiseEnable[3];
		Uint8 PSG_toneEnable[3];
		Uint16 PSG_envelopePeriod;
		Uint8 PSG_envelopeShape;
		int PSG_envelopeTimer;
#endif
		unsigned int ctrlVolume;
		unsigned int oldSampleVolume;
		unsigned int daVolume;
	
} SndStruct;

void soundData(void *userdata, Uint8 *stream, int len);

#endif