/*
 * chiptiming_test.c - userspace test for the calibrated chip-write delay table
 * (chiptiming.h) against the SDK timing figures (call/0013). No hardware needed.
 *   cc -I.. -o chiptiming_test chiptiming_test.c && ./chiptiming_test
 */
#include <stdio.h>
#include "../chiptiming.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

int main(void)
{
    /* Control Chip, per the SDK timing table. */
    CHECK(ChipWriteDelayUs(CHIP_CONTROL, 0x00) == 2500, "EEPROM reg 0x00 = 2500us");
    CHECK(ChipWriteDelayUs(CHIP_CONTROL, 0x04) == 450,  "reg 0x04 = 450us");
    CHECK(ChipWriteDelayUs(CHIP_CONTROL, 0x08) == 450,  "reg 0x08 = 450us");
    CHECK(ChipWriteDelayUs(CHIP_CONTROL, 0x09) == 5,    "reg 0x09 = 5us");
    CHECK(ChipWriteDelayUs(CHIP_CONTROL, 0x16) == 5,    "reg 0x16 = 5us");
    /* Boundaries stay in the short-delay class. */
    CHECK(ChipWriteDelayUs(CHIP_CONTROL, 0x03) == 5,    "reg below 0x04 = 5us");
    CHECK(ChipWriteDelayUs(CHIP_CONTROL, 0x17) == 5,    "reg above 0x16 = 5us");
    /* Other chips. */
    CHECK(ChipWriteDelayUs(CHIP_OPL3, 0) == 23,         "OPL3 write = 23us");
    /* The SDK requires 470ns between MMA register writes; 1us is the KeStallExecutionProcessor
     * granularity and 1us >= 470ns, so the speed-sensitive YMZ263 is never outrun on a fast CPU. */
    CHECK(ChipWriteDelayUs(CHIP_MMA,  0) >= 1,          "MMA write spacing >= the 470ns minimum");
    CHECK(ChipWriteDelayUs(CHIP_SP2,  0) >= 1,          "SP2 byte settles");
    /* Every delay is a positive, CPU-speed-independent microsecond count. */
    CHECK(ChipWriteDelayUs(CHIP_CONTROL, 0x04) > ChipWriteDelayUs(CHIP_CONTROL, 0x09),
          "the 450us class exceeds the 5us class");

    if (failures == 0) { printf("chiptiming_test: all checks passed\n"); return 0; }
    printf("chiptiming_test: %d failure(s)\n", failures);
    return 1;
}
