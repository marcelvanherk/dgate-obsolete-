REM compiles jasperlib; jpeg through jasper disabled for now

set spath=%path%
call ..\asetcompiler64.bat
set path=%compiler%;%path%
set include=%include%;..\jpeg-6c;src\libjasper\include;src\libjasper
cl -DJAS_WIN_MSVC_BUILD /Zi /MT /O2 -c jasperlib.c -Fo..\ms8amd64\jasperlib.obj

set path=%spath%
call ..\asetcompiler32.bat
set path=%compiler%;%path%
set include=%include%;..\jpeg-6c;src\libjasper\include;src\libjasper
cl -DJAS_WIN_MSVC_BUILD /Zi /MT /O2 -c jasperlib.c -Fo..\ms8\jasperlib.obj

set path=%spath%
