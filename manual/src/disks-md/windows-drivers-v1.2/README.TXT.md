# `windows-drivers-v1.2/README.TXT`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`README.TXT`](../../disks/windows-drivers-v1.2/README.TXT).

```text
* Ad Lib Gold Mixer and Windows Drivers, Version 1.2           (July 1993)
*****************************************************
    
    This document will instruct you on how to install the Ad Lib Gold
    Mixer and drivers for Windows 3.1 (found on the diskette entitled
    "Windows 3.1 Mixer and Drivers").


* Contents of the diskette 
***************************
    
    There are 2 separate  Ad Lib Gold Windows Drivers. Each of the
    driver covers separate aspects of the Ad Lib Gold multimedia
    capabilities.

    The first driver, called the Yamaha GSS Midi Synth, is used by
    Windows to play back MIDI data using the internal synthesizer of
    the Ad Lib Gold Card (which uses an FM synthesizer). Once loaded,
    this driver will install a special configuration applet in the
    Control Panel. 

    The second driver, called the Yamaha GSS Wave/Midi/Aux driver,
    covers all other multimedia capabilities of the Gold Card.  The
    Wave section enables digitized sound playback under Windows for all
    applications that use wave files (such as the Sound Recorder,
    StudioSonic 8 or the System Sounds).  The MIDI section is used to
    play back and record MIDI data through the Ad Lib Gold MIDI Port. 
    Finally, the AUX section of the driver provides the applications
    with a mechanism to dynamically control the various mixer controls
    (volume, bass, treble) of the Gold Card.


* Ad Lib Gold Mixer for Windows
*******************************

    You can use this application to control the Ad Lib Gold card's
    programmable mixer.


* Configuration file for Windows MIDI Mapper
********************************************

    This configuration file can be used to replace your current MIDI
    mapper configuration with setups that make optimal use of the Gold
    Card's capabilities.


* Installing the Software
*************************

* Configuring the Gold Card

    In order for the drivers to function properly, an interrupt and at
    least one DMA channel must have been allocated to the Gold card. 
    To allocate an interrupt and DMA channel, use the Gold Setup
    program, supplied on the Gold program disks, under DOS.

    You should take note of the interrupt line (IRQ) and DMA channel
    that you've selected.  This information will be needed when
    installing the Yamaha GSS Wave/Midi/Aux driver.

* Installing the drivers

    To install the drivers, you will need to activate the "Drivers"
    applet found in the "Control Panel" (you'll find this last one in
    the "Main" program group).

    This is where you will add the new Ad Lib Gold Windows Drivers:

    Click on the "Add..." button.  You will be shown a list of the
    available drivers that can be added to Windows.  From this list,
    click on "Unlisted or Updated Driver".  A new dialog box will
    appear asking  you to specify the location of the new driver.  Once
    you have answered this question you will be given a choice of the
    two drivers to install: the Yamaha GSS Midi Synth and the Yamaha
    GSS Wave/Midi/Aux. You should install both of these drivers in
    sequence, first installing the Yamaha GSS Midi Synth, and then the
    Yamaha GSS Wave/Midi/Aux.

    When you will select the Yamaha GSS Midi Synth driver,  this driver
    will be installed in Windows.  This driver doesn't require any more
    installation than that.  Windows will prompt you  with "Don't
    Restart Now" or "Restart Now."  If you haven't yet installed the
    Yamaha GSS Wave/Midi/Aux driver you should choose to postpone the
    restarting of Windows until  you've installed both drivers. 
    Otherwise select "Restart Now."

* Configuring the Yamaha GSS Wave/Midi/Aux driver according to your
  current Gold Card configuration

    The specified address should be "38C."  Even if the Gold card's
    address is "388" the correct address setting for the driver under
    Windows is "38C" (address should always be Gold card base address +
    4 in hexadecimal).

    The DMA Buffer size (Memory allocated to the driver) should be set
    to the maximum available value. 

    The interrupt line and DMA channel should correspond to those
    you've allocated to the Gold Card using the Gold Setup program.

    Windows will prompt you  with "Don't Restart Now" or "Restart Now." 
    If you haven't yet installed the Yamaha GSS Midi Synth driver you
    may as well choose to postpone the restarting of Windows until
    you've installed both drivers.  Otherwise select "Restart Now."

* Restarting Windows

    In order for the changes to take effect, Windows must be restarted.
    
* Reconfigure the MIDI Mapper

    The MIDI Mapper applet from the control panel contains standard
    configurations that maximize the use of your existing hardware. 

    A complete set of configurations for the Ad Lib Gold Card has been
    supplied in the MIDIMAP.CFG file.  This file can be used to replace
    your existing MIDIMAP.CFG file in the "..\WINDOWS\SYSTEM"
    directory.  If you wish to use this configuration, just copy the
    supplied MIDIMAP.CFG file in this directory.

    When copied, you will have two options to choose from (when in the
    MIDI Mapper) to set your Gold card up for MIDI: GOLD MIDI and GOLD
    SYNTH.  GOLD MIDI uses the Yamaha GSS Midi Out (part of the Yamaha
    GSS Wave/Midi/Aux) driver on all 16 of the MIDI channels whereas
    GOLD SYNTH makes use of the Yamaha GSS Midi Synth driver.  This means
    that you would select GOLD MIDI whenever you have a MIDI device hooked
    up to your Gold card and would like to play your MIDI information
    through it.  Any other time you would select GOLD SYNTH.  By default
    the GOLD SYNTH is selected.

* Installing the Ad Lib Gold Mixer for Windows

    Copy the Mixer program (MIXERGLD.EXE) to the "...\WINDOWS"
    directory.

    Now, in Windows, create a new program item in the Program Manager's
    "Accessories" program group:

    Open the "Accessories" program group.  Now select the "New..."
    option in the "File" menu heading.  "Program Item" should be
    highlighted so that you simply have  to click on "OK."  

    You will be prompted to fill in 4 fields: Description:, Command
    Line:, Working Directory:, and Short Cut Key:.  The suggested
    "Description:" is "Gold Mixer."  For the "Command Line:" you should
    type in "MIXERGLD.EXE" (specifying its path is optional).  The last
    two fields don't require any entry so you can simply click on "OK"
    and a new icon will appear in your "Accessories" program group.

    This completes the installation of the Ad Lib Gold Windows drivers
    and mixer.

------------------------------------------------------------------

* Notes:
********

*   The Gold Card must use at least 1 DMA channel.

*   In order for the drivers to function correctly, at least one DMA
    channel must have been allocated to the Gold Card, and that DMA
    channel must have been specified in the drivers configuration.

    To access the driver configuration, use the "Setup..." button in
    the Control Panel "Drivers" applet.

    To allocate a DMA channel to the Gold Card, use the Setup program,
    supplied in the program disks, under DOS. 

*   The Drivers Configuration must reflect the Hardware Configuration

    If the digitized sound playback does not occur or "loops", it may
    indicate that the driver configuration do not reflect the actual
    hardware configuration, in terms of DMA channel allocation or
    interrupt line (IRQ) selection.  It could also indicate that your
    choice of interrupt conflicts with some other peripheral in the
    system.

    To access the driver configuration, use the "Setup..." button in
    the Control Panel "Drivers" applet.

    To allocate a DMA channel to the Gold Card or alter interrupt line
    selection, use the Setup program, supplied in the program disks,
    under DOS.

*   Running DOS applications under Windows

    If a DOS application uses the the ressources of the Ad Lib card,
    the Windows drivers are allocated exclusively to that DOS
    application for the duration of the DOS application. Therefore, you
    will not be able to share those ressources between simultaneously
    opened Windows and DOS applications.


```
