@ECHO OFF
rem Nnecessary ext. tools (lib.exe mspdb100.dll msvcr100.dll)
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
SET GCCROOT="%ProgramData%\BestTech\GCC Developer Lite\GCC"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramFiles%\BestTech\GCC Developer Lite\GCC"
IF NOT EXIST %GCCROOT% SET GCCROOT="%ProgramFiles(x86)%\BestTech\GCC Developer Lite\GCC"
IF NOT EXIST %GCCROOT% GOTO FAIL

REM ****************************************
REM **** Create semantic link
REM ****************************************
mklink /j ".\GCCROOT" %GCCROOT%
mklink /j ".\MSYSROOT" %MSYSROOT%

REM ****************************************
REM **** 32bit
REM ****************************************
SET PATH=.\GCCROOT\x86\bin;.\MSYSROOT;%ORGPATH%
del dx2lib_x32.dll
gcc -shared -static-libgcc -static-libstdc++ -D__MAKE_LIB__ -o dx2lib_x32.dll dx2lib_intuitive.cpp dx2lib.cpp -O3 -Wall -Wno-main -Wno-sign-compare -Wshadow -Wcast-align -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused -Wl,--add-stdcall-alias,--output-def,dx2lib_x32.def,--out-implib,libdx2lib_x32.a
if not exist dx2lib_x32.dll GOTO err
if not exist lib.exe GOTO err
lib /DEF:dx2lib_x32.def /MACHINE:X86 /out:dx2lib_x32.lib
strip dx2lib_x32.dll

REM ****************************************
REM **** 64bit
REM ****************************************
SET PATH=.\GCCROOT\x64\bin;.\MSYSROOT;%ORGPATH%
del dx2lib_x64.dll
gcc -shared -static-libgcc -static-libstdc++ -D__MAKE_LIB__ -o dx2lib_x64.dll dx2lib_intuitive.cpp dx2lib.cpp -O3 -Wall -Wno-main -Wno-sign-compare -Wshadow -Wcast-align -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused -Wl,--add-stdcall-alias,--output-def,dx2lib_x64.def,--out-implib,libdx2lib_x64.a
if not exist dx2lib_x64.dll GOTO err
if not exist lib.exe GOTO err
lib /DEF:dx2lib_x64.def /MACHINE:X64 /out:dx2lib_x64.lib
strip dx2lib_x64.dll
GOTO ok

:err
echo ---------------------------
echo !! create lib error !!
echo ---------------------------
pause > NUL
ECHO Press the Enter key to exit.

GOTO fin

:ok
echo ---------------------------
echo !! create lib success !!
echo ---------------------------
@TIMEOUT 3 > nul
GOTO fin

:fin
SET PATH=%ORGPATH%
rmdir .\GCCROOT
rmdir .\MSYSROOT
