#pragma warning(disable: 4786)
#include <assert.h>
#include <iostream>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "AbstractPath.h"
#include "OXCNode.h"
#include "Channel.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "LightpathDB.h"
#include "ConnectionDB.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "Log.h"
#include "NetMan.h"
#include "Lightpath.h"
#include "LightpathDB.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

UINT LightpathDB::g_nNextLightpathId = 1;	// lightpath 0 reserved for new lightpaths

LightpathDB::LightpathDB()
{
}

LightpathDB::~LightpathDB()
{
	list<Lightpath*>::iterator itr;
	for (itr=m_hLightpathList.begin(); itr!=m_hLightpathList.end(); itr++)
		delete (*itr);
}

void LightpathDB::dump(ostream& out) const
{
#ifdef DEBUGB
	cout << "->LightpathDB DUMP" << endl;
#endif // DEBUGB
	out<<"\tLightpathDB, # = "<<m_hLightpathList.size() <<endl;
	list<Lightpath*>::const_iterator itr;
	for (itr = m_hLightpathList.begin(); itr != m_hLightpathList.end(); itr++) {
		//out<<'\t';
		// -B: LIGHTPATH DUMP
		(*itr)->dump(out);
		out<<endl;
	}
}

UINT LightpathDB::getNextId()
{
	// NB: need to consider wrap back
	return g_nNextLightpathId++;
}

UINT LightpathDB::peekNextId()
{
	return g_nNextLightpathId;
}

void LightpathDB::appendLightpath(Lightpath* pLightpath)
{
	assert(pLightpath);
	m_hLightpathList.push_front(pLightpath);
	m_hLightpathMap.insert(LightpathMapPair(pLightpath->getId(), m_hLightpathList.begin()));
}

void LightpathDB::removeLightpath(Lightpath* pLightpath)
{
#ifdef DEBUGB
	cout << "-> remove LightpathDB" << endl;
#endif // DEBUGB
	assert(pLightpath);
	LightpathMap::iterator itr = m_hLightpathMap.find(pLightpath->getId());
	if (itr == m_hLightpathMap.end()) 
		return;
	m_hLightpathList.erase(itr->second);
	m_hLightpathMap.erase(itr);
}

void LightpathDB::logPeriodical(Log &hLog, SimulationTime hTimeSpan)
{
	list<Lightpath*>::const_iterator itr;
	for (itr = m_hLightpathList.begin(); itr != m_hLightpathList.end(); itr++)
		(*itr)->logPeriodical(hLog, hTimeSpan);
}

//-B: for each lightpath, it logs: primary/backup hop distance
//	holding time, lightpath load, link load, tx load
void LightpathDB::logFinal(Log &hLog)
{
	list<Lightpath*>::const_iterator itr;
	for (itr = m_hLightpathList.begin(); itr != m_hLightpathList.end(); itr++)
		(*itr)->logFinal(hLog);
}
