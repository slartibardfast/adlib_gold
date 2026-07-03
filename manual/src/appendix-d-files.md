# Appendix D: List of Installed Files

*Every file installed by the toolkit: drivers, TSRs, applications, batch files and resources.*

---

## Appendix D: List of Installed Files

The Ad Lib Gold Developer Toolkit software included in the diskettes contains, when decompressed and installed, several files related to the utilization of the Gold card: drivers, application programs, music, sounds, and other various files. These files are:

## README.TXT

This file is not compressed on the diskette. It contains information on the latest program updates, if there are any, and any other pertinent information.

## CTRLDRV.EXE

This file is not compressed on the diskette. It contains the Ad Lib Gold Control chip driver. This low level driver is used by other programs, such as the Setup program, to implement: DMA channel & interrupt number select; sampling source select; sampling gain and input filter; microphone input gain; sampling output filtering and volume & tone control; mixing control; card localization setup and ID code reading; saving registers in non volatile memory.

## SETUP.EXE

This file is not compressed on the diskette. It contains the Installation and Configuration program. This program enables you to install the drivers and all associated programs, and to configure your Ad Lib Gold card.

## Drivers and TSRs

Are located in the "DRIVERS" subdirectory.

## FMDRV.EXE

This file contains the FM driver. This low level driver implements: preset change; note on; note off; pitch bend; volume and stereo positioning.

## WAVEDRV.EXE

This file contains the Sampling driver. This low level driver implements: recording and playback of samples by DMA and interrupt.

## TIMERDRV.EXE

## MIDIDRV.EXE

This file contains the Timer driver. This low level driver implements: the five timers of the Yamaha Magic Chip Set.

This file contains the MIDI driver. This low level driver implements: MIDI In and Out serial port control.

## RL2DRV.EXE

This file contains the ROL2 driver. This low level TSR driver implements: playback of the .RL2 music files and user control commands.

## AppendIx D

## List of Installed Files

## MIXER.EXE

This file contains the Mixer Panel TSR. This memory resident application allows for the control of the programmable volume and tone control, mixer settings, surround features, and setting of activation and volume keys.

## Application Programs (Executables)

## TESTGOLD.EXE

This file contains the Ad Lib Gold Test Program. This program enables you to verify that the Gold card is functioning properly in all of its different components.

## JUKEG.EXE

This file contains the executable code of Juke Box Gold Music Playback Program.

## ED.EXE

This file contains the executable code of Instrument Maker Gold.

## SAMPL.EXE

This file contains the executable code of Sample Maker Program.

## SURR.EXE

This file contains the executable code of Juke Box Gold with the Surround Sound Editor.

## PLAYRL2.EXE

This file contains the executable code of ROL2 Playback utility.

## PLAYDIGI.EXE

This file contains the executable code of Digitized Sound Playback utility.

## Batch Files

## TEST.BAT

This file contains the DOS command sequence which loads the necessary drivers and calls the Ad Lib Gold Test Program.

## JUKEGOLD.BAT

This file contains the DOS command sequence which loads the necessary drivers and calls the Juke Box Gold Music Playback Program.

## INSGOLD.BAT

This file contains the DOS command sequence which loads Instrument Maker Gold.

## SURROUND.BAT

This file contains the DOS command sequence which loads the necessary drivers and calls the Juke Box Gold Music Playback Program with the Surround Sound Editor.

## DRIVERS.BAT

This file contains the DOS command sequence which loads all Ad Lib Gold drivers.

## Other Files

## * *.RL2

The ".RL2" files contain the pieces of music that will be played with the Juke Box Gold.

## * *.SMP

The ".SMP" files contain the PCM digitized sounds. TESTGLD1.SMP is the sound file that will be used by the Test Program.

## SAMPLBNK.EQU

This file contains a translation table of digitized instrument sound names, which is used by the ROL2 Playback driver.

## OPL3.BNK

This file contains the FM synthesized instrument sounds compatible with the OPL3 FM synthesis chip.

## ED.RSR

This file contains the resources required by Instrument Maker Gold.

## SAMPL.RSR

This file contains the resources required by Sample Maker Program.

## Files Created by Programs

## JUKEGOLD.DAT

This file is created the first time you make a selection of songs in the Juke Box Gold program, permitting not to lose your selection even after rebooting the computer.

## TESTGLD1.SMP

This file is created by the Test Program when you test Sampling and Playback.

## SAMPLES.BNK

This bank file is created by the Sample Maker Program the first time you save a digitized sound in the ADPCM format.
