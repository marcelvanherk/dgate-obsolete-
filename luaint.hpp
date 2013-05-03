//
//  luaint.hpp
//
/*
 bcb 20120710: Created, moved lua interface here
 bcb 20120918: Removed unused defines.
 bcb 20130313: Merged with 1.4.17a and b.

 */
#ifndef luaint_hpp
#define luaint_hpp

#include "dgate.hpp"
#include "lua.hpp"

#define COMPRESSION_SZ 16
#define STATUSSTRING_SZ 256

extern ExtendedPDU_Service globalPDU; // for global script context

extern "C"
{
    struct scriptdata *getsd(lua_State *L);
    int *getExtInt(const char *what );
    int getExtSLen(const char *what , char **valueH);
    static int _luaprint(lua_State *L, Debug *d);
    static int luaprint(lua_State *L);
    static int luadebuglog(lua_State *L);
    static int luaservercommand(lua_State *L);
    static int luadictionary(lua_State *L);
    static int luascript(lua_State *L);
    static int luaget_amap(lua_State *L);
    static int luaget_sqldef(lua_State *L);
    static int luadbquery(lua_State *L);
    static int luasql(lua_State *L);
    static int luadicomquery(lua_State *L);
    static int luadicommove(lua_State *L);
    static int luadicomdelete(lua_State *L);
    static int getptr(lua_State *L, unsigned int *ptr, int *veclen, int *bytes, int *count, int *step, VR **vr, int mode);
    static int luagetpixel(lua_State *L);
    static int luasetpixel(lua_State *L);
    static int luagetrow(lua_State *L);
    static int luagetcolumn(lua_State *L);
    static int luasetrow(lua_State *L);
    static int luasetcolumn(lua_State *L);
    static int luagetvr(lua_State *L);
    static int luasetvr(lua_State *L);
    static int luareaddicom(lua_State *L);
    static int luawritedicom(lua_State *L);
    static int luawriteheader(lua_State *L);
    static int luanewdicomobject(lua_State *L);
    static int luanewdicomarray(lua_State *L);
    static int luadeletedicomobject(lua_State *L);
    static int luaheapinfo(lua_State *L);
    static int luaHTML(lua_State *L);
    static int luaCGI(lua_State *L);
    static int luagpps(lua_State *L);
    static int luaGlobalIndex(lua_State *L);
    static int luaGlobalNewIndex(lua_State *L);
    static int luaAssociationIndex(lua_State *L);
    static int luaSeqClosure(lua_State *L);
    static int luaSeqnewindex(lua_State *L);
    static int luaSeqindex(lua_State *L);
    static int luaSeqgc(lua_State *L);
    static int luaSeqlen(lua_State *L);
    static void luaCreateObject(lua_State *L, DICOMDataObject *pDDO, Array < DICOMDataObject *> *A, BOOL owned);
}

const char *do_lua(lua_State **L, const char *cmd, struct scriptdata *sd);//bcb made const
void lua_setvar(ExtendedPDU_Service *pdu, char *name, char *value);
void lua_getstring(ExtendedPDU_Service *pdu, DICOMCommandObject *dco, DICOMDataObject *ddo, char *cmd, char *result);
int luaopen_pack(lua_State *L);
char *heapinfo( void );

#endif
