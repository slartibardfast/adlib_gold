/*
 * sp2_test.c - userspace test for the SP2 bit-serial encoder (sp2.h). Decodes the
 * command-byte sequence back to (register, value) and checks the ADR sequencing, so
 * the register-0x18 protocol (call/0012) is verified without hardware.
 *   cc -I.. -o sp2_test sp2_test.c && ./sp2_test
 */
#include <stdio.h>
#include "../sp2.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

/* Bits latch on the CLK-high (third) byte of each triple; ADR is low for the address
 * byte and high for the value byte. Decode both and confirm they round-trip. */
static void roundtrip(unsigned char addr, unsigned char data)
{
    unsigned char buf[SP2_BYTES_PER_REG];
    int n = Sp2EncodeReg(addr, data, buf);
    unsigned a = 0, d = 0;
    int idx = 0, k;
    CHECK(n == SP2_BYTES_PER_REG, "byte count");
    for (k = 0; k < 8; k++)                       /* address bits, MSB first, ADR low */
    {
        unsigned char clkhi = buf[idx + 2];
        a = (a << 1) | (clkhi & SP2_DATA);
        CHECK(!(clkhi & SP2_ADR), "ADR low during the address");
        idx += 3;
    }
    CHECK((buf[idx] & SP2_ADR) != 0, "address latched with ADR high");
    idx++;
    for (k = 0; k < 8; k++)                       /* value bits, MSB first, ADR high */
    {
        unsigned char clkhi = buf[idx + 2];
        d = (d << 1) | (clkhi & SP2_DATA);
        CHECK((clkhi & SP2_ADR) != 0, "ADR high during the value");
        idx += 3;
    }
    CHECK(!(buf[idx] & SP2_ADR), "value latched with ADR low");
    idx++;
    CHECK(idx == SP2_BYTES_PER_REG, "all bytes consumed");
    CHECK(a == addr, "address round-trips");
    CHECK(d == data, "value round-trips");
}

int main(void)
{
    roundtrip(0x00, 0x00);
    roundtrip(0x1E, 0xFF);
    roundtrip(0x0A, 0x55);
    roundtrip(0x15, 0xAA);
    roundtrip(0x07, 0x3C);
    if (failures == 0) { printf("sp2_test: all checks passed\n"); return 0; }
    printf("sp2_test: %d failure(s)\n", failures);
    return 1;
}
