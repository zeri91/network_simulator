#ifndef TOPOREADER_H
#define TOPOREADER_H

#include <fstream>
namespace NS_OCH {

class WDMNetwork;
class TopoReader: public NS_OCH::OchObject {
	TopoReader(const TopoReader&);
	const TopoReader& operator=(const TopoReader&);
public:
	TopoReader();
	~TopoReader();

	bool readTopo(WDMNetwork&, const char*);
	bool BBUReadTopo(WDMNetwork & hNetwork, const char * pTopoFile); //-B

protected: //protected
	bool readTopoHelper(WDMNetwork&, ifstream&);
	bool BBUReadTopoHelper(WDMNetwork & hNetwork, ifstream & fin);
};
};

#endif
