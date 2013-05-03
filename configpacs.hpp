/****************************************************************************
 
 THIS SOFTWARE IS MADE AVAILABLE, AS IS, AND IMAGE RETERIVAL SERVICES
 DOES NOT MAKE ANY WARRANTY ABOUT THE SOFTWARE, ITS
 PERFORMANCE, ITS MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR
 USE, FREEDOM FROM ANY COMPUTER DISEASES OR ITS CONFORMITY TO ANY
 SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND PERFORMANCE OF
 THE SOFTWARE IS WITH THE USER.
 
 Copyright of the software and supporting documentation is
 owned by the Image Reterival Services, and free access
 is hereby granted as a license to use this software, copy this
 software and prepare derivative works based upon this software.
 However, any distribution of this software source code or
 supporting documentation or derivative works (source code and
 supporting documentation) must include this copyright notice.
 ****************************************************************************/

/***************************************************************************
 *
 * Image Reterival Services
 * Version 0.1 Beta
 *
 ***************************************************************************/

/* 
 bcb 20130226: Created this file. Version to 1.4.18a.
 bcb 20130328: Removed ConfigPadAEWithZeros
 bcb 20130331: Killed lines and sourceOrLine0.
 */

#ifndef configpacs_hpp
#define configpacs_hpp

#include "dicom.hpp"
#include "amap.hpp"

//File looks.
#define EQ_INDENT 25

// Standard lengths help prevent over runs.
#define VSHORT_NAME_LEN 32
#define VIRT_SERV_LEV 48
#define SHORT_NAME_LEN 64
#define STD_NAME_LEN 128
#define LONG_NAME_LEN 256
#define VLONG_NAME_LEN 512
#define BUF_CMD_LEN 1024
#ifndef MAX_PATH
#define MAX_PATH 256
#endif


// Type of System
#	define	E_WORKGROUP		1
#	define	E_PERSONAL		2
#	define	E_ENTERPRISE	3

#define DEFAULT_LOSSY 95
#define THREAD_WAIT_TIME 10//10 seconds to finsh reading dicom.ini before error


enum LEVELTYPE { LT_PATIENT, LT_STUDY, LT_SERIES, LT_IMAGE, LT_SOPCLASS, LT_GLOBAL};
enum DATAPTRTYPE { DPT_UNUSED, DPT_BOOL, DPT_STRING, DPT_STRING_ARRAY, DPT_INT,
        DPT_INT_ARRAY, DPT_PATH, DPT_UINT, DPT_UINT_ARRAY, DPT_DBTYPE, DPT_EDITION,
        DPT_FOR_ASSOC, DPT_FILE_COMP, DPT_END, DPT_MICROP, DPT_SKIP, DPT_TIME, DPT_UNKNOWN};

//Moved from odbci.hpp
enum DBTYPE {DT_ODBC, DT_DBASEIII, DT_POSTGRES, DT_MYSQL, DT_SQLITE, DT_DBASEIIINOINDEX, DT_NULL};

#include <stdio.h>
#include <time.h>

typedef	struct	_iniTable
{
        char            valueName[SHORT_NAME_LEN];
        char            value[BUF_CMD_LEN];
        
} iniTable;

typedef	struct	_iEGroup
{
        Array < char    * >      *CalledAEPtr;
        Array < char    * >      *CallingAEPtr;
        Array < char    * >      *ConverterPtr;
        Array < char    * >      *ModalityPtr;
        Array < char    * >      *StationNamePtr;
        Array < char    * >      *FilterPtr;
} iEGroup;

//ADD new values or arrays to sscscp section here and  3 or 4 other places.
//sscscp value add
//sscscp array add
typedef	struct	_DicomIni
{
        Array < iniTable    * >  *iniTablePtr;

        char            MicroPACS[SHORT_NAME_LEN];
//        char            ACPipeName[MAX_PATH]; //AccessUpdates
        char            ACRNemaMap[MAX_PATH];
        char            AllowTruncate[LONG_NAME_LEN];
        char            AnyPage[LONG_NAME_LEN];
        char            ArchiveCompression[ACRNEMA_COMP_LEN];
//        unsigned int    CACHEDevices;//Use CACHEDevicePtr array count
        BOOL            CacheVirtualData;
//        unsigned int    CDRDevices;//Use CDRDevicePtr array count
        enum DBTYPE     db_type;
        unsigned int    DebugVRs;
        BOOL            DecompressNon16BitsJpeg;
        char            DefaultPage[LONG_NAME_LEN];
        char            Dictionary[MAX_PATH];
//        char            DMarkTableName[SHORT_NAME_LEN];//Unused!
        char            DroppedFileCompression[ACRNEMA_COMP_LEN];
        BOOL            DoubleBackSlashToDB;
        unsigned int    Edition;
        BOOL            EnableComputedFields;
        BOOL            EnableReadAheadThread;
//        unsigned int    ExportConverters;//Use ExportConverterPtr array count
        unsigned int    FailHoldOff;
        unsigned int    FileCompressMode;
        char            FileNameSyntaxString[SHORT_NAME_LEN];
        BOOL            FixKodak;
        BOOL            FixPhilips;
        unsigned int    ForwardAssociationCloseDelay;
        unsigned int    ForwardAssociationRefreshDelay;
        BOOL            ForwardAssociationRelease;
        enum LEVELTYPE  Level;//ForwardAssociationLevel
        unsigned int    ForwardCollectDelay;
//        BOOL            HasArchiveConverter;//Use ArchiveConverterPtr array count
//        BOOL            HasMoveDeviceConverter;//Use MoveDeviceConverterPtr array count
        BOOL            IgnoreMAGDeviceThreshHold;
        BOOL            IgnoreOutOfMemoryErrors;
        char            ImageQuerySortOrder[LONG_NAME_LEN];
        char            ImageTableName[SHORT_NAME_LEN];
        BOOL            ImportExportDragAndDrop;
        char            IncomingCompression[ACRNEMA_COMP_LEN];
        unsigned int    IndexDBF;
//        unsigned int    JUKEBOXDevices;//Use JUKEBOXDevicePtr array count
        char            kFactorFile[MAX_PATH];
        unsigned int    LongQueryDBF;
        unsigned int    LossyQuality;
        char            LRUSort[SHORT_NAME_LEN];
//        unsigned int    MAGDevices;//Use MAGDevicePtr array count
        int             MaximumDelayedFetchForwardRetries;
        unsigned int    MAGDeviceFullThreshHold;
        unsigned int    MAGDeviceThreshHold;
        unsigned int    MaximumExportRetries;
        unsigned int    MaxFieldLength;
        unsigned int    MaxFileNameLength;
//        unsigned int    MIRRORDevices;//Use MIRRORDevicePtr array count
//        unsigned int    MOPDevices;//Use MOPDevicePtr array count
        char            MyACRNema[ACRNEMA_AE_LEN];
        unsigned int    NightlyCleanThreshhold;
        BOOL            NoDicomCheck;
        char            OCPipeName[MAX_PATH];//OperatorConsole
        int             OverlapVirtualGet;
        BOOL            PackDBF;
        char            ServerName[STD_NAME_LEN];//PACSName
//        BOOL            ConfigPadAEWithZeros; //PadAEWithZeros bcb 130328
        char            Password[STD_NAME_LEN];
        char            PatientQuerySortOrder[LONG_NAME_LEN];
        char            PatientTableName[SHORT_NAME_LEN];
        unsigned int    Prefetcher;
        unsigned int    QueueSize;
//        char            RegisteredMOPDeviceTable[SHORT_NAME_LEN];//Unused
        char            RegisteredMOPIDTableName[SHORT_NAME_LEN];
        unsigned int    RetryDelay;
        BOOL            RetryForwardFailed;
        BOOL            SendUpperCaseAE;
        char            SeriesQuerySortOrder[LONG_NAME_LEN]; 
        char            SeriesTableName[SHORT_NAME_LEN];
        char            SOPClassFile[MAX_PATH];
        char            DataHost[MAX_PATH];//SQLHost
        char            DataSource[STD_NAME_LEN];//SQLServer
        unsigned int    StorageFailedErrorCode;
        char            StudyQuerySortOrder[LONG_NAME_LEN];
        char            StudyTableName[SHORT_NAME_LEN];
        unsigned int    TCPIPTimeOut;
        char            Port[ACRNEMA_PORT_LEN];//TCPPort
        char            TempDir[MAX_PATH];
        char            TroubleLogFile[MAX_PATH];
        unsigned int    TruncateFieldNames;//Has different default values.
        int             TruncateFieldNamesLen;
        char            UIDPrefix[SHORT_NAME_LEN];
//        BOOL            UseBuiltInDecompressor;//Unused!
        BOOL            UseBuiltInJPEG;
        BOOL            UseEscapeStringConstants;
#ifdef HAVE_BOTH_J2KLIBS
        BOOL            UseOpenJPG;
#endif
        char            UserLogFile[MAX_PATH];
        char            UserName[STD_NAME_LEN];
        char            monitorfolder2[MAX_PATH];//WatchFolder
        char            WebCodeBase[VLONG_NAME_LEN];
        char            WebMAG0Address[LONG_NAME_LEN];
        BOOL            WebPush;
        BOOL            WebReadOnly;
        char            WebScriptAddress[LONG_NAME_LEN];
        char            WebServerFor[SHORT_NAME_LEN];
        unsigned int    WorkListMode;
        int             WorkListReturnsISO_IR_100;
        int             WorkListReturnsISO_IR;
        char            WorkListTableName[SHORT_NAME_LEN];
        char            UIDToCDRIDTableName[SHORT_NAME_LEN];
        char            UIDToMOPIDTableName[SHORT_NAME_LEN];
//        char            ziptime[SHORT_NAME_LEN];
        struct tm       ziptime;
// The arrays
//        Array < char    * >      *ArchiveConverterPtr;
        Array < char    * >      *CACHEDevicePtr;
///        Array < char    * >      *CDRDevicePtr;//Unused
        Array < char    * >      *JUKEBOXDevicePtr;
        Array < char    * >      *MAGDevicePtr;
        Array < char    * >      *MIRRORDevicePtr;
//        Array < char    * >      *MOPDevicePtr;//Unused
//        Array < char    * >      *MoveDeviceConverterPtr;
//        Array < char    * >      *ImportConverterPtr;
        Array < char    * >      *VirtualServerForPtr;
        Array < int     * >      *VirtualServerPerSeriesPtr;
// The Import Export Groups        
        iEGroup      *ArchivePtr;
        iEGroup      *CompressionPtr;
        iEGroup      *ExportPtr;
        iEGroup      *ImportPtr;
        iEGroup      *MergeSeriesPtr;
        iEGroup      *MergeStudiesPtr;
        iEGroup      *ModalityWorkListQueryResultPtr;
        iEGroup      *MoveDevicePtr;
        iEGroup      *QueryResultPtr;
        iEGroup      *QueryPtr;
        iEGroup      *RetrieveResultPtr;
        iEGroup      *RetrievePtr;
        iEGroup      *RejectedImageWorkListPtr;
        iEGroup      *RejectedImagePtr;
        iEGroup      *WorkListQueryPtr;

} DicomIni;

typedef	struct	_sourceLines
{
        Array < iniTable    * > *iniTablePtr;
        char                    source[BUF_CMD_LEN];
//        Array < char    * >     *linesPtr; //Killed lines and sourceOrLine0.
//        bool                    sourceOrLine0;
        char                    caption[BUF_CMD_LEN];
        char                    header[BUF_CMD_LEN];
} sourceLines;

//ADD new values or arrays to lua section here and  3 or 4 other places.
//lua value add
//lua array add
typedef	struct	_luaIni
{
// The converter array
        Array < iniTable    * >  *iniTablePtr;
        Array < char    * >      *ExportConverterPtr;
        char            association[BUF_CMD_LEN];
        char            background[BUF_CMD_LEN];
        char            clienterror[BUF_CMD_LEN];
        char            command[BUF_CMD_LEN];
        char            endassociation[BUF_CMD_LEN];
        char            nightly[BUF_CMD_LEN];
        char            startup[BUF_CMD_LEN];
        // The Import Groups        
        iEGroup      *ArchivePtr;
        iEGroup      *CompressionPtr;
        iEGroup      *ImportPtr;
        iEGroup      *MergeSeriesPtr;
        iEGroup      *MergeStudiesPtr;
        iEGroup      *ModalityWorkListQueryResultPtr;
        iEGroup      *MoveDevicePtr;
        iEGroup      *QueryResultPtr;
        iEGroup      *QueryPtr;
        iEGroup      *RetrieveResultPtr;
        iEGroup      *RetrievePtr;
        iEGroup      *RejectedImageWorkListPtr;
        iEGroup      *RejectedImagePtr;
        iEGroup      *WorkListQueryPtr;

} luaIni;

typedef	struct	_wadoserversIni
{
// The converter array
        Array < iniTable    *> *iniTablePtr;
        char            bridge[LONG_NAME_LEN];
} wadoserversIni;

typedef	struct	_webdefaultsIni
{
        Array < iniTable    *> *iniTablePtr;
        char            address[SHORT_NAME_LEN];
        char            compress[SHORT_NAME_LEN];
        char            dsize[VSHORT_NAME_LEN];
        char            graphic[VSHORT_NAME_LEN];
        char            iconsize[VSHORT_NAME_LEN];
        char            port[ACRNEMA_PORT_LEN];
        char            size[VSHORT_NAME_LEN];
        char            viewer[STD_NAME_LEN];
        char            studyviewer[STD_NAME_LEN];
} webdefaultsIni;

typedef	struct	_sectionTable
{
        char            section[SHORT_NAME_LEN];
        Array < iniTable    *> *iniTablePtr;        
} sectionTable;

typedef	struct	_iniStrings
{
        char            basePath[MAX_PATH];
        char            iniFile[MAX_PATH];
        char            newConfigFile[MAX_PATH];
        char            currentSection[LONG_NAME_LEN];
        char            errorBuffer[STD_NAME_LEN];
} iniStrings;


class   IniValue
        {
        private:
// Class globals, mostly used to speed up calls without stack pushes.
                size_t          i, iSize, lastLine;
                FILE            *tempFile;
                unsigned int    iStrLen;
                int             iNum;
                char            *pSrc;
                void            *iVoidPtr, **iVoidHandle, ***iVoidDHandle;
// For test mode, dump ini without running!
                BOOL            dumpIni;
// File paths and current setion struct.
                iniStrings      *iniStringsPtr;
// The temporary section pointers to make it thread safe.
                sourceLines     *AnyPageTmpPtr;
                sourceLines     *DefaultPageTmpPtr;
                luaIni          *luaTmpPtr;
                sourceLines     *markseriesTmpPtr;
                sourceLines     *markstudyTmpPtr;
                sourceLines     *shoppingcartTmpPtr;
                DicomIni        *sscscpTmpPtr;
                wadoserversIni  *wadoserversTmpPtr;
                webdefaultsIni  *webdefaultsTmpPtr;
                Array < iniTable    *> *scriptsTmpPtr;
                Array < sectionTable *> *unknownSectionTmpPtr;
                Array < sectionTable *> *unknownSectionPtr;
        protected:
// Create and destroy the class protected to make it a singleton.
                IniValue();
                ~IniValue();
        public:
// Just for lua and status display.
                int             gpps, gppstime;
// A buffer for error messages
                char            *errorBufferPtr;
// Section pointers.
                sourceLines     *AnyPagePtr;
                sourceLines     *DefaultPagePtr;
                luaIni          *luaPtr;
                sourceLines     *markseriesPtr;
                sourceLines     *markstudyPtr;
                Array < iniTable    *> *scriptsPtr;
                sourceLines     *shoppingcartPtr;
                DicomIni        *sscscpPtr;
                wadoserversIni  *wadoserversPtr;
                webdefaultsIni  *webdefaultsPtr;
// The interface routines.
        // Gets a pointer to the singleton.  May not be fully thread safe, but ok for us.
                static IniValue *GetInstance()
                {
                        //"Lazy" initialization. Singleton not created until it's needed
                        static IniValue iniInstance; // One and only one instance.
                        return &iniInstance;
                }
        // Will delete the name and value from the ini file,the name can be name:section or the default MicroPACS section will be used.
        // Will not delete an empty section yet.
                BOOL            DeleteNameFromIniFile(const char *theName);
        // Will delete the name and value in the section from the ini file
        // Will not delete an empty section yet.
                BOOL            DeleteNameFromIniFile(const char *theName, const char *theSection);
        // Will output, to the outputFile, ALL possible Ini values including defaults of any section found.
                void            DumpIni(FILE *outputFile = stdout);
        // Where dgate was started from or the value from a -w option.
                char            *GetBasePath(){return iniStringsPtr->basePath;};
        // The "dicom.ini" full path, influenced by the -w option. 
                char            *GetIniFile(){return iniStringsPtr->iniFile;};
        // This returns a pointer to the actual data, never free it! Used by lua to get any pointer to an ini MicroPACS int or uint.
                int             *GetIniInitPtr(const char *theName){return GetIniInitPtr(theName, sscscpPtr->MicroPACS);};
        // This returns a pointer to the actual data, never free it! May be used by lua to get any ini string pointer.
                int             *GetIniInitPtr(const char *theName, const char *theSection);
        // Will return a string that must be freeded!  Returns the line asked for.
                char            *GetIniLine(const char *lineNum);
        // Will return a string that must be freeded!  The format for the search is "value name":"section" (no quotes).
        // If "section" is not included, sscscpPtr->MicroPACS is used as the default.  AnyPage and Default page will
        // respond to sscscpPtr->AnyPage and sscscpPtr->DefaultPage, as well as "AnyPage" and "Default", case insensitive.
        // The micropacs section used is only the sscscpPtr->MicroPACS value. If more than one micropac section exist,
        // the value pointed to by iniValuePtr->sscscpPtr->"value name" is used by dgate.  The other is found in the 
        // unknown section array (unknownSectionPtr) and only read though this interface.  To switch, change the sscscpPtr->MicroPACS
        // value and reread ( ReadIni() ).
                char            *GetIniString(const char *theName);
        // Same as above, but the section is seperate.
                char            *GetIniString(const char *theName, const char *theSection);
        // This returns a pointer to the actual data, never free it! Used by lua to get any pointer to an ini MicroPACS string.
                int             GetIniStringLen(const char *theName, char **strH){return GetIniStringLen(theName, strH, sscscpPtr->MicroPACS);};
        // This returns a pointer to the actual data, never free it! May be used by lua to get any ini string pointer.
                int             GetIniStringLen(const char *theName, char **strH, const char *theSection);
        // This returns a pointer to the actual data, never free it!  It is current used to quickly retrieve values in the
        // scripts section, all stored as unknowns, but it can retrieve from any section's unknowns values tables.
                char            *GetIniTableValue(const char *theName, Array <iniTable *> *iniTablePtr);
        // This returns a pointer to the actual data, never free it!  It is currently not used externally.
        // It searches the unknown section table.
                char            *GetUnknownSectionTableValue(const char *theName, const char *theSection);
        // True if just a dump of the file.
                BOOL            IniDumped(){return dumpIni;};
        // It just tells you if a read or reread is done.
                BOOL            IniReadDone(){return (sscscpPtr->MicroPACS[0] != 0  && sscscpTmpPtr == NULL);};
        // Reads or rereads the Ini file from iniStringsPtr->iniFile.  It uses temporary pointers to make it thread safe.
        // The pointers ars swapped after all of the parsing is done, so no ini values change to the default
        // value durning the reread.
                BOOL            ReadIni();
        // Sets where to find the needed dgate support files if a full path is not specified in the Ini file.
                BOOL            SetBasePath(const char *argv0);
        // Sets the name, location or both for the INI iniStringsPtr->iniFile.  It will change the base path if a path is included.
                BOOL            SetIniFile(const char *iniFile);
        // Writes an updated value to the ini file, the name can be name:section or the default MicroPACS section will be used.
                BOOL            WriteToIni(const char *theName, const char *theValue);
        // Writes an updated value to the section of the ini file,
                BOOL            WriteToIni(const char *theName, const char *theValue, const char *theSection);
        private:
                BOOL            addToCharArray(const char *theStr, Array<char *> **arrayHand);
                BOOL            addToIntArray(int theInt, Array<int *> **arrayHand);
                BOOL            chechThreadError(void *anyPtr);
                void            defaultIni();
                void            dumpCharArray(FILE *outputFile, Array < char    * > *theArray, const char *who);
                void            dumpIEGroup(FILE *outputFile, iEGroup *theGroup, const char *who);
                void            dumpIntArray(FILE *outputFile, Array < int    * > *theArray, const char *who);
                void            dumpsourceLines(FILE *outputFile, sourceLines *theSL, const char *who);
                void            dumpTime(FILE *outputFile, struct tm *timeStruct, const char *who);
                void            errorCleanNewConfigFile();
                BOOL            errorToBuffer(const char *errorMsg);
                void            freeCharArray(Array<char *> **arrayHandle);
                void            freeIntArray(Array<int *> **arrayHandle);
                void            freeIEGroup(iEGroup **theGroupH);
                void            freeSourceLines(sourceLines **sourceLinesHandle);
                void            freeLua();
                void            freesection();
                void            freesscscp();
                void            freewadoservers();
                void            freewebdefaults();
                BOOL            getBoolWithTemp();
                char            *getIniBoolString();
                char            *getIniInitFromArray(Array<int *> **arrayHandle);
                char            *getIniInitFromArray(Array<int *> ***arrayDHandle);
                char            *getIniString();
                char            *getIniStringFromArrayH(Array<char *> **arrayHandle);
                char            *getIniStringFromArrayDH(Array<char *> ***arrayDHandle);
                char            *getIniTime();
                int             getReplyLength();//-1 is not found; 0 is found, but empty.
                char            *getStringPtrFromArray();
                DATAPTRTYPE     getTypeForLua();
                DATAPTRTYPE     getTypeForsourceLines();
                DATAPTRTYPE     getTypeForsscscp();
                DATAPTRTYPE     getTypeForWebDefaults();
                BOOL            goToNextDataLine();
                BOOL            goToNextSection();
                BOOL            goToStartIsBlank();
                BOOL            goToStartOfName(const char *theName);
                void            initIEGroup(iEGroup *theGroup);
                char            *intTableValue(Array<iniTable *> *iniTableAray, const char *theName);
                BOOL            isIEGroup(unsigned int iIncrease);
                BOOL            keepLine(const char *startOfLine);
                char            *lstrcpysplim(char * dest, size_t limit);
                BOOL            lstrieq(const char * lowUp);
                BOOL            lstrileq(const char * lowOnly);
                int             lstrileqNum(const char * lowOnly);
                void            makeFullPath(char *inFile);
                char            *mallocReturnBuf(const char *theString);
                BOOL            openNewConfigFile();
                BOOL            printAllocateError(int allocSize ,const char *what);
                BOOL            readIniFile();
                BOOL            readlua();
                BOOL            readScripts();
                BOOL            readsourceLines(sourceLines  **lsourceLinesH);
                BOOL            readsscscp();
                void            readsscscpFixes();
                BOOL            readUknownSection();
                BOOL            readwadoservers();
                BOOL            readwebdefaults();
                void            setIniBool();
                void            setIniFullPath();
                void            setIniINT(int *outInt);
                void            setIniString(char *outStr, unsigned int limit);
                void            setIniUINT(unsigned int *outUInt);
                void            setIntForArray(Array <int *> ***arrayHand);
                BOOL            setStringForArray(Array <char *> ***arrayDHand);
                void            setIniTime();
                void            setUIntForArray(Array <unsigned int *> ***arrayHand);
                BOOL            skipComments();
                BOOL            swapConfigFiles();
                void            swapPointers(void **handleOne, void **handleTwo);
                char            *threadError();
                BOOL            unknownEntry(Array <iniTable *> **theTableHdl);
                void            unknownPrint(Array<iniTable *>*iniTablePtr, FILE *outputFile);
                BOOL            writeByPutParms(const char *what);
                BOOL            writeNameValue(const char *theName, const char *theValue);
                BOOL            writeTonewConfigFile(const char* from, size_t amount);

        private:// This will prevent it from being copied (it has a pointer)
                IniValue(const IniValue&);
                const	IniValue & operator = (const IniValue&);
        };
#endif
