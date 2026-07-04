/*****************************************************************************
 * sp2modes.h - the documented YM7128 (SP2) surround presets the topology's
 * sp2_mode node selects among (call/0012), as pure data so they are unit-tested
 * without hardware.
 *
 * The SP2 is programmed by downloading a 31-register preset (one value per
 * register 0x00..0x1E) over Control-Chip register 0x18 (sp2.h encodes the
 * bit-serial write; the SDK's Write_Surround sends preset[0..30] to registers
 * 0..30 in order, appendix-sp2.md). This header supplies the preset *values*:
 * mode 0 is the datasheet's initial-clear state (VM=VC=VL=VR=0, "surround off"),
 * and the remaining modes are distinct surround presets composed from the
 * datasheet-documented delay and attenuation encodings.
 *
 * Register map (index = SP2 register address, appendix-sp2.md REGISTER MAP):
 *   0x00-0x07 GL1-GL8  Lch tap attenuations   (bit5 sign, bits4-0 level; 0x1F=0dB)
 *   0x08-0x0F GR1-GR8  Rch tap attenuations   (same encoding)
 *   0x10 VM  input attenuation   0x11 VC  FIR/feedback attenuation
 *   0x12 VL  Lch output atten.    0x13 VR  Rch output attenuation
 *   0x14 C0  0x15 C1  FIR low-pass coefficients (lower 6 bits)
 *   0x16-0x1E T0-T8   delay-tap positions      (5-bit; 0x00=0ms .. 0x1F=100ms)
 *
 * SP2_MODE_COUNT matches topology.allium's sp2_mode_count config default.
 *****************************************************************************/
#ifndef _SP2MODES_H_
#define _SP2MODES_H_

/* The SP2 has 31 registers (0x00..0x1E). sp2.h defines this too, for its
 * bit-serial encoder; guard so either header may be included first. */
#ifndef SP2_NUM_REGS
#define SP2_NUM_REGS     31
#endif

#define SP2_MODE_COUNT   4       /* documented surround modes the mode node offers */

/* Register indices used by the presets and the tests, per the datasheet map. */
#define SP2_REG_GL1      0x00
#define SP2_REG_GR1      0x08
#define SP2_REG_VM       0x10    /* input attenuation                            */
#define SP2_REG_VC       0x11    /* FIR / feedback attenuation                   */
#define SP2_REG_VL       0x12    /* left output attenuation                      */
#define SP2_REG_VR       0x13    /* right output attenuation                     */
#define SP2_REG_C0       0x14
#define SP2_REG_C1       0x15
#define SP2_REG_T0       0x16    /* delay taps T0..T8 run 0x16..0x1E             */

#define SP2_ATTEN_0DB    0x3F    /* bit5 in-phase + level 0x1F (0 dB)            */
#define SP2_ATTEN_MAX    0x3F    /* widest meaningful attenuation byte (6 bits)  */
#define SP2_DELAY_MAX    0x1F    /* widest delay-tap value (5 bits, 100 ms)      */

/*
 * The presets, one row per mode, indexed by SP2 register address 0x00..0x1E.
 * Mode 0 is the initial-clear (all attenuators 0) "off" state; modes 1-3 open
 * the output attenuators and add spread delay taps for progressively wider
 * surround. Values stay inside the datasheet encodings (asserted by the test).
 */
static const unsigned char Sp2ModeTable[SP2_MODE_COUNT][SP2_NUM_REGS] =
{
    /* Mode 0 - Off (datasheet initial-clear: VM=VC=VL=VR=0) */
    { 0 },

    /* Mode 1 - Narrow surround: unity in/out, one early Lch/Rch tap */
    {
        /* GL1..GL8 */ SP2_ATTEN_0DB,0,0,0, 0,0,0,0,
        /* GR1..GR8 */ SP2_ATTEN_0DB,0,0,0, 0,0,0,0,
        /* VM VC VL VR */ SP2_ATTEN_0DB, 0x00, SP2_ATTEN_0DB, SP2_ATTEN_0DB,
        /* C0 C1 */ 0x20, 0x20,
        /* T0..T8 */ 0x00, 0x04,0x04, 0,0,0,0,0,0
    },

    /* Mode 2 - Wide surround: two taps, longer delays */
    {
        SP2_ATTEN_0DB,0x38,0,0, 0,0,0,0,
        SP2_ATTEN_0DB,0x38,0,0, 0,0,0,0,
        SP2_ATTEN_0DB, 0x00, SP2_ATTEN_0DB, SP2_ATTEN_0DB,
        0x20, 0x20,
        0x00, 0x08,0x10, 0,0,0,0,0,0
    },

    /* Mode 3 - Hall: feedback (VC) + three spread taps for reverberation */
    {
        SP2_ATTEN_0DB,0x38,0x30,0, 0,0,0,0,
        SP2_ATTEN_0DB,0x38,0x30,0, 0,0,0,0,
        SP2_ATTEN_0DB, 0x30, SP2_ATTEN_0DB, SP2_ATTEN_0DB,
        0x20, 0x20,
        0x00, 0x0C,0x14,0x1C, 0,0,0,0,0
    }
};

/* The number of documented modes (matches the spec's sp2_mode_count). */
static int
Sp2ModeCount(void)
{
    return SP2_MODE_COUNT;
}

/*
 * Copy the 31-register preset for `mode` into out (SP2_NUM_REGS bytes). An
 * out-of-range mode yields the off preset, so a bad selection is never audible.
 */
static void
Sp2ModePreset(int mode, unsigned char *out)
{
    int i;
    if (mode < 0 || mode >= SP2_MODE_COUNT)
        mode = 0;
    for (i = 0; i < SP2_NUM_REGS; i++)
        out[i] = Sp2ModeTable[mode][i];
}

#endif /* _SP2MODES_H_ */
