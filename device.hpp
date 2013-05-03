#ifndef DEVICE_H_
#define DEVICE_H_
/*
20120703	bcb	Created this file
20130226	bcb 	Replaced gpps with IniValue class.  Removed all globals and routines now in IniValue class.
                        Version to 1.4.18a.
*/

#ifndef UNUSED_ARGUMENT
#define UNUSED_ARGUMENT(x) (void)x
#endif

#include "nkiqrsop.hpp"
#include "odbci.hpp"	// ODBC Interface routines

// prototypes
BOOL PanicKillOff(unsigned int MAGThreshHold);
BOOL
#ifndef UNIX
WINAPI
#endif
    MAGRampage();
UINT LargestFreeMAG();
UINT MaxMagDevice();
int MaxCACHEDevice(UINT NeededSpace);
#ifndef UNIX
UINT CalcMegsOnDevice(char *RootDirectory);
#else // UNIX 
UINT CalcMegsOnDevice(const char *theRootDirectory);
#endif
UINT CheckFreeStoreOnMAGDevice(UINT MagNumber);
UINT CheckFreeStoreOnMIRRORDevice(UINT MIRRORNumber);
UINT CheckFreeStoreOnCACHEDevice(UINT CACHENumber);
BOOL GetDevice(char *DeviceName, UINT DeviceFlags);
BOOL DirectoryExists( char	*path);
BOOL GetPhysicalDevice( const char *Device, char *Physical);
BOOL FindPhysicalDevice(char *Device, char *Physical, char *ObjectFile);
//BOOL RegisterUsedSpace( char	*Device, int	Kbytes);//Not yet
int GetKBUsedOnDevice (char *Device);
BOOL MakeSafeString (const char *in, char *string, Database *db);
int GetKBUsedForPatient (char *Device, char *PatientID, int ComputeSpace);
int GetKBUsedForSeries (char *Device, char *SeriesUID, int ComputeSpace);
void RecompressPatient (char *Device, char *PatientID, char *Compression, ExtendedPDU_Service *PDU);
void ProcessMoveDevicePatient (char *Device, char *PatientID);
void ProcessMoveDeviceSeries (char *Device, char *SeriesUID);
BOOL ModifyDeviceNameForPatient (char *Device, const char *PatientID, char *UpdateString);
BOOL ModifyDeviceNameForSeries (char *Device, const char *SeriesUID, char *UpdateString);
BOOL ModifyImageFile(char *filename, const char *script);
BOOL MovePatientData (char *Device, char *PatientID, char *From, char *To);
BOOL CopyPatientData (char *Device, char *PatientID, char *From, char *To);
BOOL CopySeriesData (char *Device, char *SeriesUID, char *From, char *To);
BOOL ComparePatientData (char *Device, char *PatientID, char *From, char *To);
BOOL CompareSeriesData (char *Device, char *SeriesUID, char *From, char *To);
BOOL TestFile(char *FullFilename, char *status);
BOOL TestPatientData (char *Device, char *PatientID, char *From);
BOOL TestSeriesData (char *Device, char *SeriesUID, char *From);
BOOL DeletePatientData (char *Device, char *PatientID, char *From, char *To);
BOOL DeleteSeriesData (char *Device, char *SeriesUID, char *From, char *To);
BOOL FindCacheDirectory(char *DeviceTo, char *PhysicalCache);
int MakeListOfPatientsOnDevice(char *Device, char **PatientIDList);
BOOL RestoreMAGFlags();
BOOL SelectLRUForArchival(char *Device, int KB, ExtendedPDU_Service *PDU);
int MakeListOfSeriesOnDevice(char *Device, char **SeriesList, int age, int kb);
BOOL SelectSeriesForArchival(char *Device, int age, int kb);
BOOL PrepareBunchForBurning(char *DeviceFrom, char *DeviceTo);
BOOL MoveDataToDevice(char *DeviceFrom, char *DeviceTo);
BOOL MoveSeriesToDevice(char *DeviceFrom, char *DeviceTo);
BOOL CompareBunchAfterBurning(char *DeviceTo);
BOOL DeleteBunchAfterBurning(char *DeviceTo);
BOOL VerifyMirrorDisk(char *DeviceFrom);
BOOL TestImages(char *DeviceFrom);
BOOL RenameDevice(char *DeviceFrom, char *DeviceTo);
int MakeListOfOldestPatientsOnDevice(char **PatientIDList, int Max, const char *Device, char *Sort);
#ifdef UNIX
//static BOOL FileExistsOld(char *Path);
static BOOL FileExists(char *path);
static int FileSize(char *Path);
static BOOL FileMove(char *oldname, char *newname);
static BOOL FileCopy(char *source, char *target, int smode);
static BOOL FileCompare(char *source, char *target, int smode);
static int MakeListOfLRUPatients(char *Device, int KB, char **PatientIDList, char *lLRUSort, ExtendedPDU_Service *PDU);
#else
//BOOL FileExistsOld(char *Path);
BOOL FileExists(char *path);
int FileSize(char *Path);
BOOL FileMove(char *oldname, char *newname);
BOOL FileCopy(char *source, char *target, int smode);
BOOL FileCompare(char *source, char *target, int smode);
int MakeListOfLRUPatients(char *Device, int KB, char **PatientIDList, char *lLRUSort, ExtendedPDU_Service *PDU);
#endif
#endif

