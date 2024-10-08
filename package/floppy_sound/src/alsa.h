/*
 *  alsa.h
 *  floppy_sound
 *
 *  Created by Peter Lundkvist on 17/05/2009.
 *
 *  This is free and unencumbered software released into the public domain.
 *  See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#ifndef Player_alsa_h
#define Player_alsa_h

#include <alsa/asoundlib.h>
#include "common.h"

// Global variables
//
extern snd_pcm_t *gAudioDevice;

// Functions
//
bool initAudioDevice();
bool openAudioDevice();
bool fillBuffer();
bool playAudio(uint8_t *inAudioDataBuffer, uint32_t inByteCount);
void closeAudioDevice();
void freeAudioDevice();

#endif
