# Release Notes (v1.01)

*Addenda distributed with version 1.01 of the Ad Lib Gold Developer Toolkit.*

---

## AdLib®

GOLD

Developer Toolkit Reproduction

## Ad Lib Gold Developer Toolkit Version 1.01 Release Notes

## RL2DRV.EXE

The current version of the ROL2 playback TSR does not check for the presence of other drivers. The drivers used by RL2DRV.EXE must be loaded prior to executing this TSR.

When loading the ROL2 playback driver, the current directory must be the directory where the .SMP files are located.

## SAMPL.EXE

The current version of the Sample Editor does not use the AD Lib Gold drivers. It uses linkable libraries that conflict with the drivers.

In order to execute the Sample Editor, all drivers must be removed from memory, otherwise the program may hang or display an "Insufficient Memory" message.

## Disabling Interrupts when accessing the hardware

In order to avoid possible conflicts between applications that try to access the same hardware at the same time, it is recommended that interrupts be disabled when accessing the OPL3, the Control Chip or the MMA. This will avoid conflicts between applications, TSR programs and drivers that will be supplied with the Gold card in the future.

This procedure should be strictly adhered to for all software developed for the Gold card.

To insure that the interrupt flag status is not destroyed when re-enabling interrupts, the following procedure is recommended:

To disable interrupts:

pushf ; push flags, include interrupt flags

cli ; clear interrupts

To re-enable interrupts:

popf ; pop flag, includes interrupt flags

## TSR Hotkey reconfiguration

The Mixer Panel TSR and ROL2 Playback TSR hotkeys can now be reconfigured from the SETUP.EXE application. Please the README.TXT file for more details on the changes that were made to SETUP.EXE, RL2DRV.EXE and MIXER.EXE

## MIDI Driver, SCSI Driver, Windows DLLs

To be released
