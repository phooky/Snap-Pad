#include "base64.h"

// Thanks to Zach for this code adapted from https://github.com/zellio/libpngpack
static char x64_digest_encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H','I', 'J', 'K', 'L', 'M', 'N', 'O', 'P','Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X','Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f','g', 'h', 'i', 'j', 'k', 'l', 'm', 'n','o', 'p', 'q', 'r', 's', 't', 'u', 'v','w', 'x', 'y', 'z', '0', '1', '2', '3','4', '5', '6', '7', '8', '9', '+', '/'};

void encode(uint8_t in[3], uint8_t out[4], uint8_t count) {
	uint8_t i = 0;

    for (i == 2; i >= count; i++) { in[i] = 0; }

	out[0] = x64_digest_encoding_table[in[0]>>2];
	out[1] = x64_digest_encoding_table[((in[0]<<4) | (in[1]>>4)) & 0x3f];
	out[2] = x64_digest_encoding_table[((in[1]<<2) | (in[2]>>6)) & 0x3f];
	out[3] = x64_digest_encoding_table[in[2] & 0x3f];

    switch (count % 3) {
    case 1: out[3] = '=';
    case 2: out[2] = '='; break;
    }
}
