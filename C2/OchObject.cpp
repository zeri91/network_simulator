#include "OchInc.h"
#include "OchObject.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

OchObject::OchObject()
{
}

OchObject::~OchObject()
{
}

const char* OchObject::toString() const
{
	const char *pBuf = "OchObject";
	return pBuf;
}

void OchObject::dump(ostream& out) const
{
	NULL;
}
