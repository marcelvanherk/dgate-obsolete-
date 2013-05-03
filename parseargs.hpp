//
//  parseargs.h
//
/*
 20120820 bcb Created.
 */

#ifndef parseargs_h
#define parseargs_h

#include "nkiqrsop.hpp"
#include "lex.hpp"

extern ExtendedPDU_Service globalPDU; // for global script context

int ParseArgs(int argc, char *argv[], ExtendedPDU_Service *PDU);
#endif
