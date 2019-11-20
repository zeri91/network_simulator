#pragma warning(disable: 4786)
#include <assert.h>
#include <iostream>

#include "OchInc.h"
#include "OchObject.h"

#include "MappedLinkList.h"
#include "BinaryHeap.h"
#include "AbsPath.h"
#include "AbstractGraph.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

AbsPath::AbsPath(): m_hCost(0), overlap(false)
{
}

AbsPath::AbsPath(int wl): m_hCost(0), overlap(false),wlAssigned(wl)
{
}


AbsPath::AbsPath(const list<AbstractLink*>& hList)
{
	m_hLinkList = hList;
	calculateCost();
}

AbsPath::~AbsPath()
{
}

AbsPath::AbsPath(const AbsPath& rhs)
{
	*this = rhs;
}

const AbsPath& AbsPath::operator=(const AbsPath& rhs)
{
	m_hLinkList = rhs.m_hLinkList;
	calculateCost();
	return (*this);
}

bool AbsPath::operator==(const AbsPath& rhs)const
{
	if (m_hCost != rhs.m_hCost) {
		return false;
	}
	if (m_hLinkList.size() != rhs.m_hLinkList.size()) {
		return false;
	}
	list<AbstractLink*>::const_iterator itr, itrRHS;
	for (itr=m_hLinkList.begin(), itrRHS=rhs.m_hLinkList.begin();
	((itr!=m_hLinkList.end()) && (itrRHS!=rhs.m_hLinkList.end()));
	itr++, itrRHS++) {
		if ((*itr) != (*itrRHS)) {
			return false;
		}
	}
	return true;
}

void AbsPath::dump(ostream& out) const
{
	out<<m_hCost<<": ";
	if (0 == m_hLinkList.size()) {
		out<<"NULL";
		return;
	}
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		out<<(*itr)->m_pSrc->m_nNodeId<<' ';
	}
	out<<m_hLinkList.back()->m_pDst->m_nNodeId<<endl;
}

LINK_COST AbsPath::calculateCost()
{
#ifdef DEBUGB
	cout << "-> calculateCost" << endl;
	cout << "\tLinklist abspath: " << m_hLinkList.size() << endl;
#endif // DEBUGB

	assert(m_hLinkList.size() > 0);
	m_hCost = 0;
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		m_hCost += (*itr)->getCost();
	}
#ifdef DEBUG
	cout << "\tCost: " <<m_hCost << endl;
#endif
	return m_hCost;
}

LINK_COST AbsPath::getCost() const
{
	return m_hCost;
}

void AbsPath::invalidate()
{
	list<AbstractLink*>::iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++)
		(*itr)->invalidate();
}
