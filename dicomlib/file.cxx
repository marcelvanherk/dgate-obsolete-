// compile with: /EHac
//
//  File.cpp
//
//  Created by Bruce Barton on 4/2/13.
//
//
/*
* This uses fstream class for file functions.  
* Because is a class writen for each system,
* no Windows or Unix problems!

 bcb 20130402: Created this file.
 bcb 20130520: Fixed ; starting a comment. (bad for lua)
*/
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <errno.h>
#include "file.hpp"

#ifndef UNIX
#define snprintf _snprintf
#endif

int fileToBuffer(void **bufferH, const char *theFile, BOOL crlf)
{
        int             err, extra;

// Clear any old errors
        errno = 0;
// Check for a pointer to a null.
        if( bufferH == NULL || *bufferH != NULL)
        {
                return INV_HANDLE;
        }
// How much to add to the buffer.
        extra = 1;
        if(crlf) extra = 3;
// Get the file
        std::ifstream myFile;
        myFile.open( theFile, std::ifstream::in |
		std::ifstream::binary |
		std::ifstream::ate);
	if (!myFile.is_open()) return ERR_READ;
// Get the file info.
	std::ifstream::pos_type fEnd;
	fEnd = myFile.tellg();
	myFile.seekg (0, std::ifstream::beg);// Back to the start.
// Check file size
        if (fEnd <= 0)
        {
                myFile.close();
                return ERR_SIZE; //Leave errno at 0 for 0 file size.
        }
        if (fEnd > INT_MAX - extra)// Limits the file to 2gb
        {
                myFile.close();
                errno = EFBIG;
                return ERR_SIZE;
        }
// Make the buffer.
        int fSize = fEnd & INT_MAX;
        *bufferH = (char*)malloc((fSize) + extra);
        if (!bufferH)
        {
                err = errno;
                myFile.close();// May change errno.
                errno = err;
                return ERR_ALLOC;
        }
// Read it into memory.
        myFile.read(*(char **)bufferH, fSize);
        if ( myFile.gcount() != fSize)
        {
                err = errno;
                myFile.close();// May change errno.
                free(*bufferH);
                *bufferH = NULL;
                errno = err;
                return ERR_READ;
        }
// Done with it.
        myFile.close();// Will set errno if fails.
// Make a clean CR/LF at the end and a zero to terminated string.
        if(crlf)
        {
                while(fSize > 0)
                {
                        if(((char *) *bufferH)[fSize - 1] != '\r' &&
                           ((char *) *bufferH)[fSize - 1] != '\n' &&
                           ((char *) *bufferH)[fSize - 1] != '\0')break;
                        fSize--;
                }
                ((char *) *bufferH)[fSize++] = '\r';
                ((char *) *bufferH)[fSize++] = '\n';
        }
        ((char *) *bufferH)[fSize]   = '\0';// Always ends in zero.
        return fSize & INT_MAX;
}

int fileToBufferWithErr(void **bufferH, const char *theFile, char *errorBuf, int errSize, BOOL crlf)
{
// Check for a pointer to a null.
        if( errorBuf == NULL ) return 0;
        errorBuf[0] = 0; //Clear any old errors.
        int fSize = fileToBuffer(bufferH, theFile, crlf);
        if (fSize <= 0)// Fatal errors
        {
                switch (fSize)
                {
                        case INV_HANDLE: strcpylim( errorBuf, "Invalid handle\n", errSize);
                                return 0;
                        case ERR_OPEN: snprintf( errorBuf, errSize,
                                        "Can not open the needed file %s: %s\n",
                                        theFile, strerror(errno));
                                return 0;
                        case ERR_FSTAB: snprintf( errorBuf, errSize,
                                        "Can not get the size of %s: %s\n",
                                        theFile, strerror(errno));
                                return 0;
                        case ERR_FILE_TYPE: snprintf( errorBuf, errSize,
                                        "%s is not a file", theFile);
                                return 0;
                        case ERR_ALLOC: snprintf( errorBuf, errSize,
                                        "Memory allocation error: %s", strerror(errno));
                                return 0;
                        case ERR_READ: snprintf( errorBuf, errSize,
                                        "Error reading file %s: %s",
                                        theFile, strerror(errno));
                                return 0;
                        case ERR_SIZE:if (errno == 0)
                                {
                                        snprintf( errorBuf, errSize,
                                                "The file %s is empty\n",
                                                theFile);
                                        return 0;
                                }
                                if (errno == EFBIG)
                                {
                                        snprintf( errorBuf, errSize,
                                                "The file %s is to big\n",
                                                theFile);
                                        return 0;
                                        
                                }
                        default: strcpylim( errorBuf, "Unknown error.\n", errSize );
                                return 0;
                }
        }
        if (errno != 0)snprintf( errorBuf, errSize,
                               "Error while closing %s: %s",
                               theFile, strerror(errno));
        return fSize;
}

int decommentBuffer(char *pSrc, int bSize)
{
        // Get rid of all comments within asterick slash and
        // replace all LF by CR, and all '\t' by ' ' with no doubles
        char *pDest;
        int curr = 0;
        
        pDest = pSrc;
        while (curr<bSize)
        {
                switch (pSrc[curr])
                {
                        case '\\':
                                *pDest++ = pSrc[curr++];//Copy the backslash
                                *pDest++ = pSrc[curr];//Copy whats next
                                break;
                        case '\"':
                                *pDest++ = pSrc[curr++];// Save the first quote.
                                // Save everything until the next quote.
                                while(curr < bSize)
                                {
                                        if(pSrc[curr] == '\"')
                                        {
                                                *pDest++ = pSrc[curr];// Save the last quote.
                                                break;
                                        }
                                        //Copy the backslash and what is next without test.
                                        if(pSrc[curr] == '\\') *pDest++ = pSrc[curr++];
                                        *pDest++ = pSrc[curr++];
                                }
                                break;
                        case '\n':
                        case '\r':
                                if (pDest != pSrc && pDest[-1] != '\r') *pDest++ = '\r';//No doubles or leading.
                                break;
                        case ' ':
                        case '\t'://No doubles or spaces before a return.
                                if (pDest != pSrc && pDest[-1] != ' ' &&
                                    pSrc[curr + 1] != '\r' && pSrc[curr + 1] != '\n') *pDest++ = ' ';
                                break;
                        case '/':
                                if (pSrc[curr + 1] == '*')
                                { // Found the start of a comment
                                        curr += 2;
                                        curr += commentSize(&pSrc[curr], bSize - curr);
                                        break;
                                }
                                if (pSrc[curr + 1] != '/')//A comment will fall through.
                                {
                                        *pDest++ = pSrc[curr];//Not a comment, copy and next.
                                        break;
                                }
//                        case ';'://Bad for lua.
                        case '#'://Comment line.
                                while (curr<bSize && pSrc[curr] != '\r' && pSrc[curr] != '\n') curr++;//Look for the end of the line.
                                //delme ?                                if (pDest != pBuffer && pDest[-1] != '\r') *pDest++ = '\r';
                                break;
                        default:
                                *pDest++ = pSrc[curr];
                                break;
                }
                curr++;
        }
        *pDest = 0;
        return (pDest - pSrc) & INT_MAX;
}

int commentSize( const char *startOfComments, int maxSize)
{
        int curr = 0;
        
        while (curr < maxSize)
                if (startOfComments[curr++] == '*' &&
                        curr < maxSize && (startOfComments[curr++] == '/')) break;
        if (curr < maxSize) return curr;
        return maxSize;
}
