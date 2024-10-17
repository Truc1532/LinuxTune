AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = linuxtune
linuxtune_SOURCES = src/main.c

texturesdir = $(bindir)/textures
install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(texturesdir)
	cp -r textures/* $(DESTDIR)$(texturesdir)


AM_CFLAGS = -Wall
AM_LDFLAGS = -lvorbisfile -lvorbis -logg -lmpg123 -lSDL2 -lGL -lGLEW

clean-local:
	rm -rf autom4te.cache/
