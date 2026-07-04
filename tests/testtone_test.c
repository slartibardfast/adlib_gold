/*
 * testtone_test.c - userspace test for the test-tone generator (tools/testtone.h).
 * Checks the 8-bit PCM tone samples and the WAV header without audio hardware.
 *   cc -I.. -o testtone_test testtone_test.c && ./testtone_test
 */
#include <stdio.h>
#include "../tools/testtone.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

static unsigned long rd32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8)
         | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

static void test_tone_fill(void)
{
    unsigned char buf[512];
    unsigned long i;
    int seen64 = 0, seen192 = 0, other = 0;

    TestToneFill(buf, 512, 22050, 440);
    for (i = 0; i < 512; i++)
    {
        if (buf[i] == 64) seen64 = 1;
        else if (buf[i] == 192) seen192 = 1;
        else other = 1;
    }
    /* A square wave: only the two levels, and it actually oscillates. */
    CHECK(!other, "tone uses only the two 8-bit levels (no clipping/garbage)");
    CHECK(seen64 && seen192, "tone oscillates between both levels");
    /* First sample is the low phase (i=0 -> phase 0 -> even). */
    CHECK(buf[0] == 64, "tone starts on a defined phase");

    /* Silence would be a constant; a tone must not be constant. */
    {
        int constant = 1;
        for (i = 1; i < 512; i++) if (buf[i] != buf[0]) constant = 0;
        CHECK(!constant, "tone is not silence");
    }
}

static void test_wav_header(void)
{
    unsigned char h[TESTTONE_WAV_HEADER];
    unsigned long nsamples = 22050;   /* 1 second at 22050 Hz */

    TestToneWavHeader(h, nsamples, 22050);

    CHECK(h[0]=='R'&&h[1]=='I'&&h[2]=='F'&&h[3]=='F', "RIFF magic");
    CHECK(h[8]=='W'&&h[9]=='A'&&h[10]=='V'&&h[11]=='E', "WAVE magic");
    CHECK(h[12]=='f'&&h[13]=='m'&&h[14]=='t'&&h[15]==' ', "fmt chunk");
    CHECK(h[36]=='d'&&h[37]=='a'&&h[38]=='t'&&h[39]=='a', "data chunk");

    CHECK(rd32(h+16)==16, "fmt chunk length 16");
    CHECK(h[20]==1 && h[21]==0, "PCM format tag");
    CHECK(h[22]==1 && h[23]==0, "mono");
    CHECK(rd32(h+24)==22050, "sample rate 22050");
    CHECK(rd32(h+28)==22050, "byte rate = rate for 8-bit mono");
    CHECK(h[32]==1 && h[33]==0, "block align 1");
    CHECK(h[34]==8 && h[35]==0, "8 bits per sample");
    CHECK(rd32(h+40)==nsamples, "data length = sample count");
    CHECK(rd32(h+4)==36+nsamples, "RIFF length = 36 + data");
}

int main(void)
{
    test_tone_fill();
    test_wav_header();
    if (failures == 0) { printf("testtone_test: all checks passed\n"); return 0; }
    printf("testtone_test: %d failure(s)\n", failures);
    return 1;
}
