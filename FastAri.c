/*  FastAri 0.3 - A Fast Block-Based Bitwise Arithmetic Compressor
Copyright (C) 2013  David Catt

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdlib.h>
#define ORDER 17
#define ADAPT 4
const int mask = (1 << ORDER) - 1;
int fa_compress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t* olen) {
	unsigned int low=0, mid, high=0xFFFFFFFF;
	size_t ipos, opos=0;
	int bp, bv;
	int ctx;
	/* Initialize model */
	unsigned short* mdl = malloc((mask + 1) * sizeof(unsigned short));
	if(!mdl) return 1;
	for(ctx = 0; ctx <= mask; ++ctx) mdl[ctx] = 0x8000;
	ctx = 0;
	/* Walk through input */
	for(ipos = 0; ipos < ilen; ++ipos) {
		bv = ibuf[ipos];
		for(bp = 7; bp >= 0; --bp) {
			mid = low + (((high - low) >> 16) * mdl[ctx]);
			if(bv&1) {
				high = mid;
				mdl[ctx] += (0xFFFF - mdl[ctx]) >> ADAPT;
				ctx = ((ctx << 1) | 1) & mask;
			} else {
				low = mid + 1;
				mdl[ctx] -= mdl[ctx] >> ADAPT;
				ctx = (ctx << 1) & mask;
			}
			bv >>= 1;
			while((high ^ low) < 0x1000000) {
				obuf[opos] = high >> 24;
				++opos;
				high = (high << 8) | 0xFF;
				low <<= 8;
			}
		}
	}
	/* Flush coder */
	obuf[opos] = (high >> 24); ++opos;
	obuf[opos] = (high >> 16) & 0xFF; ++opos;
	obuf[opos] = (high >> 8) & 0xFF; ++opos;
	obuf[opos] = high & 0xFF; ++opos;
	/* Cleanup */
	free(mdl);
	*olen = opos;
	return 0;
}
int fa_decompress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t olen) {
	unsigned int low = 0, mid, high = 0xFFFFFFFF, cur = 0, c;
	size_t opos = 0;
	int bp, bv;
	int ctx;
	/* Initialize model */
	unsigned short* mdl = malloc((mask + 1) * sizeof(unsigned short));
	if(!mdl) return 1;
	for(ctx = 0; ctx <= mask; ++ctx) mdl[ctx] = 0x8000;
	ctx = 0;
	/* Initialize coder */
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	cur = (cur << 8) | *ibuf; ++ibuf;
	/* Walk through input */
	while(olen) {
		bv = 0;
		for(bp = 0; bp < 8; ++bp) {
			/* Decode bit */
			mid = low + (((high - low) >> 16) * mdl[ctx]);
			if(cur <= mid) {
				high = mid;
				mdl[ctx] += (0xFFFF - mdl[ctx]) >> ADAPT;
				bv |= (1 << bp);
				ctx = ((ctx << 1) | 1) & mask;
			} else {
				low = mid + 1;
				mdl[ctx] -= mdl[ctx] >> ADAPT;
				ctx = (ctx << 1) & mask;
			}
			while((high ^ low) < 0x1000000) {
				high = (high << 8) | 0xFF;
				low <<= 8;
				c = *ibuf; ++ibuf;
				cur = (cur << 8) | c;
			}
		}
		*obuf = bv;
		++obuf;
		--olen;
	}
	/* Cleanup */
	free(mdl);
	return 0;
}