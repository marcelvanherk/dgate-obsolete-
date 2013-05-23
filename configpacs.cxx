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
 bcb 20130402: Used new file.xpp for file opening.
 bcb 20130416: Data Soucre type (name/file) set by dbtype, fixed win backslash.
                Fixed some full paths. Fixed blank line check for arrays.
 bcb 20130521: Fixed quotes in command lines.
*/

#define NAME_SECTION_SEPERATOR ':'
#ifndef UNIX
#include	<direct.h>
#include	<io.h>
#ifndef snprintf
#define snprintf _snprintf
#endif
#define PATHSEPSTR "\\"
#define PATHSEPCHAR '\\'
#else // UNIX
#define PATHSEPSTR "/"
#define PATHSEPCHAR '/'
#include <unistd.h>
#endif
#define setStringLim(a) setIniString(a, sizeof(a))
#ifndef ctime_r
#define ctime_r(a, b) ctime(a)
#endif

#include "configpacs.hpp"
#include "file.hpp"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

/*************************************************************************
 *                                                                       *
 * IniValue Class                                                        *
 *                                                                       *
 *  IniValue creates structures from the dicom.ini file                  *
 *                                                                       *
 ************************************************************************/
/*
 To add a new value, It must be put into 4 or 5 places.
 configpacs.hpp
        defaultIni() or subsection to set the default value.
        DumpIni() to show it works.
        getPtrForxxx to get the value.
        free... if an array.

 All arrays should be set to NULL util needed and set back to NULL when freed.
 When an array is used, it must be tested for a NULL first.

 Each sections has two pointers, a temporary one used during reading,
 and a real one to be called by the program.  This is to make it thread safe.
 If one thread is rereading the ini file, the other threads will not see the
 default values during the read.

 If in the sscscp section, DumpIni is set to 1, dgate will just dump all
 ini values, including the default values of items not in the ini file.

 Rules for  the ini file:
        Lables may not contain spaces or tabs. MAG Device will not work.
        Lables may be followed spaces or tabs.
        Lables must be followed be an equal sign.
        The equal sign may be followed spaces or tabs.
        A lable and an equal sign with out a value is just a waste of typing,
          it has no effect.
        After an equal sign, quotes may be used, anything in the quotes will
          be attached to that lable.  To put a quote or a back slash with in quotes
          use a back slash. ( ex: \" or \\ ).  Quotes may be in the middle of a value.
        Quotes do not copy into the value unless back slashed.
        A '/' followed by a return (any type) will not continue on to the next line.

 MyACRNema = PAC
 MyACRNema            = PAC
 MyACRNema =          PAC
 MyACRNema= PAC
 MyACRNema =PAC
 MyACRNema=PAC
 MyACRNema = "PAC"
 MyACRNema = P"A"C
 Are all the same.
*/
// Start of the good stuff!

//Creates the class and sets all of the temporary and real section pointers to NULL.
IniValue        ::       IniValue():AnyPagePtr(NULL), AnyPageTmpPtr(NULL), DefaultPagePtr(NULL), DefaultPageTmpPtr(NULL),
dumpIni(FALSE), luaPtr(NULL), luaTmpPtr(NULL), markseriesPtr(NULL), markseriesTmpPtr(NULL), markstudyPtr(NULL),
markstudyTmpPtr(NULL), sscscpPtr(NULL), scriptsPtr(NULL), scriptsTmpPtr(NULL), unknownSectionPtr(NULL), unknownSectionTmpPtr(NULL),
shoppingcartPtr(NULL), shoppingcartTmpPtr(NULL), wadoserversPtr(NULL), wadoserversTmpPtr(NULL),
webdefaultsPtr(NULL), webdefaultsTmpPtr(NULL)
{
        if((iniStringsPtr = (iniStrings *)malloc(sizeof(iniStrings))) == NULL)//Always needed, so make it now.
        {
                fprintf(stderr,"Could not allocate %ld bytes for Ini strings.\n", sizeof(iniStrings));
                exit(EXIT_FAILURE);// Who would think?
        }
        errorBufferPtr = iniStringsPtr->errorBuffer;
        iniStringsPtr->iniFile[0] = 0;
        if((sscscpTmpPtr = (DicomIni *)malloc(sizeof(DicomIni))) == NULL)//Always needed, so make it now.
        {
                printAllocateError(sizeof(DicomIni),"DicomIni Table");
                fprintf(stderr, "%s", errorBufferPtr);
                exit(EXIT_FAILURE);// Who would think?
        }
// Clear error buffer.
        *errorBufferPtr = 0;
//Just for lua and status display.
        gpps = 0;
        gppstime = (time(NULL) & INT_MAX);
}

IniValue        ::       ~IniValue()
{
        if(sscscpPtr != NULL)
        {
                freesscscp();//Free sscscpTmpPtr
                sscscpTmpPtr = sscscpPtr;
                freesscscp();
                freeLua();//Free luaTmpPtr
                luaTmpPtr = luaPtr;
                freeLua();
                freeSourceLines(&AnyPageTmpPtr);
                freeSourceLines(&DefaultPageTmpPtr);
                freeSourceLines(&markseriesTmpPtr);
                freeSourceLines(&markstudyTmpPtr);
                freeSourceLines(&shoppingcartTmpPtr);
                freeSourceLines(&AnyPagePtr);
                freeSourceLines(&DefaultPagePtr);
                freeSourceLines(&markseriesPtr);
                freeSourceLines(&markstudyPtr);
                freeSourceLines(&shoppingcartPtr);
                freeCharArray((Array<char *> **)&scriptsTmpPtr);
                freeCharArray((Array<char *> **)&scriptsPtr);
                freewadoservers();//Free wadoserversTmpPtr
                wadoserversTmpPtr = wadoserversPtr;
                freewadoservers();
                freewebdefaults();//Free webdefaultsTmpPtr
                webdefaultsTmpPtr = webdefaultsPtr;
                freewebdefaults();
                freesection();//Free unknownSectionTmpPtr
                unknownSectionTmpPtr = unknownSectionPtr;
                freesection();
        }
/*        if(instancePtr != 0)
        {
                delete instancePtr;
                instancePtr = 0;
        }*/
}

// If there is no array, it is created
BOOL IniValue        ::       addToCharArray(const char *theStr, Array<char *> **arrayHand)
{
        char *arrayStr, *oldArrayStr;
        size_t theStrLen;
        
        if(*arrayHand == NULL) *arrayHand = new Array<char *>;
        theStrLen = strlen(theStr) + 1;
        if((arrayStr = (char *)malloc(theStrLen)) == NULL)
                return (printAllocateError((int)theStrLen , "a small array string"));
        strcpylim(arrayStr, theStr, theStrLen);
        if(iNum < 0 || (unsigned int)iNum >= (*arrayHand)->GetSize())
        {
                (*arrayHand)->Add(arrayStr);
                return TRUE;
        }
        oldArrayStr = (*arrayHand)->Get((unsigned int)iNum);
        if(oldArrayStr) free(oldArrayStr);
        (*arrayHand)->ReplaceAt(arrayStr, iNum);//Set to new data.
        return TRUE;
}

// If there is no array, it is created
BOOL IniValue        ::       addToIntArray(int theInt, Array<int *> **arrayHand)
{
        int *arrayIntPtr, *oldIntPtr;
        
        if(*arrayHand == NULL) *arrayHand = new Array<int *>;
        arrayIntPtr = new int;
        *arrayIntPtr = theInt;
        if(iNum < 0 || (unsigned int)iNum >= (*arrayHand)->GetSize())
        {
                (*arrayHand)->Add(arrayIntPtr);
                return TRUE;
        }
        oldIntPtr = (*arrayHand)->Get((unsigned int)iNum);
        if(oldIntPtr) delete oldIntPtr;
        (*arrayHand)->ReplaceAt(arrayIntPtr, iNum);//Set to new data.
        return TRUE;
}

BOOL IniValue        ::       chechThreadError(void *anyPtr)
{
        unsigned int cnt = 0;
        while( anyPtr != NULL)//Should be null unless another thread is using it
        {
                if(cnt > THREAD_WAIT_TIME)
                {
                        pSrc = NULL; //Used by the GetIni... pSrc should always be null when leaving.
                        return true;
                }
#ifdef UNIX
                sleep(1);
#else
                Sleep(1000);
#endif
        }
        return false;
}

// The default values are placed into the temp pointer so the default values to not appear to other threads
//ADD new values or arrays to sscscp section here and 3 or 4 other places.
//sscscp value add
//sscscp array add
void IniValue        ::       defaultIni()
{        
        sscscpTmpPtr->MicroPACS[0]                       = 0;
        sscscpTmpPtr->MAGDevicePtr                       = NULL;//Set next
#ifndef	UNIX
//        strcpy(sscscpTmpPtr->ACPipeName,                 "\\\\.\\pipe\\MicroPACSAccess");//Unused!
        strcpy(sscscpTmpPtr->ACRNemaMap,                 ".\\acrnema.map");
        strcpy(sscscpTmpPtr->Dictionary,                 ".\\dgate.dic");
#ifdef DGJAVA
        strcpy(sscscpTmpPtr->JavaHapiDirectory,              ".\\javahl7");
#endif
        strcpy(sscscpTmpPtr->kFactorFile,                ".\\dicom.sql");
        strcpy(sscscpTmpPtr->OCPipeName,                 "\\\\.\\pipe\\MicroPACSStatus");
        addToCharArray(".\\data\\", &(sscscpTmpPtr->MAGDevicePtr));
#else
//        strcpy(sscscpTmpPtr->ACPipeName,                 "/tmp/MicroPACSAccess");//Unused!
        strcpy(sscscpTmpPtr->ACRNemaMap,                 "acrnema.map");
        strcpy(sscscpTmpPtr->Dictionary,                 "dgate.dic");
#ifdef DGJAVA
        strcpy(sscscpTmpPtr->JavaHapiDirectory,              "javahl7");
#endif
        strcpy(sscscpTmpPtr->kFactorFile,                "dicom.sql");
        strcpy(sscscpTmpPtr->OCPipeName,                 "/tmp/MicroPACSStatus");
        addToCharArray("./data/", &(sscscpTmpPtr->MAGDevicePtr));
#endif
        sscscpTmpPtr->AllowTruncate[0]                   = 0;
        sscscpTmpPtr->AnyPage[0]                         = 0;
        strcpy(sscscpTmpPtr->ArchiveCompression,         "as");
        sscscpTmpPtr->CacheVirtualData                   = 1;
        sscscpTmpPtr->db_type                            = DT_ODBC;
        sscscpTmpPtr->DebugVRs                           = 0;
        sscscpTmpPtr->DecompressNon16BitsJpeg            = 1;
        sscscpTmpPtr->DefaultPage[0]                     = 0;
//        strcpy(sscscpTmpPtr->DMarkTableName,             "DICOMAccessUpdates");//Unused
        sscscpTmpPtr->DoubleBackSlashToDB                = 0;
        sscscpTmpPtr->DroppedFileCompression[0]          = 0;
        sscscpTmpPtr->Edition                            = E_WORKGROUP;
        sscscpTmpPtr->EnableComputedFields               = 0;
        sscscpTmpPtr->EnableReadAheadThread              = 1;
        sscscpTmpPtr->FailHoldOff                        = 60;
        sscscpTmpPtr->FileCompressMode                   = 0;//Obsolete   
        strcpy(sscscpTmpPtr->FileNameSyntaxString,       "0");
        sscscpTmpPtr->FixKodak                           = 0;
        sscscpTmpPtr->FixPhilips                         = 0;
        sscscpTmpPtr->ForwardAssociationCloseDelay       = 5;
        sscscpTmpPtr->ForwardAssociationRefreshDelay     = 3600;
        sscscpTmpPtr->ForwardAssociationRelease          = 1;
        sscscpTmpPtr->Level                              =LT_STUDY;//ForwardAssociationLevel
        sscscpTmpPtr->ForwardCollectDelay                = 600;    
        sscscpTmpPtr->IgnoreMAGDeviceThreshHold          = 0;
        sscscpTmpPtr->IgnoreOutOfMemoryErrors            = 0;
        sscscpTmpPtr->ImportExportDragAndDrop            = 0;
        sscscpTmpPtr->IncomingCompression[0]             = 0;
        sscscpTmpPtr->ImageQuerySortOrder[0]             = 0;
        strcpy(sscscpTmpPtr->ImageTableName,             "DICOMImages");
        sscscpTmpPtr->IndexDBF                           = 10;
#ifdef DGJAVA
        sscscpTmpPtr->JavaMinVersion                     = JNI_VERSION_1_2;
#endif
        sscscpTmpPtr->LongQueryDBF                       = 1000;
        sscscpTmpPtr->LossyQuality                       = DEFAULT_LOSSY;
// When LRUSort was delared it was set to "StudyDate" and changed to "" when read from ini.
        sscscpTmpPtr->LRUSort[0]                         = 0;
        sscscpTmpPtr->MAGDeviceFullThreshHold            = 30;
        sscscpTmpPtr->MAGDeviceThreshHold                = 40;
        sscscpTmpPtr->MaximumDelayedFetchForwardRetries  = 0;
        sscscpTmpPtr->MaximumExportRetries               = 0;
        sscscpTmpPtr->MaxFieldLength                     = 0;
        sscscpTmpPtr->MaxFileNameLength                  = 255;
        strcpy(sscscpTmpPtr->MyACRNema,                  "none");
        sscscpTmpPtr->NightlyCleanThreshhold             = 0;
        sscscpTmpPtr->NoDicomCheck                       = 0;
        sscscpTmpPtr->OverlapVirtualGet                  = 0;
        sscscpTmpPtr->PackDBF                            = 0;
        strcpy(sscscpTmpPtr->ServerName,                   "MicroPACS");
//        sscscpTmpPtr->ConfigPadAEWithZeros               = 0; //PadAEWithZeros bcb 130328
        strcpy(sscscpTmpPtr->Password,                   "password");
        sscscpTmpPtr->PatientQuerySortOrder[0]           = 0;
        strcpy(sscscpTmpPtr->PatientTableName,           "DICOMPatients");
        sscscpTmpPtr->Prefetcher                         = 0;
        sscscpTmpPtr->QueueSize                          = 128;
//        strcpy(sscscpTmpPtr->RegisteredMOPDeviceTable,   "RegisteredMOPIDs");//Unused
        strcpy(sscscpTmpPtr->RegisteredMOPIDTableName,   "RegisteredMOPIDs");
        sscscpTmpPtr->RetryDelay                         = 100;
        sscscpTmpPtr->RetryForwardFailed                 = 0;
        sscscpTmpPtr->SendUpperCaseAE                    = 0;
        sscscpTmpPtr->SeriesQuerySortOrder[0]            = 0;
        strcpy(sscscpTmpPtr->SeriesTableName,            "DICOMSeries");
        strcpy(sscscpTmpPtr->DataHost,                   "localhost");//SQLHost
        strcpy(sscscpTmpPtr->SOPClassFile,               "dgatesop.lst");//SOPClassList
        sscscpTmpPtr->DataSource[0]                      = 0;//SQLServer
        sscscpTmpPtr->StorageFailedErrorCode             = 272;
        sscscpTmpPtr->StudyQuerySortOrder[0]             = 0;
        strcpy(sscscpTmpPtr->StudyTableName,             "DICOMStudies");
        sscscpTmpPtr->TCPIPTimeOut                       = 300;
        strcpy(sscscpTmpPtr->Port,                       "104");//TCPPort
        sscscpTmpPtr->TempDir[0]                         = 0;
        strcpy(sscscpTmpPtr->TroubleLogFile,             "PacsTrouble.log");
        sscscpTmpPtr->TruncateFieldNames                 = 0;
        sscscpTmpPtr->TruncateFieldNamesLen              = -1;
        strcpy(sscscpTmpPtr->UIDPrefix,                  "1.2.826.0.1.3680043.2.135.1066");
        strcpy(sscscpTmpPtr->UIDToCDRIDTableName,        "UIDToCDRID");
        strcpy(sscscpTmpPtr->UIDToMOPIDTableName,        "UIDToMOPID");
//        sscscpTmpPtr->UseBuiltInDecompressor             = TRUE;//Unused!
        sscscpTmpPtr->UseBuiltInJPEG                     = TRUE;
        sscscpTmpPtr->UseEscapeStringConstants           = 0;
        strcpy(sscscpTmpPtr->UserLogFile,                "PacsUser.log");
        strcpy(sscscpTmpPtr->UserName,                   "Administrator");
#ifdef HAVE_BOTH_J2KLIBS
        sscscpTmpPtr->UseOpenJPG                         = 0;
#endif
        sscscpTmpPtr->monitorfolder2[0]                  = 0;//WatchFolder
        sscscpTmpPtr->WebCodeBase[0]                     = 0;
        sscscpTmpPtr->WebMAG0Address[0]                  = 0;
        sscscpTmpPtr->WebPush                            = TRUE;
        sscscpTmpPtr->WebReadOnly                        = TRUE;
        sscscpTmpPtr->WebScriptAddress[0]                = 0;
        sscscpTmpPtr->WebServerFor[0]                    = 0;
        sscscpTmpPtr->WorkListMode                       = 0;
        sscscpTmpPtr->WorkListReturnsISO_IR_100          = -1;
        sscscpTmpPtr->WorkListReturnsISO_IR              = 100;
        strcpy(sscscpTmpPtr->WorkListTableName,          "DICOMWorkList");
//        sscscpTmpPtr->ziptime[0]                         = 0;
        memset(&(sscscpTmpPtr->ziptime), 0, sizeof(sscscpTmpPtr->ziptime));
// Add the arrays
        sscscpTmpPtr->CACHEDevicePtr                     = NULL;
//        sscscpTmpPtr->CDRDevicePtr                       = NULL;//Unused
        sscscpTmpPtr->JUKEBOXDevicePtr                   = NULL;
        sscscpTmpPtr->MIRRORDevicePtr                    = NULL;
//        sscscpTmpPtr->MOPDevicePtr                       = NULL;//Unused
        iNum = 0;//Default for new array
        sscscpTmpPtr->VirtualServerForPtr                = NULL;
        sscscpTmpPtr->VirtualServerPerSeriesPtr          = NULL;
        sscscpTmpPtr->iniTablePtr                        = NULL;
// And the export import groups
        sscscpTmpPtr->ArchivePtr                         = NULL;
        sscscpTmpPtr->CompressionPtr                     = NULL;
        sscscpTmpPtr->ExportPtr                          = NULL;
        sscscpTmpPtr->ImportPtr                          = NULL;
        sscscpTmpPtr->MergeSeriesPtr                     = NULL;
        sscscpTmpPtr->MergeStudiesPtr                    = NULL;
        sscscpTmpPtr->ModalityWorkListQueryResultPtr     = NULL;
        sscscpTmpPtr->MoveDevicePtr                      = NULL;
        sscscpTmpPtr->QueryResultPtr                     = NULL;
        sscscpTmpPtr->QueryPtr                           = NULL;
        sscscpTmpPtr->RetrieveResultPtr                  = NULL;
        sscscpTmpPtr->RetrievePtr                        = NULL;
        sscscpTmpPtr->RejectedImageWorkListPtr           = NULL;
        sscscpTmpPtr->RejectedImagePtr                   = NULL;
        sscscpTmpPtr->WorkListQueryPtr                   = NULL;
}

BOOL IniValue        ::       DeleteNameFromIniFile(const char *theName)
{
        char lName[2 * SHORT_NAME_LEN], *inWhat;
        
        strcpylim(lName, theName, SHORT_NAME_LEN);
        inWhat = strchr(lName, NAME_SECTION_SEPERATOR);
        if(inWhat != NULL) *inWhat++ = 0;//Break into two strings
        else inWhat = sscscpPtr->MicroPACS;
        return DeleteNameFromIniFile(lName, inWhat);
}

BOOL IniValue        ::       DeleteNameFromIniFile(const char *theName, const char *theSection)
{
// Check all inputs and get the file.
        if(theName == NULL || theSection == NULL ||
           strnlen(theName, SHORT_NAME_LEN) > SHORT_NAME_LEN - 1 ||
           strnlen(theSection, SHORT_NAME_LEN) > SHORT_NAME_LEN - 1)
                return errorToBuffer("Input size error");
        if(!readIniFile()) return false;
// and a tempory file.
//        if(!openNewConfigFile()) return false;
// Look for the section.
        while(goToNextSection() && !strnieq(iniStringsPtr->currentSection, theSection, sizeof(iniStringsPtr->currentSection)));
        if(i < iSize)
        {
                if(goToStartOfName(theName))
                {
                        size_t iName;
                        // Open a tempory file.
                        if(!openNewConfigFile())
                        {
                                if(pSrc)free(pSrc);
                                pSrc = NULL;
                                return false;
                        }
                // Write until the line before the name
                        if(!writeByPutParms("Written")) return false;
                        iName = i;//Save name start.
                        i = 0;//Reset
                        if(!keepLine(pSrc)) goToNextDataLine();// Check the first line.
                        if(!writeTonewConfigFile(&pSrc[i], lastLine - i)) return false;
                        if(keepLine(&pSrc[lastLine]) && !writeTonewConfigFile(&pSrc[lastLine], iName - lastLine)) return false;
                        i = iName;// Set i to the start of the line to delete
                        if(goToNextDataLine())// Skip the line and test for more data to write.
                                if(!writeTonewConfigFile(&pSrc[i], iSize - i)) return false;
                        return swapConfigFiles();
                }
                if(pSrc)free(pSrc);
                pSrc = NULL;
                return errorToBuffer("Could not find name");
        }
        if(pSrc)free(pSrc);
        pSrc = NULL;
        return errorToBuffer("Could not find section");
}

// This is the print section for testing.
//Set dumpIni true in the Ini file to dump and quit.
#define sscstprint(a) fprintf(outputFile,#a":\t%s\n",sscscpPtr->a);
#define luastprint(a) fprintf(outputFile,#a":\t%s\n",luaPtr->a);
#define sscstprint2(a,b) fprintf(outputFile,#a" \""#b"\":\t%s\n",sscscpPtr->a);
#define ssciprint(a) fprintf(outputFile,#a":\t%d\n",sscscpPtr->a);
#define ssciprint2(a,b) fprintf(outputFile,#a" \""#b"\":\t%d\n",sscscpPtr->a);
#define sscbprint(a) if(sscscpPtr->a)fprintf(outputFile,#a":TRUE\n");else fprintf(outputFile,#a":FALSE\n");
#define ssctprint(a) dumpTime(outputFile, &(sscscpPtr->a), #a);
#define webstprint(a) fprintf(outputFile,#a":\t%s\n",webdefaultsPtr->a);
#define wadostprint(a) fprintf(outputFile,#a":\t%s\n",wadoserversPtr->a);

void IniValue        ::       dumpCharArray(FILE *outputFile, Array < char    * > *theArray, const char *who)
{
        if(theArray == NULL) return;
        unsigned int tSize, cnt;
        tSize = theArray->GetSize();
        for (cnt = 0; cnt < tSize; cnt++)
        {
                char *charData = theArray->Get(cnt);
                fprintf(outputFile,"%s%d = %s\n", who, cnt, charData);
        }
}

void IniValue        ::       dumpIEGroup(FILE *outputFile, iEGroup *theGroup, const char *who)
{
        if(theGroup == NULL) return;
        fprintf(outputFile,"*** %s Group ***\n", who);
        dumpCharArray(outputFile, theGroup->CalledAEPtr, "CalledAE");
        dumpCharArray(outputFile, theGroup->CallingAEPtr, "CallingAE");
        dumpCharArray(outputFile, theGroup->ConverterPtr, "Converter");
        dumpCharArray(outputFile, theGroup->ModalityPtr, "Modality");
        dumpCharArray(outputFile, theGroup->StationNamePtr, "StationName");
}

// This dumps all values.  All new must be added here.
//lua value add
//lua array add
//sscscp value add
//sscscp array add
void IniValue        ::       DumpIni(FILE *outputFile)
{
        strcpylim(errorBufferPtr, "DumpIni set to true, dgate exiting!", sizeof(iniStringsPtr->errorBuffer));
        fprintf(outputFile,"*** A dump of the %s file. ***\n", iniStringsPtr->iniFile);
        fprintf(outputFile,"     with a working directory of %s.\n", iniStringsPtr->basePath);
        fprintf(outputFile,"*** sscscp section ***:\n");
        sscstprint(MicroPACS)
        ssciprint(Edition)
//        sscstprint(ACPipeName) //AccessUpdates unenabled!
        sscstprint(ACRNemaMap)
        sscstprint(AllowTruncate)
        sscstprint(ArchiveCompression)
        sscbprint(CacheVirtualData)
        ssciprint(db_type)
        ssciprint2(DebugVRs, DebugLevel)
        sscstprint(Dictionary)
        //      sscstprint(DMarkTableName) //Unused!
        sscstprint(DroppedFileCompression)
        sscbprint(DoubleBackSlashToDB)
        sscbprint(EnableReadAheadThread)
        sscbprint(EnableComputedFields)
        ssciprint(FailHoldOff)
        ssciprint(FileCompressMode) //Obsolete
        sscstprint(FileNameSyntaxString)
        sscbprint(FixKodak)
        sscbprint(FixPhilips)
        ssciprint2(Level,ForwardAssociationLevel)
        ssciprint(ForwardAssociationCloseDelay)
        ssciprint(ForwardAssociationRefreshDelay)
        sscbprint(ForwardAssociationRelease)
        ssciprint(ForwardCollectDelay)
        sscbprint(IgnoreMAGDeviceThreshHold)
        sscbprint(IgnoreOutOfMemoryErrors)
        sscstprint(ImageQuerySortOrder)
        sscstprint(ImageTableName)
        sscbprint(ImportExportDragAndDrop)
        sscstprint(IncomingCompression)
        ssciprint(IndexDBF)
#ifdef DGJAVA
        sscstprint(JavaHapiDirectory)
#endif
        sscstprint(kFactorFile)
        ssciprint(LongQueryDBF)
        ssciprint(LossyQuality)
        sscstprint(LRUSort)
        ssciprint(MAGDeviceFullThreshHold)
        ssciprint(MAGDeviceThreshHold)
        ssciprint(MaximumDelayedFetchForwardRetries)
        ssciprint(MaximumExportRetries)
        ssciprint(MaxFieldLength)
        ssciprint(MaxFileNameLength)
        sscstprint(MyACRNema)
        ssciprint(NightlyCleanThreshhold)
        sscbprint(NoDicomCheck)
        sscstprint2(OCPipeName, OperatorConsole)
        ssciprint(OverlapVirtualGet)
        sscbprint(PackDBF)
        sscstprint2(ServerName, PACSName)
//        sscbprint(ConfigPadAEWithZeros) bcb 130328
        sscstprint(Password)
        sscstprint(PatientQuerySortOrder)
        sscstprint(PatientTableName)
        ssciprint(Prefetcher)
        ssciprint(QueueSize)
        //      sscstprint(RegisteredMOPDeviceTable)//Unused
        sscstprint(RegisteredMOPIDTableName)
        ssciprint(RetryDelay)
        sscbprint(RetryForwardFailed)
        sscbprint(SendUpperCaseAE)
        sscstprint(SeriesQuerySortOrder)
        sscstprint(SeriesTableName)
        sscstprint2(SOPClassFile, SOPClassList)
        sscstprint2(DataHost, SQLHost)
        sscstprint2(DataSource, SQLServer)
        ssciprint(StorageFailedErrorCode)
        sscstprint(StudyQuerySortOrder)
        sscstprint(StudyTableName)
        ssciprint(TCPIPTimeOut)
        sscstprint2(Port, TCPPort)
        sscstprint(TempDir)
        //      sscbprint(UseBuiltInDecompressor) //Unused!
        ssciprint(TruncateFieldNames)
        ssciprint(TruncateFieldNamesLen)
        sscbprint(UseBuiltInJPEG)
        sscbprint(UseEscapeStringConstants)
#ifdef HAVE_BOTH_J2KLIBS
        sscbprint(UseOpenJPG)
#endif
        sscstprint(UserName)//Username
        sscstprint(UIDPrefix)
        sscstprint2(UIDToCDRIDTableName, UIDToCDRIDTable)
        sscstprint2(UIDToMOPIDTableName, UIDToMOPIDTable)
        sscstprint2(monitorfolder2, WatchFolder)
        sscstprint(WebCodeBase)
        sscstprint(WebMAG0Address)
        sscbprint(WebPush)
        sscbprint(WebReadOnly)
        sscstprint(WebScriptAddress)
        sscstprint(WebServerFor)
        ssciprint(WorkListMode)
        ssciprint(WorkListReturnsISO_IR)
        ssciprint(WorkListReturnsISO_IR_100)
        sscstprint(WorkListTableName)
        ssctprint(ziptime)
        fprintf(outputFile,"**** Ini arrays. ****\n");
        dumpCharArray(outputFile, sscscpPtr->CACHEDevicePtr, "CACHEDevice");
        //        dumpCharArray(outputFile, sscscpPtr->CDRDevicePtr, "CDRDevice");//Unused
        dumpCharArray(outputFile, sscscpPtr->JUKEBOXDevicePtr, "JUKEBOXDevice");
        dumpCharArray(outputFile, sscscpPtr->MAGDevicePtr, "MAGDevice");
        dumpCharArray(outputFile, sscscpPtr->MIRRORDevicePtr, "MIRRORDevice");
        //        dumpCharArray(outputFile, sscscpPtr->MOPDevicePtr, "MOPDevice");//Unused
        dumpCharArray(outputFile, sscscpPtr->VirtualServerForPtr, "VirtualServerFor");
        dumpIntArray(outputFile, sscscpPtr->VirtualServerPerSeriesPtr, "VirtualServerPerSeries");
        if(sscscpPtr->ArchivePtr != NULL || sscscpPtr->CompressionPtr != NULL || sscscpPtr->ExportPtr != NULL || sscscpPtr->ImportPtr != NULL ||
           sscscpPtr->MergeSeriesPtr != NULL || sscscpPtr->MergeStudiesPtr != NULL || sscscpPtr->ModalityWorkListQueryResultPtr != NULL
           || sscscpPtr->MoveDevicePtr != NULL || sscscpPtr->QueryResultPtr != NULL || sscscpPtr->QueryPtr != NULL || sscscpPtr->RetrieveResultPtr != NULL
           || sscscpPtr->RetrievePtr != NULL || sscscpPtr->RejectedImageWorkListPtr != NULL || sscscpPtr->RejectedImagePtr != NULL ||
           sscscpPtr->WorkListQueryPtr != NULL )fprintf(outputFile,"**** Import export arrays. ****\n");
        dumpIEGroup(outputFile, sscscpPtr->ArchivePtr, "Archive");
        dumpIEGroup(outputFile, sscscpPtr->CompressionPtr, "Compression");
        dumpIEGroup(outputFile, sscscpPtr->ExportPtr, "Export");
        dumpIEGroup(outputFile, sscscpPtr->ImportPtr, "Import");
        dumpIEGroup(outputFile, sscscpPtr->MergeSeriesPtr, "MergeSerie");
        dumpIEGroup(outputFile, sscscpPtr->MergeStudiesPtr, "MergeStudies");
        dumpIEGroup(outputFile, sscscpPtr->ModalityWorkListQueryResultPtr, "ModalityWorkListQueryResult");
        dumpIEGroup(outputFile, sscscpPtr->MoveDevicePtr, "MoveDevice");
        dumpIEGroup(outputFile, sscscpPtr->QueryResultPtr, "QueryResult");
        dumpIEGroup(outputFile, sscscpPtr->QueryPtr, "Query");
        dumpIEGroup(outputFile, sscscpPtr->RetrieveResultPtr, "RetrieveResult");
        dumpIEGroup(outputFile, sscscpPtr->RetrievePtr, "Retrieve");
        dumpIEGroup(outputFile, sscscpPtr->RejectedImageWorkListPtr, "RejectedImageWorkList");
        dumpIEGroup(outputFile, sscscpPtr->RejectedImagePtr, "RejectedImage");
        dumpIEGroup(outputFile, sscscpPtr->WorkListQueryPtr, "WorkListQuery");
        unknownPrint(sscscpPtr->iniTablePtr, outputFile);
        sscstprint(AnyPage)
        if(AnyPagePtr != NULL) dumpsourceLines(outputFile, AnyPagePtr, "AnyPage");
        sscstprint(DefaultPage);
        if(DefaultPagePtr != NULL) dumpsourceLines(outputFile, DefaultPagePtr, "DefaultPage");
        if(luaPtr != NULL)
        {
                fprintf(outputFile,"\n**** lua section ****\n");
                luastprint(association)
                luastprint(background)
                luastprint(clienterror)
                luastprint(endassociation)
                luastprint(nightly)
                luastprint(startup)
                fprintf(outputFile,"*** Import export arrays. ***\n");
                dumpCharArray(outputFile, luaPtr->ExportConverterPtr, "ExportConvter");
                dumpIEGroup(outputFile, luaPtr->ArchivePtr,"Archive");
                dumpIEGroup(outputFile, luaPtr->CompressionPtr,"Compression");
                dumpIEGroup(outputFile, luaPtr->ImportPtr,"Import");
                dumpIEGroup(outputFile, luaPtr->MergeSeriesPtr,"MergeSeries");
                dumpIEGroup(outputFile, luaPtr->MergeStudiesPtr,"MergeStudies");
                dumpIEGroup(outputFile, luaPtr->ModalityWorkListQueryResultPtr,"ModalityWorkListQueryResult");
                dumpIEGroup(outputFile, luaPtr->MoveDevicePtr,"MoveDevice");
                dumpIEGroup(outputFile, luaPtr->QueryResultPtr,"QueryResult");
                dumpIEGroup(outputFile, luaPtr->QueryPtr,"Query");
                dumpIEGroup(outputFile, luaPtr->RetrieveResultPtr,"RetrieveResult");
                dumpIEGroup(outputFile, luaPtr->RetrievePtr,"Retrieve");
                dumpIEGroup(outputFile, luaPtr->RejectedImageWorkListPtr,"RejectedImageWorkList");
                dumpIEGroup(outputFile, luaPtr->RejectedImagePtr,"RejectedImage");
                dumpIEGroup(outputFile, luaPtr->WorkListQueryPtr,"WorkListQuery");
                unknownPrint(luaPtr->iniTablePtr, outputFile);
        }
        if(markseriesPtr != NULL) dumpsourceLines(outputFile, markseriesPtr, "markseries");
        if(markstudyPtr != NULL) dumpsourceLines(outputFile, markstudyPtr, "markstudy");
        if(shoppingcartPtr != NULL) dumpsourceLines(outputFile, shoppingcartPtr, "shoppingcart");
        if(scriptsPtr != NULL)
        {
                fprintf(outputFile,"\n**** Scripts section ****\n");
                unknownPrint(scriptsPtr, outputFile);
        }
        if(wadoserversPtr != NULL )
        {
                fprintf(outputFile,"\n**** WadoServers section ****\n");
                wadostprint(bridge);
                unknownPrint(wadoserversPtr->iniTablePtr, outputFile);
        }
        if(webdefaultsPtr != NULL )
        {
                fprintf(outputFile,"\n**** WebDefaults section ****\n");
                webstprint(address);
                webstprint(compress);
                webstprint(dsize);
                webstprint(graphic);
                webstprint(iconsize);
                webstprint(port);
                webstprint(size);
                webstprint(viewer);
                webstprint(studyviewer);
                unknownPrint(webdefaultsPtr->iniTablePtr, outputFile);
        }
        if(unknownSectionPtr != NULL)
        {
                unsigned int cnt, aSize;
                
                fprintf(outputFile,"\n**** Unknoun section(s) ****\n");
                aSize = unknownSectionPtr->GetSize();
                sectionTable *unknownTable;
                for (cnt = 0; cnt < aSize; cnt++)
                {
                        unknownTable = unknownSectionPtr->Get(cnt);
                        fprintf(outputFile,"%s section\n", unknownTable->section);
                        unknownPrint(unknownTable->iniTablePtr, outputFile);
                }
        }
}

void IniValue        ::       dumpIntArray(FILE *outputFile, Array < int    * > *theArray, const char *who)
{
        if(theArray == NULL) return;
        unsigned int tSize, cnt;
        tSize = theArray->GetSize();
        for (cnt = 0; cnt < tSize; cnt++)
        {
                int *intDataPtr = theArray->Get(cnt);
                fprintf(outputFile,"%s%d = %d\n", who, cnt, *intDataPtr);
        }
}

void IniValue        ::       dumpsourceLines(FILE *outputFile, sourceLines *theSL, const char *who)
{
        fprintf(outputFile,"\n**** %s Section ****\n", who);
        if(theSL->caption[0] != 0) fprintf(outputFile,"caption:%s\n",theSL->caption);
        if(theSL->header[0] != 0) fprintf(outputFile,"header:%s\n",theSL->header);
//        if(theSL->sourceOrLine0) fprintf(outputFile,"Has Source or Line0 values\n");//Killed lines and sourceOrLine0.
        else  fprintf(outputFile,"Has no Source or Lines values\n");
        if(theSL->source[0] != 0) fprintf(outputFile,"source:%s\n",theSL->source);
//        dumpCharArray(outputFile, theSL->linesPtr, "line");//Killed lines and sourceOrLine0.
        unknownPrint(theSL->iniTablePtr, outputFile);
        fprintf(outputFile,"\n");
}

void IniValue        ::       dumpTime(FILE *outputFile, struct tm *timeStruct, const char *who)
{
        if(timeStruct->tm_hour == 0 && timeStruct->tm_min == 0 && timeStruct->tm_sec == 0) return;
        fprintf(outputFile,"%s:%02d:%02d:%02d\n", who, timeStruct->tm_hour, timeStruct->tm_min, timeStruct->tm_sec);
}

void IniValue        ::       errorCleanNewConfigFile()
{
        fclose(tempFile);
        unlink(iniStringsPtr->newConfigFile);
        if(pSrc)free(pSrc);
        pSrc = NULL;
}

BOOL IniValue        ::      errorToBuffer(const char *errorMsg)
{
        strcpylim(errorBufferPtr, errorMsg, sizeof(iniStringsPtr->errorBuffer));
        return false;
}

void IniValue        ::      freeCharArray(Array<char *> **arrayHand)
{
        
        if( arrayHand == NULL || *arrayHand == NULL ) return;
        while ((*arrayHand)->GetSize())
        {
                char *alloMem;
                alloMem = (*arrayHand)->Get(0);
                if(alloMem) free(alloMem);
                (*arrayHand)->RemoveAt(0);
        }
        free(*arrayHand);
        *arrayHand = NULL;
}

void IniValue        ::      freeIEGroup(iEGroup **theGroupH)
{
        
        if( theGroupH == NULL || *theGroupH == NULL ) return;
        freeCharArray(&((*theGroupH)->CalledAEPtr));
        freeCharArray(&((*theGroupH)->CallingAEPtr));
        freeCharArray(&((*theGroupH)->ConverterPtr));
        freeCharArray(&((*theGroupH)->FilterPtr));
        freeCharArray(&((*theGroupH)->ModalityPtr));
        freeCharArray(&((*theGroupH)->StationNamePtr));
        free(*theGroupH);
        *theGroupH = NULL;
}

void IniValue        ::      freeIntArray(Array<int *> **arrayHand)
{
        
        if( arrayHand == NULL || *arrayHand == NULL ) return;
        while ((*arrayHand)->GetSize())
        {
                int *intPtr;
                intPtr = (*arrayHand)->Get(0);
                if(intPtr) delete intPtr;
                (*arrayHand)->RemoveAt(0);
        }
        free(*arrayHand);
        *arrayHand = NULL;
}

void IniValue        ::      freeLua()
{
        if( luaTmpPtr == NULL ) return;
        freeCharArray(&(luaTmpPtr->ExportConverterPtr));
        freeCharArray((Array<char *> **)&(luaTmpPtr->iniTablePtr));
        freeIEGroup(&(luaTmpPtr->ArchivePtr));
        freeIEGroup(&(luaTmpPtr->CompressionPtr));
        freeIEGroup(&(luaTmpPtr->ImportPtr));
        freeIEGroup(&(luaTmpPtr->MergeSeriesPtr));
        freeIEGroup(&(luaTmpPtr->MergeStudiesPtr));
        freeIEGroup(&(luaTmpPtr->ModalityWorkListQueryResultPtr));
        freeIEGroup(&(luaTmpPtr->MoveDevicePtr));
        freeIEGroup(&(luaTmpPtr->QueryResultPtr));
        freeIEGroup(&(luaTmpPtr->QueryPtr));
        freeIEGroup(&(luaTmpPtr->RetrieveResultPtr));
        freeIEGroup(&(luaTmpPtr->RetrievePtr));
        freeIEGroup(&(luaTmpPtr->RejectedImageWorkListPtr));
        freeIEGroup(&(luaTmpPtr->RejectedImagePtr));
        freeIEGroup(&(luaTmpPtr->WorkListQueryPtr));
        free(luaTmpPtr);
        luaTmpPtr = NULL;
}

// Clear the tempory arrays, so it must be set to clear the main array (shutdown).
//sscscp array add
void IniValue        ::       freesscscp()
{
        if( sscscpTmpPtr != NULL)
        {
                freeCharArray(&(sscscpTmpPtr->CACHEDevicePtr));
                //              freeCharArray(&(sscscpTmpPtr->CDRDevicePtr));//Unused
                freeCharArray(&(sscscpTmpPtr->JUKEBOXDevicePtr));
                freeCharArray(&(sscscpTmpPtr->MAGDevicePtr));
                freeCharArray(&(sscscpTmpPtr->MIRRORDevicePtr));
                //              freeCharArray(&(sscscpTmpPtr->MOPDevicePtr));//Unused
                freeCharArray(&(sscscpTmpPtr->VirtualServerForPtr));
                freeIntArray(&(sscscpTmpPtr->VirtualServerPerSeriesPtr));
                freeIEGroup(&(sscscpTmpPtr->ArchivePtr));
                freeIEGroup(&(sscscpTmpPtr->CompressionPtr));
                freeIEGroup(&(sscscpTmpPtr->ExportPtr));
                freeIEGroup(&(sscscpTmpPtr->ImportPtr));
                freeIEGroup(&(sscscpTmpPtr->MergeSeriesPtr));
                freeIEGroup(&(sscscpTmpPtr->MergeStudiesPtr));
                freeIEGroup(&(sscscpTmpPtr->ModalityWorkListQueryResultPtr));
                freeIEGroup(&(sscscpTmpPtr->MoveDevicePtr));
                freeIEGroup(&(sscscpTmpPtr->QueryResultPtr));
                freeIEGroup(&(sscscpTmpPtr->QueryPtr));
                freeIEGroup(&(sscscpTmpPtr->RetrieveResultPtr));
                freeIEGroup(&(sscscpTmpPtr->RetrievePtr));
                freeIEGroup(&(sscscpTmpPtr->RejectedImageWorkListPtr));
                freeIEGroup(&(sscscpTmpPtr->RejectedImagePtr));
                freeIEGroup(&(sscscpTmpPtr->WorkListQueryPtr));
                
                free(sscscpTmpPtr);
                sscscpTmpPtr = NULL;
        }
}

void IniValue        ::      freeSourceLines(sourceLines **sourceLinesHandle)
{
        if( sourceLinesHandle == NULL || *sourceLinesHandle == NULL ) return;
        freeCharArray((Array<char *> **)&((*sourceLinesHandle)->iniTablePtr));
//        freeCharArray((Array<char *> **)&((*sourceLinesHandle)->linesPtr));//Killed lines and sourceOrLine0.
        free(*sourceLinesHandle);
        *sourceLinesHandle = NULL;
}

void IniValue        ::      freesection()
{
        if( unknownSectionTmpPtr == NULL ) return;
        
        sectionTable *theSectionPtr;
        while( unknownSectionTmpPtr->GetSize() )
        {
                theSectionPtr = unknownSectionTmpPtr->Get(0);
                freeCharArray((Array<char *> **)&(theSectionPtr->iniTablePtr));
                free(theSectionPtr);
                unknownSectionTmpPtr->RemoveAt(0);
        }
        unknownSectionTmpPtr = NULL; 
}

void IniValue        ::      freewadoservers()
{
        if( wadoserversTmpPtr == NULL ) return;
        freeCharArray((Array<char *> **)&(wadoserversTmpPtr->iniTablePtr));
        free(wadoserversTmpPtr);
        wadoserversTmpPtr = NULL;
}

void IniValue        ::      freewebdefaults()
{
        if( webdefaultsTmpPtr == NULL ) return;
        freeCharArray((Array<char *> **)&(webdefaultsTmpPtr->iniTablePtr));
        free(webdefaultsTmpPtr);
        webdefaultsTmpPtr = NULL;
}

BOOL IniValue        ::       getBoolWithTemp()
{
        BOOL testBool = FALSE;
        iVoidPtr = &testBool;
        iVoidHandle = &iVoidPtr;
        setIniBool();
        return testBool;
}

char* IniValue        ::       getIniBoolString()
{
        char returnStr[VSHORT_NAME_LEN];
        
        if(iVoidHandle == NULL || *iVoidHandle == NULL) return mallocReturnBuf(NULL);
        snprintf(returnStr, sizeof(returnStr), "%d", **(BOOL **)iVoidHandle);
        return mallocReturnBuf(returnStr);
}

char* IniValue        ::       getIniInitFromArray(Array<int *> **arrayHandle)
{
        char returnStr[BUF_CMD_LEN];
        int arraySize;
        
        if((arrayHandle == NULL || *arrayHandle == NULL) && iNum == -1)
                return mallocReturnBuf("0");//An array that is null has zero members.
        if(iNum < -1 || arrayHandle == NULL || *arrayHandle == NULL) return mallocReturnBuf(NULL);//Check for real.
        arraySize = (int)(*arrayHandle)->GetSize();//The array is real, how big?
        if(iNum == -1)snprintf(returnStr, sizeof(returnStr), "%d", arraySize);
        else
        {
                if(iNum < arraySize)
                {
                        snprintf(returnStr, sizeof(returnStr), "%d",*((*arrayHandle)->Get(iNum)));
                }
                else snprintf(returnStr, sizeof(returnStr), "Error: %d is greater the the group size of %d", iNum + 1, arraySize);
        }
        return mallocReturnBuf(returnStr);
}

char* IniValue        ::       getIniInitFromArray(Array<int *> ***arrayDHandle)
{
        
        if(arrayDHandle == NULL) return mallocReturnBuf(NULL);
        return getIniInitFromArray(*arrayDHandle);
}

int* IniValue        ::       GetIniInitPtr(const char *theName, const char *theSection)
{
        char lName[SHORT_NAME_LEN];
        int *returnPtr;
        
        iNum = 0;
        // The compare functions expects a " =" in the name for parsing, add it.
        strcpylim(lName, theName, SHORT_NAME_LEN - 2);
        strcat(lName, " =");
        pSrc = lName;
        i = 0;
        iSize = strnlen(lName, SHORT_NAME_LEN);
        iVoidPtr = NULL;
        iVoidHandle = &iVoidPtr;
        returnPtr = NULL;
        
        if (strieq(theSection, sscscpPtr->MicroPACS))
        {
                if(chechThreadError( sscscpTmpPtr )) return NULL;
                sscscpTmpPtr = sscscpPtr;//Needed for read.
                iNum = 0;
                switch(getTypeForsscscp())
                {
                        case DPT_BOOL:
                        case DPT_INT:
                        case DPT_UINT:
                                returnPtr = (int *)*iVoidHandle;
                                break;
                        case DPT_INT_ARRAY:
                        case DPT_UINT_ARRAY:
                                if(iVoidHandle != NULL && *iVoidHandle != NULL && **(Array<int *> ***)iVoidHandle != NULL
                                   && iNum >= 0 && iNum < (int)(**(Array<char *> ***)iVoidHandle)->GetSize())
                                        returnPtr = ((**(Array<int *> ***)iVoidHandle)->Get(iNum));
                                break;
                        case DPT_DBTYPE:
                                returnPtr = (int *)&(sscscpPtr->db_type);
                                break;
                        case DPT_EDITION:
                                returnPtr = (int *)&(sscscpPtr->Edition);
                                break;
                        case DPT_FOR_ASSOC:
                                returnPtr = (int *)&(sscscpPtr->Level);
                                break;
                         case DPT_FILE_COMP:
                                returnPtr = (int *)&(sscscpTmpPtr->FileCompressMode);
                                break;
                        case DPT_UNUSED:
                        case DPT_STRING:
                        case DPT_PATH:
                        case DPT_STRING_ARRAY:
                        case DPT_END:
                        case DPT_MICROP:
                        case DPT_SKIP:
                        case DPT_TIME:
                        case DPT_UNKNOWN:
                                break;
#ifdef DGJAVA
                        case DPT_MIN_JAVA:
                                returnPtr = (int *)&(sscscpPtr->JavaMinVersion);
                                break;
#endif
                }
                sscscpTmpPtr = NULL;//Set back to NULL
        }
        else
        {//The only sourceLines int is the bcb created BOOL sourceOrLine0 to tell the existance of the line "source" 0r "Line0".
                sourceLines *tempPtr;
                
                iVoidHandle = NULL;
                if(strieq(theSection, sscscpPtr->AnyPage) || strileq(theSection, "anypage"))
                {
                        iVoidHandle = (void **)&AnyPagePtr;
                        tempPtr = AnyPageTmpPtr;
                }
                else if(strieq(theSection, sscscpPtr->DefaultPage) || strileq(theSection, "defaultpage"))
                {
                        iVoidHandle = (void **)&DefaultPagePtr;
                        tempPtr = DefaultPageTmpPtr;
                }
                else if(strileq(theSection, "markseries"))
                {
                        iVoidHandle = (void **)&markseriesPtr;
                        tempPtr = markseriesTmpPtr;
                }
                else if(strileq(theSection, "markstudy"))
                {
                        iVoidHandle = (void **)&markstudyPtr;
                        tempPtr = markstudyTmpPtr;
                }
                else if(strileq(theSection, "shoppingcart"))
                {
                        iVoidHandle = (void **)&shoppingcartPtr;
                        tempPtr = shoppingcartTmpPtr;
                }
                if(iVoidHandle != NULL)
                {
                        //We have a reconized section, but doe it esist?
                        
                        if(*iVoidHandle != NULL)
                        {
                                if(chechThreadError( tempPtr )) return NULL;
                                //We have the selected section. Read it.  No tmpPtr used, so just return.
                                iNum = 0;//Changed to 1 if string lable is source.
                                //Create pointer and handles that we can pass and destroy.
                                iVoidPtr = *iVoidHandle;
                                iVoidHandle = &iVoidPtr;
                                iVoidDHandle = &iVoidHandle;
                                switch(getTypeForsourceLines())
                                {
                                        case DPT_BOOL:
                                                returnPtr = (int *)*iVoidHandle;
                                                break;
                                        case DPT_STRING:
                                        case DPT_STRING_ARRAY:
                                        case DPT_UNKNOWN:
                                        case DPT_UNUSED:
                                        case DPT_END:
                                        case DPT_SKIP:
                                        case DPT_INT_ARRAY:
                                        case DPT_UINT_ARRAY:
                                        case DPT_DBTYPE:
                                        case DPT_EDITION:
                                        case DPT_FOR_ASSOC:
                                        case DPT_FILE_COMP:
                                        case DPT_MICROP:
                                        case DPT_INT:
                                        case DPT_TIME:
                                        case DPT_UINT:
                                        case DPT_PATH:
#ifdef DGJAVA
                                        case DPT_MIN_JAVA:
#endif
                                                break;
                                                
                                }
                        }
                }
        }
        pSrc = NULL;
        return returnPtr;
}

char* IniValue        ::       GetIniLine(const char *lineNum)
{
        int cnt, lNum;
        size_t lineLen;
        char *returnBuf = NULL;
        
        if(lineNum == NULL)return NULL;
        lNum = atoi(lineNum);
        if(lNum < 0 || !readIniFile()) return NULL;
// Go to the line.
        for (cnt = 1; cnt < lNum; cnt++)
        {
                while(pSrc[i] != '\r' && pSrc[i] != '\n')
                {
                        i++;//Go to the end.
                        if(i >= iSize)
                        {
                                if(pSrc) free(pSrc);
                                pSrc = NULL;
                                return NULL;
                        }
                }
                if(pSrc[i] == '\r' && pSrc[i + 1] == '\n') i += 2;
                else if(pSrc[i] == '\n' && pSrc[i + 1] == '\r') i += 2;
                else i++;
        }
//Have a start of the line
        for (lineLen = i; lineLen < iSize; lineLen++)
                if(pSrc[lineLen] == '\r' ||pSrc[lineLen] == '\n') break;
        if(lineLen > i)
        {// Have a line length.
                lineLen -= i;
                if((returnBuf = (char *)malloc(1 + lineLen)) != NULL)
                        strcpylim(returnBuf, &pSrc[i], lineLen);
        }
        if(pSrc) free(pSrc);
        pSrc = NULL;
        return returnBuf;
}

char* IniValue        ::       getIniString()
{
        if(iVoidHandle == NULL) return mallocReturnBuf(NULL);
        return mallocReturnBuf((char *)*iVoidHandle);
}

// A -1 is to return a count of the array instead of a value from it.
char* IniValue        ::       getIniStringFromArrayH(Array<char *> **arrayHandle)
{
        char returnStr[BUF_CMD_LEN];
        int arraySize;
        
        if((arrayHandle == NULL || *arrayHandle == NULL) && iNum == -1)
                return mallocReturnBuf("0");//An array that is null has zero members.
        if(iNum < -1 || arrayHandle == NULL || *arrayHandle == NULL) return mallocReturnBuf(NULL);//Check for real.
        arraySize = (int)(*arrayHandle)->GetSize();//The array is real, how big?
        if(iNum == -1)snprintf(returnStr, BUF_CMD_LEN, "%d", arraySize);
        else
        {
                if(iNum < arraySize) return mallocReturnBuf((*arrayHandle)->Get(iNum));
                snprintf(returnStr, BUF_CMD_LEN, "Error: %d is greater the the group size of %d", iNum + 1, arraySize);
        }
        return mallocReturnBuf(returnStr);
}

char* IniValue        ::       getIniStringFromArrayDH(Array<char *> ***arrayDHandle)
{
        
        if(arrayDHandle == NULL) return mallocReturnBuf(NULL);
        return getIniStringFromArrayH(*arrayDHandle);
}

char* IniValue        ::       GetIniString(const char *theName)
{
        char lName[SHORT_NAME_LEN], *inWhat;
        
        strcpylim(lName, theName, SHORT_NAME_LEN);
        inWhat = strchr(lName, NAME_SECTION_SEPERATOR);
        if(inWhat != NULL) *inWhat++ = 0;//Break into two strings
        else inWhat = sscscpPtr->MicroPACS;
        return GetIniString(lName, inWhat);
}

char* IniValue        ::       GetIniString(const char *theName, const char *theSection)
{
        char lName[SHORT_NAME_LEN], returnStr[BUF_CMD_LEN], *returnBuf;

        iNum = 0;
        returnStr[0] = 0;
        returnBuf = NULL;
// The compare functions expects a " =" in the name for parsing, add it.
        strcpylim(lName, theName, SHORT_NAME_LEN - 2);
        strcat(lName, " =");
        pSrc = lName;
        i = 0;
        iSize = strnlen(lName, SHORT_NAME_LEN);
        iVoidPtr = NULL;
        iVoidHandle = &iVoidPtr;

        if (strieq(theSection, sscscpPtr->MicroPACS))
        {
                if(chechThreadError( sscscpTmpPtr )) return (threadError());
                sscscpTmpPtr = sscscpPtr;//Needed for read.
                iNum = 0;
                switch(getTypeForsscscp())
                {
                        case DPT_PATH:
                        case DPT_STRING:
                        case DPT_MICROP:
                                returnBuf = getIniString();
                                break;
                        case DPT_BOOL:
                                returnBuf = getIniBoolString();
                               break;
                        case DPT_INT:
                        case DPT_UINT:
                        case DPT_FILE_COMP:
                                snprintf(returnStr, BUF_CMD_LEN, "%d", *(int *)*iVoidHandle);
                                break;
                        case DPT_UNUSED:
                                strcpy(returnStr, "Unused!");
                                break;
                        case DPT_STRING_ARRAY:
                                returnBuf = getIniStringFromArrayDH((Array<char *> ***)iVoidHandle);
                                break;
                        case DPT_INT_ARRAY:
                        case DPT_UINT_ARRAY:
                                returnBuf = getIniInitFromArray((Array<int *> ***)iVoidHandle);
                                break;
                        case DPT_DBTYPE:
                                strcpy(returnStr, "0");
                                switch ((enum DBTYPE) iNum)
                                {
                                        case DT_MYSQL:
#ifdef USEMYSQL
                                                if(sscscpPtr->db_type == (enum DBTYPE) iNum) strcpy(returnStr, "1");
#else
                                                strcpy(returnStr, "MySQL not enabled");
#endif
                                                break;
                                        case DT_POSTGRES:
#ifdef POSTGRES
                                                if(sscscpPtr->db_type == (enum DBTYPE) iNum)strcpy(returnStr, "1");
#else
                                                strcpy(returnStr, "PostgreSQL not enable");
#endif
                                                break;
                                        case DT_SQLITE:
#ifdef USESQLITE
                                                if(sscscpPtr->db_type == (enum DBTYPE) iNum)strcpy(returnStr, "1");
#else
                                                strcpy(returnStr, "SQLite not enable");
#endif
                                                break;
                                        case DT_DBASEIII://No way to select this!
                                        case DT_DBASEIIINOINDEX://No way to select this!
                                        case DT_NULL://No way to select this!
                                        case DT_ODBC://No way to select this!
                                                strcpy(returnStr, "Not an dicom.ini option");
                                        break;
                                }
                                break;
                        case DPT_EDITION:
                                switch (sscscpPtr->Edition)
                                {
                                        case E_PERSONAL:
                                                strcpy(returnStr, "Personal");
                                                break;
                                        case E_ENTERPRISE:
                                                strcpy(returnStr, "Enterprise");
                                                break;
                                        default:
                                                strcpy(returnStr, "Workgroup");
                                        break;
                                }
                                break;
                        case DPT_END:
                                break;
                        case DPT_FOR_ASSOC:
                                switch (sscscpPtr->Level)
                                {
                                        case LT_PATIENT:
                                                strcpy(returnStr, "Patient");
                                                break;
                                        case LT_STUDY:
                                                strcpy(returnStr, "Study");
                                                break;
                                        case LT_SERIES:
                                                strcpy(returnStr, "Series");
                                                break;
                                        case LT_IMAGE:
                                                strcpy(returnStr, "Image");
                                                break;
                                        case LT_SOPCLASS:
                                                strcpy(returnStr, "Sopclass");
                                                break;
                                        case LT_GLOBAL:
                                                strcpy(returnStr, "Global");
                                                break;
                                        default:
                                                break;
                                }
                                break;
                        case DPT_SKIP:
                                break;
                        case DPT_TIME:
                                returnBuf = getIniTime();
                                break;
                        case DPT_UNKNOWN:
                                returnBuf = intTableValue(sscscpPtr->iniTablePtr, theName);
                                break;
#ifdef DGJAVA
                        case DPT_MIN_JAVA:
                                switch (sscscpPtr->JavaMinVersion)
                                {
                                        case JNI_VERSION_1_1:
                                                strcpy(returnStr, "1.1");
                                                break;
                                        case JNI_VERSION_1_4:
                                                strcpy(returnStr, "1.4");
                                                break;
                                        case JNI_VERSION_1_6:
                                                strcpy(returnStr, "1.6");
                                                break;
                                        default:
                                                strcpy(returnStr, "1.2");
                                                break;
                                }
#endif
                }
                sscscpTmpPtr = NULL;//Set back to NULL
        }
        else
        {
                sourceLines *tempPtr;
                
                iVoidHandle = NULL;
                if(strieq(theSection, sscscpPtr->AnyPage) || strileq(theSection, "anypage"))
                {
                        iVoidHandle = (void **)&AnyPagePtr;
                        tempPtr = AnyPageTmpPtr;
                }
                else if(strieq(theSection, sscscpPtr->DefaultPage) || strileq(theSection, "defaultpage"))
                {
                        iVoidHandle = (void **)&DefaultPagePtr;
                        tempPtr = DefaultPageTmpPtr;
                }
                else if(strileq(theSection, "markseries"))
                {
                        iVoidHandle = (void **)&markseriesPtr;
                        tempPtr = markseriesTmpPtr;
                }
                else if(strileq(theSection, "markstudy"))
                {
                        iVoidHandle = (void **)&markstudyPtr;
                        tempPtr = markstudyTmpPtr;
                }
                else if(strileq(theSection, "shoppingcart"))
                {
                        iVoidHandle = (void **)&shoppingcartPtr;
                        tempPtr = shoppingcartTmpPtr;
                }
                if(iVoidHandle != NULL)
                {
        //We have a reconized section, but doe it esist?

                        if(*iVoidHandle == NULL) snprintf(returnStr, BUF_CMD_LEN, "No %s section", theSection);
                        else
                        {
                                if(chechThreadError( tempPtr )) return (threadError());
        //We have the selected section. Read it.
                                iNum = 0;//Changed to 1 if string lable is source.
        //Create pointer and handles that we can pass and destroy.
                                iVoidPtr = *iVoidHandle;
                                iVoidHandle = &iVoidPtr;
                                iVoidDHandle = &iVoidHandle;
                                switch(getTypeForsourceLines())
                                {
                                        case DPT_BOOL:
                                                returnBuf = getIniBoolString();
                                                break;
                                        case DPT_STRING:
                                                returnBuf = getIniString();
                                                break;
                                        case DPT_STRING_ARRAY:
                                                returnBuf = getIniStringFromArrayDH((Array<char *> ***)iVoidHandle);
                                                break;
                                        case DPT_UNUSED:
                                                strcpy(returnStr, "Unused!");
                                                break;
                                        case DPT_UNKNOWN:
                                                returnBuf = intTableValue(((sourceLines *)iVoidPtr)->iniTablePtr, theName);
                                                break;
                                        case DPT_END:
                                        case DPT_SKIP:
                                        case DPT_INT_ARRAY:
                                        case DPT_UINT_ARRAY:
                                        case DPT_DBTYPE:
                                        case DPT_EDITION:
                                        case DPT_FOR_ASSOC:
                                        case DPT_FILE_COMP:
                                        case DPT_MICROP:
                                        case DPT_INT:
                                        case DPT_TIME:
                                        case DPT_UINT:
                                        case DPT_PATH:
#ifdef DGJAVA
                                        case DPT_MIN_JAVA:
#endif
                                                break;
                                                
                                }
                        }
                }
                else
                {// Not a sourec line section, reset the pointer and handles.
                        iVoidPtr = NULL;
                        iVoidHandle = &iVoidPtr;
                        iVoidDHandle = &iVoidHandle;

                        if(strieq(theSection, "lua"))
                        {
                                if(luaPtr == NULL) strcpy(returnStr, "No lua section");
                                else
                                {
                                        iNum = 0;
                                        if(chechThreadError( luaTmpPtr )) return (threadError());
                                        luaTmpPtr = luaPtr;//Needed for read.

                                        switch(getTypeForLua())
                                        {
                                                case DPT_STRING:
                                                        returnBuf = getIniString();
                                                        break;
                                                case DPT_STRING_ARRAY:
                                                        returnBuf = getIniStringFromArrayDH((Array<char *> ***)iVoidHandle);
                                                        break;
                                                case DPT_UNUSED:
                                                        strcpy(returnStr, "Unused!");
                                                        break;
                                                case DPT_UNKNOWN:
                                                        returnBuf = intTableValue(luaPtr->iniTablePtr, theName);
                                                        break;
                                                case DPT_END:
                                                case DPT_SKIP:
                                                case DPT_INT_ARRAY:
                                                case DPT_UINT_ARRAY:
                                                case DPT_DBTYPE:
                                                case DPT_EDITION:
                                                case DPT_FOR_ASSOC:
                                                case DPT_FILE_COMP:
                                                case DPT_MICROP:
                                                case DPT_INT:
                                                case DPT_UINT:
                                                case DPT_TIME:
                                                case DPT_PATH:
                                                case DPT_BOOL:
#ifdef DGJAVA
                                                case DPT_MIN_JAVA:
#endif
                                                        break;
                                                        
                                        }
                                        luaTmpPtr = NULL;
                                }
                                
                        }
                        else if(strieq(theSection, "scripts"))
                        {
                                if(scriptsPtr == NULL) strcpy(returnStr, "No scripts section");
                                else
                                {
                                        if(chechThreadError( scriptsTmpPtr )) return (threadError());
                                        returnBuf = intTableValue(scriptsPtr, theName);
                                }
                        }
                        else if(strieq(theSection, "wadoservers"))
                        {
                                if(wadoserversPtr == NULL) strcpy(returnStr, "No wado servers section");
                                else
                                {
                                        if(chechThreadError( wadoserversTmpPtr )) return (threadError());
                                        if(strileq(lName, "bridge"))
                                        {
                                                if(wadoserversPtr->bridge[0] == 0)
                                                {
                                                        returnBuf = new char[1];
                                                        *returnBuf = 0;
                                                }
                                                else snprintf(returnStr, BUF_CMD_LEN, "%s", wadoserversPtr->bridge);
                                        }
                                        else returnBuf = intTableValue(wadoserversPtr->iniTablePtr, theName);
                                }
                        }
                        else if(strieq(theSection, "webdefaults"))
                        {
                                if(webdefaultsPtr == NULL)sprintf(returnStr, "No webdefaults section");
                                else
                                {
                                        iNum = 0;
                                        if(chechThreadError( webdefaultsTmpPtr )) return (threadError());
                                        webdefaultsTmpPtr = webdefaultsPtr;//Needed for read.
                                        
                                        switch(getTypeForWebDefaults())
                                        {
                                                case DPT_STRING:
                                                        returnBuf = getIniString();
                                                        break;
                                                case DPT_UNUSED:
                                                       strcpy(returnStr, "Unused!");
                                                        break;
                                                case DPT_UNKNOWN:
                                                        returnBuf = intTableValue(webdefaultsPtr->iniTablePtr, theName);
                                                        break;
                                                case DPT_END:
                                                case DPT_SKIP:
                                                case DPT_STRING_ARRAY:
                                                case DPT_INT_ARRAY:
                                                case DPT_UINT_ARRAY:
                                                case DPT_DBTYPE:
                                                case DPT_EDITION:
                                                case DPT_FOR_ASSOC:
                                                case DPT_FILE_COMP:
                                                case DPT_MICROP:
                                                case DPT_INT:
                                                case DPT_UINT:
                                                case DPT_TIME:
                                                case DPT_PATH:
                                                case DPT_BOOL:
#ifdef DGJAVA
                                                case DPT_MIN_JAVA:
#endif
                                                        break;
                                                        
                                        }
                                        webdefaultsTmpPtr = NULL;
                                }
                        }
                        else
                        {
                                if(chechThreadError( unknownSectionTmpPtr )) return (threadError());
                                returnBuf = mallocReturnBuf(GetUnknownSectionTableValue(theName, theSection));
                        }
                }
                
        }
        pSrc = NULL;
        if(returnBuf != NULL) return returnBuf;
        if( returnStr[0] == 0) return mallocReturnBuf("Unknown!");
        return mallocReturnBuf(returnStr);
}

int IniValue        ::       GetIniStringLen(const char *theName, char **strH, const char *theSection)
{
        char lName[SHORT_NAME_LEN];
        
        iNum = 0;
        // The compare functions expects a " =" in the name for parsing, add it.
        strcpylim(lName, theName, SHORT_NAME_LEN - 2);
        strcat(lName, " =");
        pSrc = lName;
        i = 0;
        iSize = strnlen(lName, SHORT_NAME_LEN);
        iVoidPtr = NULL;
        iVoidHandle = &iVoidPtr;
        iStrLen = 0;
        *strH = NULL;
        
        if (strieq(theSection, sscscpPtr->MicroPACS))
        {
                if(chechThreadError( sscscpTmpPtr )) return NULL;
                sscscpTmpPtr = sscscpPtr;//Needed for read.
                iNum = 0;
                switch(getTypeForsscscp())
                {
                        case DPT_PATH:
                        case DPT_STRING:
                        case DPT_MICROP:
                                *strH = (char *)*iVoidHandle;
                                break;
                        case DPT_STRING_ARRAY:
                                *strH = getStringPtrFromArray();
                                break;
                        case DPT_UNKNOWN:
                                *strH = GetIniTableValue(theName, sscscpPtr->iniTablePtr);
                                break;
                        case DPT_BOOL:
                        case DPT_FILE_COMP:
                        case DPT_INT:
                        case DPT_UINT:
                        case DPT_UNUSED:
                        case DPT_INT_ARRAY:
                        case DPT_UINT_ARRAY:
                        case DPT_DBTYPE:
                        case DPT_EDITION:
                        case DPT_END:
                        case DPT_FOR_ASSOC:
                        case DPT_SKIP:
                        case DPT_TIME:
#ifdef DGJAVA
                        case DPT_MIN_JAVA:
#endif
                                break;
                }
                sscscpTmpPtr = NULL;//Set back to NULL
        }
        else
        {
                sourceLines *tempPtr;
                
                iVoidHandle = NULL;
                if(strieq(theSection, sscscpPtr->AnyPage) || strileq(theSection, "anypage"))
                {
                        iVoidHandle = (void **)&AnyPagePtr;
                        tempPtr = AnyPageTmpPtr;
                }
                else if(strieq(theSection, sscscpPtr->DefaultPage) || strileq(theSection, "defaultpage"))
                {
                        iVoidHandle = (void **)&DefaultPagePtr;
                        tempPtr = DefaultPageTmpPtr;
                }
                else if(strileq(theSection, "markseries"))
                {
                        iVoidHandle = (void **)&markseriesPtr;
                        tempPtr = markseriesTmpPtr;
                }
                else if(strileq(theSection, "markstudy"))
                {
                        iVoidHandle = (void **)&markstudyPtr;
                        tempPtr = markstudyTmpPtr;
                }
                else if(strileq(theSection, "shoppingcart"))
                {
                        iVoidHandle = (void **)&shoppingcartPtr;
                        tempPtr = shoppingcartTmpPtr;
                }
                if(iVoidHandle != NULL)
                {
        //We have a reconized section, but doe it esist?
                        
                        if(*iVoidHandle != NULL)
                        {
                                if(chechThreadError( tempPtr )) return NULL;
        //We have the selected section. Read it.
                                iNum = 0;//Changed to 1 if string lable is source.
        //Create pointer and handles that we can pass and destroy.
                                iVoidPtr = *iVoidHandle;
                                iVoidHandle = &iVoidPtr;
                                iVoidDHandle = &iVoidHandle;
                                switch(getTypeForsourceLines())
                                {
                                        case DPT_BOOL:
                                                break;
                                        case DPT_STRING:
                                                *strH = (char *)*iVoidHandle;
                                                break;
                                        case DPT_STRING_ARRAY:
                                                *strH = getStringPtrFromArray();
                                                break;
                                        case DPT_UNKNOWN:
                                                *strH = GetIniTableValue(theName, ((sourceLines *)iVoidPtr)->iniTablePtr);
                                                break;
                                        case DPT_UNUSED:
                                        case DPT_END:
                                        case DPT_SKIP:
                                        case DPT_INT_ARRAY:
                                        case DPT_UINT_ARRAY:
                                        case DPT_DBTYPE:
                                        case DPT_EDITION:
                                        case DPT_FOR_ASSOC:
                                        case DPT_FILE_COMP:
                                        case DPT_MICROP:
                                        case DPT_INT:
                                        case DPT_TIME:
                                        case DPT_UINT:
                                        case DPT_PATH:
#ifdef DGJAVA
                                        case DPT_MIN_JAVA:
#endif
                                                break;
                                                
                                }
                        }
                }
                else
                {// Not a sourec line section, reset the pointer and handles.
                        iVoidPtr = NULL;
                        iVoidHandle = &iVoidPtr;
                        iVoidDHandle = &iVoidHandle;
                        
                        if(strieq(theSection, "lua"))
                        {
                                if(luaPtr != NULL)
                                {
                                        iNum = 0;
                                        if(chechThreadError( luaTmpPtr )) return NULL;
                                        luaTmpPtr = luaPtr;//Needed for read.
                                        
                                        switch(getTypeForLua())
                                        {
                                                case DPT_STRING:
                                                        *strH = (char *)*iVoidHandle;
                                                        break;
                                                case DPT_STRING_ARRAY:
                                                        *strH = getStringPtrFromArray();
                                                        break;
                                                case DPT_UNKNOWN:
                                                        *strH = GetIniTableValue(theName, luaPtr->iniTablePtr);
                                                        break;
                                                case DPT_UNUSED:
                                                case DPT_END:
                                                case DPT_SKIP:
                                                case DPT_INT_ARRAY:
                                                case DPT_UINT_ARRAY:
                                                case DPT_DBTYPE:
                                                case DPT_EDITION:
                                                case DPT_FOR_ASSOC:
                                                case DPT_FILE_COMP:
                                                case DPT_MICROP:
                                                case DPT_INT:
                                                case DPT_UINT:
                                                case DPT_TIME:
                                                case DPT_PATH:
                                                case DPT_BOOL:
#ifdef DGJAVA
                                                case DPT_MIN_JAVA:
#endif
                                                        break;
                                                        
                                        }
                                        luaTmpPtr = NULL;
                                }
                                
                        }
                        else if(strieq(theSection, "scripts"))
                        {
                                if(scriptsPtr != NULL)
                                {
                                        if(chechThreadError( scriptsTmpPtr )) return NULL;
                                        *strH = GetIniTableValue(theName, scriptsPtr);
                                }
                        }
                        else if(strieq(theSection, "wadoservers"))
                        {
                                if(wadoserversPtr != NULL)
                                {
                                        if(chechThreadError( wadoserversTmpPtr )) return NULL;
                                        if(strileq(lName, "bridge"))
                                        {
                                                *strH =  wadoserversPtr->bridge;
                                                iStrLen = sizeof(wadoserversPtr->bridge);
                                        }
                                        else *strH = GetIniTableValue(theName, wadoserversPtr->iniTablePtr);
                                }
                        }
                        else if(strieq(theSection, "webdefaults"))
                        {
                                if(webdefaultsPtr != NULL)
                                {
                                        iNum = 0;
                                        if(chechThreadError( webdefaultsTmpPtr )) return NULL;
                                        webdefaultsTmpPtr = webdefaultsPtr;//Needed for read.
                                        
                                        switch(getTypeForWebDefaults())
                                        {
                                                case DPT_STRING:
                                                        *strH =  (char *)*iVoidHandle;
                                                        break;
                                                case DPT_UNKNOWN:
                                                        *strH = GetIniTableValue(theName, webdefaultsPtr->iniTablePtr);
                                                        break;
                                                case DPT_UNUSED:
                                                case DPT_END:
                                                case DPT_SKIP:
                                                case DPT_STRING_ARRAY:
                                                case DPT_INT_ARRAY:
                                                case DPT_UINT_ARRAY:
                                                case DPT_DBTYPE:
                                                case DPT_EDITION:
                                                case DPT_FOR_ASSOC:
                                                case DPT_FILE_COMP:
                                                case DPT_MICROP:
                                                case DPT_INT:
                                                case DPT_UINT:
                                                case DPT_TIME:
                                                case DPT_PATH:
                                                case DPT_BOOL:
#ifdef DGJAVA
                                                case DPT_MIN_JAVA:
#endif
                                                        break;
                                                        
                                        }
                                        webdefaultsTmpPtr = NULL;
                                }
                        }
                        else
                        {
                                if(chechThreadError( unknownSectionTmpPtr )) return NULL;
                                *strH =  GetUnknownSectionTableValue(theName, theSection);
                        }
                }
                
        }
        if(*strH == NULL) return 0;
        return iStrLen;
}

char* IniValue        ::       GetIniTableValue(const char *theName, Array <iniTable *> *iniTablePtr)
{
        if(iniTablePtr == NULL) return NULL;
        unsigned int aSize;
        if((aSize = iniTablePtr->GetSize()) == 0)  return NULL;
        for(unsigned int cnt = 0; cnt < aSize; cnt++)
        {
                iniTable *lIniTable = iniTablePtr->Get(cnt);
                if(strieq(lIniTable->valueName, theName))
                {
                        iStrLen = sizeof(lIniTable->value);//Used in lua interface.
                        return lIniTable->value;
                        break;
                }
        }
        return NULL;        
}

char* IniValue        ::       getIniTime()
{
        if(*iVoidHandle == NULL || (((struct tm *)*iVoidHandle)->tm_hour == 0 &&
                                    ((struct tm *)*iVoidHandle)->tm_min == 0 &&
                                    ((struct tm *)*iVoidHandle)->tm_sec == 0)) return mallocReturnBuf(NULL);
        char returnStr[VSHORT_NAME_LEN];
        snprintf(returnStr, sizeof(returnStr), "%02d:%02d:%02d", ((struct tm *)*iVoidHandle)->tm_hour,
                 ((struct tm *)*iVoidHandle)->tm_min, ((struct tm *)*iVoidHandle)->tm_sec);
        return mallocReturnBuf(returnStr);
}

//This is not truely multi thread safe, but will be ok for us( not double locked).
/*IniValue* IniValue        ::       GetInstance()
{
        //"Lazy" initialization. Singleton not created until it's needed
        static IniValue iniInstance; // One and only one instance.
        return iniInstance;
}
*/
char * IniValue        ::       getStringPtrFromArray()
{
        if(iVoidHandle != NULL && *iVoidHandle != NULL && **(Array<char *> ***)iVoidHandle != NULL
           && iNum >= 0 && iNum < (int)(**(Array<char *> ***)iVoidHandle)->GetSize())
                return ((**(Array<char *> ***)iVoidHandle)->Get(iNum));
        return NULL;
}

#define checkAndReturnBool(a,b) if(lstrileq( #b )) { \
        *iVoidHandle = &a; \
        return DPT_BOOL; }
/*#define checkAndReturnDB(a,b) if(lstrileq( #b )) { \
        iNum = a; \
        return DPT_DBTYPE; }*/ //Does not work, a is an enum type.
#define checkAndReturnFullPath(a,b) if(lstrileq( #b )) { \
        *iVoidHandle = a; \
        iStrLen = sizeof(a); \
        return DPT_PATH; }
#define checkAndReturnInt(a,b) if(lstrileq( #b )) { \
        *iVoidHandle = &a; \
        return DPT_INT; }
#define checkAndReturnIntForArray(a,b) if(( iNum = lstrileqNum(#b)) != -2) { \
        *iVoidHandle = &a; \
        return DPT_INT_ARRAY; }
#define checkAndReturnIEGroup(a,b) *iVoidHandle = &a; \
        if(strnileq(&pSrc[i], #b, sizeof( #b )) && isIEGroup(sizeof( #b ))) \
        return DPT_STRING_ARRAY;
#define checkAndReturnString(a,b) if(lstrileq( #b )) { \
        *iVoidHandle = a; \
        iStrLen = sizeof(a); \
        return DPT_STRING; }
#define checkAndReturnStringforArray(a,b) if(( iNum = lstrileqNum(#b)) != -2) { \
        *iVoidHandle = &a; \
        return DPT_STRING_ARRAY; }
#define checkAndReturnTime(a,b) if(lstrileq( #b )) { \
        *iVoidHandle = &a; \
        return DPT_TIME; }
#define checkAndReturnUInt(a,b)if(lstrileq( #b )) { \
        *iVoidHandle = &a; \
        return DPT_UINT; }
#define checkAndReturnUnused(b) if(lstrileq( #b )) \
        return DPT_UNUSED;
#define checkAndReturnUnusedArray(b) if(( iNum = lstrileqNum(#b)) != -2) { \
        return DPT_UNUSED; }

//Returns a pointer to the data for reading or writing.
//It uses the temporary pointer, so it must be set first.
//lua value add
//lua array add
DATAPTRTYPE IniValue        ::       getTypeForLua()
{//"lua"
        switch (pSrc[i++])
        {
                case '[':// New section!
                case 0:// The end
                        return DPT_END;
                case ']':// End of current section!
                        return DPT_UNUSED;
                case ' '://Space
                case '\r':// End of line?
                        return DPT_SKIP; // The switch skipps over what it was.
                case 'a':
                case 'A':
                        checkAndReturnString(luaTmpPtr->association,ssociation)
                        checkAndReturnIEGroup(luaTmpPtr->ArchivePtr,rchive)
                        break;
                case 'b':
                case 'B':
                        checkAndReturnString(luaTmpPtr->background,ackground)
                        break;
                case 'c':
                case 'C':
                        checkAndReturnString(luaTmpPtr->clienterror,lienterror)
                        checkAndReturnString(luaTmpPtr->command,ommand)
                        checkAndReturnIEGroup(luaTmpPtr->CompressionPtr,ompression)
                        break;
                case 'e':
                case 'E':
                        checkAndReturnString(luaTmpPtr->endassociation,ndassociation)
                        checkAndReturnStringforArray(luaTmpPtr->ExportConverterPtr,xportconverter)//ExportConverterN
                        break;
                case 'i':
                case 'I':
                        checkAndReturnIEGroup(luaTmpPtr->ImportPtr,mport)
                        break;
                case 'm':
                case 'M':
                        checkAndReturnIEGroup(luaTmpPtr->MergeSeriesPtr,ergeseries)
                        checkAndReturnIEGroup(luaTmpPtr->MergeStudiesPtr,ergestudies)
                        checkAndReturnIEGroup(luaTmpPtr->ModalityWorkListQueryResultPtr,odalityworklistqueryresult)
                        checkAndReturnIEGroup(luaTmpPtr->MoveDevicePtr,ovedevice)
                        break;
                case 'n':
                case 'N':
                        checkAndReturnString(luaTmpPtr->nightly,ightly)
                        break;
                case 'q':
                case 'Q':
                        checkAndReturnIEGroup(luaTmpPtr->QueryResultPtr,ueryresult)
                        checkAndReturnIEGroup(luaTmpPtr->QueryPtr,uery)
                        break;
                case 'r':
                case 'R':
                        checkAndReturnIEGroup(luaTmpPtr->RetrieveResultPtr,etrieveresult)
                        checkAndReturnIEGroup(luaTmpPtr->RetrievePtr,etrieve)
                        checkAndReturnIEGroup(luaTmpPtr->RejectedImageWorkListPtr,ejectedimageworklist)
                        checkAndReturnIEGroup(luaTmpPtr->RejectedImagePtr,ejectedimage)
                        break;
                case 's':
                case 'S':
                        checkAndReturnString(luaTmpPtr->startup,tartup)
                        break;
                case 'w':
                case 'W':
                        checkAndReturnIEGroup(luaTmpPtr->WorkListQueryPtr,orklistquery)
                default:
                        break;
        }
        return DPT_UNKNOWN;
}

#define slCheckAndReturnBool(a,b) if(lstrileq( #b )) { \
**iVoidDHandle = (void *)((**(sourceLines ***)iVoidDHandle)->a); \
return DPT_BOOL; }
#define slCheckAndReturnString(a,b) if(lstrileq( #b )) { \
**iVoidDHandle = ((**(sourceLines ***)iVoidDHandle)->a); \
iStrLen = sizeof((**(sourceLines ***)iVoidDHandle)->a); \
return DPT_STRING; }
#define slCheckAndReturnStringforArray(a,b) if(( iNum = lstrileqNum(#b)) != -2) { \
**iVoidDHandle  = &((**(sourceLines ***)iVoidDHandle)->a); \
return DPT_STRING_ARRAY; }


DATAPTRTYPE IniValue        ::       getTypeForsourceLines()
{
        switch (pSrc[i++])
        {
                case '[':// New section!
                case 0:// The end
                        return DPT_END;
                case ']':// End of current section!
                        return DPT_UNUSED;
                case ' '://Space
                case '\r':// End of line?
                        return DPT_SKIP; // The switch skipps over what it was.
                case 'c':
                case 'C':
                        slCheckAndReturnString(caption,aption)
                        break;
                case 'h':
                case 'H':
                        slCheckAndReturnString(header,eader)
                        break;
                case 'l'://Killed lines and sourceOrLine0.
                case 'L':
                        checkAndReturnUnusedArray(ine)
//                        slCheckAndReturnStringforArray(linesPtr,ine)//Killed lines and sourceOrLine0.
                        break;
                case 's':
                case 'S':
//                        slCheckAndReturnBool(sourceOrLine0,ourceorline0)//Killed lines and sourceOrLine0.
                        slCheckAndReturnString(source,ource)
/*                       if(lstrileq("ource"))//source
                        {
                                iNum = 1;//If 1 and DPT_STRING, string is "source" to set sourceorline0 true.
                                iStrLen = sizeof((**(sourceLines ***)iVoidDHandle)->source);
                                **iVoidDHandle = (**(sourceLines ***)iVoidDHandle)->source;
                                return DPT_STRING;
                        }*/
                        break;
                default:
                        break;
        }
        return DPT_UNKNOWN;
        
}

//Returns a pointer to the data for reading or writing.
//It uses the temporary pointer, so it must be set first.
//ADD new values or arrays to sscscp section here and 3 or 4 other places.
//sscscp value add
//sscscp array add
DATAPTRTYPE IniValue        ::       getTypeForsscscp()
{
                char testPtr[15];
                for(int cnt = 0; cnt < 14; cnt++) testPtr[cnt] = pSrc[i +cnt];//delme
                testPtr[14] = 0;//delme
        switch (pSrc[i++])
        {
                case '[':// New section!
                case 0:// The end
                        return DPT_END;
                case ']':// End of current section!
                        return DPT_UNUSED;
                case ' '://Space
                case '\r':// End of line?
                        return DPT_SKIP; // The switch skipps over what it was.
                case 'A':
                case 'a':
                        checkAndReturnUnused(ccessupdates) //AccessUpdates
                        checkAndReturnFullPath(sscscpTmpPtr->ACRNemaMap, crnemamap)
                        if(lstrileq("llowtruncate"))//AllowTruncate
                        {
                                *iVoidHandle = sscscpTmpPtr->AllowTruncate;
                                iStrLen = sizeof(sscscpTmpPtr->AllowTruncate) -1;// -1 for strcat a ',' later.
                                return DPT_STRING;
                        }
                        checkAndReturnString(sscscpTmpPtr->AnyPage, nypage)
                        checkAndReturnString(sscscpTmpPtr->ArchiveCompression, rchivecompression)
                        checkAndReturnIEGroup(sscscpTmpPtr->ArchivePtr, rchive)//ArchivexxxN
                        break;
                case 'C':
                case 'c':
                        checkAndReturnStringforArray(sscscpTmpPtr->CACHEDevicePtr, achedevice)//CACHEDeviceN
                        checkAndReturnBool(sscscpTmpPtr->CacheVirtualData,achevirtualdata)
//                        checkAndReturnUnused(drdevices)//CDRDevices
                        checkAndReturnUnusedArray(drdevice)//CDRDeviceN//Unused
                        checkAndReturnIEGroup(sscscpTmpPtr->CompressionPtr,ompression)  //CompressionxxxN
                        break;
                case 'D':
                case 'd':
                        checkAndReturnInt(sscscpTmpPtr->DebugVRs,ebuglevel)//DebugLevel
                        checkAndReturnBool(sscscpTmpPtr->DecompressNon16BitsJpeg,ecompressnon16bitsjpeg)//DecompressNon16BitsJpeg
                        checkAndReturnFullPath(sscscpTmpPtr->Dictionary, ictionary)//Dictionary
                        checkAndReturnUnused(marktablename)//DMarkTableName Unused
                        checkAndReturnString(sscscpTmpPtr->DroppedFileCompression,roppedfilecompression)//DroppedFileCompression
                        checkAndReturnBool(sscscpTmpPtr->DoubleBackSlashToDB,oublebackslashtodb)//DoubleBackSlashToDB
                        checkAndReturnBool(dumpIni,umpini)//DumpIni
                        break;
                case 'E':
                case 'e':
                        if(lstrileq("dition"))//Edition
                                return DPT_EDITION;
                        checkAndReturnBool(sscscpTmpPtr->EnableComputedFields,nablecomputedfields)//EnableComputedFields
                        checkAndReturnBool(sscscpTmpPtr->EnableReadAheadThread,nablereadaheadthread)//EnableReadAheadThread
                        checkAndReturnIEGroup(sscscpTmpPtr->ExportPtr,xport)//ExportxxxN
                        break;
                case 'F':
                case 'f':
                        checkAndReturnUInt(sscscpTmpPtr->FailHoldOff,ailholdoff)//FailHoldOff
                        if(lstrileq("ilecompressmode"))//FileCompressMode
                                return DPT_FILE_COMP;
                        checkAndReturnString(sscscpTmpPtr->FileNameSyntaxString,ilenamesyntax)//FileNameSyntax
                        checkAndReturnBool(sscscpTmpPtr->FixKodak,ixkodak)//FixKodak
                        checkAndReturnBool(sscscpTmpPtr->FixPhilips,ixphilips)//FixPhilips
                        if(lstrileq("orwardassociationlevel"))//ForwardAssociationLevel
                                return DPT_FOR_ASSOC;
                        checkAndReturnUInt(sscscpTmpPtr->ForwardAssociationCloseDelay,orwardassociationclosedelay)//ForwardAssociationCloseDelay
                        checkAndReturnUInt(sscscpTmpPtr->ForwardAssociationRefreshDelay,orwardassociationrefreshdelay)//ForwardAssociationRefreshDelay
                        checkAndReturnBool(sscscpTmpPtr->ForwardAssociationRelease,orwardassociationrelease)//ForwardAssociationRelease
                        checkAndReturnUInt(sscscpTmpPtr->ForwardCollectDelay,orwardcollectdelay)//ForwardCollectDelay
                        break;
                case 'I':
                case 'i':
                        checkAndReturnBool(sscscpTmpPtr->IgnoreMAGDeviceThreshHold,gnoremagdevicethreshhold)//IgnoreMAGDeviceThreshHold
                        checkAndReturnBool(sscscpTmpPtr->IgnoreOutOfMemoryErrors,gnoreoutofmemoryerrors)//IgnoreOutOfMemoryErrors
                        checkAndReturnString(sscscpTmpPtr->ImageQuerySortOrder,magequerysortorder)//ImageQuerySortOrder
                        checkAndReturnString(sscscpTmpPtr->ImageTableName,magetablename)//ImageTableName
                        checkAndReturnBool(sscscpTmpPtr->ImportExportDragAndDrop,mportexportdraganddrop)//ImportExportDragAndDrop
                        checkAndReturnString(sscscpTmpPtr->IncomingCompression,ncomingcompression)//IncomingCompression
                        checkAndReturnUInt(sscscpTmpPtr->IndexDBF,ndexdbf)//IndexDBF
                        checkAndReturnIEGroup(sscscpTmpPtr->ImportPtr,mport) //ImportxxxN
                        break;
                case 'J':
                case 'j':
#ifdef DGJAVA
                        checkAndReturnFullPath(sscscpTmpPtr->JavaHapiDirectory, avadirectory)//JavaHapiDirectory
                        if(lstrileq("avaminversion"))//JavaMinVersion
                                return DPT_MIN_JAVA;
#endif
                        checkAndReturnStringforArray(sscscpTmpPtr->JUKEBOXDevicePtr,ukeboxdevice)//JUKEBOXDeviceN
                        break;
                case 'K':
                case 'k':
                        checkAndReturnFullPath(sscscpTmpPtr->kFactorFile,factorfile)//kFactorFile
                        break;
                case 'L':
                case 'l':
                        checkAndReturnUInt(sscscpTmpPtr->LongQueryDBF,ongquerydbf)//LongQueryDBF
                        checkAndReturnUInt(sscscpTmpPtr->LossyQuality,ossyquality)//LossyQuality
                        checkAndReturnString(sscscpTmpPtr->LRUSort,rusort)//LRUSort
                        break;
                case 'M':
                case 'm':
                        checkAndReturnUInt(sscscpTmpPtr->MAGDeviceFullThreshHold,agdevicefullthreshhold)//MAGDeviceFullThreshhold
                        checkAndReturnStringforArray(sscscpTmpPtr->MAGDevicePtr,agdevice)//MAGDeviceN
                        checkAndReturnUInt(sscscpTmpPtr->MAGDeviceThreshHold,agdevicethreshhold)//MAGDeviceThreshhold
                        checkAndReturnInt(sscscpTmpPtr->MaximumDelayedFetchForwardRetries,aximumdelayedfetchforwardretries)//MaximumDelayedFetchForwardRetries
                        checkAndReturnUInt(sscscpTmpPtr->MaximumExportRetries,aximumexportretries)//MaximumExportRetries
                        checkAndReturnUInt(sscscpTmpPtr->MaxFieldLength,axfieldlength)//MaxFieldLength
                        checkAndReturnUInt(sscscpTmpPtr->MaxFileNameLength,axfilenamelength)//MaxFileNameLength
                        if(lstrileq("icropacs")) return DPT_MICROP;//MicroPACS
                        checkAndReturnStringforArray(sscscpTmpPtr->MIRRORDevicePtr,irrordevice)//MIRRORDeviceN
                        checkAndReturnUnused(opdevices)//MOPDevices.  Use MOPDevicePtr Array count.
                        checkAndReturnUnusedArray(opdevice)//MOPDeviceN//Unused
                        checkAndReturnString(sscscpTmpPtr->MyACRNema,yacrnema)//MyACRNema
//                        checkAndReturnDB(DT_MYSQL,"ysql")//MySQL
                        if(lstrileq( "ysql" )) {
                                iNum = DT_MYSQL;
                                return DPT_DBTYPE; }
                        checkAndReturnIEGroup(sscscpTmpPtr->MergeSeriesPtr,ergeseries)//MergeSeriesxxxN
                        checkAndReturnIEGroup(sscscpTmpPtr->MergeStudiesPtr,ergestudies)//MergeStudiesxxxN
                        //ModalityWorkListQueryResultxxxN
                        checkAndReturnIEGroup(sscscpTmpPtr->ModalityWorkListQueryResultPtr,odalityworklistqueryresult)
                        checkAndReturnIEGroup(sscscpTmpPtr->MoveDevicePtr,ovedevice)//MoveDevicexxxN
                        break;
                case 'N':
                case 'n':
                        checkAndReturnUInt(sscscpTmpPtr->NightlyCleanThreshhold,ightlycleanthreshhold)//NightlyCleanThreshhold
                        checkAndReturnBool(sscscpTmpPtr->NoDicomCheck,odicomcheck)//NoDicomCheck
                        break;
                case 'O':
                case 'o':
                        checkAndReturnString(sscscpTmpPtr->OCPipeName,peratorconsole)//OperatorConsole
                        checkAndReturnInt(sscscpTmpPtr->OverlapVirtualGet,verlapvirtualget)//OverlapVirtualGet
                        break;
                case 'P':
                case 'p':
                        checkAndReturnBool(sscscpTmpPtr->PackDBF,ackdbf)//PackDBF
                        checkAndReturnString(sscscpTmpPtr->ServerName,acsname)//PACSName
                        checkAndReturnUnused(adaewithzeros)//PadAEWithZeros
//                        checkAndReturnBool(sscscpTmpPtr->ConfigPadAEWithZeros,adaewithzeros)//PadAEWithZeros bcb 130328
                        checkAndReturnString(sscscpTmpPtr->Password,assword)//Password
                        checkAndReturnString(sscscpTmpPtr->PatientQuerySortOrder,atientquerysortorder)//PatientQuerySortOrder
                        checkAndReturnString(sscscpTmpPtr->PatientTableName,atienttablename)//PatientTableName
//                        checkAndReturnDB(DT_POSTGRES,"ostgres")//Postgres
                        if(lstrileq( "ostgres" )) {
                                iNum = DT_POSTGRES;
                                return DPT_DBTYPE; }
                        checkAndReturnUInt(sscscpTmpPtr->Prefetcher,refetcher)//Prefetcher
                        break;
                case 'Q':
                case 'q':
                        checkAndReturnUInt(sscscpTmpPtr->QueueSize,ueuesize)//QueueSize
                        checkAndReturnIEGroup(sscscpTmpPtr->QueryResultPtr,ueryesult)//QueryResultxxxN
                        checkAndReturnIEGroup(sscscpTmpPtr->QueryPtr,uery)//QueryxxxN
                        break;
                case 'R':
                case 'r':
                        checkAndReturnUnused(egisteredmopdevicetable)//RegisteredMOPDeviceTable
                        checkAndReturnUnused(egisteredmopidtablename)//RegisteredMOPIDTableName
                        checkAndReturnUInt(sscscpTmpPtr->RetryDelay,etrydelay)//RetryDelay
                        checkAndReturnBool(sscscpTmpPtr->RetryForwardFailed,etryforwardfailed)//RetryForwardFailed
                        checkAndReturnIEGroup(sscscpTmpPtr->RetrieveResultPtr,etrieveresult)//RetrieveResultxxxN
                        checkAndReturnIEGroup(sscscpTmpPtr->RetrievePtr,etrieve)//RetrievexxxN
                        checkAndReturnIEGroup(sscscpTmpPtr->RejectedImageWorkListPtr,ejectedimageworklist)//RejectedImageWorkListxxxN
                        checkAndReturnIEGroup(sscscpTmpPtr->RejectedImagePtr,ejectedimage)//RejectedImagexxxN
                        break;
                case 'S':
                case 's':
                        checkAndReturnBool(sscscpTmpPtr->SendUpperCaseAE,enduppercaseae)//SendUpperCaseAE
                        checkAndReturnString(sscscpTmpPtr->SeriesQuerySortOrder,eriesquerysortorder)//SeriesQuerySortOrder
                        checkAndReturnString(sscscpTmpPtr->SeriesTableName,eriestablename)//SeriesTableName
                        checkAndReturnFullPath(sscscpTmpPtr->SOPClassFile,opclasslist)//SOPClassList
                        checkAndReturnString(sscscpTmpPtr->DataHost,qlhost)//SQLHost
                        checkAndReturnString(sscscpTmpPtr->DataSource,qlserver)//SQLServer
//                        checkAndReturnDB(DT_SQLITE,"qlite")// SqLite
                        if(lstrileq( "qlite" )) {
                                iNum = DT_SQLITE;
                                return DPT_DBTYPE; }
                        checkAndReturnUInt(sscscpTmpPtr->StorageFailedErrorCode,toragefailederrorcode)//StorageFailedErrorCode
                        checkAndReturnString(sscscpTmpPtr->StudyQuerySortOrder,tudyquerysortorder)//StudyQuerySortOrder
                        checkAndReturnString(sscscpTmpPtr->StudyTableName,tudytablename)//StudyTableName
                        break;
                case 'T':
                case 't':
                        checkAndReturnUInt(sscscpTmpPtr->TCPIPTimeOut,cpiptimeout)//TCPIPTimeOut
                        checkAndReturnString(sscscpTmpPtr->Port,cpport)//TCPPort
                        checkAndReturnFullPath(sscscpTmpPtr->TempDir,sempdir)//TempDir
                        checkAndReturnFullPath(sscscpTmpPtr->TroubleLogFile,roublelogfile)//TroubleLogFile
                        if(lstrileq("runcatefieldnames"))//TruncateFieldNames
                        {
                                iNum = 1;
                                *iVoidHandle = &(sscscpTmpPtr->TruncateFieldNames);
                                return DPT_UINT;
                        }
                        break;
                case 'U':
                case 'u':
                        checkAndReturnUnused(sebuiltindecompressor)//UseBuiltInDecompressor
                        checkAndReturnBool(sscscpTmpPtr->UseBuiltInJPEG,sebuiltinjpeg)//UseBuiltInJPEG
                        checkAndReturnBool(sscscpTmpPtr->UseEscapeStringConstants,seescapestringconstants)//UseEscapeStringConstants
#ifdef HAVE_BOTH_J2KLIBS
                        checkAndReturnBool(sscscpTmpPtr->UseOpenJPG,seopenjpg)
#else
                        checkAndReturnUnused(seopenjpg)
#endif
                        checkAndReturnFullPath(sscscpTmpPtr->UserLogFile,serlogfile)//UserLogFile
                        checkAndReturnString(sscscpTmpPtr->UserName,sername)//Username
                        checkAndReturnString(sscscpTmpPtr->UIDPrefix,idprefix)//UIDPrefix
                        checkAndReturnString(sscscpTmpPtr->UIDToCDRIDTableName,idtocdridtable)//UIDToCDRIDTable
                        checkAndReturnString(sscscpTmpPtr->UIDToMOPIDTableName,idtomopidtable)//UIDToMOPIDTable;
                        break;
                case 'V':
                case 'v':
// Should these have a count variable (ex:MAGDevices). bcb?
                        checkAndReturnStringforArray(sscscpTmpPtr->VirtualServerForPtr,irtualserverfor)//VirtualServerForN
                        checkAndReturnIntForArray(sscscpTmpPtr->VirtualServerPerSeriesPtr,irtualserverperseries)//VirtualServerPerSeriesN
                        break;
                case 'W':
                case 'w':
                        checkAndReturnFullPath(sscscpTmpPtr->monitorfolder2,atchfolder)//WatchFolder;
                        checkAndReturnString(sscscpTmpPtr->WebCodeBase,ebcodebase)//WebCodeBase;
                        checkAndReturnString(sscscpTmpPtr->WebMAG0Address,ebmag0address)//WebMAG0Address;
                        checkAndReturnBool(sscscpTmpPtr->WebPush,ebpush)//WebPush
                        checkAndReturnBool(sscscpTmpPtr->WebReadOnly,ebreadonly)//WebReadOnly
                        checkAndReturnString(sscscpTmpPtr->WebScriptAddress,ebscriptaddress)//WebScriptAddress;
                        checkAndReturnString(sscscpTmpPtr->WebServerFor,ebserverfor)//WebServerFor;
                        checkAndReturnUInt(sscscpTmpPtr->WorkListMode,orklistmode)//WorkListMode
                        checkAndReturnInt(sscscpTmpPtr->WorkListReturnsISO_IR_100,orklistreturnsiso_ir_100)//WorkListReturnsISO_IR_100
                        checkAndReturnInt(sscscpTmpPtr->WorkListReturnsISO_IR,orklistreturnsiso_ir)//WorkListReturnsISO_IR
                        checkAndReturnString(sscscpTmpPtr->WorkListTableName,orklisttablename)//WorkListTableName;
                        checkAndReturnIEGroup(sscscpTmpPtr->WorkListQueryPtr,orklistquery)//WorkListQueryxxxN
                        break;
                case 'Z':
                case 'z':
                        checkAndReturnTime(sscscpTmpPtr->ziptime,iptime)//ziptime
                default:
                        break;
        }
        return DPT_UNKNOWN;// Should end with a \0 or new section [.
}

DATAPTRTYPE IniValue        ::       getTypeForWebDefaults()
{
        switch (pSrc[i++])
        {                
                case '[':// New section!
                case 0:// The end
                        return DPT_END;
                case ']':// End of current section!
                        return DPT_UNUSED;
                case ' '://Space
                case '\r':// End of line?
                        return DPT_SKIP; // The switch skipps over what it was.
                case 'A':
                case 'a':
//                        checkAndReturnString(webdefaultsTmpPtr->address,ddress);
                        if(lstrileq( "ddress" )) { 
                                *iVoidHandle = webdefaultsTmpPtr->address; 
                                iStrLen = sizeof(webdefaultsTmpPtr->address); 
                                return DPT_STRING; }
                        break;
                case 'C':
                case 'c':
                        checkAndReturnString(webdefaultsTmpPtr->compress,ompress);
                        break;
                case 'D':
                case 'd':
                        checkAndReturnString(webdefaultsTmpPtr->dsize,size);
                        break;
                case 'G':
                case 'g':
                        checkAndReturnString(webdefaultsTmpPtr->graphic,raphic);
                        break;
                case 'I':
                case 'i':
                        checkAndReturnString(webdefaultsTmpPtr->iconsize,consize);
                        break;
                case 'P':
                case 'p':
                        checkAndReturnString(webdefaultsTmpPtr->port,ort);
                        break;
                case 'S':
                case 's':
                        checkAndReturnString(webdefaultsTmpPtr->size,ize);
                        checkAndReturnString(webdefaultsTmpPtr->studyviewer,tudyviewer);
                        break;
                case 'V':
                case 'v':
                        checkAndReturnString(webdefaultsTmpPtr->viewer,iewer);
        }
        return DPT_UNKNOWN;// Should end with a \0 or new section [.
        
}

int  IniValue        ::       getReplyLength()//-1 is not found, if here, >= 0 .
{
        if(goToStartIsBlank()) return 0;// i skips blanks.
        unsigned int cnt;
        size_t iLoc = i;// no more changes to i
        for(cnt = 0;  pSrc[iLoc] != '\r' && iLoc++ < iSize; cnt++);
        return cnt;
}

char* IniValue        ::       GetUnknownSectionTableValue(const char *theName, const char *theSection)
{
        if(unknownSectionPtr == NULL) return NULL;
        
        unsigned int    cnt, aSize;
        sectionTable      *lSection;
        
        aSize = unknownSectionPtr->GetSize();
        for(cnt = 0; cnt < aSize; cnt++)
        {
                lSection = unknownSectionPtr->Get(cnt);
                if(streq(lSection->section, theSection))
                        return GetIniTableValue(theName, lSection->iniTablePtr);
        }
        return NULL;
}

// The goTos are used in cleaned buffers (read) and unclean buffers (delete and write), must handle both.
BOOL IniValue        ::       goToNextDataLine()
{
        while(pSrc[i] != '\r' && pSrc[i] != '\n')
        {
                i++;//Go to the end.
                if(i >= iSize)return false;
        }
        while(pSrc[i] == '\r' || pSrc[i] == '\n')
        {
                i++;//Go to next data.
                if(i >= iSize)return false;
        }
        return true;
}

BOOL IniValue        ::       goToNextSection()
{
        iniStringsPtr->currentSection[0] = 0;
        while(i < iSize)
        {
                if(!goToStartIsBlank())
                {
                        if(pSrc[i] == '/' && pSrc[i + 1] == '*')
                        {
                                if(!skipComments()) return false;
                                continue;
                        }
                        if(pSrc[i] == '[')//First thing on the line, after blanks.
                        {
                                i++;//Skip '[';
                                setIniString(iniStringsPtr->currentSection, sizeof(iniStringsPtr->currentSection));
                                if(iniStringsPtr->currentSection[0] != 0) return true;// Check for a stray [.
                        }
                }
                lastLine =i;//Save the last line of the last section.
                goToNextDataLine();
        }
        return false;
}

BOOL IniValue        ::       goToStartIsBlank()
{
        while(pSrc[i] == ' ' || pSrc[i] == '\t')
        {
                i++;// Remove leading space.
                if(i >= iSize)return true;
        }
        if(pSrc[i] == '\r'|| pSrc[i] == '\n')//check for end of line, no data (blank).
        {
                while(pSrc[i] == '\r'|| pSrc[i] == '\n')
                {
                        i++; //Get to new line.
                        if(i >= iSize)return true;
                }
                return true;//No change to the value for a blank line
        }
        if(i >= iSize ) return true;
        return false;
}

BOOL IniValue        ::       goToStartOfName(const char *theName)
{
        lastLine = i;//Make lastLine always valid.
        while(i < iSize)
        {
                if(goToStartIsBlank())continue;
                if(pSrc[i] == '/' && i + 1 < iSize && pSrc[i + 1] == '*')
                {
                        if(!skipComments()) return false;
                        continue;
                }
                if(pSrc[i] == '[') return false;
                if(strnieq(&pSrc[i], theName ,strlen(theName))) return true;
                lastLine = i;
                goToNextDataLine();
        }
        return false;
}

void IniValue        ::       initIEGroup(iEGroup *theGroup)
{
        theGroup->CalledAEPtr = NULL;
        theGroup->CallingAEPtr = NULL;
        theGroup->ConverterPtr = NULL;
        theGroup->FilterPtr = NULL;
        theGroup->ModalityPtr = NULL;
        theGroup->StationNamePtr = NULL;
}

char * IniValue        ::       intTableValue(Array<iniTable *> *iniTableAray, const char *theName)
{
        return mallocReturnBuf(GetIniTableValue(theName, iniTableAray));
}

#define checkAndReturnIEStringforArray(a,b) if(( iNum = lstrileqNum(#b)) != -2) { \
*iVoidHandle = &((*ieGrpHandle)->a); \
break; }

BOOL IniValue        ::       isIEGroup(unsigned int iIncrease)
{
        if(iVoidHandle == NULL) return FALSE; //Should never happen!
        iEGroup **ieGrpHandle;
        ieGrpHandle = (iEGroup **)*iVoidHandle;
        BOOL madeNow = FALSE;
        iNum = -2;  //The default is to fail
        i += iIncrease - 1; //Set i to the next part on the name.
        if(*ieGrpHandle == NULL)
        {
                if((*ieGrpHandle = (iEGroup *)malloc(sizeof(iEGroup))) == NULL)
                        return (printAllocateError(sizeof(iEGroup), "export group"));
                initIEGroup(*ieGrpHandle);
                madeNow = TRUE;
        }
        switch (pSrc[i++])
        {
                case 'c':
                case 'C':
                        checkAndReturnIEStringforArray(CalledAEPtr,alledae)//xxxCalledAEN
                        /*                        if((iNum = lstrileqNum("alledae")) != -2)//xxxCalledAEN
                         {
                         *iVoidHandle = &((*ieGrpHandle)->CalledAEPtr);
                         break;
                         }*/
                        checkAndReturnIEStringforArray(CallingAEPtr,allingae)//xxxCallingAEN
                        /*                        if((iNum = lstrileqNum("allingae")) != -2)//xxxCallingAEN
                         {
                         *iVoidHandle = &((*ieGrpHandle)->CallingAEPtr);
                         break;
                         }*/
                        checkAndReturnIEStringforArray(ConverterPtr,onverter)//xxxConverterN
                        /*                        if((iNum = lstrileqNum("onverter")) != -2)//xxxConverterN
                         {
                         *iVoidHandle = &((*ieGrpHandle)->ConverterPtr);
                         break;
                         }*/
                        break;
                case 'f':
                case 'F':
                        checkAndReturnIEStringforArray(FilterPtr,ilter)//xxxFilterN
                        /*                        if((iNum = lstrileqNum("ilter")) != -2)//xxxFilterN
                         {
                         *iVoidHandle = &((*ieGrpHandle)->FilterPtr);
                         break;
                         }*/
                        break;
                case 'm':
                case 'M':
                        checkAndReturnIEStringforArray(ModalityPtr,odality)//xxxModalityN
                        /*                        if((iNum = lstrileqNum("odality")) != -2)//xxxModalityN
                         {
                         *iVoidHandle = &((*ieGrpHandle)->ModalityPtr);
                         break;
                         }*/
                        break;
                case 's':
                case 'S':
                        checkAndReturnIEStringforArray(StationNamePtr,tationname)//xxxStationNameN
                        /*                        if((iNum = lstrileqNum("tationname")) != -2)//xxxStationNameN
                         {
                         *iVoidHandle = &((*ieGrpHandle)->StationNamePtr);
                         break;
                         }*/
                default:
                        break;
        }
        if(iNum >= 0)return true; //We have an xxxN
        if(madeNow) //Not found or an xxxs, stop leaks.
        {
                free(*ieGrpHandle);
                *ieGrpHandle = NULL;
                *iVoidHandle = NULL; //Incase of an xxxs, do not leave an invalid handle
        }
        if(iNum == -1) return true; //xxxs
        i -= iIncrease;//Put i back to before what brought us here.
        return FALSE;
}

BOOL IniValue        ::       keepLine(const char *startOfLine)
{
        
        if(*startOfLine++ != '#') return true;//look for comment start
        while (true)
        {
                switch (*startOfLine)
                {
                        case 0:
                        case '\r':
                        case '\n':
                                return true;
                                break;
                        case 'P':
                        case 'p':
                                if(strneq(startOfLine, "put_param", 8)) return false;
                        default:
                                break;
                }
                startOfLine++;
        }
        return true;
}

// strcpysplim will always terminate the string!  Returns a pointer to the null.
// It will copy until a space or null. i is advanced, source is pSrc[i].
char* IniValue        ::       lstrcpysplim(char * dest, size_t limit)
{
        while(--limit > 0 && i < iSize)//decrement first, last char always 0
        {
                *dest = pSrc[i++];
                if(*dest == ' ' || *dest == '\r')break;
                if(*dest++ == 0)return(dest);
        }
        *dest = 0;
        return(dest);
}

// A quicker compare of an anycase string to an anycase string.
// As soon as a mismatch is found, returns FALSE.
BOOL IniValue        ::       lstrieq(const char * lowUp)
{
        size_t loci = i;// i does not change unless a match is found
        while(*lowUp != 0 && pSrc[loci] != 0 && loci < iSize)
        {
                if(*lowUp != pSrc[loci] && *lowUp != pSrc[loci] + 0x20
                   && *lowUp + 0x20 != pSrc[loci])return FALSE;
                lowUp++;
                loci++;
        }
        if(*lowUp != 0)return FALSE;//What? Ran out of file buffer!
        if(pSrc[loci] == ']')//section compare
        {
                i = loci + 1;
                return TRUE;
        }
        return TRUE;
        
}

// A quicker compare of an anycase string to a lower case only string.
// As soon as a mismatch is found, returns FALSE.
BOOL IniValue        ::       lstrileq(const char * lowOnly)
{
        size_t loci = i;// i does not change unless a match is found
        while(*lowOnly != 0 && pSrc[loci] != 0 && loci < iSize)
        {
                if(*lowOnly != pSrc[loci] && *lowOnly != pSrc[loci] + 0x20)return FALSE;
                lowOnly++;
                loci++;
        }
        if(*lowOnly != 0)return FALSE;//What? Ran out of file buffer!
        if(pSrc[loci] == ']')//section compare
        {
                i = loci + 1;
                return TRUE;
        }
        while(pSrc[loci] == ' ') loci++;//Name compare.
        if(pSrc[loci] != '=')return FALSE;
        i = loci + 1;//Pass the "="
        return TRUE;
}

int IniValue        ::       lstrileqNum(const char * lowOnly)
{
        int lNum;
        
        size_t loci = i;// i does not change unless a match is found
        while(*lowOnly != 0 && pSrc[loci] != 0 && loci < iSize)
        {
                if(*lowOnly != pSrc[loci] && *lowOnly != pSrc[loci] + 0x20)return -2;
                lowOnly++;
                loci++;
        }
        if(*lowOnly != 0)return -2;//What? Ran out of file buffer(iSize)!
        if(pSrc[loci] == 's' || pSrc[loci] == 'S')
        {
                lNum = -1;
                loci++;//Skip the s\'s'
        }
        else
        {
                if(pSrc[loci] < '0' || pSrc[loci] > '9')return -2;//Not a number!
                lNum = 0;
                while (!(pSrc[loci] < '0' || pSrc[loci] > '9') && loci < iSize)// while a number
                {
                        lNum *= 10;
                        lNum += pSrc[loci++] - 0x30;
                }
        }
        while(pSrc[loci] == ' ') loci++;//May have a space.
        if(pSrc[loci++] != '=')return -2;// Must have an eq sign.
        i = loci;//Pass the "="
        return lNum;
}

void IniValue        ::       makeFullPath(char *inFile)
{
        char *theFile = inFile;
        char tempStr[MAX_PATH];
        
        if (inFile[0] == 0) return; // Must be not set, do nothing.
        if (inFile[0] == PATHSEPCHAR) return;// PATHSEPCHAR starts a full path, done
#ifndef UNIX
        if(inFile[1] == ':') return; //Root of a drive, full path.
#endif
        if (inFile[0] == '.'  && inFile[1] == PATHSEPCHAR)// PWD
            theFile += 2;// Skip over the "working dir".
        
        strcpylim(tempStr, theFile, sizeof(tempStr) - strlen(iniStringsPtr->basePath));
        strcpy(inFile, iniStringsPtr->basePath);
        strcat(inFile, tempStr);
}

char* IniValue        ::       mallocReturnBuf(const char *theString)
{
        char *returnBuf;
        size_t theStringLen = 0;
        
        if(theString != NULL) theStringLen = strnlen(theString, BUF_CMD_LEN - 1);
        returnBuf = (char *)malloc((theStringLen + 1) * sizeof(char));
        if(returnBuf != NULL)
        {
                if(theStringLen > 0)strcpylim(returnBuf, theString, theStringLen + 1);
                else *returnBuf = 0;
        }
        return returnBuf;
        
}

BOOL IniValue        ::       openNewConfigFile()
{
        strcpylim(iniStringsPtr->newConfigFile, iniStringsPtr->iniFile, MAX_PATH - 4);
        strcat(iniStringsPtr->newConfigFile, ".new");
        tempFile = fopen(iniStringsPtr->newConfigFile, "wb");
        if(tempFile == NULL)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                         "Failed to open temporary file %s :%s\n",
                         iniStringsPtr->newConfigFile, strerror(errno));
                return false;
        }
        return true;
}

// This is the main read section of the code.  The file is read and parsed into temporary pointers
// so it does not effect other threads until all reading is finished.  Than the temporary pointers
// are swapped for the real ones.  The old pointers are than freed.
BOOL IniValue        ::       ReadIni()
{
//        char                    *pDest;
        time_t                  nowT;
        unsigned int            cnt = 0;

//Just for lua and status display.
        if(gpps == 0)nowT = gppstime;//First time we count the hole run.
        else nowT = time(NULL);
//Check for threads using temporary pointers. Should all be null unless another thread is using it
        while( sscscpTmpPtr != NULL && AnyPageTmpPtr != NULL && DefaultPageTmpPtr != NULL && luaTmpPtr != NULL &&
              markseriesTmpPtr != NULL && markstudyTmpPtr != NULL && scriptsTmpPtr != NULL &&
              shoppingcartTmpPtr != NULL && wadoserversTmpPtr != NULL && webdefaultsTmpPtr != NULL )
        {
                if(cnt > THREAD_WAIT_TIME) return errorToBuffer("Read Ini thread error.\n");
#ifdef UNIX
                sleep(1);
#else
                Sleep(1000);
#endif
        }
        if(!readIniFile()) return false;
        iSize = decommentBuffer(pSrc, iSize & INT_MAX);
// Now fill the structure.  To the start.
        i = 0;
        if(sscscpTmpPtr == NULL)//Not the first time! Make thread safe.
        {//Create and load the new one while still using the old one in other threads.
                sscscpTmpPtr = (DicomIni *)malloc(sizeof(DicomIni));
                if(sscscpTmpPtr == NULL)
                {
                        if(pSrc)free(pSrc);
                        pSrc = NULL;
                        return (printAllocateError(sizeof(DicomIni),"DicomIni Table"));
                }
        }
        defaultIni();//Set the default values.
// The section name is in the section, so first scan the whole file for the first MicroPACS label.
        while(i < iSize)
        {
                while(i < iSize && pSrc[i] != 'M' &&  pSrc[i] != 'm')i++;
                if(lstrileq("micropacs"))
                {
                        setStringLim(sscscpTmpPtr->MicroPACS);
                        break;
                }
                i++;
        }
        if(sscscpTmpPtr->MicroPACS[0] == 0)
        {
                if(pSrc)free(pSrc);
                pSrc = NULL;
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                         "MicroPACS entry in %s not found, but required.\n", iniStringsPtr->iniFile );
                return FALSE;
        }
//Back to the start.
        i = 0;
//Look for other sections.
        while(i < iSize && pSrc[i] != '[')i++;//Find section marker, sb first.
        i++;//Skip section marker
        while(i < iSize)
        {
                if(lstrieq(sscscpTmpPtr->MicroPACS))readsscscp();
                else if((sscscpTmpPtr->AnyPage[0] != 0 && lstrieq(sscscpTmpPtr->AnyPage))//AnyPage
                        || lstrileq("anypage"))readsourceLines(&AnyPageTmpPtr);
                else if((sscscpTmpPtr->DefaultPage[0] != 0 && lstrieq(sscscpTmpPtr->DefaultPage))//DefaultPage
                        || lstrileq("defaultpage"))readsourceLines(&DefaultPageTmpPtr);
                else if(lstrileq("lua")) readlua();
                else if(lstrileq("markseries")) readsourceLines(&markseriesTmpPtr);
                else if(lstrileq("markstudy")) readsourceLines(&markstudyTmpPtr);
                else if(lstrileq("scripts")) readScripts();//scripts
                else if(lstrileq("shoppingcart")) readsourceLines(&shoppingcartTmpPtr);
                else if(lstrileq("wadoservers")) readwadoservers();//wadoservers
                else if(lstrileq("webdefaults")) readwebdefaults();//wadoservers
                else readUknownSection();
        }
// Read of the buffer is done.
        if(pSrc)free(pSrc);
        pSrc = NULL;        
//Now for the changes between sections.
        if(webdefaultsTmpPtr != NULL)
        {
                if(webdefaultsTmpPtr->address[0] == 0)
                        strcpylim(webdefaultsTmpPtr->address,sscscpTmpPtr->WebServerFor,
                                  sizeof(webdefaultsTmpPtr->address));
                else strcpylim(sscscpTmpPtr->WebServerFor,webdefaultsTmpPtr->address,
                               sizeof(sscscpPtr->WebServerFor));
                if(webdefaultsTmpPtr->port[0] == 0)
                        strcpylim(webdefaultsTmpPtr->port,sscscpTmpPtr->Port,
                                  sizeof(webdefaultsTmpPtr->port));
                else strcpylim(sscscpTmpPtr->Port,webdefaultsTmpPtr->port,
                               sizeof(sscscpPtr->Port));
        }
// Check the SQLServer path and expand if needed
        switch ((enum DBTYPE)sscscpTmpPtr->db_type)
        {
                case DT_DBASEIII:
                case DT_DBASEIIINOINDEX:
                case DT_SQLITE:
                        makeFullPath(sscscpTmpPtr->DataHost);
                        break;
                case DT_MYSQL:
                case DT_ODBC:
                case DT_POSTGRES:
                        break;
                case DT_NULL:
                        sscscpTmpPtr->DataHost[0] = 0;// As per windowsmanual.pdf
                        break;
        }
// Time to bring it live with defaults to Source Lines sections!
        swapPointers((void **)&sscscpPtr, (void **)&sscscpTmpPtr);
        freesscscp();//Clears the sscscpTmpPtr arrays.
        if(AnyPageTmpPtr != NULL)//Found section.
        {
                swapPointers((void **)&AnyPagePtr, (void **)&AnyPageTmpPtr);
                freeSourceLines(&AnyPageTmpPtr);
        }
        if(DefaultPageTmpPtr != NULL)//Found section.
        {
                swapPointers((void **)&DefaultPagePtr, (void **)&DefaultPageTmpPtr);
                freeSourceLines(&DefaultPageTmpPtr);                }
        if(luaTmpPtr != NULL)//Found section.
        {
                swapPointers((void **)&luaPtr, (void **)&luaTmpPtr);
                freeLua();
        }
        if(markseriesTmpPtr != NULL)//Found section.
        {
                if(markseriesTmpPtr->caption[0] == 0)strcpy(markseriesTmpPtr->caption, "Mark");
                swapPointers((void **)&markseriesPtr, (void **)&markseriesTmpPtr);
                freeSourceLines(&markseriesTmpPtr);
        }
        if(markstudyTmpPtr != NULL)//Found section.
        {
                if(markstudyTmpPtr->caption[0] == 0)strcpy(markstudyTmpPtr->caption, "Mark");
                swapPointers((void **)&markstudyPtr, (void **)&markstudyTmpPtr);
                freeSourceLines(&markstudyTmpPtr);
        }
        if(scriptsTmpPtr != NULL)//Found section.
        {
                swapPointers((void **)&scriptsPtr, (void **)&scriptsTmpPtr);
                freeCharArray((Array<char *> **)&scriptsTmpPtr);
        }
        if(shoppingcartTmpPtr != NULL)//Found section.
        {
                if(shoppingcartTmpPtr->caption[0] == 0)strcpy(shoppingcartTmpPtr->caption, "Shopping Cart");
                swapPointers((void **)&shoppingcartPtr, (void **)&shoppingcartTmpPtr);
                freeSourceLines(&shoppingcartTmpPtr);
        }
        if(wadoserversTmpPtr != NULL)//Found section.
        {
                swapPointers((void **)&wadoserversPtr, (void **)&wadoserversTmpPtr);
                freewadoservers();
        }
        if(webdefaultsTmpPtr != NULL)//Found section.
        {
                swapPointers((void **)&webdefaultsPtr, (void **)&webdefaultsTmpPtr);
                freewebdefaults();
        }
        if(unknownSectionTmpPtr != NULL)//Found section.
        {
                swapPointers((void **)&unknownSectionPtr, (void **)&unknownSectionTmpPtr);
                freesection();
        }
// Just for show?
        if(dumpIni)
        {
                DumpIni();
                return FALSE;
        }
//Just for lua and status display.
        gppstime += (time(NULL) - nowT) & INT_MAX;
        gpps += 1;
        return TRUE;
}

BOOL IniValue        ::       readIniFile()
{
//        int                     fd;
//        struct                  stat stbuf;
        *errorBufferPtr = 0;
        if(iniStringsPtr->iniFile[0] == 0)return errorToBuffer("No file to load ini values from.\n");
        iSize = fileToBufferWithErr( (void **)&pSrc, iniStringsPtr->iniFile, errorBufferPtr,
                                    sizeof(iniStringsPtr->errorBuffer), true);
        if (iSize == 0) return false;
/*
// Open  the ini file.
        fd = open(iniStringsPtr->iniFile, O_RDONLY);
        if (fd == -1)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                         "Can not open the needed file %s: %s\n", iniStringsPtr->iniFile,
                         strerror(errno));
                return FALSE;
        }
// Get the file length.
        if ((fstat(fd, &stbuf) != 0) || (((stbuf.st_mode) & S_IFMT) != S_IFREG))
        {
                close(fd);
                if((((stbuf.st_mode) & S_IFMT) != S_IFREG))
                {
                        snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                                 "%s is not a file!\n", iniStringsPtr->iniFile);
                        return false;
                }
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                         "Can not get the size of %s: %s\n", iniStringsPtr->iniFile,
                         strerror(errno));
                return FALSE;
        }
// Check file size
        if(stbuf.st_size <= 0)
        {
                close(fd);
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                                 "The file %s size error, size = %lld\n",
                                 iniStringsPtr->iniFile, stbuf.st_size);
                return FALSE;
        }
// Make the buffer.
        pSrc = (char*)malloc(stbuf.st_size + 2);
        if (!pSrc)
        {
                close(fd);
                return (printAllocateError((int)iSize + 2 , "the ini buffer"));
        }
// Read it into memory.
        iSize = read(fd, pSrc, stbuf.st_size);
        if (iSize  != (size_t)stbuf.st_size)
        {
                close(fd);
                if(pSrc)free(pSrc);
                pSrc = NULL;
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                         "Only read %ld bytes of %ld from the ini file %s\n",
                         iSize, (size_t)stbuf.st_size, iniStringsPtr->iniFile );
                return FALSE;
        }
// Done with it.
        if(close(fd) != 0)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), "Error closing the file %s: %s",
                         iniStringsPtr->iniFile, strerror(errno));
                return false;
        }
        
// Make a clean CR/LF at the end and a zero to terminated string.
        while(iSize > 0)
        {
                if(pSrc[iSize - 1] != '\r' && pSrc[iSize - 1] != '\n')break;
                iSize--;
        }
        pSrc[iSize++] = '\r';
        pSrc[iSize++] = '\n';// Needed for ini writes.
        pSrc[iSize++]   = '\0';*/
// Back to the start
        i = 0;
        
        return true;
}

// Here creates the lua section pointer. Set defaults here if any.
//ADD new values or arrays to lua section here and  3 or 4 other places.
//lua value add
//lua array add
BOOL IniValue        ::       readlua()
{
        BOOL returnVal = true;
        
        if(luaTmpPtr == NULL)
        {
                if((luaTmpPtr = (luaIni *)malloc(sizeof(luaIni))) == NULL)
                        return (printAllocateError(sizeof(luaIni), "lua ini data"));
                luaTmpPtr->ExportConverterPtr                   = NULL;
                luaTmpPtr->iniTablePtr                          = NULL;
                luaTmpPtr->ArchivePtr                           = NULL;
                luaTmpPtr->CompressionPtr                       = NULL;
                luaTmpPtr->ImportPtr                            = NULL;
                luaTmpPtr->MergeSeriesPtr                       = NULL;
                luaTmpPtr->MergeStudiesPtr                      = NULL;
                luaTmpPtr->ModalityWorkListQueryResultPtr       = NULL;
                luaTmpPtr->MoveDevicePtr                        = NULL;
                luaTmpPtr->QueryResultPtr                       = NULL;
                luaTmpPtr->QueryPtr                             = NULL;
                luaTmpPtr->RetrieveResultPtr                    = NULL;
                luaTmpPtr->RetrievePtr                          = NULL;
                luaTmpPtr->RejectedImageWorkListPtr             = NULL;
                luaTmpPtr->RejectedImagePtr                     = NULL;
                luaTmpPtr->WorkListQueryPtr                     = NULL;
                luaTmpPtr->association[0]                       = 0;
                luaTmpPtr->background[0]                        = 0;
                luaTmpPtr->clienterror[0]                       = 0;
                luaTmpPtr->command[0]                           = 0;
                luaTmpPtr->endassociation[0]                    = 0;
                luaTmpPtr->nightly[0]                           = 0;
                luaTmpPtr->startup[0]                           = 0;
        }
        iVoidPtr = NULL;
        while(i < iSize)
        {
                iVoidHandle = &iVoidPtr;
                switch(getTypeForLua())
                {
                        case DPT_STRING:
                                setIniString((char *)*iVoidHandle, iStrLen);
                                break;
                        case DPT_UNUSED:
                                goToNextDataLine();
                                break;
                        case DPT_STRING_ARRAY:
                                if(iNum < 0)// devices that end in s are unused.
                                {
                                        goToNextDataLine();
                                        break;
                                }
                                returnVal = setStringForArray((Array<char *> ***)iVoidHandle);
                                break;
                        case DPT_END:
                                return TRUE;
                        case DPT_SKIP:
                                break;
                        case DPT_UNKNOWN:
                                unknownEntry(&(luaTmpPtr->iniTablePtr));
                                break;
                        case DPT_INT_ARRAY:
                        case DPT_UINT_ARRAY:
                        case DPT_DBTYPE:
                        case DPT_EDITION:
                        case DPT_FOR_ASSOC:
                        case DPT_FILE_COMP:
                        case DPT_MICROP:
                        case DPT_INT:
                        case DPT_UINT:
                        case DPT_TIME:
                        case DPT_PATH:
                        case DPT_BOOL:
#ifdef DGJAVA
                        case DPT_MIN_JAVA:
#endif
                                break;
                }
        }
        return returnVal;
}

BOOL IniValue        ::       readScripts()
{
        while(i < iSize)
        {
                switch (pSrc[i++])
                {                
                        case '[':// New section!
                        case 0:// The end
                                return TRUE;
                        case ']':// End of current section!
                        case ' '://Space
                        case '\r':// End of line?
                                break; // The switch skipps over what it was.
                        default:
                                unknownEntry(&(scriptsTmpPtr));
                }
        }
        return TRUE;
}

BOOL IniValue        ::       readsourceLines(sourceLines **lsourceLinesH)
{
        bool madeHereUnused = false, returnVal = true;
        if(*lsourceLinesH == NULL)
        {
                if((*lsourceLinesH = (sourceLines *)malloc(sizeof(sourceLines))) == NULL)
                        return (printAllocateError(sizeof(sourceLines), "data structure ini data"));
//                (*lsourceLinesH)->sourceOrLine0 = false;//Killed lines and sourceOrLine0.
                (*lsourceLinesH)->caption[0] = 0;
                (*lsourceLinesH)->source[0] = 0;
                strcpy((*lsourceLinesH)->header,"Content-type: text/html\n");
//                (*lsourceLinesH)->linesPtr = NULL;//Killed lines and sourceOrLine0.
                (*lsourceLinesH)->iniTablePtr = NULL;
                madeHereUnused = true;
        }
//        sourceLines *sourceLinesPtr, **sourceLinesHandle;

        while(i < iSize)
        {
                iNum = 0;
        //Create a temporary handle for getTypeForsourceLines to change.
//                sourceLinesPtr = *lsourceLinesH;
//                sourceLinesHandle = &sourceLinesPtr;
                iVoidPtr = *lsourceLinesH;
                iVoidHandle = &iVoidPtr;
                iVoidDHandle = &iVoidHandle;
                switch (getTypeForsourceLines())
                {
                        case DPT_STRING:
                                setIniString((char *)*iVoidHandle, iStrLen);
                                if((char *)*iVoidHandle != NULL)
                                {
                                        madeHereUnused = false;//Found something
                                //Check if the string lable was source.
//                                        if(iNum == 1)(*lsourceLinesH)->sourceOrLine0 = true;//Killed lines and sourceOrLine0.
                                }
                                break;
                        case DPT_STRING_ARRAY://Killed lines and sourceOrLine0, line0,1,.. is the only array.
/*                                if(iNum < 0)// devices that end in s are unused.
                                {
                                        goToNextDataLine();
                                        break;
                                }
                                returnVal = setStringForArray((Array<char *> ***)iVoidHandle);
                                if((*lsourceLinesH)->linesPtr != NULL)
                                {
                                        madeHereUnused = false;//Found something
                                        (*lsourceLinesH)->sourceOrLine0 = true;//Only lines is an array
                                }
                                break;*/
                        case DPT_UNUSED:
                                goToNextDataLine();
                                break;
                        case DPT_END:
                                return TRUE;
                        case DPT_SKIP:
                                break;
                        case DPT_UNKNOWN:
                                unknownEntry(&((*lsourceLinesH)->iniTablePtr));
                                if((*lsourceLinesH)->iniTablePtr != NULL) madeHereUnused = false;//Found something
                                break;
                        case DPT_INT_ARRAY:
                        case DPT_UINT_ARRAY:
                        case DPT_DBTYPE:
                        case DPT_EDITION:
                        case DPT_FOR_ASSOC:
                        case DPT_FILE_COMP:
                        case DPT_MICROP:
                        case DPT_INT:
                        case DPT_UINT:
                        case DPT_TIME:
                        case DPT_PATH:
                        case DPT_BOOL:
#ifdef DGJAVA
                        case DPT_MIN_JAVA:
#endif
                                break;
                }
        }
        if( madeHereUnused )//We made it and did not use it.
        {
                free(*lsourceLinesH);
                *lsourceLinesH = NULL;
        }
        return returnVal;
}

BOOL IniValue        ::       readsscscp()
{
        BOOL returnVal = true;
        char tempStr[SHORT_NAME_LEN ];
        tempStr[0] = 0;
        
        while(i < iSize)
        {
                iVoidPtr = NULL;
                iVoidHandle = &iVoidPtr;
                iNum = 0;
                switch(getTypeForsscscp())
                {
                        case DPT_PATH:
                                setIniFullPath();
                                break;
                        case DPT_BOOL:
                                setIniBool();
                                break;
                        case DPT_STRING:
                                setIniString((char *)*iVoidHandle, iStrLen);
                                break;
                        case DPT_INT:
                                setIniINT((int *)*iVoidHandle);
                                break;
                        case DPT_UINT:
                                if(iNum == 1) sscscpTmpPtr->TruncateFieldNamesLen = getReplyLength();
                                setIniUINT((unsigned int *)*iVoidHandle);
                                break;
                        case DPT_UNUSED:
                                goToNextDataLine();
                                break;
                        case DPT_STRING_ARRAY:
                                if(iNum < 0)// devices that end in s are unused.
                                {
                                        goToNextDataLine();
                                        break;
                                }
                                returnVal = setStringForArray((Array<char *> ***)iVoidHandle );
                                break;
                        case DPT_INT_ARRAY:
                                if(iNum < 0)// devices that end in s are unused.
                                {
                                        goToNextDataLine();
                                        break;
                                }
                                setIntForArray((Array<int *> ***)iVoidHandle);
                                break;
                        case DPT_UINT_ARRAY:
                                if(iNum < 0)// devices that end in s are unused.
                                {
                                        goToNextDataLine();
                                        break;
                                }
                                setUIntForArray((Array<unsigned int *> ***)iVoidHandle);
                                break;
                        case DPT_DBTYPE:
                                if(getBoolWithTemp())// If FALSE, leave alone.
                                {
                                        switch ((enum DBTYPE) iNum)
                                        {
                                                case DT_MYSQL:
#ifdef USEMYSQL
                                                        sscscpTmpPtr->db_type = DT_MYSQL;
#endif
                                                        break;
                                                case DT_POSTGRES:
#ifdef POSTGRES
                                                        sscscpTmpPtr->db_type = DT_POSTGRES;
#endif
                                                        break;
                                                case DT_SQLITE:
#ifdef USESQLITE
                                                        sscscpTmpPtr->db_type = DT_SQLITE;
#endif
                                                        break;
                                                case DT_DBASEIII://No way to select this!
                                                case DT_DBASEIIINOINDEX://No way to select this!
                                                case DT_NULL://No way to select this!
                                                case DT_ODBC://No way to select this except by default!
                                                        break;
                                        }
                                }
                                break;
                        case DPT_EDITION:
//                                if(pSrc[i] == ' ') i++; //Remove leading space
                                setStringLim(tempStr);
                                sscscpTmpPtr->Edition = E_WORKGROUP;
                                if(tempStr[0] != 0)
                                {
                                        if(strileq(tempStr, "personal"))
                                                sscscpTmpPtr->Edition = E_PERSONAL;
                                        else if (strileq(tempStr, "enterprise"))
                                                sscscpTmpPtr->Edition = E_ENTERPRISE;
                                }
                                break;
                        case DPT_END:
                                readsscscpFixes();
                                return TRUE;
                        case DPT_FOR_ASSOC:
//                                if(pSrc[i] == ' ') i++; //Remove leading space
                                setStringLim(tempStr);
                                if(tempStr[0] == 0)break;
                                if      (strileq(tempStr, "patient")) sscscpTmpPtr->Level = LT_PATIENT;
                                else if (strileq(tempStr, "study")) sscscpTmpPtr->Level = LT_STUDY;
                                else if (strileq(tempStr, "series"))sscscpTmpPtr->Level = LT_SERIES;
                                else if (strileq(tempStr, "image")) sscscpTmpPtr->Level = LT_IMAGE;
                                else if (strileq(tempStr, "sopclass"))sscscpTmpPtr->Level = LT_SOPCLASS;
                                else if (strileq(tempStr, "global")) sscscpTmpPtr->Level = LT_GLOBAL;
                                break;
#ifdef DGJAVA
                        case DPT_MIN_JAVA:
                                setStringLim(tempStr);
                                if(tempStr[0] == 0)break;
                                if      (strileq(tempStr, "1.1")) sscscpTmpPtr->JavaMinVersion = JNI_VERSION_1_1;
                                else if (strileq(tempStr, "1.4")) sscscpTmpPtr->JavaMinVersion = JNI_VERSION_1_4;
                                else if (strileq(tempStr, "1.6")) sscscpTmpPtr->JavaMinVersion = JNI_VERSION_1_6;
                                else sscscpTmpPtr->JavaMinVersion = JNI_VERSION_1_2;
#endif
                        case DPT_FILE_COMP:
                                setIniUINT(&(sscscpTmpPtr->FileCompressMode));
                                if (sscscpTmpPtr->FileCompressMode > 4)
                                        sscscpTmpPtr->FileCompressMode = 0;
                                if (sscscpTmpPtr->FileCompressMode != 0)
                                {
                                        if(sscscpTmpPtr->DroppedFileCompression[0] == 0)
                                        {
                                                sscscpTmpPtr->DroppedFileCompression[0] = 'n';
                                                sscscpTmpPtr->DroppedFileCompression[1] =
                                                sscscpTmpPtr->FileCompressMode + 0x30;
                                                sscscpTmpPtr->DroppedFileCompression[2] = 0;
                                        }
                                        if(sscscpTmpPtr->IncomingCompression[0] == 0)
                                        {
                                                sscscpTmpPtr->IncomingCompression[0] = 'n';
                                                sscscpTmpPtr->IncomingCompression[1] =
                                                sscscpTmpPtr->FileCompressMode + 0x30;
                                                sscscpTmpPtr->IncomingCompression[2] = 0;
                                        }
                                }
                                break;
                        case DPT_MICROP:
                                char tempStr2[sizeof(sscscpTmpPtr->MicroPACS)];
                                setStringLim(tempStr2);
                                if(!streq(sscscpTmpPtr->MicroPACS, tempStr2))
                                {
                                        snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                                                "A second MicroPACS entry was found that is different.\nfirst: %s\nsecond: %s\n"
                                                ,sscscpTmpPtr->MicroPACS, tempStr2);
                                        return FALSE;
                                }
                                break;
                        case DPT_SKIP:
                                break;
                        case DPT_TIME:
                                setIniTime();
                                break;
                        case DPT_UNKNOWN:
                                unknownEntry(&(sscscpTmpPtr->iniTablePtr));
                                break;
                }
        }
        readsscscpFixes();
        return returnVal;
}

void IniValue        ::       readsscscpFixes()
{
        strcat(sscscpTmpPtr->AllowTruncate, ",");
        if (sscscpTmpPtr->WorkListReturnsISO_IR_100 >= 0)
                sscscpTmpPtr->WorkListReturnsISO_IR_100 *= 100;
        else sscscpTmpPtr->WorkListReturnsISO_IR_100 = sscscpTmpPtr->WorkListReturnsISO_IR;
}

BOOL IniValue        ::       readUknownSection()
{
        sectionTable *tempTablePtr = NULL;
        unsigned int cnt, aSize;

        iniStringsPtr->currentSection[0] = 0;
        setIniString(iniStringsPtr->currentSection, sizeof(iniStringsPtr->currentSection));
        if (iniStringsPtr->currentSection[0] == 0 || i >= iSize) return FALSE;
        if(unknownSectionTmpPtr == NULL) unknownSectionTmpPtr = new Array < sectionTable *>;
        aSize = unknownSectionTmpPtr->GetSize();
        for(cnt = 0; cnt < aSize; cnt++)
        {
                tempTablePtr = unknownSectionTmpPtr->Get(cnt);
                if(streq(iniStringsPtr->currentSection, tempTablePtr->section)) break;
                tempTablePtr = NULL;
        }
        if(tempTablePtr == NULL)
        {
                if((tempTablePtr = (sectionTable *)malloc(sizeof(sectionTable))) == NULL)
                        return (printAllocateError(sizeof(sectionTable) ,"Unknown section table"));
                strcpy(tempTablePtr->section, iniStringsPtr->currentSection);
                tempTablePtr->iniTablePtr = NULL;
        }
        while (i < iSize)
        {
                if(pSrc[i] == '[' || pSrc[i] == '\0')break;
                if(pSrc[i] != ']' && pSrc[i] != '\r')
                {
                        i++; // Undo backup in unknownEntry, used to get switch letter.
                        unknownEntry(&(tempTablePtr->iniTablePtr));
                }
                else i++;
        }
        if(aSize == 0 || aSize <= cnt)
                unknownSectionTmpPtr->Add(tempTablePtr);
        if(pSrc[i] == '[')i++;
        return TRUE;
}

BOOL IniValue        ::       readwadoservers()
{
        if(wadoserversTmpPtr == NULL)
        {
                if((wadoserversTmpPtr = (wadoserversIni *)malloc(sizeof(wadoserversIni))) == NULL)
                        return (printAllocateError(sizeof(wadoserversIni) ,"Wado servers"));
                wadoserversTmpPtr->bridge[0] = 0;
                wadoserversTmpPtr->iniTablePtr = NULL;
        }
        while(i < iSize)
        {
                switch (pSrc[i++])
                {                
                        case '[':// New section!
                        case 0:// The end
                                return TRUE;
                        case ']':// End of current section!
                        case ' '://Space
                        case '\r':// End of line?
                                break; // The switch skipps over what it was.
                        case 'b':
                        case 'B':
                                if(lstrileq("ridge")) 
                                {
                                        setIniString(wadoserversTmpPtr->bridge, sizeof(wadoserversTmpPtr->bridge));
                                        break;
                                }
                        default:
                                unknownEntry(&(wadoserversTmpPtr->iniTablePtr));
                }
        }
        return TRUE;
}

BOOL IniValue        ::       readwebdefaults()
{
        if(webdefaultsTmpPtr == NULL)
        {
                if((webdefaultsTmpPtr = (webdefaultsIni *)malloc(sizeof(webdefaultsIni))) == NULL)
                        return (printAllocateError(sizeof(webdefaultsIni) ,"Web defaults"));
                webdefaultsTmpPtr->address[0]           = 0;
                strcpy(webdefaultsTmpPtr->compress,     "n4");
                strcpy(webdefaultsTmpPtr->dsize,        "0");
                strcpy(webdefaultsTmpPtr->graphic,      "bmp");
                strcpy(webdefaultsTmpPtr->iconsize,     "48");
                webdefaultsTmpPtr->port[0]              = 0;
                strcpy(webdefaultsTmpPtr->size,         "5124");
                strcpy(webdefaultsTmpPtr->viewer,       "seriesviewer");
                webdefaultsTmpPtr->iniTablePtr          = NULL;
                webdefaultsTmpPtr->studyviewer[0]       = 0;
        }
//        void *voidPtr = NULL;
//        void **voidHandle;
        iVoidPtr = NULL;
        iVoidHandle = &iVoidPtr;
        
        while(i < iSize)
        {
                switch (getTypeForWebDefaults())
                {
                        case DPT_STRING:
                                setIniString((char *)*iVoidHandle, iStrLen);
                                break;
                        case DPT_UNUSED:
                                goToNextDataLine();
                                break;
                        case DPT_END:
                                return TRUE;
                        case DPT_SKIP:
                                break;
                        case DPT_UNKNOWN:
                                unknownEntry(&(webdefaultsTmpPtr->iniTablePtr));
                                break;
                        case DPT_STRING_ARRAY:
                        case DPT_INT_ARRAY:
                        case DPT_UINT_ARRAY:
                        case DPT_DBTYPE:
                        case DPT_EDITION:
                        case DPT_FOR_ASSOC:
                        case DPT_FILE_COMP:
                        case DPT_MICROP:
                        case DPT_INT:
                        case DPT_UINT:
                        case DPT_TIME:
                        case DPT_PATH:
                        case DPT_BOOL:
#ifdef DGJAVA
                        case DPT_MIN_JAVA:
#endif
                                break;
                }
        }
        return TRUE;
}

BOOL IniValue        ::       printAllocateError(int allocSize ,const char *what)
{
        snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), "Could not allocate %d bytes for %s.\n", allocSize, what);
        return FALSE;
}

BOOL IniValue        ::       SetBasePath(const char *argv0)
{
        size_t baseLen;
        char tempBase[MAX_PATH];
        struct	stat st;

        strcpylim(tempBase, argv0, MAX_PATH - 2);
        if(strrchr(argv0, PATHSEPCHAR))// Take off any file name and last path char.
                *(strrchr(tempBase, PATHSEPCHAR)) = 0;
        baseLen = strlen(tempBase);
        if(baseLen > MAX_PATH - 4) 
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), 
                         "The basePath is to long to append file names:%lu\n",(unsigned long)baseLen);
                return FALSE;
        }
        if(baseLen == 0)return errorToBuffer("The basePath is length is 0.\n");
        if(stat(tempBase, &st) != 0)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), "Base directory %s does not exist.\n", tempBase);
                return FALSE;
        }
        if (!(st.st_mode & S_IFDIR))
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), "%s is not a directory.\n", tempBase);
                return FALSE;
        }
        tempBase[baseLen++] = PATHSEPCHAR;
        tempBase[baseLen++] = 0;
        strcpy(iniStringsPtr->basePath, tempBase);
        return TRUE;
}

//Will only change the value if is TRUE of FALSE, else no change
void IniValue        ::       setIniBool()
{
        if(goToStartIsBlank()) return;
        if(pSrc[i] == '1' || strileq(&pSrc[i], "true") || strileq(&pSrc[i], "yes")) **(BOOL **)iVoidHandle = TRUE;
        else if(pSrc[i] == '0' || strileq(&pSrc[i], "false") || strileq(&pSrc[i], "no"))**(BOOL **)iVoidHandle = FALSE;
        goToNextDataLine();
        return;
}

BOOL IniValue        ::       SetIniFile(const char *iniFile)
{
        size_t slen;
        struct	stat st;

        if( iniFile != NULL) slen = strnlen(iniFile, MAX_PATH);
        else slen = 0;
// Default ini values.
        if(slen < 4)
        {
#ifndef UNIX
                strcpy(iniStringsPtr->iniFile, ".\\dicom.ini");// Set the default.
#else
                strcpy(iniStringsPtr->iniFile, "dicom.ini");// Set the default.
#endif
                makeFullPath(iniStringsPtr->iniFile);
        }
        // Its the full file name or a name change.
        else if(strileq(iniFile + slen - 4, ".ini"))
        {
                strcpylim(iniStringsPtr->iniFile, iniFile, MAX_PATH);
                if(!strrchr(iniStringsPtr->iniFile, PATHSEPCHAR)) makeFullPath(iniStringsPtr->iniFile);//Its just the name.
                else if(!SetBasePath(iniStringsPtr->iniFile)) return FALSE;// Full name with path
        }
        else// Just a path
        {                                               // No ini file, check if directory.
                strcpylim(iniStringsPtr->iniFile, iniFile, MAX_PATH - 9);
                if(iniStringsPtr->iniFile[strlen(iniStringsPtr->iniFile) - 1] != PATHSEPCHAR)
                        strcat(iniStringsPtr->iniFile, PATHSEPSTR);//Taken off by SetBasePath.
                SetBasePath(iniStringsPtr->iniFile);
                strcpylim(iniStringsPtr->iniFile, iniStringsPtr->basePath, MAX_PATH - 9);
                strcat(iniStringsPtr->iniFile, "dicom.ini"); // Add the default.
        }
//Just a name change, no PATHSEPCHAR
        if(stat(iniStringsPtr->iniFile, &st) != 0)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer),
                         "Configuration file %s not found, but required.\n", iniStringsPtr->iniFile);
                return FALSE;
        }
        return TRUE;
}

void IniValue        ::       setIniFullPath()
{
        if(goToStartIsBlank()) return;
        setIniString((char *)*iVoidHandle, MAX_PATH );
        makeFullPath((char *)*iVoidHandle);
}

void IniValue        ::       setIniINT(int *outInt)
{
        BOOL isNeg;
        
        if(goToStartIsBlank()) return;
        
        isNeg = FALSE;
        if(pSrc[i] == '-')
        {
                isNeg = TRUE;
                i++;
        }
        setIniUINT((unsigned int *)outInt);
        if(isNeg) *outInt = 0 - *outInt;
}

void IniValue        ::       setIniString(char *outStr, unsigned int limit)
{
        if(goToStartIsBlank()) return;
        limit--;
        while(limit > 0 && i < iSize)
        {
                switch (pSrc[i])
                {
                        case ']'://Section end.
                                i++;// Fall through to normal end.
                        case '\r'://Normal end.
                        case '\n'://Normal end.
                                goToNextDataLine();
                        case 0: //End of input string?
                                *outStr = 0;
                                return;
                        case '\"':
                                *outStr++ = pSrc[i++];//Copy the quote for commands.
//                                i++;//Skip the quote, than "almost" blind copy. Importconverter problem
                                while(limit > 0 && i < iSize)
                                {
                                        if(pSrc[i] == '\"')
                                        {
                                                *outStr++ = pSrc[i++];//Copy the quote for commands
//                                                i++;//Dump the last Quote
                                                break;
                                        }
                                // The only thing we need to escappe in a quote is a quote char.
                                        else if(pSrc[i] == '\\' && pSrc[i + 1] == '\"')
                                        {
                                                i++;//Skip the escape char.
                                                if(i >= iSize) break;// End?
                                        }
                                        *outStr++ = pSrc[i++];
                                        limit--;
                                }
                                break;
                        case '\\'://Check for an escaped qoute, all else passes.
                                if(pSrc[i + 1] == '\"')
                                {
                                        i++;//Skip the backslash, copy the quote on fall through.
                                        if(i >= iSize)break;// End?
                                }
                        default:
                                *outStr++ = pSrc[i++];
                                limit--;
                                break;
                }
        }
        *outStr = 0;//End the string in all cases.
        goToNextDataLine();
        return;
}

void IniValue        ::       setIntForArray(Array <int *> ***arrayHand)
{
        if(goToStartIsBlank()) return;
        
        int tempInt = 0;
        
        setIniINT(&tempInt);
        addToIntArray(tempInt, *arrayHand);
}

void IniValue        ::       setIniTime()
{
        struct tm *timeSructPtr;
        char tempStr[VSHORT_NAME_LEN ], *min, *sec;
        
        timeSructPtr = (struct tm *)*iVoidHandle;
        tempStr[0] = 0;
        setIniString(tempStr, sizeof(tempStr));
        if(tempStr[0] == 0) return;
        min = strchr(tempStr, ':');
        if(min != NULL)
        {
                *min++ = 0;//Break into two strings
                sec = strchr(tempStr, ':');
                if(sec != NULL)
                {
                        *sec++ = 0;//Break into three strings
                        timeSructPtr->tm_sec = atoi(sec);
                }
                timeSructPtr->tm_min = atoi(min);
        }
        timeSructPtr->tm_hour = atoi(tempStr);
}

void IniValue        ::       setIniUINT(unsigned int *outUInt)
{
        unsigned int temp, total = 0;
        
        if(goToStartIsBlank()) return;
        if(pSrc[i] == '0' && (pSrc[i + 1] == 'x' || pSrc[i + 1] == 'X'))
        {//HEX
                i += 2;
                while(pSrc[i] != '\r' && pSrc[i] != 0 && i < iSize )
                {
                        if(pSrc[i] >= '0' && pSrc[i] <= '9') temp = (pSrc[i] - '0');
                        else if(pSrc[i] >= 'A' && pSrc[i] <= 'F') temp = (pSrc[i] - 0x37);// -A + 10
                        else if(pSrc[i] >= 'a' && pSrc[i] <= 'f') temp = (pSrc[i] - 0x57);// -a + 10
                        else break;
                        total = (total * 0xF) + temp;
                        i++;
                }
        }
        else
        {//Dec
                while(pSrc[i] != '\r' || pSrc[i] != 0 && i < iSize )
                {
                        if(pSrc[i] >= '0' && pSrc[i] <= '9') temp = (pSrc[i] - '0');
                        else break;
                        total = (total * 10) + temp;
                        i++;
                }
        }
        *outUInt = total;
        goToNextDataLine();
        return;
}

BOOL IniValue        ::       setStringForArray(Array <char *> ***arrayDHand)
{
        char tempStr[BUF_CMD_LEN];
        
        if(iNum < 0) return false; //An array count, ex:ExportConverters, not used.
        tempStr[0] = 0;
        setStringLim(tempStr);
        if (tempStr[0] != 0)  return addToCharArray(tempStr, *arrayDHand);
        return true;
}

void IniValue        ::       setUIntForArray(Array <unsigned int *> ***arrayHand)
{
        if(goToStartIsBlank()) return;
        
        unsigned int tempInt = 0;
        
        setIniUINT(&tempInt);
        addToIntArray(tempInt,(Array <int *> **) *arrayHand);
}

BOOL IniValue        ::       skipComments()
{
        i += 2;
        while (i < iSize) if (pSrc[i++] == '*' && i < iSize && (pSrc[i++] == '/')) break;
        while (i < iSize)
        {
                switch (pSrc[i])
                {
                        case '\r':
                        case '\n':
                        case '\t':
                        case ' ':
                                i++;
                                break;
                        default:
                                return true;
                }
        }
        return false;
}

BOOL IniValue        ::       swapConfigFiles()
{
        if(pSrc)free(pSrc);
        pSrc = NULL;
        if(fclose(tempFile) != 0)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), "Error closing the file %s: %s",
                         iniStringsPtr->newConfigFile, strerror(errno));
                unlink(iniStringsPtr->newConfigFile);
                return false;
        }
        if(unlink(iniStringsPtr->iniFile) != 0)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), "Error deleting the file %s: %s",
                         iniStringsPtr->newConfigFile, strerror(errno));
                return false;
        }
        if(rename(iniStringsPtr->newConfigFile, iniStringsPtr->iniFile) != 0)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), "Error renaming the file %s to %s: %s",
                         iniStringsPtr->newConfigFile, iniStringsPtr->iniFile, strerror(errno));
                return false;
        }
        return true;

}

void IniValue        ::       swapPointers(void **handleOne, void **handleTwo)
{
        void *locPtr = *handleOne;
        *handleOne = *handleTwo;
        *handleTwo = locPtr;
}

char* IniValue        ::      threadError()
{
        char *returnBuf;
        
        returnBuf = (char *)malloc(12);
        snprintf(returnBuf, 12, "Thread Error");
        return returnBuf;
}

BOOL IniValue        ::       unknownEntry(Array <iniTable *> **theTableHdl)
{
        iniTable *newTablePtr;
        unsigned int aSize;
        
        i--;// Back up one letter.
        
        if((newTablePtr = (iniTable *)malloc(sizeof(iniTable))) == NULL)
                return (printAllocateError(sizeof(iniTable), "ini table for unknown ini data"));
        lstrcpysplim(newTablePtr->valueName, sizeof(newTablePtr->valueName));
        if(i >= iSize)
        {
                free(newTablePtr);
                return FALSE;
        }
        while(pSrc[i] == ' ')i++;
        if(pSrc[i] != '=')
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), "Ini name %s has no equal sign.\n", newTablePtr->valueName);
                free(newTablePtr);
                return FALSE;
        }
        i++;
        setIniString(newTablePtr->value, SHORT_NAME_LEN);
        if(*theTableHdl == NULL)
        {
                *theTableHdl = new Array<iniTable *>;
                if(*theTableHdl == NULL)
                {
                        free(newTablePtr);
                        return errorToBuffer("Cound not create new ini table array.\n");
                }
                aSize = 0;
        }
        else aSize = (*theTableHdl)->GetSize();
        for (unsigned int cnt = 0; cnt < aSize; cnt++)
        {
                iniTable *oldTablePtr = (*theTableHdl)->Get(cnt);
                if(strieq(newTablePtr->valueName, oldTablePtr->valueName))
                {
                        (*theTableHdl)->ReplaceAt(newTablePtr, cnt);
                        return true;
                }
        }
        (*theTableHdl)->Add(newTablePtr);
        return TRUE;
}

void IniValue        ::       unknownPrint(Array<iniTable *>*iniTablePtr, FILE *outputFile)
{
        if(iniTablePtr == NULL) return;
        fprintf(outputFile,"*** Unknown table section. ***\n");
        unsigned int tSize, cnt;
        tSize = iniTablePtr->GetSize();
        for (cnt = 0; cnt < tSize; cnt++)
        {
                iniTable *iniTableDataPtr = iniTablePtr->Get(cnt);
                fprintf(outputFile,"%s = %s\n",
                        iniTableDataPtr->valueName,iniTableDataPtr->value);
        }
        fprintf(outputFile,"\n");
}

BOOL IniValue        ::       writeByPutParms(const char *what)
{
        // Going to write, get the time.
        char timeString[128], buf[64];
        time_t timeOfDay;
        size_t timeLen;
        
        timeOfDay = time(NULL);
        UNUSED_ARGUMENT(buf);//Stop gcc4.2 warning bcb
        if(timeOfDay != -1)
        {
                strcpylim(timeString, ctime_r(&timeOfDay, buf),sizeof(timeString));
                timeLen = strnlen(timeString, sizeof(timeString));
        // ctime(_r) adds a /n  (in unix), remove it.
                while(timeString[timeLen -1] == '\r' ||
                      timeString[timeLen -1] == '\n') timeLen--;
        }
        else strcpy(timeString, "unKnown tine");
        if(!writeTonewConfigFile("# ",2)) return false;
        if(!writeTonewConfigFile(what, strlen(what))) return false;
        if(!writeTonewConfigFile(" by put_param on ", 17)) return false;
        if(!writeTonewConfigFile(timeString, timeLen)) return false;
        if(!writeTonewConfigFile("\r\n",2)) return false;
        return true;
}
BOOL IniValue        ::       writeNameValue(const char *theName, const char *theValue)
{
        int stringLen, cnt, spaceCnt;
        char spaces[SHORT_NAME_LEN];
        
        stringLen = strnlenint(theName, SHORT_NAME_LEN);
        if(!writeTonewConfigFile(theName, stringLen)) return false;
        spaceCnt = EQ_INDENT - stringLen;
        if(spaceCnt < 1) spaceCnt = 1;
        for(cnt = 0; cnt < spaceCnt; cnt++) spaces[cnt] = ' ';
        spaces[cnt++] = '=';
        spaces[cnt++] = ' ';
        spaces[cnt] = 0;
        if(!writeTonewConfigFile(spaces, cnt)) return false;
        if(theValue != NULL)
        {
                stringLen = strnlenint(theValue, BUF_CMD_LEN);
                if(!writeTonewConfigFile(theValue, stringLen)) return false;
        }
        if(!writeTonewConfigFile("\r\n",2)) return false;
        return true;
}

BOOL IniValue        ::       WriteToIni(const char *theName, const char *theValue)
{
        char lName[2 * SHORT_NAME_LEN], *inWhat;
        
        strcpylim(lName, theName, SHORT_NAME_LEN);
        inWhat = strchr(lName, NAME_SECTION_SEPERATOR);
        if(inWhat != NULL) *inWhat++ = 0;//Break into two strings
        else inWhat = sscscpPtr->MicroPACS;
        return WriteToIni(lName, theValue, inWhat);
}

BOOL IniValue        ::       WriteToIni(const char *theName, const char *theValue, const char *theSection)
{
        char *oldValue;
        BOOL same = false, unknown = false;
        size_t loci;
// Check all inputs and get the file.
        if(theName == NULL || theSection == NULL ||
           strnlen(theName, SHORT_NAME_LEN) > SHORT_NAME_LEN - 1 ||
           strnlen(theSection, SHORT_NAME_LEN) > SHORT_NAME_LEN - 1)
                return errorToBuffer("Input size error");
// theValue can be NULL.
        if(theValue !=NULL && strnlen(theValue, BUF_CMD_LEN) > BUF_CMD_LEN - 1) return false;
// All good inputs, were going to write unless no change.
        oldValue = GetIniString(theName, theSection);
        if(oldValue != NULL)
        {
                if(streq(oldValue, "Unknown!")) unknown = true;
                else if(strieq(oldValue, theValue)) same = true;
                free(oldValue);
                if(same) return true;
        }
// Open the files.
        if(!openNewConfigFile()) return false;
        if(!readIniFile())
        {
                errorCleanNewConfigFile();
                return false;
        }
// Look for the section.
        while(goToNextSection() && !strnieq(iniStringsPtr->currentSection, theSection, sizeof(iniStringsPtr->currentSection)));
        if(i < iSize)
        {// Found the right section!  Write until the line after the section name.
                if(!writeTonewConfigFile(pSrc, i)) return false;
                loci = i;//Save amount writen.
                if(!unknown)
                {//May be known, but a value not writen in the file.
                        if(goToStartOfName(theName))
                        {
                        // Write until the line before the name 
                                if(!writeTonewConfigFile(&pSrc[loci], lastLine - loci)) return false;
                                if(keepLine(&pSrc[lastLine]) && !writeTonewConfigFile(&pSrc[lastLine], i - lastLine))
                                        return false;
                                if(!writeByPutParms("Changed")) return false;
                                goToNextDataLine();//Skip the line with matching name.
                        }
                        else
                        {
                                i = loci;// Put it back to the first line of the section
                                unknown = true;
                        }
                }
                if(unknown)
                {
                        goToNextSection();//Will set lastLine to end of last section.
                        i = lastLine;
                        goToNextDataLine();//Move i to start of next section.
                //write until start of next section or file end.
                        if(!writeTonewConfigFile(&pSrc[loci], i - loci)) return false;
                        if(!writeByPutParms("Parameter added")) return false;
                }
        }
// No section found, make a new one (don't know if I like this, but here goes)
        else
        {
                if(!writeTonewConfigFile(pSrc, iSize)) return false;
                if(!writeByPutParms("Section added")) return false;
                if(!writeTonewConfigFile("[", 1)) return false;
                if(!writeTonewConfigFile(theSection, strlen(theSection))) return false;//Length already checked.
                if(!writeTonewConfigFile("]\r\n", 3)) return false;
        }
        if(!writeNameValue(theName, theValue)) return false;
        if(i < iSize) if(!writeTonewConfigFile(&pSrc[i], iSize - i)) return false;
        return swapConfigFiles();
}

BOOL IniValue        ::       writeTonewConfigFile(const char* from, size_t amount)
{
        size_t bytesWriten;
//        if(from[amount - 1] == 0) amount--; //Some sizes are counting are counting the \0.
        bytesWriten = fwrite(from, 1, amount, tempFile);
        if(bytesWriten != amount)
        {
                snprintf(errorBufferPtr, sizeof(iniStringsPtr->errorBuffer), 
                         "Only wrote %ld bytes of %ld to temporary file %s",
                         bytesWriten, amount, iniStringsPtr->newConfigFile);
                errorCleanNewConfigFile();
                return false;
        }
        return true;
}
