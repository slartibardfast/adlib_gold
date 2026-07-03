# Chapter 5 - DOS Software Drivers

*The DOS Control Features, FM (OPL3), Wave, Timer and MIDI drivers, and their complete function references.*

---

This batch file command loads all Ad Lib Gold drivers.

Introduction iii

5.1 Interfacing DOS Drivers with Applications 1

5.2 DOS Control Features Driver 3

Function Directory 4

5.3 DOS FM Synthesis Driver 57

Voice Allocation Structure 57

Function Directory 58

5.4 DOS Wave Driver 71

DOS Wave Driver Functions 71

5.5 DOS Timer Driver 93

Function Directory 95

5.6 DOS MIDI Driver

(To be released)

5.7 DOS SCSI CD-ROM Driver

(To be released)

Ad Lib supplies memory-resident drivers as part of its end-user software packages. Developers should, when possible, use the services provided by those drivers. There are a number of advantages to using memory-resident drivers:

- A lot of the applications supplied for the Gold Card are TSR applications. Memory-resident drivers provide applications with a common software core for managing shared resources. Digital playback and recording, MIDI input and output and the timers available on the card, for instance, share a same interrupt request line.

- Memory-resident drivers can easily be maintained and updated, independently of the application code.

The method used by applications to interface with the Ad Lib Gold memory-resident drivers minimizes the overhead in calling the driver services. For most applications, calling the drivers services will not introduce a noticeable overhead.

The following drivers are available as part of the developer toolkit.

## DOS Control Features Driver

The DOS Control Features driver supports the mixer features defined in Gold Sound Standard architecture

It also controls the configuration options of the Gold card, such as interrupt line selection, DMA channel allocation and address relocation.

Finally, it acts as a main management layer for all other drivers. It manages interrupt redirection to other drivers and keeps track of the location of the drivers.

For this reason, the Control features driver should always be the first one loaded.

## DOS FM Driver

The DOS FM driver gives access to the FM sound generation features of the YMF262 chip.

## DOS Wave Driver

The DOS Wave Driver supports the digitized sound playback and recording features of the YMZ263 chip.

## DOS Timer Driver

The DOS Timer driver supplies routines to control the hardware timers on both the YMF262 and the YMZ263 chips. The timers can be used for high-precision synchronization of events.

## DOS MIDI Driver

The DOS MIDI driver offers services to input and output data through the YMZ263 MIDI FIFO buffers.

Drivers load themselves in memory and hook themselves to the DOS multiplex interrupt 2FH. Once a driver is loaded in memory, it registers itself to the Control features driver. It transmits to the Control features driver the address for an entry point to be used by applications, and an address for an entry point to a routine that will handle interrupts from the Gold card.

There are two ways an application can interface with a driver. By issuing commands through int 2FH, or by directly calling the driver entry-point function, used for command dispatching. The second method is much more efficient.

To directly call the driver entry-point function, an application that wants to use the services of a driver first needs to issue an interrupt 2FH with register AH equal to ADLIB_MULTIPLEX_DRIVER_ID and register AL equal to the GET_ALL_ENTRY command (defined in ctrldrv.h). This will return a table containing the entry points for all Gold drivers present in memory.

The application can then communicate with a specific driver just by issuing a FAR call to a specific driver. This call will take as an argument a far pointer to an argument-passing structure which is specific to each driver.

The Developer Toolkit supplies a set of linkable modules that are used to ease the interfacing to the drivers, using the second method of interfacing. The Link modules hide all the complexity of interfacing to the drivers. The application just needs to call the drivers functions as if they were part of a linkable library.

The modules can use the second method of communicating with the drivers. In order to do this, they have to call an initialization function, InitxxxLink(). These functions will build up a table of function pointer to accelerate the calling of driver routines.

<table border="1"><tr><td>Driver link module</td><td>Description</td><td>Link Initialization routine</td></tr><tr><td>CTRLLNK</td><td>Control Features Driver</td><td>InitCtrlLink()</td></tr><tr><td>FMLNK</td><td>FM Synthesis Driver</td><td>InitFMLink()</td></tr><tr><td>WAVELNK</td><td>Wave Driver</td><td>InitWaveLink()</td></tr><tr><td>TIMERLNK</td><td>Timer Driver</td><td>InitTimerLink()</td></tr><tr><td>MIDILNK</td><td>MIDI Driver</td><td>InitMidiLink()</td></tr></table>

Once the InitxxxLink() function is called, applications just need to call the routines described in the following sections.

The source code for the Link modules has been supplied as part of the Developer Toolkit. Developers can use this source to customize Link modules to their version of the C compiler. The source code can also be used as a reference in debugging environments.

## Initialization Sequence

Developers using the Gold drivers should use the following initialization steps in order to insure that their applications do not try to access drivers that are not loaded in memory.

Applications should first check for the presence of the Gold card. They should then make sure that the driver is present by calling the appropriate function.

Once the application has made verified that the driver is loaded in memory, it can call the appropriate InitxxxLink() function.

<table border="1"><tr><td>Driver or service</td><td>Detection function</td><td>Returns</td></tr><tr><td>Gold Card Presence</td><td>CtGetGoldCardPresence()</td><td>0 if the Gold card is not found.1 If the Gold card is found</td></tr><tr><td>Control Features Driver</td><td>CtGetDriverPresence()</td><td>0 if the driver is not present.1 if the driver is loaded</td></tr><tr><td>FM Synthesis Driver</td><td>GetFMDriverStatus()</td><td>0xFF if the driver is present</td></tr><tr><td>WAVELNK</td><td>GetWaveDriverStatus()</td><td>0xFF if the driver is present</td></tr><tr><td>TIMERLNK</td><td>GetTimerDriverStatus()</td><td>0xFF if the driver is present</td></tr><tr><td>MIDILNK</td><td>GetMIDIDriverStatus()</td><td>0xFF if the driver is present</td></tr></table>

## SetControlRegister

**Syntax**

```c
int SetControlRegister(int reg, WORD val) Sets register 'reg' of Ad Lib Control Chip to 'val'.;
```

**Parameters**

int reg Which register to write to. WORD val Which value to write in register.

**Return value**

If no error 0, otherwise 1.

**Comments**

This low-level routine handles the details related to accessing the Control Chip, like interrupt disabling and reenabling. It also verifies that no access is made while the Control Chip's RB & SB bits are set.

## CtStoreConfigInPermMem

**Syntax**

```c
WORD CtStoreConfigInPermMem();
```

This causes all control chip registers, in their current state, to be written to permanent memory.

Parameters

None

Return value

1 if ok. 0 if a problem occured.

Comments

None

## CtRestoreConfigFromPermMem

**Syntax**

```c
WORD CtRestoreConfigFromPermMem() Restores the Gold crd configuration from permanent memory.;
```

## CtSetChannel0SampGain

CtSetChannel1SampGain

CtGetChannel0SampGain

CtGetChannel1SampGain

Syntax

```c
WORD CtSetChannel0SampGain(WORD value);
```

```c
WORD CtSetChannel1SampGain(WORD value);
```

```c
WORD CtGetChannel0SampGain(WORD value);
```

```c
WORD CtGetChannel1SampGain(WORD value);
```

Sets the gain of sampling channels.

Parameters

WORD value

Gain value from 0 to 255.

256 different values possible giving a range from approximately 0.04 to 10 times the input value. The exact gain is given by the equation:

Gain = (registerValue * 10) / 256 Linear gain.

Return value

1 if ok.

Comments

None

## CtSetChannelFilter0Mode

CtSetChannel1FilterMode

**Syntax**

```c
WORD CtSetChannel0FilterMode(WORD value);
```

```c
WORD CtSetChannel1FilterMode(WORD value);
```

Sets the antialiasing fiters in the proper mode for the channel.

**Parameters**

WORD value

0 = playback mode, 1 = sample mode

**Return Value**

1 if ok.

**Comments**

This filter MUST be set in sample mode before sampling.

This filter MUST be set in playback mode before playback.

The Ad Lib Gold card uses the same antialiasing filters during sampling and playback. The appropriate filter mode must be set before any sampling or playback operation.

## CtGetChannelFilter0Mode

CtGetChannel1FilterMode

Syntax

```c
WORD CtGetChannel0FilterMode(void);
```

```c
WORD CtGetChannel1FilterMode(void);
```

Returns the current antialisaing filter mode for the channel.

Parameters

None

Return Value

0: playback mode. 1: Sampling mode

Comments

None

## CtStereoMonoAuxSamp

**Syntax**

```c
WORD CtStereoMonoAuxSamp(WORD value);
```

Forces auxiliary inputs to work monophonically or sterophonically.

**Parameters**

WORD value

0 = auxiliary input is stereo, 1 = auxiliary input is mono

**Return Value**

1 if ok.

**Comments**

The microphone and telephone inputs are monophonic sources and can only be sampled monophonically on channel 0. However, the auxiliary inputs are normally sampled in stereo on both channel 0 and 1 at the same time. This stereo audio input can be turned monophonic and sampled on channel 0 using this function.

## CtGetStereoMonoAuxSamp

**Syntax**

```c
WORD CtGetStereoMonoAuxSamp(void);
```

Returns whether the auxiliary inputs are used for monophonic sampling or stereophonic sampling.

Parameters

None

Return Value

0 = auxiliary input is stereo, 1 = auxiliary input is mono

**Comments**

None

## CtEnabDisabMicroOutput

**Syntax**

```c
WORD CtEnabDisabMicroOutput(WORD value);
```

Enables/disables microphone output.

**Parameters**

WORD value

0 = Microphone output enabled, 1 = Microphone output disabled

**Return Value**

1 if ok.

**Comments**

When using the microphone input and the normal loudspeaker outputs of the audio card, audio feedback could result. In normal mode, microphone output is enabledd When disabled, the microphone signal is cut from the output of the card but sent to the telephone output, eliminating possible causes of feedback.

## CtGetEnabDisabMicroOutput

**Syntax**

```c
WORD CtGetEnabDisabMicroOutput();
```

When using the microphone input and the normal loudspeaker outputs of the audio card, audio feedback could result. In normal mode, this bit is set to 0. When set to 1, the microphone signal is cut from the output of the card and only sent to the telephone output, eliminating possible causes of feedback.

Parameters

None

Return Value

0 = Microphone output enabled, 1 = Microphone output disabled Comments

See CtEnabDisabMicroOutput()

## CtEnabDisabInternPcSpeak

**Syntax**

```c
WORD CtEnabDisabInternPcSpeak(WORD value);
```

Enables/Disables redirection of the PC internal speaker output to to the Gold mixer.output

**Parameters**

WORD value

0 = Disconnect internal PC speaker,

1 = Connect internal PC speaker

**Return Value**

1 if ok.

**Comments**

This can enable the PC internal speaker signal to be mixed with the audio signals of a Gold card (directly, without any mixer volume control).

## CtGetEnabDisabInternPcSpeaker

Syntax

```c
WORD CtGetEnabDisabInternPcSpeaker();
```

Returns the state of redirection of the PC speaker.

Parameters

None

Return Value

0 = Internal PC speaker not redirected.

1 = Internal PC speaker redirected

Comments

None

## CtSelectInterruptLineNbr

**Syntax**

```c
WORD CtSelectInterruptLineNbr(WORD value);
```

Selects the interrupt request line used by the audio portion of the Gold hardware.

**Parameters**

WORD value

`0 = IRQ3, 1 = IRQ4, 2 = IRQ5, 3 = IRQ7`

`4 = IRQ10, 5 = IRQ11, 6 = IRQ12, 7 = IRQ15`

**Return Value**

1 if ok.

**Comments**

The interrupt line is used by OPL3, MMA and telephone hardware. Valid interrupt lines on an XT are IRQ3, IRQ4, IRQ5 and IRQ7. Valid interrupt lines on an AT are IRQ3, IRQ4, IRQ5, IRQ7, IRQ10, IRQ11, IRQ12 and IRQ15.

## CtGetInterruptLineNbr

**Syntax**

```c
WORD CtGetInterruptLineNbr();
```

Returns a number indicating the interrupt line used by the audio portion of the Gold hardware..

Parameters

None

Return Value

`0 = IRQ3, 1 = IRQ4, 2 = IRQ5, 3 = IRQ7`

`4 = IRQ10, 5 = IRQ11, 6 = IRQ12, 7 = IRQ15`

Comments

None

## CtSelectDMA0ChannelSampChan

CtSelectDMA1ChannelSampChan

**Syntax**

```c
WORD CtSelectDMA0ChannelSampChan(WORD value);
```

```c
WORD CtSelectDMA1ChannelSampChan(WORD value);
```

Allocates DMA channel for the specified MMA sampling channel.

**Parameters**

<table border="1"><tr><td>WORD</td><td>value</td></tr><tr><td>0=DMA0</td></tr><tr><td>1=DMA1</td></tr><tr><td>2=DMA2</td></tr><tr><td>3=DMA3</td></tr></table>

Return Value 1 if ok.

**Comments**

Only DMA channels 1,2 and 3 are available on model Gold 1000. All listed DMA channels are available on the Gold 2000 and 2000MC.

## CtGetDMA0ChannelSampChan

## CtGetDMA1ChannelSampChan

Syntax

```c
WORD CtGetDMA0ChannelSampChan();
```

```c
WORD CtGetDMA1ChannelSampChan();
```

Returns a number indicating the DMA channel used by the specified sampling channel.

Parameters

None

Return Value

The sampling channel used.

Comments

None

## CtEnabDisabDMA0SampChan

CtEnabDisabDMA1SampChan

**Syntax**

```c
WORD CtEnabDisabDMA0SampChan(WORD value);
```

```c
WORD CtEnabDisabDMA1SampChan(WORD value);
```

Disables or enables use of DMA channel for sampling channel.

Parameters WORD value 0 = disable, 1 = enable

Return Value 1 if ok.

Comments None

## CtGetEnabDisabDMA0SampChan

CtGetEnabDisabDMA1SampChan

Syntax

```c
WORD CtGetEnabDisabDMA0SampChan();
```

```c
WORD CtGetEnabDisabDMA1SampChan();
```

Tells if the DMA channel is disabled or enabled for the specified sampling channel.

Parameters

None

Return Value

0 = disabled, 1 = enabled

Comments

None

## CtSetRelocationAddress

**Syntax**

```c
WORD CtSetRelocationAddress(value);
```

Set s the base ports address for MMA, OPL3 and control chip.

**Parameters**

WORD value

New I/O address, divided by 8.

Return Value 1 if ok.

Comments

None

## CtGetRelocationAddress

**Syntax**

```c
WORD CtGetRelocationAddress();
```

Returns the base port addresses for MMA, OPL3 and control chip.

Parameters

None

Return Value

New base I/O address, divided by 8.

Range is from 0 to 127

Comments

None

## CtSetMixerLevelForFMLeft

CtSetMixerLevelForFMRight CtSetMixerLevelForLeftSamplePb CtSetMixerLevelForRightSamplePb CtSetMixerLevelForAuxLeft CtSetMixerLevelForAuxRight CtSetMixerLevelForMicrophone CtSetMixerLevelForTelephone

**Syntax**

```c
WORD CtSetMixerLevelForFMLeft(WORD value);
```

```c
WORD CtSetMixerLevelForFMRight(WORD value);
```

```c
WORD CtSetMixerLevelForLeftSamplePb(WORD value);
```

```c
WORD CtSetMixerLevelForRightSamplePb(WORD value);
```

```c
WORD CtSetMixerLevelForAuxLeft(WORD value);
```

```c
WORD CtSetMixerLevelForAuxRight(WORD value);
```

```c
WORD CtSetMixerLevelForMicrophone(WORD value);
```

```c
WORD CtSetMixerLevelForTelephone(WORD value);
```

Sets the volume for the specified device

**Parameters**

## WORD value

Volume level from 128 to 255 whereas 128 is the minimum, 255 the maximum.

**Return Value**

1 if ok.

**Comments**

Writing a value less than 128 will result in a signal with negative polarity and should be avoided because the resulting signal may cancel out another signal of opposite polarity.

## CtGetMixerLevelForFMLeft

CtGetMixerLevelForFMRight

CtGetMixerLevelForLeftSamplePb

CtGetMixerLevelForRightSamplePb

CtGetMixerLevelForAuxLeft

CtGetMixerLevelForAuxRight

CtGetMixerLevelForMicrophone

CtGetMixerLevelForTelephone

**Syntax**

```c
WORD CtGetMixerLevelForFMLeft();
```

```c
WORD CtGetMixerLevelForFMRight();
```

```c
WORD CtGetMixerLevelForLeftSamplePb();
```

```c
WORD CtGetMixerLevelForRightSamplePb();
```

```c
WORD CtGetMixerLevelForAuxLeft();
```

```c
WORD CtGetMixerLevelForAuxRight();
```

```c
WORD CtGetMixerLevelForMicrophone();
```

```c
WORD CtGetMixerLevelForTelephone();
```

Returns the volume of the specified device.

**Parameters**

None

**Return Value**

Volume level from 128 to 255 whereis 128 is the minimum, 255 the maximum.

**Comments**

None

## CtSetOutputVolumeLeft

CtSetOutputVolumeRight

**Syntax**

```c
WORD CtSetOutputVolumeLeft(WORD value);
```

```c
WORD CtSetOutputVolumeRight(WORD value);
```

Sets the final output volume

**Parameters**

WORD value Volume level from 0 to 255

Return Value 1 if ok.

**Comments**

There are actually 64 final volume levels. The driver divides the specified value by 4.

## CtGetOutputVolumeLeft

## CtGetOutputVolumeRight

Syntax

```c
WORD CtGetOutputVolumeLeft();
```

```c
WORD CtGetOutputVolumeRight();
```

Returns the the final output volume

Parameters

None

Return Value

Final output volumefrom 0 to 255

**Comments**

There are actually 64 final volume levels. The driver multiplies the specified value by 4 in the return value.the return value may not correspond exactly to the value specified with CTSetOutputVolumeXXX().

## CtSetOutputBassLevel

CtSetOutputTrebleLevel

**Syntax**

```c
WORD CtSetOutputBassLevel(WORD value);
```

```c
WORD CtSetOutputTrebleLevel(WORD value);
```

Sets the output bass and treble level.

**Parameters**

WORD value Range from -128 to 127.

Return Value 1 if ok.

**Comments**

Negative values decreases trebleor bass, positive numbers, increase treble or bass. 0 does not alter sound.

## CtGetOutputBassLevel

## CtGetOutputTrebleLevel

Syntax

```c
WORD CtGetOutputBassLevel();
```

```c
WORD CtGetOutputTrebleLevel();
```

Returns the bass or treble level setting.

Parameters

None

Return Value

Bass or treble setting, from -127 to 127

**Comments**

Since only 4 bits are actually used in the control Chip, the result obtained can differ with the value written using the CtSetOutputBassLevel() and CtSetOutputTrebleLevel function, due to rounding errors.

## CtEnabDisabOutputMuting

**Syntax**

```c
WORD CtEnabDisabOutputMuting(value) Disables or enables output muting.;
```

Parameters WORD value 0 = disable, 1 = enable

Return Value 1 if ok.

Comments None

## CtGetEnabDisabOutputMuting

Syntax

```c
WORD CtGetEnabDisabOutputMuting();
```

Returns a value indicating if output muting is disabled or enabled.

Parameters

None

Return Value

0: disabled, 1: enabled

Comments

None

## CtSelectSCS1InterruptNumber

**Syntax**

```c
WORD CtSelectSCSIInterruptNumber(WORD value);
```

Selects an interrupt request line for the SCSI hardware on the Goldcard.

**Parameters**

<table border="1"><tr><td>WORD</td><td>value</td></tr><tr><td>0=IRQ3</td></tr><tr><td>1=IRQ4</td></tr><tr><td>2=IRQ5</td></tr><tr><td>3=IRQ7</td></tr><tr><td>4=IRQ10</td></tr><tr><td>5=IRQ11</td></tr><tr><td>6=IRQ12</td></tr><tr><td>7=IRQ15</td></tr></table>

**Return Value**

1 if ok.

**Comments**

Valid interrupt lines on an XT are IRQ3, IRQ4, IRQ5 and, IRQ7. Valid interrupt lines on an AT are IRQ3, IRQ4, IRQ5, IRQ7, IRQ10, IRQ11, IRQ12 and IRQ15.

## CtGetSCSIInterruptNumber

**Syntax**

```c
WORD CtGetSCSIInterruptNumber();
```

Returns a number indicating the interrupt request line used by the SCSI hardware on the Gold card.

Parameters

None

Return Value

Interrupt request line:

Comments

None

## CtEnabDisabSCSIIinterrupt

**Syntax**

```c
WORD CtEnabDisabSCSIIinterrupt(value) Disables or enables interrupt from SCSII.;
```

Parameters WORD value 0 = disable, 1 = enable

Return Value 1 if ok.

Comments None

## CtEnabDisabSCSIDMA

**Syntax**

```c
WORD CtEnabDisabSCSIDMA(value) Disables or enables DMA transfers on SCSI hardware.;
```

Parameters

WORD

0 = disable, 1 = enable

Return Value 1 if ok.

Comments None

## CtGetEnabDisabSCSIInterrupt

**Syntax**

```c
WORD CtGetEnabDisabSCSIInterrupt();
```

Returns 1 if interrupts are enabled on the SCSI hardware.

Parameters

None

Return Value

0: Interrupts are disabled

1: Interrupts are enabled

Comments

None

## CtGetEnabDisabSCSIDMA

Syntax

```c
WORD CtGetEnabDisabSCSIDMA();
```

Returns 1 if DMA transfers are enabled on the SCSI hardware.

Parameters

None

Return Value

0: DMA is disabled

1: DMA is enabled

Comments

None

## CtSelectSCSIDMAChannel

**Syntax**

```c
WORD CtSelectSCSIDMAChannel(WORD value);
```

Assigns a DMA channel to the SCSI hardware of the Gold Card.

**Parameters**

<table border="1"><tr><td>WORD</td><td>value</td></tr><tr><td>0=DMA0</td></tr><tr><td>1=DMA1</td></tr><tr><td>2=DMA2</td></tr><tr><td>3=DMA3</td></tr></table>

**Return Value**

1 if ok.

**Comments**

Valid DMA channels are 0-3. Other channel numbers are reserved for future extensions.

## CtGetSCSIDMAChannel

**Syntax**

```c
WORD CtGetSCSIDMAChannel();
```

Returns the number of the DMA channel Assigned to the SCSI hardware of the Gold card.

Parameters

None

Return Value

Comments

None

## CtSetSCSIRelocationAddress

**Syntax**

```c
WORD CtSetSCSIRelocationAddress(value);
```

Sets the base port address addresses for SCSI controller.

**Parameters**

WORD value

New base I/O address divided by 8.

Range from 0 to 127.

Return Value

1 if ok.

Comments

None

## CtGetSCSIRelocationAddress

**Syntax**

```c
WORD CtGetSCSIRelocationAddress();
```

Returns the base port address for SCSI controller.

Parameters

None

Return Value

New base I/O address divided by 8. Range from 0 to 127.

Comments

None

## CtSetHangUpPickUpTelephoneLine

Syntax

```c
WORD CtSetHangUpPickUpTelephoneLine(WORD value) Hangs up or picks up telephone.;
```

Parameters

WORD value

0 = Disconnect telephone line,

1 = Connect telephone line

Return Value

1 if ok.

Comments

None

## CtGetHangUpPickUpTelephoneLine

Syntax

```c
WORD CtGetHangUpPickUpTelephoneLine();
```

Returns a value telling if the telephone line is on-hook or off-hook.

Parameters

None

Return Value

0: telephone line is on-hook (not connected)

1: telephone line is off-hook (connected)

Comments

None

## CtSelectOutputSources

**Syntax**

```c
WORD CtSelectOutputSources(value);
```

Selects final output mixing redirection.

**Parameters**

0 = left mixer channel to left output & right mixer channel to right output,

1 = left mixer channel to both left and right outputs,

2 = right mixer channel to both left and right outputs.

**Return Value**

1 if ok.

**Comments**

On the Adlib Gold cards, mixing and volume control is performed in two stages. First, all sources are sent to a stereo mixer. Then, the stereo output of the mixer is fed into the final volume control circuitry. The final left and right outputs can be mixed in the fashion described above.

## CtGetOutputSources

Syntax

```c
WORD CtGetOutputSources();
```

Returns the final mixer redirection mode.

Parameters

None

Return Value

0 = left mixer channel to left output & right mixer channel to right output,

1 = left mixer channel to both left and right outputs,

2 = right mixer channel to both left and right outputs.

Comments

None

## CtSelectOutputMode

**Syntax**

```c
WORD CtSelectOutputMode(value);
```

Controls the effect applied to the final output.

**Parameters**

WORD value

0 = Forced mono,

1 = linear stereo,

2 = pseudo stereo,

3 = spatial stereo.

**Return value**

1 if ok.

**Comments**

Linear stereo is ordinary, with no effects added. The spatial and pseudo-stereo effects will be useful primarily when the original source is monophonic.

## CtGetOutputMode

Syntax

WORD

CtGetOutputMode()

Returns the effect applied to the final output .

Parameters

None

Return value

0 = Forced mono,

1 = linear stereo,

2 = pseudo stereo,

3 = spatial stereo.

Comments

None

## GetControlRegister

**Syntax**

```c
WORD GetControlRegister(reg) Returns value stored on register 'reg' of Ad Lib Control Chip.;
```

Parameters int reg Which register to read from.

Return value Returns the WORD at the register position.

Comments None

## CtGetBoardIdentificationCode

**Syntax**

```c
WORD CtGetBoardIdentificationCode() Returns the board identification code.;
```

Parameters

None

Return value

Board identification code:

0- Gold 2000,

1- Gold 1000,

2- Gold 2000 MC.

Comments

None

## CtGetBoardOptions

**Syntax**

```c
WORD CtGetBoardOptions();
```

Returns a bit pattern indicating the options present on boardpresent

Parameters

None

Return value

Bit 0-3 (0 = not present, 1 = installed)

bit 0 - Telephone,

bit 1 - Surround,

bit 2 - SCSI,

bit 3 - Currently unused

Comments

None

## CtGetControllerStatus

**Syntax**

```c
WORD CtGetControllerStatus();
```

Returns the interrupt controller status.

Parameters

None

Return value

bit 0 - equals 1 when an OPL3 interrupt is pending,

bit 1 - equals 1 when an MMA interrupt is pending,

bit 2 - equals 1 when an telephone interrupt is pending,

bit 3 - equals 1 when a SCSI interrupt is pending,

bit 6 - equals 1 when the Control Chip is currently, occupied writing a value to the Mixer Chip or the Volume Control Chip.

bit 7 Set to 1 when the Control Chip is busy writing its internal registers to the external EEPROM chip. This bit must be polled after activating the "Store configuration" sequence to make sure that the Control Chip is free to proceed with another operation.

**Comments**

Bit 7 and Bit 6 are polled by all set functions, prior to writing to the registers, to make sure that the Control Chip is free to proceed with another operation.

## CtGetRingTelephoneStatus

<table border="1"><tr><td>Syntax</td><td></td></tr><tr><td>WORD</td><td>CtGetRingTelephoneStatus()</td></tr><tr><td colspan="2">Gets telephone status.</td></tr><tr><td>Parameters</td><td></td></tr><tr><td>None</td><td></td></tr><tr><td>Return value</td><td></td></tr><tr><td>bit 0:</td><td>"Ring signal" (0 = no ring, 1 = ring)</td></tr><tr><td>Comments</td><td></td></tr><tr><td>None</td><td></td></tr></table>

## CtGetInterruptRoutine

**Syntax**

```c
WORD CtGetInterruptRoutine();
```

This routine returns the corresponding interrupt number associated with the interrupt request line used by the audio section.

Parameters

None

Return value

Corresponding interrupt number

Comments

Useful utility mostly used when setting interrupt vectors.

## CtGetGoldCardPresence

**Syntax**

```c
WORD CtGetGoldCardPresence();
```

Checks for Gold card presence.

Parameters

None

Return value

1 if any Gold card is found. 0 if no Gold card is found.

None

## CtGetDriverPresence

**Syntax**

```c
WORD CtGetDriverPresence();
```

Checks for Ad Lib Gold Control Driver.

Parameters

None

Return value

1 if the Gold Control driver is found. Returns 0 otherwise.

Comments

None

## CtProgramSurroundPreset

**Syntax**

```c
WORD CtProgramSurroundPreset(ptrData);
```

This routine will store a preset into the surround module. The preset is defined by a 32 bytes array passed as argument.

**Parameters**

BYTE *ptrData

Pointer to the array of 32 bytes.

**Return value**

0 if no error, otherwise 1, no surround module.

**Comments**

The 1 bytes of the Surround Preset are a 1 to 1 image of the 32 registers of the Surround processor.

The Ad Lib Gold FM Synthesis Driver offers services to access features of the OPL3 FM Chip.

## Voice Allocation Structure

The OPL3 chip contains 36 operators which can be combined in various ways to create 1-, 2- or 4-operator voices. (You may wish to refer to the "FM Driver Voices" table on the next page for the purposes of this discussion.)

The 4-operator voices offer the richest sound. Up to six 4-operator voices can be used simultaneously. In the FM Driver, the 4-operator voices are numbered 0,2,4,6,8 and 10. By default, all six 4-operator voices are enabled. They may be selectively disabled, thus creating two 2-operator voices.

In the FM Driver, when 4-operator voice x is disabled, the two 2-operator voices are numbered x and x+1. For example, if 4-operator voice #2 was disabled, the resulting 2-operator voices will be numbered 2 and 3.

Use Set4OpMaskOPL3() to determine the grouping of the units in either 2 operator or 4 operator voices.

Six of the chip's operators can only be used as three 2-operator voices. These three voices are numbered 12, 13 and 14.

The configuration of the remaining 6 operators depends on whether the card is in melodic or percussive mode. In melodic mode, these 6 operators are configured as three 2-operator voices: driver voice numbers 15,16 and 18. In percussive mode, the 6 operators are used to create one 2-operator voice (the bass drum) and four 1-operator voices (the remaining drum sounds). The percussive voices are driver voice numbers 15 through 19.

Use SetPercModeOPL3() to configure this section in the melodic or percussive mode.

<table border="1"><tr><td>4 operator voice number</td><td>2 operator voice number</td><td>Percussive voice number</td></tr><tr><td>0</td><td>0,1</td><td>-</td></tr><tr><td>2</td><td>2,3</td><td>-</td></tr><tr><td>4</td><td>4,5</td><td>-</td></tr><tr><td>6</td><td>6,7</td><td>-</td></tr><tr><td>8</td><td>8,9</td><td>-</td></tr><tr><td>10</td><td>10,11</td><td>-</td></tr><tr><td>-</td><td>12</td><td>-</td></tr><tr><td>-</td><td>13</td><td>-</td></tr><tr><td>-</td><td>14</td><td>-</td></tr><tr><td>-</td><td>15</td><td>15(BD)</td></tr><tr><td>-</td><td>16</td><td>16(HH)</td></tr><tr><td>-</td><td>-</td><td>17(SD)</td></tr><tr><td>-</td><td>18</td><td>18(TOM)</td></tr><tr><td>-</td><td>-</td><td>19(CYMB)</td></tr></table>

FM Driver Voices

## Function Directory

The following section is an alphabetically arranged definition of all the functions available in the FM Synthesis Driver.

## InitOPL3

**Syntax**

```c
void InitOPL3(address) Initializes the FM Chip.;
```

**Parameters**

WORD address

Port address of the FM chip.

**Comments**

After initialization, percussion voices are available and all 4 op-voices are enabled.

## LeftRightOPL3

Syntax

```c
void LeftRightOPL3(voiceNum, leftRight) Modifies the stereo position of the voice.;
```

Parameters

int voiceNums

VoiceNumber between 0 and 19.

int leftRight

Position of the specified voice:

0: Center.

1: Left.

2: Right.

## LevelOPL3

**Syntax**

```c
void LevelOPL3(voiceNum, level) Specify the individual volume for a voice.;
```

**Parameters**

int voiceNum Voice number between 0 and 19

int level Volume for the voice. This in an integer number between 0 and 127. Volume scaling is linear.

**Comments**

The volume is scaled linearly by the driver software.

## NoteOffOPL3

Syntax

```c
void NoteOffOPL3(voiceNum);
```

Starts the decay of the timbre currently playing on the voice.

Parameters

int

voiceNum

VoiceNumber between 0 and 19.

## NoteOnOPL3

**Syntax**

```c
void NoteOnOPL3(voiceNum, note);
```

Starts playing a note on the specified voice.

**Parameters**

int voiceNum

VoiceNumber between 0 and 19.

int note

MIDI value for the note played, in the range 12-107.

**Comments**

If a note is already playing on the specified voice, the frequency of the voice will be modified. However, the attack for the timbre will not be heard. To reattack the timbre on the specified voice, a NoteOffOPL3 must be issued.

## PitchbendOPL3

**Syntax**

```c
void PitchBendOPL3(voiceNum, pitchBend);
```

Modifies the pitch bend scaling factor for the melodic voice.

**Parameters**

int voiceNum

Melodic voiceNumber between 0 and 15.

WORD pitchBend

Pitch bend scaling factor within the range set in SetGlobalOPL3().

The pitch bend scaling factor is a 14 bit unsigned value. 0 is the maximum negative pitch bend, 0x2000 is no bend and 0x3FFF is the maximum positive pitch bend.

**Comments**

Percussive voices cannot be bent.

## PresetOPL3

**Syntax**

```c
void PresetOPL3(voiceNum, timbrePtr);
```

Assigns a patch to the specified voice.

**Parameters**

int voiceNum voiceNumber between 0 and 19

struct TIMBRE *timbrePtr

pointer to a description (28 bytes) of the patch assigned to the voice.

**Comments**

If a 4 operator description is sent to a 2-op voice, only the first two operators are considered.

Appendix A: FM Patch format further describes the structure pointed to by timbrePtr.

## QuitOPL3

Syntax

```c
void QuitOPL3();
```

Resets the FM chip in the compatible mode.

Parameters

None.

Comments

This should be called by all applications prior to leaving, in order to put the OPL3 chip back in the Ad Lib compatible mode.

## Set4OpMaskOPL3

**Syntax**

```c
void Set4OpMaskOPL3(mask);
```

Enables or disables 4-op voices.

**Parameters**

WORD mask

Bit mask of enabled 4-op voices (in bits 0-5).

Bits 0-5 of mask specify whether the corresponding voice is in 4-op mode (bit set to 1) or in 2-op mode (bit cleared to 0).

Bit 0 corresponds to voice 0 (0-1 in 2 op), bit 1 to voice 2 (2-3 in 2 op) etc. (See to table 1 in the Voice Allocation section of this document).

**Comments**

There is a maximum of 6 4-op voices.

## SetGlobalOPL3

Syntax

```c
void SetGlobalOPL3 (noteSelectEnable, amplitudeModEnable, vibDepthEnable, pitchBendRange);
```

Modifies global operating parameters of the OPL3.

**Parameters**

BOOL noteSelectEnable For future use. Set to 0 for now.

BOOL. amplitudeModEnable

When non-zero, enables amplitude modulation for all timbres that have an amplitude modulation defined.

BOOL vibDepthEnable

When non-zero, enables vibrato for all timbres that have a vibrato depth defined.

int pitchBendRange

Range of the pitch bend in semitones. Integer between 0-12.

## SetPercModeOPL3

**Syntax**

```c
void SetPercModeOPL3(newState);
```

Sets the OPL3 in melodic or percussive mode.

**Parameters**

BOOL newState

True for percussive mode, false for melodic mode.

**Comments**

If newState is true, disables melodic voices 15-18 and enables percussive voices 15-19 instead.

If newState is false, melodic voices 15-18 are enabled in place of percussive voices 15-19.

The Ad Lib Wave Driver is a high level software interface to the sampling hardware of the Gold Card. Its interface is inspired by the Microsoft Multimedia Wave Driver specifications. But in order to support the target hardware and software more efficiently, some adaptations were necessary. The main differences are:

The support of ADPCM as well as PCM formats.

The support of a stereo sample format.

The control of multiple transfer modes from memory to hardware (polling, interrupt, DMA). (This implies an extension of the WaveFormat structure to include the new parameters.)

The use of a callback function as a message-passing mechanism between the application and the driver during waveform recording and playback.

Some syntactical differences were introduced in the naming of functions and structures, in order to respect the Ad Lib naming conventions already in use. Please note that this specification is a preliminary document and is incomplete. More functions will be added to this preliminary specification.

The Wave Driver will first be available as a linkable library of functions. It will also be made available to developers in the form of a memory-resident driver, interacting with applications via an interrupt-driven protocol.

## DOS Wave Driver Functions

The following section is an alphabetically arranged definition of all the functions available in the Wave Driver.

## InitWaveDriver

Syntax

```c
void InitWaveDriver();
```

Initializes the wave driver. It is to be called only once by the application.

Parameters

None

Return value

None
## QuitWaveDriver

**Syntax**

```c
Word QuitWaveDriver ();
```

This function resets the driver. IMPORTANT: This must be called before returning to the DOS.

Parameters

None

Return value

None

## WaveInAddBuffer

**Syntax**

```c
Word WaveInAddBuffer (hWaveIn, lpWaveInHdr, wSize);
```

Sends a buffer to a waveform input device. When the buffer is full, the application is notified.

**Parameters**

HWaveIn hWaveIn Specifies a handle to the waveform device which is to receive the buffer.

LpWaveHdr lpWaveInHdr Specifies a far pointer to a WaveHdr structure that identifies the buffer.

Word wSize Specifies the size of the WaveHdr structure.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

WERR_INVALIDHANDLE Specified device handle is invalid

## WaveInClose

**Syntax**

```c
Word WaveInClose(hWaveIn);
```

Closes the specified waveform input device.

**Parameters**

## HWaveIn hWaveIn

Specifies a handle to the waveform input device to be closed. If the function is successful, the handle is no longer valid after this call.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

WERR_INVALIDHANDLE

Specified device handle is invalid

## WERR_STILLPLAYING

There are still buffers in the queue

**Comments**

If there are input buffers that have been sent with WaveInAddBuffer, and have not been used, the close operation will fail. Call in WaveInReset to mark all pending buffers as done.

## WaveInGetNumDevs

Syntax

```c
Word WaveInGetNumDevs();
```

Retrieves the number of waveform input devices present in the system.

Parameters

None

Returns value

Returns the number of waveform input devices in the system.

## WaveInOpen

**Syntax**

```c
Word WaveInOpen (lphWaveIn, wDeviceID, lpFormat, dwCallBack, dwCallBackData, dwFlags);
```

Opens the specified waveform input device for recording.

**Parameters**

## HWaveIn far *IpWaveIn

Specifies a pointer to a HWaveIn handle. This location is filled with a handle identifying the opened waveform input device. Use this handle to identify the device when calling other waveform input functions.

This parameter may be NULL if the WAVE_FORMAT_QUERY flag is specified for the dwFlags.

Word wDeviceID

## LpWaveFormat lpFormat

Identifies the waveform input device that is to be opened.

Specifies a far pointer to a WaveFormat data structure that identifies the desired format for recording the waveform data.

```c
int (far * dwCallBack) (HWaveIn dev, LpWaveHdr block, DWord dwCallBackData);
```

Specifies the address of a callback function. The callback function is called by the driver during recording to process messages related to the progress of the recording.

Specify NULL for this parameter if no callback is desired.

## DWord dwCallbackData

Specifies 32 bits of user defined data that is passed to the callback function.

## DWord

dwFlags

Specifies flags for opening the device.

## WAVE_FORMAT_QUERY

If this flag is specified, the device driver will determine if it supports the given format, but will not actually open the device.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

WERR_ALLOCATED

Specified resource is already allocated.

WERR_BADDEVICEID

Specified device is out of range.

## WERR_BADTRANSFERMODE

Specified transfer mode is unsupported or unavailable.

## WERR_STEREOBADCHANNEL

Invalid channel for stereo output (stereo output is only possible on channel 0).

## WERR_STEREONEED2FREECHNL

Could not allocate two consecutive channels for stereo output.

## WERR_UNSUPPORTEDFORMAT

Attempted to open with an unsupported wave format.

(This error code not currently supported).

**Comments**

Use WaveInGetNumDevs to determine the number of input devices present in the system. The device ID specified by wDeviceID varies from 0 to one less than the specified number of devices present.

The application should make sure that the transfer mode specified in the lpFormat variable is supported by the hardware configuration. The wave driver does NOT validate a DMA or interrupt transfer. This can be done by calling the appropriate functions in the control chip driver.

## WaveInReset

**Syntax**

```c
Word WaveInReset(hWaveIn);
```

Stops input on a given waveform device and resets the current position to 0. All pending buffers are marked as done.

**Parameters**

HWaveIn hWaveIn

Specifies a handle to the input device that is to be reset.

**Return value**

Returns zero if the function is successful. Otherwise, it returns an error code. Possible error codes are:

WERR_INVALIDHANDLE

Specified device handle is invalid.

## WaveInStart

**Syntax**

```c
Word WaveInStart(hWaveIn);
```

Starts input on a given waveform input device.

**Parameters**

## HWaveIn hWaveIn

Specifies a handle to the input device to be started.

**Return value**

Returns zero if the function is successful. Otherwise, it returns an error code. Possible error codes are:

## WERR_INVALIDHANDLE

Specified device handle is invalid.

**Comments**

Buffers are returned to the client when full or when WaveInReset is called (the dwBytesRecorded field in the header will contain the actual length of the data). If there are no buffers available, the data is thrown away without notification to the client and input will continue.

Calling this function when input is already started will have no effect and 0 will be returned.

## WaveOutBreakLoop

**Syntax**

```c
Word WaveOutReset(hWaveOut);
```

Breaks a loop on a given waveform device and allows playback to continue with the next block in the driver list.

**Parameters**

## HWaveOut hWaveOut

Specifies a handle to the waveform output device to receive the command.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

## WERR_INVALIDHANDLE

Specified device handle is invalid

**Comments**

Waveform looping is controlled by the dwLoops and dwFlags fields in the WaveHdr structures passed to the device with WaveOutWrite. Use the WHDR_BEGINLOOP and WHDR_ENDLOOP flags in the WaveHdr structure to specify the beginning and ending data blocks for looping. To loop on a single block, specify both flags for the same block. Use the dwLoops field in the WaveHdr structure for the first block in the loop to specify the number of loops.

Calling this function when nothing is playing or looping will have no effect and 0 will be returned.

## WaveOutClose

**Syntax**

```c
Word WaveOutClose(hWaveOut);
```

This function closes the specified waveform output device.

**Parameters**

## HWaveOut hWaveOut

Specifies a handle to the waveform output device to be closed. If the function is successful, the handle is no longer valid after the call.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

## WERR_INVALIDHANDLE

Specified device handle is invalid.

## WERR_STILLPLAYING

There are still buffers in the device queue.

**Comments**

If the device is still playing a waveform, the close operation will fail. Use WaveOutReset to terminate playback before calling WaveOutClose.

## WaveOutGetNumDevs

**Syntax**

```c
Word WaveOutGetNumDevs();
```

Retrieves the number of waveform output devices present in the system.

Parameters

None

Returns value

Returns the number of waveform output devices in the system.

## WaveOutGetVolume

**Syntax**

```c
Word WaveOutGetVolume(hWaveOut, lpdwVolume);
```

This function queries the current volume setting of a waveform output device.

**Parameters**

HWaveOut hWaveOut

Identifies the wave output device.

## LPDWord lpdwVolume

Specifies a far pointer to a location that will be filled with the current volume setting.

The high-order word contains the left channel volume and the low-order word contains the right channel volume.

If a device does not support volume control on both left and right channels (if the device is opened in mono), only the right channel value is used.

A value of 0xFFFF specifies full volume and a value of 0x0000 is silence.

**Return Value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

WERR_INVALIDHANDLE

Specified device handle is invalid.

**Comments**

Volume control is supported on the left and right channels only if the device was opened specifying 2 in the nChannel field of the IpWaveFormat structure of WaveInOpen.

## WaveOutOpen

**Syntax**

```c
Word WaveOutOpen (lphWaveOut, wDeviceId, lpFormat, dwCallBack, dwCallBackData, dwFlags);
```

Opens a specified waveform output device for playback.

**Parameters**

HWaveOut far *lphWaveOut

Specifies a pointer to an HWAVEOUT handle. This location is filled with a handle identifying the opened waveform output device. Use the handle to identify the device when calling other wave output functions. This parameter may be NULL if WAVE_FORMAT_QUERY is specified in dwFlags.

Word wDeviceID

Identifies the waveform output device that is to be opened.

## LpWaveFormat lpFormat

Specifies a pointer to a WaveFormat structure that identifies the format of the waveform that will be sent to the output device. The WaveFormat structure is also used to specify the "mode" by which the data will be sent to the hardware (WAVE_TRANF_POLLING, WAVE_TRANSF_INTERRUPT, WAVE_TRANSF_DMA).

```c
int (far $ ^{*} $ dwCallBack) (HWaveOut dev, LpWaveHdr block, DWord dwCallBackData);
```

Specifies the address of a callback function. The callback function is called by the driver during playback to process messages related to the progress of the playback.

Specify NULL for this parameter if no callback is desired.

DWord dwCallbackData

Specifies 32 bits of user defined data that is passed to the callback.

## DWORD dwFlags

Specifies flags for opening the device.

## WAVE_FORMAT_QUERY

If this flag is specified, the device driver will determine if it supports the given format, but will not actually open the device.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

## WERR_ALLOCATED

Specified resource is already allocated.

## WERR_BADDEVICEID

Specified device is out of range.

## WERR_BADTRANSFERMODE

Specified transfer mode is unsupported or unavailable.

## WERR_STEREOBADCHANNEL

Invalid channel for stereo output (stereo output is only possible on channel 0).

## WERR_STEREONEED2FREECHNL

Could not allocate two consecutive channels for stereo output.

## WERR_UNSUPPORTEDFORMAT

Attempted to open with an unsupported wave format.

(This error code not currently supported).

**Comments**

Use WaveOutGetNumDevs to determine the number of output devices present in the system. The device ID specified by wDeviceID varies from 0 to one less than the specified number of devices present.

The application should make sure that the transfer mode specified in the lpFormat structure is supported by the hardware configuration. The wave driver does NOT validate a DMA or interrupt transfer. This can be made by calling the appropriate functions in the control chip driver. The wave driver uses information stored in the control chip to determine which interrupt and which DMA line it will use.

## WaveOutPause

**Syntax**

```c
Word WaveOutPause(hWaveOut);
```

Pauses playback on a specified waveform output device. The current playback position is saved. Use WaveOutRestart to resume playback from the current playback position.

**Parameters**

## HWaveOut hWaveOut

Specifies a handle to the waveform output device to be paused.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

## WERR_INVALIDHANDLE

Specified device handle is invalid.

**Comments**

Calling this function when output is already paused will have no effect and 0 will be returned.

## WaveOutReset

**Syntax**

```c
Word WaveOutReset(hWaveOut);
```

Stops playback on a given waveform output device and resets the current position to 0. All pending playback buffers are marked as done.

**Parameters**

## HWaveOut hWaveOut

Specifies a handle to the waveform output device that is to be reset.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

## WERR_INVALIDHANDLE

Specified device handle is invalid.

## WaveOutRestart

**Syntax**

```c
Word WaveOutRestart(hWaveOut);
```

This function restarts a paused waveform output device.

**Parameters**

## HWaveOut hWaveOut

Specifies a handle to the waveform output device that is to be restarted.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

WERR_INVALIDHANDLE

Specified device handle is invalid.

**Comments**

Calling this function when the output is not paused will have no effect and 0 will be returned.

## WaveOutSetLeftRight

Syntax

```c
Word WaveOutSetLeftRight(hWaveOut, leftRight);
```

Selects which sides the output will be directed to.

**Parameters**

HWaveOut hWaveOut

Specifies a handle to the waveform output device that is to be restarted.

## Word

leftRight

gs specifying the output direction:

WAVE_STEREO_LEFT

WAVE_STEREO_CENTER

WAVE_STEREO_RIGHT

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

WERR_INVALIDHANDLE

Specified device handle is invalid

**Comments**

This function is useful only when the channel is monophonic. Stereophonic channels are always output left and right.

## WaveOutSetVolume

**Syntax**

```c
Word WaveOutSetVolume(hWaveOut, dwVolume);
```

Sets the volume of a waveform output device.

**Parameters**

HWaveOut hWaveOut

Identifies the wave output device.

## Dword dwVolume

Specifies the volume setting.

The high-order word contains the left channel volume and the low-order word contains the right channel volume.

If a device does not support volume control on both left and right channels (if the device is opened in mono), only the right channel value is used.

A value of 0xFFFF specifies full volume and a value of 0x0000 is silence.

**Return value**

Returns zero if the function was successful. Otherwise, it returns an error code. Possible error codes are:

## WERR_INVALIDHANDLE

Specified device handle is invalid.

**Comments**

Volume control is supported on the left and right channels only if the device was opened specifying 2 in the nChannel field of the IpWaveFormat structure specified in WaveOutOpen.

Note that this controls output volume only.

## WaveOutWrite

**Syntax**

```c
Word WaveOutWrite(hWaveOut, lpWaveOutHdr, wSize);
```

Sends a data block to the specified waveform output device.

**Parameters**

## HWaveOut hWaveOut

Specifies a handle to the waveform device that the data is to be sent to.

## LpWaveHdr lpWaveOutHdr

Specifies a far pointer to a WaveHdr structure containing information about the data block.

## Word wSize

Specifies the size of the WaveHdr structure.

**Return value**

Returns 0 if the function was successful. Otherwise, it returns an error code. Possible error codes are:

## WERR_INVALIDHANDLE

Specified device handle is invalid.

**Comments**

Unless playback is paused by WaveoutPause, playback begins when the first data block is sent to the device.

When writing to a device opened using the WAVE_TRANSF_POLLING mode, control will be returned to the application only when the buffer has been completely played. Using this transfer mode, wave output must be paused with WaveOutPause prior to calling WaveOutWrite if the application must write more than one buffer.

The Ad Lib Gold card offers to developers 5 multi-purpose timers. They are physically located on two different chips but their implementation are similar.

All timers have their own base clock (time resolution) and counter size (maximum period). The controls available for all timers are:

- Stop and start (decrementing the initial stored count until it reach zero and re-writing the original count, again and again).

- Write access in their register of different count values (divider).

- Enable/disable interrupts to occur on zero count crossing.

- Read the interrupt status (access on the zero count crossing).

- Some differences exist and need to be noticed:

- The timer 2 from the MMA chip is the only timer whose current count can be read.

- Yamaha in its own documentation use the terms timer 1 and 2 for the timers physically located in the OPL3 chip and timers located in the MMA chip.

- A base counter (another timer) is used in the MMA chip as an input clock for the timers 1 and 2. Those last two timers are decremented each time the base counter reaches zero. This means that the software must initialized the base counter with an appropriate value then the timer 1 or 2.

Here is a table that illustrates the specifications of all timers:

<table border="1"><tr><td rowspan="2"></td><td colspan="2">OPL3 chip</td><td colspan="4">MMA chip</td></tr><tr><td>Tim.1</td><td>Tim.2</td><td>Tim.0</td><td>B.C.</td><td>Tim.1</td><td>Tim.2</td></tr><tr><td>time resolution in Î¼sec</td><td>80</td><td>320</td><td>1.89</td><td>1.89</td><td>1.89</td><td>1.89</td></tr><tr><td>max period length in msec</td><td>20.4</td><td>81.6</td><td>123.83</td><td>7.738</td><td>116.07</td><td>507116</td></tr><tr><td>counter size in bits</td><td>8</td><td>8</td><td>16</td><td>12</td><td>4+12</td><td>16+12</td></tr></table>

Table 1: Hardware specifications of timers

Remember that the MMA timer 1 and 2 are combined with the MMA base counter and that their combined specifications gives for the timer 1 a size of 16 bits and for the timer 2 a size of 28 bits.

The timer's function can be access directly or by the TimerDrvService functions which is a dispatcher.

Each timer function is presented in the following pages.

## LoadStartOPL3Timer1

LoadStartOPL3Timer2

LoadStartMMATimer0

LoadStartMMATimer1

LoadStartMMATimer2

**Syntax**

```c
WORD LoadStartOPL3Timer1(void);
```

```c
WORD LoadStartOPL3Timer2(void);
```

```c
WORD LoadStartMMATimer0(void);
```

```c
WORD LoadStartMMATimer1(void);
```

```c
WORD LoadStartMMATimer2(void);
```

This will load the physical counter with the count associated and start the counter.

Parameters

None

Return value

TIMER_NO_ERROR If the function was successful.

TIMER_FUNCTION_ERROR If a problem occured when loading.

Comments None

## StopOPL3Timer1

StopOPL3Timer2

StopMMATimer0

StopMMATimer1

StopMMATimer2

**Syntax**

```c
WORD StopOPL3Timer1(void);
```

```c
WORD StopOPL3Timer2(void);
```

```c
WORD StopMMATimer0(void);
```

```c
WORD StopMMATimer1(void);
```

```c
WORD StopMMATimer2(void);
```

Stop the associated timer.

Parameters None Return value

TIMER_NO_ERROR If the function was successful.

TIMER_FUNCTION_ERROR If a problem occured when stoping.

Comments None

## SetOPL3Timer1Counter

SetOPL3Timer2Counter

SetMMATimer0Counter

SetMMATimer1Counter

SetMMATimer2Counter

MMABaseCounterCounter

**Syntax**

```c
WORD SetOPL3Timer1Counter(BYTE count);
```

```c
WORD SetOPL3Timer2Counter(BYTE count);
```

```c
WORD SetMMATimer0Counter(WORD count);
```

```c
WORD SetMMATimer1Counter(BYTE count);
```

```c
WORD SetMMATimer2Counter(WORD count);
```

```c
WORD SetMMABaseCounterCounter(WORD count);
```

Set the OPL3 and MMA timer with the count value. Base clock periods are the following:

OPL3Timer1: 79.9682 us

OPL3Timer2: 319.873 us

MMATimer0: 1.89 us

MMATimer1: 1.89 us

MMATimer2: 1.89 us

MMATimerBaseCounter: 1.89 us

See table xx for more information the capacity of each timer.

**Parameters**

BYTE count WORD count The parameters count specified the number of cycle the timer is supposed to do. Depending of timer count is BYTE or WORD parameter.

**Return value**

TIMER_NO_ERROR If the function was successful.

TIMER_FUNCTION_ERROR If a problem occured when setting.

Comments It is important to check the table xx because each timer don't use all of the bits in the count parameters.

## SetOPL3Timer1Period

SetOPL3Timer2Period

SetMMATimer0Period

SetMMATimer1Period

SetMMATimer2Period

SetMMABaseCounterPeriod

**Syntax**

<table border="1"><tr><td>WORD</td><td>SetOPL3Timer1Period(DWORD IPeriod)</td></tr><tr><td>WORD</td><td>SetOPL3Timer2Period(DWORD IPeriod)</td></tr><tr><td>WORD</td><td>SetMMATimer0Period(DWORD IPeriod)</td></tr><tr><td>WORD</td><td>SetMMATimer1Period(DWORD IPeriod)</td></tr><tr><td>WORD</td><td>SetMMATimer2Period(DWORD IPeriod)</td></tr><tr><td>WORD</td><td>SetMMABaseCounterPeriod(DWORD IPeriod)</td></tr></table>

This set of functions offer another way to set the count of a timer. The period of a cycle is passed instead of passing the divider. It becomes more easy for the programmer to think in terms of period rather than in terms of a divider to associate with the required period.

**Parameters**

## DWORD IPeriod

Period in usec to be passed to the timer.

**Return value**

## TIMER_NO_ERROR

If the function was sucessful.

## TIMER_FUNCTION_ERROR

If a problem occured when setting.

**Comments**

Check the table xx to be sure to respect the maximum capacity of the timer. The period will be round to the precision of the timer.

## EnableOPL3Timer1

EnableOPL3Timer2

EnableMMATimer0

EnableMMATimer1

EnableMMATimer2

**Syntax**

```c
WORD EnableOPL3Timer1(void);
```

```c
WORD EnableOPL3Timer2(void);
```

```c
WORD EnableMMATimer0(void);
```

```c
WORD EnableMMATimer1(void);
```

```c
WORD EnableMMATimer2(void);
```

This will set the mask bit associated with the timer interrupt.

Parameters

None

Return value

TIMER_NO_ERROR If the function was successful.

TIMER_FUNCTION_ERROR If a problem occured when enabling.

Comments None

## DisableOPL3Timer1

DisableOPL3Timer2

DisableMMATimer0

DisableMMATimer1

DisableMMATimer2

Syntax

```c
WORD DisableOPL3Timer1(void);
```

```c
WORD DisableOPL3Timer2(void);
```

```c
WORD DisableMMATimer0(void);
```

```c
WORD DisableMMATimer1(void);
```

```c
WORD DisableMMATimer2(void);
```

This will reset the mask bit associated with the timer interrupt.

Parameters

None

Return value

TIMER_NO_ERROR If the function was successful.

TIMER_FUNCTION_ERROR If a problem occured when disabling.

Comments None

## GetOPL3TimerIntStatus

GetMMATimerIntStatus

**Syntax**

```c
WORD GetOPL3TimerIntStatus(void);
```

```c
WORD GetMMATimerIntStatus(void);
```

These functions will return the state of timer interrupt of the OPL3 and MMA.

**Parameters**

None

## OPL3

**Return value**

return 0 if no timer has interrupted.

return 2 if timer 1 has interrupted.

return 1 if timer 2 has interrupted.

return 3 if timer 1 and 2 has interrupted.MMA

return 0 if no timer has interrupted.

return 1 if timer 0 has interrupted.

return 2 if timer 1 has interrupted.

return 4 if tmer 2 has interrupted.

or any combination of 1,2 and 4 if multiple timer has interrupted.

**Comments**

The MMA chip has a special behavior: it will reset the interrupt bit after a status register reading. Note that this routine is automatically called by the main interrupt handler from the Control Chip Driver. Using GetOPL3TimerIntStatus will not reset the OPL3 status register bits.

## AssignOPL3Timer1IntService

AssignOPL3Timer2IntService

AssignMMATimer0IntService

AssignMMATimer1IntService

AssignMMATimer2IntService

**Syntax**

<table border="1"><tr><td>WORD</td><td>AssignOPL3Timer1IntService(void (*function)(void))</td></tr><tr><td>WORD</td><td>AssignOPL3Timer2IntService(void (*function)(void))</td></tr><tr><td>WORD</td><td>AssignMMATimer0IntService(void (*function)(void))</td></tr><tr><td>WORD</td><td>AssignMMATimer1IntService(void (*function)(void))</td></tr><tr><td>WORD</td><td>AssignMMATimer2IntService(void (*function)(void))</td></tr></table>

Use by applications to assign their callback function on a specific interrupt.

**Parameters**

```c
void (*function)(void);
```

The parameter is the callback prototype.

**Return value**

TIMER_NO_ERROR

If the function was sucessful.

TIMER_FUNCTION_ERROR

If a problem occured with the assign procedure.

**Comments**

The application user must specifie a callback routine that will automatically be called when the interrupt occurs. This callback function must be very short to execute because this is a timer interrupt that may occurs at a very high rate. At initialisation the default service hooked on each timer interrupt is a local DoNothing function that must be replaced by the application user.

## RestoreOPL3Timer1IntService

RestoreOPL3Timer2IntService

RestoreMMATimer0IntService

RestoreMMATimer1IntService

RestoreMMATimer2IntService

**Syntax**

```c
WORD RestoreOPL3Timer1IntService(void);
```

```c
WORD RestoreOPL3Timer2IntService(void);
```

```c
WORD RestoreMMATimer0IntService(void);
```

```c
WORD RestoreMMATimer1IntService(void);
```

```c
WORD RestoreMMATimer2IntService(void);
```

Use by applications to remove their callback function from the interrupt process.

Parameters

None

Return value

TIMER_NO_ERROR If the function was sucessful.

TIMER_FUNCTION_ERROR If a problem occured with the restore procedure.

Comments

None

## ExecOPL3Timer1IntService

ExecOPL3Timer2IntService

ExecMMATimer0IntService

ExecMMATimer1IntService

ExecMMATimer2IntService

Syntax

```c
void ExecOPL3Timer1IntService(void);
```

```c
void ExecOPL3Timer2IntService(void);
```

```c
void ExecMMATimer0IntService(void);
```

```c
void ExecMMATimer1IntService(void);
```

```c
void ExecMMATimer2IntService(void);
```

Those routines will execute the function associated with each interrupt.

Parameters

None

Return value None

Comments None

## ResetOPL3LastTimerInt

**Syntax**

```c
WORD ResetOPL3LastTimerInt(void);
```

This will reset the IRQ signal generated by timers 1 and 2.

**Parameters**

**Return value**

TIMER_NO_ERROR If the function was sucessful.

## TIMER_FUNCTION_ERROR

If a problem occured with the reset procedure.

**Comments**

This function does not exist for the MMA because the MMA clear the status after each reading of the status register.

## AllocateOPL3Timer1

AllocateOPL3Timer2

AllocateMMATimer0

AllocateMMATimer1

AllocateMMATimer2

AllocateMMABaseCounter

**Syntax**

```c
WORD AllocateOPL3Timer1(void);
```

```c
WORD AllocateOPL3Timer2(void);
```

```c
WORD AllocateMMATimer0(void);
```

```c
WORD AllocateMMATimer1(void);
```

```c
WORD AllocateMMATimer2(void);
```

```c
WORD AllocateMMABaseCounter(void);
```

This procedure will reserve and from then denied any external application access to this timer.

**Parameters**

None

Return value

1: if available

0: if not available

**Comments**

Any application who wants to use the service of any timers should ask the Timer Driver for its disponibility using an allocation routine. The application should free the timer after use.

## FreeOPL3Timer1

FreeOPL3Timer2 FreeMMATimer0 FreeMMATimer1 FreeMMATimer2 MMABaseCounter

**Syntax**

<table border="1"><tr><td>WORD</td><td>FreeOPL3Timer1(void)</td></tr><tr><td>WORD</td><td>FreeOPL3Timer2(void)</td></tr><tr><td>WORD</td><td>FreeMMATimer0(void)</td></tr><tr><td>WORD</td><td>FreeMMATimer1(void)</td></tr><tr><td>WORD</td><td>FreeMMATimer2(void)</td></tr><tr><td>WORD</td><td>FreeMMABaseCounter(void)</td></tr></table>

Free the the timer.

Parameters

None

Return value

1: if operation succed

0: if operation not succed

Comments

None

## GetMMATimer2Content

**Syntax**

```c
WORD GetMMATimer2Content(void);
```

This routine returns the content of the MMA timer 2.

Parameters

None

Return value

16 bit content of MMA timer 2

Comments

This is the only timer that can be read. These timers respect the specification of Windows Multi-Media.

## GetOPL3Timer1Caps

GetOPL3Timer2Caps

GetMMATimer0Caps

GetMMATimer1Caps

GetMMATimer2Caps

**Syntax**

```c
WORD GetOPL3Timer1Caps (DWORD far *lPeriodMin, DWORD far *lPeriodMax);
```

```c
WORD GetOPL3Timer2Caps (DWORD far *lPeriodMin, DWORD far *lPeriodMax);
```

```c
WORD GetMMATimer0Caps (DWORD far *lPeriodMin, DWORD far *lPeriodMax);
```

```c
WORD GetMMATimer1Caps (DWORD far *lPeriodMin, DWORD far *lPeriodMax);
```

```c
WORD GetMMATimer2Caps (DWORD far *lPeriodMin, DWORD far *lPeriodMax);
```

Used by external modules to query the driver on physical limits of each timer. It returns the minimum and maximum period covered by the timer in micro seconds.

**Parameters**

DWORD far *IPeriodMin

DWORD far *IPeriodMax

These two address will receive the minimum and the maximum period capacity respectively of the timer.

**Return value**

TIMER_NO_ERROR If the function was sucessful.

TIMER_FUNCTION_ERROR If a problem occured with the procedure.

**Comments**

None

## InitTimerDriver

**Syntax**

```c
WORD InitTimerDriver(WORD base);
```

This procedure initialize the Timer Driver structure with default values. This procedure should be used the first time the driver is called.

**Parameters**

WORD base

Actual address of the Ad Lib control chip.

**Return value**

TIMER_NO_ERROR If the function was successful.

TIMER_FUNCTION_ERROR If a problem occured with the procedure.

Comments

None

## TimerDrvService

**Syntax**

```c
WORD far TimerDrvService(WORD segm, WORD offs) Entry point for the AdLib timer dispatcher. The segment and offset of the argument structure are passed as argument.;
```

Parameters

WORD segm

WORD offs

These two parameters specify the segment and the offset of the following structure which is used to pass parameters to the TimerDrvService routine.

struct TimerArgum {

WORD     controlID;             which service to be used

WORD     timerDv;               on which timer

DWORD    param;                 optionnal based on service used

DWORD    param2;                 optionnal based on service used

```c
void     (interrupt far *function)();    optionnal based on service used;
```

}

Return value Service result if any.

Comments See TimerDrv.h for all ID of services.
