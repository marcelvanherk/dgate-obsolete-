REM 20080113 mvh adapted for MS7
REM 20100120 mvh adapted for MS84
set spath=%path%
set watcom=c:\lang\ms8
set path=%watcom%\bin;%path%
set lib=%watcom%\lib
set include=%watcom%\include;\quirt\comps\dll\cqdicom
del cong.obj
c:\lang\ms8\bin\nmake
set path=%spath%
set spath=
