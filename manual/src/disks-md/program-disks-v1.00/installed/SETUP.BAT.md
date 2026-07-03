# `program-disks-v1.00/installed/SETUP.BAT`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`SETUP.BAT`](../../../disks/program-disks-v1.00/installed/SETUP.BAT).

```bat
echo off
:ok
if "%1" == "/R" goto reset
if "%1" == "/r" goto reset
@ctrldrv
@setupgld %1
goto end
:reset
@setupgld /R
goto end
:end		  
echo on

```
