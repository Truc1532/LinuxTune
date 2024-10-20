# Linux tune command line edition

----DEPENDENCIES----
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
	1. chmod +x autogen.sh 
	This first command is to let autogen.sh to run as a program
	2. ./autogen.sh
	This second command is to run the autogen.sh. This will check if you have the required libraries and create the makefile
	3. make
	This command is to compile the main.c file into an executable
	4. sudo make install
	The last command is to install the program so you will be able to run the uap program from any directory.
---INFO---
	Linux tune is a command line based, lightweight open source audio player
	made for linux. This version supports mp3, ogg and mp2 files.
---USAGE---
	Enter this command
	linuxtune <audioFile.mp3/mp2/ogg>
	(change audiofile with the actual file name and mp3/mp2/ogg with the actual file type)
	
