echo off
if exist setupgld.exe goto ok
goto err
:ok
if "%1" == "/R" goto reset
if "%1" == "/r" goto reset
@ctrldrv
@setupgld /install
goto end
:reset
@setupgld /R
goto end
:err
echo Ad Lib Gold Installation Program.
echo *********************************
echo .
echo The drive containing Program Disk 1 should be the current drive.
echo For example, to install the program from drive "A:" type:
echo    A:
echo    SETUP
goto end
:end		  
echo on
