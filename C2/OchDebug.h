#ifndef _OCHDEBUG_H
#define _OCHDEBUG_H

#include <iostream>
#include <stdio.h>
#include <assert.h>

#define WHERE			{cerr<<__FILE__<<", line "<<__LINE__<<":"; cerr.flush();}
#define DEFAULT_SWITCH	{assert(false); cerr<<"- ERROR: Should NOT reach "; cerr<<__FILE__<<", line "<<__LINE__<<endl;}


#ifdef _OCHDEBUG
	#define ODS0(s)			{WHERE; cerr<<s; cout.flush();}
	#define ODS1(s)			{WHERE; cerr<<#s<<" = "<<s<<endl;}
	#define ODS2(s1, s2)	{WHERE; cerr<<#s1<<" = "<<s1<<"; "<<#s2<<" = "<<s2<<endl;}
#else
	#define ODS0(s)			NULL
	#define ODS1(s)			NULL
	#define ODS2(s1, s2)	NULL
#endif

#ifdef _OCHDEBUG
	#define TRACE0(sHint)					printf(sHint)
	#define TRACE1(sHint, p1)				printf(sHint, p1)
	#define TRACE2(sHint, p1, p2)			printf(sHint, p1, p2)
	#define TRACE3(sHint, p1, p2, p3)		printf(sHint, p1, p2, p3)
	#define TRACE4(sHint, p1, p2, p3, p4)	printf(sHint, p1, p2, p3, p4)
#else
	#define TRACE0(sHint)					NULL
	#define TRACE1(sHint, p1)				NULL
	#define TRACE2(sHint, p1, p2)			NULL
	#define TRACE3(sHint, p1, p2, p3)		NULL
	#define TRACE4(sHint, p1, p2, p3, p4)	NULL
#endif


#endif
