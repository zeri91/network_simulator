#ifndef OCHOBJECT_H
#define OCHOBJECT_H
//#define DEBUGB
//#define DEBUG
//#define DEBUGC
#define DEBUGF

namespace NS_OCH {

#include <iostream>

class OchObject {
public:
	OchObject();
	virtual ~OchObject();

	// for debugging
	virtual const char* toString() const;
	virtual void dump(ostream& out) const;
};

};	// namespace
#endif
