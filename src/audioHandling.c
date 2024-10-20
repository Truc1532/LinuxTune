
void audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioData *audio_data = (AudioData *)userdata;
    size_t done;

    if (audio_data->is_paused) {
        memset(stream, 0, len);
        return;
    }

    if (audio_data->is_mp3 || audio_data->is_mp2) {
        int err = mpg123_read(audio_data->mpg123_handle, stream, len, &done);
        if (err == MPG123_DONE || done == 0) {
            SDL_PauseAudio(1);
        }
    } else {
        int current_section;
        long bytes_read;

        while (len > 0) {
            bytes_read = ov_read(&audio_data->vorbis_file, (char *)stream, len, 0, 2, 1, &current_section);
            if (bytes_read == 0) {
                SDL_PauseAudio(1);
                break;
            }
            len -= bytes_read;
            stream += bytes_read;
        }
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
