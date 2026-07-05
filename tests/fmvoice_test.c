/*
 * fmvoice_test.c - userspace test for the OPL3 FM-synth voice allocator
 * (fmvoice.h), the driver's own voice policy (call/0014, fmsynth.allium). Drives
 * a note-on/note-off simulation and checks the spec's rules without hardware.
 *   cc -I.. -o fmvoice_test fmvoice_test.c && ./fmvoice_test
 */
#include <stdio.h>
#include "../fmvoice.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

/* A software model of the 18 voices the allocator decides over. */
static unsigned long vtime[FM_VOICE_COUNT];
static unsigned char von[FM_VOICE_COUNT], vpatch[FM_VOICE_COUNT];
static unsigned char vchan[FM_VOICE_COUNT], vnote[FM_VOICE_COUNT];
static unsigned char vprotect[FM_VOICE_COUNT];   /* 4-op-reserved slots (plan/0009) */
static unsigned long clock_;

static void reset_voices(void)
{
    int i;
    for (i = 0; i < FM_VOICE_COUNT; i++)
    { vtime[i] = 0; von[i] = 0; vpatch[i] = 0; vchan[i] = 0; vnote[i] = 0; vprotect[i] = 0; }
    clock_ = 0;
}

/* Note-on: allocate a voice (free or stolen) and mark it sounding. */
static int note_on(unsigned char ch, unsigned char note, unsigned char patch)
{
    int v = FmVoiceAllocate(vtime, von, vpatch, vprotect, FM_VOICE_COUNT, patch);
    von[v] = 1; vtime[v] = ++clock_;
    vpatch[v] = patch; vchan[v] = ch; vnote[v] = note;
    return v;
}

/* Note-off: release the voice sounding that note (time kept for age ordering). */
static int note_off(unsigned char ch, unsigned char note)
{
    int v = FmVoiceFind(vchan, vnote, von, FM_VOICE_COUNT, ch, note);
    if (v != FM_VOICE_NONE) von[v] = 0;
    return v;
}

static int sounding_count(void)
{
    int i, n = 0;
    for (i = 0; i < FM_VOICE_COUNT; i++) if (von[i]) n++;
    return n;
}

static void test_fm_drum_channel(void)
{
    CHECK(FmVoiceIsDrum(9), "channel 9 is percussion (spec drum_channel)");
    CHECK(!FmVoiceIsDrum(0), "channel 0 is melodic");
    CHECK(!FmVoiceIsDrum(15), "channel 15 is melodic");
}

static void test_fm_allocate(void)
{
    int i, v;

    reset_voices();
    /* Free preference: the first allocation takes voice 0 (all time == 0). */
    v = note_on(0, 60, 5);
    CHECK(v == 0, "first note takes a free voice");

    /* Fill all 18 voices with distinct sounding notes. */
    reset_voices();
    for (i = 0; i < FM_VOICE_COUNT; i++)
    {
        v = note_on(0, (unsigned char)(40 + i), 5);
        CHECK(v >= 0 && v < FM_VOICE_COUNT, "allocation is always a valid voice");
    }
    CHECK(sounding_count() == FM_VOICE_COUNT, "all voices sound after 18 notes");

    /* The 19th note must steal: still a valid voice, budget never exceeded. */
    v = note_on(0, 99, 5);
    CHECK(v >= 0 && v < FM_VOICE_COUNT, "the 19th note steals a valid voice");
    CHECK(sounding_count() <= FM_VOICE_COUNT, "VoiceBudget: never more than 18 sound");

    /* A released voice is stolen before a sounding one. Release voice 3, then a
     * new note must reuse exactly that slot (it is the only non-sounding one). */
    reset_voices();
    for (i = 0; i < FM_VOICE_COUNT; i++)
        note_on(0, (unsigned char)(40 + i), 5);
    note_off(0, (unsigned char)(40 + 3));          /* voice 3 released */
    v = note_on(0, 100, 7);
    CHECK(v == 3, "a released voice is stolen before a sounding one");
    CHECK(sounding_count() == FM_VOICE_COUNT, "stealing keeps the budget full");
}

static void test_fm_find(void)
{
    int v;
    reset_voices();
    v = note_on(2, 64, 1);
    CHECK(FmVoiceFind(vchan, vnote, von, FM_VOICE_COUNT, 2, 64) == v,
          "find locates the sounding note");
    CHECK(FmVoiceFind(vchan, vnote, von, FM_VOICE_COUNT, 2, 65) == FM_VOICE_NONE,
          "find returns NONE for a note not sounding");
    note_off(2, 64);
    CHECK(FmVoiceFind(vchan, vnote, von, FM_VOICE_COUNT, 2, 64) == FM_VOICE_NONE,
          "a released note is no longer found");
}

/* A protected slot (a 4-op primary or its reserved secondary) is never handed out, even
 * under full polyphony where the allocator must steal (plan/0009). */
static void test_fm_protect(void)
{
    int i, v;
    reset_voices();
    /* Protect two slots, then fill every slot and keep allocating (forcing steals). */
    vprotect[3] = 1;
    vprotect[7] = 1;
    for (i = 0; i < FM_VOICE_COUNT; i++)
        note_on((unsigned char)(i & 0x0f), (unsigned char)(60 + i), (unsigned char)i);
    for (i = 0; i < 40; i++)
    {
        v = note_on(1, (unsigned char)(30 + (i & 0x1f)), (unsigned char)(i & 0x7f));
        CHECK(v != 3 && v != 7, "a protected slot is never allocated, even when stealing");
    }
    /* With no protected slots the allocator uses every slot. */
    reset_voices();
    for (i = 0; i < FM_VOICE_COUNT; i++)
        vtime[i] = 0;
    v = note_on(0, 60, 0);
    CHECK(v == 0, "first free slot is index 0 when nothing is protected");
}

int main(void)
{
    test_fm_drum_channel();
    test_fm_allocate();
    test_fm_find();
    test_fm_protect();
    if (failures == 0) { printf("fmvoice_test: all checks passed\n"); return 0; }
    printf("fmvoice_test: %d failure(s)\n", failures);
    return 1;
}
