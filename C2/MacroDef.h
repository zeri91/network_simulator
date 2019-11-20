#ifndef MACRODEF_H
#define MACRODEF_H

// define some handy macros

#undef DELETE_THEN_NULL
#define DELETE_THEN_NULL(p)		{ delete p; p = NULL; }
#undef CHECK_THEN_DELETE
#define CHECK_THEN_DELETE(p)	if (p) DELETE_THEN_NULL(p)

#endif
