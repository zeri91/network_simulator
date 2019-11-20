#ifndef CONNECTIONDB_H
#define CONNECTIONDB_H

#include <list>

namespace NS_OCH {

class OchObject;
class Connection;
class Log;
//class OXCNode;
// class NetMan;

template<class Key, class T> class MappedLinkList;

class ConnectionDB: public OchObject
{
	ConnectionDB(const ConnectionDB&);
	const ConnectionDB& operator=(const ConnectionDB&);
public:
//	void tearDownAllConnections(NetMan *, bool bLog=false);
	void logFinal(Log &);
	ConnectionDB();
	~ConnectionDB();

	virtual void dump(ostream&) const;
	void addConnection(Connection*);
	void removeConnection(Connection*);
	UINT getTotalBandwidth() const;
	UINT countBackConn(UINT&);

public:
	MappedLinkList<UINT, Connection*>	m_hConList;
};

};
#endif
