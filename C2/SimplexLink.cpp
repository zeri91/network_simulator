#pragma warning(disable: 4786)
#include <assert.h>
#include "OchInc.h"

#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "Vertex.h"
#include "UniFiber.h"
#include "AbstractPath.h"
#include "Lightpath.h"
#include "SimplexLink.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

bool NS_OCH::operator<(const SimplexLink::SimplexLinkKey& lhs, 
					   const SimplexLink::SimplexLinkKey& rhs)
{
	if (lhs.m_nKeyId < rhs.m_nKeyId)
		return true;
	else if (lhs.m_nKeyId > rhs.m_nKeyId)
		return false;
	
	if (lhs.m_eLType < rhs.m_eLType)
		return true;
	else if (lhs.m_eLType > rhs.m_eLType)
		return false;

	// can be equal cause < is called by find as well
//	assert(lhs.m_nChannel != rhs.m_nChannel);	 
	return (lhs.m_nChannel < rhs.m_nChannel);
}

SimplexLink::SimplexLinkKey::SimplexLinkKey(int nId, SimplexLinkType eLType, 
											int nChannel):
	m_nKeyId(nId), m_eLType(eLType), m_nChannel(nChannel)
{
}

SimplexLink::SimplexLinkKey::SimplexLinkKey(const SimplexLink& hSimplexLink):
	m_eLType(hSimplexLink.m_eSimplexLinkType), 
	m_nChannel(hSimplexLink.m_nChannel)
{
	if (hSimplexLink.m_pUniFiber)
		m_nKeyId = hSimplexLink.m_pUniFiber->getId();
	else
		m_nKeyId = -1;
}

SimplexLink::SimplexLinkKey::~SimplexLinkKey()
{
}

///////////////////////////////////////////////////////////////////////////////
//
SimplexLink::SimplexLink(int nLinkId, Vertex* pSrc, Vertex* pDst, 
						 LINK_COST hCost, float nLength,
						 UniFiber* pFiber, SimplexLinkType eType, int nChannel,
						 LINK_CAPACITY hCap):
	AbstractLink(nLinkId, pSrc, pDst, hCost, nLength),
	m_pUniFiber(pFiber), m_eSimplexLinkType(eType), m_nChannel(nChannel),
	m_hFreeCap(hCap), m_nBackupCap(0), m_nBWToBeAllocated(0)
{
	if (this->m_eSimplexLinkType == LT_Grooming || this->m_eSimplexLinkType == LT_Converter)
	{
		this->m_latency = ELSWITCHLATENCY;
	}
	/* -B: COMMENTED BUT ACTUALLY IT WORKS, ONLY IF this fiber (==> LT_Channel) has length > 0
		else if (this->m_eSimplexLinkType == LT_Channel || this->m_eSimplexLinkType == LT_Lightpath)
	{
		assert(this->m_latency > 0);
	}
	else
	{
		assert(this->m_latency == 0);
	}
	*/

	//nothing to do for other cases
}

SimplexLink::~SimplexLink()
{
}

void SimplexLink::dumpSL(ostream &out) const
{
#ifdef DEBUGB
	cout << "-> Simplex Link dump" << endl;
#endif // DEBUGB

//	if (m_hFreeCap > 0)
		cout << "  ->";
		cout.width(4);
		cout.flags((cout.flags() & ~ios::right) | ios::left);
		cout << m_pDst->m_nNodeId;
		cout.flags((cout.flags() & ~ios::left) | ios::right);
		cout << " ";
		cout.width(4);
		cout << m_nLinkId << "\tC=" << m_hCost;
		cout << "\tFC=" << m_hFreeCap;
		cout << "\tW=" << m_nChannel;
		if (m_bValid)
			cout << " VALID";
		else
			cout << " INVALID";
		cout << endl;

#ifdef _OCHDEBUG9
		if (m_pBackupCost) {
			out<<"\tCS=";
			out<<'{';
			int i;
			for (i=0; i<m_nBackupVHops; i++) {
				out.width(3);
				out<<m_pBackupCost[i];
				if (i<(m_nBackupVHops - 1))
					out<<", ";
			}
			out<<'}'<<endl;
		}
#endif
}

AbstractLink::LinkType SimplexLink::getLinkType() const
{
	return LT_Simplex;
}

SimplexLink::SimplexLinkType SimplexLink::getSimplexLinkType() const
{
	return m_eSimplexLinkType;
}

void SimplexLink::consumeBandwidth(UINT nBW)
{
	assert(m_hFreeCap >= nBW);
	consumeBandwidthHelper(0 - (int)nBW);
}

void SimplexLink::releaseBandwidth(UINT nBW)
{
	consumeBandwidthHelper(nBW);
}

void SimplexLink::consumeBandwidthHelper(int nBW)
{
#ifdef DEBUGC
	cout << "->consumeBandwidthHelper" << endl;
	cout << "\tSto rilasciando(+)/consumando(-) " << nBW << " sul simplex link corrispondente al canale " << this->m_nChannel << endl;
#endif // DEBUGB
	m_hFreeCap += nBW;
}

void SimplexLink::allocateConflictSet(UINT nSize)
{
	if (m_pCSet && (nSize != m_nCSetSize)) {
		delete []m_pCSet;
		m_pCSet = NULL;
	}
	if (NULL == m_pCSet) {
		m_pCSet = new UINT[m_nCSetSize = nSize];
		if (NULL == m_pCSet)
			THROW_OCH_EXEC("Out of memory!");
	}
	memset(m_pCSet, 0, nSize*sizeof(UINT));
}

void SimplexLink::extractNonAuxLinks(list<SimplexLink*>& hNewList,
									 const list<AbstractLink*>& hOldList)
{
	SimplexLink *pSLink;
	list<AbstractLink*>::const_iterator itr;
	for (itr=hOldList.begin(); itr!=hOldList.end(); itr++) {
		pSLink = (SimplexLink*)(*itr);
		assert(pSLink);
		switch (pSLink->m_eSimplexLinkType) {
		case LT_Channel:
			assert(false);	// to do, wavelength-continues case
			break;
		case LT_UniFiber:
		case LT_Lightpath:
			hNewList.push_back(pSLink);
			break;
		case LT_Channel_Bypass:
		case LT_Grooming:
		case LT_Mux:
		case LT_Demux:
		case LT_Tx:
		case LT_Rx:
		case LT_Converter:
			NULL;		// aux links
			break;
		default:
			DEFAULT_SWITCH;
		}
	}
}

bool SimplexLink::auxVirtualLink() const
{
	bool bAuxVLink = true;
	switch (m_eSimplexLinkType) {
	case LT_Channel:
	case LT_UniFiber:
	case LT_Lightpath:
		bAuxVLink = false;
		break;
	case LT_Channel_Bypass:
	case LT_Grooming:
	case LT_Mux:
	case LT_Demux:
	case LT_Tx:
	case LT_Rx:
	case LT_Converter:
		bAuxVLink = true;
		break;
	default:
		DEFAULT_SWITCH;
	}
	return bAuxVLink;
}
