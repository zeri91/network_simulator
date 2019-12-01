#pragma warning (disable: 4786)
#include <iostream>
#include <assert.h>

#include "OchInc.h"
#include <stdlib.h>		// for srand and rand

#include "OchObject.h"
#include "Connection.h"
#include "MappedLinkList.h"
#include "ConnectionDB.h"
//#include "OXCNode.h"
//#include "AbstractGraph.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

ConnectionDB::ConnectionDB()
{
}

ConnectionDB::~ConnectionDB()
{
	list<Connection*>::iterator itr;
	for (itr=m_hConList.begin(); itr!=m_hConList.end(); itr++)
		delete (*itr);
}

void ConnectionDB::dump(ostream& out) const
{
#ifdef DEBUGB
	cout << "\n->ConnectionDB DUMP" << endl;
#endif // DEBUGB

	out<<"\tConnectionDB, # = "<<m_hConList.size()<<endl;
	list<Connection*>::const_iterator itr;
	for (itr=m_hConList.begin(); itr!=m_hConList.end(); itr++)
		(*itr)->dump(out);
}

void ConnectionDB::addConnection(Connection *pConnection)
{
	assert(pConnection);
#ifdef DEBUGC
	cout << "I'm going to add connection with id " << pConnection->m_nSequenceNo 
		<< " from " << pConnection->m_nSrc << " to " << pConnection->m_nDst
		<< " to the DB" << endl;
#endif

	m_hConList.push_front(pConnection->m_nSequenceNo, pConnection);
}

void ConnectionDB::removeConnection(Connection *pConnection)
{
	assert(pConnection);
	m_hConList.erase(pConnection->m_nSequenceNo);
}

//-B: nothing to do
void ConnectionDB::logFinal(Log &hLog)
{
}

UINT ConnectionDB::getTotalBandwidth() const
{
	UINT nBW = 0;
	list<Connection*>::const_iterator itr;
	for (itr=m_hConList.begin(); itr!=m_hConList.end(); itr++)
		nBW += (*itr)->m_eBandwidth;
	return nBW;
}




UINT ConnectionDB::countBackConn(UINT&totBwd)
{
	UINT count = 0;
	list<Connection*>::const_iterator itr;
	for (itr = m_hConList.begin(); itr != m_hConList.end(); itr++)
	{
		switch ((*itr)->m_eConnType)
		{
			case Connection::FIXED_BACKHAUL:
			case Connection::MOBILE_BACKHAUL:
			case Connection::FIXEDMOBILE_BACKHAUL:
				count++;
				totBwd += (*itr)->m_eBandwidth;
				break;
			case Connection::MOBILE_FRONTHAUL:
			case Connection::FIXEDMOBILE_FRONTHAUL:
			case Connection::FIXED_MIDHAUL:
				NULL;
				break;
			default:
				DEFAULT_SWITCH;
		} //end SWITCH
	} //end FOR
	return count;
}