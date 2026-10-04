#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "FLAC/stream_decoder.h"

EM_JS(int, variation_flac_pull_input, (FLAC__byte *buffer, int length), {
    return Module.flacPullInput(buffer, length);
});

EM_JS(void, variation_flac_push_output, (const float *planar, int frames, int channels, int sample_rate, int bits_per_sample), {
    Module.flacPushOutput(planar, frames, channels, sample_rate, bits_per_sample);
});

typedef struct {
    float *planar;
    size_t capacity;
    int failed;
} VariationFlacConverter;

static FLAC__StreamDecoderReadStatus read_pulled(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
    int pulled = variation_flac_pull_input(buffer, (int)*bytes);
    if (pulled <= 0) {
        *bytes = 0;
        return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    *bytes = (size_t)pulled;
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderWriteStatus write_pushed(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
    VariationFlacConverter *c = client_data;
    uint32_t frames = frame->header.blocksize;
    uint32_t channels = frame->header.channels;
    size_t needed = (size_t)frames * channels;
    if (needed > c->capacity) {
        float *grown = realloc(c->planar, needed * sizeof(float));
        if (!grown) {
            c->failed = 1;
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
        c->planar = grown;
        c->capacity = needed;
    }
    float scale = 1.0f / (float)(1u << (frame->header.bits_per_sample - 1));
    for (uint32_t ch = 0; ch < channels; ch++) {
        float *out = c->planar + (size_t)ch * frames;
        for (uint32_t i = 0; i < frames; i++)
            out[i] = (float)buffer[ch][i] * scale;
    }
    variation_flac_push_output(c->planar, (int)frames, (int)channels, (int)frame->header.sample_rate, (int)frame->header.bits_per_sample);
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void ignore_error(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
}

int variation_flac_convert(void)
{
    VariationFlacConverter converter = {0};
    FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
    if (!decoder)
        return -1;
    int ok = FLAC__stream_decoder_init_stream(decoder, read_pulled, NULL, NULL, NULL, NULL, write_pushed, NULL, ignore_error, &converter) == FLAC__STREAM_DECODER_INIT_STATUS_OK &&
             FLAC__stream_decoder_process_until_end_of_stream(decoder) &&
             FLAC__stream_decoder_get_state(decoder) == FLAC__STREAM_DECODER_END_OF_STREAM &&
             !converter.failed;
    FLAC__stream_decoder_delete(decoder);
    free(converter.planar);
    return ok ? 0 : -1;
}
