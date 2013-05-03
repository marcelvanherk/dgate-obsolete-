set spath=%path%
call ..\asetcompiler64.bat
set path=%compiler%;%path%
rem set include=%compiler%\include
cl /Zi /MT /nologo /O2 /W2 /c /D_CRT_SECURE_NO_DEPRECATE cong.cxx
lib /out:..\ms8amd64\dicom.lib cong.obj

set path=%spath%
call ..\asetcompiler32.bat
set path=%compiler%;%path%
rem set include=%compiler%\include
cl /Zi /MT /nologo /O2 /W2 /c /D_CRT_SECURE_NO_DEPRECATE cong.cxx
lib /out:..\ms8\dicom.lib cong.obj

set spath=%spath%
