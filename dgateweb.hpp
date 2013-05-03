//
//  dgateweb.hpp
//

/*
bcb 20120710  Created.
*/
//

#ifndef dgateweb_hpp
#define dgateweb_hpp

#include "dgate.hpp"

//Cannot be static, used in dgate
int htoin(const char *value, int len);
int isxdig(char ch);
int CGI(char *out, const char *name, const char *def);
void DgateCgi(char *query_string, char *ext);
void HTML(const char *string, ...);
#ifdef UNIX
static int xdigit(char ch);
static bool Tabulate(const char *c1, const char *par, const char *c4, BOOL edit=FALSE);
static void replace(char *string, const char *key, const char *value);
static bool DgateWADO(char *query_string, char *ext);
#else
int xdigit(char ch);
bool Tabulate(const char *c1, const char *par, const char *c4, BOOL edit=FALSE);
void replace(char *string, const char *key, const char *value);
bool DgateWADO(char *query_string, char *ext);
#endif
#endif
