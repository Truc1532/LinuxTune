#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <vorbis/vorbisfile.h>
#include <mpg123.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

#define AUDIO_FORMAT AUDIO_S16SYS  

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

AudioData audio_data;

float volume = 0.5f;

const GLchar* vertexShaderSource = R"(
        #version 460
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 texCoord;
        out vec2 TexCoord;
        void main() {
	  gl_Position = vec4(position, 1.0);
	  TexCoord = texCoord;
	}
)";

const GLchar* fragmentShaderSource = R"(
	#version 460
        in vec2 TexCoord;
        out vec4 color;
        uniform sampler2D texture1;
        void main() {
            color = texture(texture1, TexCoord);
        }
)";			 

void audio_callback(void *userdata, Uint8 *stream, int len) {
  size_t done;

    if (audio_data.is_paused) {
        memset(stream, 0, len);
        return;
    }

    if (audio_data.is_mp3 || audio_data.is_mp2) {
        int err = mpg123_read(audio_data.mpg123_handle, stream, len, &done);
        if (err == MPG123_DONE || done == 0) {
            SDL_PauseAudio(1);  
        }
    } else {
        int current_section;
        long bytes_read;
        int16_t *sampleBuffer = (int16_t *)stream; 
        int sampleCount = len / sizeof(int16_t); 

        while (sampleCount > 0) {
            bytes_read = ov_read(&audio_data.vorbis_file, (char *)sampleBuffer, len, 0, 2, 1, &current_section);
            if (bytes_read == 0) {
                SDL_PauseAudio(1); 
                break;
            }

            sampleCount = bytes_read / sizeof(int16_t);
            
            for (int i = 0; i < sampleCount; i++) {
                sampleBuffer[i] = (int16_t)(sampleBuffer[i] * volume);
            }
	    
            sampleBuffer += sampleCount; 
            len -= bytes_read; 
        }
    }
    for (int i = 0; i < len / sizeof(int16_t); i++) {
        int16_t *audioSample = (int16_t *)(stream + i * sizeof(int16_t));
        *audioSample = (int16_t)(*audioSample * volume);
    }
}

GLuint loadTexture(const char* filename) {
    SDL_Surface* surface = SDL_LoadBMP(filename);
    if (!surface) {
        printf("Failed to load BMP image: %s\n", SDL_GetError());
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_FreeSurface(surface);
    return textureID;
}

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("Error: Vertex Shader Compilation failed\n%s\n", infoLog);
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("Error: Fragment Shader Compilation failed\n%s\n", infoLog);
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Error: Shader Program Linking failed\n%s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void renderTexture(GLuint shaderProgram, GLuint VAO, GLuint texture) {
    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

int audio_supported = 1;

void handleAudioFile(const char *filename, SDL_AudioSpec *wav_spec) {
    const char *ext = strrchr(filename, '.');

    if (ext && (strcmp(ext, ".mp3") == 0 || strcmp(ext, ".mp2") == 0)) {
        if (mpg123_init() != MPG123_OK || (audio_data.mpg123_handle = mpg123_new(NULL, NULL)) == NULL) {
            fprintf(stderr, "Failed to initialize mpg123.\n");
            return;
        }
        if (mpg123_open(audio_data.mpg123_handle, filename) != MPG123_OK) {
            fprintf(stderr, "Could not open MP3/MP2 file: %s\n", filename);
            mpg123_delete(audio_data.mpg123_handle);
            return;
        }

        long rate;
        int channels, encoding;
        if (mpg123_getformat(audio_data.mpg123_handle, &rate, &channels, &encoding) != MPG123_OK) {
            fprintf(stderr, "Could not get MP3/MP2 format.\n");
            mpg123_delete(audio_data.mpg123_handle);
            return;
        }

        wav_spec->freq = rate;
        wav_spec->channels = channels;
        audio_data.is_mp3 = (strcmp(ext, ".mp3") == 0) ? 1 : 0;
        audio_data.is_mp2 = (strcmp(ext, ".mp2") == 0) ? 1 : 0;

    } else if (ext && strcmp(ext, ".ogg") == 0) {
        if (ov_fopen(filename, &audio_data.vorbis_file) < 0) {
            fprintf(stderr, "Could not open Ogg Vorbis file: %s\n", filename);
            return;
        }

        vorbis_info *info = ov_info(&audio_data.vorbis_file, -1);
        audio_data.total_duration = ov_time_total(&audio_data.vorbis_file, -1);

        wav_spec->freq = info->rate;
        wav_spec->channels = info->channels;
        audio_data.is_mp3 = 0;
        audio_data.is_mp2 = 0;

    } else {
        fprintf(stderr, "Unsupported file format: %s\n", filename);
	audio_supported = 0;
        return;
    }
}

int audio_thread(void *ptr) {
    SDL_PauseAudio(0);  
    while (!audio_data.is_paused) {
        SDL_Delay(100);  
    }
    return 0;
}

void setVolume(float newVolume) {
    if (newVolume < 0.0f) {
        volume = 0.0f;  
    } else if (newVolume > 1.0f) {
        volume = 1.0f;  
    } else {
        volume = newVolume;  
    }
}

void print_audio_progress(AudioData *audio_data) {
    double current_time = 0.0;

    if (audio_data->is_mp3 || audio_data->is_mp2) {
        off_t current_sample = mpg123_tell(audio_data->mpg123_handle);
        off_t total_samples = mpg123_length(audio_data->mpg123_handle);

        long sample_rate;
        int channels, encoding;
        mpg123_getformat(audio_data->mpg123_handle, &sample_rate, &channels, &encoding);

        current_time = (double)current_sample / sample_rate;
        audio_data->total_duration = (double)total_samples / sample_rate;  
    } else {
        current_time = ov_time_tell(&audio_data->vorbis_file);
    }

    int cur_minutes = (int)(current_time / 60);
    int cur_seconds = (int)(current_time) % 60;
    int total_minutes = (int)(audio_data->total_duration / 60);
    int total_seconds = (int)(audio_data->total_duration) % 60;

    double percentage = (current_time / audio_data->total_duration) * 100;

    if (audio_data->is_paused) {
        printf("\rA: (Paused) ");
    } else {
        printf("\rA: %d:%02d/%d:%02d (%.0f%%)", cur_minutes, cur_seconds, total_minutes, total_seconds, percentage);
    }
    fflush(stdout);
}

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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <audio_file.ogg|mp3|mp2>\n", argv[0]);
	audio_supported = 0;
        return 1;
    }

    const char *filename = argv[1];
    memset(&audio_data, 0, sizeof(audio_data));
    audio_data.is_paused = 0;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL2: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_AudioSpec wav_spec;
    memset(&wav_spec, 0, sizeof(SDL_AudioSpec));
    wav_spec.format = AUDIO_FORMAT;
    wav_spec.samples = 4096;
    wav_spec.callback = audio_callback;
    wav_spec.userdata = &audio_data;

    handleAudioFile(filename, &wav_spec);
    if (!audio_supported) {
      SDL_Quit();
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
        printf("Sample rate: %d Hz\n", info->rate);
        printf("Channels: %d\n", info->channels);

    } else {
        fprintf(stderr, "Unsupported file format: %s\n", filename);
        return -1;
    }

    SDL_PauseAudio(0); 

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("LinuxTune", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                           800, 290, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "SDL2: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW.\n");
        return 1;
    }

    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
       

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
    };

    GLuint indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 

    GLuint texture1 = loadTexture("/usr/local/bin/textures/mainEntry.bmp");
    GLuint texture2 = loadTexture("/usr/local/bin/textures/mainEntryI.bmp");
    GLuint texture3 = loadTexture("/usr/local/bin/textures/mainEntryII.bmp");
    GLuint texture4 = loadTexture("/usr/local/bin/textures/mainEntryIII.bmp");
    GLuint texture5 = loadTexture("/usr/local/bin/textures/mainEntryIV.bmp");
    GLuint texture6 = loadTexture("/usr/local/bin/textures/mainEntry_exit.bmp");

    int encoding;
    long rate;
    int channels;
    GLuint currentTexture = 0;

    if (audio_data.is_mp3 || audio_data.is_mp2) {
      mpg123_getformat(audio_data.mpg123_handle, &rate, &channels, &encoding);
    } else {
      vorbis_info *info = ov_info(&audio_data.vorbis_file, -1);
      channels = info->channels;
    }


    
    if (channels == 1) {
      currentTexture = texture2;
    } else {
      currentTexture = texture1;
    }
    
    SDL_Rect pausePos = { 470, 136, 250, 131 };
    SDL_Rect plusVolume = { 150, 160, 101, 90 };
    SDL_Rect subtractVolume = { 284, 160, 101, 90 };
    SDL_Rect exitPos = { 153, 11, 244, 100 };


    disable_echo();
    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEMOTION) {
	      int mouseX = event.motion.x;
	      int mouseY = event.motion.y;

	      if (mouseX >= pausePos.x && mouseX <= pausePos.x + pausePos.w && 
		  mouseY >= pausePos.y && mouseY <= pausePos.y + pausePos.h) {
		currentTexture = texture3;
	      } else if (mouseX >= plusVolume.x && mouseX <= plusVolume.x + plusVolume.w &&
		  mouseY >= plusVolume.y && mouseY <= plusVolume.y + plusVolume.h) {
		currentTexture = texture4;
	      } else if (mouseX >= subtractVolume.x && mouseX <= subtractVolume.x + subtractVolume.w &&
		  mouseY >= subtractVolume.y && mouseY <= subtractVolume.y + subtractVolume.h) {
		currentTexture = texture5;
	      } else if (mouseX >= exitPos.x && mouseX <= exitPos.x + exitPos.w &&
			 mouseY >= exitPos.y && mouseY <= exitPos.y + exitPos.h) {
		currentTexture = texture6;
	      } else {
		if (channels == 1) {
		  currentTexture = texture2;
		} else {
		  currentTexture = texture1;
		}
	      }
	    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
		  int mouseX = event.button.x;
		  int mouseY = event.button.y;

		  if (mouseX >= pausePos.x && mouseX <= (pausePos.x + pausePos.w) &&
		      mouseY >= pausePos.y && mouseY <= (pausePos.y + pausePos.h)) {
		    audio_data.is_paused = !audio_data.is_paused;
		  }
		  if (mouseX >= plusVolume.x && mouseX <= (plusVolume.x + plusVolume.w) &&
		      mouseY >= plusVolume.y && mouseY <= (plusVolume.y + plusVolume.h)) {
		    setVolume(volume + 0.1f);
		  }
		  if (mouseX >= subtractVolume.x && mouseX <= (subtractVolume.x + subtractVolume.w) &&
		      mouseY >= subtractVolume.y && mouseY <= (subtractVolume.y + subtractVolume.h)) {
		    setVolume(volume - 0.1f);
		  }
		  if (mouseX >= exitPos.x && mouseX <= (exitPos.x + exitPos.w) &&
		      mouseY >= exitPos.y && mouseY <= (exitPos.y + exitPos.h)) {
		    running = 0;
		  }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        renderTexture(shaderProgram, VAO, currentTexture);
	print_audio_progress(&audio_data);
        SDL_GL_SwapWindow(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    if (audio_data.is_mp3 || audio_data.is_mp2) {
        mpg123_delete(audio_data.mpg123_handle);
    } else {
        ov_clear(&audio_data.vorbis_file);
    }

    restore_echo();
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    glDeleteTextures(1, &texture3);
    glDeleteTextures(1, &texture4);
    glDeleteTextures(1, &texture5);
    glDeleteTextures(1, &texture6);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
