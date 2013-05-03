/*
 20101227	bcb	Created this file
*/

#ifndef DGATEFN_HPP_
#define DGATEFN_HPP_

#include "dicom.hpp"
#include "odbci.hpp"

static int dgatefnmin(int a, int b);
static int GetFileNameSyntax(unsigned int *maxfilenamelength);
/* Cleanup e.g., patient name */
static void Cleanup(char *Name);
// Generate a file name prefix for an IOD
BOOL GenerateFileName( DICOMDataObject *DDOPtr, char *Device, char *Root,
	char *filename, BOOL NoKill, int Syntax, char *called, char *calling,
					  Database *db);
#endif //DGATEFN_HPP_
