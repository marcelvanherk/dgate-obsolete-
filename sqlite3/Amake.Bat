set spath=%path%
call ..\asetcompiler64.bat
set path=%compiler%;%path%
set include=%include%;..\..\cqdicom
cl /Zi /MT /nologo /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE sqlite3.c
copy sqlite3.obj ..\ms8amd64

set spath=%path%
call ..\asetcompiler32.bat
set path=%compiler%;%path%
set include=%include%;..\..\cqdicom
cl /Zi /MT /nologo /O2 /W3 /c /D_CRT_SECURE_NO_DEPRECATE sqlite3.c
copy sqlite3.obj ..\ms8

