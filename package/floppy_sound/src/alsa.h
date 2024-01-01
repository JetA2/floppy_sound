/*
 *  alsa.h
 *  Player
 *
 *  Created by Peter Lundkvist on 17/05/2009.
 *  Copyright 2009 Peter Lundkvist. All rights reserved.
 *
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
