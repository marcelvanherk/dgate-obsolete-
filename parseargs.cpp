//
//  parseargs.cpp
//
/*
20120820 bcb Created.  Changed for ACRNemaMap class.
20130226 bcb Replaced gpps with IniValue class.  Removed all globals and routines now in IniValue class.
                Made ACRNemaMap class a singleton, fixed strlen warnings.  Version to 1.4.18a.

*/

#include "parseargs.hpp"
#include "parse.hpp"
#include "dgate.hpp"
#include "device.hpp"
#include "configpacs.hpp"
#include "luaint.hpp"

extern BOOL	RunServer;				// reset for regen etc
extern BOOL	NoThread;					// debug non-threaded
extern int UIDPostfix;
extern unsigned int gl_iFileCounter;

//Evil Globals
static char monitorfolder[MAX_PATH];

#ifndef UNIX
#include <conio.h>

#if !defined(__BORLANDC__) && !defined(__WATCOMC__)
#	include	<odbcinst.h>
#endif
#endif

#if defined(__BORLANDC__) || defined(__WATCOMC__)
typedef BOOL (__stdcall *_SQLConfigDataSource)(HWND       hwndParent,
                                               WORD       fRequest,
                                               LPCSTR     lpszDriver,
                                               LPCSTR     lpszAttributes);
_SQLConfigDataSource __SQLConfigDataSource;
#define SQLConfigDataSource __SQLConfigDataSource
#define  ODBC_ADD_DSN     1               // Add data source
#endif

#ifndef ODBC_ADD_SYS_DSN
#define  ODBC_ADD_SYS_DSN 4         // add a system DSN
#endif

// IPC Block is a block of data which is passed from parent to child with the socket#
#define IPCBlockMagic 0xafaced0a
typedef	struct	_IPCBlock
{
#ifdef	UNIX
        int			Magic;
#endif
        int			Socketfd;
}	IPCBlock;

// Main routine for parsing the command line; return FALSE when not running
// as server or a socket # when running as server thread
int
ParseArgs (int	argc, char	*argv[], ExtendedPDU_Service *PDU)
	{
	UINT		Valid;
	int		valid_argc, mode1=0;
	int		Socketfd = 0;
	BOOL		bUIDPostfixSet = FALSE;

	Valid = 2;

//	ConfigMicroPACS();
	IniValue *iniValuePtr = IniValue::GetInstance();
	UserLog.On(iniValuePtr->sscscpPtr->UserLogFile);
	UserLog.AddTimeStamps(TRUE);
	TroubleLog.On(iniValuePtr->sscscpPtr->TroubleLogFile);
	TroubleLog.AddTimeStamps(TRUE);

	valid_argc = 1;

	while ( valid_argc < argc )
		{
#ifndef UNIX	// Win32...
		if ((argv[valid_argc][0]=='-')||(argv[valid_argc][0]=='/'))
#else		// UNIX..
		if(argv[valid_argc][0]=='-')
#endif
			{
			switch ( argv[valid_argc][1] )
				{
				case	'h':	// wait e.g., to allow attaching debugger
				case	'H':
#ifndef UNIX
					getch();
#endif //!UNIX
					break;	//Done already.

				case	'w':	// set workdir for ini, map, dic
				case	'W':
					break;	//Done already.

				case	'v':	// verbose mode
				case	'V':
					SystemDebug.On();
					OperatorConsole.On();
					break;

				case	'b':	// debug mode
				case	'B':
					SystemDebug.On();
					OperatorConsole.On();
					DebugLevel = 4;
					NoThread=TRUE;
					break;

				case	'c':	// set UID counter
				case	'C':
					UIDPostfix = atoi(argv[valid_argc]+2);
					bUIDPostfixSet = TRUE;
					break;

				case	'u':
				case	'U':	// be verbose to specified pipe/udp
					if(argv[valid_argc][2] == PATHSEPCHAR)
						{
						OperatorConsole.OnMsgPipe(argv[valid_argc]+2);
						SystemDebug.OnMsgPipe(argv[valid_argc]+2);
						}
					else
						{
						OperatorConsole.OnUDP(iniValuePtr->sscscpPtr->OCPipeName, argv[valid_argc]+2);
						SystemDebug.OnUDP(iniValuePtr->sscscpPtr->OCPipeName, argv[valid_argc]+2);
						}
					break;


				case	'!':	// be verbose to pipe/udp specified in DICOM.INI (no debug)
					if(iniValuePtr->sscscpPtr->OCPipeName[0] == PATHSEPCHAR)
						{
						OperatorConsole.OnMsgPipe(iniValuePtr->sscscpPtr->OCPipeName);
						}
					else
						{
                                                if (argv[valid_argc][2]>='0' && argv[valid_argc][2]<='9')
						{
							OperatorConsole.OnUDP(iniValuePtr->sscscpPtr->OCPipeName, argv[valid_argc]+2);
#if 0
							SystemDebug.OnUDP(iniValuePtr->sscscpPtr->OCPipeName, argv[valid_argc]+2);
#endif
						}
						else
							OperatorConsole.OnUDP(iniValuePtr->sscscpPtr->OCPipeName, "1111");
						}
					break;

				case	'^':	// be verbose to passed file with timestamps (no debug)
					OperatorConsole.On(argv[valid_argc]+2);
					OperatorConsole.AddTimeStamps(1);
					StartZipThread();
					break;

				case	'#':	// be verbose to passed file with timestamps (with debug)
					OperatorConsole.On(argv[valid_argc]+2);
					OperatorConsole.AddTimeStamps(1);
					SystemDebug.On(argv[valid_argc]+2);
					SystemDebug.AddTimeStamps(1);
					StartZipThread();
					break;

				case	'p':	// override dicom.ini port#
				case	'P':
					strcpylim(iniValuePtr->sscscpPtr->Port, argv[valid_argc]+2 ,sizeof(iniValuePtr->sscscpPtr->Port));
					++Valid;
					break;
					// fall through was not intended !

				case	'q':	// override dicom.ini ServerCommandAddress (for dgate -- options)
				case	'Q':
					strcpy((char*)ServerCommandAddress, argv[valid_argc]+2);
					++Valid;
					break;
					// fall through was not intended !

				case	'r':	// init and regenerate database
				case	'R':
					RunServer = FALSE;
					NeedPack = 2;
					SystemDebug.printf("Regen Database\n");

					if(!LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile))
						{
						OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
						exit(1);
						}

					OperatorConsole.printf("Step 1: Re-intialize SQL Tables\n");

					mode1 = atoi(argv[valid_argc]+2);
					InitializeTables (mode1);

					OperatorConsole.printf("Step 2: Load / Add DICOM Object files\n");

					Regen();

					OperatorConsole.printf("Regeneration Complete\n");

					return ( FALSE );

				case	'd':	// List magnetic device status
				case	'D':
					RunServer = FALSE;
					NeedPack = FALSE;

					SystemDebug.Off();
					PrintFreeSpace();
					return ( FALSE );

				case	'm':	// list map of outgoing AE's
				case	'M':
					RunServer = FALSE;
					NeedPack = FALSE;

					char *aMap;
					if(iniValuePtr->sscscpPtr->ACRNemaMap[0] == 0)
						{
						OperatorConsole.printf("***Could not get acr-nema map file path\n");
						exit(1);
						}

					if(!(ACRNemaMap::GetInstance()->InitACRNemaAddressArray(iniValuePtr->sscscpPtr->ACRNemaMap)))
						{
						OperatorConsole.printf("***Error loading acr-nema map file:%s\n",
						iniValuePtr->sscscpPtr->ACRNemaMap);
						exit(1);
						}                                                
					aMap = ACRNemaMap::GetInstance()->PrintAMapToString();
					if(aMap != NULL)
						{
						OperatorConsole.printf(aMap);
						free(aMap);
						}
						return ( FALSE );


				case	'k':	// list K factor file (DICOM.SQL)
				case	'K':
					RunServer = FALSE;
					NeedPack = FALSE;

					if(!LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile))
						{
						OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
						exit (1);
						}

					PrintKFactorFile();
					return ( FALSE );


				case	't':	// test (TCP/IP) console connnection
				case	'T':
					{ 
					char lt[] = "This is a very long text output for testing -- ";
					RunServer = FALSE;
					NeedPack = FALSE;
					OperatorConsole.printf("This output is generated by the dicom server application\n");
					OperatorConsole.printf("If you can read this, the console communication is OK\n");
                          		SystemDebug.printf("This is systemdebug output; can you read this ?\n");
                          		SystemDebug.printf("%s%s%s%s%s%s\n", lt, lt, lt, lt, lt, lt);
					OperatorConsole.printf(" ---------- Succesful end of test -----------\n");
					return ( FALSE );
					}

				case	'o':	// test ODBC database connnection
				case	'O':
					{
					Database aDB;
					int	i;
					RunServer = FALSE;
					NeedPack = FALSE;
					for (i=1; i<=10; i++)
						{
						OperatorConsole.printf("Attempting to open database; test #%d of 10\n", i);
						if(!aDB.Open(iniValuePtr->sscscpPtr->DataSource, iniValuePtr->sscscpPtr->UserName,
							iniValuePtr->sscscpPtr->Password, iniValuePtr->sscscpPtr->DataHost))
							{
							OperatorConsole.printf("***Unable to open database %s as user %s on %s\n",
							iniValuePtr->sscscpPtr->DataSource, iniValuePtr->sscscpPtr->UserName, iniValuePtr->sscscpPtr->DataHost);
							exit (1);
							}
	
						OperatorConsole.printf("Creating test table\n");
						aDB.CreateTable ( "xtesttable", "Name varchar(254), AccessTime  int" );

						OperatorConsole.printf("Adding a record\n");
						aDB.AddRecord("xtesttable", "Name", "'Testvalue'");

						OperatorConsole.printf("Dropping test table\n");
						aDB.DeleteTable ( "xtesttable" );

						OperatorConsole.printf("Closing database\n");
						aDB.Close();
						}

					OperatorConsole.printf(" ---------- Succesful end of test -----------\n");
					return ( FALSE );
					}

				case	'a':	// Archival code
				case	'A':	// Archival code
					{
					RunServer = FALSE;
					NeedPack = 5;
					OperatorConsole.printf(" ---------- Start archival operation  -----------\n");

					if(!LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile))
						{
						OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
						exit(1);
						}

					// Select a number of KB to archive from device MAGn (default MAG0)
					if (argv[valid_argc][2] == 's' || argv[valid_argc][2] == 'S')
						{
						char MAG[256];
						char *p;
						p = strchr(argv[valid_argc]+3,',');
						if (p) 
							sprintf(MAG, "MAG%d", atoi(p+1));
						else 
							strcpy(MAG, "MAG?");
						if (!SelectLRUForArchival(MAG, atoi(argv[valid_argc]+3), PDU))
							exit(1);
						}

					// Create cache set for burning for JUKEBOXn.n from MAGn (default MAG0)
					else if (argv[valid_argc][2] == 'b' || argv[valid_argc][2] == 'B')
						{
						char MAG[256], Device[256];
						char *p;
						strcpy(Device, argv[valid_argc]+3);
						p = strchr(Device,',');
						if (p) 
							{
							sprintf(MAG, "MAG%d", atoi(p+1));
							*p = 0;
							}
						else 
							strcpy(MAG, "MAG?");
						if (!PrepareBunchForBurning(MAG, Device))
							exit(1);
						}

					// Move (all) data from device1 to device2
					else if (argv[valid_argc][2] == 'm' || argv[valid_argc][2] == 'M')
						{
						char Device1[256];
						char *p;
						strcpy(Device1, argv[valid_argc]+3);
						p = strchr(Device1,',');
						if (p) 
							{
							*p = 0;
							if (!MoveDataToDevice(Device1, p+1)) exit(1);
							}
						else
							exit(1);
						}

					// Undo any archiving that was not completed
					else if (argv[valid_argc][2] == 'u' || argv[valid_argc][2] == 'U')
						{
						if (!RestoreMAGFlags())
							exit(1);
						}

					// Verify CD in jukebox against its cache set
					else if (argv[valid_argc][2] == 'c' || argv[valid_argc][2] == 'C')
						{
						if (!CompareBunchAfterBurning(argv[valid_argc]+3))
							exit(1);
						}

					// Verify MAG device against its mirror device
					else if (argv[valid_argc][2] == 'v' || argv[valid_argc][2] == 'V')
						{
						if (!VerifyMirrorDisk(argv[valid_argc]+3))
							exit(1);
						}

					// Test read slices on device
					else if (argv[valid_argc][2] == 't' || argv[valid_argc][2] == 'T')
						{
						if (!TestImages(argv[valid_argc]+3))
							exit(1);
						}

					// Delete cache set for burned CD
					else if (argv[valid_argc][2] == 'd' || argv[valid_argc][2] == 'D')
						{
						if (!DeleteBunchAfterBurning(argv[valid_argc]+3))
							exit(1);
						}

					// Regen a single device (for database maintenance)
					else if (argv[valid_argc][2] == 'r' || argv[valid_argc][2] == 'R')
						{
						if(!LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile))
							{
							OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
							exit(1);
							}
						OperatorConsole.printf("Regen single device: %s\n", argv[valid_argc]+3);
						if (!Regen(argv[valid_argc]+3, FALSE)) exit(1);
						OperatorConsole.printf("Regeneration Complete\n");
						}
						
					// rEname device name
					else if (argv[valid_argc][2] == 'e' || argv[valid_argc][2] == 'E')
						{
						char DeviceFrom[256];
						char *p;
						strcpy(DeviceFrom, argv[valid_argc]+3);
						p = strchr(DeviceFrom,',');
						if (p)
							{ 
							*p = 0;
							if (!RenameDevice(DeviceFrom, p+1)) exit(1);
							}
						else
							{
							OperatorConsole.printf("option requires two device names\n");
							exit(1);
							}
						}

					else
						{
						OperatorConsole.printf("Unknown archiving option\n");
						exit(1);
						}

					OperatorConsole.printf(" ---------- End of archival operation -----------\n");
					return ( FALSE );
					}


#ifdef WIN32
				case	's':
				case	'S':	// Create ODBC data source
					{
					char Options[1512], Driver[512];
					int i, len;
#if defined(__BORLANDC__) || defined(__WATCOMC__)
					HINSTANCE hODBCInst = LoadLibrary("odbccp32.dll");
					__SQLConfigDataSource= (_SQLConfigDataSource)GetProcAddress(hODBCInst, "SQLConfigDataSource");
#endif
					strcpy(Driver, "Microsoft dBase Driver (*.dbf)");

					strcpy(Options, 
						"DSN=ConQuestPacs;"
                   				"Description=ConQuest DICOM server data source;"
						"Fil=dBase III;"
						"DriverID=21;"
						"Deleted=1;"
                   				"DefaultDir=C:\\quirt");

					for (i=2; i<strlen(argv[valid_argc]); i++)
						{
						if (argv[valid_argc][i] == ';')
							{
							memcpy(Driver, argv[valid_argc]+2, i-2);
							Driver[i-2]=0;
							strcpy(Options, argv[valid_argc]+i+1);
							Options[strlen(Options)+1] = 0;
							break;
							}
						}

					OperatorConsole.printf("Creating data source\n");
					OperatorConsole.printf("Driver = %s\n", Driver);
					OperatorConsole.printf("Options = %s\n", Options);

					len = strlen(Options);
					for (i=0; i<len; i++)
						if (Options[i] == ';') Options[i] = 0;

					RunServer = FALSE;
					NeedPack = FALSE;
					if ( !SQLConfigDataSource(NULL, ODBC_ADD_SYS_DSN, Driver, Options) )
					// if ( !SQLConfigDataSource(NULL, ODBC_ADD_DSN, Driver, Options) )
						{ char		szMsg[256] = "(No details)";
						  char		szTmp[280];
						  DWORD		dwErrorCode;
						  OperatorConsole.printf("***Datasource configuration FAILED\n");
#if !defined(__BORLANDC__) && !defined(__WATCOMC__) && 0
						  SQLInstallerError(1, &dwErrorCode, szMsg, sizeof(szMsg) - 1, NULL);
						  sprintf(szTmp, "***%s (ec=%d)\n", szMsg, dwErrorCode);
						  OperatorConsole.printf(szTmp);
#endif
						  exit(1);
						}
					else
						OperatorConsole.printf("Datasource configuration succesful\n");
					OperatorConsole.printf(" ----------------------------------\n");
					return ( FALSE );
					}
#endif


				case	'e':	// create database (-eSAPW DB USER PW)
				case	'E':
					{
					Database DB;
					DebugLevel=4;
					if (DB.db_type==DT_POSTGRES)
						{
						if (DB.Open ( "postgres", "postgres", argv[valid_argc]+2, iniValuePtr->sscscpPtr->DataHost))
							if (DB.CreateDatabase(argv[valid_argc+1], argv[valid_argc+2], argv[valid_argc+3]))
								OperatorConsole.printf("succesfully created database and login\n");
						}
					else if (strcmp(argv[valid_argc+2], "root")==0 || strcmp(argv[valid_argc+2], "sa")==0)
						{
						if (DB.Open ( iniValuePtr->sscscpPtr->DataSource, argv[valid_argc+2], argv[valid_argc]+2,
							iniValuePtr->sscscpPtr->DataHost))
							if (DB.CreateDatabase(argv[valid_argc+1], argv[valid_argc+2], argv[valid_argc]+3))
								OperatorConsole.printf("succesfully created database\n");
						}
					else
						{
						if (DB.Open ( iniValuePtr->sscscpPtr->DataSource, "sa", argv[valid_argc]+2,
							iniValuePtr->sscscpPtr->DataHost))
							if (DB.CreateDatabase(argv[valid_argc+1], argv[valid_argc+2], argv[valid_argc+3]))
								OperatorConsole.printf("succesfully created database and login\n");
						}
					OperatorConsole.printf(" ----------------------------------\n");
					exit(0);
					}

				case	'i':	// initialize database
				case	'I':
					RunServer = FALSE;
					NeedPack = FALSE;

					OperatorConsole.printf("Initializing Tables\n");

					if(!LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile))
						{
						OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
						exit(1);
						}

					mode1 = atoi(argv[valid_argc]+2);
					InitializeTables (mode1);

					return ( FALSE );

				case	'z':	// delete (zap) patient
				case	'Z':
					RunServer = FALSE;
					NeedPack = 5;

					if(!LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile))
						{
						OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
						exit (1);
						}

					if (!DeletePatient(argv[valid_argc]+2, FALSE))
						exit(1);

					return ( FALSE );

				case	'g':	// grab data from server
				case	'G':
					{
					char *p;

					RunServer = FALSE;
					NeedPack = 5;

					if(iniValuePtr->sscscpPtr->ACRNemaMap[0] == 0)
						{
						OperatorConsole.printf("***Could not get acr-nema map file path\n");
						exit(1);
						}
					if(!(ACRNemaMap::GetInstance()->InitACRNemaAddressArray(iniValuePtr->sscscpPtr->ACRNemaMap)))
						{
						OperatorConsole.printf("***Error loading acr-nema map file:%s\n", iniValuePtr->sscscpPtr->ACRNemaMap);
						exit(1);
						}

					if(!LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile))
						{
						OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
						exit (1);
						}

					p = strchr(argv[valid_argc]+2,',');
					if (p)
						{
						*p=0;
						if (!GrabImagesFromServer((unsigned char *)argv[valid_argc]+2, p+1, iniValuePtr->sscscpPtr->MyACRNema))
							exit(1);
						}
					else
						{
						if (!GrabImagesFromServer((unsigned char *)argv[valid_argc]+2, "*", iniValuePtr->sscscpPtr->MyACRNema))
							exit(1);
						}

					return ( FALSE );
					}

				case	'f':	// Database fix options
				case	'F':
					RunServer = FALSE;
					NeedPack = 5;

					if(!LoadKFactorFile((char*)iniValuePtr->sscscpPtr->kFactorFile))
						{
						OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
						exit (1);
						}

					
					if (argv[valid_argc][2] == 'p' || argv[valid_argc][2] == 'P')
						{
						if (!DeletePatient(argv[valid_argc]+3, TRUE))
							exit(1);
						}

					if (argv[valid_argc][2] == 'f' || argv[valid_argc][2] == 'F')
						{
						if (LargestFreeMAG()>(unsigned int)atoi(argv[valid_argc]+3)) exit(0);

						if (!PanicKillOff((unsigned int)atoi(argv[valid_argc]+3)))
							exit(1);
						}

					else if (argv[valid_argc][2] == 't' || argv[valid_argc][2] == 'T')
						{
						if (!DeleteStudy(argv[valid_argc]+3, TRUE))
							exit(1);
						}

					else if (argv[valid_argc][2] == 's' || argv[valid_argc][2] == 'S')
						{
						if (!DeleteSeries(argv[valid_argc]+3, TRUE))
							exit(1);
						}

					else if (argv[valid_argc][2] == 'i' || argv[valid_argc][2] == 'I')
						{
						if (!DeleteImage(argv[valid_argc]+3, TRUE))
							exit(1);
						}

					else if (argv[valid_argc][2] == 'r' || argv[valid_argc][2] == 'R')
						{
						char par[256];
						char *p;
						strcpy(par, argv[valid_argc]+3);
						p = strchr(par,',');
						if (p)
							{
							*p=0;
							if (!Regen(par, FALSE, p+1))
								exit(1);
							}
						else
							if (!Regen("MAG0", FALSE, par))
								exit(1);
						}

					else if (argv[valid_argc][2] == 'e' || argv[valid_argc][2] == 'E')
						{
						if (!RegenFile(argv[valid_argc]+3))
							exit(1);
						}

					else if (argv[valid_argc][2] == 'd' || argv[valid_argc][2] == 'D')
						{
						if (!DeleteImageFile(argv[valid_argc]+3, TRUE))
							exit(1);
						}

					else if (argv[valid_argc][2] == 'z' || argv[valid_argc][2] == 'Z')
						{
						if (!DeleteImageFile(argv[valid_argc]+3, FALSE))
							exit(1);
						}

					else if (argv[valid_argc][2] == 'a' || argv[valid_argc][2] == 'A')
						{
						char par[1024];
						char *p;

						strcpy(par, argv[valid_argc]+3);
						p = strchr(par,',');
						if (bUIDPostfixSet)
							gl_iFileCounter = UIDPostfix;
						else
							gl_iFileCounter = ComputeCRC(par, strnlenint(par, sizeof(par)));
						if (p)
							{
							*p=0;
							if (!AddImageFile(par, p+1, PDU))
								exit(1);
							}
						else
							{
							if (!AddImageFile(par, NULL, PDU))
								exit(1);
							}
						}

					else if (argv[valid_argc][2] == '?')
						{
						char UID[256];
						if (!GetImageFileUID(argv[valid_argc]+3, UID))
							exit(1);
						printf("%s\n", UID);
						}

					else if (argv[valid_argc][2] == 'u' || argv[valid_argc][2] == 'U')
						{
						char UID[256];
						if (!GenUID(UID))
							exit(1);
						printf("%s\n", UID);
						}

					else if (argv[valid_argc][2] == 'c' || argv[valid_argc][2] == 'C')
						{
						char par[256];
						char *p;
						strcpy(par, argv[valid_argc]+3);
						p = strchr(par,',');
						if (p)
							{
							*p=0;
							if (!ModifyPATIDofImageFile(p+1, par, TRUE, NULL, PDU))
								exit(1);
							}
						}
					else if (argv[valid_argc][2] == 'k' || argv[valid_argc][2] == 'K')
						{
						char par[256];
						char *p;
						strcpy(par, argv[valid_argc]+3);
						p = strchr(par,',');
						if (p)
							{
							*p=0;
							if (!ModifyPATIDofImageFile(p+1, par, FALSE, NULL, PDU))
								exit(1);
							}
						}
					return ( FALSE );



				case	'n':	// NKI compression options
				case	'N':
					RunServer = FALSE;
					NeedPack = FALSE;

					if (argv[valid_argc][2] == 'd' || argv[valid_argc][2] == 'D')
						{
						//int dum;
						//if (!DecompressImageFile(argv[valid_argc]+3, &dum))
						char dum[3]="un";
						if (!recompressFile(argv[valid_argc]+3, dum, PDU))
							exit(1);
						}

					if (argv[valid_argc][2] == 'c' || argv[valid_argc][2] == 'C')
						{
						//int dum;
						//if (!CompressNKIImageFile(argv[valid_argc]+4, argv[valid_argc][3]-'0', &dum))
						if (!recompressFile(argv[valid_argc]+4, argv[valid_argc]+2, PDU))
							exit(1);
						}

					return ( FALSE );

				case	'j':	// JPEG compression options
				case	'J':
					RunServer = FALSE;
					NeedPack = FALSE;

					if (argv[valid_argc][2] == 'd' || argv[valid_argc][2] == 'D')
						{
						//int dum;
						//if (!DecompressImageFile(argv[valid_argc]+3, &dum))
						char dum[3]="un";
						if (!recompressFile(argv[valid_argc]+3, dum, PDU))
							exit(1);
						}

					if (argv[valid_argc][2] == 'c' || argv[valid_argc][2] == 'C')
						{
						//int dum;
						//if (!CompressJPEGImageFile(argv[valid_argc]+4, argv[valid_argc][3], &dum))
						if (!recompressFile(argv[valid_argc]+4, argv[valid_argc]+2, PDU))
							exit(1);
						}

					if (argv[valid_argc][2] == '*')		// test recompressFile
						{
						char mode[4];
						mode[0] = argv[valid_argc][3];
						mode[1] = argv[valid_argc][4];
						mode[2] = 0;
						if (!recompressFile(argv[valid_argc]+5, mode, PDU))
							exit(1);
						}

					if (argv[valid_argc][2] == '-')		// test recompress
						{
						PDU_Service	PDU2;
//						VR *pVR;

						PDU2.AttachRTC(&VRType);

						DICOMDataObject *pDDO;
						pDDO = PDU2.LoadDICOMDataObject(argv[valid_argc]+5);
						char mode[4];
						mode[0] = argv[valid_argc][3];
						mode[1] = argv[valid_argc][4];
						mode[2] = 0;
						if (!recompress(&pDDO, mode, "", mode[0]=='n' || mode[0]=='N', PDU))
							exit(1);

						PDU2.SaveDICOMDataObject(argv[valid_argc]+5, ACRNEMA_VR_DUMP, pDDO);
						delete pDDO;
						}

					return ( FALSE );


				case	'-':	// Server command
					if (memcmp(argv[valid_argc]+2, "addlocalfile:", 13)==0)
						{
						char *p1 = strchr(argv[valid_argc]+2, ',');
						if (p1) *p1=0;
						char com[512];
						if (p1) sprintf(com, "addimagefile:%s,%s", argv[valid_argc]+15, p1+1);
						else    sprintf(com, "addimagefile:%s", argv[valid_argc]+15);
						printf("%s", com);
						printf("%s", argv[valid_argc]+15);
						SendServerCommand("Server command sent using DGATE -- option", com, 0, argv[valid_argc]+15, FALSE, TRUE);
						exit(1);
						}
					if (memcmp(argv[valid_argc]+2, "dolua:", 6)==0)
						{
					        OperatorConsole.On();
						//SystemDebug.On();
						if(!LoadKFactorFile((char*)iniValuePtr->sscscpPtr->kFactorFile))
					        {
					        OperatorConsole.printf("***Could not load kfactor file: %s\n", iniValuePtr->sscscpPtr->kFactorFile);
					        exit (1);
					        }
						if(!(ACRNemaMap::GetInstance()->InitACRNemaAddressArray(iniValuePtr->sscscpPtr->ACRNemaMap)))
					        {
					        OperatorConsole.printf("***Error loading acr-nema map file:%s\n", iniValuePtr->sscscpPtr->ACRNemaMap);
					        exit(1);
					        }                                                
						struct scriptdata sd = {&globalPDU, NULL, NULL, -1, NULL, NULL, NULL, NULL, NULL, 0, NULL};
						globalPDU.SetLocalAddress ( (BYTE *)"global" );
						globalPDU.SetRemoteAddress ( (BYTE *)"dolua" );
						globalPDU.ThreadNum = 0;
						do_lua(&(globalPDU.L), argv[valid_argc]+8, &sd);
						if (sd.DDO) delete sd.DDO;
						exit(1);
						}

					SendServerCommand("Server command sent using DGATE -- option", argv[valid_argc]+2, 0, NULL, FALSE);
					exit(1);

				default:   	// provide some help
					RunServer = FALSE;
					NeedPack = FALSE;
					PrintOptions();
					return (FALSE);
				}
			}
		else
			{
//			FILE	*logFile;

// This is a simple way to get a socket#; but why make it simple if it can be done complex ?
//			Socketfd = (void*)atoi(argv[valid_argc]);


// Open some channel (UDP/pipe) for statistics when running as server child (from .ini file)
			if(iniValuePtr->sscscpPtr->OCPipeName[0] == PATHSEPCHAR)
				{
				OperatorConsole.OnMsgPipe(iniValuePtr->sscscpPtr->OCPipeName);
				}
			else
				{
				OperatorConsole.OnUDP(iniValuePtr->sscscpPtr->OCPipeName, "1111");
				}

// WIN32: get the socket# through shared memory
#ifndef UNIX
			HANDLE		hMap;
			IPCBlock	*IPCBlockPtrInstance;
			hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,
				FALSE,
				argv[valid_argc]);
			if(!hMap)
				{
				OperatorConsole.printf("***Child: Unable to OpenFileMapping : %d (%s)\n",
					GetLastError(), argv[valid_argc]);
				exit(0);
				}

			IPCBlockPtrInstance = (IPCBlock*)MapViewOfFile(hMap,
				FILE_MAP_ALL_ACCESS, 0,0, sizeof(IPCBlock));
			if(!IPCBlockPtrInstance)
				{
				OperatorConsole.printf("***Child: Unable to MapViewOfFile : %d\n",
					GetLastError());
				}
			Socketfd = IPCBlockPtrInstance->Socketfd;
			UnmapViewOfFile(IPCBlockPtrInstance);
			CloseHandle(hMap);

// UNIX: get the socket# from a small file or pipe
#else
#if 0
			IPCBlock aIPCBlock;
			ifstream aStream(argv[valid_argc], ios::in | ios::nocreate);
			if(aStream.good())
				{
				aStream.read((unsigned char *)&aIPCBlock, sizeof(aIPCBlock));
				}
			if(!aStream.good() || aStream.gcount() != sizeof(aIPCBlock) ||
				aIPCBlock.Magic != IPCBlockMagic)
				{
				// magic # assures safety --
				// we don't try unlink("/"), for example
				OperatorConsole.printf("***Child: Unable to read file %s\n",
					argv[valid_argc]);
				}
			else
				{
					unlink(argv[valid_argc]);
				}
			Socketfd = aIPCBlock.Socketfd;
#endif
#endif
			}

		++valid_argc;
		}

        /* check access for logging and saving images */

	if (1)
		{
	        FILE *f;
	        char Filename[1024];
		int i;

	        f = fopen(iniValuePtr->sscscpPtr->TroubleLogFile, "at");
        	if (f) fclose(f);
	        if (!f) OperatorConsole.printf("*** Not enough rights to write to logfiles\n");

		memset(Filename, 0, 1024);
		GetPhysicalDevice("MAG0", Filename);
		strcat(Filename, "printer_files");
		mkdir(Filename);
		i = strnlenint(Filename, sizeof(Filename));
		Filename[i]   = PATHSEPCHAR;
		Filename[i+1] = 0;
		strcat(Filename, "accesstest.log");
	        f = fopen(Filename, "at");
        	if (f) fclose(f);
	        if (!f) OperatorConsole.printf("*** Not enough rights to write in MAG0\n");

		memset(Filename, 0, 1024);
		GetPhysicalDevice("MAG0", Filename);
		strcat(Filename, "incoming");
		if (dgate_IsDirectory(Filename))
			{
		        strcat(Filename, "\\");
			Filename[strlen(Filename)-1] = PATHSEPCHAR;
			OperatorConsole.printf("Monitoring for files in: %s\n", Filename);
			strcpy(monitorfolder, Filename);//bcb? Why a global?
			StartMonitorThread(monitorfolder);//bcb? Why not StartMonitorThread(Filename); make monitorfolder ini setable?
			}
		}

// prepare to run as a child server thread
	if(!LoadKFactorFile(iniValuePtr->sscscpPtr->kFactorFile))
		{
		OperatorConsole.printf("***Error loading kfactor file:%s\n", iniValuePtr->sscscpPtr->kFactorFile);
		return ( FALSE );
		}

	if(iniValuePtr->sscscpPtr->ACRNemaMap[0] == 0)
        {
        OperatorConsole.printf("***Could not get acr-nema map file path\n");
        return ( FALSE );
        }
	if(!ACRNemaMap::GetInstance()->InitACRNemaAddressArray(iniValuePtr->sscscpPtr->ACRNemaMap))
        {
        OperatorConsole.printf("***Error loading acr-nema map file:%s\n",
          iniValuePtr->sscscpPtr->ACRNemaMap);
        return ( FALSE );
        }                                                

	if(!atoi(iniValuePtr->sscscpPtr->Port))
		return ( Socketfd );
	if(Valid > 1)				// always TRUE !!!!!
		return ( Socketfd );

	return ( Socketfd );
	}
