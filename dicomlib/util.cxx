/****************************************************************************
          Copyright (C) 1995, University of California, Davis

          THIS SOFTWARE IS MADE AVAILABLE, AS IS, AND THE UNIVERSITY
          OF CALIFORNIA DOES NOT MAKE ANY WARRANTY ABOUT THE SOFTWARE, ITS
          PERFORMANCE, ITS MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR
          USE, FREEDOM FROM ANY COMPUTER DISEASES OR ITS CONFORMITY TO ANY
          SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND PERFORMANCE OF
          THE SOFTWARE IS WITH THE USER.

          Copyright of the software and supporting documentation is
          owned by the University of California, and free access
          is hereby granted as a license to use this software, copy this
          software and prepare derivative works based upon this software.
          However, any distribution of this software source code or
          supporting documentation or derivative works (source code and
          supporting documentation) must include this copyright notice.
****************************************************************************/

/***************************************************************************
 *
 * University of California, Davis
 * UCDMC DICOM Network Transport Libraries
 * Version 0.1 Beta
 *
 * Technical Contact: mhoskin@ucdavis.edu
 *
 ***************************************************************************/

/*
20120820 bcb Added functions
20120920 bcb Added limited length string compares
20130226 bcb Added time compares, NULL checks and strnlenint.
                Size limits now size_t.  Version to 1.4.18a.
                Commented unused functions.
*/

#	include	"dicom.hpp"

/*BOOL ByteCopy(BYTE *s1, BYTE *s2, UINT Count)
{
        memcpy((void *) s1, (void *) s2, Count);
        return ( TRUE );
}Unsued! */

/*UINT ByteStrLength(BYTE	*s)
{
        return (UINT)strnlen((char *) s, UINT_MAX);
}Unused!*/

UINT16 ConvertNumber(char	*s)
{
        UINT16	Value;
        int		Index;

        if(s == NULL) return 0;
        if(strlen(s)>2)
        {
                if((s[1] == 'x')||(s[1] == 'X'))
                {
                        Value = 0;
                        Index = 2;
                        while(s[Index])
                        {
                                Value = (Value << 4) + Hex2Dec(s[Index]);
                                ++Index;
                        }
                        return ( Value );
                }
        }
        return((UINT16)atoi(s));
}

UINT16 Hex2Dec(char	ch)
{
        if((ch>='a')&&(ch<='f'))
                return(ch-'a'+10);
        if((ch>='A')&&(ch<='F'))
                return(ch-'A'+10);
        if((ch>='0')&&(ch<='9'))
                return(ch-'0'+0);
        return ( 0 );
}

UINT secofday(struct tm *tm1)
{
        if(tm1 == NULL) return 0;
        return ((((tm1->tm_hour * 60) + tm1->tm_min) *60) + tm1->tm_sec);
}

BOOL SpaceMem( BYTE *mem, UINT Count )
{
        if(mem == NULL) return false;
        memset((void *) mem, ' ', Count);
        return ( TRUE );
}

// strcpylim will always terminate the string!  Returns a pointer to the null.
char *  strcatlim(char * dest, const char * source, size_t limit)
{
        if(dest == NULL)return(NULL);
        while(--limit > 0 && dest != 0)dest++;
        return strcpylim(dest, source, limit);
}

// strcpylim will always terminate the string!  Returns a pointer to the null.
char *  strcpylim(char * dest, const char * source, size_t limit)
{
        if(dest == NULL)return(NULL);
        if(source != NULL)
        {
                while(--limit > 0)//decrement first, last char always 0
                {
                        *dest = *source++;
                        if(*dest++ == 0)return(dest);
                }
        }
        *dest = 0;
        return(dest);
}

// strcpysplim will always terminate the string!  Returns a pointer to the null.
// It will copy until a space or null.
char *  strcpysplim(char * dest, const char * source, size_t limit)
{
        if(dest == NULL)return(NULL);
        if(source != NULL)
        {
                while(--limit > 0)//decrement first, last char always 0
                {
                        *dest = *source++;
                        if(*dest == ' ')break;
                        if(*dest++ == 0)return(dest);
                }
        }
        *dest = 0;
        return(dest);
}

// A quicker compare of a string to a string.
// As soon as a mismatch is found, returns false.
BOOL streq(const char * strOne, const char * strTwo)
{
        if(strOne == NULL && strTwo == NULL) return true;
        if(strOne == NULL || strTwo == NULL) return false;
        while(*strOne != 0 && *strTwo != 0)
        {
                if(*strOne != *strTwo)return false;
                strOne++;
                strTwo++;
        }
        return (*strOne == *strTwo);//both should end in 0!
}

// A quicker compare of a string to a string, limited by n.
// As soon as a mismatch is found, returns false.
BOOL strneq(const char * strOne, const char * strTwo ,size_t cnt)
{
        if(strOne == NULL && strTwo == NULL) return true;
        if(strOne == NULL || strTwo == NULL) return false;
        while(cnt > 0)
        {
                if(*strOne == 0 && *strTwo == 0) return true;
                if(*strOne != *strTwo)return false;
                strOne++;
                strTwo++;
                cnt--;
        }
        return true;
}

// A quicker compare of an anycase string to an any case string.
// As soon as a mismatch is found, returns false.
BOOL strieq(const char * lowUp, const char * lowUp2)
{
        if(lowUp == NULL && lowUp2 == NULL) return true;
        if(lowUp == NULL || lowUp2 == NULL) return false;
        while(*lowUp2 != 0 && *lowUp != 0)
        {
                if(*lowUp2 != *lowUp && *lowUp2 != *lowUp + 0x20 && 
                   *lowUp != *lowUp2 + 0x20 )return false;
                lowUp2++;
                lowUp++;
        }
        return (*lowUp2 == *lowUp);//both should end in 0!
}

// A quicker compare of an anycase string to an any case string, limited by n.
// As soon as a mismatch is found, returns false.
BOOL strnieq(const char * lowUp, const char * lowUp2, size_t cnt)
{
        if(lowUp == NULL && lowUp2 == NULL) return true;
        if(lowUp == NULL || lowUp2 == NULL) return false;
        while(cnt > 0)
        {
                if(*lowUp2 == 0 && *lowUp == 0) return true;//End of strings.
                if(*lowUp2 != *lowUp && *lowUp2 != *lowUp + 0x20 && 
                   *lowUp != *lowUp2 + 0x20 )return false;
                lowUp2++;
                lowUp++;
                cnt--;
        }
        return  true;
}

// A quicker compare of an anycase string to a lower case only string.
// As soon as a mismatch is found, returns false.
BOOL strileq(const char * lowUp, const char * lowOnly)
{
        if(lowUp == NULL && lowOnly == NULL) return true;
        if(lowUp == NULL || lowOnly == NULL) return false;
        while(*lowOnly != 0 && *lowUp != 0)
        {
                if(*lowOnly != *lowUp && *lowOnly != *lowUp + 0x20)return false;
                lowOnly++;
                lowUp++;
        }
        return (*lowOnly == *lowUp);//both should end in 0!
}

// A quicker compare of an anycase string to a lower case only string, limited by n.
// As soon as a mismatch is found, returns false.
BOOL strnileq(const char * lowUp, const char * lowOnly, size_t cnt)
{
        if(lowUp == NULL && lowOnly == NULL) return true;
        if(lowUp == NULL || lowOnly == NULL) return false;
        while(--cnt > 0)
        {
                if(*lowOnly == 0 || *lowUp == 0) cnt = 0;//One more compare of 0s.
                if(*lowOnly != *lowUp && *lowOnly != *lowUp + 0x20)return false;
                lowOnly++;
                lowUp++;
        }
        return true;
}

INT32 strnlen32(const char * lstring, size_t maxLen)
{
        INT32	sLen, max;
        if(lstring == NULL) return 0;
        max = (int)(maxLen & 0x7FFFFFFF);  //Just being carful, should never need, but ...
        for (sLen = 0; sLen < max; sLen++) if(*lstring++ == 0) return(sLen);
        return (max);
}

UINT32 strnlen32u(const char * lstring, size_t maxLen)
{
        UINT32	sLen, max;
        if(lstring == NULL) return 0;
        max = (int)(maxLen & 0xFFFFFFFF);
        for (sLen = 0; sLen < max; sLen++) if(*lstring++ == 0) return(sLen);
        return (max);
}

int strnlenint(const char * lstring, size_t maxLen)
{
        int	sLen, max;
        if(lstring == NULL) return 0;
        max = (int)(maxLen & INT_MAX);
        for (sLen = 0; sLen < max; sLen++) if(*lstring++ == 0) return(sLen);
        return (max);
}

#ifndef strnlen
size_t strnlen(const char * lstring, size_t maxLen)
{
        size_t	sLen;
        if(lstring == NULL) return 0;
        for (sLen = 0; sLen < maxLen; sLen++) if(*lstring++ == 0) return(sLen);
        return (maxLen);
}
#endif

/*size_t strn0len(char * lstring, size_t maxLen)
 {
 size_t	sLen;
 if(lstring == NULL) return 0;
 for (sLen = 0; sLen < maxLen; sLen++) if(*lstring++ == 0) return(sLen);
 *lstring == 0;//Fix it!
 return (maxLen);
 }*/

// Compare the time only part of a tm structure, both structures must have values for a true.
BOOL timeofdayeq(struct tm *tm1, struct tm *tm2)
{
        UINT sec1, sec2;
        if(tm1 == NULL || tm2 == NULL)return false;
        if((sec1 = secofday(tm1) == 0) || (sec2 = secofday(tm2) == 0) || (sec1 != sec2)) return false;
        return true;
}

BOOL ZeroMem(BYTE *mem, UINT Count)
{
        if(mem == NULL) return false;
        memset((void *) mem, 0, Count);
        return ( true );
}

