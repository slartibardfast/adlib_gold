/*****************************************************************************
 * testtone.h - pure generation of an 8-bit PCM test tone and its WAV header.
 *
 * The test tone is an APPLICATION artifact, not something the driver produces:
 * this utility writes a standard 8-bit PCM WAV that the operator plays through
 * the installed driver (Media Player, sndPlaySound, ...) to exercise the
 * WaveCyclic path on hardware (call/0005 acceptance). The logic here is pure and
 * integer-only, so it is unit-tested without audio hardware.
 *****************************************************************************/
#ifndef _TESTTONE_H_
#define _TESTTONE_H_

#define TESTTONE_WAV_HEADER 44   /* canonical PCM WAV header size, in bytes */

/*
 * Fill buf with `nsamples` mono 8-bit UNSIGNED PCM samples of a square-wave tone
 * at `freq` Hz for a `rate` Hz sample rate. Silence is 128; the wave swings to
 * 64 and 192 (about -6 dBFS), loud enough to hear on a test but not clipping.
 * Square so it is exactly integer, with no FPU.
 */
static void
TestToneFill(unsigned char *buf, unsigned long nsamples,
             unsigned long rate, unsigned long freq)
{
    unsigned long i;
    for (i = 0; i < nsamples; i++)
        buf[i] = (unsigned char)((((2UL * freq * i) / rate) & 1UL) ? 192 : 64);
}

/*
 * Write the 44-byte canonical WAV header for `nsamples` mono 8-bit PCM samples
 * at `rate` Hz into h. Little-endian, PCM format tag 1, as Windows expects.
 */
static void
TestToneWavHeader(unsigned char *h, unsigned long nsamples, unsigned long rate)
{
    unsigned long dataLen = nsamples;           /* 1 byte/sample, mono */
    unsigned long riffLen = 36 + dataLen;

    h[0]='R'; h[1]='I'; h[2]='F'; h[3]='F';
    h[4]=(unsigned char)riffLen;       h[5]=(unsigned char)(riffLen>>8);
    h[6]=(unsigned char)(riffLen>>16); h[7]=(unsigned char)(riffLen>>24);
    h[8]='W'; h[9]='A'; h[10]='V'; h[11]='E';

    h[12]='f'; h[13]='m'; h[14]='t'; h[15]=' ';
    h[16]=16; h[17]=0; h[18]=0; h[19]=0;         /* fmt chunk length 16 */
    h[20]=1;  h[21]=0;                           /* PCM                 */
    h[22]=1;  h[23]=0;                           /* mono                */
    h[24]=(unsigned char)rate;         h[25]=(unsigned char)(rate>>8);
    h[26]=(unsigned char)(rate>>16);   h[27]=(unsigned char)(rate>>24);
    h[28]=(unsigned char)rate;         h[29]=(unsigned char)(rate>>8);  /* byte rate = rate*1*1 */
    h[30]=(unsigned char)(rate>>16);   h[31]=(unsigned char)(rate>>24);
    h[32]=1;  h[33]=0;                           /* block align 1 byte  */
    h[34]=8;  h[35]=0;                           /* 8 bits per sample   */

    h[36]='d'; h[37]='a'; h[38]='t'; h[39]='a';
    h[40]=(unsigned char)dataLen;       h[41]=(unsigned char)(dataLen>>8);
    h[42]=(unsigned char)(dataLen>>16); h[43]=(unsigned char)(dataLen>>24);
}

#endif /* _TESTTONE_H_ */
