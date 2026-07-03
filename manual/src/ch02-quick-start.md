# Chapter 2 - Quick Start and Evaluation Software

*Installing the Gold card and software, and using the evaluation applications.*

> **Related source in this repository:**  
> [`adlibgold.inf`](https://github.com/slartibardfast/adlib_gold/blob/main/adlibgold.inf) - Windows setup information for the Gold card  

---

- Installing the Hardware

Installing the Gold Card 1

Connect the Other Peripherals 1

- Installing the Software

Read the README.TXT File 1

Install Gold Applications and Resources 2

Test Hardware 2

- Using the Gold Card Evaluation Software

Running Gold DOS Applications 2

- Adjusting the Volume

## Quick Start and Evaluation Software

The Quick Start is intended for developers who want to have quick access to some of the evaluation programs provided with the Gold card.

## Installing the Hardware

## Installing the Gold card

1. Make sure that the on-board jumpers, the "Game port enable jumper", the "Port address jumper" and the "Dual joystick selector jumpers", are in the desired position.

2. Plug the Gold card into the computer in a free slot as far as possible from the video adapter card.

* NOTE: Certain cards, such as video adapters, produce high-frequency signals which can interfere with the sound quality of the sound card.

See "3.2: Getting Installed".

## Connect the Other Peripherals

- Plug headphones or external speakers into the main audio output of the card, or connect the output to the input of a stereo system.

- Connect your microphone to the microphone input of the card.

- Connect the output of your stereo source (CD player, CD-ROM drive, synthesizer or cassette player) to the stereo auxiliary input of the card, using a stereo cable.

- Connect your joystick to the DB-15 game port of the card. If you plan to use the MIDI interface, connect your MIDI device with the Ad Lib adapter cable.

See "3.2: Getting Installed".

## Installing the Software

## Read the README.TXT File

We suggest that you examine the README.TXT file prior to installing the software. This file contains information on the latest program updates, and other necessary information.

- Insert Ad Lib diskette No.1 into the floppy drive, set the current drive to A (or B, depending on which drive you are using), and type the following command:

A:>type readme.txt

## Install Gold Applications and Resources

- Run the Gold Setup Program by typing the following commands:

A: \>ctrldrv

A:\>setup

See "4.1: Software Installation and Configuration".

## Test Hardware

Once installation is complete, run the Test program to verify that the Ad Lib Gold card is functioning properly.

- Go to the directory where you placed the Gold Test Program at installation and load this program by typing the following command:

testgold

See "4.2: Test Program".

## Using the Gold Card Evaluation Software

## Running Gold DOS Applications

Once the Gold hardware and software are installed, you can run any Ad Lib Gold application by proceeding as follows:

1. Set the current directory to the one where you placed the Gold programs during the installation process.

2. Load the Mixer Panel TSR program first, which serves to control the different sound parameters (balance, tone, volume, etc.), by typing the following command:

mixer

3. Load the program you want by typing the corresponding command:

jukegold Juke Box Gold Music Playback Program

insgold Instrument Maker Gold Program

sample Sample Maker Program

surround Juke Box Gold Music Playback Program including the Surround Sound Editor

Note that the Juke Box Gold Music Playback Program offers on-line Help containing summarized information on how to operate the program and how to use the various features.

See "Chapter 4 - Software Applications".

## Adjusting the Volume

When running a program with the Gold card, you can adjust output volume at any time, without opening the Mixer Panel, using the following shortcuts:

Alt U For increasing output volume.

Alt D For decreasing output volume.

See "4.3: Mixer Panel TSR".
