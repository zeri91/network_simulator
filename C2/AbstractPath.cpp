#pragma warning(disable: 4786)
#include <assert.h>
#include <iostream>

#include "OchInc.h"
#include "OchObject.h"
#include "AbstractPath.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

///////////////////////////////////////////////////////////////////////////////
AbstractPath::AbstractPath()
{
}

AbstractPath::AbstractPath(const list<UINT>& hPath)
{
	*this = hPath;
}

AbstractPath::~AbstractPath()
{
}

AbstractPath::AbstractPath(const AbstractPath& rhs)
{
	*this = rhs;
}

const AbstractPath& AbstractPath::operator=(const AbstractPath& rhs)
{
	if (this == &rhs) return *this;
	m_hLinkList = rhs.m_hLinkList;
	m_nSrc = m_hLinkList.front();
	m_nDst = m_hLinkList.back();
	return *this;
}

const AbstractPath& AbstractPath::operator=(const list<UINT>& hPath)
{
	m_hLinkList = hPath;
	m_nSrc = hPath.front();
	m_nDst = hPath.back();
	return *this;
}

void AbstractPath::dump(ostream& out) const
{
	list<UINT>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		out.width(3);
		out<<*itr;
	}
//	out<<endl;
}

void AbstractPath::setSrc(UINT nSrc)
{
	m_nSrc = nSrc;
}

void AbstractPath::setDst(UINT nDst)
{
	m_nDst = nDst;
}

UINT AbstractPath::getSrc() const
{
	return m_nSrc;
}

UINT AbstractPath::getDst() const
{
	return m_nDst;
}

bool AbstractPath::containAbstractLink(UINT nSrc, UINT nDst) const
{
	list<UINT>::const_iterator itr = m_hLinkList.begin();
	while (itr != m_hLinkList.end()) {
		if (*itr == nSrc) {
			list<UINT>::const_iterator itrNext = itr;
			itrNext++;
			if (*itrNext == nDst)
				return true;
			else
				return false;	// NB: assume no circle
		}
		itr++;
	}
	return false;
}

bool AbstractPath::appendAbstractLink(UINT nSrc, UINT nDst)
{
	if (m_hLinkList.size() > 0) {
		UINT nLastHop = m_hLinkList.back();
		if (nLastHop != nSrc) return false;
	}
	m_hLinkList.push_back(nDst);
	
	// NB: may use a map to speed up searching

	return true;
}

void AbstractPath::deleteContent()
{
	m_hLinkList.clear();
}

UINT AbstractPath::getPhysicalHops() const
{
	UINT nNodes = m_hLinkList.size();
	if (0 == nNodes)
		return 0;
	else
		return (nNodes - 1);
}
