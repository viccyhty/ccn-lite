/*
 * @f util/ccn-lite-mkC.c
 * @b CLI mkContent, write to Stdout
 *
 * Copyright (C) 2013-15, Christian Tschudin, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2013-07-06  created
 */


//#define NEEDS_PACKET_CRAFTING

#include "ccnl-common.h"
#include "ccnl-crypto.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-ext-hmac.h"


// ----------------------------------------------------------------------

char *private_key_path;
char *witness;

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    unsigned char body[64*1024];
    unsigned char out[65*1024];
    unsigned char *publisher = out;
    char *infname = 0, *outfname = 0;
    unsigned int chunknum = UINT_MAX, lastchunknum = UINT_MAX;
    int f, len, opt, plen, offs = 0;
    int contentpos;
    struct ccnl_prefix_s *name;
    int suite = CCNL_SUITE_DEFAULT;
    struct key_s *keys = NULL;
    struct ccnl_prefix_s *prefix;
    struct ccnl_buf_s *buf;
    ccnl_data_opts_u data_opts;
    (void)prefix;
    (void)buf;
    (void) contentpos;
    (void)keys;
    (void)lastchunknum;
    (void)chunknum;
    (void) data_opts;

    while ((opt = getopt(argc, argv, "hg:i:k:l:n:o:p:s:v:w:")) != -1) {
        switch (opt) {
        case 'i':
            infname = optarg;
            break;
        case 'k':
            keys = load_keys_from_file(optarg);
            break;
        case 'l':
            lastchunknum = (int)strtol(optarg, (char**)NULL, 10);
            break;
        case 'n':
            chunknum = (int)strtol(optarg, (char**)NULL, 10);
            break;
        case 'o':
            outfname = optarg;
            break;
        case 'p':
            publisher = (unsigned char*) optarg;
            plen = unescape_component((char*) publisher);
            if (plen != 32) {
                DEBUGMSG(ERROR,
                  "publisher key digest has wrong length (%d instead of 32)\n",
                  plen);
                exit(-1);
            }
            break;
        case 's':
            suite = ccnl_str2suite(optarg);
            if (!ccnl_isSuite(suite)) {
                DEBUGMSG(ERROR, "Unsupported suite %d\n", suite);
                goto Usage;
            }
            break;
        case 'v':
#ifdef USE_LOGGING
            if (isdigit(optarg[0]))
                debug_level = (int)strtol(optarg, (char**)NULL, 10);
            else
                debug_level = ccnl_debug_str2level(optarg);
#endif
            break;

        case 'w':
            witness = optarg;
            break;
        case 'h':
        default:
Usage:
        fprintf(stderr, "usage: %s [options] URI\n"
        "  -i FNAME    input file (instead of stdin)\n"
        "  -k FNAME    HMAC256 key (base64 encoded)\n"
        "  -l LASTCHUNKNUM number of last chunk\n"
        "  -n CHUNKNUM chunknum\n"
        "  -o FNAME    output file (instead of stdout)\n"
        "  -p DIGEST   publisher fingerprint\n"
        "  -s SUITE    (ccnb, ccnx2015, ndn2013)\n"
#ifdef USE_LOGGING
        "  -v DEBUG_LEVEL (fatal, error, warning, info, debug, verbose, trace)\n"
#endif
        "  -w STRING   witness\n"
        "Examples:\n"
        "%% mkC /ndn/edu/wustl/ping             (classic lookup)\n"
        "%% mkC /rpc/site \"call 1 /test/data\"   (lambda RPC, directed)\n",
        argv[0]);
        exit(1);
        }
    }

    if (!argv[optind])
        goto Usage;

    if (infname) {
        f = open(infname, O_RDONLY);
        if (f < 0)
            perror("file open:");
    } else
        f = 0;
    len = read(f, body, sizeof(body));
    close(f);
    memset(out, 0, sizeof(out));

    name = ccnl_URItoPrefix(argv[optind], suite, 
                            chunknum == UINT_MAX ? NULL : &chunknum);

    switch (suite) {
#ifdef USE_SUITE_CCNB
    case CCNL_SUITE_CCNB:
        len = ccnl_ccnb_fillContent(name, body, len, NULL, out);
        break;
#endif
#ifdef USE_SUITE_CCNTLV
    case CCNL_SUITE_CCNTLV:

        offs = CCNL_MAX_PACKET_SIZE;
        if (keys) {
            unsigned char keyval[64];
            unsigned char keyid[32];
            // use the first key found in the key file
            ccnl_hmac256_keyval(keys->key, keys->keylen, keyval);
            ccnl_hmac256_keyid(keys->key, keys->keylen, keyid);
            len = ccnl_ccntlv_prependSignedContentWithHdr(name, body, len,
                  lastchunknum == UINT_MAX ? NULL : &lastchunknum,
                  NULL, keyval, keyid, &offs, out);
        } else
            len = ccnl_ccntlv_prependContentWithHdr(name, body, len,
                          lastchunknum == UINT_MAX ? NULL : &lastchunknum,
                          NULL /* Int *contentpos */, &offs, out);
        break;
#endif
#ifdef USE_SUITE_NDNTLV
    case CCNL_SUITE_NDNTLV:
        offs = CCNL_MAX_PACKET_SIZE;
        if (keys) {
            unsigned char keyval[64];
            unsigned char keyid[32];
            // use the first key found in the key file
            ccnl_hmac256_keyval(keys->key, keys->keylen, keyval);
            ccnl_hmac256_keyid(keys->key, keys->keylen, keyid);
            len = ccnl_ndntlv_prependSignedContent(name, body, len,
                  lastchunknum == UINT_MAX ? NULL : &lastchunknum,
                  NULL, keyval, keyid, &offs, out);
        } else {
            data_opts.ndntlv.finalblockid = lastchunknum;
            len = ccnl_ndntlv_prependContent(name, body, len,
                  NULL, lastchunknum == UINT_MAX ? NULL : &(data_opts.ndntlv),
                  &offs, out);
        }
        break;
#endif
    default:
        break;
    }

    if (outfname) {
        f = creat(outfname, 0666);
        if (f < 0)
            perror("file open:");
    } else
        f = 1;
    write(f, out + offs, len);
    close(f);

    return 0;
}

// eof
