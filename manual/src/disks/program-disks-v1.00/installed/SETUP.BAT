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