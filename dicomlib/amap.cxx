/* mvh  19990827: added (limited) wildcard mapping in GetACRNema: AE, port or IP may end on '*' 
   mvh  20001105: replaced m-alloc by new
   mvh  20011109: Made AE mapping case insensitive
   ljz  20020524: In function 'GetACRNema', first try case SENSITIVE then INSENSITIVE
   mvh  20030701: Added compression column (with this code is this column is not optional!)
   mvh  20030703: KNOWN BUG: number of columns may not be 3 (crashes)
   ljz  20030709: Solved above problem; rewritten parsing of Acrnema.map
   ljz  20030711: Fixed trailing spaces in acrnema.map
   ljz  20031118: Fixed leak InitACRNemaAddressArray
   bcb  20101227: removed WHEDGE and master.h
   bcb  20120820: Global ACRNemaAddressArray changed to a pointer and created ACRNemaMap class.
   bcb  20130214: Fixed AppendMapToOpenFile size warning.
   bcb  20130226: Made the ACRNemaMap class a singleton.  Version to 1.4.18a.
   bcb  20130311: changed ftell to stat, ftell will give diffent values depending on opening mode!
   bcb  20130311: Added GetACRNemaNoWild.
   bcb  20130402: Used new file.xpp for file opening.
*/
   
#include	"amap.hpp"
#include	"file.hpp"
#include	"util.h"

#include	<stdio.h>
#include	<time.h>
#include    <sys/stat.h>
#include    <fcntl.h>
#ifndef UNIX
#include	<direct.h>
#include	<io.h>
#ifndef snprintf
#define snprintf _snprintf
#endif
#ifndef off_t
#define off_t __int64
#endif
#else //UNIX
#include    <unistd.h>
#endif

#define ACRNEMAMAP_MAX_SIZE 80000 //Curently 4K file size
#ifndef ctime_r
#define ctime_r(a, b) ctime(a)
#endif

// Use anonymous namespace to force internal linkage for instance.
/*namespace
{
        ACRNemaMap* instanceAPtr = 0;//Address of the singleton
}*/


/*************************************************************************
 *
 * ACRNemaMap Class
 *
 ************************************************************************/
ACRNemaMap      ::      ACRNemaMap()
{
// Create the default format.
        defaultFormat = (char *) malloc(24 + MAX_PATH);
        strcpy (defaultFormat, "%-17s %-30s %-10s %-16s");
// Create the array,
        ACRNemaAddressArrayPtr = new Array< ACRNemaAddress * >;
// Start of the file name.
        ACRNEMAMAP = defaultFormat + 24;//One allocate, two strings
}

ACRNemaMap :: ~ACRNemaMap()
{
        if(defaultFormat != NULL) free(defaultFormat);
        ClearMap();
        if(ACRNemaAddressArrayPtr != NULL)free(ACRNemaAddressArrayPtr);
}

BOOL ACRNemaMap :: AppendMapToOpenFile(FILE *f, char *format)
{
        char *wholeMap;
        size_t wholeMapLen;
        BOOL retrn = FALSE;
        
        if((wholeMap = GetWholeAMapToString(format)) == NULL) return FALSE;
        wholeMapLen = strnlen(wholeMap, ACRNemaAddressArrayPtr->GetSize() * 75);//sizeof(ACRNemaAddress));
        if(fwrite(wholeMap, wholeMapLen, 1, f) == wholeMapLen )retrn = TRUE;
        free(wholeMap);
        return FALSE;
}

void ACRNemaMap :: ClearMap()
{
        while ( ACRNemaAddressArrayPtr->GetSize() )
        {
                delete ACRNemaAddressArrayPtr->Get(0);
                ACRNemaAddressArrayPtr->RemoveAt(0);
        }
        
}

BOOL ACRNemaMap :: DeleteEntry(unsigned int theEntry)
{
        ACRNemaAddress *AAPtr;
        if (theEntry < ACRNemaAddressArrayPtr->GetSize() )
        {
                AAPtr = ACRNemaAddressArrayPtr->Get(theEntry);
                delete AAPtr;
                ACRNemaAddressArrayPtr->RemoveAt(theEntry);
                return TRUE;
        }
        return FALSE;
}

BOOL ACRNemaMap :: GetACRNemaNoWild(char* ACRNema, char* ip, char* port, char* compress)
{
        ip[0] = 0;
        return GetACRNema(ACRNema, ip, port, compress);
}

BOOL ACRNemaMap :: GetACRNema(char* ACRNema, char* ip, char* port, char* compress)
{
        UINT	Index;
        char	s[ACRNEMA_AE_LEN];
        char	*t;
        
        
        //        memset(s, 0, 20); //bcb strcpylim always terminates the string!
        
        strcpylim(s, ACRNema, ACRNEMA_AE_LEN);
        if(!strnlen(s, ACRNEMA_AE_LEN))
                return ( FALSE );
        // trim the ACRNema address
        while(iswhitespacea(s[strnlen(s, ACRNEMA_AE_LEN)-1]))
        {
                s[strnlen(s, ACRNEMA_AE_LEN)-1] = '\0';
                if(!strlen(s))
                        return ( FALSE );
        }
        
        Index = 0;
        while ( Index < ACRNemaAddressArrayPtr->GetSize())
        {
                t = ACRNemaAddressArrayPtr->Get(Index)->Name;
                
                // try wildcard mapping, e.g., s=NG12, t=NG*, IP=101.11.11.*, Port=56*
                if (t[strnlen(t, ACRNEMA_AE_LEN)-1] == '*')
                {
                        if (!memicmp(t, s, strnlen(t, 20)-1))
                        {
                                strcpylim(ip, ACRNemaAddressArrayPtr->Get(Index)->IP, ACRNEMA_IP_LEN);
                                if (ip[strnlen(ip, ACRNEMA_IP_LEN)-1]=='*')
                                        strcpylim(ip+strnlen(ip, ACRNEMA_IP_LEN)-1, s+strnlen(t, ACRNEMA_AE_LEN)-1,
                                                  ACRNEMA_IP_LEN + 1 - strnlen(ip, ACRNEMA_IP_LEN) );
                                
                                strcpylim(port, ACRNemaAddressArrayPtr->Get(Index)->Port, ACRNEMA_PORT_LEN);
                                if (port[strnlen(port, ACRNEMA_PORT_LEN)-1]=='*')
                                        strcpylim(port+strnlen(port, ACRNEMA_PORT_LEN)-1, s+strnlen(t, ACRNEMA_AE_LEN)-1,
                                                  ACRNEMA_PORT_LEN + 1 - strnlen(port, ACRNEMA_PORT_LEN));
                                strcpylim(compress, ACRNemaAddressArrayPtr->Get(Index)->Compress, ACRNEMA_COMP_LEN);
                                return ( TRUE );
                        }
                }
                
                // try exact match (case sensitive)
                if (!strcmp(t, s))
                {
                        wildIPCheck(ip, ACRNemaAddressArrayPtr->Get(Index)->IP,
                                    ACRNemaAddressArrayPtr->Get(Index)->LastGoodIP);
                        strcpylim(port, ACRNemaAddressArrayPtr->Get(Index)->Port, ACRNEMA_PORT_LEN);
                        strcpylim(compress, ACRNemaAddressArrayPtr->Get(Index)->Compress,16 );
                        return ( TRUE );
                }
                ++Index;
        }
        
        Index = 0;
        while ( Index < ACRNemaAddressArrayPtr->GetSize())
        {
                t = ACRNemaAddressArrayPtr->Get(Index)->Name;
                
                // try exact match (case insensitive)
                if (!stricmp(t, s))
                {
                        wildIPCheck(ip, ACRNemaAddressArrayPtr->Get(Index)->IP,
                                    ACRNemaAddressArrayPtr->Get(Index)->LastGoodIP);
                        strcpylim(port, ACRNemaAddressArrayPtr->Get(Index)->Port, ACRNEMA_PORT_LEN);
                        strcpylim(compress, ACRNemaAddressArrayPtr->Get(Index)->Compress, ACRNEMA_COMP_LEN);
                        return ( TRUE );
                }
                ++Index;
        }
        
        return ( FALSE );
}

const char* ACRNemaMap :: GetFileName()
{
        return ACRNEMAMAP;
}

//This is not truely multi thread safe, but will be ok for us( not double locked).
/*ACRNemaMap* ACRNemaMap        ::       GetInstance()
{
        //"Lazy" initialization. Singleton not created until it's needed
        if (!instanceAPtr)
        {
                instanceAPtr = new ACRNemaMap;
        }
        return instanceAPtr;
}*/

void ACRNemaMap :: GetOneEntry(unsigned int theEntry, char *output, unsigned int outLen, char *format)
{
        ACRNemaAddress	*AAPtr;
        
        if(format==NULL || format[0] == 0) format = defaultFormat;
        if(theEntry < ACRNemaAddressArrayPtr->GetSize())
        {
                AAPtr = ACRNemaAddressArrayPtr->Get(theEntry);
                snprintf(output, outLen, format, AAPtr->Name, AAPtr->IP, AAPtr->Port, AAPtr->Compress);
        }
}

ACRNemaAddress*  ACRNemaMap :: GetOneACRNemaAddress(unsigned int theEntry)
{
        if(theEntry < ACRNemaAddressArrayPtr->GetSize())
                return ACRNemaAddressArrayPtr->Get(theEntry);
        return NULL;
}

unsigned int ACRNemaMap :: GetSize()
{
        return (ACRNemaAddressArrayPtr->GetSize());
}

//Do not forget to free the return string!
char* ACRNemaMap :: GetWholeAMapToString(char *format)
{
        unsigned int index, mapCount, maxLen;
        char *currentPos, *returnStr;
        ACRNemaAddress	*AAPtr;
        
        mapCount = ACRNemaAddressArrayPtr->GetSize();
        maxLen = mapCount * sizeof(ACRNemaAddress);
        if((returnStr = (char*)malloc(maxLen)) == NULL) return NULL;
        currentPos = returnStr;
        if (format==NULL || format[0] == 0) format = defaultFormat;
        for( index = 0;index < mapCount; index++ )
        {
                AAPtr = ACRNemaAddressArrayPtr->Get(index);
                
                snprintf(currentPos, maxLen - strnlen32u(returnStr,maxLen), format,
                         AAPtr->Name,
                         AAPtr->IP,
                         AAPtr->Port,
                         AAPtr->Compress);
                if(strnlen32u(currentPos,maxLen) > 0)// Got some to write.
                {
                        currentPos = returnStr + strnlen32u(returnStr,maxLen);//Move to the end.
                        *currentPos++ = '\n';//Change termination to a return  & move to start of next.
                        *currentPos = '\0';//Terminate the string, never hurts.
                }
        }
        return returnStr;
}

BOOL ACRNemaMap :: InitACRNemaAddressArray(char *amapFile)
{ int		iSize;
  char		*pSrc, *pCurr;
  ACRNemaAddress*	pACR;
	
  ClearMap();
  if(strnlen(amapFile, 2) < 2)return FALSE;
  strcpylim ((char*) ACRNEMAMAP, amapFile, MAX_PATH);//Save the file name.
  pSrc = NULL;
  if((iSize = fileToBuffer((void **)&pSrc, (char*)ACRNEMAMAP, true)) <= 0) return false;
  iSize = decommentBuffer( pSrc, iSize);
  pCurr = pSrc;
// Now fill the ACRNemaAddressArray
  while(*pCurr != 0)// Only a 0 will cause getItemNotEnd not to incrament pCurr
  {
    pACR = new ACRNemaAddress;
//Test for the must haves, getItemNotEnd returns false for a \r or 0
    if(getItemNotEnd(pACR->Name, &pCurr, ACRNEMA_AE_LEN) &&
                getItemNotEnd(pACR->IP, &pCurr, ACRNEMA_IP_LEN))
    {// We have one two and three.
      if(getItemNotEnd(pACR->Port, &pCurr, ACRNEMA_PORT_LEN))
      {// Got three and four
        if(getItemNotEnd(pACR->Compress, &pCurr, ACRNEMA_COMP_LEN))
        {// How strange, more than 4 items.  Should be the end!  Comment?
          while(*pCurr != '\r' && *pCurr != 0)pCurr++;//Dump any more items.
          if(*pCurr != 0) pCurr++;//Next line
        }
      }
      else strcpy(pACR->Compress, "un");//Only 3 items.  Set the default.
                  
    }
    else pACR->Port[0] = 0;// Two or less, not good, make it fail the test.
//Must have all four to enter.  More testing could be added here.
    if(pACR->Name[0] != 0 && pACR->IP[0] != 0 &&  pACR->Port[0] != 0 &&  pACR->Compress[0] != 0)
                  ACRNemaAddressArrayPtr->Add(pACR);
    else delete pACR;
  }
  free(pSrc);
  return TRUE;
}

//Do not forget to free the return string!
char* ACRNemaMap :: PrintAMapToString(char *format)
{
        unsigned int  maxLen;
        char *returnStr, *wholeMap;
        
        maxLen = ACRNemaAddressArrayPtr->GetSize() * sizeof(ACRNemaAddress);
        if((returnStr = (char*)malloc(maxLen + 27)) == NULL) return NULL;
        strcpy(returnStr, "**	AE / IP-PORT Map dump\n\n");
        wholeMap = GetWholeAMapToString(format);
        strcpylim(returnStr + 26, wholeMap, maxLen);
        free(wholeMap);
        return returnStr;
}

// Replaces a ACRNema address at the location (addes if location > listings, adds)
BOOL ACRNemaMap :: ReplaceEntry(unsigned int position, const char *name, const char *ip, const char *port, const char *compress)
{
        if(name == NULL || ip == NULL || port  == NULL) return FALSE;
        ACRNemaAddress *AAPtr = new ACRNemaAddress;
        strcpylim(AAPtr->Name,     name,20);
        strcpylim(AAPtr->IP,       ip,128);
        strcpylim(AAPtr->Port,     port,16);
        if(compress == NULL) strcpy(AAPtr->Compress, "un");
        else strcpylim(AAPtr->Compress, compress, 16);
        if (position >= ACRNemaAddressArrayPtr->GetSize())
                ACRNemaAddressArrayPtr->Add(AAPtr);
        else 
                ACRNemaAddressArrayPtr->ReplaceAt(AAPtr, position);
        return TRUE;
}

BOOL ACRNemaMap :: WriteAMapToTheFile(char *format)
{
        FILE *f;
        time_t timeOfDay3;
        char timeString[128], buf[64], *wholeMap, *formatEnd, *currChar;
        size_t wholeMapLen;
        BOOL retn = FALSE;
        int ses;
        UNUSED_ARGUMENT(buf);//Stop gcc4.2 warning bcb
        
        timeOfDay3 = time(NULL);
        strcpylim(timeString, ctime_r(&timeOfDay3, buf), 128);
// Really check format in this case.  Must have 4 %...s's
        if(format == NULL || strnlen(format, 11) < 11) format = defaultFormat;
        else
        {
                formatEnd = format + strnlen(format, 128);//I just pick 128, no real reason.
                currChar = format;
                ses = 4; // Look for 4 %...s's
                while( currChar < formatEnd)
                {
                        if(*currChar++ == '%')
                        {
                                while(currChar < formatEnd) if(*currChar++ == 's')
                                {
                                        ses--;
                                        break;
                                }
                                if(ses == 0) break;
                        }
                }
                if(ses != 0) format = defaultFormat;
        }
        if((wholeMap = GetWholeAMapToString(format)) == NULL) return FALSE;
        
        f = fopen(ACRNEMAMAP, "wt");
        if(f != NULL)
        {
                if(fprintf(f, "/* **********************************************************\n"
                        " *                                                          *\n"
                        " * DICOM AE (Application entity) -> IP address / Port map   *\n"
                        " * (This is file ACRNEMA.MAP)                               *\n"
                        " * Written by write_amap on %-32s*\n"
                        " *                                                          *\n"
                        " * All DICOM systems that want to retrieve images from the  *\n"
                        " * Conquest DICOM server must be listed here with correct   *\n"
                        " * AE name, (IP adress or hostname) and port number.        *\n"
                        " * The first entry is the Conquest system as example.       *\n"
                        " *                                                          *\n"
                        " *                                                          *\n"
                        " * The syntax for each entry is :                           *\n"
                        " *   AE   <IP adress|Host name>   port number   compression *\n"
                        " *                                                          *\n"
                        " * For compression see manual. Values are un=uncompressed;  *\n"
                        " * ul=littleendianexplicit,ub=bigendianexplicit,ue=both     *\n"
                        " * j1,j2=lossless jpeg;j3..j6=lossy jpeg;n1..n4=nki private *\n"
#ifdef HAVE_J2K
                        " * jk = jpeg2000 lossless, jl = jpeg2000 lossy              *\n"
#endif
                        " *                                                          *\n"
                        " ********************************************************** */\n"
                        "\n", timeString) != -1)
                {
                        wholeMapLen = strnlen(wholeMap, ACRNemaAddressArrayPtr->GetSize() * sizeof(ACRNemaAddress));
                        if(fwrite(wholeMap, wholeMapLen, 1, f) == wholeMapLen )retn = TRUE;
                }
                fclose(f);
        }
        free(wholeMap);
        return retn;

}

BOOL ACRNemaMap :: WriteAMapToAFile(const char *outFile, char *format)
{
        FILE *fptr;
        BOOL retrn = FALSE;
        
        if (outFile == NULL) return FALSE;
        if(format==NULL || format[0] == 0) format = defaultFormat;
        if((fptr = fopen(outFile, "wt")) == NULL )return FALSE;
        if(AppendMapToOpenFile(fptr, format)) retrn = TRUE;
        fclose(fptr);
        return retrn;
}

BOOL  ACRNemaMap :: getItemNotEnd(char *item, char **from, size_t limit)
{
//Look for spaces
        char *pChar;
        BOOL retrn = FALSE;
        
        limit--;//Leave room for the terminator
        pChar = *from;
        while(*pChar != ' ' &&  *pChar != '\r' &&  *pChar != 0 && limit > 0)
        {
                *item++ = *pChar++;
                limit--;
        }
        switch (*pChar)
        {
                case ' ':
                        retrn = TRUE;//The end of an item (Not line or buffer end).
                case '\r':
                        pChar++;
                        if(*pChar == '\r')
                        {// Space followed by a cr;
                                pChar++;
                                retrn = FALSE;
                        }
                        *from = pChar;//Move to next item or line.
                        break;                        
                case 0://The real end, a 0.  No next line
                        break;
                default: //Limit reached
                        while(*pChar != ' ' &&  *pChar != '\r' &&  *pChar != 0) pChar++;// Dump the rest.
                        if(*pChar == ' ')retrn = TRUE;
                        if(*pChar != 0) pChar++;
                        if(*pChar == '\r')
                        {
                                pChar++;
                                retrn =  FALSE;
                        }
                        *from = pChar;//Move to next item or line.
                        break;
        }
        *item = 0;
        return retrn;
}

BOOL  ACRNemaMap :: iswhitespacea(char ch)
{
        switch ( ch )
        {
                case	' ':
                case	'\t':
                case	'\r':
                case	'\n':
                case	0:
                        return ( TRUE );
        }
        return ( FALSE );
}

// For now this works . 4 notation only.  The last char must be an *.
void  ACRNemaMap :: wildIPCheck(char *remoteIP, const char *mappedIP, char *lastGoodIP)
{
        if(remoteIP[0] != 0 && mappedIP[strnlen(mappedIP, ACRNEMA_IP_LEN) - 1] == '*')
        {
                const char *currentChar, *ipEnd, *remoteIPPtr;
                BOOL grpStart = TRUE;
                ipEnd = mappedIP + strnlen(mappedIP, DOT_FOUR_LEN);
                remoteIPPtr = remoteIP;
                for (currentChar = mappedIP; currentChar < ipEnd; currentChar++, remoteIPPtr++)
                {
                        if(grpStart)
                        {
                                while(currentChar == 0)currentChar++;//Skip leading 0s.
                                while(remoteIPPtr == 0)remoteIPPtr++;//Skip leading 0s again.
                                grpStart = FALSE;
                        }
                        if(*currentChar == '*')
                        {// Reached the end still matching.
                                strcpylim(lastGoodIP, remoteIP, DOT_FOUR_LEN);
                                return;
                        }
                        if(*currentChar != *remoteIPPtr) break; //  No match, kill remote IP

                        if(*currentChar == '.')grpStart = TRUE;
                }
                lastGoodIP[0] = 0;
                remoteIP[0] = 0;// No match, kill it
                return;
        }
        lastGoodIP[0] = 0;
        strcpylim(remoteIP, mappedIP, ACRNEMA_IP_LEN);
        return;
}


