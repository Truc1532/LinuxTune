#!/bin/bash

LIBRARIES=("vorbis" "vorbisfile" "sdl2" "libmpg123" "ogg")

MESSAGE=("vorbis library required to continue the build"
	 "Vorbisfile library required to continue the build"
	 "SDL2 library required to continue the build"
	 "libmpg123 library required to continue the build"
	 "ogg library required to continue the build")


for i in "${!LIBRARIES[@]}"; do
	LIBRARY=${LIBRARIES[$i]}
	MESSAGE=${MESSAGE[$i]}

	if pkg-config --exists "$LIBRARY"; then
		echo "$LIBRARY installed: yes"
	else 
		echo "$MESSAGE"
		exit 1
	fi
done

gcc src/main.c -lvorbis -lvorbisfile -logg -lmpg123 -lSDL2 -o linuxtune


