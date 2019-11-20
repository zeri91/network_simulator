#include "OchMemDebug.h"

#ifdef WIN32
#ifdef _DEBUG

	void* operator new(size_t nSize, const char * lpszFileName, int nLine)
	{
		return ::operator new(nSize, 1, lpszFileName, nLine);
	}

#endif	// _DEBUG
#endif	// WIN32
