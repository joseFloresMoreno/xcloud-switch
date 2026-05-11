#ifndef XCLOUD_VIDEO_DECODER_H
#define XCLOUD_VIDEO_DECODER_H

typedef struct {
    int initialized;
    int width;
    int height;
    void* decoder_context;  // ffmpeg context
} XCloudVideoDecoder;

// Video decoder functions
XCloudVideoDecoder* video_decoder_create(void);
void video_decoder_destroy(XCloudVideoDecoder* decoder);
int video_decoder_init(XCloudVideoDecoder* decoder);
int video_decoder_decode(XCloudVideoDecoder* decoder, const unsigned char* data, int size, void* frame_out);
int video_decoder_get_width(XCloudVideoDecoder* decoder);
int video_decoder_get_height(XCloudVideoDecoder* decoder);

#endif // XCLOUD_VIDEO_DECODER_H
