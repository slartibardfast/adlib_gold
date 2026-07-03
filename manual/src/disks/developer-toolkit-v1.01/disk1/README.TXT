Ad Lib Developer Toolkit  Version 1.01                     March 31,1992
************************************************************************

To install the software, type the following commands:

	ctrldrv
	setup

************************************************************************

The following features have been modified and may be different from the
published documentation.

	- The MIXER.EXE and RL2DRV.EXE TSRs have been altered. The hotkeys
          they support can now be redefined from the SETUP program.

	- The MIXER.EXE TSR hotkey redefinition screen has been removed from
          the application.
	
	- The SETUP program now has a new screen to redefine the 
          RL2DRV and MIXER hotkeys.

	- You can use the F4 key in the main screen of the MIXER TSR 
          to permanently save the current configuration. The saved 
	  configuration will be restored when you perform a cold-boot
	  of the computer. The manual states that the F4 key is used to 
          reconfigure the MIXER hotkeys. This information is no longer valid.


************************************************************************

The following tree represents the structure of the Ad Lib DEVKIT directory. 

All stay-resident drivers are located in the DRIVERS directory. The CtrlDrv
must be installed (executed) first before any other drivers.

All object modules to access the resident drivers are located in the LINK
directory in appropriate directories based on the compiler and the 
memory model. Source of these access files are offered.

Utility programs are located in the UTIL directory.

The "include" files for compilation are located in the INCLUDES directory. 

Examples on how to use the different drivers are located in the EXAMPLES
directory. Two makefiles are offered to recompile the examples in any
memory model (by selecting it in the makefile). Link files are kept in
sub-directories: BorLnk and MicLnk.


\DEVKIT 
|   README.TXT          ; This file your reading now
|   SETUP.EXE           ; Installation and Configuration Program
|   MIXER.EXE     	; Mixer Panel
|   SAMPL.*             ; Sample Editor
|   ED.*                ; Instrument Maker 
|   DRIVERS.BAT         ; Main Drivers Loading batch file
|   TEST.BAT            ; Test Program Loading batch file
|   JUKEGOLD.BAT        ; Juke Box Program Loading batch file
|   SURROUND.BAT        ; Surround Editor Loading batch file
|   INSGOLD.BAT         ; Instrument Maker Gold Loading batch file
|                                       
|---DRIVERS             ; The executable drivers
|       CTRLDRV.EXE     ; Control Chip Driver, interrupts, Dispatcher
|       FMDRV.EXE       ; FM Driver (OPL3)
|       MIDIDRV.EXE     ; MIDI Driver (MMA)
|       TIMERDRV.EXE    ; OPL3 & MMA Timer Driver
|       WAVEDRV.EXE     ; Wave Driver (MMA)
|       RL2DRV.EXE      ; ROL2 Driver (to play .rl2 files)
|                    
|---EXAMPLES            ; Examples of application using the drivers
|   |   MAKEFILE.MIC    ; Makefile to compile examples using MICROSOFT
|   |   MAKEFILE.BOR    ; Makefile to compile examples using BORLAND
|   |   FM.C            ; Plays a little melody
|   |   MIDI.C          ; Transmit a short MIDI melody and monitor MIDI in
|   |   TIMER.C         ; Work out for the 5 timers
|   |   WA.C            ; Plays a sample (GLISS1.SMP)
|   |   FM.EXE          ; Executable of FM.C
|   |	MIDI.EXE	; Executable of MIDI.C
|   |	TIMER.EXE	; Executable of TIMER.C
|   |	WA.EXE		; Executable of WA.C
|   |	GLISS1.SMP	; Glissando sample for WA.EXE (type: WA GLISS1.SMP)
|   |                
|   |---BORLNK		; Link files for all Borland memory model
|   |       FMC.LNK     ; Compact FM.EXE 
|   |       FMH.LNK     ; Huge FM.EXE 
|   |       FML.LNK     ; Large FM.EXE 
|   |       FMM.LNK     ; Medium FM.EXE 
|   |       FMS.LNK     ; Small FM.EXE 
|   |       MIDIC.LNK   ; Compact MIDI.EXE 
|   |       MIDIH.LNK   ; Huge MIDI.EXE 
|   |       MIDIL.LNK   ; Large MIDI.EXE 
|   |       MIDIM.LNK   ; Medium MIDI.EXE 
|   |       MIDIS.LNK   ; Small MIDI.EXE 
|   |       TIMERC.LNK  ; Compact TIMER.EXE 
|   |       TIMERH.LNK  ; Huge TIMER.EXE 
|   |       TIMERL.LNK  ; Large TIMER.EXE 
|   |       TIMERM.LNK  ; Medium TIMER.EXE 
|   |       TIMERS.LNK  ; Small TIMER.EXE 
|   |       WAC.LNK     ; Compact WA.EXE 
|   |       WAH.LNK     ; Huge WA.EXE 
|   |       WAL.LNK     ; Large WA.EXE 
|   |       WAM.LNK     ; Medium WA.EXE 
|   |       WAS.LNK     ; Small WA.EXE 
|   |                    
|   |---MICLNK          ; Link file for all Microsoft memory model        
|           FMC.LNK     ; Compact FM.EXE 
|           FML.LNK     ; Large FM.EXE 
|           FMM.LNK     ; Medium FM.EXE
|           FMS.LNK     ; Small FM.EXE  
|           MIDIC.LNK   ; Compact MIDI.EXE 
|           MIDIL.LNK   ; Large MIDI.EXE 
|           MIDIM.LNK   ; Medium MIDI.EXE 
|           MIDIS.LNK   ; Small MIDI.EXE 
|           TIMERC.LNK  ; Compact TIMER.EXE 
|           TIMERL.LNK  ; Large TIMER.EXE 
|           TIMERM.LNK  ; Medium TIMER.EXE
|           TIMERS.LNK  ; Small TIMER.EXE 
|           WAC.LNK     ; Compact WA.EXE 
|           WAL.LNK     ; Large WA.EXE  
|           WAM.LNK     ; Medium WA.EXE 
|           WAS.LNK     ; Small WA.EXE 
|                       
|---INCLUDES            ; Include files to link with the LINK object files
|       CTRLDRV.H       ; With CtrlLnk.OBJ
|       DMA.H           ; With WaveLnk.OBJ
|       FMDRV.H         ; With FmLnk.OBJ
|       GLOBAL.H        ; Utility
|       GLOBALS.H       ; Utility
|       GLOBWIN.H       ; Utility
|       MIDIDRV.H       ; With MIDILnk.OBJ
|       TIMERDRV.H      ; With TimerLnk.OBJ
|       WAVE.H          ; With WaveLnk.OBJ
|       WAVEDRV.H       ; With WaveLnk.OBJ
|                        
|---LINK                ; Source of link files to access drivers
|   |   CTRLLNK.C       ; Access the Control Chip Driver
|   |   FMLNK.C         ; Access the FM Driver
|   |   MIDILNK.C       ; Access the MIDI Driver
|   |   TIMERLNK.C      ; Access the Timer Driver
|   |   WAVELNK.C       ; Access the Wave Driver
|   |   MAKEFILE        ; To create sub-directories
|   |                    
|   |---BORLANDC        ; Link object file Borland (COMPACT)
|   |       CTRLLNK.OBJ 
|   |       DRVASM.OBJ  ; Used by all link modules
|   |       FMLNK.OBJ   
|   |       MIDILNK.OBJ 
|   |       TIMERLNK.OBJ
|   |       WAVELNK.OBJ 
|   |                    
|   |---BORLANDH        ; Link object file Borland (HUGE)
|   |       CTRLLNK.OBJ 
|   |       DRVASM.OBJ  
|   |       FMLNK.OBJ   
|   |       MIDILNK.OBJ 
|   |       TIMERLNK.OBJ
|   |       WAVELNK.OBJ 
|   |                    
|   |---BORLANDL        ; Link object file Borland (LARGE)
|   |       CTRLLNK.OBJ
|   |       DRVASM.OBJ
|   |       FMLNK.OBJ
|   |       MIDILNK.OBJ
|   |       TIMERLNK.OBJ
|   |       WAVELNK.OBJ
|   |                    
|   |---BORLANDM        ; Link object file Borland (MEDIUM)
|   |       CTRLLNK.OBJ
|   |       DRVASM.OBJ
|   |       FMLNK.OBJ
|   |       MIDILNK.OBJ
|   |       TIMERLNK.OBJ
|   |       WAVELNK.OBJ
|   |                    
|   |---BORLANDS        ; Link object file Borland (SMALL)
|   |       CTRLLNK.OBJ
|   |       DRVASM.OBJ
|   |       FMLNK.OBJ
|   |       MIDILNK.OBJ
|   |       TIMERLNK.OBJ
|   |       WAVELNK.OBJ
|   |                    
|   |---MICROC          ; Link object file Microsoft (COMPACT)
|   |       CTRLLNK.OBJ
|   |       DRVASM.OBJ
|   |       FMLNK.OBJ
|   |       MIDILNK.OBJ
|   |       TIMERLNK.OBJ
|   |       WAVELNK.OBJ
|   |                    
|   |---MICROL          ; Link object file Microsoft (LARGE)
|   |       CTRLLNK.OBJ
|   |       DRVASM.OBJ
|   |       FMLNK.OBJ
|   |       MIDILNK.OBJ
|   |       TIMERLNK.OBJ
|   |       WAVELNK.OBJ
|   |                    
|   |---MICROM          ; Link object file Microsoft (MEDIUM)
|   |       CTRLLNK.OBJ
|   |       DRVASM.OBJ
|   |       FMLNK.OBJ
|   |       MIDILNK.OBJ
|   |       TIMERLNK.OBJ
|   |       WAVELNK.OBJ
|   |                    
|   |---MICROS          ; Link object file Microsoft (SMALL)
|           CTRLLNK.OBJ
|           DRVASM.OBJ
|           FMLNK.OBJ
|           MIDILNK.OBJ
|           TIMERLNK.OBJ
|           WAVELNK.OBJ
|---JUKEGOLD            ; Juke Box Directory
|       JUKEG.EXE	; Juke Box Program
|       SURR.EXE 	; Surround Editor
|       *.RL2           ; Music files
|       *.SMP           ; Sample files
|       *.BNK           ; Sound Bank File
|       SAMPLBNK.EQU    ; Sample Name translation table
|                        
|---UTIL                ; Utility programs
        WHO.EXE         ; Reports which driver is currently installed
        TESTGOLD.EXE	; Test Program
        TESTGLD1.SMP	; Sample for the Test Program
	PLAYDIGI.EXE    ; Sample Playback Utility
	PLAYRL2.EXE     ; ROL2 File Playback Utility
