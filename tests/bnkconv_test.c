/*
 * bnkconv_test.c - validate the generated 4-operator overlay (fmbank4op.h) that
 * tools/bnkconv.c produces from OPL3.BNK (plan/0009). No hardware needed.
 *   cc -Wall -Werror -I.. -o bnkconv_test bnkconv_test.c && ./bnkconv_test
 *
 * Discharges the #convert-bank task: a program marked 4-operator carries four operators and
 * the 4-operator patch marker; an unmatched program carries the 2-operator fallback marker;
 * and a known timbre (program 0) converts to the exact register bytes traced from the bank.
 */
#include <stdio.h>

/* Minimal mirror of fmsynth.h's note structure so the generated header compiles standalone. */
typedef unsigned char BYTE;
#define NUMOPS 4
#define PATCH_1_4OP 0
#define PATCH_1_2OP 2
typedef struct { BYTE bAt20, bAt40, bAt60, bAt80, bAtE0; } operStruct;
typedef struct {
    operStruct op[NUMOPS];
    BYTE bAtA0[2], bAtB0[2], bAtC0[2], bOp, bDummy;
} noteStruct;

#include "../fmbank4op.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

static int op_nonzero(const operStruct *o)
{
    return o->bAt20 || o->bAt40 || o->bAt60 || o->bAt80 || o->bAtE0;
}

/* Discharges plan/0009 #convert-bank: the overlay is well-formed and pins a known record. */
static void test_bank4op(void)
{
    int prog, matched = 0;

    for (prog = 0; prog < 128; prog++)
    {
        const noteStruct *ns = &gGm4OpNote[prog];
        if (gGm4OpHas[prog])
        {
            matched++;
            /* A 4-operator override carries the 4-op marker and four real operators. */
            CHECK(ns->bOp == PATCH_1_4OP, "matched program carries the 4-op marker");
            CHECK(op_nonzero(&ns->op[0]) && op_nonzero(&ns->op[2]),
                  "matched program has operators 1 and 3 populated");
        }
        else
        {
            /* An unmatched program carries the 2-operator fallback marker. */
            CHECK(ns->bOp == PATCH_1_2OP, "unmatched program carries the 2-op fallback marker");
        }
    }

    /* Some programs must map, or the overlay is inert. */
    CHECK(matched > 0, "at least one program maps to a 4-operator timbre");

    /* Program 0 (pianomid1) pins the conversion: its first operator's five register bytes,
     * and the connect byte split into C0 (first channel) and C3 (second). */
    CHECK(gGm4OpHas[0], "program 0 is a 4-operator override");
    CHECK(gGm4OpNote[0].op[0].bAt20 == 0x00 &&
          gGm4OpNote[0].op[0].bAt40 == 0x0e &&
          gGm4OpNote[0].op[0].bAt60 == 0xf8 &&
          gGm4OpNote[0].op[0].bAt80 == 0x06 &&
          gGm4OpNote[0].op[0].bAtE0 == 0x80,
          "program 0 first operator converts to the traced bytes");
    CHECK(gGm4OpNote[0].bAtC0[0] == 0x08 && gGm4OpNote[0].bAtC0[1] == 0x00,
          "program 0 connect byte splits into C0=0x08, C3=0x00");

    printf("bnkconv_test: %d of 128 programs are 4-operator overrides\n", matched);
}

int main(void)
{
    test_bank4op();
    if (failures == 0) { printf("bnkconv_test: all checks passed\n"); return 0; }
    printf("bnkconv_test: %d failure(s)\n", failures);
    return 1;
}
