#include <pthread.h>
#include <gpiod.h>

#include "alsa.h"

#define GPIO_DEVICE "/dev/gpiochip0"
#define GPIO_OFFSET 26
#define GPIO_CONSUMER_NAME "floppy_sound"

#define ALSA_DEVICE "default"

#define WAV_FILE "floppy_sound.wav"

typedef struct wav_header_
{
  char str_riff[4]; // "RIFF"
  int wav_size;     // (file size) - 8
  char str_wave[4]; // "WAVE"

  // Format Header
  char str_fmt[4];    // "fmt "
  int fmt_chunk_size; // Should be 16 for PCM
  short audio_format; // Should be 1 for PCM. 3 for IEEE Float
  short channels;
  int sample_rate;        // ex: 8000, 44100, 48000
  int byte_rate;          // Number of bytes per second. sample_rate * channels * Bytes Per Sample
  short sample_alignment; // channels * Bytes Per Sample
  short bit_depth;        // bits per sample, ex: 8, 16, 24

  // Data
  char str_data[4]; // "data"
  int data_bytes;   // (file size) - 44
} wav_header;       // ensure (sizeof(wav_header) == 44)
_Static_assert((sizeof(wav_header) == 44), "sizeof wav_header must be 44");

// Playback variables
//
uint8_t *wav_buffer = NULL;
_Atomic(int) wav_offset = 0;

int gpio_event_callback(int event, unsigned int offset, const struct timespec *timestamp, void *unused)
{
  // Reset the playback offset to start from the beginning
  //
  wav_offset = 0;

  return GPIOD_CTXLESS_EVENT_CB_RET_OK;
}

void *playback_thread(void *param)
{
  int period_size = atoi(gExtraData[1]); // Frames
  int period_byte_count = gCurrentSampleFormat.channelCount * (gCurrentSampleFormat.bitsPerSample / 8) * period_size;
  int sleep_duration = (period_size * 1e6) / gCurrentSampleFormat.sampleRate; // Microseconds

  // Start in non-playing state
  //
  wav_offset = gCurrentSampleFormat.audioDataSize;

  while (true)
  {
    if (wav_offset < gCurrentSampleFormat.audioDataSize)
    {
      // Write one period per iteration
      //
      int offset = wav_offset;
      int byte_count = period_byte_count;
      int overrun = (offset + byte_count) - gCurrentSampleFormat.audioDataSize;

      if (overrun > 0)
      {
        byte_count -= overrun;
      }

      wav_offset += byte_count;

      if (!playAudio(wav_buffer + offset, byte_count))
      {
        fprintf(stderr, "Playback error");
        pthread_exit(NULL);
      }
    }
    else
    {
      // Audio finished playing, sleep until wav_offset is reset
      //
      usleep(sleep_duration);
    }
  }
}

int main(int argc, char *argv[])
{
  // Process arguments
  //
  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s period_count period_size\n", argv[0]);
    return 0;
  }

  gExtraDataCount = 2;
  gExtraData = (char **)malloc(gExtraDataCount);

  int i;
  for (i = 0; i < gExtraDataCount; ++i)
  {
    gExtraData[i] = (char *)malloc(strlen(argv[1 + i]) + 1);
    strcpy(gExtraData[i], argv[1 + i]);
  }

  // Open sound file
  //
  int error = 0;
  wav_header wav_info;

  FILE *f = fopen(WAV_FILE, "rb");
  if (f == NULL)
  {
    fprintf(stderr, "Could not open file: %s\n", WAV_FILE);
    error = 1;
  }

  if (!error)
  {
    // Read WAV header
    //
    int success = fread(&wav_info, 44, 1, f);
    if (success)
    {
      // Read audio data into buffer
      //
      wav_buffer = (unsigned char *)malloc(wav_info.data_bytes);
      long bytes_read = fread(wav_buffer, 1, wav_info.data_bytes, f);
      fclose(f);

      if (bytes_read == wav_info.data_bytes)
      {
        // Set sample format
        //
        gCurrentSampleFormat.bitsPerSample = wav_info.bit_depth;
        gCurrentSampleFormat.channelCount = wav_info.channels;
        gCurrentSampleFormat.sampleRate = wav_info.sample_rate;
        gCurrentSampleFormat.flags = 0;
        gCurrentSampleFormat.audioDataSize = wav_info.data_bytes;

        // Open audio device
        //
        error = !openAudioDevice();
        if (!error)
        {
          // Create playback thread
          //
          pthread_t thread;

          error = pthread_create(&thread, NULL, playback_thread, NULL);
          if (!error)
          {
            // Wait for floppy motor events. This function blocks.
            //
            error = gpiod_ctxless_event_monitor(GPIO_DEVICE,
                                                GPIOD_CTXLESS_EVENT_RISING_EDGE,
                                                GPIO_OFFSET,
                                                0,
                                                GPIO_CONSUMER_NAME,
                                                NULL,
                                                NULL,
                                                gpio_event_callback,
                                                NULL);
            if (error)
            {
              fprintf(stderr, "Could not start GPIO event monitor\n");
            }
          }
        }
        else
        {
          fprintf(stderr, "Could not create playback thread\n");
        }
      }
      else
      {
        fprintf(stderr, "Could not read audio data\n");
        error = 1;
      }
    }
    else
    {
      fprintf(stderr, "Could not read WAV header\n");
      error = 1;
    }
  }

  closeAudioDevice();
  free(wav_buffer);

  for (i = 0; i < gExtraDataCount; ++i)
    free(gExtraData[i]);

  free(gExtraData);

  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
