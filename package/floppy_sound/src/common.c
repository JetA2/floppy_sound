/*
 *  common.c
 *  floppy_sound
 *
 *  Created by Peter Lundkvist on 17/05/2009.
 *
 *  This is free and unencumbered software released into the public domain.
 *  See the file COPYING for more details, or visit <http://unlicense.org>.
 */

#include "common.h"

struct AudioFrameHeader gCurrentSampleFormat;
uint8_t *gAudioDataBuffer;
bool gPlayerIsConnected;
char **gExtraData;
int gExtraDataCount;