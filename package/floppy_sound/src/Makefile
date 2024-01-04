CFLAGS = -Wall -lasound -lgpiod -lpthread
CC = gcc

.PHONY: clean

all: floppy_sound floppy_sound_debug

floppy_sound: floppy_sound.c alsa.c common.c
	$(CC) $(CFLAGS) -o $@ $^

floppy_sound_debug: floppy_sound.c alsa.c common.c
	$(CC) $(CFLAGS) -DPLAYER_DEBUG -o $@ $^

clean:  
	rm floppy_sound floppy_sound_debug
