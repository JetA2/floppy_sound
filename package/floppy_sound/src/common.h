/*
 *  common.h
 *  floppy_sound
 *
 *  Created by Peter Lundkvist on 17/05/2009.
 *
 *  This is free and unencumbered software released into the public domain.
 *  See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#ifndef Player_common_h
#define Player_common_h

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Audio frame header struct
//
#define PLAYER_AUDIO_FRAME_HEADER_SIZE 11 // Number of actual bytes sent over the network for the audio frame header

struct AudioFrameHeader
{
	uint8_t flags;
	uint32_t sampleRate;
	uint8_t channelCount;
	uint8_t bitsPerSample;
	uint32_t audioDataSize;
};

// Header flags
//
#define HEADER_FLAG_SYNC 0x01

// Global variables
//
extern struct AudioFrameHeader gCurrentSampleFormat;
extern uint8_t *gAudioDataBuffer;
extern bool gPlayerIsConnected;
extern char **gExtraData;
extern int gExtraDataCount;

#endif
