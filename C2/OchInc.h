#ifndef OCHINC_H
#define OCHINC_H

// Standard include files
#include <iostream>
#include <string>
#include <float.h>	// for DBL_MAX
using namespace std;

// Customized include files
#include "TypeDef.h"
#include "ConstDef.h"
#include "MacroDef.h"
#include "OchDebug.h"
#include "OchException.h"	// exceptions

#ifdef _OCH_JIT_DEBUG
	bool g_bJIT_Debug = false;	// global variable, true for temp debug
#else
	bool extern g_bJIT_Debug;
#endif

#endif
