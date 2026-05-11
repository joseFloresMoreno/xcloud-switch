#ifndef XCLOUD_AUDIO_DECODER_H
#define XCLOUD_AUDIO_DECODER_H

typedef struct {
    int initialized;
    int sample_rate;
    int channels;
    void* decoder_context;  // libopus context
} XCloudAudioDecoder;

// Audio decoder functions
XCloudAudioDecoder* audio_decoder_create(void);
void audio_decoder_destroy(XCloudAudioDecoder* decoder);
int audio_decoder_init(XCloudAudioDecoder* decoder, int sample_rate, int channels);
int audio_decoder_decode(XCloudAudioDecoder* decoder, const unsigned char* data, int size, float* pcm_out, int pcm_out_size);
int audio_decoder_get_sample_rate(XCloudAudioDecoder* decoder);
int audio_decoder_get_channels(XCloudAudioDecoder* decoder);

#endif // XCLOUD_AUDIO_DECODER_H
