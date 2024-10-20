#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/vorbisfile.h>
#include <mpg123.h>
#include <SDL2/SDL.h>
#include <time.h>
#include <termios.h>  
#include <unistd.h>   
#include <sys/select.h>

#define AUDIO_FORMAT AUDIO_S16SYS  
#define SEEK_SECONDS 5

typedef struct {
    OggVorbis_File vorbis_file;
    mpg123_handle *mpg123_handle;
    Uint8 *stream; 
    int len;
    int is_mp3;
    int is_mp2;
    double total_duration; 
    clock_t start_time;     
    int is_paused;           
} AudioData;

void disable_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ECHO | ICANON); 
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void restore_echo() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ECHO | ICANON); 
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int kbhit() {
    struct timeval tv = { 0L, 0L };  
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);  
    return select(1, &fds, NULL, NULL, &tv); 
}

#include "audioHandling.c"

void seek_audio(AudioData *audio_data, double offset) {
    if (audio_data->is_mp3 || audio_data->is_mp2) {
        mpg123_seek(audio_data->mpg123_handle, offset * 44100, SEEK_CUR);
    } else {
        double new_time = ov_time_tell(&audio_data->vorbis_file) + offset;
        if (new_time < 0) {
            new_time = 0;
        }
        ov_time_seek(&audio_data->vorbis_file, new_time);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <audio_file.ogg|mp3|mp2>\n", argv[0]);
        return -1;
    }

    const char *filename = argv[1];
    AudioData audio_data;
    memset(&audio_data, 0, sizeof(audio_data));
    audio_data.is_paused = 0;  

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    SDL_AudioSpec wav_spec;
    memset(&wav_spec, 0, sizeof(SDL_AudioSpec));
    wav_spec.format = AUDIO_FORMAT;
    wav_spec.samples = 4096;
    wav_spec.callback = audio_callback;
    wav_spec.userdata = &audio_data;

    const char *ext = strrchr(filename, '.');

    if (ext && (strcmp(ext, ".mp3") == 0 || strcmp(ext, ".mp2") == 0)) {
        if (mpg123_init() != MPG123_OK || (audio_data.mpg123_handle = mpg123_new(NULL, NULL)) == NULL) {
            fprintf(stderr, "Failed to initialize mpg123.\n");
            return -1;
        }
        if (mpg123_open(audio_data.mpg123_handle, filename) != MPG123_OK) {
            fprintf(stderr, "Could not open MP3/MP2 file: %s\n", filename);
            mpg123_delete(audio_data.mpg123_handle);
            return -1;
        }

        long rate;
        int channels, encoding;
        if (mpg123_getformat(audio_data.mpg123_handle, &rate, &channels, &encoding) != MPG123_OK) {
            fprintf(stderr, "Could not get MP3/MP2 format.\n");
            mpg123_delete(audio_data.mpg123_handle);
            return -1;
        }

        wav_spec.freq = rate;
        wav_spec.channels = channels;
        audio_data.is_mp3 = (strcmp(ext, ".mp3") == 0) ? 1 : 0;
        audio_data.is_mp2 = (strcmp(ext, ".mp2") == 0) ? 1 : 0;

        printf("Playing %s file: %s\n", audio_data.is_mp3 ? "MP3" : "MP2", filename);
        printf("Sample rate: %ld Hz\n", rate);
        printf("Channels: %d\n", channels);

    } else if (ext && strcmp(ext, ".ogg") == 0) {
        if (ov_fopen(filename, &audio_data.vorbis_file) < 0) {
            fprintf(stderr, "Could not open Ogg Vorbis file: %s\n", filename);
            return -1;
        }

        vorbis_info *info = ov_info(&audio_data.vorbis_file, -1);
        audio_data.total_duration = ov_time_total(&audio_data.vorbis_file, -1);

        wav_spec.freq = info->rate;
        wav_spec.channels = info->channels;
        audio_data.is_mp3 = 0;
        audio_data.is_mp2 = 0;

        printf("Playing Ogg Vorbis file: %s\n", filename);
        printf("Sample rate: %ld Hz\n", info->rate);
        printf("Channels: %d\n", info->channels);

    } else {
        fprintf(stderr, "Unsupported file format: %s\n", filename);
        return 1;
    }

    if (SDL_OpenAudio(&wav_spec, NULL) < 0) {
        fprintf(stderr, "Could not open audio: %s\n", SDL_GetError());
        if (audio_data.is_mp3 || audio_data.is_mp2) {
            mpg123_delete(audio_data.mpg123_handle);
        } else {
            ov_clear(&audio_data.vorbis_file);
        }
        return 1;
    }

    disable_echo();  

    SDL_PauseAudio(0);

    while (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING) {
         if (kbhit()) {
            char c = getchar();
            if (c == 'p') {
                audio_data.is_paused = !audio_data.is_paused;
            } else if (c == 27) {
                getchar();
                c = getchar();
                if (c == 'C') {
                    seek_audio(&audio_data, SEEK_SECONDS);
                } else if (c == 'D') {
                    seek_audio(&audio_data, -SEEK_SECONDS);
                }
            } else if (c == 'q') {
                break;
            }
        }
        print_audio_progress(&audio_data);
    }

    restore_echo();  

    if (audio_data.is_mp3 || audio_data.is_mp2) {
        mpg123_close(audio_data.mpg123_handle);
        mpg123_delete(audio_data.mpg123_handle);
        mpg123_exit();
    } else {
        ov_clear(&audio_data.vorbis_file);
    }

    SDL_CloseAudio();
    SDL_Quit();

    return 0;
}
