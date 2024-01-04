CFLAGS = -Wall -lasound -lgpiod -lpthread
CC = gcc

.PHONY: clean

all: floppy_sound

debug: CFLAGS += -DPLAYER_DEBUG
debug: floppy_sound

floppy_sound: floppy_sound.c alsa.c common.c
	$(CC) $(CFLAGS) -o $@ $^

clean:  
	rm floppy_sound
