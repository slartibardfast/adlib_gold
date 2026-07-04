/*
 * sp2modes_test.c - userspace test for the SP2 surround presets (sp2modes.h),
 * the data the topology's sp2_mode node downloads (call/0012). Checks the mode
 * count against the spec, the documented off-state, and that every preset value
 * stays inside the datasheet encodings. No hardware needed.
 *   cc -I.. -o sp2modes_test sp2modes_test.c && ./sp2modes_test
 */
#include <stdio.h>
#include "../sp2modes.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

/* The count the sp2_mode node offers must equal topology.allium's sp2_mode_count. */
static void test_sp2_mode_count(void)
{
    CHECK(Sp2ModeCount() == 4, "four documented modes (matches sp2_mode_count)");
    CHECK(Sp2ModeCount() == SP2_MODE_COUNT, "count equals the table dimension");
}

static void test_sp2_presets(void)
{
    unsigned char a[SP2_NUM_REGS], b[SP2_NUM_REGS];
    int mode, i;

    /* Mode 0 is the datasheet initial-clear state: VM=VC=VL=VR = 0 (off). */
    Sp2ModePreset(0, a);
    CHECK(a[SP2_REG_VM] == 0 && a[SP2_REG_VC] == 0 &&
          a[SP2_REG_VL] == 0 && a[SP2_REG_VR] == 0, "mode 0 clears the attenuators (off)");

    for (mode = 0; mode < SP2_MODE_COUNT; mode++)
    {
        Sp2ModePreset(mode, a);
        for (i = 0; i < SP2_NUM_REGS; i++)
        {
            if (i >= SP2_REG_T0)                       /* delay taps T0..T8: 5-bit */
                CHECK(a[i] <= SP2_DELAY_MAX, "delay tap within the 5-bit range");
            else                                       /* attenuations/coeffs: 6-bit */
                CHECK(a[i] <= SP2_ATTEN_MAX, "attenuation within the 6-bit range");
        }
        /* Every active mode opens the output attenuators; off keeps them shut. */
        if (mode == 0)
            CHECK(a[SP2_REG_VL] == 0, "off mode has no output");
        else
            CHECK(a[SP2_REG_VL] == SP2_ATTEN_0DB && a[SP2_REG_VR] == SP2_ATTEN_0DB,
                  "active mode drives both outputs at 0 dB");
    }

    /* Modes are distinct from off, so the selector actually changes the effect. */
    Sp2ModePreset(0, a);
    for (mode = 1; mode < SP2_MODE_COUNT; mode++)
    {
        int differs = 0;
        Sp2ModePreset(mode, b);
        for (i = 0; i < SP2_NUM_REGS; i++)
            if (a[i] != b[i]) differs = 1;
        CHECK(differs, "active mode differs from off");
    }

    /* An out-of-range selection falls back to the off preset (never a bad write). */
    Sp2ModePreset(99, a);
    Sp2ModePreset(0, b);
    {
        int same = 1;
        for (i = 0; i < SP2_NUM_REGS; i++)
            if (a[i] != b[i]) same = 0;
        CHECK(same, "out-of-range mode falls back to off");
    }
}

int main(void)
{
    test_sp2_mode_count();
    test_sp2_presets();
    if (failures == 0) { printf("sp2modes_test: all checks passed\n"); return 0; }
    printf("sp2modes_test: %d failure(s)\n", failures);
    return 1;
}
