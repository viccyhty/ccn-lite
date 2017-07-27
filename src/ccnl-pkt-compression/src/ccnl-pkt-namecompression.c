/**
 * @f ccnl-pkt-namecompression.h
 * @b CCN lite (CCNL), functions for compressing and decompressing ICN names
 * note requires USE_SUITE_COMPRESSED to be defined 
 *
 * author Christopher Scherb <christopher.scherb@unibas.ch>
 * author Cenk Gündoğan <cenk.guendogan@haw-hamburg.de>
 * author Balazs Faludi <balazs.faludi@unibas.ch>
 *
 * copyright (C) 2011-17, University of Basel
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "ccnl-pkt-namecompression.h"

#include <string.h>

#ifdef USE_SUITE_COMPRESSED
static inline int 
ccnl_pkt_compression_min(int a, int b){
    return a < b ? a : b;
}

unsigned char 
ccnl_pkt_compression_map_char2byte(unsigned char c){
    int shift = 0;
    if(c == '.') {
        return 0x3E << 2;
    }
    if(c == '/') {
        return 0x3F << 2;
    }
    if(c >= 0x30 && c <= 0x39){
        shift = 0x30;
    }
    else if(c >= 0x41 && c <= 0x5A){
        shift = 0x37;
    }
    else if(c >= 0x61 && c <= 0x7A){
        shift = 0x3D;
    }
    return (unsigned char) (c - shift) << 2;
}

unsigned char 
ccnl_pkt_compression_map_byte2char(unsigned char c){
    int shift = 0;
    if(c >= 252){
         return '/';
    }
    if(c >= 248 && c <= 252){
         return '.';
    }
    if(c <= 39){
        shift = 0x30;
    }
    else if(c >= 40 && c <= 143){
        shift = 0x37;
    }
    else if(c >= 144 && c <= 247){
        shift = 0x3D;
    }
    return (unsigned char) ((c >> 2) + shift);
}

unsigned char
ccnl_pkt_compression_char2bitpos(unsigned char c, int pos){
    if (pos > 0){
        return  c >> pos;
    }else
    {
        return c << -(pos);
    }
}

int
ccnl_pkt_compression_str2bytes(unsigned char *str, int charlen, 
                              unsigned char *out, int outlen){
    int len = strlen((const char *)str);
    memset(out, 0, outlen);

    int out_pos = 0;
    int str_pos = 0;
    int written_bits = 0;
    int offset = 0;
    int remaining_bits = 0;
    while(str_pos < len && out_pos < outlen){
        unsigned char c = str[str_pos];
        unsigned char c_mapped = ccnl_pkt_compression_map_char2byte(c);
        if(remaining_bits > 0 ){
            offset = -(charlen-remaining_bits);
        }
        else{
            offset = written_bits % 8;
        }
        unsigned char c_shift = ccnl_pkt_compression_char2bitpos(c_mapped, offset);
        out[out_pos] |= c_shift;

        if(offset > 8-charlen){
            ++out_pos;
        }
        else{
            ++str_pos;
            if(offset + charlen == 8){
                ++out_pos;
            }
        }
        written_bits += offset >= 0 ? ccnl_pkt_compression_min(8-offset, charlen) : remaining_bits;
        remaining_bits = charlen - ccnl_pkt_compression_min(8-offset, charlen);
    }
    return written_bits%8 == 0 ? written_bits/8 : written_bits/8+1;
}

int
ccnl_pkt_compression_bytes2str(unsigned char *in, int inlen, int charlen, 
                              unsigned char *out, int outlen)
{
    int offset = 0;
    int index = 0;
    memset(out, 0, outlen);
    for (int i = 0; i < inlen; i++) {
        int bitpos = i * charlen;
        index = bitpos / 8;
        offset = bitpos - index * 8;

        short shifted = 0;
        shifted = in[index + 1];
        *((char*)(&shifted) + 1) = in[index];

        shifted = shifted << offset;
        unsigned char outchar = *((unsigned char*)(&shifted) + 1);
        outchar = ccnl_pkt_compression_map_byte2char(outchar); 

        out[i] = outchar;
    }
    return inlen;
}


#endif