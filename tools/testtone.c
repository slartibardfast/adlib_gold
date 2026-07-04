/*
 * testtone.c - write an 8-bit PCM WAV test tone (call/0005 acceptance helper).
 *
 * The driver plays PCM that applications stream to it; the test tone is one such
 * application artifact. This standalone utility writes a WAV the operator plays
 * through the installed driver to confirm the 8-bit tone is audible on the card.
 *
 *   cl testtone.c            (Win2K DDK / VC6, native Windows)
 *   cc -I. -o testtone testtone.c   (host)
 *   testtone [out.wav] [rate] [freq] [seconds]
 * Defaults: testtone.wav, 22050 Hz (a hardware rate), 440 Hz tone, 1 second.
 */
#include <stdio.h>
#include <stdlib.h>
#include "testtone.h"

int
main(int argc, char **argv)
{
    const char   *out  = (argc > 1) ? argv[1] : "testtone.wav";
    unsigned long rate  = (argc > 2) ? strtoul(argv[2], 0, 10) : 22050;
    unsigned long freq  = (argc > 3) ? strtoul(argv[3], 0, 10) : 440;
    unsigned long secs  = (argc > 4) ? strtoul(argv[4], 0, 10) : 1;
    unsigned long n;
    unsigned char hdr[TESTTONE_WAV_HEADER];
    unsigned char *pcm;
    FILE *f;

    if (rate == 0 || secs == 0)
    {
        fprintf(stderr, "rate and seconds must be non-zero\n");
        return 1;
    }
    n = rate * secs;
    pcm = (unsigned char *)malloc(n);
    if (!pcm)
    {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    TestToneFill(pcm, n, rate, freq);
    TestToneWavHeader(hdr, n, rate);

    f = fopen(out, "wb");
    if (!f)
    {
        fprintf(stderr, "cannot open %s\n", out);
        free(pcm);
        return 1;
    }
    fwrite(hdr, 1, TESTTONE_WAV_HEADER, f);
    fwrite(pcm, 1, n, f);
    fclose(f);
    free(pcm);

    printf("wrote %s: %lu samples, %lu Hz, %lu Hz tone, 8-bit mono PCM\n",
           out, n, rate, freq);
    return 0;
}
