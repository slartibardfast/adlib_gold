/*****************************************************************************
 * fmvoice.h - the driver's own OPL3 FM-synth voice allocation (call/0014), as
 * pure logic so the policy is the single tested source of truth (fmsynth.allium).
 *
 * The OPL3 (YMF262) offers 18 two-operator voices. A MIDI note-on allocates a
 * voice; a note-off releases the voice sounding that note; when every voice is
 * busy a new note steals one. This header holds the *decision* (which voice),
 * with no hardware or kernel dependency, so it is unit-tested without a build;
 * fmsynth.cpp's Opl3_FindEmptySlot / Opl3_FindFullSlot delegate to it and then
 * program the chip. OPL3 inter-write timing lives in chiptiming.h / the .tla spec.
 *****************************************************************************/
#ifndef _FMVOICE_H_
#define _FMVOICE_H_

#define FM_VOICE_COUNT    18     /* OPL3 two-operator voices (spec voice_count)  */
#define FM_DRUM_CHANNEL    9     /* MIDI channel 10 is percussion (spec)         */
#define FM_VOICE_NONE     (-1)   /* "no voice sounding that note"                */

/* A note on the drum channel is percussion (spec DrumChannelIsPercussion). */
static int
FmVoiceIsDrum(int channel)
{
    return channel == FM_DRUM_CHANNEL;
}

/*
 * Choose the voice to sound a new note whose program is `newpatch`. The voices
 * are described by three parallel arrays of length `count`:
 *   time[i]  - allocation timestamp, 0 meaning the voice was never used
 *   on[i]    - nonzero while the voice is currently sounding (key down)
 *   patch[i] - the program the voice last played
 *   protect[i] - nonzero for a voice the 2-op path must never take (a 4-op primary or its
 *                reserved secondary, plan/0009); skipped in every pass
 * Preference, matching the classic OPL3 allocator:
 *   1. a never-used voice (time == 0),
 *   2. else the oldest released (on == 0) voice,
 *   3. else the oldest voice of the same patch,
 *   4. else the globally oldest voice.
 * Always returns a valid index 0..count-1, so no more than `count` voices ever
 * sound at once (spec VoiceBudget); the (count+1)-th note steals.
 */
static int
FmVoiceAllocate(const unsigned long *time, const unsigned char *on,
                const unsigned char *patch, const unsigned char *protect,
                int count, unsigned char newpatch)
{
    int i, found;
    unsigned long oldest;

    for (i = 0; i < count; i++)
        if (!protect[i] && time[i] == 0)
            return i;

    oldest = 0xFFFFFFFFUL; found = -1;
    for (i = 0; i < count; i++)
        if (!protect[i] && !on[i] && time[i] < oldest) { oldest = time[i]; found = i; }
    if (found >= 0)
        return found;

    oldest = 0xFFFFFFFFUL; found = -1;
    for (i = 0; i < count; i++)
        if (!protect[i] && patch[i] == newpatch && time[i] < oldest) { oldest = time[i]; found = i; }
    if (found >= 0)
        return found;

    oldest = 0xFFFFFFFFUL; found = -1;
    for (i = 0; i < count; i++)
        if (!protect[i] && time[i] < oldest) { oldest = time[i]; found = i; }
    return (found >= 0) ? found : 0;
}

/*
 * Find the voice currently sounding (channel, midinote); FM_VOICE_NONE if none.
 * This is the voice a note-off releases (spec NoteOffReleases).
 */
static int
FmVoiceFind(const unsigned char *chan, const unsigned char *note,
            const unsigned char *on, int count,
            unsigned char channel, unsigned char midinote)
{
    int i;
    for (i = 0; i < count; i++)
        if (on[i] && chan[i] == channel && note[i] == midinote)
            return i;
    return FM_VOICE_NONE;
}

#endif /* _FMVOICE_H_ */
