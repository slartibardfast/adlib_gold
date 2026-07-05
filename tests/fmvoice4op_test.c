/*
 * fmvoice4op_test.c - validate the pure 4-operator voice model (fmvoice4op.h), plan/0009.
 *   cc -Wall -Werror -I.. -o fmvoice4op_test fmvoice4op_test.c && ./fmvoice4op_test
 *
 * Discharges the #voice-allocator task: the six pairs are disjoint and match the OPL3
 * connection-select mapping, and the free-voice search respects both halves of a pair.
 */
#include <stdio.h>
#include "../fmvoice4op.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

static void test_fmvoice4op(void)
{
    int v, s;
    unsigned char busy[18];

    /* Each secondary is the primary's channel plus three, the OPL3 pairing. */
    for (v = 0; v < FOUR_OP_COUNT; v++)
        CHECK(gFourOpSecondary[v] == gFourOpPrimary[v] + 3,
              "secondary is the primary channel plus three");

    /* Primaries and secondaries are disjoint: no slot is used by two voices. */
    for (v = 0; v < FOUR_OP_COUNT; v++)
    {
        CHECK(FourOpVoiceOfPrimary(gFourOpPrimary[v]) == v, "primary maps back to its voice");
        CHECK(!FourOpIsSecondary(gFourOpPrimary[v]), "a primary is never a secondary");
        CHECK(FourOpIsSecondary(gFourOpSecondary[v]), "a secondary reads as a secondary");
        CHECK(FourOpVoiceOfPrimary(gFourOpSecondary[v]) == -1, "a secondary is not a primary");
    }

    /* Slots outside any pair (6,7,8,15,16,17) are neither primary nor secondary. */
    {
        int standalone[6] = { 6, 7, 8, 15, 16, 17 };
        for (s = 0; s < 6; s++)
        {
            CHECK(FourOpVoiceOfPrimary(standalone[s]) == -1, "standalone slot is not a primary");
            CHECK(!FourOpIsSecondary(standalone[s]), "standalone slot is not a secondary");
        }
    }

    /* All free -> voice 0 available; block voice 0's secondary -> voice 0 unavailable. */
    for (s = 0; s < 18; s++) busy[s] = 0;
    CHECK(FourOpFindFree(busy) == 0, "all free returns the first voice");
    busy[gFourOpSecondary[0]] = 1;
    CHECK(FourOpFindFree(busy) == 1, "a busy secondary skips that voice");

    /* Every pair busy -> no free voice. */
    for (v = 0; v < FOUR_OP_COUNT; v++) busy[gFourOpPrimary[v]] = 1;
    CHECK(FourOpFindFree(busy) == -1, "no free voice when every pair is occupied");
}

int main(void)
{
    test_fmvoice4op();
    if (failures == 0) { printf("fmvoice4op_test: all checks passed\n"); return 0; }
    printf("fmvoice4op_test: %d failure(s)\n", failures);
    return 1;
}
