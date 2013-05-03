//
//  parse.hpp
//
/*
20120710 bcb Created.
20130226 Removed GetDBSpecials(void) as part of the IniValue Class change.  Version to 1.4.18a.
*/

#ifndef parse_hpp
#define parse_hpp

//#include "nkiqrsop.hpp"
#include "dbsql.hpp"
#include "lex.hpp"

BOOL LoadKFactorFile(char	*KFile);
BOOL ParseComment(Lex & );
#ifdef UNIX //Statics
static BOOL ConvertDB(DBENTRY	**, Array < DBENTRY > * );
static BOOL Parse(Lex	&, Array < DBENTRY > *, Array < DBENTRY > *, Array < DBENTRY > *, Array < DBENTRY > *, Array < DBENTRY > * );
static BOOL ParseDB(Lex &, Array < DBENTRY > *, BOOL HasHL7);
#else
//void GetDBSpecials(void);
BOOL ConvertDB(DBENTRY	**, Array < DBENTRY > * );
BOOL Parse(Lex	&, Array < DBENTRY > *, Array < DBENTRY > *, Array < DBENTRY > *, Array < DBENTRY > *, Array < DBENTRY > * );
BOOL ParseDB(Lex &, Array < DBENTRY > *, BOOL HasHL7);
#endif
#endif
