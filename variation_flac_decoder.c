#include <stdlib.h>
#include <string.h>
#include "FLAC/stream_decoder.h"

typedef struct {
	FLAC__StreamDecoder *decoder;
	const FLAC__byte *input;
	size_t input_length;
	size_t input_offset;
	FLAC__int32 *output;
	uint32_t output_capacity;
	uint32_t output_frames;
	FLAC__uint64 first_sample;
	int has_first_sample;
	int failed;
	int errors;
	int has_stream_info;
	FLAC__StreamMetadata_StreamInfo stream_info;
} VariationFlacDecoder;

static FLAC__StreamDecoderReadStatus read_input(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	// Note to self: c has no generics. C people just pass void* around so its value can be filled in
	// by libFLAC itself.
	VariationFlacDecoder *d = client_data;
	size_t remaining = d->input_length - d->input_offset;
	if(remaining == 0) {
		*bytes = 0;
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	if(*bytes > remaining)
		*bytes = remaining;
	memcpy(buffer, d->input + d->input_offset, *bytes);
	d->input_offset += *bytes;
	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static FLAC__StreamDecoderWriteStatus write_output(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
	VariationFlacDecoder *d = client_data;
	uint32_t frames = frame->header.blocksize;
	uint32_t channels = frame->header.channels;
	if(d->output_frames + frames > d->output_capacity) {
		d->failed = 1;
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if(!d->has_first_sample) {
		d->first_sample = frame->header.number.sample_number;
		d->has_first_sample = 1;
	}
	FLAC__int32 *out = d->output + (size_t)d->output_frames * channels;
	for(uint32_t i = 0; i < frames; i++)
		for(uint32_t c = 0; c < channels; c++)
			*out++ = buffer[c][i];
	d->output_frames += frames;
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void read_metadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	VariationFlacDecoder *d = client_data;
	if(metadata->type != FLAC__METADATA_TYPE_STREAMINFO)
		return;
	d->stream_info = metadata->data.stream_info;
	d->has_stream_info = 1;
}

static void record_error(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	VariationFlacDecoder *d = client_data;
	d->errors++;
}

void variation_flac_decoder_destroy(VariationFlacDecoder *d)
{
	if(!d)
		return;
	if(d->decoder)
		FLAC__stream_decoder_delete(d->decoder);
	free(d);
}

VariationFlacDecoder *variation_flac_decoder_create(const FLAC__byte *header, size_t length)
{
	VariationFlacDecoder *d = calloc(1, sizeof(VariationFlacDecoder));
	if(!d)
		return NULL;
	d->decoder = FLAC__stream_decoder_new();
	if(!d->decoder ||
	   FLAC__stream_decoder_init_stream(d->decoder, read_input, NULL, NULL, NULL, NULL, write_output, read_metadata, record_error, d) != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		variation_flac_decoder_destroy(d);
		return NULL;
	}
	d->input = header;
	d->input_length = length;
	d->input_offset = 0;
	if(!FLAC__stream_decoder_process_until_end_of_metadata(d->decoder) || !d->has_stream_info) {
		variation_flac_decoder_destroy(d);
		return NULL;
	}
	return d;
}

int variation_flac_decoder_decode(VariationFlacDecoder *d, const FLAC__byte *input, size_t length, FLAC__int32 *output, uint32_t capacity_frames)
{
	if(!FLAC__stream_decoder_flush(d->decoder))
		return -1;
	d->input = input;
	d->input_length = length;
	d->input_offset = 0;
	d->output = output;
	d->output_capacity = capacity_frames;
	d->output_frames = 0;
	d->has_first_sample = 0;
	d->failed = 0;
	d->errors = 0;
	while(FLAC__stream_decoder_get_state(d->decoder) != FLAC__STREAM_DECODER_END_OF_STREAM) {
		if(!FLAC__stream_decoder_process_single(d->decoder) || d->failed)
			return -1;
	}
	return (int)d->output_frames;
}

double variation_flac_decoder_first_sample(VariationFlacDecoder *d) { return (double)d->first_sample; }
int variation_flac_decoder_errors(VariationFlacDecoder *d) { return d->errors; }
uint32_t variation_flac_decoder_sample_rate(VariationFlacDecoder *d) { return d->stream_info.sample_rate; }
uint32_t variation_flac_decoder_channels(VariationFlacDecoder *d) { return d->stream_info.channels; }
uint32_t variation_flac_decoder_bits_per_sample(VariationFlacDecoder *d) { return d->stream_info.bits_per_sample; }
uint32_t variation_flac_decoder_max_block_size(VariationFlacDecoder *d) { return d->stream_info.max_blocksize; }
double variation_flac_decoder_total_samples(VariationFlacDecoder *d) { return (double)d->stream_info.total_samples; }
