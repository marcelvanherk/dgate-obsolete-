# 19980321 target ACR2DCM.EXE added (mvh)
# 19980321 TRUNCATEFIELDNAMES added (mvh)
# 19980325 TRUNCATEFIELDNAMES now in dicom.ini; removed it again (mvh)
# 19980405 Adde odbccp32.lib (ODBC config) (mvh)
# 19980405 Refer to dicom.lib in \quirt\comps\dll\cqdicom (mvh)
# 19980619 target KILLER.EXE enabled (mvh)
# 19990318 Added nkiqrsop.cxx and .cxx goal (mvh)
# 19990827 Added creation of dgate.map file for profiling (mvh)
# 20010830 Replaced lex.o by lex.cpp (mvh)
# 20010831 Added kernel32.lib (mvh)
# 20020415 Added nkiqrsop and loadddo to killer (needed by device) (mvh)
# 20041012 Put stack size to 10 MB with /F10000000 (mvh)
# 20041013 removed that again
# 20041102 Added JPEG support by ljz (mvh)
# 20050130 Added xvgifwr.c (mvh)
# 20050205 Removed killer.exe as target (mvh)
# 20060311 Set /DUSEMSQL (mvh)
# 20070705 Added sqlite3.c (mvh)
# 20070902 handle c seperately (mvh)
# 20081120 Added postgres loaded dynamically (mvh)
# 20081201 Added jpeg_encoder (mvh)
# 20100120 Added JASPER and JAS_ stuff (mvh)
# 20100120 Added HAVE_LIBJPEG stuff (mvh)
# 20100124 Use gpps.cpp also in windows version (mvh)
# 20100823 Added /DHAVE_J2K
# 20110115 Added lua
# 20120710 Added dgatecgi,  dgatehl7,  luaint, and jpegconv.cpp
# 20120820 Moved amap to cqdicom
# 20130223 Removed gpps.cpp (replaced by configpacs.cxx)
# 20130410 Moved configpacs.cxx here

CC = cl

#LD = link
#LFLAGS = /NODEFAULTLIB:LIBCMT /VERBOSE /DEBUG

LD = cl /Zi /MT
# /Zi /MD added (mvh)

CFLAGS = /Zi /MT /O2 /FaSave.asm
# /O2 added (mvh)

DEFINES = /DWIN32 /DWINDOWS /DNATIVE_ENDIAN=1 /DUSEMYSQL /DUSESQLITE /DPOSTGRES /DHAVE_LIBJPEG /DHAVE_LIBJASPER /DJAS_WIN_MSVC_BUILD /DHAVE_J2K /D__WATCOMC__
# /D "_DEBUG" removed (mvh)

INCLUDES = /I\quirt\comps\dll\cqdicom /I\quirt\comps\exe\Dgate\jasper-1.900.1-6ct\src\libjasper\include /I\quirt\comps\exe\Dgate\jpeg-6c /I\quirt\comps\exe\Dgate\lua-5.1.4

SYSLIBS = odbc32.lib odbccp32.lib wsock32.lib advapi32.lib user32.lib kernel32.lib

LIBS = \quirt\comps\dll\cqdicom\dicom.lib libjpeg.lib
# ijg8.lib ijg12.lib ijg16.lib 

CFILES_DGATE = dgate.cpp dgateweb.cpp dgatehl7.cpp luaint.cpp odbci.cpp parse.cpp parseargs.cpp \
			dbsql.cpp dgatefn.cpp loadddo.cpp dprintf.cpp regen.cpp \
			device.cpp vrtosql.cpp nkiqrsop.cpp jpegconv.cpp lex.cpp \
			xvgifwr.cpp jpeg_encoder.cpp configpacs.cxx
# cqjpeg8.cpp cqjpeg12.cpp cqjpeg16.cpp			

CFILES_MILIB = odbci.cpp parse.cpp parseargs.cpp dbsql.cpp dgatefn.cpp \
			loadddo.cpp dprintf.cpp regen.cpp \
			device.cpp vrtosql.cpp pcomp.cpp configpacs.cxx
			
CFILES_DRIVER = driver.cpp dprintf.cpp

CFILES_KILLER = killer.cpp odbci.cpp parse.cpp parseargs.cpp dbsql.cpp lex.cpp \
			dprintf.cpp device.cpp vrtosql.cpp nkiqrsop.cpp jpegconv.cpp \
			loadddo.obj xvgifwr.c configpacs.cxx
# cqjpeg8.cpp cqjpeg12.cpp cqjpeg16.cpp 

CFILES_FCHECK = fcheck.cpp odbci.cpp parse.cpp parseargs.cpp dbsql.cpp \
			dprintf.cpp device.cpp vrtosql.cpp configpacs.cxx

CFILES_AUTOROUTE = aroute.cpp odbci.cpp parse.cpp parseargs.cpp dbsql.cpp \
			ap.cpp dprintf.cpp device.cpp vrtosql.cpp pcomp.cpp configpacs.cxx

CFILES_FORWARD = forward.cpp odbci.cpp parse.cpp parseargs.cpp dbsql.cpp loadddo.cpp \
			dprintf.cpp device.cpp vrtosql.cpp pcomp.cpp rtcstore.cpp configpacs.cxx

CFILES_FAKEAR = fakear.cpp dprintf.cpp

CFILES_REGENDEV = regendev.cpp odbci.cpp parse.cpp parseargs.cpp dbsql.cpp dgatefn.cpp \
			loadddo.cpp dprintf.cpp regen.cpp \
			device.cpp vrtosql.cpp configpacs.cxx

CFILES_ACR2DCM = acr2dcm.cpp odbci.cpp parse.cpp parseargs.cpp dbsql.cpp dgatefn.cpp \
			loadddo.cpp dprintf.cpp regen.cpp \
			device.cpp vrtosql.cpp pcomp.cpp configpacs.cxx

CFILES_DUMP = dumppacs.cpp odbci.cpp parse.cpp parseargs.cpp dbsql.cpp \
			dprintf.cpp device.cpp vrtosql.cpp configpacs.cxx


.SUFFIXES:	.obj .c .cpp .cxx

.cpp.obj:
	$(CC)	$(CFLAGS) $(DEFINES) $(INCLUDES) /c $*.cpp
	
.c.obj:
	$(CC)	$(CFLAGS) $(DEFINES) $(INCLUDES) /c $*.c

.cxx.obj:
	$(CC)	$(CFLAGS) $(DEFINES) $(INCLUDES) /c $*.cxx

goal:	dgate.exe
# killer.exe
# driver.exe fcheck.exe aroute.exe forward.exe fakear.exe regendev.exe dumppacs.exe acr2dcm.exe mi.lib

#lex.obj:	lex.o
#	copy lex.o lex.obj
	
mi.lib:		$(CFILES_MILIB:.cpp=.obj) $(LIBS)
	lib /OUT:mi.lib \
		$(CFILES_MILIB:.cpp=.obj) lex.obj
	
dgate.exe:	$(CFILES_DGATE:.cpp=.obj) sqlite3.obj $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_DGATE:.cpp=.obj) sqlite3.obj jasperlib.obj lua.obj $(LIBS) $(SYSLIBS) /Fmdgate.map
	
killer.exe:	$(CFILES_KILLER:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_KILLER:.cpp=.obj) $(LIBS) $(SYSLIBS)

driver.exe:	$(CFILES_DRIVER:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_DRIVER:.cpp=.obj) $(LIBS) $(SYSLIBS)

fcheck.exe:	$(CFILES_FCHECK:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_FCHECK:.cpp=.obj) $(LIBS) $(SYSLIBS)

aroute.exe:	$(CFILES_AUTOROUTE:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_AUTOROUTE:.cpp=.obj) $(LIBS) $(SYSLIBS)

forward.exe:	$(CFILES_FORWARD:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_FORWARD:.cpp=.obj) $(LIBS) $(SYSLIBS)

fakear.exe:	$(CFILES_FAKEAR:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_FAKEAR:.cpp=.obj) $(LIBS) $(SYSLIBS)

regendev.exe:	$(CFILES_REGENDEV:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_REGENDEV:.cpp=.obj) $(LIBS) $(SYSLIBS)

acr2dcm.exe:	$(CFILES_ACR2DCM:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_ACR2DCM:.cpp=.obj) $(LIBS) $(SYSLIBS)

dumppacs.exe:	$(CFILES_DUMP:.cpp=.obj) $(LIBS)
	$(LD) $(LFLAGS) $(CFILES_DUMP:.cpp=.obj) $(LIBS) $(SYSLIBS)

