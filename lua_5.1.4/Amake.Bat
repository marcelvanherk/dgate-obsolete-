set spath=%path%
call ..\asetcompiler64.bat
set path=%compiler%;%path%
set include=%include%;..\..\cqdicom
cl /Zi /MT /nologo /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE all.c
copy all.obj ..\ms8amd64\lua.obj

set path=%spath%
call ..\asetcompiler32.bat
set watcom=c:\lang\ms8
set path=%compiler%;%path%
set include=%include%;..\..\cqdicom
cl /Zi /MT /nologo /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE all.c
copy all.obj ..\ms8\lua.obj
set path=%spath%
