REM 20100116: mvh modified to use bcb's libjpeg
REM 20100119: mvh generate 32 and 64 bit library

set spath=%path%
call ..\asetcompiler64.bat
set path=%compiler%;%path%

nmake -fmakefile.vc clean
nmake -fmakefile.vc conlibs=bufferoverflowu.lib

copy libjpeg.lib ..\ms8amd64

set path=%spath%
call ..\asetcompiler32.bat
set path=%compiler%;%path%

nmake -fmakefile.vc clean
nmake -fmakefile.vc
copy libjpeg.lib ..\ms8

nmake -fmakefile.vc clean
set path=%spath%
