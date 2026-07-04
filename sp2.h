/*****************************************************************************
 * sp2.h - the YM7128 (SP2) surround processor bit-serial protocol (call/0012).
 *
 * Distilled from the SDK's SURROUND sample code (appendix-sp2.md, "Write_Surround").
 * The SP2 is programmed through Control Chip register 0x18: DATA is bit 0, CLK is
 * bit 1, ADR is bit 2. A register write clocks 8 address bits (MSB first, ADR low),
 * latches the address (ADR high), clocks 8 value bits (ADR high), then latches the
 * value (ADR low). Each bit is three writes: CLK low, DATA set, CLK high (the rising
 * edge latches). This encoder is pure, so it is unit-tested without hardware; the
 * driver writes the returned command bytes to register 0x18.
 *****************************************************************************/
#ifndef _SP2_H_
#define _SP2_H_

#define SP2_DATA          0x01
#define SP2_CLK           0x02
#define SP2_ADR           0x04
#define SP2_NUM_REGS      31     /* registers in a surround preset (SDK sample) */
#define SP2_BYTES_PER_REG 50     /* 8*3 address + 1 latch + 8*3 value + 1 latch */

/*
 * Encode one SP2 register write into the sequence of Control-Chip register-0x18
 * command bytes. Writes SP2_BYTES_PER_REG bytes to out and returns the count.
 */
static int
Sp2EncodeReg(unsigned char addr, unsigned char data, unsigned char *out)
{
    int n = 0;
    int k;
    unsigned char cmd = 0;                                  /* CLK low, ADR low */

    for (k = 7; k >= 0; k--)                                /* address, MSB first, ADR low */
    {
        cmd = (unsigned char)(cmd & ~SP2_CLK);                              out[n++] = cmd;
        cmd = (unsigned char)((cmd & ~SP2_DATA) | ((addr >> k) & 1));       out[n++] = cmd;
        cmd = (unsigned char)(cmd | SP2_CLK);                              out[n++] = cmd;
    }
    cmd = (unsigned char)(cmd | SP2_ADR);                    out[n++] = cmd; /* latch address */

    for (k = 7; k >= 0; k--)                                /* value, MSB first, ADR high */
    {
        cmd = (unsigned char)(cmd & ~SP2_CLK);                              out[n++] = cmd;
        cmd = (unsigned char)((cmd & ~SP2_DATA) | ((data >> k) & 1));       out[n++] = cmd;
        cmd = (unsigned char)(cmd | SP2_CLK);                              out[n++] = cmd;
    }
    cmd = (unsigned char)(cmd & ~SP2_ADR);                   out[n++] = cmd; /* latch value */

    return n;
}

#endif /* _SP2_H_ */
