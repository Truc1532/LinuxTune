AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = linuxtune
linuxtune_SOURCES = src/main.c

AM_CFLAGS = -Wall 
AM_LDFLAGS =  -lvorbisfile -lvorbis -lopus -lopusfile -logg -lmpg123 -lSDL2 

clean-local:
	rm -d -r autom4te.cache/
