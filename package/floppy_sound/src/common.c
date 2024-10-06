/*
 *  common.c
 *  floppy_sound
 *
 *  Created by Peter Lundkvist on 17/05/2009.
 *  Copyright 2009 Peter Lundkvist. All rights reserved.
 *
 */

#include "common.h"

struct AudioFrameHeader gCurrentSampleFormat;
uint8_t *gAudioDataBuffer;
bool gPlayerIsConnected;
char **gExtraData;
int gExtraDataCount;