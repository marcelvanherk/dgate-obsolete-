#ifndef DBSQL_H_
#define DBSQL_H_
/*
20120703	bcb	Created this file
20120820	bcb	SetString warning change
20130226	bcb	Replaced gpps with IniValue class and removed functions not needed. Version to 1.4.18a.

*/

#define NCACHE 256

#ifndef UNUSED_ARGUMENT
#define UNUSED_ARGUMENT(x) (void)x
#endif

#ifndef UNIX
#define PATHSEPCHAR '\\'
#else
#define PATHSEPCHAR '/'
#endif

#ifndef UNIX
#	include	<direct.h>
#else
#include <unistd.h>
#include <ctype.h>
#define	stricmp(s1, s2)	strcasecmp(s1, s2)
#ifndef EnterCriticalSection
#	define EnterCriticalSection(a) pthread_mutex_lock(a)
#endif
#ifndef LeaveCriticalSection
#	define LeaveCriticalSection(a) pthread_mutex_unlock(a)
#endif
#ifndef CRITICAL_SECTION
#	define CRITICAL_SECTION pthread_mutex_t
#endif
#ifndef InitializeCriticalSection
#	define InitializeCriticalSection(a) pthread_mutex_init(a, NULL);
#endif
#ifndef DeleteCriticalSection
#	define DeleteCriticalSection(a) pthread_mutex_destroy(a);
#endif
#endif
#	include	<time.h>
#	include	<stdlib.h>
#	include <ctype.h>

//#include "gpps.hpp"
#include "dicom.hpp"
#include "dprintf.hpp"
#include "odbci.hpp"

typedef	struct
{
	UINT16		Group;
	UINT16		Element;
	char		SQLColumn[32];
	unsigned int 	SQLLength;
	int		SQLType;
	int		DICOMType;
	char		HL7Tag[32];
}	DBENTRY;

//#include "dgate.hpp" //DUCKHEAD92

// prototypes

BOOL	DICOM2SQLValue ( char *s );
DWORD CurrentTime();
const char	* UniqueKey(DBENTRY *DBE);
const char	* UniqueLink(DBENTRY *DBE);
BOOL AddStudyInstanceUID (DICOMDataObject *DDOPtr);
BOOL AddSeriesInstanceUID (DICOMDataObject	*DDOPtr);
BOOL FixImage(DICOMDataObject *DDOPtr);
int into_UpdateCache(char *in);
int isin_UpdateCache(char *in);
void clear_UpdateCache(void);
BOOL SaveToDataBase( Database &DB, DICOMDataObject *DDOPtr, char *filename, const char *device, BOOL JustAdd);
char *SetString(VR	*vr, char	*s, UINT32	Max);
BOOL AddToTable( Database &DB, DBENTRY *DCMGateDB, char *TableName, DICOMDataObject *DDOPtr, char *ObjectFile, char *DeviceName);
BOOL UpdateOrAddToTable( Database &DB, DBENTRY *DCMGateDB, char *TableName, DICOMDataObject *DDOPtr, const char *ObjectFile, const char *DeviceName, char *Patid, char *Modality, BOOL *Added, BOOL JustAdd, BOOL CheckDuplicates);
BOOL UpdateAccessTimes( Database &ConnectedDB, char *SOPInstanceUID);
BOOL ChangeUID(char *OldUID, const char *Type, char *NewUID);
BOOL ChangeUIDTo(char *OldUID, char *Type, char *NewUID);
BOOL ChangeUIDBack(char *NewUID, char *OldUID);
BOOL DeleteUIDChanges(char *Reason);
BOOL MergeUIDs(char *OldUID[], int n, const char *Type, char *NewUID);
BOOL MakeSafeStringValues(VR *vr, char *string );
BOOL MakeSafeDate(VR *vr, char *string );
BOOL GetFileName(VR *vr, char *filename, char *dv, BOOL UpdateLRU = TRUE, char *patid=NULL, char *study=NULL, char *series=NULL);
BOOL GetFileName(VR *SOPInstance, char *filename, char *device, Database &ConnectedDB, BOOL UpdateLRU = TRUE, char *patid=NULL, char *study=NULL, char *series=NULL);
DICOMDataObject	*MakeCopy(DICOMDataObject *DO);
BOOL NewDeleteFromDB(DICOMDataObject	*pDDO, Database	&aDB);
BOOL NewDeleteSopFromDB(char *pat, char *study, char *series, char *sop, Database &aDB);
BOOL RemoveFromWorld(DICOMDataObject *DDO, Database &DB);
BOOL RemoveFromPACS(DICOMDataObject	*DDO, BOOL KeepImages = FALSE);
BOOL
RemoveDuplicates(Database &DB, DBENTRY *DBE, char *TableName, DICOMDataObject *DDOPtr, BOOL KeyOnAll);
BOOL PrintTable(DBENTRY	*DBT, char	*name);
BOOL MakeTableString ( DBENTRY	*DBE, char	*s, int mode );
BOOL InitializeTables(int mode);
BOOL VerifyIsInDBE(VR *vr, DBENTRY *DBE, DBENTRY * &TempDBEPtr );
UINT LastDBE(DBENTRY *DBE);
UINT DBEIndex(DBENTRY *DBE, VR *vr);
DBENTRY	*FindDBE(VR	*vr);
VR	*ConstructVRFromSQL(DBENTRY *DBE, UINT16 Group, UINT16 Element, UINT32 Length, char *SQLResultString );
VR	*ConstructAE ();
#ifdef UNIX //Statics
#if NATIVE_ENDIAN == BIG_ENDIAN //Big Endian like Apple power pc
static void swap(BYTE *src, BYTE *dst, int N);
static void swap(BYTE *buffer, int N);
static BOOL dbGenUID(char *oString);
#endif
#else
#if NATIVE_ENDIAN == BIG_ENDIAN //Big Endian like Apple power pc
void swap(BYTE *src, BYTE *dst, int N);
void swap(BYTE *buffer, int N);
BOOL dbGenUID(char *oString);
#endif
#endif

#endif

