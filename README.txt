LinuxTune

----DEPENDENCIES----
	automake
	autoconf
	make
	mpg123 lib
	SDL2 lib
	GLEW (opengl) lib
	GL (opengl) lib
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
	This command is to install the program so you will be able to run the uap program from any directory.
	5. make clean
	This last command is to remove the autom4te.cache folder. 
---INFO---
	LinuxTune is a lightweight open source audio player made for linux that works both with command line and a GUI together.
	This version supports mp3, ogg and mp2 files. 
---USAGE---
	linuxtune <audiofile.mp3/mp2/ogg>
	
