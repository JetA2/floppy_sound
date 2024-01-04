/*
 *  alsa.c
 *  Player
 *
 *  Created by Peter Lundkvist on 17/05/2009.
 *  Copyright 2009 Peter Lundkvist. All rights reserved.
 *
 */

#include "alsa.h"

#ifdef PLAYER_DEBUG
#include <time.h> // For clock_gettime
#endif

snd_pcm_t *gAudioDevice = NULL;

// Unit global variables
//
snd_pcm_uframes_t gBufferSizeFrames;
snd_pcm_format_t gALSASampleFormat;
bool gUse32BitContainerFor24BitSamples;

#ifdef PLAYER_DEBUG
struct timespec gLastPlayTime;
#endif

// Audio interface functions for ALSA
//
bool initAudioDevice()
{
	return true;
}

bool openAudioDevice()
{
	int result = snd_pcm_open(&gAudioDevice, "default", SND_PCM_STREAM_PLAYBACK, 0);

	if (result < 0)
	{
		fprintf(stderr, "Unable to open pcm device: %s\n", snd_strerror(result));
		return false;
	}

	// Allocate hw_params on the stack, no need to free it
	//
	snd_pcm_hw_params_t *hwParams;
	snd_pcm_hw_params_alloca(&hwParams);

	// Set hardware parameters
	//
	result = snd_pcm_hw_params_any(gAudioDevice, hwParams);
	if (result < 0)
		fprintf(stderr, "snd_pcm_hw_params_any failed: %s\n", snd_strerror(result));

	result = snd_pcm_hw_params_set_access(gAudioDevice, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (result < 0)
		fprintf(stderr, "snd_pcm_hw_params_set_access failed: %s\n", snd_strerror(result));

	result = snd_pcm_hw_params_set_rate_resample(gAudioDevice, hwParams, 0);
	if (result < 0)
		fprintf(stderr, "snd_pcm_hw_params_set_rate_resample failed: %s\n", snd_strerror(result));

	// Set sample format
	//
	gUse32BitContainerFor24BitSamples = false;

	if (gCurrentSampleFormat.bitsPerSample == 8)
	{
		gALSASampleFormat = SND_PCM_FORMAT_S8;
	}
	else if (gCurrentSampleFormat.bitsPerSample == 16)
	{
		gALSASampleFormat = SND_PCM_FORMAT_S16_LE;
	}
	else if (gCurrentSampleFormat.bitsPerSample == 24)
	{
		if (snd_pcm_hw_params_test_format(gAudioDevice, hwParams, SND_PCM_FORMAT_S24_3LE) != 0)
		{
			gUse32BitContainerFor24BitSamples = true; // 3 byte sample format not available
		}

		gALSASampleFormat = gUse32BitContainerFor24BitSamples ? SND_PCM_FORMAT_S24_LE : SND_PCM_FORMAT_S24_3LE;
	}
	else
	{
		gALSASampleFormat = SND_PCM_FORMAT_UNKNOWN;
	}

	result = snd_pcm_hw_params_set_format(gAudioDevice, hwParams, gALSASampleFormat);
	if (result < 0)
		fprintf(stderr, "snd_pcm_hw_params_set_format failed: %s\n", snd_strerror(result));

	result = snd_pcm_hw_params_set_channels(gAudioDevice, hwParams, gCurrentSampleFormat.channelCount);
	if (result < 0)
		fprintf(stderr, "snd_pcm_hw_params_set_channels failed: %s\n", snd_strerror(result));

	result = snd_pcm_hw_params_set_rate(gAudioDevice, hwParams, gCurrentSampleFormat.sampleRate, 0);
	if (result < 0)
		fprintf(stderr, "snd_pcm_hw_params_set_rate failed: %s\n", snd_strerror(result));

	// Set buffering parameters
	//
	if (gExtraDataCount >= 2)
	{
		int periodCount = atoi(gExtraData[0]);
		int periodSize = atoi(gExtraData[1]);

		result = snd_pcm_hw_params_set_periods(gAudioDevice, hwParams, periodCount, 0);
		if (result < 0)
			fprintf(stderr, "snd_pcm_hw_params_set_periods failed: %s\n", snd_strerror(result));

		result = snd_pcm_hw_params_set_period_size(gAudioDevice, hwParams, periodSize, 0);
		if (result < 0)
			fprintf(stderr, "snd_pcm_hw_params_set_period_size failed: %s\n", snd_strerror(result));
	}

	// Write parameters
	//
	result = snd_pcm_hw_params(gAudioDevice, hwParams);
	if (result < 0)
	{
		fprintf(stderr, "Unable to set hw parameters: %s\n", snd_strerror(result));
		return false;
	}

	// Allocate sw_params on the stack, no need to free it
	//
	snd_pcm_sw_params_t *swParams;
	snd_pcm_sw_params_alloca(&swParams);

	// Set software parameters
	//
	result = snd_pcm_sw_params_current(gAudioDevice, swParams);
	if (result < 0)
		fprintf(stderr, "snd_pcm_sw_params_any failed: %s\n", snd_strerror(result));

	// Wait until buffer is full before starting playback
	//
	result = snd_pcm_hw_params_get_buffer_size(hwParams, &gBufferSizeFrames);
	if (result < 0)
		fprintf(stderr, "snd_pcm_hw_params_get_buffer_size failed: %s\n", snd_strerror(result));

	result = snd_pcm_sw_params_set_start_threshold(gAudioDevice, swParams, gBufferSizeFrames);
	if (result < 0)
		fprintf(stderr, "snd_pcm_sw_params_set_start_threshold failed: %s\n", snd_strerror(result));

	// Wait until buffer is empty before stopping playback
	//
	result = snd_pcm_sw_params_set_stop_threshold(gAudioDevice, swParams, gBufferSizeFrames);
	if (result < 0)
		fprintf(stderr, "snd_pcm_sw_params_set_stop_threshold failed: %s\n", snd_strerror(result));

	// Write parameters
	//
	result = snd_pcm_sw_params(gAudioDevice, swParams);
	if (result < 0)
	{
		fprintf(stderr, "Unable to set sw parameters: %s\n", snd_strerror(result));
		return false;
	}

	// Prepare audio interface for use
	//
	result = snd_pcm_prepare(gAudioDevice);
	if (result < 0)
	{
		fprintf(stderr, "Cannot prepare audio interface for use: %s\n", snd_strerror(result));
		return false;
	}

#ifdef PLAYER_DEBUG

	// Display parameters
	//
	int dir;
	unsigned int val1, val2;
	snd_pcm_uframes_t frames;
	snd_pcm_format_t format;

	snd_pcm_hw_params_get_format(hwParams, &format);
	printf("\nSample format: %s\n", snd_pcm_format_name(format));

	val1 = 0;
	val2 = 0;
	snd_pcm_hw_params_get_rate_numden(hwParams, &val1, &val2);
	printf("Sample rate: %f Hz\n", (double)val1 / (double)val2);

	val1 = 0;
	snd_pcm_hw_params_get_rate_resample(gAudioDevice, hwParams, &val1);
	if (val1)
		printf("Resampling IS active\n");
	else
		printf("Resampling is NOT active\n");

	val1 = 0;
	snd_pcm_hw_params_get_periods(hwParams, &val1, &dir);
	printf("Periods: %i\n", val1);

	frames = 0;
	snd_pcm_hw_params_get_period_size(hwParams, &frames, &dir);
	printf("Period size: %li frames\n", (unsigned long)frames);

	frames = 0;
	snd_pcm_hw_params_get_buffer_size(hwParams, &frames);
	printf("Buffer size: %li frames\n", (unsigned long)frames);

	val1 = 0;
	snd_pcm_hw_params_get_period_time(hwParams, &val1, &dir);
	printf("Period time: %i us\n", val1);

	val1 = 0;
	snd_pcm_hw_params_get_buffer_time(hwParams, &val1, &dir);
	printf("Buffer time: %i us\n", val1);

	snd_pcm_sw_params_current(gAudioDevice, swParams);

	frames = 0;
	snd_pcm_sw_params_get_start_threshold(swParams, &frames);
	printf("Start threshold: %li frames\n", (unsigned long)frames);

	frames = 0;
	snd_pcm_sw_params_get_stop_threshold(swParams, &frames);
	printf("Stop threshold: %li frames\n", (unsigned long)frames);

#endif

	return true;
}

void repack24BitTo32Bit(uint8_t *inAudioDataBuffer, uint32_t inByteCount, uint8_t *outResultBuffer)
{
	// Set the most significant byte (highest memory address) to zero
	//
	static const uint32_t sampleSize = 3;
	uint32_t paddedOffset = 0;
	uint32_t i = 0;

	for (i = 0; i < inByteCount; i += sampleSize)
	{
		memcpy(&outResultBuffer[paddedOffset], &inAudioDataBuffer[i], sampleSize);
		outResultBuffer[paddedOffset + sampleSize] = 0;

		paddedOffset += (sampleSize + 1);
	}
}

bool internalPlayAudio(uint8_t *inAudioDataBuffer, uint32_t inByteCount)
{
#ifdef PLAYER_DEBUG
	snd_pcm_sframes_t framesInBuffer = gBufferSizeFrames - snd_pcm_avail(gAudioDevice);

	printf("\rBuffer Level: %i%% ", (int)(framesInBuffer * 100) / (int)gBufferSizeFrames);
	fflush(stdout);
#endif

	snd_pcm_uframes_t frameCount = snd_pcm_bytes_to_frames(gAudioDevice, inByteCount);
	snd_pcm_uframes_t framesWritten = 0;

	while (framesWritten < frameCount)
	{
		snd_pcm_uframes_t framesToWrite = frameCount - framesWritten;
		int result = snd_pcm_writei(gAudioDevice, inAudioDataBuffer + snd_pcm_frames_to_bytes(gAudioDevice, framesWritten), framesToWrite);

		if (result < 0)
		{
			if (result == -EPIPE) // Buffer underrun, need to prepare the device again before next write
			{
#ifdef PLAYER_DEBUG
				struct timespec time;
				if (clock_gettime(CLOCK_REALTIME, &time) != 0)
					perror("clock_gettime failed");

				int lastPlayTimeUsec = (gLastPlayTime.tv_sec * 1000000) + (gLastPlayTime.tv_nsec / 1000);
				int nowUsec = (time.tv_sec * 1000000) + (time.tv_nsec / 1000);

				fprintf(stderr, "Buffer underrun, time since last audio write call: %i usec\n", (nowUsec - lastPlayTimeUsec));
#endif
				result = snd_pcm_recover(gAudioDevice, result, 1);
				if (result < 0)
				{
					fprintf(stderr, "Cannot recover audio interface after buffer underrun: %s\n", snd_strerror(result));
					return false;
				}
			}
			else
			{
				fprintf(stderr, "Write to audio interface failed: %s\n", snd_strerror(result));
				return false;
			}
		}
		else
		{
			framesWritten += result;

#ifdef PLAYER_DEBUG
			if (result != framesToWrite)
			{
				printf("Interrupted write, wrote %i frames out of %i\n", result, (int)framesToWrite);
			}
#endif
		}
	}

#ifdef PLAYER_DEBUG
	if (clock_gettime(CLOCK_REALTIME, &gLastPlayTime) != 0)
		perror("clock_gettime failed");
#endif

	return true;
}

bool fillBuffer()
{
	// Get space available in buffer
	//
	snd_pcm_sframes_t framesAvailable = gBufferSizeFrames;

	if (snd_pcm_state(gAudioDevice) == SND_PCM_STATE_RUNNING)
		framesAvailable = snd_pcm_avail(gAudioDevice);

	if (framesAvailable < 0)
	{
		fprintf(stderr, "snd_pcm_avail failed: %s\n", snd_strerror(framesAvailable));
		return false;
	}
	else
	{
		ssize_t bytesAvailable = snd_pcm_frames_to_bytes(gAudioDevice, framesAvailable);
		bool playResult = true;

		if (bytesAvailable)
		{
			// Allocate memory
			//
			uint8_t *silenceBuffer = (uint8_t *)malloc(bytesAvailable);

			// Fill with silence
			//
			long sampleCount = snd_pcm_bytes_to_samples(gAudioDevice, bytesAvailable);

			int result = snd_pcm_format_set_silence(gALSASampleFormat, silenceBuffer, sampleCount);
			if (result != 0)
			{
				fprintf(stderr, "snd_pcm_format_set_silence failed: %s\n", snd_strerror(framesAvailable));

				free(silenceBuffer);
				return false;
			}

			// Add silence to buffer
			//
			playResult = internalPlayAudio(silenceBuffer, bytesAvailable);
			free(silenceBuffer);
		}

#ifdef PLAYER_DEBUG
		printf("\nBuffer sync, %i bytes\n", (int)bytesAvailable);
		fflush(stdout);
#endif

		return playResult;
	}
}

bool playAudio(uint8_t *inAudioDataBuffer, uint32_t inByteCount)
{
	if (!gUse32BitContainerFor24BitSamples)
	{
		return internalPlayAudio(inAudioDataBuffer, inByteCount);
	}
	else
	{
		// Allocate a new buffer for the 32 bit samples
		//
		uint32_t sampleCount = (inByteCount / 3);							 // Total samples for all channels
		uint32_t paddedBufferSize = inByteCount + sampleCount; // 1 extra byte per sample

		uint8_t *paddedBuffer = (uint8_t *)malloc(paddedBufferSize);

		// Repack samples
		//
		repack24BitTo32Bit(inAudioDataBuffer, inByteCount, paddedBuffer);

		// Play padded samples
		//
		bool result = internalPlayAudio(paddedBuffer, paddedBufferSize);

		free(paddedBuffer);
		return result;
	}
}

void closeAudioDevice()
{
	if (gAudioDevice)
	{
		snd_pcm_drain(gAudioDevice);
		snd_pcm_close(gAudioDevice);
	}
}

void freeAudioDevice()
{
	// Not required
}
