# `program-disks-v1.00/disk2/G2S/G2S.DOC`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`G2S.DOC`](../../../../disks/program-disks-v1.00/disk2/G2S/G2S.DOC).

```text
SOUND BLASTER COMPATIBILITY

In this package we include a new driver.  This driver will let you emulate the 
Sound Blaster product from your Ad Lib Gold 1000.  This give you the power of 
all the Ad Lib, Sound Blaster and all the compagnies compatible with those.

You will find this driver on the diskette name "Program Disk No. 2" in the 
G2S directory.  The name of this driver is G2S.EXE.

Instalation:  To install this driver you just have to copy it to your Ad Lib 
Gold directory.  Type; copy a: (or b:) G2S.* c:\ (or the drive where is the Gold 
directory) Gold (or the name you did give to the Ad Lib Gold directory) ENTER.
Then follow the instruction bellow (or in the G2S.DOC file, this file).  A list 
of the compatible games and updates of the driver will be on the Ad Lib 
Multimedia BBS at (418)656-0351.

Ad Lib Multimedia.




┌──────────────────┬─────────────────────────────────────────────────────────
│ WHAT IS IT FOR ? │
└──────────────────┘    G2S tries to simulate the Sound Blaster card on
                        the Ad Lib Gold 1000. It's still in a development
                        stage, but it should work with around 50% of the
                        Sound Blaster applications. G2S simulates a DAC on
                        LPT1 as well (this may work even if the SB simulation
                        does not).

                        When run, G2S installs itself into XMS, leaving
                        just 352 bytes in DOS memory. G2S switches CPU
                        into V86 mode, traps any I/O at SB addresses and
                        converts them into corresponding I/O to GOLD
                        (easier said than done :-) ). Run it before
                        an SB application, G2S will do the rest. Just
                        don't try to run any Ad Lib Gold application
                        with G2S installed (reset your PC first) !

                        G2S requires the following:

                           - i386 compatible CPU.
                           - Ad Lib Gold configured for DMA 1 and IRQ 5.
                           - HIMEM.SYS or compatible XMS manager installed.
                           - _NO_ V86 mode software (EMM386, ...).

┌─────────────────┬──────────────────────────────────────────────────────────
│ ALPHA TESTING ! │
└─────────────────┘     I have decided to release this version to test
                        the demand for such a software. Please let me know
                        if you find the simulator useful.

                        I have tested several SB applications with G2S.
                        Several of them worked (DUNE, DUNE II, GODS,
                        GOBLIIINS, GOBLIIINS 2, MODPLAY, DMP, TETRAMED,
                        several demos etc.), while the others did not
                        (DRAGON'S LAIR, several more demos etc.). You may
                        find out that the DAC-on-LPT1 works better than
                        the simulated SB.

```
