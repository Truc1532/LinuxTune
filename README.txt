# Linux tune

----DEPENDENCIES----
	pkg config
	automake
	autoconf
	make
	mpg123 lib
	SDL2 lib
	vorbis lib
	vorbisfile lib
	ogg lib
---BUILD---
	Building the program is pretty easy. Here is the list of the commands:
	chmod +x autogen.sh
	Then to build the program enter:
	./autogen.sh
	If you already have all the dependencies the autogen should have completed with not errors. Now you can build the program by running this command:
	make
	And finally run:
	sudo make install
---INFO---
	Linux tune is a command line based, lightweight open source audio player
	made for linux. This version supports mp3, ogg, mp2 and wav files.
---USAGE---
	Enter this command
	linuxtune <audioFile.mp3/mp2/ogg/wav>
	(change audiofile with the actual file name and mp3/mp2/ogg/wav with the actual file type)

