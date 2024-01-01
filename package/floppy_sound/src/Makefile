CFLAGS = -Wall -lasound -lgpiod -lpthread
CC = gcc

.PHONY: clean

floppy_sound: floppy_sound.c alsa.c common.c
	$(CC) $(CFLAGS) -o $@ $^

clean:  
	rm floppy_sound floppy_sound.wav
