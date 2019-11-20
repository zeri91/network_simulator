#ifndef OCHMEMDEBUG_H
#define OCHMEMDEBUG_H

#ifdef WIN32
#ifdef _DEBUG
	// Enable MS memory-leak detection, may cause warning C4291
	#include <crtdbg.h>
	#pragma warning(disable: 4291)
	void* operator new(size_t nSize, const char * lpszFileName, int nLine);
	#define DEBUG_NEW new(THIS_FILE, __LINE__)

	#define MALLOC_DBG(x) _malloc_dbg(x, 1, THIS_FILE, __LINE__);
	#define malloc(x) MALLOC_DBG(x)
#endif	// _DEBUG
#endif	// WIN32

#endif	// OCHMEMDEBUG_H
