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
20120820 bcb Added functions, prevented double loading
20120920 bcb Added limited length string compares
20130226 bcb Added time compares, NULL checks and strnlenint.
                Size limits now size_t.   Version to 1.4.18a.
*/

#ifndef dutils_h
#define dutils_h

#include <limits.h>
#include <time.h>

/* Utility C functions */
UINT		uniq();
UINT32		uniq32();
UINT16		uniq16();
UINT8		uniq8();
UINT8		uniq8odd();
UINT16		uniq16odd();

//BOOL ByteCopy(BYTE *, BYTE *, UINT); Unsued
//UINT ByteStrLength(BYTE *);Unused
UINT16 ConvertNumber(char*);
UINT16 Hex2Dec(char ch);
UINT secofday(struct tm *tm1);
BOOL SpaceMem(BYTE *, UINT);
// strcpylim will always terminate the string!  Returns a pointer to the null.
char *  strcatlim(char * dest, const char * source, size_t limit);
char *  strcpylim(char * dest, const char * source, size_t limit);
char *  strcpysplim(char * dest, const char * source, size_t limit);
BOOL streq(const char * strOne, const char * strTwo);
BOOL strneq(const char * strOne, const char * strTwo, size_t cnt);
BOOL strieq(const char * lowUp, const char * lowUp2);
BOOL strnieq(const char * lowUp, const char * lowUp2, size_t cnt);
BOOL strileq(const char * lowUp, const char * lowOnly);
BOOL strnileq(const char * lowUp, const char * lowOnly, size_t cnt);
INT32 strnlen32(const char * lstring, size_t maxLen = 0x7FFFFFFF);
UINT32 strnlen32u(const char * lstring, size_t maxLen = 0xFFFFFFFF);
int strnlenint(const char * lstring, size_t maxLen = INT_MAX);
#ifndef strnlen
size_t strnlen(const char * lstring, size_t maxLen);
#endif
//size_t strn0len(char * lstring, size_t maxLen);
BOOL timeofdayeq(struct tm *tm1, struct tm *tm2);
BOOL ZeroMem(BYTE *, UINT);

#endif
