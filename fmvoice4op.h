/*****************************************************************************
 * fmvoice4op.h - the pure 4-operator voice model for the OPL3 (plan/0009).
 *
 * The YMF262 pairs two 2-operator channels into one 4-operator voice. There are six such
 * voices: three in register Array 0 and three in Array 1, each pairing a primary channel
 * with the channel three above it (the connection-select bits of reg 0x104, in order). The
 * primary slot carries operators 1-2, the play/rate/GO registers and the first paired
 * C-register; the secondary slot carries operators 3-4 and the second C-register. The six
 * remaining channels (slots 6,7,8,15,16,17) stay 2-operator voices.
 *
 * Kept free of DDK headers so a userspace test pins the pairing and the allocation policy,
 * per the no-hollow-green discipline. The driver combines a voice's two slots with the
 * existing gw2OpOffset table to reach all four operator register offsets.
 *****************************************************************************/
#ifndef _FMVOICE4OP_H_
#define _FMVOICE4OP_H_

/* Six 4-operator voices; the connection-select value that promotes all six (reg 0x104). */
#define FOUR_OP_COUNT 6
#define FOUR_OP_MASK  0x3F

/* The primary and secondary 2-operator slots of each 4-operator voice, indexed by voice.
 * Slots 0-8 are Array 0 channels 0-8; slots 9-17 are Array 1 channels 0-8. */
static const int gFourOpPrimary[FOUR_OP_COUNT]   = { 0, 1, 2,  9, 10, 11 };
static const int gFourOpSecondary[FOUR_OP_COUNT] = { 3, 4, 5, 12, 13, 14 };

/* If `slot` is the primary of a 4-operator voice, that voice's index; otherwise -1. */
static int
FourOpVoiceOfPrimary(int slot)
{
    int v;
    for (v = 0; v < FOUR_OP_COUNT; v++)
        if (gFourOpPrimary[v] == slot)
            return v;
    return -1;
}

/* Whether `slot` is the secondary half of some 4-operator voice. A 2-operator allocation
 * must skip such a slot while its voice is active, so the two halves never collide. */
static int
FourOpIsSecondary(int slot)
{
    int v;
    for (v = 0; v < FOUR_OP_COUNT; v++)
        if (gFourOpSecondary[v] == slot)
            return 1;
    return 0;
}

/* If `slot` is the secondary of a 4-operator voice, that voice's index; otherwise -1. The
 * voice's primary is then gFourOpPrimary[result], letting a caller skip a reserved secondary
 * by testing whether that primary is an active 4-op voice. */
static int
FourOpVoiceOfSecondary(int slot)
{
    int v;
    for (v = 0; v < FOUR_OP_COUNT; v++)
        if (gFourOpSecondary[v] == slot)
            return v;
    return -1;
}

/* Whether `slot` must not be taken by the 2-op allocator: it is half of a pair still
 * sounding a 4-op note (the primary carries b4op and on). A released pair keeps its
 * connection bit through the tail, since the bit may change only while both halves are
 * silent (plan/0016), so release state alone no longer protects: the reuse paths
 * silence both halves and split instead. b4op[] and on[] are per-slot views of the
 * voice table. */
static int
FourOpSlotProtected(const unsigned char *b4op, const unsigned char *on, int slot)
{
    int v = FourOpVoiceOfPrimary(slot);
    if (v < 0)
        v = FourOpVoiceOfSecondary(slot);
    if (v < 0)
        return 0;
    return b4op[gFourOpPrimary[v]] && on[gFourOpPrimary[v]];
}

/* Find a free 4-operator voice given the busy state of the 18 slots (busy[slot] != 0 when
 * occupied); return its index, or -1 when no pair is free. A voice is free only when both
 * its primary and secondary slots are free. */
static int
FourOpFindFree(const unsigned char *busy)
{
    int v;
    for (v = 0; v < FOUR_OP_COUNT; v++)
        if (!busy[gFourOpPrimary[v]] && !busy[gFourOpSecondary[v]])
            return v;
    return -1;
}

#endif /* _FMVOICE4OP_H_ */
