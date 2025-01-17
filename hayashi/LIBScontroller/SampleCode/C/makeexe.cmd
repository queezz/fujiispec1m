@ECHO OFF
cd %~dp0
SET ORGPATH=%PATH%

REM ****************************************
REM **** path to msys2 commands
REM ****************************************
SET MSYSROOT="%ProgramFiles%\BestTech\GCC Developer Lite\msys2"
IF NOT EXIST %MSYSROOT% SET MSYSROOT="%ProgramFiles(x86)%\BestTech\GCC Developer Lite\msys2"
IF NOT EXIST %MSYSROOT% GOTO FAIL

REM ****************************************
REM **** path to GCC
REM ****************************************
SET GCCROOT="%ProgramData%\BestTech\GCC Developer Lite\GCC\x64"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramData%\BestTech\GCC Developer Lite\GCC\x86"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramFiles%\BestTech\GCC Developer Lite\GCC\x64"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramFiles%\BestTech\GCC Developer Lite\GCC\x86"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramFiles(x86)%\BestTech\GCC Developer Lite\GCC\x64"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramFiles(x86)%\BestTech\GCC Developer Lite\GCC\x86"
IF NOT EXIST %GCCROOT% GOTO FAIL

REM ****************************************
REM **** Create semantic link
REM ****************************************
mklink /j ".\GCC" %GCCROOT%
mklink /j ".\MSYS" %MSYSROOT%

echo %GCCROOT% | find "x64" >NUL
if not ERRORLEVEL 1 SET ENV_WINX64=1

SET PATH=.\MSYS;.\GCC\bin;%ORGPATH%

make %~n1

IF ERRORLEVEL 1 GOTO FAIL
GOTO SUCCESS
:FAIL
ECHO ERROR !!!
ECHO Press the Enter key to exit.
PAUSE > NUL
GOTO END
:SUCCESS
ECHO SUCCESS !!!
timeout 2 > nul
:END
SET PATH=%ORGPATH%
rmdir .\GCC
rmdir .\MSYS
