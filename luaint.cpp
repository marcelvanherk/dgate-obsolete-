//
//  luaint.cpp
//
/*
 mvh 20110710: Fix compile of getch and heapinfo linux
 mvh 20120701: Added dicomdelete; note: tried to make heapinfo() thread safe but failed;
 mvh 20120701: Added [lua]nightly script that runs at 03:00; and [lua]background that runs every second
 mvh 20120702: Protect lua_setvar against NULL string
 bcb 20120710: Created, moved lua interface here
 mvh 20120722: Added LUA_5_2 compatibility by bcb; removed unneeded buf[1024]
 bcb 20120820: Added ACRNemaMap class.
 mvh 20120829: Version to 1.4.17
 bcb 20120918: Removed global tables for future Ini class read of dicom.ini, fix warinigs as noted.
 mvh 20121016: 1.4.17alpha release
 mvh 20130125: Added changeuid and changeuidback to lua interface;
 mvh 20130128: Added crc to lua interface
 mvh 20130218: Added studyviewer (default disabled); added dgate --dolua:
                Documented lua command line options; socket.core only for win32 now
 mvh 20130219: 1.4.17beta released as update
 bcb 20130226: Replaced gpps with IniValue class.  Removed all globals now in IniValue class, fix warinigs.
                Made ACRNemaClass a singleton.  Version to 1.4.18a.
 bcb 20130313: Merged with 1.4.17a and b.  Fixed size warnings and lua 5.2 code.
*/

#include "luaint.hpp"
#include "dgateweb.hpp"
//#include "gpps.hpp"
#include "configpacs.hpp"
#ifndef UINT16_MAX
#define UINT16_MAX 0xFFFF
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif
#ifndef INT32_MAX
#define INT32_MAX 0x7FFFFFFF
#endif
#ifndef UNUSED_ARGUMENT
#define UNUSED_ARGUMENT(x) (void)x
#endif

// counters for status display
extern int	StartTime;					// when dgate is started
extern int	TotalTime;					// total run time in s
extern int	LoadTime;					// total time loading
extern int	ProcessTime;					// total time processing (downsize)
extern int	SaveTime;					// total time saving

extern int	ImagesSent;					// how many images were sent
extern int	ImagesReceived;				// idem recieved
extern int	ImagesSaved;					// idem saved
extern int	ImagesForwarded;				// idem forwarded
extern int	ImagesExported;				// executable as export converted
extern int	ImagesCopied;					// copy as export converter

extern int	IncomingAssociations;				// accepted incoming associations (start thread)
extern int	EchoRequest;					// c-echo requests
extern int	C_Find_PatientRoot;
extern int	C_Move_PatientRootNKI;
extern int	C_Move_PatientRoot;
extern int	C_Find_StudyRoot;
extern int	C_Move_StudyRootNKI;
extern int	C_Move_StudyRoot;
extern int	C_Find_PatientStudyOnly;
extern int	C_Find_ModalityWorkList;
extern int	C_Move_PatientStudyOnlyNKI;
extern int	C_Move_PatientStudyOnly;
extern int	UnknownRequest;

extern int	CreateBasicFilmSession;			// printer actions
extern int	DeleteBasicFilmSession;
extern int	ActionBasicFilmSession;
extern int	SetBasicFilmSession;
extern int	CreateBasicFilmBox;
extern int	ActionBasicFilmBox;
extern int	SetBasicFilmBox;
extern int	DeleteBasicFilmBox;
extern int	SetBasicGrayScaleImageBox;
extern int	SetBasicColorImageBox;

extern int	GuiRequest;					// server command requests from GUI
extern int	ImagesToGifFromGui;
extern int	ImagesToDicomFromGui;
extern int	ExtractFromGui;
extern int	QueryFromGui;
extern int	DeleteImageFromGui;
extern int	DeletePatientFromGui;
extern int	DeleteStudyFromGui;
extern int	DeleteStudiesFromGui;
extern int	DeleteSeriesFromGui;
extern int	MovePatientFromGui;
extern int	MoveStudyFromGui;
extern int	MoveStudiesFromGui;
extern int	MoveSeriesFromGui;
extern int	AddedFileFromGui;
extern int	DumpHeaderFromGui;
extern int	ForwardFromGui;
extern int	GrabFromGui;
extern int	DatabaseOpen;				// database activity
extern int	DatabaseClose;
extern int	DatabaseQuery;
extern int	DatabaseAddRecord;
extern int	DatabaseDeleteRecord;
extern int	DatabaseNextRecord;
extern int	DatabaseCreateTable;
extern int	DatabaseUpdateRecords;
extern int	QueryTime;

extern int	SkippedCachedUpdates;			// entering into database levels
extern int	UpdateDatabase;
extern int	AddedDatabase;

extern int	NKIPrivateCompress;			// compression activity
extern int	NKIPrivateDecompress;
extern int	DownSizeImage;
extern int	DecompressJpeg;
extern int	CompressJpeg;
extern int	DecompressJpeg2000;
extern int	CompressJpeg2000;
extern int	DecompressedRLE;
extern int	DePlaned;
extern int	DePaletted;
extern int	RecompressTime;
//extern int	gpps, gppstime;

//extern int	NoDicomCheck;

extern int	DebugLevel;					// increased after error
extern int	ThreadCount;				// Counts total number of associations
extern int	OpenThreadCount;				// Counts total number of open associations
extern int  NumIndexing;		// from odbci.cpp: is -1 on startup, >0 during DBF indexing
//extern char	AutoRoutePipe[];//Unused
//extern char	AutoRouteExec[];//Unused

char	TransmitCompression[COMPRESSION_SZ];			// compression of transmitted images
char	DefaultNKITransmitCompression[COMPRESSION_SZ];		// default compression of transmitted images using NKI C-MOVE
char	TestMode[COMPRESSION_SZ]="";				// if set, appended to filename for comparison
char	StatusString[STATUSSTRING_SZ]="";				// status of submission and move for web i/f

//Global
DICOMDataObject *dummyddo;

/////////////////////////////////////////////////////////////////////////////////////////////////
// lua integration
/////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{ struct scriptdata *getsd(lua_State *L)
  { lua_getglobal(L, "scriptdata");
    struct scriptdata *sd = (struct scriptdata *)lua_topointer(L, -1);
    lua_pop(L, 1);
    return sd;
  }
    // bcb 20120703 moved to hpp
    //  static void luaCreateObject(lua_State *L, DICOMDataObject *pDDO, Array < DICOMDataObject *> *A, BOOL owned);
        
// This is a basic lookup table for global and now non golbal variables.
// The code is faster than before and allows classes declared after main starts, it is also bigger.
// I also used "strileq", now in util.cxx.  It is a very fast compare ignoring the case of the FIRST string,
// the second string must be lower case!  Unlike before, it only compares util a "no match" is found
// or a terminator in either string.
// To add new items add an 
// if(strileq(what, "lower case variablename minus first letter")) return VariableName;
// after the case of the main switch statement
    int *getExtInt(const char *what )
    {
        if(what[0] == 0) return NULL;
        IniValue *iniValuePtr = IniValue::GetInstance();
        switch (*what++)
        {
            case 'A':
            case 'a':
                if(strileq(what, "ctionbasicfilmbox"))return &ActionBasicFilmBox;
                if(strileq(what, "ctionbasicfilmsession"))return &ActionBasicFilmSession;
                if(strileq(what, "ddeddatabase"))return &AddedDatabase;
                if(strileq(what, "ddedfilefromgui"))return &AddedFileFromGui;
                break;
            case 'C':
            case 'c':
                if(strileq(what, "_find_modalityworklist"))return &C_Find_ModalityWorkList;
                if(strileq(what, "_find_patientroot"))return &C_Find_PatientRoot;
                if(strileq(what, "_find_patienstudyonly"))return &C_Find_PatientStudyOnly;
                if(strileq(what, "_find_studyroot"))return &C_Find_StudyRoot;
                if(strileq(what, "_move_patientrootnki"))return &C_Move_PatientRootNKI;
                if(strileq(what, "_move_patientroot"))return &C_Move_PatientRoot;
                if(strileq(what, "_move_patientstudyonlynki")) return &C_Move_PatientStudyOnlyNKI;
                if(strileq(what, "_move_patientstudyonly")) return &C_Move_PatientStudyOnly;
                if(strileq(what, "_move_studyrootnki")) return &C_Move_StudyRootNKI;
                if(strileq(what, "_move_studyroot")) return &C_Move_StudyRoot;
//                if(strileq(what, "achevirtualdata")) return (int *)&(iniValuePtr->sscscpPtr->CacheVirtualData);
                if(strileq(what, "ompressjpeg2000")) return &CompressJpeg2000;
                if(strileq(what, "ompressjpeg")) return &CompressJpeg;
                if(strileq(what, "eatebasicfilmbox"))return &CreateBasicFilmBox;
                if(strileq(what, "eatebasicfilmsession"))return &CreateBasicFilmSession;
                break;  
            case 'D':
            case 'd':
                if(strileq(what, "atabaseaddrecord"))return &DatabaseAddRecord;
                if(strileq(what, "atabasecreatetable"))return &DatabaseCreateTable;
                if(strileq(what, "atabaseclose"))return &DatabaseClose;
                if(strileq(what, "atabasedeleterecord"))return &DatabaseDeleteRecord;
                if(strileq(what, "atabaseopen"))return &DatabaseOpen;
                if(strileq(what, "atabasequery"))return &DatabaseQuery;
                if(strileq(what, "atabaseupdaterecords"))return &DatabaseUpdateRecords;
                if(strileq(what, "ebuglevel"))return &DebugLevel;
                if(strileq(what, "ecompressedrle"))return &DecompressedRLE;
                if(strileq(what, "ecompressejpeg2000")) return &DecompressJpeg2000;
                if(strileq(what, "ecompressejpeg")) return &DecompressJpeg;
                if(strileq(what, "eletebasicfilmbox"))return &DeleteBasicFilmBox;
                if(strileq(what, "eletebasicfilmsession"))return &DeleteBasicFilmSession;
                if(strileq(what, "eleteimagefromgui"))return &DeleteImageFromGui;
                if(strileq(what, "eletepatientfromgui"))return &DeletePatientFromGui;
                if(strileq(what, "eleteseriesfromgui"))return &DeleteSeriesFromGui;
                if(strileq(what, "eletestudiesfromgui"))return &DeleteStudiesFromGui;
                if(strileq(what, "eletestudyfromgui"))return &DeleteStudyFromGui;
                if(strileq(what, "epaletted"))return &DePaletted;
                if(strileq(what, "eplaned"))return &DePlaned;
//                if(strileq(what, "oublebackslashtodb"))
//                        return (int *)(&(iniValuePtr->sscscpPtr->DoubleBackSlashToDB));
                if(strileq(what, "ownsizeimage"))return &DownSizeImage;
                if(strileq(what, "umpheaderfromgui"))return &DumpHeaderFromGui;
                break;
            case 'E':
            case 'e':
                if(strileq(what, "chorequest"))return &EchoRequest;
 //               if(strileq(what, "nablecomputedfields"))return (int *)&(iniValuePtr->sscscpPtr->EnableComputedFields);
 //               if(strileq(what, "nablereadaheadthread"))return (int *)&(iniValuePtr->sscscpPtr->EnableReadAheadThread);
                if(strileq(what, "xtractfromgui"))return &ExtractFromGui;
                break;
            case 'F':
            case 'f':
 //               if(strileq(what, "ailholdoff"))return (int *)&(iniValuePtr->sscscpPtr->FailHoldOff);
 //               if(strileq(what, "ilecompressmode"))
 //                       return (int *)(&(iniValuePtr->sscscpPtr->FileCompressMode));
 //               if(strileq(what, "ixkodak"))return (int *)&(iniValuePtr->sscscpPtr->FixKodak);
 //               if(strileq(what, "orwardcollectdelay"))return (int *)&(iniValuePtr->sscscpPtr->ForwardCollectDelay);
                if(strileq(what, "orwardfromguik"))return &ForwardFromGui;
                break;
            case 'G':
            case 'g':
 //               if(strileq(what, "jpegquality"))return (int *)&(iniValuePtr->sscscpPtr->LossyQuality);
                if(strileq(what, "ppstime"))return &(iniValuePtr->gppstime);
                if(strileq(what, "pps"))return &(iniValuePtr->gpps);
                if(strileq(what, "rabfromgui"))return &GrabFromGui;
                if(strileq(what, "uirequest"))return &GuiRequest;
#ifdef HAVE_BOTH_J2KLIBS
 //               if(strileq(what, "useopenjpeg"))return (int *)&(iniValuePtr->sscscpPtr->UseOpenJPG);
#endif
                break;
            case 'I':
            case 'i':
                if(strileq(what, "ncomingassociations"))return &IncomingAssociations;
                if(strileq(what, "magescopied"))return &ImagesCopied;
                if(strileq(what, "magesexported"))return &ImagesExported;
                if(strileq(what, "magesforwarded"))return &ImagesForwarded;
                if(strileq(what, "magesreceived"))return &ImagesReceived;
                if(strileq(what, "magessaved"))return &ImagesSaved;
                if(strileq(what, "magessent"))return &ImagesSent;
                if(strileq(what, "magestodicomfromgui"))return &ImagesToDicomFromGui;
                if(strileq(what, "magestogiffromgui"))return &ImagesToGifFromGui;
                break;
            case 'L':
            case 'l':
                if(strileq(what, "oadtime"))return &LoadTime;
                break;
            case 'M':
            case 'm':
                if(strileq(what, "ovepatientfromgui"))return &MovePatientFromGui;
                if(strileq(what, "oveseriesfromgui"))return &MoveSeriesFromGui;
                if(strileq(what, "ovestudiesfromgui"))return &MoveStudiesFromGui;
                if(strileq(what, "ovestudyfromgui"))return &MoveStudyFromGui;
                break;
            case 'N':
            case 'n':
  //              if(strileq(what, "odicomcheck"))return (int *)&(iniValuePtr->sscscpPtr->NoDicomCheck);
                if(strileq(what, "umindexing"))return &NumIndexing;
                if(strileq(what, "kiprivatecompress"))return &NKIPrivateCompress;
                if(strileq(what, "kiprivatedecompress"))return &NKIPrivateDecompress;
                break;
            case 'O':
            case 'o':
                if(strileq(what, "penthreadcount"))return &OpenThreadCount;
                break;
            case 'P':
            case 'p':
                if(strileq(what, "rocesstime"))return &ProcessTime;
                break;
            case 'Q':
            case 'q':
                if(strileq(what, "ueryfromgui"))return &QueryFromGui;
                if(strileq(what, "uerytime"))return &QueryTime;
 //               if(strileq(what, "ueuesize"))return (int *)&(iniValuePtr->sscscpPtr->QueueSize);
                break;
            case 'R':
            case 'r':
                if(strileq(what, "ecompresstime"))return &RecompressTime;
//                if(strileq(what, "etrydelay"))return (int *)&(iniValuePtr->sscscpPtr->RetryDelay);
                break;
            case 'S':
            case 's':
                if(strileq(what, "avetime"))return &SaveTime;
                if(strileq(what, "etbasiccolorimagebox"))return &SetBasicColorImageBox;
                if(strileq(what, "etbasicfilmbox"))return &SetBasicFilmBox;
                if(strileq(what, "etbasicfilmsession"))return &SetBasicFilmSession;
                if(strileq(what, "etbasicgrayscaleimagebox"))return &SetBasicGrayScaleImageBox;
                if(strileq(what, "kippedcachedupdates"))return &SkippedCachedUpdates;
                if(strileq(what, "tarttime"))return &StartTime;
//                if(strileq(what, "toragefailederrorcode"))return (int *)&(iniValuePtr->sscscpPtr->StorageFailedErrorCode);
                break;
            case 'T':
            case 't':
                if(strileq(what, "hreadcount"))return &ThreadCount;
                if(strileq(what, "otaltime"))return &TotalTime;
                break;
            case 'U':
            case 'u':
                if(strileq(what, "nknownrequest"))return &UnknownRequest;
                if(strileq(what, "pdatedatabase"))return &UpdateDatabase;
//                if(strileq(what, "seescapestringconstants"))return (int *)&(iniValuePtr->sscscpPtr->UseEscapeStringConstants);
                break;
//            case 'W':
//            case 'w':
//                if(strileq(what, "orklistmode"))return (int *)&(iniValuePtr->sscscpPtr->WorkListMode);
            default:
                break;
        }
        what--;
        return iniValuePtr->GetIniInitPtr(what);
    }

// bcb, used this instead of a global structure because of new IniValue class. (also its faster!)
    int getExtSLen(const char *what , char **valueH)
    {
        if(what[0] == 0) return 0;
        IniValue *iniValuePtr = IniValue::GetInstance();
        switch (*what++)
        {
            case 'A':
            case 'a':
/*            if(strileq(what, "rchivecompression"))//ArchiveCompression
                {
                    *valueH = iniValuePtr->sscscpPtr->ArchiveCompression;
                    return COMPRESSION_SZ;
                }*/
                break;
            case 'B':
            case 'b':
                if(strileq(what, "asedir"))//BaseDir
                {
                    *valueH = iniValuePtr->GetBasePath();
                    return MAX_PATH;
                }
                break;
            case 'C':
            case 'c':
                if(strileq(what, "onfigfile"))//ConfigFile
                {
                    *valueH = iniValuePtr->GetIniFile();
                    return MAX_PATH;
                }
                break;
            case 'D':
            case 'd':
                if(strileq(what, "efaultnkitransmitcompression"))//DefaultNKITransmitCompression(unused now)
                {
                    *valueH = DefaultNKITransmitCompression;
                    return COMPRESSION_SZ;
                }
/*                if(strileq(what, "icomdict"))//DicomDict
                {
                    *valueH = iniValuePtr->sscscpPtr->Dictionary;
                    return MAX_PATH;
                }
                if(strileq(what, "roppedfilecompression"))//DroppedFileCompression
                {
                    *valueH = iniValuePtr->sscscpPtr->DroppedFileCompression;
                    return ACRNEMA_COMP_LEN;
                }*/
                break;
            case 'I':
            case 'i':
                if(strileq(what, "ncomingcompression"))//IncomingCompression
                {
                    *valueH = iniValuePtr->sscscpPtr->IncomingCompression;
                    return ACRNEMA_COMP_LEN;
                }
                break;
            case 'R':
            case 'r':
                if(strileq(what, "ootconfig"))//RootConfig
                {
                    *valueH = iniValuePtr->sscscpPtr->MicroPACS;
                    return SHORT_NAME_LEN;
                }
                break;
            case 'S':
            case 's':
                if(strileq(what, "tatusString"))//StatusString
                {
                    *valueH = StatusString;
                    return STATUSSTRING_SZ;
                }
                break;
            case 'T':
            case 't':
                if(strileq(what, "estmode"))//TestMode
                {
                    *valueH = TestMode;
                    return COMPRESSION_SZ;
                }
                if(strileq(what, "ransmitcompression"))//TransmitCompression
                {
                    *valueH = TransmitCompression;
                    return COMPRESSION_SZ;
                }
            default:
                break;
        }
        what--;
        return iniValuePtr->GetIniStringLen(what, valueH);
    }

  static int _luaprint(lua_State *L, Debug *d)
  { int i, n=lua_gettop(L);
    char text[2000]; text[0]=0;
    for (i=1; i<=n; i++)
    { if (i>1)                     sprintf(text + strlen(text), "\t");
      if (lua_isstring(L,i))       sprintf(text + strlen(text), "%s",lua_tostring(L,i));
      else if (lua_isnil(L,i))     sprintf(text + strlen(text), "%s","nil");
      else if (lua_isboolean(L,i)) sprintf(text + strlen(text), "%s",lua_toboolean(L,i) ? "true" : "false");
      else                         sprintf(text + strlen(text), "%s:%p",luaL_typename(L,i),lua_topointer(L,i));
    }
    d->printf("%s\n", text);
    return 0;
  }
  static int luaprint(lua_State *L)    { return _luaprint(L, &OperatorConsole); }
  static int luadebuglog(lua_State *L) { return _luaprint(L, &SystemDebug);     }
  static int luaservercommand(lua_State *L)
  { int n=lua_gettop(L); UNUSED_ARGUMENT(n);
    char buf[2000];
    buf[0]=0;
    if (lua_isstring(L,1)) SendServerCommand("", lua_tostring(L,1), 0, buf);
    if (buf[0]) { lua_pushstring(L, buf); return 1; }
    return 0;
  }
  static int luadictionary(lua_State *L)
  { if (lua_gettop(L)==2)
    { lua_Integer g = lua_tointeger(L,1);
      lua_Integer e = lua_tointeger(L,2);
      char s[256]; s[0] = '\0';
      if (VRType.RunTimeClass(g & UINT16_MAX , e & UINT16_MAX, s))
      { lua_pushstring(L, s);
      	return 1;
      }
    }
    else if (lua_gettop(L)==1)
    { RTCElement Entry;
      char s[256];
      strcpy(s, lua_tostring(L, 1));
      Entry.Description = s;
      if (VRType.GetGroupElement(&Entry))
      { lua_pushinteger(L, Entry.Group);
        lua_pushinteger(L, Entry.Element);
        return 2;
      }
    }
    return 0;
  }
  static int luachangeuid(lua_State *L)
  { if (lua_gettop(L)==1)
    { char from[256], to [256];
      strcpy(from, lua_tostring(L, 1));
      if (ChangeUID(from, "lua", to))
      { lua_pushstring(L, to);
        return 1;
      }
    }
    if (lua_gettop(L)==2)
    { char from[256], to[256];//, result[256];
      strcpy(from, lua_tostring(L, 1));
      strcpy(to, lua_tostring(L, 2));
      if (ChangeUIDTo(from, "lua", to))
      { lua_pushstring(L, to);
        return 1;
      }
    }
    lua_pushnil(L);
    return 1;
  }
  static int luachangeuidback(lua_State *L)
  { if (lua_gettop(L)==1)
    { char from[256], to [256];
      strcpy(from, lua_tostring(L, 1));
      if (ChangeUIDBack(from, to))
      { lua_pushstring(L, to);
        return 1;
      }
    }
    lua_pushnil(L);
    return 1;
  }
  static int luacrc(lua_State *L)
  { if (lua_gettop(L)==1)
    { char from[256];
      strcpylim(from, lua_tostring(L, 1), 256);
      unsigned int crc = ComputeCRC(from, strlen(from));
      sprintf(from, "%u", crc);
      lua_pushstring(L, from);
      return 1;
    }
    lua_pushnil(L);
    return 1;
  }
  static int luagenuid(lua_State *L)
  { char n[80];
    GenUID(n);
    lua_pushstring(L, n);
    return 1;
  }
  static int luascript(lua_State *L)
  { struct scriptdata *sd = getsd(L);
    int n=lua_gettop(L);
    char cmd[2000]="";
    DICOMDataObject *pDDO;
    pDDO = sd->DDO;
    if (n>0)
    { if (lua_isuserdata(L, 1)) 
      { lua_getmetatable(L, 1);
          lua_getfield(L, -1, "DDO");  pDDO = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
        lua_pop(L, 1);
      }
      else if (lua_isstring(L,1)) 
      	strcpy(cmd, lua_tostring(L,1));
    }
    if (n>1 && lua_isstring(L,2)) 
      strcpy(cmd, lua_tostring(L,2));
    sd->rc = CallImportConverterN(pDDO, -1, sd->pszModality, sd->pszStationName, sd->pszSop, sd->patid, sd->PDU, sd->Storage, cmd);
    return 0;
  }
  static int luaget_amap(lua_State *L)
  { ACRNemaAddress*	AAPtr;
    if((AAPtr = ACRNemaMap::GetInstance()->GetOneACRNemaAddress(lua_tointeger(L, 1) & UINT_MAX)) != NULL)
    { lua_pushstring(L, AAPtr->Name);
      lua_pushstring(L, AAPtr->IP);
      lua_pushstring(L, AAPtr->Port);
      lua_pushstring(L, AAPtr->Compress);
      return 4;
    }
    return 0;
  }
  static int luaget_sqldef(lua_State *L)
  { lua_Integer r = lua_tointeger(L, 1);
    lua_Integer s = lua_tointeger(L, 2);
    DBENTRY *DBE = NULL;
    if (r==0) DBE = PatientDB;
    if (r==1) DBE = StudyDB;
    if (r==2) DBE = SeriesDB;
    if (r==3) DBE = ImageDB;
    if (r==4) DBE = WorkListDB;
    if (DBE)
    { if (DBE[s].Group)
      { lua_pushinteger(L, DBE[s].Group);
        lua_pushinteger(L, DBE[s].Element);
	lua_pushstring(L, DBE[s].SQLColumn);
	lua_pushinteger(L, DBE[s].SQLLength);
	lua_pushstring(L, SQLTypeSymName(DBE[s].SQLType));
	lua_pushstring(L, DICOMTypeSymName(DBE[s].DICOMType));
	return 6;
      }
    }
    return 0;
  }
  static int luadbquery(lua_State *L)
  { unsigned int i, N, flds=1;
    const char *items[4];
    SQLLEN SQLResultLength;
    for (i=0; i<4; i++) items[i]=lua_tostring(L,i+1);
    if (items[1]) for (i=0; i<strlen(items[1]); i++) if (items[1][i]==',') flds++;
    if (items[2]) if (*items[2]==0) items[2]=NULL;
    if (items[3]) if (*items[3]==0) items[3]=NULL;
//    if (items[4]) if (*items[4]==0) items[4]=NULL;//bcb overflow.

    IniValue *iniValuePtr = IniValue::GetInstance();
    Database DB;
    if (DB.Open ( iniValuePtr->sscscpPtr->DataSource, iniValuePtr->sscscpPtr->UserName,
      iniValuePtr->sscscpPtr->Password, iniValuePtr->sscscpPtr->DataHost ) )
    { char fld[48][256]; memset(fld, 0, sizeof(fld));
      DB.Query(items[0], items[1], items[2], items[3]);
      for (i=0; i<flds; i++) DB.BindField (i+1, SQL_C_CHAR, fld[i], 255, &SQLResultLength);
      N = 1; lua_newtable(L);
      while (DB.NextRecord())
      { lua_newtable(L);
        for (i=0; i<flds; i++) { lua_pushstring(L, fld[i]); lua_rawseti(L, -2, i+1); }
	lua_rawseti(L, -2, N++);
	memset(fld, 0, sizeof(fld));
      }
      DB.Close();
      QueryFromGui++;
      return 1;
    }
    return 0;
  }
  static int luasql(lua_State *L)
  { const char *sql = lua_tostring(L,1);
    BOOL f = false;

    IniValue *iniValuePtr = IniValue::GetInstance();
    Database DB;
    if (DB.Open ( iniValuePtr->sscscpPtr->DataSource, iniValuePtr->sscscpPtr->UserName,
      iniValuePtr->sscscpPtr->Password, iniValuePtr->sscscpPtr->DataHost ) )
    { f = DB.Exec(sql);
      DB.Close();
      QueryFromGui++;
    }
    if (f)
    { lua_pushinteger(L, 1);
      return 1;
    }
    else
      return 0;
  }
  static int luadicomquery(lua_State *L)
  { const char *ae    = lua_tostring(L,1);
    const char *level = lua_tostring(L,2);
    if (lua_isuserdata(L, 3)) 
    { DICOMDataObject *O = NULL;
      lua_getmetatable(L, 3);
        lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_pop(L, 1);
      Array < DICOMDataObject * > *A = new Array < DICOMDataObject * >;
      luaCreateObject(L, NULL, A, TRUE); 
      if (O) 
      { DICOMDataObject *P = MakeCopy(O);
        VirtualQuery(P, level, 0, A, (char *)ae);
        delete P;
      }
      return 1;
    }
    return 0;
  }
  static int luadicommove(lua_State *L)
  { const char *source = lua_tostring(L,1);
    const char *dest   = lua_tostring(L,2);
    if (lua_isuserdata(L, 3)) 
    { DICOMDataObject *O = NULL;
      lua_getmetatable(L, 3);
        lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_pop(L, 1);
      if (O)
      { DICOMDataObject *P = MakeCopy(O);
        DcmMove2((char *)source, dest, lua_tointeger(L,4) & UINT_MAX, 0x555, P, lua_tostring(L,4));
        delete P;
      }
    }
    return 0;
  }
  static int luadicomdelete(lua_State *L)
  { if (lua_isuserdata(L, 1))
    { DICOMDataObject *O = NULL;
      lua_getmetatable(L, 1);
        lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_pop(L, 1);
      Array < DICOMDataObject * > *A = new Array < DICOMDataObject * >;
      luaCreateObject(L, NULL, A, TRUE);
      if (O)
      { DICOMDataObject *P = MakeCopy(O);
	RemoveFromPACS(P, FALSE);
        delete P;
      }
      return 1;
    }
    return 0;
  }

  static int getptr(lua_State *L, unsigned int *ptr, int *veclen, int *bytes, int *count, int *step, VR **vr, int mode)
  { struct scriptdata *sd = getsd(L); //mode:0,1,2,3=pixel,row,col,image
    int pars=lua_gettop(L), n, rows, cols, frames, vec;
    lua_Integer x=0, y=0, z=0;
    Array < DICOMDataObject *> *pADDO;
    DICOMDataObject *pDDO;
    *veclen = 0;
    n=1;
    pDDO = sd->DDO;
      
    if (pars>0)
    { if (lua_isuserdata(L, n)) 
      { lua_getmetatable(L, n);
          lua_getfield(L, -1, "DDO");  pDDO = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
        lua_pop(L, 1);
        n++; pars--;
      }
    }

    if (pars>0 && (mode==0 || mode==2)) x = lua_tointeger(L,n++), pars--;
    if (pars>0 && (mode==0 || mode==1)) y = lua_tointeger(L,n++), pars--;
    if (pars>0                        ) z = lua_tointeger(L,n++), pars--;

    vec   = pDDO->GetUINT16(0x0028,0x0002);
    rows  = pDDO->GetUINT16(0x0028, 0x0010); if (!rows) return 0;
    cols  = pDDO->GetUINT16(0x0028, 0x0011); if (!cols) return 0;
    if (pDDO->GetUINT16(0x0028, 0x0006)>1)              return 0;
    frames= pDDO->Getatoi(0x0028, 0x0008);
    *bytes = (pDDO->GetUINT16(0x0028, 0x0101)+7)/8;
    *ptr   = ((y * cols + x) & UINT_MAX) * vec;
    DecompressNKI(pDDO);
    *vr    = pDDO->GetVR(0x7fe0, 0x0010); if (!vr)   return 0;
    pADDO = (Array<DICOMDataObject*>*)(*vr)->SQObjectArray;
    if (pADDO==NULL)
    { *ptr += z*rows * cols * vec;
      if (*ptr * *bytes > (*vr)->Length) return 0;
    }
    else
    { if (z>=(int)(pADDO->GetSize())-1) return 0;// bcb cast
      *vr = pADDO->Get((z+1) & UINT_MAX)->GetVR(0x7fe0, 0x0010);
      if (*ptr * *bytes > (*vr)->Length) return 0;
    }
    if (mode!=2) *step  = 1;   else *step = cols;
    if (mode>=2) *count= rows; else *count = cols;
    if (mode==3) *count *= cols;
    *veclen = vec;
    return n;
  }
  static int luagetpixel(lua_State *L)
  { int veclen, count, step, bytes;
    unsigned int ptr;
    VR *vr;
    getptr(L, &ptr, &veclen, &bytes, &count, &step, &vr, 0);
    if (veclen==0) return 0;
    switch(bytes)
    { case 1: for (int i=0; i<veclen; i++) lua_pushinteger(L, *((BYTE   *)(vr->Data)+ptr+i*step)); break;
      case 2: for (int i=0; i<veclen; i++) lua_pushinteger(L, *((UINT16 *)(vr->Data)+ptr+i*step)); break;
      case 4: for (int i=0; i<veclen; i++) lua_pushinteger(L, *((INT32  *)(vr->Data)+ptr+i+step)); break;
      default:for (int i=0; i<veclen; i++) lua_pushinteger(L,                                  0); break;
    }
    return veclen;
  }
  static int luasetpixel(lua_State *L)
  { int pars=lua_gettop(L), veclen, count, step, bytes, n;
    unsigned int ptr;
    VR *vr;
    n=getptr(L, &ptr, &veclen, &bytes, &count, &step, &vr, 0);
    if (veclen==0) return 0;
    if (veclen>pars-n+1) veclen=pars-n+1;
    switch(bytes)
    { case 1: for (int i=0; i<veclen; i++) *((BYTE   *)(vr->Data)+ptr+i*step) = lua_tointeger(L,n++) & 0xFF; break;
      case 2: for (int i=0; i<veclen; i++) *((UINT16 *)(vr->Data)+ptr+i*step) = lua_tointeger(L,n++) & UINT16_MAX; break;
      case 4: for (int i=0; i<veclen; i++) *((INT32  *)(vr->Data)+ptr+i*step) = lua_tointeger(L,n++) & UINT32_MAX; break;
    }
    return 0;
  }
  static int luagetrow(lua_State *L)
  { int veclen, count, step, bytes;
    unsigned int ptr;
    VR *vr;
    getptr(L, &ptr, &veclen, &bytes, &count, &step, &vr, 1);
    if (veclen==0) return 0;
    lua_newtable(L);
    for(int x=0; x<count; x++)
    { switch(bytes)
      { case 1: for (int i=0; i<veclen; i++) {lua_pushinteger(L, *((BYTE   *)(vr->Data)+ptr+i)); lua_rawseti(L, -2, x*veclen+i);} break;
        case 2: for (int i=0; i<veclen; i++) {lua_pushinteger(L, *((UINT16 *)(vr->Data)+ptr+i)); lua_rawseti(L, -2, x*veclen+i);} break;
        case 4: for (int i=0; i<veclen; i++) {lua_pushinteger(L, *((INT32  *)(vr->Data)+ptr+i)); lua_rawseti(L, -2, x*veclen+i);} break;
      }
      ptr += step*veclen;
    }
    return 1;
  }
  static int luagetcolumn(lua_State *L)
  { int veclen, count, step, bytes;
    unsigned int ptr;
    VR *vr;
    getptr(L, &ptr, &veclen, &bytes, &count, &step, &vr, 2);
    if (veclen==0) return 0;
    lua_newtable(L);
    for(int x=0; x<count; x++)
    { switch(bytes)
      { case 1: for (int i=0; i<veclen; i++) {lua_pushinteger(L, *((BYTE   *)(vr->Data)+ptr+i)); lua_rawseti(L, -2, x*veclen+i);} break;
        case 2: for (int i=0; i<veclen; i++) {lua_pushinteger(L, *((UINT16 *)(vr->Data)+ptr+i)); lua_rawseti(L, -2, x*veclen+i);} break;
        case 4: for (int i=0; i<veclen; i++) {lua_pushinteger(L, *((INT32  *)(vr->Data)+ptr+i)); lua_rawseti(L, -2, x*veclen+i);} break;
      }
      ptr += step*veclen;
    }
    return 1;
  }
  static int luasetrow(lua_State *L)
  { int pars=lua_gettop(L), veclen, count, step, bytes, n;
    unsigned int ptr;
    VR *vr;
    n=getptr(L, &ptr, &veclen, &bytes, &count, &step, &vr, 1);
    if (veclen==0 || n>pars) return 0;
    for(int x=0; x<count; x++)
    { switch(bytes)
      { case 1: for (int i=0; i<veclen; i++)
                { lua_rawgeti(L, pars, x*veclen+i); *((BYTE   *)(vr->Data)+ptr+i) =lua_tointeger(L,-1) & 0xFF;
                  lua_pop(L, 1); } break;
        case 2: for (int i=0; i<veclen; i++)
                { lua_rawgeti(L, pars, x*veclen+i); *((UINT16 *)(vr->Data)+ptr+i) = lua_tointeger(L,-1) & UINT16_MAX;
                  lua_pop(L, 1); } break;
        case 4: for (int i=0; i<veclen; i++)
                { lua_rawgeti(L, pars, x*veclen+i); *((INT32  *)(vr->Data)+ptr+i) = lua_tointeger(L,-1) & UINT32_MAX;
                  lua_pop(L, 1); } break;
      }
      ptr += step*veclen;
    }
    return 1;
  }
  static int luasetcolumn(lua_State *L)
  { int pars=lua_gettop(L), veclen, count, step, bytes, n;
    unsigned int ptr;
    VR *vr;
    n=getptr(L, &ptr, &veclen, &bytes, &count, &step, &vr, 2);
    if (veclen==0 || n>pars) return 0;
    for(int x=0; x<count; x++)
    { switch(bytes)
      { case 1: for (int i=0; i<veclen; i++)
                { lua_rawgeti(L, pars, x*veclen+i); *((BYTE   *)(vr->Data)+ptr+i) = lua_tointeger(L,-1) & 0xFF;
                  lua_pop(L, 1); } break;
        case 2: for (int i=0; i<veclen; i++)
                { lua_rawgeti(L, pars, x*veclen+i); *((UINT16 *)(vr->Data)+ptr+i) = lua_tointeger(L,-1) & UINT16_MAX;
                  lua_pop(L, 1); } break;
        case 4: for (int i=0; i<veclen; i++)
                { lua_rawgeti(L, pars, x*veclen+i); *((INT32  *)(vr->Data)+ptr+i) = lua_tointeger(L,-1) & UINT32_MAX;
                  lua_pop(L, 1); } break;
      }
      ptr += step*veclen;
    }
    return 1;
  }
  static int luagetimage(lua_State *L)
  { int veclen, count, step, bytes;
    unsigned int ptr;
    VR *vr;
    getptr(L, &ptr, &veclen, &bytes, &count, &step, &vr, 3);
    if (veclen==0) return 0;
    lua_pushlstring(L, (char *)(vr->Data)+ptr, count*bytes);
    return 1;
  }
  static int luasetimage(lua_State *L)
  { int pars=lua_gettop(L), veclen, count, step, bytes, n;
    size_t len; unsigned int ptr;
    VR *vr; const char *data;
    n=getptr(L, &ptr, &veclen, &bytes, &count, &step, &vr, 3);
    if (veclen==0 || n>pars) return 0;
    data = lua_tolstring (L, pars, &len);
    if (data && len==(size_t)count*(size_t)bytes)//bcb casts.
      memcpy((BYTE *)(vr->Data)+ptr, (BYTE *)data, len);
    return 1;
  }
  
  // getvr(Object, group, element) returns table for normal VR, addo for sequence
  static int luagetvr(lua_State *L)
  { VR *vr;
    if (lua_isuserdata(L,1)) 
    { DICOMDataObject *O = NULL;
      lua_getmetatable(L, 1);
        lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_pop(L, 1);
      lua_Integer g = lua_tointeger(L,2);
      lua_Integer e = lua_tointeger(L,3);
      int b = lua_toboolean(L,4);
      vr = O->GetVR(g & UINT16_MAX, e & UINT16_MAX);
      if (vr)
      { if (vr->SQObjectArray)
          luaCreateObject(L, NULL, (Array < DICOMDataObject * > *)vr->SQObjectArray, FALSE);
        else if (b==0)
        { lua_newtable(L);
          for(unsigned int i=0; i<vr->Length; i++)//bcb unsigned
          { lua_pushinteger(L, *((BYTE *)(vr->Data)+i));
            lua_rawseti(L, -2, i);
          }
        }
        else
        { lua_pushlstring(L, (char *)(vr->Data), vr->Length);
        }
        return 1;
      }
    }
    return 0;
  }
  // setvr(Object, group, element, data); if data is a table sets a simple VR, if it is an ADDO sets a sequence
  static int luasetvr(lua_State *L)
  { VR *vr;
    if (lua_isuserdata(L,1)) 
    { DICOMDataObject *O = NULL;
      Array < DICOMDataObject * > *A = NULL;
      lua_getmetatable(L, 1);
        lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_pop(L, 1);
      lua_Integer g = lua_tointeger(L,2);
      lua_Integer e = lua_tointeger(L,3);
      if (lua_isuserdata(L, 4) && O)
      { lua_getmetatable(L, 4);
          lua_getfield(L, -1, "ADDO");  A = (Array < DICOMDataObject * > *) lua_topointer(L, -1); lua_pop(L, 1);
        lua_pop(L, 1);
	if (A)
        { vr = new VR(g & UINT16_MAX, e & UINT16_MAX, 0, (void *) NULL, FALSE);
          O->DeleteVR(vr);
	  delete vr;
	  vr = new VR(g & UINT16_MAX, e & UINT16_MAX, 0, (void *) NULL, FALSE);
          Array < DICOMDataObject * > *SQE  = new Array <DICOMDataObject *>;
	  for (unsigned int j=0; j<A->GetSize(); j++)//bcb unsigned
	  { DICOMDataObject *dd = MakeCopy(A->Get(j));
	    SQE->Add(dd);
	  }
	  vr->SQObjectArray = SQE;
          O->Push(vr);
	}
      }
      else if (lua_istable(L, 4) && O)
      { size_t llen;
        vr = O->GetVR(g, e);
        if (!vr) 
        { vr = new VR(g, e, 0, (void *) NULL, FALSE);
          O->Push(vr);
        }
#ifdef LUA_5_2
        llen = lua_rawlen(L, 4);
#else
        llen = lua_objlen(L, 4);
#endif
        vr->ReAlloc((llen+1) & UINT32_MAX);
        for(size_t i=0; i<=llen; i++)
        { lua_rawgeti(L, 4, i & UINT_MAX);
          *((BYTE *)(vr->Data)+i) = lua_tointeger(L,-1);
          lua_pop(L, 1);
        }
      }
      else if (lua_isstring(L, 4) && O)
      { size_t llen;
	const char *data;
        vr = O->GetVR(g, e);
        if (!vr) 
        { vr = new VR(g, e, 0, (void *) NULL, FALSE);
          O->Push(vr);
        }
        data = lua_tolstring (L, 4, &llen);
        vr->ReAlloc(llen & INT32_MAX);
	if (data && llen) memcpy((BYTE *)(vr->Data), (BYTE *)data, llen);
      }
    }
    return 0;
  }

  // readdicom(filename) into Data, or readdicom(x, filename)
  static int luareaddicom(lua_State *L)
  { struct scriptdata *sd = getsd(L);
    DICOMDataObject *O, *P;
    if (lua_isstring(L,1)) 
    { char name[512];
      strcpylim(name, (char *)lua_tostring(L,1), 512);
      O = LoadForGUI(name);
      if (!O) return 0;
      if (sd->DDO && sd->DDO!=dummyddo) delete (sd->DDO);
      sd->DDO = O;
      lua_getglobal   (L, "Data");  
      lua_getmetatable(L, -1);
        lua_pushlightuserdata(L, sd->DDO); lua_setfield(L, -2, "DDO");
      lua_pop(L, 2);
    }
    else if (lua_isuserdata(L,1))
    { lua_getmetatable(L, 1);
        lua_getfield(L, -1, "DDO");  P = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
	  char name[512];
        strcpylim(name, (char *)lua_tostring(L,2), 512);
        O = LoadForGUI(name);
        if (!O)
        { lua_pop(L, 1);
          return 0;
        }
        if (P) delete P;
        lua_pushlightuserdata(L, O); lua_setfield(L, -2, "DDO");
      lua_pop(L, 1);
    }
    lua_pushinteger(L, 1);
    return 1;
  }
  // writedicom(filename), writedicom(Data, filename); Data:writedicom(filename)
  static int luawritedicom(lua_State *L)
  { struct scriptdata *sd = getsd(L);
    if (lua_isstring(L,1) && sd->DDO) 
    { DICOMDataObject *pDDO = MakeCopy(sd->DDO);
      SaveDICOMDataObject((char *)lua_tostring(L,1), pDDO);
      delete pDDO;
    }
    else if (lua_isuserdata(L,1)) 
    { DICOMDataObject *O = NULL;
      lua_getmetatable(L, 1);
        lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_pop(L, 1);

      if (O)
      { DICOMDataObject *pDDO = MakeCopy(O);
        SaveDICOMDataObject((char *)lua_tostring(L,2), pDDO);
        delete pDDO;
      }
    }
    return 0;
  }

  // Data:writeheader(filename) or writeheader(filename)
  static int luawriteheader(lua_State *L) 
  { struct scriptdata *sd = getsd(L);
    if (lua_isuserdata(L,1)) 
    { DICOMDataObject *O = NULL;
      lua_getmetatable(L, 1);
        lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_pop(L, 1);

      if (O)
      { FILE *f = fopen(lua_tostring(L, 2), "wt");
        if (f) 
        { NonDestructiveDumpDICOMObject(O, f);
          fclose(f);
        }
      }
    }
    else if (lua_isstring(L,1)) 
    { FILE *f = fopen(lua_tostring(L, 1), "wt");
      if (f) 
      { if (sd->DDO) NonDestructiveDumpDICOMObject(sd->DDO, f);
        fclose(f);
      }
    }
    return 0;
  }
  
  // newdicomobject()
  static int luanewdicomobject(lua_State *L)
  { DICOMDataObject *O = new DICOMDataObject;
    luaCreateObject(L, O, NULL, TRUE); 
    return 1;
  }
  
  // newdicomarray()
  static int luanewdicomarray(lua_State *L)
  { Array < DICOMDataObject * > *A = new Array < DICOMDataObject * >;
    luaCreateObject(L, NULL, A, TRUE); 
    return 1;
  }

  // deletedicomobject()
  static int luadeletedicomobject(lua_State *L)
  { struct scriptdata *sd = getsd(L);
    DICOMDataObject *O = NULL;
    lua_Integer owned = false;//bcb Ininitailized warning.
    Array < DICOMDataObject * > *A = NULL;
    if (lua_isuserdata(L,1)) 
    { lua_getmetatable(L, 1);
        lua_getfield(L, -1, "DDO");   O = (DICOMDataObject *)             lua_topointer(L, -1); lua_pop(L, 1);
        lua_getfield(L, -1, "ADDO");  A = (Array < DICOMDataObject * > *) lua_topointer(L, -1); lua_pop(L, 1);
        lua_getfield(L, -1, "owned"); owned =                             lua_tointeger(L, -1); lua_pop(L, 1);
        lua_pushlightuserdata(L, NULL); lua_setfield(L, -2, "DDO");
        lua_pushlightuserdata(L, NULL); lua_setfield(L, -2, "ADDO");
      lua_pop(L, 1);
    }
    if (owned)
    { if (O && O!=(DICOMDataObject *)(sd->DCO) && O!=sd->DDO) 
        delete O;
      if (A) 
      { for (unsigned int i=0; i<A->GetSize(); i++) delete (DICOMDataObject *)(A->Get(i));//bcb unsigned
        delete A;
      }
    }
    return 0;
  }

  // heap information for leak debugging
  static int luaheapinfo(lua_State *L)
  { lua_pushstring(L, heapinfo());
    return 1;
  }

// bcb 20120703 moved to hpp
//static void HTML(const char *string, ...);
//static int CGI(char *out, const char *name, const char *def);

  static int luaHTML(lua_State *L)
  { // char buf[1000];
    int n=lua_gettop(L);
    switch(n)
    { case 1: HTML(lua_tostring(L, 1)); break;
      case 2: HTML(lua_tostring(L, 1), lua_tostring(L, 2)); break;
      case 3: HTML(lua_tostring(L, 1), lua_tostring(L, 2), lua_tostring(L, 3)); break;
      case 4: HTML(lua_tostring(L, 1), lua_tostring(L, 2), lua_tostring(L, 3), lua_tostring(L, 4)); break;
      case 5: HTML(lua_tostring(L, 1), lua_tostring(L, 2), lua_tostring(L, 3), lua_tostring(L, 4), lua_tostring(L, 5)); break;
    }
    return 0;
  }
  static int luaCGI(lua_State *L)
  { char buf[1000];
    int n=lua_gettop(L);
    switch(n)
    { case 1: CGI(buf, lua_tostring(L, 1), ""); break;
      case 2: CGI(buf, lua_tostring(L, 1), lua_tostring(L,2)); break;
    }
    lua_pushstring(L, buf);
    return 1;
  }
  static int luagpps(lua_State *L)
  { char buf[LONG_NAME_LEN], *tempPtr;
    int n=lua_gettop(L);
    buf[0]=0;
    IniValue *iniValuePtr = IniValue::GetInstance();
    switch(n)
    { case 1: if((tempPtr = iniValuePtr->GetIniString(lua_tostring(L,1))) != NULL)
              { strcpylim(buf, tempPtr, sizeof(buf));
                free(tempPtr);
              }
      case 2: if((tempPtr = iniValuePtr->GetIniString(lua_tostring(L,2),lua_tostring(L,1))) != NULL)
              { strcpylim(buf, tempPtr, sizeof(buf));
                free(tempPtr);
              }
      case 3: if((tempPtr = iniValuePtr->GetIniString(lua_tostring(L,2),lua_tostring(L,1))) != NULL)
              { strcpylim(buf, tempPtr, sizeof(buf));
                free(tempPtr);
              }
              else strcpylim(buf, lua_tostring(L,3), sizeof(buf));
    }
    lua_pushstring(L, buf);
    return 1;
  }

//todone! (now read/write directly from dicom.ini): iniValuePtr->( Get, Write, Delete )
// Just keep track of the data type.
//IgnoreOutOfMemoryErrors, ImportExportDragAndDrop, TruncateFieldNames, ForwardAssociationRelease, 
//RetryForwardFailed, UIDPrefix, ForwardAssociationLevel, ForwardAssociationRefreshDelay
//ForwardAssociationCloseDelay, ForwardAssociationRelease, MaximumExportRetries,
//MaximumDelayedFetchForwardRetries, ziptime, NightlyCleanThreshhold, NightlyCleanThreshhold
//OverlapVirtualGet, SendUpperCaseAE, TCPIPTimeOut, PadAEWithZeros
//acrnema.map, dicom.sql, dgatesop.lst, dgate.dic, Database, Patient, Study[i], Series[i], Image[i]

  static int luaGlobalIndex(lua_State *L)
  { int *idata=NULL;
    char *sdata=NULL;
    char *tempPtr, Parameter[512];//, szRootSC[ROOTCONFIG_SZ];
    if((idata = getExtInt(lua_tostring(L,2))) != NULL)
    { lua_pushinteger(L, *idata);
      return 1;
    }
    getExtSLen(lua_tostring(L,2) , &sdata);
    if (sdata)
    { lua_pushstring (L,  sdata);
      return 1;
    }
    Parameter[0] = 0;
    if((tempPtr = IniValue::GetInstance()->GetIniString(lua_tostring(L,2))) != NULL)
    { strcpylim(Parameter, tempPtr, sizeof(Parameter));
      free(tempPtr);
    }
    if (*Parameter)
    { lua_pushstring (L,  Parameter);
      return 1;
    }
    return 0;
  }
  static int luaGlobalNewIndex(lua_State *L)
  { int slen;
    int *idata=NULL;
    char *sdata=NULL;
    char Parameter[512], script[1024], *tempPtr;
    if((idata = getExtInt(lua_tostring(L,2))) == NULL)// Not an int.
      slen = getExtSLen(lua_tostring(L,2) , &sdata);
    Parameter[0] = 0;
    if((tempPtr = IniValue::GetInstance()->GetIniString(lua_tostring(L,2))) != NULL)
    { strcpylim(Parameter, tempPtr, sizeof(Parameter));
      free(tempPtr);
    }
    if (*Parameter)
    { sprintf(script, "put_param:%s,%s", lua_tostring(L,2), lua_tostring(L,3));
      SendServerCommand("", script, 0, NULL);
      SendServerCommand("", "read_ini:", 0, NULL);
    }
    if (idata) *idata = lua_tointeger(L,3) & INT_MAX;
    if (sdata) strncpy(sdata, lua_tostring(L,3), slen-1), sdata[slen-1]=0;
    return 0;
  }

  static int luaAssociationIndex(lua_State *L)
  { struct scriptdata *sd = getsd(L);
    char *cdata=NULL;
    int *idata=NULL;
    BYTE buf[64]; buf[0]=0;
    if (strcmp(lua_tostring(L,2), "Calling")==0)     { cdata = (char *)buf; sd->PDU->GetLocalAddress (buf); }
    else if (strcmp(lua_tostring(L,2), "Called")==0) { cdata = (char *)buf; sd->PDU->GetRemoteAddress(buf); }
    else if (strcmp(lua_tostring(L,2), "ConnectedIP")==0) 
    { unsigned int ip = (unsigned int) sd->ConnectedIP; 
      sprintf((char *)buf, "%d.%d.%d.%d", ip&255, (ip>>8)&255, (ip>>16)&255, (ip>>24)&255);
      cdata = (char *)buf;
    }

    if (cdata) while (strlen(cdata)>0 && cdata[strlen(cdata)-1]==' ') cdata[strlen(cdata)-1] = 0;

    if (strcmp(lua_tostring(L,2), "Thread")==0) idata = &(sd->PDU->ThreadNum);
		        
    if (cdata) { lua_pushstring (L,  cdata);  return 1; }
    if (idata) { lua_pushinteger(L, *idata);  return 1; }
    return 0;
  }

  static int luaSeqClosure(lua_State *L)
  { const char *s = lua_tostring(L, lua_upvalueindex(1));
/*    if (strcmp(s, "Read" )==0)    return luareaddicom(L);
    if (strcmp(s, "Write")==0)    return luawritedicom(L);
    if (strcmp(s, "Dump" )==0)    return luawriteheader(L);
    if (strcmp(s, "SetVR")==0)    return luasetvr(L);
    if (strcmp(s, "GetVR")==0)    return luagetvr(L);
    if (strcmp(s, "GetPixel")==0) return luagetpixel(L);
    if (strcmp(s, "SetPixel")==0) return luasetpixel(L);
    if (strcmp(s, "GetRow")==0)   return luagetrow(L);
    if (strcmp(s, "SetRow")==0)   return luasetrow(L);
    if (strcmp(s, "GetColumn")==0)return luagetcolumn(L);
    if (strcmp(s, "SetColumn")==0)return luasetcolumn(L);
    if (strcmp(s, "GetImage")==0) return luagetimage(L);
    if (strcmp(s, "SetImage")==0) return luasetimage(L);
    if (strcmp(s, "Script")==0)   return luascript(L);

    if (strcmp(s, "new")==0)      return luanewdicomobject(L);
    if (strcmp(s, "newarray")==0) return luanewdicomarray(L);
    if (strcmp(s, "free")==0)     return luadeletedicomobject(L);*/
    switch (s[0])
    { case 'R': if (streq(&s[1], "ead"))     return luareaddicom(L);
                break;
      case 'W': if (streq(&s[1], "rite"))    return luawritedicom(L);
                break;
      case 'D': if (streq(&s[1], "ump"))     return luawriteheader(L);
                break;
      case 'S': if (streq(&s[1], "etVR"))    return luasetvr(L);
                if (streq(&s[1], "etRow"))   return luasetrow(L);
                if (streq(&s[1], "etColumn"))return luasetcolumn(L);
                if (streq(&s[1], "etPixel")) return luasetpixel(L);
                if (streq(&s[1], "cript"))   return luascript(L);
                if (streq(&s[1], "etImage")) return luasetimage(L);
                break;
      case 'G': if (streq(&s[1], "etVR"))    return luagetvr(L);
                if (streq(&s[1], "etPixel")) return luagetpixel(L);
                if (streq(&s[1], "etRow"))   return luagetrow(L);
                if (streq(&s[1], "etColumn"))return luagetcolumn(L);
                if (streq(&s[1], "etImage")) return luagetimage(L);
                break;
      case 'n': if (streq(&s[1], "ewarray")) return luanewdicomobject(L);
                if (streq(&s[1], "ew"))      return luagetimage(L);
      case 'f': if (streq(&s[1], "ree"))     return luanewdicomobject(L);
      default:  break;
    }
    return 0;
  }
  static int luaSeqnewindex(lua_State *L)
  { struct scriptdata *sd = getsd(L);
    char script[1024];

    Array < DICOMDataObject * > *A = NULL;
    DICOMDataObject *O = NULL;
    lua_getmetatable(L, 1);
      lua_getfield(L, -1, "ADDO"); A = (Array < DICOMDataObject * > *)lua_topointer(L, -1); lua_pop(L, 1);
      lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *)            lua_topointer(L, -1); lua_pop(L, 1);
    lua_pop(L, 1);

    // Data.Sequence, e.g., need to write into Data.Sequence.Item
    if (A)
    { if (A->GetSize()>0) 
        O=A->Get(0);
      else 
      { O = new DICOMDataObject; 
        A->Add(O);
      }
    }

    // write into Data.Storage
    if (O==sd->DDO && strcmp(lua_tostring(L,2), "Storage")==0)
    { sprintf(script, "storage %s", lua_tostring(L,3));
      CallImportConverterN(O, -1, sd->pszModality, sd->pszStationName, sd->pszSop, sd->patid, sd->PDU, sd->Storage, script);
    }
    // write into Data.Item, Data.Sequence.Item, or Data.Sequence[N].Item
    else if (O) 
    { // write table: creates empty! sequence
      if (lua_istable(L, 3))
        sprintf(script, "set %s to \"\"", lua_tostring(L,2));
      // write nil: deletes item
      else if (lua_isnil(L, 3))
        sprintf(script, "set %s to nil", lua_tostring(L,2));
       // write string: creates item
      else
        sprintf(script, "set %s to \"%s\"", lua_tostring(L,2), lua_tostring(L,3));
      CallImportConverterN(O, -1, sd->pszModality, sd->pszStationName, sd->pszSop, sd->patid, sd->PDU, sd->Storage, script);
    }

    return 0;
  }
  static int luaSeqindex(lua_State *L)
  { struct scriptdata *sd = getsd(L);
    Array < DICOMDataObject * > *A, *A2;
    DICOMDataObject *O, *O2;
    char buf[2000]; buf[0]=0;

    A2=NULL;//bcb Ininitailized warning.
    O2=NULL;//Ininitailized warning.

    lua_getmetatable(L, 1);
      lua_getfield(L, -1, "ADDO"); A = (Array < DICOMDataObject * > *)lua_topointer(L, -1); lua_pop(L, 1);
      lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *)            lua_topointer(L, -1); lua_pop(L, 1);
    lua_pop(L, 1);

    // Address sequence
    if (A) 
    { // push table for selected element (Data.Sequence[N], allow new elements in sequence)
      if (isdigit(*lua_tostring(L,2)))
      { lua_Number N = lua_tonumber(L, 2);// bcb lua_Number, was int
        if (N >A->GetSize())
        { O2=NULL;
          A2=NULL;
        }
        else 
        { if (N==A->GetSize()) 
	  { DICOMDataObject *D = new DICOMDataObject; A->Add(D);
	  }
          O2=A->Get(N);
          A2=NULL;
        }
      }
      // Address default element 0 (Data.Sequence.Item)
      else if (A->GetSize()>0)
	SearchDICOMObject(A->Get(0), lua_tostring(L,2), buf, &A2, &O2);
    }
    // Address Data.Storage
    // Address listed methods by name, e.g., Data:Write('c:\\x.dcm')
    // or get selected item (Data.Item, Data.Sequence[N].Item)
    else if (O) 
    { if (O==sd->DDO && strcmp(lua_tostring(L,2), "Storage")==0) 
      { lua_pushstring(L, sd->Storage);
        return 1;
      }
      else if (strstr("Write|Read|Dump|GetVR|SetVR|GetPixel|SetPixel|GetRow|SetRow|GetColumn|SetColumn|GetImage|SetImage|Script|new|newarray|free", lua_tostring(L,2))) 
      { lua_pushvalue(L, 2);
	lua_pushcclosure(L, luaSeqClosure, 1);
	return 1;
      }
      else 
	SearchDICOMObject(O, lua_tostring(L,2), buf, &A2, &O2);
    }
    else
    { A2=NULL; O2=NULL;
      if (strstr("new|newarray", lua_tostring(L,2))) 
      { lua_pushvalue(L, 2);
	lua_pushcclosure(L, luaSeqClosure, 1);
	return 1;
      }
    }

    // The item is a sequence or sequence element: return a new table (non-gc'd)
    if (A2 || O2)
    { luaCreateObject(L, O2, A2, FALSE);
      return 1;
    }

    // return a read string: empty string coded as \00\01; non-existing element as \00\00
    if (buf[0]) { lua_pushstring(L, buf); return 1; }
    if (buf[1]) { lua_pushstring(L, buf); return 1; }
    return 0;
  }
  static int luaSeqgc(lua_State *L)
  { struct scriptdata *sd = getsd(L);
    DICOMDataObject *O = NULL;
    Array < DICOMDataObject * > *A = NULL;
    lua_Integer owned;
    lua_getmetatable(L, 1);
      lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_getfield(L, -1, "ADDO");  A = (Array < DICOMDataObject * > *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_getfield(L, -1, "owned"); owned =                             lua_tointeger(L, -1); lua_pop(L, 1);
    lua_pop(L, 1);

    if (owned)
    { if (O && O!=(DICOMDataObject *)(sd->DCO) && O!=sd->DDO) delete O;
      if (A) 
      { for (unsigned int i=0; i<A->GetSize(); i++) delete (DICOMDataObject *)(A->Get(i));//bcb unsigned
        delete A;
      }
    }
    return 0;
  }

  static int luaSeqlen(lua_State *L)
  { struct scriptdata *sd = getsd(L); UNUSED_ARGUMENT(sd);
    DICOMDataObject *O = NULL;
    Array < DICOMDataObject * > *A = NULL;
    lua_getmetatable(L, 1);
      lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_getfield(L, -1, "ADDO");  A = (Array < DICOMDataObject * > *) lua_topointer(L, -1); lua_pop(L, 1);
    lua_pop(L, 1);

    if (O) lua_pushinteger(L, 1);
    else if (A) lua_pushinteger(L, A->GetSize());
    return 1;
  }

  static int luaSeqToString(lua_State *L)
  { struct scriptdata *sd = getsd(L); UNUSED_ARGUMENT(sd);
    char text[80];
    DICOMDataObject *O = NULL;
    Array < DICOMDataObject * > *A = NULL;
    lua_getmetatable(L, 1);
      lua_getfield(L, -1, "DDO");  O = (DICOMDataObject *) lua_topointer(L, -1); lua_pop(L, 1);
      lua_getfield(L, -1, "ADDO");  A = (Array < DICOMDataObject * > *) lua_topointer(L, -1); lua_pop(L, 1);
    lua_pop(L, 1);

    if (O) lua_pushstring(L, "Dicom Data Object");
    else if (A) 
    { sprintf(text, "Dicom sequence with %d entries", A->GetSize());
      lua_pushstring(L, text);
    }
    return 1;
  }

  static void luaCreateObject(lua_State *L, DICOMDataObject *pDDO, Array < DICOMDataObject *> *A, BOOL owned)
  { lua_newuserdata(L, 0);
    lua_createtable(L, 0, 0);
    lua_pushcfunction(L, luaSeqindex);    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luaSeqnewindex); lua_setfield(L, -2, "__newindex");
    lua_pushcfunction(L, luaSeqgc);       lua_setfield(L, -2, "__gc");
    lua_pushcfunction(L, luaSeqlen);      lua_setfield(L, -2, "__len");
    lua_pushcfunction(L, luaSeqToString); lua_setfield(L, -2, "__tostring");
    lua_pushlightuserdata(L, A);          lua_setfield(L, -2, "ADDO");
    lua_pushlightuserdata(L, pDDO);       lua_setfield(L, -2, "DDO");
    lua_pushinteger      (L, (int)owned); lua_setfield(L, -2, "owned");
    lua_setmetatable(L, -2);
  }
}
  
/*
* lpack.c
* a Lua library for packing and unpacking binary data
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br>
* 29 Jun 2007 19:27:20
* This code is hereby placed in the public domain.
* with contributions from Ignacio CastaÒo <castanyo@yahoo.es> and
* Roberto Ierusalimschy <roberto@inf.puc-rio.br>.
*/

#define	OP_ZSTRING	'z'		/* zero-terminated string */
#define	OP_BSTRING	'p'		/* string preceded by length byte */
#define	OP_WSTRING	'P'		/* string preceded by length word */
#define	OP_SSTRING	'a'		/* string preceded by length size_t */
#define	OP_STRING	'A'		/* string */
#define	OP_FLOAT	'f'		/* float */
#define	OP_DOUBLE	'd'		/* double */
#define	OP_NUMBER	'n'		/* Lua number */
#define	OP_CHAR		'c'		/* char */
#define	OP_BYTE		'b'		/* byte = unsigned char */
#define	OP_SHORT	'h'		/* short */
#define	OP_USHORT	'H'		/* unsigned short */
#define	OP_INT		'i'		/* int */
#define	OP_UINT		'I'		/* unsigned int */
#define	OP_LONG		'l'		/* long */
#define	OP_ULONG	'L'		/* unsigned long */
#define	OP_LITTLEENDIAN	'<'		/* little endian */
#define	OP_BIGENDIAN	'>'		/* big endian */
#define	OP_NATIVE	'='		/* native endian */

#include <ctype.h>
#include <string.h>

static void badcode(lua_State *L, int c)
{
 char s[]="bad code `?'";
 s[sizeof(s)-3]=c;
 luaL_argerror(L,1,s);
}

static int doendian(int c)
{
 int x=1;
 int e=*(char*)&x;
 if (c==OP_LITTLEENDIAN) return !e;
 if (c==OP_BIGENDIAN) return e;
 if (c==OP_NATIVE) return 0;
 return 0;
}

static void doswap(int swap,void *p, size_t n)
{
 if (swap)
 {char *a=(char *)p;
  int i,j;
  for (i=0, j=(n-1) & INT_MAX, n=n/2; n--; i++, j--)
  {
   char t=a[i]; a[i]=a[j]; a[j]=t;
  }
 }
}
//Size warnings: changed m from int to size_t.
#define UNPACKNUMBER(OP,T)		\
   case OP:				\
   {					\
    T a;				\
    size_t m=sizeof(a);			\
    if (i+m>len) goto done;		\
    memcpy(&a,s+i,m);			\
    i+=m;				\
    doswap(swap,&a,m);			\
    lua_pushnumber(L,(lua_Number)a);	\
    ++n;				\
    break;				\
   }

#define UNPACKSTRING(OP,T)		\
   case OP:				\
   {					\
    T l;				\
    size_t m=sizeof(l);			\
    if (i+m>len) goto done;		\
    memcpy(&l,s+i,m);			\
    doswap(swap,&l,m);			\
    if (i+m+l>len) goto done;		\
    i+=m;				\
    lua_pushlstring(L,s+i,l);		\
    i+=l;				\
    ++n;				\
    break;				\
   }

static int l_unpack(lua_State *L) 		/** unpack(s,f,[init]) */
{
 size_t len;
 const char *s=luaL_checklstring(L,1,&len);
 const char *f=luaL_checkstring(L,2);
 int i=luaL_optnumber(L,3,1)-1;
 int n=0;
 int swap=0;
 lua_pushnil(L);
 while (*f)
 {
  int c=*f++;
  int N=1;
  if (isdigit(*f)) 
  {
   N=0;
   while (isdigit(*f)) N=10*N+(*f++)-'0';
   if (N==0 && c==OP_STRING) { lua_pushliteral(L,""); ++n; }
  }
  while (N--) switch (c)
  {
   case OP_LITTLEENDIAN:
   case OP_BIGENDIAN:
   case OP_NATIVE:
   {
    swap=doendian(c);
    N=0;
    break;
   }
   case OP_STRING:
   {
    ++N;
    if ((size_t)(i+N)>len) goto done;
    lua_pushlstring(L,s+i,N);
    i+=N;
    ++n;
    N=0;
    break;
   }
   case OP_ZSTRING:
   {
    size_t l;
    if ((size_t)i>=len) goto done;
    l=strlen(s+i);
    lua_pushlstring(L,s+i,l);
    i+=l+1;
    ++n;
    break;
   }
   UNPACKSTRING(OP_BSTRING, unsigned char)
   UNPACKSTRING(OP_WSTRING, unsigned short)
   UNPACKSTRING(OP_SSTRING, size_t)
   UNPACKNUMBER(OP_NUMBER, lua_Number)
   UNPACKNUMBER(OP_DOUBLE, double)
   UNPACKNUMBER(OP_FLOAT, float)
   UNPACKNUMBER(OP_CHAR, char)
   UNPACKNUMBER(OP_BYTE, unsigned char)
   UNPACKNUMBER(OP_SHORT, short)
   UNPACKNUMBER(OP_USHORT, unsigned short)
   UNPACKNUMBER(OP_INT, int)
   UNPACKNUMBER(OP_UINT, unsigned int)
   UNPACKNUMBER(OP_LONG, long)
   UNPACKNUMBER(OP_ULONG, unsigned long)
   case ' ': case ',':
    break;
   default:
    badcode(L,c);
    break;
  }
 }
done:
 lua_pushnumber(L,i+1);
 lua_replace(L,-n-2);
 return n+1;
}

#define PACKNUMBER(OP,T)			\
   case OP:					\
   {						\
    T a=(T)luaL_checknumber(L,i++);		\
    doswap(swap,(void *)&a,sizeof(a));			\
    luaL_addlstring(&b,(const char *)&a,sizeof(a));	\
    break;					\
   }

#define PACKSTRING(OP,T)			\
   case OP:					\
   {						\
    size_t l;					\
    const char *a=luaL_checklstring(L,i++,&l);	\
    T ll=(T)l;					\
    doswap(swap,(void *)&ll,sizeof(ll));		\
    luaL_addlstring(&b,(const char *)&ll,sizeof(ll));	\
    luaL_addlstring(&b,a,l);			\
    break;					\
   }

static int l_pack(lua_State *L) 		/** pack(f,...) */
{
 int i=2;
 const char *f=luaL_checkstring(L,1);
 int swap=0;
 luaL_Buffer b;
 luaL_buffinit(L,&b);
 while (*f)
 {
  int c=*f++;
  int N=1;
  if (isdigit(*f)) 
  {
   N=0;
   while (isdigit(*f)) N=10*N+(*f++)-'0';
  }
  while (N--) switch (c)
  {
   case OP_LITTLEENDIAN:
   case OP_BIGENDIAN:
   case OP_NATIVE:
   {
    swap=doendian(c);
    N=0;
    break;
   }
   case OP_STRING:
   case OP_ZSTRING:
   {
    size_t l;
    const char *a=luaL_checklstring(L,i++,&l);
    luaL_addlstring(&b,a,l+(c==OP_ZSTRING));
    break;
   }
   PACKSTRING(OP_BSTRING, unsigned char)
   PACKSTRING(OP_WSTRING, unsigned short)
   PACKSTRING(OP_SSTRING, size_t)
   PACKNUMBER(OP_NUMBER, lua_Number)
   PACKNUMBER(OP_DOUBLE, double)
   PACKNUMBER(OP_FLOAT, float)
   PACKNUMBER(OP_CHAR, char)
   PACKNUMBER(OP_BYTE, unsigned char)
   PACKNUMBER(OP_SHORT, short)
   PACKNUMBER(OP_USHORT, unsigned short)
   PACKNUMBER(OP_INT, int)
   PACKNUMBER(OP_UINT, unsigned int)
   PACKNUMBER(OP_LONG, long)
   PACKNUMBER(OP_ULONG, unsigned long)
   case ' ': case ',':
    break;
   default:
    badcode(L,c);
    break;
  }
 }
 luaL_pushresult(&b);
 return 1;
}

static const luaL_Reg R[] = //bcb was luaL_reg
{
	{"pack",	l_pack},
	{"unpack",	l_unpack},
	{NULL,	NULL}
};

int luaopen_pack(lua_State *L)
{
#ifdef USE_GLOBALS
 lua_register(L,"bpack",l_pack);
 lua_register(L,"bunpack",l_unpack);
#else
#ifdef LUA_5_2 // bcb Got this from the web, is it right?
 luaL_newlib (L, R);
 lua_setglobal(L,LUA_STRLIBNAME);
#else
 luaL_openlib(L, LUA_STRLIBNAME, R, 0);
#endif
#endif
 return 0;
}

// endof pack library

//extern "C" int luaopen_socket_core(lua_State *L);//bcb? Used or not used?

const char *do_lua(lua_State **L, const char *cmd, struct scriptdata *sd)//bcb made const
{ if (!*L) 
  {
#ifdef LUA_5_2
    *L = luaL_newstate();
#else
    *L = lua_open();
#endif
    luaL_openlibs (*L);

    lua_register      (*L, "print",         luaprint);
    lua_register      (*L, "script",        luascript);
    lua_register      (*L, "get_amap",      luaget_amap);
    lua_register      (*L, "get_sqldef",    luaget_sqldef);
    lua_register      (*L, "dbquery",       luadbquery);
    lua_register      (*L, "sql",           luasql);
    lua_register      (*L, "dicomquery",    luadicomquery);
    lua_register      (*L, "dicommove",     luadicommove);
    lua_register      (*L, "dicomdelete",   luadicomdelete);
    lua_register      (*L, "debuglog",      luadebuglog);
    lua_register      (*L, "servercommand", luaservercommand);
    lua_register      (*L, "setpixel",      luasetpixel);
    lua_register      (*L, "getpixel",      luagetpixel);
    lua_register      (*L, "getrow",        luagetrow);
    lua_register      (*L, "setrow",        luasetrow);
    lua_register      (*L, "getcolumn",     luagetcolumn);
    lua_register      (*L, "setcolumn",     luasetcolumn);
    lua_register      (*L, "getimage",      luagetimage);
    lua_register      (*L, "setimage",      luasetimage);
    lua_register      (*L, "readdicom",     luareaddicom);
    lua_register      (*L, "writedicom",    luawritedicom);
    lua_register      (*L, "writeheader",   luawriteheader);
    lua_register      (*L, "newdicomobject",luanewdicomobject);
    lua_register      (*L, "newdicomarray", luanewdicomarray);
    lua_register      (*L, "deletedicomobject",luadeletedicomobject);
    lua_register      (*L, "getvr",         luagetvr);
    lua_register      (*L, "setvr",         luasetvr);
    lua_register      (*L, "HTML",          luaHTML);
    lua_register      (*L, "CGI",           luaCGI);
    lua_register      (*L, "gpps",          luagpps);
    lua_register      (*L, "heapinfo",      luaheapinfo);
    lua_register      (*L, "dictionary",    luadictionary);
    lua_register      (*L, "changeuid",     luachangeuid);
    lua_register      (*L, "changeuidback", luachangeuidback);
    lua_register      (*L, "genuid",        luagenuid);
    lua_register      (*L, "crc",           luacrc);
    
    lua_createtable   (*L, 0, 0); 
    lua_createtable   (*L, 0, 0);
    lua_pushcfunction (*L, luaGlobalIndex);    lua_setfield (*L, -2, "__index");
    lua_pushcfunction (*L, luaGlobalNewIndex); lua_setfield (*L, -2, "__newindex");
    lua_setmetatable  (*L, -2);
    lua_setglobal     (*L, "Global");

    lua_createtable(*L, 0, 0); 
    lua_createtable(*L, 0, 0);
    lua_pushcfunction(*L, luaAssociationIndex); lua_setfield(*L, -2, "__index");
    lua_setmetatable(*L, -2);
    lua_setglobal(*L, "Association");  
    
//    lua_getfield(*L, LUA_GLOBALSINDEX, "package");//bcb 5.1 only
    lua_getglobal(*L, "package");// bcb Both 5.1 and 5.2
    lua_getfield(*L, -1, "preload");
//#ifndef UNIX
//    lua_pushcfunction(*L, luaopen_socket_core);//bcb? Used or not used?
//    lua_setfield(*L, -2, "socket.core");
//#endif
    lua_pushcfunction(*L, luaopen_pack);
    lua_setfield(*L, -2, "pack");
  }

  if (!sd->DDO) 
  { sd->DDO = dummyddo = new(DICOMDataObject);
  }

  luaCreateObject(*L, (DICOMDataObject *)sd->DCO, NULL, FALSE); lua_setglobal(*L, "Command");  
  luaCreateObject(*L,                    sd->DDO, NULL, FALSE); lua_setglobal(*L, "Data");
  luaCreateObject(*L,                    NULL,    NULL, FALSE); lua_setglobal(*L, "DicomObject");

  lua_pushlightuserdata(*L, sd); 
  lua_setglobal(*L, "scriptdata");

  if (cmd[0]) 
  { if (luaL_loadstring(*L, cmd)) 
    { OperatorConsole.printf("*** lua syntax error %s in '%s'\n", lua_tostring(*L, -1), cmd);
      if (sd->DDO == dummyddo) 
      { sd->DDO = NULL;
        delete dummyddo;
      }
      lua_pop(*L, 1);
      return NULL;
    } 
    else 
    { 
#ifdef LUA_5_2
      if (lua_pcallk(*L, 0, 1, 0, 0, NULL)) 
#else        
      if (lua_pcall(*L, 0, 1, 0))
#endif
      { OperatorConsole.printf("*** lua run error %s in '%s'\n", lua_tostring(*L, -1), cmd);
        if (sd->DDO == dummyddo) 
        { sd->DDO = NULL;
          delete dummyddo;
	}
        lua_pop(*L, 1);
        return NULL;
      }
      else
      { if (sd->DDO == dummyddo) 
        { sd->DDO = NULL;
          delete dummyddo;
	}
        if (lua_isstring(*L, -1))
          return lua_tostring(*L, -1);
        else
	  lua_pop(*L, 1);
      }
    }
  }
  return NULL;  
}

void lua_setvar(ExtendedPDU_Service *pdu, char *name, char *value)
{ struct scriptdata sd = {pdu, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, 0, 0};
  char nada[1];//bcb for warning: Conversion from string literal to 'char *' is deprecated.
  nada[0] = 0;
  do_lua(&(pdu->L), nada, &sd);
  if (value)
    lua_pushstring(pdu->L, value);
  else
    lua_pushstring(pdu->L, "NULL");
  lua_setglobal (pdu->L, name);
}

void lua_getstring(ExtendedPDU_Service *pdu, DICOMCommandObject *dco, DICOMDataObject *ddo, char *cmd, char *result)
{ char script[1000];
  struct scriptdata lsd1 = {pdu, dco, ddo, -1, NULL, NULL, NULL, NULL, NULL, 0, 0};
  strcpy(script, "return ");
  strcat(script, cmd);
  strcpy(result, do_lua(&(pdu->L), script, &lsd1));
}
#ifndef UNIX

#include <stdio.h>
#include <malloc.h>

char *heapinfo( void )
{ UINT32 stot=0, sn=0, mtot=0, mn=0, ltot=0, ln=0;
  static char s[256];

  HeapLock(GetProcessHeap());

  _HEAPINFO hinfo;
  int heapstatus;
  hinfo._pentry = NULL;
  while( ( heapstatus = _heapwalk( &hinfo ) ) == _HEAPOK )
  { if (hinfo._useflag == _USEDENTRY)
    { if (hinfo._size < 256) 
      { sn++, stot += hinfo._size;
        //if (hinfo._size == 100)
        //{ FILE *f;
        //  f = fopen("c:\\debug.bin", "ab");
        //  fwrite(hinfo._pentry, hinfo._size, 1, f);
        //  fclose(f);
        //}
      }
      else if (hinfo._size < 4096) mn++, mtot += hinfo._size;
           else ln++, ltot += hinfo._size;
    }
  }

  HeapUnlock(GetProcessHeap());

  sprintf(s, "%d small (%d); %d medium (%d) %d large (%d) ", sn, stot, mn, mtot, ln, ltot);

  switch( heapstatus )
  {
    case _HEAPEMPTY:
      strcat(s, "OK - empty heap\n" );
      break;
    case _HEAPEND:
      strcat(s, "OK - end of heap\n" );
      break;
    case _HEAPBADPTR:
      strcat(s, "*** ERROR - bad heap ptr\n" );
      break;
    case _HEAPBADBEGIN:
      strcat(s, "*** ERROR - bad heap start\n" );
      break;
    case _HEAPBADNODE:
      strcat(s, "*** ERROR - bad heap node\n" );
      break;
  }
  return s;
}

#else

char *heapinfo( void )
{ static char s[] = "not available";
  return s;
}

#endif //UNIX
