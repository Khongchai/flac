#include <stdlib.h>
#include <string.h>
#include "FLAC/stream_encoder.h"

typedef struct {
	FLAC__StreamEncoder *encoder;
	FLAC__byte *header;
	size_t header_length;
	size_t header_capacity;
	FLAC__byte *output;
	size_t output_length;
	size_t output_capacity;
	FLAC__uint64 position;
	int header_done;
} VariationFlacEncoder;

static int reserve(FLAC__byte **buffer, size_t *capacity, size_t needed)
{
	if (needed <= *capacity)
		return 1;
	size_t grown = *capacity ? *capacity : 4096;
	while (grown < needed)
		grown *= 2;
	FLAC__byte *next = realloc(*buffer, grown);
	if (!next)
		return 0;
	*buffer = next;
	*capacity = grown;
	return 1;
}

static FLAC__StreamEncoderWriteStatus write_bytes(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, uint32_t samples, uint32_t current_frame, void *client_data)
{
	VariationFlacEncoder *e = client_data;
	if (!e->header_done || e->position < e->header_length) {
		size_t end = (size_t)e->position + bytes;
		if (!reserve(&e->header, &e->header_capacity, end))
			return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
		memcpy(e->header + e->position, buffer, bytes);
		if (end > e->header_length)
			e->header_length = end;
	} else {
		if (!reserve(&e->output, &e->output_capacity, e->output_length + bytes))
			return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
		memcpy(e->output + e->output_length, buffer, bytes);
		e->output_length += bytes;
	}
	e->position += bytes;
	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static FLAC__StreamEncoderSeekStatus seek_to(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	VariationFlacEncoder *e = client_data;
	e->position = absolute_byte_offset;
	return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

static FLAC__StreamEncoderTellStatus tell_position(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	VariationFlacEncoder *e = client_data;
	*absolute_byte_offset = e->position;
	return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}

void variation_flac_encoder_destroy(VariationFlacEncoder *e)
{
	if (!e)
		return;
	if (e->encoder)
		FLAC__stream_encoder_delete(e->encoder);
	free(e->header);
	free(e->output);
	free(e);
}

VariationFlacEncoder *variation_flac_encoder_create(uint32_t sample_rate, uint32_t channels, uint32_t bits_per_sample, uint32_t compression_level)
{
	VariationFlacEncoder *e = calloc(1, sizeof(VariationFlacEncoder));
	if (!e)
		return NULL;
	e->encoder = FLAC__stream_encoder_new();
	if (!e->encoder ||
		!FLAC__stream_encoder_set_channels(e->encoder, channels) ||
		!FLAC__stream_encoder_set_bits_per_sample(e->encoder, bits_per_sample) ||
		!FLAC__stream_encoder_set_sample_rate(e->encoder, sample_rate) ||
		!FLAC__stream_encoder_set_compression_level(e->encoder, compression_level) ||
		FLAC__stream_encoder_init_stream(e->encoder, write_bytes, seek_to, tell_position, NULL, e) != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		variation_flac_encoder_destroy(e);
		return NULL;
	}
	e->header_done = 1;
	return e;
}

int variation_flac_encoder_encode(VariationFlacEncoder *e, const FLAC__int32 *interleaved, uint32_t frames)
{
	return FLAC__stream_encoder_process_interleaved(e->encoder, interleaved, frames) ? 0 : -1;
}

int variation_flac_encoder_finish(VariationFlacEncoder *e)
{
	return FLAC__stream_encoder_finish(e->encoder) ? 0 : -1;
}

FLAC__byte *variation_flac_encoder_header(VariationFlacEncoder *e) { return e->header; }
size_t variation_flac_encoder_header_length(VariationFlacEncoder *e) { return e->header_length; }
FLAC__byte *variation_flac_encoder_output(VariationFlacEncoder *e) { return e->output; }
size_t variation_flac_encoder_output_length(VariationFlacEncoder *e) { return e->output_length; }
void variation_flac_encoder_clear_output(VariationFlacEncoder *e) { e->output_length = 0; }
