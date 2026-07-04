/*****************************************************************************
 * chiptiming.h - the calibrated inter-write delays the Ad Lib Gold chips need
 * (call/0013), in microseconds, from the SDK timing table.
 *
 * On period CPUs the delay between register writes came for free; on radically
 * faster CPUs the driver must stall explicitly, or it outruns what the chips
 * accept. A microsecond stall (KeStallExecutionProcessor) is CPU-speed-independent,
 * so the same driver behaves the same on slow and fast silicon. This table is pure,
 * so it is unit-tested against the SDK figures; the driver stalls
 * ChipWriteDelayUs(...) after each write. The ordering contract is the .tla timing
 * spec (spec/ChipTiming.tla).
 *****************************************************************************/
#ifndef _CHIPTIMING_H_
#define _CHIPTIMING_H_

#define CHIP_CONTROL 0
#define CHIP_OPL3    1
#define CHIP_MMA     2
#define CHIP_SP2     3

/*
 * Microseconds to stall after writing register `reg` of `chip`, per the SDK table:
 *   Control reg 0x00 (EEPROM) 2500us; regs 0x04-0x08 450us; regs 0x09-0x16 5us.
 *   OPL3 23us. MMA and the SP2 bit-serial byte a short settle.
 */
static unsigned long
ChipWriteDelayUs(int chip, unsigned reg)
{
    switch (chip)
    {
    case CHIP_CONTROL:
        if (reg == 0x00)                  return 2500;   /* EEPROM save/restore */
        if (reg >= 0x04 && reg <= 0x08)   return 450;    /* master, tone, mute */
        if (reg >= 0x09 && reg <= 0x16)   return 5;      /* volumes, DMA/IRQ config */
        return 5;
    case CHIP_OPL3:                       return 23;      /* OPL3 register write */
    case CHIP_MMA:                        return 1;
    case CHIP_SP2:                        return 1;
    default:                              return 1;
    }
}

#endif /* _CHIPTIMING_H_ */
