#pragma warning(disable:4786)
#include <assert.h>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "AbstractPath.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "Connection.h"
#include "Event.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

Event::Event()
{
}

Event::Event(SimulationTime hTime, SIM_EVENTS hEvent, Connection* pConnection): 
m_hTime(hTime), m_hEvent(hEvent), isDepValid(true)
{
	m_pSource = NULL;
	m_pConnection = pConnection;
	fronthaulEvent = NULL;
	backhaulBlocked = false;
}

Event::Event(SimulationTime hTime, SIM_EVENTS hEvent, Connection* pConnection, OXCNode*pOXCNode):
	m_hTime(hTime), m_hEvent(hEvent), isDepValid(true)
{
	m_pSource = pOXCNode;
	m_pConnection = pConnection;
	fronthaulEvent = NULL;
	backhaulBlocked = false;
}

NS_OCH::Event::Event(SimulationTime hTime, SIM_EVENTS hEvent, Connection *pConnection, Event *pEvent):
m_hTime(hTime), m_hEvent(hEvent), isDepValid(true)
{
	m_pSource = NULL;
	m_pConnection = pConnection;
	fronthaulEvent = pEvent;
	backhaulBlocked = false;
}

Event::~Event()
{

//	if (EVT_DEPARTURE == m_hEvent) {
//		delete m_pConnection;
//		m_pConnection = NULL;
//	}
}

void Event::dump(ostream& out) const
{
	out.width(14);
	out.precision(4);
	out.setf(ios::fixed, ios::floatfield);
	out << m_hTime << ": "; //-B: event's arrival time
	char *pEvent;
	switch (m_hEvent) {
		case EVT_ARRIVAL:
			pEvent = "Arrive";
			break;
		case EVT_DEPARTURE:
			pEvent = "Depart";
			break;
		default:
			DEFAULT_SWITCH;
	}
	out<<pEvent<<", ";
	if (m_pConnection)
		m_pConnection->dump(out);
	else
		out<<"connection NULL";
	if (fronthaulEvent)
	{
		cout << ", fronthaul event SI ";
		if (fronthaulEvent->m_pConnection)
		{
			cout << ", connection " << fronthaulEvent->m_pConnection->m_nSequenceNo
				<< ": " << fronthaulEvent->m_pConnection->m_nSrc
				<< "->" << fronthaulEvent->m_pConnection->m_nDst << endl;
		}
		else
			cout << ", connection NULL" << endl;
	}
	else
		cout << ", fronthaul event NULL" << endl;
}
