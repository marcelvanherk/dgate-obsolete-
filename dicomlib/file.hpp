//
//  file.h
//
//  Created by Bruce Barton on 4/2/13.
//
//
/*
 bcb 20130402: Created this file.
*/

#ifndef file_h
#define file_h

#define ERR_SIZE 0
#define ERR_OPEN -1
#define INV_HANDLE -2
#define ERR_FSTAB -3
#define ERR_FILE_TYPE -4
#define ERR_ALLOC -5
#define ERR_READ -6

// This will read a file size, create a buffer the size of the file and fill it.
// The buffer will always end in a 0, this so good for text and has no effect on binary
// crlf true will add a cr and lf to the end if not present, counted in the size.
// It returns the buffer size minus the 0 if ok or a 0 and sets errorno to the first error.
// *data must be freed when done!
int fileToBuffer(void **bufferH, const char *theFile, BOOL crlf = false);
// Same as above, but it will print an error message into *errorBuf if there is a problem.
int fileToBufferWithErr(void **bufferH, const char *theFile, char *errorBuf, int errSize, BOOL crlf = false);
// Get rid of all comments within asterick slash and
// replace all LF by CR, and all '\t' by ' ' with no doubles in the buffer
// Returns the new buffer data size, it does not reallocate the buffer.
int decommentBuffer(char *pSrc, int bSize);
// Returns the size of the comment section ended by */
int commentSize( const char *startOfComments, int maxSize);
#endif
