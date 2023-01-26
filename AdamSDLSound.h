#ifndef _ADAMSDLSOUND_H
#define _ADAMSDLSOUND_H

#include "Sound.h"
#include "SDL.h"
/*
 *  AdamSDLSound.h
 *  adamem
 *
 *  Created by Geoffrey Oltmans on 6/10/08.
 *  Copyright 2008 Geoffrey Oltmans. All rights reserved.
 *
 */

typedef struct _sndstruct {
		unsigned int sampfreq;
		unsigned int channels;
		float phase[4];
		unsigned int freq[4];
		unsigned int amp[4];

} SndStruct;

void soundData(void *userdata, Uint8 *stream, int len);

#endif
