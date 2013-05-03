#ifndef	_DGATE_H_
#	define	_DGATE_H_

/*
MvH 19980705: Added CACHEDevices and JUKEBOXDevices, FindPhysicalDevice
ljz 19990108: Added LoadImplicitLittleEndianFile and NKI PrivateCompession algorithms
MvH 19990109: Regen has extra parameter
ljz 19990317: Changed parameters of LoadImplicitLittleEndianFile
ljz 19991117: Added parameter FileCompressMode to prototype of nki_private_compress
ljz 20000629: Added TroubleLogFile and UserLogFile
mvh 20010415: Added KeepImages flag to RemoveFromPacs: clear from DB only
              Added SubDir parameter to regen to allow regen of one directory only
mvh 20010416: Added ChangeUID routine and RegenFile - to allow modification of images
mvh 20010429: Changed decompressor: now has extra parameter
mvh 20020529: InitializeTables now has mode parameter (0=normal, 1=simple)
mvh 20021016: added patid to GetFilename routines
mvh 20021017: Added NeedPack here
mvh 20021018: GenerateFileName has NoKill option (for interactive dgate tasks)
ljz 20030120: Added prototype of FreeDeviceTables
mvh 20030701: compression parameter for amap
mvh 20030703: Added prototypes of recompression functions + ArchiveCompression
mvh 20030706: Optional filename parameter for recompress
mvh 20030706: Export VRType for implicit little endian support
mvh 20030921: Added DEVICE_TYPE_MIRROR
mvh 20040401: Added Changed and ActualMode flags to compress routines
mvh 20040626: Added study and series UID to getfilename interface
mvh 20040930: Adapted return type of SetString; added maxlen to BuildSearchString
mvh 20041013: Added MAXQUERYLENGTH
mvh 20041029: Added MergeUIDs
mvh 20041108: Added Syntax input to GenerateFileName
mvh 20050108: Adapted for linux compile
mvh 20050109: Added configurable TCPIPTimeOut
mvh 20050129: Added optional FILE to DumpVR, added CheckFreeStoreOnMIRRORDevice
mvh 20050401: Added QueryOnModalityWorkList, WorkListTableName, WorkListDB
mvh 20050404: Added DT_START/ENDSEQUENCE to code sequence in WorkList table
mvh 20050902: Made space for HL7Tag in DBEntry
mvh 20051229: Added iDepth to DumpVR
mvh 20060317: Added called and calling to GenerateFilename
mvh 20060324: Added StripGroup2 option to recompress
mvh 20060628: AddToDatabase has JustAdd parameter: skip one unnecessary query
mvh 20060702: Pass DB to GenerateFilename to avoid zillions of db open and closes
mvh 20070122: Added MIRRORDevices
mvh 20070201: Added DebugLevel
mvh 20070207: Added MakeTableString
mvh 20080818: DbaseIII check now uses DB flags, not PATHSEP in datasource name
              Added DT_MSTR
jf  20090616: Include file stuff
bcb 20091231: Changed char* to const char* for gcc4.2 warnings
mvh 20100111: Merged
mvh 20100123: Added DT_FL and DT_FD
mvh 20100125: Removed linux warning
bcb 20100309: Changed SQLLength to unsigned int
mvh 20100703: Merged
mvh 20110119: Moved two functions to the correct place
bcb 20120703: Removed WHEDGE, cleaned up prototypes, moved includes here.
bcb 20120710: Split up dgate into dgate, dgatecgi, dgatehl7 and nkiqrsop into nkiqrsop, jpegconv; moved parseArgs to parse.
bcb 20120820: Added ACRNemaMap class and IP wild carding
bcb 20130226: Replaced gpps with IniValue class.  Removed all globals now in IniValue class. Version to 1.4.18a.
*/

#define DGATE_VERSION "1.4.18a"

#ifndef UNIX
#	include	<direct.h>
#else // UNIX
#include "wintypes.hpp"	// windows data types
#endif // UNIX

#define bool BOOL
#define true TRUE
#define false FALSE

#ifndef	UNIX //WIN32
#	define	PATHSEPCHAR	'\\'
#else
#	define	PATHSEPCHAR	'/'
#endif

#define	MAXQUERYLENGTH 310000
#   include <stdlib.h>
#ifndef UNIX
#	include	<windows.h>
#endif


#   include "dbsql.hpp"
#   include "nkiqrsop.hpp"
#   include "amap.hpp"

#ifndef UNIX  	// Win32...
#	include	<process.h>
#	define THREAD_RETURN_FALSE FALSE
#	define THREAD_RETURN_TRUE TRUE
typedef BOOL ThreadRoutineType;
typedef int ThreadRoutineArgType;
#else		// UNIX...
#	include <pthread.h>
#	include <unistd.h>
//#	include <stdlib.h>
#	include <string.h>
#	include <signal.h>
#	include <sys/types.h>
#	include <sys/stat.h>
//#	include <fstream.h>
#	include "npipe.hpp"
#ifndef PATH_MAX
#   define PATH_MAX FILENAME_MAX
#endif //PATH_MAX
#	define THREAD_RETURN_FALSE ((void *)NULL)
#	define THREAD_RETURN_TRUE ((void *)1)
//#	define closesocket(s) close(s)
#	define CloseHandle(h)
#	define eof(h) FALSE
typedef void *	ThreadRoutineType;
typedef void * ThreadRoutineArgType;
#endif //UNIX

static char ServerCommandAddress[64] = "127.0.0.1";

struct scriptdata
{ ExtendedPDU_Service *PDU;
    DICOMCommandObject *DCO;
    DICOMDataObject *DDO;
    int  N;
    char *pszModality;
    char *pszStationName;
    char *pszSop;
    char *patid;
    char *Storage;
    int  rc;
    unsigned int ConnectedIP;
};// sd1;

enum
	{
	DT_STR	=1,
	DT_DATE,
	DT_UINT16,
	DT_UINT32,
	DT_UI,
	DT_FL,
	DT_FD,
	DT_STARTSEQUENCE,
	DT_ENDSEQUENCE,
	DT_MSTR
	};

extern	int	DebugLevel;
extern	DBENTRY	*PatientDB, *StudyDB, *SeriesDB, *ImageDB, *WorkListDB;
extern  int 	NeedPack;

extern  RTC	VRType;

#	define	DEVICE_TYPE_MAG		0x01
#	define	DEVICE_TYPE_MOP		0x02
#	define	DEVICE_TYPE_CDR		0x04
#	define	DEVICE_TYPE_MIRROR	0x08
#	define	DEVICE_TYPE_NOKILL	0x100

#	define	DEVICE_OPTI			0x00
#	define	DEVICE_0			0x10
#	define	DEVICE_1			0x20
#	define	DEVICE_2			0x30
#	define	DEVICE_X(xxx)		((xxx+1)<<4)

#ifdef UNIX
//DUCKHEAD92 BEGIN
#define stricmp(s1, s2) strcasecmp(s1, s2)
#define memicmp(s1, s2, n) strncasecmp(s1, s2, n)

/* These next lines are scattered all around the place, 
   should they be WIN32 only? */
#define O_BINARY 0              /* LINE UNIX ONLY? */
#define _SH_DENYRW      0x10    /* deny read/write mode */
#define _SH_DENYWR      0x20    /* deny write mode */
#define _SH_DENYRD      0x30    /* deny read mode */
#define _SH_DENYNO      0x40    /* deny none mode */
#define SH_DENYNO       _SH_DENYNO
#define sopen(a, b, c, d) open(a, b) /* LINE UNIX ONLY? */
#define WINAPI /* LINE UNIX ONLY */
#define DeleteCriticalSection(a) pthread_mutex_destroy(a);  /* LINE UNIX ONLY */
void strupr(char *s);
void strlwr(char *s);
#define	closesocket(xxx)	close(xxx) /* LINE UNIX ONLY? */
#define strnicmp(s1, s2, n) strncasecmp(s1, s2, n)
//DUCKHEAD92 END
#endif

// Special RunTime-Classing Storage mechanism.  Used for "Generic"
// Unknown outgoing C-Store requests from the Query / Retrieve Engine
// Defined here because needed in forward code

class	RunTimeClassStorage	:
public	StandardStorage
{
    UID		MyUID;
public:
    // is not used anymore and would not be thread safe (db's may not be shared)
    // Database	DB;
    BOOL	uGetUID(UID &uid) { return ( GetUID(uid) ); };
    inline	BOOL Read ( PDU_Service *PDU, DICOMCommandObject *DCO,
                       DICOMDataObject *DDO )
    { return ( StandardStorage :: Read ( PDU, DCO, DDO ) ); };
    inline	BOOL Write ( PDU_Service *PDU, DICOMDataObject *DDO)
    { return ( StandardStorage :: Write ( PDU, DDO ) ); };
    BOOL	GetUID(UID	&);
    BOOL	SetUID(UID	&);
    BOOL	SetUID(VR	*);
    BOOL	SetUID(DICOMDataObject	*);
#ifdef __GNUC__
    RunTimeClassStorage():MyUID() {};
#endif
};

// Query / Retrieve Engine

class	DriverApp
{
public:
    virtual	BOOL	ServerChild ( int, unsigned int ) = 0;
public:
    BOOL		ServerChildThread ( int, unsigned int );
    BOOL		Server ( BYTE * );
    int		ChildSocketfd;
    volatile int	Lock;
//    char		ConnectedIPStr[DOT_FOUR_LEN];
    unsigned int        ConnectedIP;
#ifndef __GNUC__
    DriverApp () { Lock = 0; ConnectedIP = 0; /*ConnectedIPStr[0] = 0;*/ };
#else //The GCC way
        DriverApp ():ChildSocketfd(0), Lock(0), ConnectedIP(0) {};/*, ConnectedIPStr() { ConnectedIPStr[0] = 0; };*/
    virtual ~DriverApp() {};
#endif
};
class	MyPatientRootQuery	:
public	PatientRootQuery
{
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	QueryResultScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    int	RecordsFound;
#ifdef __GNUC__
    MyPatientRootQuery():RecordsFound(0) {};
#endif
};

class	MyStudyRootQuery	:
public	StudyRootQuery
{
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	QueryResultScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    int	RecordsFound;
#ifdef __GNUC__
    MyStudyRootQuery():RecordsFound(0) {};
#endif
};

class	MyPatientStudyOnlyQuery	:
public	PatientStudyOnlyQuery
{
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	QueryResultScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    int	RecordsFound;
#ifdef __GNUC__
    MyPatientStudyOnlyQuery():RecordsFound(0) {};
#endif
};


class	MyModalityWorkListQuery	:
public	ModalityWorkListQuery
{
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	QueryResultScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    int	RecordsFound;
#ifdef __GNUC__
    MyModalityWorkListQuery():RecordsFound(0) {};
#endif
};


class	MyPatientRootRetrieve	:
public	PatientRootRetrieve
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyPatientRootRetrieve():RTCStorage() {};
#endif
};

class	MyPatientRootRetrieveNKI	:
public	PatientRootRetrieveNKI
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	QueryResultScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **,
                        DICOMCommandObject	   *,
                        Array < DICOMDataObject *> *,
                        void 		*);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyPatientRootRetrieveNKI():RTCStorage() {};
#endif
};

class	MyPatientRootRetrieveGeneric	:
public	PatientRootRetrieveGeneric
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **,
                        DICOMCommandObject	   *,
                        Array < DICOMDataObject *> *,
                        void 		*);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyPatientRootRetrieveGeneric():RTCStorage() {};
#endif
};

class	MyStudyRootRetrieve	:
public	StudyRootRetrieve
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyStudyRootRetrieve():RTCStorage() {};
#endif
};

class	MyStudyRootRetrieveNKI	:
public	StudyRootRetrieveNKI
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **,
                        DICOMCommandObject	   *,
                        Array < DICOMDataObject *> *,
                        void 		*);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyStudyRootRetrieveNKI():RTCStorage() {};
#endif
};

class	MyStudyRootRetrieveGeneric	:
public	StudyRootRetrieveGeneric
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **,
                        DICOMCommandObject	   *,
                        Array < DICOMDataObject *> *,
                        void 		*);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyStudyRootRetrieveGeneric():RTCStorage() {};
#endif
};

class	MyPatientStudyOnlyRetrieve	:
public	PatientStudyOnlyRetrieve
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyPatientStudyOnlyRetrieve():RTCStorage() {};
#endif
};

class	MyPatientStudyOnlyRetrieveNKI	:
public	PatientStudyOnlyRetrieveNKI
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **,
                        DICOMCommandObject	   *,
                        Array < DICOMDataObject *> *,
                        void 		*);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyPatientStudyOnlyRetrieveNKI():RTCStorage() {};
#endif
};

class	MyPatientStudyOnlyRetrieveGeneric	:
public	PatientStudyOnlyRetrieveGeneric
{
    RunTimeClassStorage	RTCStorage;
    
public:
    BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO);
    BOOL	SearchOn (DICOMDataObject *, Array < DICOMDataObject *> *);
    BOOL	CallBack (DICOMCommandObject *, DICOMDataObject *);
    BOOL	RetrieveOn ( DICOMDataObject *, DICOMDataObject **,
                        StandardStorage **,
                        DICOMCommandObject	   *,
                        Array < DICOMDataObject *> *,
                        void 		*);
    BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE *, BYTE * );
#ifdef __GNUC__
    MyPatientStudyOnlyRetrieveGeneric():RTCStorage() {};
#endif
};

class	StorageApp	:
public	DriverApp
{
    // MyUnknownStorage		SOPUnknownStorage;
    //Verification				SOPVerification;
    //MyPatientRootQuery			SOPPatientRootQuery;
    //MyPatientRootRetrieve			SOPPatientRootRetrieve;
    //MyPatientRootRetrieveNKI		SOPPatientRootRetrieveNKI;
    //MyPatientRootRetrieveGeneric		SOPPatientRootRetrieveGeneric;
    
    //MyStudyRootQuery			SOPStudyRootQuery;
    //MyStudyRootRetrieve			SOPStudyRootRetrieve;
    //MyStudyRootRetrieveNKI			SOPStudyRootRetrieveNKI;
    //MyStudyRootRetrieveGeneric		SOPStudyRootRetrieveGeneric;
    
    //MyPatientStudyOnlyQuery			SOPPatientStudyOnlyQuery;
    //MyPatientStudyOnlyRetrieve		SOPPatientStudyOnlyRetrieve;
    //MyPatientStudyOnlyRetrieveNKI		SOPPatientStudyOnlyRetrieveNKI;
    //MyPatientStudyOnlyRetrieveGeneric	SOPPatientStudyOnlyRetrieveGeneric;
    
    //MyModalityWorkListQuery			SOPModalityWorkListQuery;
public:
    BOOL	ServerChild ( int, unsigned int );
    void FailSafeStorage(CheckedPDU_Service *PDU);
    BOOL PrinterSupport( CheckedPDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *PrintData[]);
    BOOL StorageCommitmentSupport( CheckedPDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject **CommitData);
};

/************* prototypes  ********/
#ifdef UNIX
void Sleep(int h);
#endif
//void ConfigDgate(void);
void MyDcmError(int error, const char *message, int count);
void StatusDisplay(FILE *f);
void IARQ (AAssociateRQ	&ARQ, BOOL showall);
BOOL	IsAlpha(BYTE	c);
BOOL	IsDataAlpha(UINT	Size, BYTE	*data);
BOOL	PrintUDATA(UINT	Size, void *data, UINT16 TypeCode, char *dest);
BOOL	DumpVR(VR	*, FILE * f = NULL, int iDepth = 0);
BOOL    NonDestructiveDumpDICOMObject(DICOMObject *DO, FILE *f = NULL, int iDepth=0);
BOOL    PrintFreeSpace();
BOOL    PrintAMap();
const char *SQLTypeSymName(int SQLType);
const char *DICOMTypeSymName(int DICOMType);
BOOL    DumpDD(DBENTRY	*DBE);
BOOL    PrintKFactorFile();
BOOL    DeletePatient(char *PID, BOOL KeepImages);
BOOL    DeleteStudy(char *ID, BOOL KeepImages);
BOOL    DeleteStudies(char *date, BOOL KeepImages);
BOOL    DeleteSeries(char *ID, BOOL KeepImages);
BOOL    DeleteImage(char *ID, BOOL KeepImages);
DICOMDataObject *LoadForGUI(char *filename);
DICOMDataObject *LoadForBridge(char *stsesop, char *ae);
BOOL    DeleteImageFile(char *filename, BOOL KeepImages);
BOOL    GetImageFileUID(char *filename, char *UID);
//BOOL ChangeUIDinDDO(DICOMDataObject *pDDO, int group, int element, char *name);
//BOOL ChangeVRinDDO(DICOMDataObject *pDDO, int group, int element, char *text);
BOOL    MergeUIDofDDO(DICOMDataObject *pDDO, const char *type, const char *Reason);
BOOL	dgate_IsDirectory(char *TempPath);
BOOL    LoadAndDeleteDir(char *dir, char *NewPatid, ExtendedPDU_Service *PDU);
BOOL    AddImageFile(char *filename, char *NewPatid, ExtendedPDU_Service *PDU);
BOOL    GenUID(char	*oString);
BOOL    ModifyPATIDofImageFile(char *filename, char *NewPATID, BOOL DelFile, char *script, ExtendedPDU_Service *PDU);
BOOL    ModifyImageFile(char *filename, char *script, ExtendedPDU_Service *PDU);
int	ModifyData(char *pat, char *study, char *series, char *script, ExtendedPDU_Service *PDU);
BOOL    AttachFile(char *filename, char *script, char *rFilename, ExtendedPDU_Service *PDU);
BOOL    AttachRTPLANToRTSTRUCT(char *planfilename, char *structfilename, ExtendedPDU_Service *PDU);
BOOL    AttachAnyToPatient(char *anyfilename, char *samplefilename, ExtendedPDU_Service *PDU);
BOOL    AttachAnyToStudy(char *anyfilename, char *samplefilename, ExtendedPDU_Service *PDU);
BOOL    AttachAnyToSeries(char *anyfilename, char *samplefilename, ExtendedPDU_Service *PDU);
BOOL    BackgroundExec(char *ProcessBinary, char *Args);
int TestFilter(char *query, char *sop, int maxname, char *patid=NULL);
BOOL    DICOM2SQLValue (char *s);
#ifndef __SearchDICOMObject
#define __SearchDICOMObject
int SearchDICOMObject(DICOMObject *DDO, const char *desc, char *result, Array < DICOMDataObject * > **A = NULL, DICOMDataObject **O = NULL);
#endif
BOOL    CallExportConverterN(char *pszFileName, int N, char *pszModality, char *pszStationName, char *pszSop, char *patid, ExtendedPDU_Service *PDU, char *ForwardLastUID, char *calling, char *called);
//void  CallExportConverters(char *pszFileName, char *pszModality, char *pszStationName, char *pszSop, char *patid, char *calling, char *called);
int CallImportConverterN(DICOMDataObject *DDO, int N, char *pszModality, char *pszStationName, char *pszSop, char *patid, ExtendedPDU_Service *PDU, char *Storage, char *Script);
int CallImportConverters(DICOMDataObject *DDO, char *pszModality, char *pszStationName, char *pszSop, char *patid, ExtendedPDU_Service *PDU, char *Storage);
struct conquest_queue *new_queue(int num, int size, int delay, BOOL (*process)(char *, ExtendedPDU_Service *PDU, char *), ExtendedPDU_Service *PDU, int maxfails);
void    into_queue(struct conquest_queue *q, char *in);
BOOL    into_queue_unique(struct conquest_queue *q, char *in, int numunique);
BOOL    exportprocessN(char *data, ExtendedPDU_Service *PDU, char *t);
void    reset_queue_fails(int N);
void    export_queueN(struct conquest_queue **q, char *pszFileName, int N, char *pszModality, char *pszStationName, char *pszSop, char *patid, char *calling, char *called);
void    QueueExportConverters(char *pszFileName, char *pszModality, char *pszStationName, char *pszSop, char *patid, char *calling, char *called);
BOOL    copyprocess(char *data, ExtendedPDU_Service *PDU, char *dum);
void    mirrorcopy_queue(char *pszFileName, char *pszDestination);
BOOL    PrefetchPatientData (char *PatientID, unsigned int MaxRead);
BOOL DcmMove2(char* pszSourceAE, const char* pszDestinationAE, BOOL patroot, int id, DICOMDataObject *DDO, const char *callback);
BOOL    prefetchprocess(char *data, ExtendedPDU_Service *PDU, char *dum);
BOOL    mayprefetchprocess(char *data, ExtendedPDU_Service *PDU, char *dum);
BOOL    prefetch_queue(const char *operation, char *patientid, const char *server, const char *studyuid, const char *seriesuid, const char *compress, const char *modality, const char *date, const char *sop, const char *imagetype, const char *seriesdesc, int delay=0, const char *script=NULL);
struct conquest_queue *new_prefetcherqueue(void);
BOOL    prefetcher(struct DICOMDataObject *DDO, BOOL move);
BOOL    ApplyWorklist(DICOMDataObject *DDOPtr, Database *DB);
int     SaveToDisk(Database	&DB, DICOMDataObject *DDOPtr, char *Filename, BOOL NoKill, ExtendedPDU_Service *PDU, int Syntax=0, BOOL nopreget=FALSE);
void    StartZipThread(void);
void    StopZipThread(void);
BOOL    PrintOptions ();
//int     ParseArgs (int	argc, char	*argv[], ExtendedPDU_Service *PDU);
//BOOL    SetString(VR	*vr, char	*s, int	Max);
void    KodakFixer(DICOMDataObject	*DDOPtr, BOOL tokodak);
UINT32  crc_ddo(DICOMObject *DO);
BOOL    compare_ddo(DICOMObject *DO1, DICOMObject *DO2);
int     VirtualQueryCached(DICOMDataObject *DDO, const char *Level, int N, Array < DICOMDataObject  *> *pADDO);
int     VirtualQuery(DICOMDataObject *DDO, const char *Level, int N, Array < DICOMDataObject  *> *pADDO, char *ae = NULL);
int     VirtualQueryToDB(DICOMDataObject *DDO, int N, char *ae=NULL);
void    RemoveQueryDuplicates(const char *Level, Array < DICOMDataObject * > *ADDO);
BOOL	PatientStudyFinder(char *server, char *str, char *fmt, FILE *f, const char *level);
BOOL	ImageFileLister(const char *server, char *pat, char *study, char *series, char *sop, char *fmt, FILE *f);
BOOL	SeriesUIDLister(char *server, char *pat, char *study, char *fmt, FILE *f);
int     DcmMergeStudy(const char *server, char *pat, char *study, char *modality, char *seriesdesc, char *script, ExtendedPDU_Service *PDU);
int     DcmSubmitData(char *pat, char *study, char *series, char *sop, char *script, char *modesubmit, char *hostsubmit, int portsubmit, char *ident);
BOOL    VirtualServer2(struct ReadAheadThreadData *ratd, int N);
BOOL    RetrieveOn(DICOMDataObject	*DDOMask, DICOMDataObject **DDOOutPtr, StandardStorage **SStorage, RunTimeClassStorage &RTCStorage,
                   DICOMCommandObject *pDCO  = NULL, Array < DICOMDataObject *> *pADDO = NULL, void *ExtraBytes = NULL);
BOOL    QualifyOn(BYTE *ACRNema, BYTE *MyACRNema, BYTE *RemoteIP, BYTE *RemotePort, BYTE *Compress);
#ifndef UNIX
BOOL	WINAPI DriverHelper ( DriverApp	* );
#else
ThreadRoutineType	DriverHelper ( ThreadRoutineArgType	);
#endif
void    LogUser(const char* pszOperation, AAssociateRQ* pARQ, DICOMCommandObject* pDCO);
void    WipeStack(void);
void    TestCompress(char *filename, const char *modes, ExtendedPDU_Service *PDU);
void    TestForward(char *filename, const char *mode, char *server, ExtendedPDU_Service *PDU);
void    TestSyntax(char *filename, int syntax, ExtendedPDU_Service *PDU);
void    TestThreadedSave(char *filename);
void    CloneDB(char *AE);
void NewTempFile(char *name, const char *ext);
BOOL    MakeQueryString ( DBENTRY	*DBE, char	*s);
int main ( int	argc, char	*argv[] );
void SetStringVR(VR **vr, int g, int e, const char *String);
BOOL GrabImagesFromServer(BYTE *calledae, const char *studydate, char *destination);
int processhtml(char *out, char *in, int len);
BOOL SendServerCommand(const char *NKIcommand1, const char *NKIcommand2, int con, char *buf=NULL, BOOL html=TRUE, BOOL upload=FALSE);
#ifdef UNIX  //Statics
static BOOL AbstractSyntaxEnabled(CheckedPDU_Service &p, AbstractSyntax	&AbsSyntax);
static void TranslateText(char* pSrc, char* pDest, int iSize);
static unsigned int  DFileSize(char *Path);
static BOOL DFileExists(char *Path);
static BOOL NewUIDsInDICOMObject(DICOMObject *DO, const char *Exceptions, const char *Reason=NULL);
static BOOL OldUIDsInDICOMObject(DICOMObject *DO, const char *Exceptions);
static BOOL ModifyPATIDofDDO(DICOMDataObject *pDDO, char *NewPATID, char *Reason=NULL);
static BOOL MergeUIDofImageFile(char *filename, BOOL DelFile, const char *type, char *script, char *Reason, ExtendedPDU_Service *PDU);
static BOOL DFileCopy2(char *source, char *target, int smode);
static BOOL match(const char *source, const char *target);//bcb Made target const.
static BOOL WINAPI processthread(struct conquest_queue *q);
static int DFileRead(char *source, unsigned int MaxRead);
static int DcmMove(const char *patid, char* pszSourceAE, char* pszDestinationAE, const char *studyuid, const char *seriesuid, const char *compress, const char *modality, const char *date,
             const char *sop, const char *imagetype, const char *seriesdesc, int id, char *script);
static BOOL DcmDelete(char *patid, char *studyuid, char *seriesuid, char *modality, char *date, char *sop, char *imagetype);
static BOOL WINAPI prefetcherthread(struct conquest_queue *q);
static void    StartMonitorThread(char *folder);
static BOOL WINAPI monitorthread(char *folder);
static BOOL WINAPI zipthread(void);
static void dgateStrip2(DICOMDataObject *pDDO);
static int SortPatient(const void* pElem1, const void* pElem2);
static int SortStudy(const void* pElem1, const void* pElem2);
static int SortSeries(const void* pElem1, const void* pElem2);
static int SortImages(const void* pElem1, const void* pElem2);
static int SortAccession(const void* pElem1, const void* pElem2);
static int SortImageNumber(const void* pElem1, const void* pElem2);
static int ProcessRATData(struct ReadAheadThreadData *ratd, int maxfiles);
static BOOL WINAPI ReadAheadThread(struct ReadAheadThreadData *ratd);
static BOOL DcmEcho(const char *AE);
#else
BOOL AbstractSyntaxEnabled(CheckedPDU_Service &p, AbstractSyntax	&AbsSyntax);
void TranslateText(char* pSrc, char* pDest, int iSize);
unsigned int  DFileSize(char *Path);
BOOL DFileExists(char *Path);
BOOL NewUIDsInDICOMObject(DICOMObject *DO, const char *Exceptions, const char *Reason=NULL);
BOOL OldUIDsInDICOMObject(DICOMObject *DO, const char *Exceptions);
BOOL ModifyPATIDofDDO(DICOMDataObject *pDDO, char *NewPATID, char *Reason=NULL);
BOOL MergeUIDofImageFile(char *filename, BOOL DelFile, const char *type, char *script, char *Reason, ExtendedPDU_Service *PDU);
BOOL DFileCopy2(char *source, char *target, int smode);
BOOL match(const char *source, const char *target);
BOOL WINAPI processthread(struct conquest_queue *q);
int DFileRead(char *source, unsigned int MaxRead);
int DcmMove(const char *patid, char* pszSourceAE, char* pszDestinationAE, const char *studyuid, const char *seriesuid, const char *compress, const char *modality, const char *date,
             const char *sop, const char *imagetype, const char *seriesdesc, int id, char *script);
BOOL DcmDelete(char *patid, char *studyuid, char *seriesuid, char *modality, char *date, char *sop, char *imagetype);
BOOL WINAPI prefetcherthread(struct conquest_queue *q);
void    StartMonitorThread(char *folder);
BOOL WINAPI monitorthread(char *folder);
BOOL WINAPI zipthread(void);
void dgateStrip2(DICOMDataObject *pDDO);
int SortPatient(const void* pElem1, const void* pElem2);
int SortStudy(const void* pElem1, const void* pElem2);
int SortSeries(const void* pElem1, const void* pElem2);
int SortImages(const void* pElem1, const void* pElem2);
int SortAccession(const void* pElem1, const void* pElem2);
int SortImageNumber(const void* pElem1, const void* pElem2);
int ProcessRATData(struct ReadAheadThreadData *ratd, int maxfiles);
BOOL WINAPI ReadAheadThread(struct ReadAheadThreadData *ratd);
BOOL DcmEcho(const char *AE);
#endif

/******* loadddo prototypes  ********/
DICOMDataObject* LoadImplicitLittleEndianFile(char* filename, unsigned int iVrSizeLimit);

/******* regen prototypes  ********/
BOOL IsDirectory(char *path);
BOOL RegenToDatabase(Database *DB, char *basename, char *filename, const char *device);
BOOL RegenFile(char *filename);
BOOL RegenDir(Database *DB, char *PathString, const char *Device);
BOOL Regen(const char*, int IsCacheOrJukeBox, char *SubDir = NULL);
BOOL	Regen();

/******* vrtosql prototypes  ********/

void safestrcat(char *result, const char *tocat, int maxlen);
BOOL MakeSafeString(VR *vr, char *string, Database *db );
BOOL DICOM2SQLQuery(char *s, Database *db );
BOOL BuildSearchString(Database *DB, DBENTRY	*DBE, char	*TableName, VR	*vr, char	*Search, char	*TempString, int maxlen);
BOOL BuildColumnString(DBENTRY	*DBE, char	*TableName, VR	*vr,	char	*TempString);
UINT32 SQLRealSize(char *Str, SDWORD Max);
BOOL QueryOnPatient(DICOMDataObject *DDO, Array < DICOMDataObject *> *ADDO);
BOOL QueryOnStudy(DICOMDataObject *DDO, Array < DICOMDataObject *> *ADDO);
BOOL QueryOnSeries(DICOMDataObject *DDO, Array < DICOMDataObject *> *ADDO);
BOOL QueryOnImage(DICOMDataObject *DDO, Array < DICOMDataObject *> *ADDO);
BOOL QueryOnModalityWorkList(DICOMDataObject *DDO, Array < DICOMDataObject *> *ADDO);
#ifdef UNIX  //Statics
static VR *ComputeField(DICOMDataObject *DDO, int group, int element, UINT16 mask=0xffff, BOOL allowcount = FALSE);
static void ProcessQuery(DICOMDataObject *DDO, DBENTRY *DB, Array <VR *> *EMask, Array <char *> *SQLResult, Array <DBENTRY *> *DBQ, Array <int> *Levels, int level, Database *dbf);
static void CodeSequence(Array < VR * >*EMask, Array <char *> *SQLResult, Array <SQLLEN *> *SQLResultLength, Array <DBENTRY *> *DBQMaster, Array <int> *Levels, DICOMDataObject *DDO, UINT *Index);
#else
VR *ComputeField(DICOMDataObject *DDO, int group, int element, UINT16 mask=0xffff, BOOL allowcount = FALSE);
void ProcessQuery(DICOMDataObject *DDO, DBENTRY *DB, Array <VR *> *EMask, Array <char *> *SQLResult, Array <DBENTRY *> *DBQ, Array <int> *Levels, int level, Database *dbf);
void CodeSequence(Array < VR * >*EMask, Array <char *> *SQLResult, Array <SQLLEN *> *SQLResultLength, Array <DBENTRY *> *DBQMaster, Array <int> *Levels, DICOMDataObject *DDO, UINT *Index);
#endif
#endif
