set spath=%path%
call asetcompiler64.bat
set path=%compiler%;%path%
set include=%include%;dicomlib;jpeg-6c;jasper-1.900.1-6ct\src\libjasper\include;sqlite3;C:\mysql-5.0.22-win32\include;lua_5.1.4
cl /Zi /MT /nologo /O2 /W2 /c /D_CRT_SECURE_NO_DEPRECATE -DWIN32 -DNATIVE_ENDIAN=1 -DUSESQLITE -DUSEMYSQL -DPOSTGRES -DHAVE_LIBJPEG -DHAVE_LIBJASPER -DJAS_WIN_MSVC_BUILD total.cpp
copy total.obj ms8amd64\total.obj
link /OUT:ms8amd64\dgate.exe /MAP:ms8amd64\dgate.map /DEBUG ms8amd64\total.obj ms8amd64\sqlite3.obj ms8amd64\lua.obj odbc32.lib odbccp32.lib wsock32.lib advapi32.lib user32.lib kernel32.lib ms8amd64\libjpeg.lib ms8amd64\jasperlib.obj

set path=%spath%
call asetcompiler32.bat
set path=%compiler%;%path%
set include=%include%;dicomlib;jpeg-6c;jasper-1.900.1-6ct\src\libjasper\include;sqlite3;C:\mysql-5.0.22-win32\include;lua_5.1.4
cl /Zi /MT /nologo /O2 /W2 /c /D_CRT_SECURE_NO_DEPRECATE -DWIN32 -DNATIVE_ENDIAN=1 -DUSESQLITE -DUSEMYSQL -DPOSTGRES -DHAVE_LIBJPEG -DHAVE_LIBJASPER -DJAS_WIN_MSVC_BUILD total.cpp
copy total.obj ms8\total.obj
link /OUT:ms8\dgate.exe /MAP:ms8\dgate.map /DEBUG ms8\total.obj ms8\sqlite3.obj ms8\lua.obj odbc32.lib odbccp32.lib wsock32.lib advapi32.lib user32.lib kernel32.lib ms8\libjpeg.lib ms8\jasperlib.obj

set path=%spath%
