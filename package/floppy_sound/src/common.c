#include "common.h"

struct AudioFrameHeader gCurrentSampleFormat;
uint8_t *gAudioDataBuffer;
bool gPlayerIsConnected;
char **gExtraData;
int gExtraDataCount;