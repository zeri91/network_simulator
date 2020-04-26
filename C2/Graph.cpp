#pragma warning(disable: 4786)
#include <assert.h>
#include <algorithm>
#include <random>
#include "OchInc.h"

#include "OchObject.h"
#include "MappedLinkList.h"
#include "BinaryHeap.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "AbstractPath.h"
#include "Vertex.h"
#include "UniFiber.h"
#include "SimplexLink.h"
#include "OXCNode.h"
#include "Lightpath.h"
#include "Circuit.h"
#include "Graph.h"
#include "OchMemDebug.h"
#include "Netman.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

Graph::Graph()
{
}

Graph::~Graph()
{
}

Graph::Graph(const Graph& rhs)
{
	*this = rhs;
}

const Graph& Graph::operator=(const Graph& rhs)
{
	if (&rhs == this) return (*this);

	// copy nodes
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=rhs.m_hNodeList.begin(); itrNode!=rhs.m_hNodeList.end(); 
	itrNode++) {
		Vertex *pVertex = (Vertex*)(*itrNode);
		assert(pVertex);
		bool bSuccess = this->addNode(new Vertex(pVertex->m_pOXCNode, 
			pVertex->getId(), pVertex->m_eVType, pVertex->m_nChannel));
		assert(bSuccess);
	}

	// copy links
	list<AbstractLink*>::const_iterator itrLink;
	for (itrLink=rhs.m_hLinkList.begin(); itrLink!=rhs.m_hLinkList.end();
	itrLink++) {
		SimplexLink *pLink = (SimplexLink*)(*itrLink);
		assert(pLink);
		Vertex *pSrc = (Vertex*)pLink->m_pSrc;
		Vertex *pDst = (Vertex*)pLink->m_pDst;
		assert(pSrc && pDst);
		SimplexLink *pNewLink = this->addSimplexLink(pLink->getId(), 
			pSrc->m_pOXCNode->getId(), pSrc->m_eVType, pSrc->m_nChannel,
			pDst->m_pOXCNode->getId(), pDst->m_eVType, pDst->m_nChannel,
			pLink->getCost(), pLink->getLength(), pLink->m_pUniFiber, 
			pLink->m_eSimplexLinkType, pLink->m_nChannel, pLink->m_hFreeCap);
		assert(pNewLink);
	}

	return (*this);
}

void Graph::dump(ostream& out) const
{
#ifdef DEBUGB
	cout << "Graph DUMP" << endl;
	cin.get();
#endif // DEBUGB
	out<<"Graph: Vertices = "<<m_hNodeList.size();
	out<<", SimplexLinks = "<<m_hLinkList.size();
	out<<", NextLinkId = "<<m_nNextLinkId;
	out<<endl;
	list<AbstractNode*>::const_iterator itr;
	for (itr = m_hNodeList.begin(); itr != m_hNodeList.end(); itr++) {
		//VERTEX DUMP (not AbstractNode dump!!!) (very long dump)
		(*itr)->dump(out);
		out<<endl;
	}
}

bool Graph::addNode(AbstractNode *pNode)
{
	Vertex *pVertex = (Vertex*)pNode;
	assert(pVertex);
	if (m_hVertexMap.end() != m_hVertexMap.find(Vertex::VertexKey(*pVertex)))
		return false;	// already exist

	if (!AbstractGraph::addNode(pNode))
		return false;

	m_hVertexMap.insert(VertexMapPair(Vertex::VertexKey(*pVertex), pVertex));
	return true;
}

SimplexLink* Graph::addSimplexLink(UINT nLinkId, 
				UINT nSrc, Vertex::VertexType eSrcType, int nSrcChannel,
				UINT nDst, Vertex::VertexType eDstType, int nDstChannel,
				LINK_COST hCost, float nLength,
				UniFiber* pFiber, SimplexLink::SimplexLinkType eType,
				int nChannel, LINK_CAPACITY hCap)
{
	int nFiberId = -1;
	if (pFiber) nFiberId = pFiber->getId();
	VertexMap::const_iterator hSrcIter = 
		m_hVertexMap.find(Vertex::VertexKey(nSrc, eSrcType, nSrcChannel));
	VertexMap::const_iterator hDstIter = 
		m_hVertexMap.find(Vertex::VertexKey(nDst, eDstType, nDstChannel));

	if ((hSrcIter == m_hVertexMap.end()) || (hDstIter == m_hVertexMap.end()))
		return NULL;
// NB: see what need to be store later
//	WBEs (TxE, or RxE) have the same triple
//	if (m_hSimplexLinkMap.end() != m_hSimplexLinkMap.find(
//		SimplexLink::SimplexLinkKey(nFiberId, eType, nChannel)))
//		return false;	// a link w/ same id exists

//	assert(pFiber);	// pFiber is NULL for GrmE, TxE, RxE, WBE, etc.
	SimplexLink *pLink = new SimplexLink(nLinkId, hSrcIter->second, 
		hDstIter->second, hCost, nLength, pFiber, eType, nChannel, hCap);
	if (!addLink(pLink)) {
		delete pLink;
		return NULL;
	}
	
//	m_hSimplexLinkMap.insert(SimplexLinkMapPair(
//		SimplexLink::SimplexLinkKey(nFiberId, eType, nChannel), pLink));
	return pLink;
}

SimplexLink* Graph::addSimplexLink(UINT nLinkId, Vertex *pSrc, Vertex *pDst,
			LINK_COST hCost, float nLength,
			UniFiber* pFiber, SimplexLink::SimplexLinkType eType,
			int nChannel, LINK_CAPACITY hCap)
{
	int nFiberId = -1;
	if (pFiber)
		nFiberId = pFiber->getId();

// NB: see what need to be store later
//	WBEs (TxE, or RxE) have the same triple
//	if (m_hSimplexLinkMap.end() != m_hSimplexLinkMap.find(
//		SimplexLink::SimplexLinkKey(nFiberId, eType, nChannel)))
//		return false;	// a link w/ same id exists

	SimplexLink *pLink = new SimplexLink(nLinkId, pSrc, 
		pDst, hCost, nLength, pFiber, eType, nChannel, hCap);
	if (!addLink(pLink))
		return NULL;
	
//	m_hSimplexLinkMap.insert(SimplexLinkMapPair(
//		SimplexLink::SimplexLinkKey(nFiberId, eType, nChannel), pLink));

	return pLink;
}


void Graph::deleteContent()
{
	AbstractGraph::deleteContent();

	m_hVertexMap.clear();
	m_hSimplexLinkMap.clear();
}

Vertex* Graph::lookUpVertex(UINT nOXCId, Vertex::VertexType eVType, 
							int nChannel) const
{
	VertexMap::const_iterator hIter = 
		m_hVertexMap.find(Vertex::VertexKey(nOXCId, eVType, nChannel));

	if (hIter == m_hVertexMap.end())
		return NULL;

	return (hIter->second);
}

// NB: may cause problems if there are multiple links between the same
//     node pair.
SimplexLink* Graph::lookUpSimplexLink(
			UINT nSrcOXC, Vertex::VertexType eSrcVType, int nSrcChannel,
			UINT nDstOXC, Vertex::VertexType eDstVType, int nDstChannel) const
{
	Vertex *pSrc = lookUpVertex(nSrcOXC, eSrcVType, nSrcChannel);
	Vertex *pDst = lookUpVertex(nDstOXC, eDstVType, nDstChannel);
	assert(pSrc && pDst);
	list<AbstractLink*>::const_iterator itr;
	SimplexLink* s;
	for (itr = pSrc->m_hOLinkList.begin(); itr != pSrc->m_hOLinkList.end(); itr++) {
		/*s = (SimplexLink*)(*itr);
		cout << "Link is from " << ((Vertex*)(s->getSrc()))->m_pOXCNode->getId()
			<< " to " << ((Vertex*)(s->getDst()))->m_pOXCNode->getId() << endl;
		cout << "Vertex " << ((Vertex*)(s->getSrc()))->m_pOXCNode->getId() << " of type " << ((Vertex*)(s->getSrc()))->m_eVType
			<< " ch: " << ((Vertex*)(s->getSrc()))->m_nChannel << endl;

		cout << "Vertex " << ((Vertex*)(s->getDst()))->m_pOXCNode->getId() << " of type " << ((Vertex*)(s->getDst()))->m_eVType
			<< " ch: " << ((Vertex*)(s->getDst()))->m_nChannel << endl;
			*/
		if ((Vertex*)(*itr)->getDst() == pDst)
			break;
	}
	assert(itr != pSrc->m_hOLinkList.end());
	return (SimplexLink*)(*itr);
}

// Invalidate those SimplexLinks that are in the same SRG as pUniFiber
// NB: here is ARC-DISJOINT
void Graph::invalidateSimplexLinkDueToSRG(const UniFiber* pUniFiber)
{
	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);

		switch (pSimplexLink->m_eSimplexLinkType)
		{
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_UniFiber:
				if (pSimplexLink->m_pUniFiber == pUniFiber)
					pSimplexLink->invalidate();
				break;
			case SimplexLink::LT_Lightpath:
				{
					assert(pSimplexLink->m_pLightpath);
					if (pSimplexLink->m_pLightpath->traverseUniFiber(pUniFiber))
						pSimplexLink->invalidate();
				}
				break;
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Converter:
				NULL;	// Nothing to do so far
				break;
			default:
				DEFAULT_SWITCH;
		}
	}
}


// Invalidate those SimplexLinks related to pUniFiber, for PAL_SPP
// NB: here is ARC-DISJOINT
void Graph::invalidateUniFiberSimplexLink(const UniFiber* pUniFiber)
{
	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);

		switch (pSimplexLink->m_eSimplexLinkType) {
		case SimplexLink::LT_Channel:
		case SimplexLink::LT_UniFiber:
			if (pSimplexLink->m_pUniFiber == pUniFiber)
				pSimplexLink->invalidate();
			break;
		case SimplexLink::LT_Lightpath:
		case SimplexLink::LT_Channel_Bypass:
		case SimplexLink::LT_Grooming:
		case SimplexLink::LT_Mux:
		case SimplexLink::LT_Demux:
		case SimplexLink::LT_Tx:
		case SimplexLink::LT_Rx:
		case SimplexLink::LT_Converter:
			NULL;	// Nothing to do so far
			break;
		default:
			DEFAULT_SWITCH;
		}
	}
}

// Invalidate those SimplexLinks that don't have enough bandwidth
void Graph::invalidateSimplexLinkDueToCap(UINT nBW)
{
#ifdef DEBUGB
	cout << "-> invalidateSimplexLinkDueToCap" << endl;
#endif // DEBUGB

	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;

	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);
		if (pSimplexLink->getValidity())
		{
			if (pSimplexLink->m_hFreeCap < nBW)
			{
				pSimplexLink->invalidate();
				//cout << " " << pSimplexLink->m_nChannel << "(type " << pSimplexLink->getSimplexLinkType() << ")";
			}
		}
	}
#ifdef DEBUGB
	cout << endl;
#endif // DEBUGB
}

//-B: validate those links that have a free capacity > residualBWD, but were invalidated because their free capacity was < OCLightpath
void Graph::validateSimplexLinkDueToCap(BandwidthGranularity residualBWD)
{
#ifdef DEBUGB
	cout << "-> VALIDATESimplexLinkDueToCap" << endl;
	//cout << "\tVALIDO i ch:";
#endif // DEBUGB

	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;

	//-B: scann all simplex links of the graph
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);
		//-B: only consider LT_Lightpath simplex links (neither LT_Channel, neither other types: 
		//	LT_Channel simplex links with at least a connection should remain invalid)
		if (pSimplexLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
		{
			//if it was invalidated
			if (!(pSimplexLink->getValidity()))
			{
				if (pSimplexLink->m_hFreeCap < OCLightpath && pSimplexLink->m_hFreeCap >= residualBWD)
				{
					pSimplexLink->validate();
					//cout << " " << pSimplexLink->m_nChannel << "(type " << pSimplexLink->getSimplexLinkType() << ")";
				}
			}
		}
	}
#ifdef DEBUGB
	cout << endl;
#endif // DEBUGB
}

void Graph::invalidateSimplexLinkLightpath()
{
	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pSLink;

	for (itr = this->m_hLinkList.begin(); itr != this->m_hLinkList.end(); itr++)
	{
		pSLink = (SimplexLink*)(*itr);
		if (pSLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
		{
			if (pSLink->m_pLightpath->getId() == 0)
			{
				pSLink->invalidate();
			}
		}
	}
}

// Invalidate those SimplexLinks that don't have enough bandwidth
void Graph::invalidateSimplexLinkDueToPath(list<AbstractLink*>pPath)
{
#ifdef DEBUGB
	cout << "-> invalidateSimplexLinkDueToPath" << endl;
	cout << "\tInvalido i ch:";
#endif // DEBUGB

	SimplexLink *pSimplexLink, *pLink;
	list<AbstractLink*>::iterator itr1, itr2;

	//-B: scan all simplex link of the path computed in the previous iteration
	for (itr1 = pPath.begin(); itr1 != pPath.end(); itr1++)
	{
		pLink = (SimplexLink*)(*itr1);
		//consider only those simplex link having as type LT_Channel or LT_Lightpath (or LT_UniFiber)
		switch (pLink->m_eSimplexLinkType)
		{
			case SimplexLink::SimplexLinkType::LT_UniFiber: //-B: not in my case
			case SimplexLink::SimplexLinkType::LT_Channel:
			case SimplexLink::SimplexLinkType::LT_Lightpath:
			{
				//-B: scan all the simplex links of the graph
				for (itr2 = m_hLinkList.begin(); itr2 != m_hLinkList.end(); itr2++)
				{
					pSimplexLink = (SimplexLink*)(*itr2);
					assert(pSimplexLink);
					//if the selected simplex link is valid
					if (pSimplexLink->getValidity())
					{
						//if it is the same selected link
						if (pSimplexLink->getId() == pLink->getId())
						{
#ifdef DEBUGB
							cout << ((Vertex*)(pSimplexLink->getSrc()))->m_pOXCNode->getId() << "->"
								<< ((Vertex*)(pSimplexLink->getDst()))->m_pOXCNode->getId() << " ";
#endif // DEBUGB
							//invalidate this link, so that it won't be considered in the next Dijkstra
							pSimplexLink->invalidate();
							break;
						}
					}
				} //end FOR
				break;
			}
			default:
			{
				NULL;
				break;
			}
		} //end SWITCH
	} //end FOR

#ifdef DEBUGB
	cout << endl;
#endif // DEBUGB
}


//-B: some lightpaths could have been created during connection provisioning, but afterwards it happened
//	that the connection provisioning cannot be done completely, for the total amount of its bandwidth 
//	(used for connection request requiring a bandwidth > OCLightpath, that need more than a circuit)
//	(lightpaths are not appended to the lightpath db till when it's performed unprotected_setUpCircuit, so no problem about that)
void Graph::removeSimplexLinkLightpathNotUsed()
{
#ifdef DEBUGB
	cout << "-> removeSimplexLinkLightpathNotUsed" << endl;
	cout << "\tRimuovo i lightpath: ";
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pSLink;
	UINT graphSize;

	for (itr = this->m_hLinkList.begin(); itr != this->m_hLinkList.end(); itr++)
	{
		pSLink = (SimplexLink*)(*itr);
		if (pSLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
		{
			if (pSLink->m_hFreeCap == OCLightpath)
			{
#ifdef DEBUGB
				cout << "simplex link " << ((Vertex*)(pSLink->getSrc()))->m_pOXCNode->getId() << "->"
					<< ((Vertex*)(pSLink->getDst()))->m_pOXCNode->getId() << " associato al lightpath "
					<< pSLink->m_pLightpath->getId() << " ";
#endif // DEBUGB
				assert(pSLink->m_pLightpath->getId() == 0);
				graphSize = this->m_hLinkList.size();
				this->removeLink(pSLink);
				assert((graphSize - 1) == this->m_hLinkList.size()); //just to check
				itr = this->m_hLinkList.begin();
			}
		}
	}
}

// release OCLightpath to all the physical links used by pLightpath
void Graph::releaseLightpathBandwidth(const Lightpath *pLightpath)
{
	SimplexLink *pSimplexLink;
	list<UniFiber*>::const_iterator itrUniFiber;
	for (itrUniFiber = pLightpath->m_hPRoute.begin(); itrUniFiber != pLightpath->m_hPRoute.end(); itrUniFiber++) {
		list<AbstractLink*>::iterator itr;
		for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++) {
			pSimplexLink = (SimplexLink*)(*itr);
			assert(pSimplexLink);
			if (pSimplexLink->m_pUniFiber == (*itrUniFiber))
				pSimplexLink->releaseBandwidth(pLightpath->m_nCapacity);
		}
	}
}

void Graph::computeCutSet(map<UINT, OXCNode*>& hCutSetSrc) const
{
	list<AbstractNode*>::const_iterator itr;
	for (itr=m_hNodeList.begin(); itr!=m_hNodeList.end(); itr++) {
		// NB: here assume shortest-path computation uses m_pPrevLink
		if ((*itr)->m_pPrevLink) {	// reachable
			Vertex *pVertex = (Vertex*)(*itr);
			assert(pVertex);
			if (hCutSetSrc.find(pVertex->m_pOXCNode->getId()) !=
				hCutSetSrc.end()) continue;	// already added
			hCutSetSrc.insert(pair<UINT, OXCNode*>(
				pVertex->m_pOXCNode->getId(), pVertex->m_pOXCNode));
		}
	}
}

void Graph::modifyBackHaulLinkCost(const UniFiber* pUniFiber, LINK_COST hCost)
{
	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);

		switch (pSimplexLink->m_eSimplexLinkType) {
		case SimplexLink::LT_Channel:
		case SimplexLink::LT_UniFiber:
			if (pSimplexLink->m_pUniFiber == pUniFiber)
				pSimplexLink->modifyCost(hCost);
			break;
		case SimplexLink::LT_Lightpath:
			{
				assert(pSimplexLink->m_pLightpath);
				if (pSimplexLink->m_pLightpath->traverseUniFiber(pUniFiber))
					pSimplexLink->modifyCost(hCost);
			}
			break;
		case SimplexLink::LT_Channel_Bypass:
		case SimplexLink::LT_Grooming:
		case SimplexLink::LT_Mux:
		case SimplexLink::LT_Demux:
		case SimplexLink::LT_Tx:
		case SimplexLink::LT_Rx:
		case SimplexLink::LT_Converter:
			NULL;	// Nothing to do so far
			break;
		default:
			DEFAULT_SWITCH;
		}
	}
}

void Graph::PAL_SPP_invalidateSimplexLinkDueToTxRx()
{
	SimplexLink *pSLink;
	OXCNode *pOXC;
	list<AbstractLink*>::iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pSLink = (SimplexLink*)(*itr);
		assert(pSLink);
		pOXC = ((Vertex*)pSLink->m_pSrc)->m_pOXCNode;
		assert(pOXC);
		switch (pSLink->getSimplexLinkType()) {
		case SimplexLink::LT_Tx:
			if (pOXC->m_nFreeTx < 2)
				pSLink->invalidate();
			break;
		case SimplexLink::LT_Rx:
			if (pOXC->m_nFreeRx < 2)
				pSLink->invalidate();
			break;
		default:
			NULL;
		}
	}
}

void Graph::allocateConflictSet(UINT nSize)
{
	SimplexLink *pSLink;
	list<AbstractLink*>::iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pSLink = (SimplexLink*)(*itr);
		assert(pSLink);
		if (SimplexLink::LT_UniFiber == pSLink->getSimplexLinkType()) {
			pSLink->allocateConflictSet(nSize);
		}
	}
}

// compute K-shortest loopless paths
void Graph::Yen(list<AbsPath*>& hKPaths, 
				UINT nSrc, UINT nDst, 
				UINT nNumberOfPaths, NetMan *m_pNetman, LinkCostFunction hLCF, double latency)
{
	AbstractNode *pSrc = m_hNodeList.find(nSrc);
	AbstractNode *pDst = m_hNodeList.find(nDst);
	if ((NULL == pSrc) || (NULL == pDst))
		return;

	//this->YenHelper(hKPaths, pSrc, pDst, nNumberOfPaths, m_pNetman, hLCF);
}

void Graph::Yen(list<AbsPath*>& hKPaths, 
				AbstractNode* pSrc, OXCNode* pOXCsrc, AbstractNode* pDst, 
				UINT nNumberOfPaths, NetMan *m_pNetman, LinkCostFunction hLCF, double latency)
{
	this->YenHelper(hKPaths, pSrc, pOXCsrc, pDst, nNumberOfPaths, m_pNetman, hLCF, latency);
}

inline void Graph::YenHelper(list<AbsPath*>& hKPaths, 
							 AbstractNode* pSrc, OXCNode* pOXCsrc, AbstractNode* pDst,
							 UINT nNumberOfPaths, NetMan *m_pNetman, LinkCostFunction hLCF, double lat)
{
	BinaryHeap<AbsPath*, vector<AbsPath*>, PAbsPathComp> hPQ;
	LINK_COST hCost;
	list<AbstractLink*> hMinCostPath;
	list<AbsPath*> hKPathsTmp;
	bool pathToAdd = true;

	DijkstraHelperLatency(pSrc, pDst, hLCF, lat);
	hCost = recordMinCostPath(hMinCostPath, pDst);
	if (UNREACHABLE == hCost || hCost == 0)
		return;	// graph disconnected
	hPQ.insert(new AbsPath(hMinCostPath));


	list<AbstractLink*>::const_iterator itrSeg, itr;
	AbsPath *pCurrentPath;
	UINT k = 0;
	while (k<nNumberOfPaths)
	{
		pCurrentPath = hPQ.peekMin();
		hPQ.popMin();

		pathToAdd = true;

		SimplexLink *pSLinkTemp;
		list<AbstractLink*>::const_iterator itrTemp;
#ifdef DEBUGB
		cout << "CURRENT PATH:" << endl;
		for (itrTemp = pCurrentPath->m_hLinkList.begin();
			itrTemp != pCurrentPath->m_hLinkList.end(); itrTemp++) {
			pSLinkTemp = (SimplexLink*)(*itrTemp);
			if (pSLinkTemp->getSimplexLinkType() == SimplexLink::LT_Channel || pSLinkTemp->getSimplexLinkType() == SimplexLink::LT_UniFiber) {
				cout << "Fiber from " << ((Vertex*)pSLinkTemp->getSrc())->m_pOXCNode->getId() << " to " << ((Vertex*)pSLinkTemp->getDst())->m_pOXCNode->getId() 
					<< " on ch " << pSLinkTemp->m_nChannel << endl;
			}
			else if (pSLinkTemp->getSimplexLinkType() == SimplexLink::LT_Lightpath) {
				cout << "Lightpath from " << ((Vertex*)pSLinkTemp->getSrc())->m_pOXCNode->getId() << " to " << ((Vertex*)pSLinkTemp->getDst())->m_pOXCNode->getId()
					<< " on ch " << pSLinkTemp->m_nChannel << endl;
			}
		
		}
#endif
		//If the path just computed doesn't exceed the latency and the source has no BBU assigned, I can return
		// If the path respects the latency constraint, but the source has already a BBU assigned, I have to check if the capacity
		// towards the new candidate BBU is enough.
		float latency = m_pNetman->computeLatencyP3(pCurrentPath->m_hLinkList, (Vertex*)pSrc, 1);
		if (latency < lat) {
			if (!m_pNetman->isLatencyExceeded()) {

				if (pOXCsrc->m_nBBUNodeIdsAssigned > 0 && pOXCsrc->m_nBBUNodeIdsAssigned != ((Vertex*)pDst)->m_pOXCNode->getId()) {
				
					bool capacityIsEnough = m_pNetman->verifyCapacityNew(pOXCsrc, pCurrentPath->m_hLinkList, BWDGRANULARITY);
					if (capacityIsEnough) {
#ifdef DEBUG
						cout << "Capacity is enough" << endl;
#endif
						hKPaths.clear();
						hKPaths.push_back(pCurrentPath);
						break;
					}
					else {
						pathToAdd = false;
#ifdef DEBUG
						cout << "Capacity is not enough; look for another path" << endl;

#endif // DEBUG
					}
				}
				else {
					hKPaths.clear();
					hKPaths.push_back(pCurrentPath);
					break;
				}
			}
			else {
				pathToAdd = false;
#ifdef  DEBUG
				cout << "A connection on the grooming path exceeds the latency" << endl;
#endif //  DEBUG

			}
		}
		else {
			pathToAdd = false;
#ifdef DEBUG
			cout << "Latency on this path is exceeded" << endl;
#endif
		}

		m_pNetman->resetGrooming();
		

		if ((k + 1) == nNumberOfPaths) {
			break;
		}
		// optimization for nNumberOfPaths == 3
		if ((3 == nNumberOfPaths) && (1 == k)) {
			if (!hPQ.empty()) {
				if (pCurrentPath->getCost() == hPQ.peekMin()->getCost()) {
					hKPaths.push_back(pCurrentPath);
					hKPaths.push_back(hPQ.peekMin());
					hPQ.popMin();
					break;
				}
			}
		}


		list<AbstractLink*> hFirstSeg, hSecondSeg;
		SimplexLink *pSLink;
		for (itr = pCurrentPath->m_hLinkList.begin();
			itr != pCurrentPath->m_hLinkList.end(); itr++) {
			pSLink = (SimplexLink*)(*itr);
			assert(pSLink);

			if (pSLink->auxVirtualLink()) {
#ifdef DEBUGX
				cout << "Before continue: Link from " << ((Vertex*)pSLink->getSrc())->m_pOXCNode->getId() << " to " << ((Vertex*)pSLink->getDst())->m_pOXCNode->getId() <<
					" of type " << pSLink->getSimplexLinkType() << " on ch: " << pSLink->m_nChannel << endl;
#endif
				hFirstSeg.push_back(*itr);
				continue;
			}
			// NB: to do for wavelength-continuous case

			list<AbstractNode*> hDirtyNodes;	// nodes to invalidate/validate
			list<AbstractLink*> hDirtyLinks;
			SimplexLink* pSLinkSeg;
			// disable first segment nodes to ensure loopless
			for (itrSeg = hFirstSeg.begin(); itrSeg != hFirstSeg.end(); itrSeg++) {
				pSLinkSeg = (SimplexLink*)(*itrSeg);
				if (!((SimplexLink*)(*itrSeg))->auxVirtualLink()) {
					invalidateVNode(hDirtyNodes, (Vertex*)(pSLinkSeg)->m_pSrc, pSLinkSeg->m_nChannel);
#ifdef DEBUGX
					cout << "Source invalidated: " << ((Vertex*)pSLinkSeg->getSrc())->m_pOXCNode->getId() << endl;
					cout << "Link from " << ((Vertex*)pSLinkSeg->getSrc())->m_pOXCNode->getId() << " to " << ((Vertex*)pSLinkSeg->getDst())->m_pOXCNode->getId() << 
						" of type " << pSLinkSeg->getSimplexLinkType() << " on ch: " << pSLinkSeg->m_nChannel << endl;
#endif
				}
			}
			// no branch
			invalidateVLink(hDirtyLinks, pSLink);
#ifdef DEBUGX
			cout << "Link invalidated from " << ((Vertex*)pSLink->getSrc())->m_pOXCNode->getId() << " to " << ((Vertex*)pSLink->getDst())->m_pOXCNode->getId() <<
				" of type " << pSLink->getSimplexLinkType() << " on ch: " << pSLink->m_nChannel << endl;
			cout << "invalidateVLink done" << endl;
#endif		
			
			list<AbsPath*>::const_iterator itrKPaths;
			for (itrKPaths = hKPathsTmp.begin(); itrKPaths != hKPathsTmp.end();
				itrKPaths++) {
				bool bSameRoot = true;
				list<AbstractLink*>::const_iterator itrK;
				for (itrSeg = hFirstSeg.begin(),
					itrK = (*itrKPaths)->m_hLinkList.begin();
					(itrSeg != hFirstSeg.end()) &&
					(itrK != (*itrKPaths)->m_hLinkList.end()) && bSameRoot;
					itrSeg++, itrK++) {
					if ((*itrSeg) != (*itrK))
						bSameRoot = false;
				}
				if (bSameRoot && (itrSeg == hFirstSeg.end())
					&& (itrK != (*itrKPaths)->m_hLinkList.end())) {
					
					if (((SimplexLink*)(*itrK))->getSimplexLinkType() == SimplexLink::LT_Channel || ((SimplexLink*)(*itrK))->getSimplexLinkType() == SimplexLink::LT_Lightpath)
					{
						invalidateVLink(hDirtyLinks, (SimplexLink*)(*itrK));
					}
				}
			}

			UINT idSrc = ((Vertex*)(*itr)->getSrc())->m_pOXCNode->getId();
			Vertex* newSrc = lookUpVertex(idSrc, Vertex::VT_Access_Out, -1);	//-B: ATTENTION!!! VT_Access_In

			DijkstraHelperLatency(newSrc, pDst, hLCF, lat);
			// enable nodes/links
			list<AbstractNode*>::const_iterator itrDNodes;
			list<AbstractLink*>::const_iterator itrDLinks;
			for (itrDNodes = hDirtyNodes.begin(); itrDNodes != hDirtyNodes.end();
				itrDNodes++) {
				(*itrDNodes)->validate();
			}
			for (itrDLinks = hDirtyLinks.begin(); itrDLinks != hDirtyLinks.end();
				itrDLinks++) {
				(*itrDLinks)->validate();
			}


			hCost = recordMinCostPath(hSecondSeg, pDst);

			if (UNREACHABLE != hCost) {
				AbsPath *pPath = new AbsPath();
				pPath->m_hLinkList = hFirstSeg;
				pPath->m_hLinkList.splice(pPath->m_hLinkList.end(), hSecondSeg);
				pPath->calculateCost();

				// insert to binary heap
				hPQ.insert(pPath);

			}

			hFirstSeg.push_back(*itr);
		} // for itr

		
		hKPathsTmp.push_back(pCurrentPath);
		

		k++;
		if (hPQ.empty())
			break;	// done
	}

	// release memory
	vector<AbsPath*>::const_iterator itrTmp;
	for (itrTmp = hPQ.m_hContainer.begin(); itrTmp != hPQ.m_hContainer.end();
		itrTmp++) {
		delete (*itrTmp);
	}
}

void Graph::invalidateVNode(list<AbstractNode*>& hNodeList, 
							Vertex* pVertex, int channel)
{
	assert(pVertex);

	VertexMap::const_iterator itr;
	UINT nOXC = pVertex->m_pOXCNode->getId();
	
	itr = m_hVertexMap.find(Vertex::VertexKey(nOXC, Vertex::VT_Access_In, -1));
	assert(itr != m_hVertexMap.end());
	itr->second->invalidate();
	hNodeList.push_back(itr->second);
	itr = m_hVertexMap.find(Vertex::VertexKey(nOXC, Vertex::VT_Access_Out, -1));
	assert(itr != m_hVertexMap.end());
	itr->second->invalidate();
	hNodeList.push_back(itr->second);
	
	itr = m_hVertexMap.find(Vertex::VertexKey(nOXC, Vertex::VT_Lightpath_In, -1));
	assert(itr != m_hVertexMap.end());
	itr->second->invalidate();
	hNodeList.push_back(itr->second);
	itr = m_hVertexMap.find(Vertex::VertexKey(nOXC, Vertex::VT_Lightpath_Out, -1));
	assert(itr != m_hVertexMap.end());
	itr->second->invalidate();
	hNodeList.push_back(itr->second);
	
	for (int w = 0; w < 8; w++) {
		itr = m_hVertexMap.find(Vertex::VertexKey(nOXC, Vertex::VT_Channel_In, w));
		assert(itr != m_hVertexMap.end());
		itr->second->invalidate();
		hNodeList.push_back(itr->second);
		itr = m_hVertexMap.find(Vertex::VertexKey(nOXC, Vertex::VT_Channel_Out, w));
		assert(itr != m_hVertexMap.end());
		itr->second->invalidate();
		hNodeList.push_back(itr->second);
	}
	
	
}


void Graph::invalidateVLink(list<AbstractLink*>& hLinkList, 
							SimplexLink* pSLink)
{
	assert(pSLink);

	pSLink->invalidate();

	if (SimplexLink::LT_UniFiber == pSLink->getSimplexLinkType() || SimplexLink::LT_Channel == pSLink->getSimplexLinkType()) {
		hLinkList.push_back(pSLink);
		return;
	}

	assert(SimplexLink::LT_Lightpath == pSLink->getSimplexLinkType());

	
	Vertex *pLPOutVertex = (Vertex*)(pSLink->getSrc());

	//cout << "Lightpath src: " << pLPOutVertex->m_pOXCNode->getId() << endl;

	assert(pLPOutVertex);
	Lightpath *pTargetLP = pSLink->m_pLightpath;
	assert(pTargetLP);
	UINT nTargetHops = pTargetLP->m_hPRoute.size();

	Lightpath *pLP;
	list<AbstractLink*>::const_iterator itr;
	SimplexLink* pL;
	for (itr=pLPOutVertex->m_hOLinkList.begin();
	itr!=pLPOutVertex->m_hOLinkList.end(); itr++) {
		pL = (SimplexLink*)(*itr);
		if (((SimplexLink*)(*itr))->getSimplexLinkType() != SimplexLink::LT_Lightpath) {
#ifdef DEBUGX
			cout << "Link is not a lightpath; it's of type: " << ((SimplexLink*)(*itr))->getSimplexLinkType() << endl;
			cout << "\tit's from " << ((Vertex*)pL->getSrc())->m_pOXCNode->getId() << " to " << ((Vertex*)pL->getDst())->m_pOXCNode->getId() 
				<< endl;
#endif
			continue;
		}

		if (!(*itr)->valid())
			continue;
		
		pLP = ((SimplexLink*)(*itr))->m_pLightpath;
#ifdef DEBUGX
		cout << "Lightpath from " << pLP->getSrc()->getId() << " to " << pLP->getSrc()->getId() << endl;
#endif			
		assert(pLP);
		if (pLP->m_hPRoute.size() == nTargetHops) {
			bool bToInvalidate = true;
			list<UniFiber*>::const_iterator itrCandidate, itrTarget;
			for (itrTarget=pTargetLP->m_hPRoute.begin(), 
				itrCandidate=pLP->m_hPRoute.begin();
			(itrTarget!=pTargetLP->m_hPRoute.end()) &&
			(itrCandidate!=pLP->m_hPRoute.end());
			itrTarget++, itrCandidate++) {
				if ((*itrTarget) != (*itrCandidate)) {
					bToInvalidate = false;
					break;
				}
			}
			if (bToInvalidate) {
				(*itr)->invalidate();
				hLinkList.push_back(*itr);
			}
		}
	}
}

void Graph::MPAC_Opt_ComputePrimaryGivenBackup(list<AbstractLink*>& hPCandidate,
											   LINK_COST& hPCandidateCost, 
											   LINK_COST& hBCandidateCost, 
											   const Circuit& hBCircuit, 
											   Vertex* pSrc, 
											   Vertex* pDst)
{
	UINT nBackupVHops = hBCircuit.getLightpathHops();
	MPAC_Opt_ComputePrimaryGivenBackup_Aux(hBCircuit, pSrc, pDst);

	hPCandidateCost = pDst->m_hCost;
	hBCandidateCost = 0;
	assert(pDst->m_pBackupCost);
	UINT  h;
	for (h=0; h<nBackupVHops; h++)
		hBCandidateCost += pDst->m_pBackupCost[h];

	recordMinCostPath(hPCandidate, pDst);
	assert(UNREACHABLE != hPCandidateCost);
}

inline void
Graph::MPAC_Opt_ComputePrimaryGivenBackup_Aux(const Circuit& hBCircuit, 
											  Vertex* pSrc,
											  Vertex* pDst)
{
	UINT nBackupVHops = hBCircuit.getLightpathHops();

	reset4ShortestPathComputation();
	pSrc->m_hCost = 0;
	pSrc->m_nBackupVHops = nBackupVHops;
	pSrc->m_pBackupCost = new LINK_COST[nBackupVHops];
	memset(pSrc->m_pBackupCost, 0, nBackupVHops*sizeof(LINK_COST));

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbsNodeComp4MPAC> hPQ;
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=m_hNodeList.begin(); itrNode!=m_hNodeList.end(); itrNode++) {
//		if ((*itrNode)->valid())
			hPQ.insert(*itrNode);
	}

	AbstractNode *pU, *pV;
	list<AbstractLink*>::const_iterator itr;
	while (!hPQ.empty()) {
		pU = hPQ.peekMin();
		hPQ.popMin();
		if (UNREACHABLE == pU->m_hCost)
			break;
		if (pDst == pU)
			break;	// done

		for (itr=pU->m_hOLinkList.begin(); itr!=pU->m_hOLinkList.end(); itr++) 
		{
			LINK_COST hLinkCost = (*itr)->getCost();
			pV = (*itr)->m_pDst;
			// the link has been invalidated
			if ((!(*itr)->valid()) || (UNREACHABLE == hLinkCost))
				continue;

			bool bAuxLink;
			SimplexLink *pSLink = (SimplexLink*)(*itr);
			assert(pSLink);
			switch (pSLink->getSimplexLinkType()) {
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Lightpath:
				bAuxLink = false; 
				break;
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Converter:
				bAuxLink = true;
				break;
			default:
				DEFAULT_SWITCH;
			}

			LINK_COST hLHSCost, hRHSCost;
			hLHSCost = pU->m_hCost + hLinkCost;
			UINT  h;
			if (bAuxLink) {
				for (h=0; h<nBackupVHops; h++)
					hLHSCost += pU->m_pBackupCost[h];
			} else {
				for (h=0; h<nBackupVHops; h++) {
					assert(UNREACHABLE != (*itr)->m_pBackupCost[h]);
					hLHSCost += 
						((pU->m_pBackupCost[h] > (*itr)->m_pBackupCost[h])?
						pU->m_pBackupCost[h]: (*itr)->m_pBackupCost[h]);
				}
			}

			if ((UNREACHABLE == pV->m_hCost) 
				|| (NULL == pV->m_pBackupCost)) {
				hRHSCost = UNREACHABLE;
			} else {
				hRHSCost = pV->m_hCost;
				for (h=0; h<nBackupVHops; h++)
					hRHSCost += pV->m_pBackupCost[h];
			}

			if ((hRHSCost > hLHSCost) 
				|| ((hRHSCost == hLHSCost) 
				    && (pV->m_hCost > (pU->m_hCost + hLinkCost)))) {
				pV->m_hCost = pU->m_hCost + hLinkCost;
				if (NULL == pV->m_pBackupCost) {
					// used in PAbsNodeComp4MPAC
					pV->m_nBackupVHops = nBackupVHops;	
					pV->m_pBackupCost = new LINK_COST[nBackupVHops];
				}
				if (bAuxLink) {
					for (h=0; h<nBackupVHops; h++)
						pV->m_pBackupCost[h] = pU->m_pBackupCost[h];
				} else {
					for (h=0; h<nBackupVHops; h++) {
						pV->m_pBackupCost[h] =
						((pU->m_pBackupCost[h] > (*itr)->m_pBackupCost[h])?
						pU->m_pBackupCost[h]: (*itr)->m_pBackupCost[h]);
					}
				}
				// use previous link instead of node
				pV->m_pPrevLink = *itr;	
			}
		} // for
		hPQ.buildHeap();
	} // while

/*
	AbstractNode *pU, *pV;
	list<AbstractLink*>::const_iterator itr;
	int nNodes = getNumberOfNodes();
	assert(nNodes>0);
	while (nNodes-->0) {
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			pU = (*itr)->m_pSrc;
			pV = (*itr)->m_pDst;
			assert(pU && pV);
			LINK_COST hLinkCost = (*itr)->getCost();
			if ((UNREACHABLE == pU->m_hCost) 
				|| (!(*itr)->valid()) 
				|| (UNREACHABLE == hLinkCost))
				continue;

			bool bAuxLink;
			SimplexLink *pSLink = (SimplexLink*)(*itr);
			assert(pSLink);
			switch (pSLink->getSimplexLinkType()) {
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Lightpath:
				bAuxLink = false; 
				break;
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Converter:
				bAuxLink = true;
				break;
			default:
				DEFAULT_SWITCH;
			}

			LINK_COST hLHSCost, hRHSCost;
			hLHSCost = pU->m_hCost + hLinkCost;
			int h;
			if (bAuxLink) {
				for (h=0; h<nBackupVHops; h++)
					hLHSCost += pU->m_pBackupCost[h];
			} else {
				for (h=0; h<nBackupVHops; h++) {
					assert(UNREACHABLE != (*itr)->m_pBackupCost[h]);
					hLHSCost += 
						((pU->m_pBackupCost[h] > (*itr)->m_pBackupCost[h])?
						pU->m_pBackupCost[h]: (*itr)->m_pBackupCost[h]);
				}
			}

			if ((UNREACHABLE == pV->m_hCost) 
				|| (NULL == pV->m_pBackupCost)) {
				hRHSCost = UNREACHABLE;
			} else {
				hRHSCost = pV->m_hCost;
				for (h=0; h<nBackupVHops; h++)
					hRHSCost += pV->m_pBackupCost[h];
			}

			if ((hRHSCost > hLHSCost) 
				|| ((hRHSCost == hLHSCost) 
				    && (pV->m_hCost > (pU->m_hCost + hLinkCost)))) {
				pV->m_hCost = pU->m_hCost + hLinkCost;
				if (NULL == pV->m_pBackupCost) {
					// used in PAbsNodeComp4MPAC
					pV->m_nBackupVHops = nBackupVHops;	
					pV->m_pBackupCost = new LINK_COST[nBackupVHops];
				}
				if (bAuxLink) {
					for (h=0; h<nBackupVHops; h++)
						pV->m_pBackupCost[h] = pU->m_pBackupCost[h];
				} else {
					for (h=0; h<nBackupVHops; h++) {
						pV->m_pBackupCost[h] =
						((pU->m_pBackupCost[h] > (*itr)->m_pBackupCost[h])?
						pU->m_pBackupCost[h]: (*itr)->m_pBackupCost[h]);
					}
				}
				// use previous link instead of node
				pV->m_pPrevLink = *itr;	
			}
		} // for
	} // while
*/
}

void Graph::SPAC_Opt_ComputePrimaryGivenBackup(list<AbstractLink*>& hPCandidate,
											   LINK_COST& hPCandidateCost, 
											   LINK_COST& hBCandidateCost, 
											   const list<AbstractLink*>& hBPath, 
											   Vertex* pSrc, 
											   Vertex* pDst)
{
	UINT nBackupVHops = hBPath.size();
	SPAC_Opt_ComputePrimaryGivenBackup_Aux(hBPath, pSrc, pDst);

	hPCandidateCost = pDst->m_hCost;
	hBCandidateCost = 0;
	assert(pDst->m_pBackupCost);
	UINT  h;
	for (h=0; h<nBackupVHops; h++)
		hBCandidateCost += pDst->m_pBackupCost[h];

	recordMinCostPath(hPCandidate, pDst);
	assert(UNREACHABLE != hPCandidateCost);
}

inline void
Graph::SPAC_Opt_ComputePrimaryGivenBackup_Aux(const list<AbstractLink*>& hBPath, 
											  Vertex* pSrc,
											  Vertex* pDst)
{
	UINT nBackupVHops = hBPath.size();

	reset4ShortestPathComputation();
	pSrc->m_hCost = 0;
	pSrc->m_nBackupVHops = nBackupVHops;
	pSrc->m_pBackupCost = new LINK_COST[nBackupVHops];
	memset(pSrc->m_pBackupCost, 0, nBackupVHops*sizeof(LINK_COST));

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbsNodeComp4MPAC> hPQ;
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=m_hNodeList.begin(); itrNode!=m_hNodeList.end(); itrNode++) {
//		if ((*itrNode)->valid())
			hPQ.insert(*itrNode);
	}

	AbstractNode *pU, *pV;
	list<AbstractLink*>::const_iterator itr;
	while (!hPQ.empty()) {
		pU = hPQ.peekMin();
		hPQ.popMin();
		if (UNREACHABLE == pU->m_hCost)
			break;
		if (pDst == pU)
			break;	// done

		for (itr=pU->m_hOLinkList.begin(); itr!=pU->m_hOLinkList.end(); itr++) 
		{
			LINK_COST hLinkCost = (*itr)->getCost();
			pV = (*itr)->m_pDst;
			// the link has been invalidated
			if ((!(*itr)->valid()) || (UNREACHABLE == hLinkCost))
				continue;

			bool bAuxLink;
			SimplexLink *pSLink = (SimplexLink*)(*itr);
			assert(pSLink);
			switch (pSLink->getSimplexLinkType()) {
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Lightpath:
				bAuxLink = false; 
				break;
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Converter:
				bAuxLink = true;
				break;
			default:
				DEFAULT_SWITCH;
			}

			LINK_COST hLHSCost, hRHSCost;
			hLHSCost = pU->m_hCost + hLinkCost;
			UINT  h;
			if (bAuxLink) {
				for (h=0; h<nBackupVHops; h++)
					hLHSCost += pU->m_pBackupCost[h];
			} else {
				for (h=0; h<nBackupVHops; h++) {
					assert(UNREACHABLE != (*itr)->m_pBackupCost[h]);
					hLHSCost += 
						((pU->m_pBackupCost[h] > (*itr)->m_pBackupCost[h])?
						pU->m_pBackupCost[h]: (*itr)->m_pBackupCost[h]);
				}
			}

			if ((UNREACHABLE == pV->m_hCost) 
				|| (NULL == pV->m_pBackupCost)) {
				hRHSCost = UNREACHABLE;
			} else {
				hRHSCost = pV->m_hCost;
				for (h=0; h<nBackupVHops; h++)
					hRHSCost += pV->m_pBackupCost[h];
			}

			if ((hRHSCost > hLHSCost) 
				|| ((hRHSCost == hLHSCost) 
				    && (pV->m_hCost > (pU->m_hCost + hLinkCost)))) {
				pV->m_hCost = pU->m_hCost + hLinkCost;
				if (NULL == pV->m_pBackupCost) {
					// used in PAbsNodeComp4MPAC
					pV->m_nBackupVHops = nBackupVHops;	
					pV->m_pBackupCost = new LINK_COST[nBackupVHops];
				}
				if (bAuxLink) {
					for (h=0; h<nBackupVHops; h++)
						pV->m_pBackupCost[h] = pU->m_pBackupCost[h];
				} else {
					for (h=0; h<nBackupVHops; h++) {
						pV->m_pBackupCost[h] =
						((pU->m_pBackupCost[h] > (*itr)->m_pBackupCost[h])?
						pU->m_pBackupCost[h]: (*itr)->m_pBackupCost[h]);
					}
				}
				// use previous link instead of node
				pV->m_pPrevLink = *itr;	
			}
		} // for
		hPQ.buildHeap();
	} // while
}


//-B: if there is at least one link on a certain channel (in the whole network) that doesn't have enough capacity,
//	invalidate all those links of the network that are on the same channel
void Graph::invalidateSimplexLinkDueToWContinuity(UINT src, UINT nBW)
{
#ifdef DEBUGB
	cout << "-> invalidateSimplexLinkDueToWContinuity" << endl;
	cout << "\tnumber of channels: " << numberOfChannels << endl;
#endif // DEBUGB

	UINT w = 0;
	SimplexLink*link1, *link2;
	Vertex*pSrc;
	list<AbstractLink*>::const_iterator itr1, itr2;
	//-B: scorro tutti i canali
	for (; w < numberOfChannels; w++)
	{
		pSrc = lookUpVertex(src, Vertex::VT_Channel_Out, w);
		//-B: scorro tutti i link uscenti dal nodo sorgente della connessione
		for (itr1 = pSrc->m_hOLinkList.begin(); itr1 != pSrc->m_hOLinkList.end(); itr1++)
		{
			link1 = (SimplexLink*)(*itr1);
			//-B: ****************SE IL LINK USCENTE DA SRC NON E' VALIDO, NON E' DISPONIBILE PER L'INSTRADAMENTO DELLA CONNESSIONE
			//	QUINDI DEVO INVALIDARE ANCHE TUTTI GLI ALTRI LINK DELLA RETE CON LO STESSO CANALE**********************************
			if (link1->getValidity())
			{
				//-B: se il link NON ha sufficiente capacità disponibile
				if (link1->m_hFreeCap < nBW)
				{
					link1->invalidate();
					//-B: invalido tutti i simplexLink della rete che si trovano sullo stesso canale
					//-B: scorro tutti i simplex link della rete dal primo all'ultimo
					for (itr2 = m_hLinkList.begin(); itr2 != m_hLinkList.end(); itr2++)
					{
						link2 = (SimplexLink*)(*itr2);
						//-B: se è lo stesso canale che ho appena reso non valido, rendo non valido anche questo
						if (link2->m_nChannel == link1->m_nChannel)
						{
							link2->invalidate();
							cout << "\tLink " << link2->getSrc()->getId() << "->" << link2->getDst()->getId() << " non valido" << endl;
						}
					} // end FOR invalidate corresponding links
				} // end IF free capacity
			} // end IF validity
			else
			{
				//-B: invalido tutti i simplexLink della rete che si trovano sullo stesso canale
				//-B: scorro tutti i simplex link della rete dal primo all'ultimo
				for (itr2 = m_hLinkList.begin(); itr2 != m_hLinkList.end(); itr2++)
				{
					link2 = (SimplexLink*)(*itr2);
					//-B: se è lo stesso canale che ho appena reso non valido, rendo non valido anche questo
					if (link2->m_nChannel == link1->m_nChannel)
					{
						link2->invalidate();
					}
				} // end FOR invalidate corresponding links
			} //end ELSE validity
		} // end FOR outgoing links
	} // end FOR channels
}

//-B: 
UINT Graph::WPrandomChannelSelection(UINT src, UINT dst, UINT nBW)
{
#ifdef DEBUGB
	cout << endl << "-> WPrandomChannelSelection" << endl;
	cout << "\tScorro gli outgoing links del nodo src e gli incoming link del nodo dst " << src << "->" << dst << endl;
#endif // DEBUGB

	UINT w, ch, i;
	Vertex*pSrc, *pDst;
	list<AbstractLink*>::const_iterator itr1, itr2;
	SimplexLink *sLink, *sLink2;
	vector<int> admissibleChannels;
	bool found, found2;

	admissibleChannels.clear();
	cout << "\tAdmissible channels: " << endl;
	//-B: scorro tutti i canali
	for (w = 0; w < numberOfChannels; w++)
	{
		pSrc = lookUpVertex(src, Vertex::VT_Channel_Out, w);
		cout << "\n\t(VERTEX ";
		cout.width(3); cout << pSrc->getId() << ")";
		//-B: scorro tutti i link uscenti dal nodo sorgente della connessione
		for (itr1 = pSrc->m_hOLinkList.begin(); itr1 != pSrc->m_hOLinkList.end(); itr1++)
		{
			sLink = (SimplexLink*)(*itr1);
			//sLink = this->lookUpSimplexLink(src, Vertex::VT_Channel_Out, w, sLink->getDst()->getId(), Vertex::VT_Channel_In, w);
			//-B: scorro tutti i canali già inseriti nella lista degli admissible per controllare che non sia già presente
			for (found = false, i = 0; i < admissibleChannels.size(); i++)
			{
				if (admissibleChannels[i] == sLink->m_nChannel)
				{
					found = true;
					break;
				}
			}
			//-B: se non è già presente
			if (!found)
			{
				cout << " (SLINK " << sLink->getId() << ")";
				//-B: se risulta ancora valido
				if (sLink->getValidity())
				{
					cout << "(VALID)";
					//-B: se la sua capacità disponibile è sufficiente per la connessione da instradare
					if (sLink->m_hFreeCap >= nBW)
					{
						//-B: scorro tutti i canali
						//for (w2 = 0; w2 < numberOfChannels; w2++)
						//{
							pDst = lookUpVertex(dst, Vertex::VT_Channel_In, w);
							//-B: scorro tutti i link entranti nel nodo destinazione della connessione
							for (found2 =  false, itr2 = pDst->m_hILinkList.begin(); itr2 != pDst->m_hILinkList.end(); itr2++)
							{
								sLink2 = (SimplexLink*)(*itr2);
								//sLink2 = this->lookUpSimplexLink(sLink2->getSrc()->getId(), Vertex::VT_Channel_Out, w, dst, Vertex::VT_Channel_In, w);
								//-B: se il link è sullo stesso canale
								if (sLink2->m_nChannel == sLink->m_nChannel)
								{
									//-B: se anche il link entrante nella destinazione è valido e con capacità sufficiente
									if (sLink2->getValidity() && sLink2->m_hFreeCap >= nBW)
									{
										found2 = true;
										break;
									}
								} // end IF same outgoing/incoming channel
							} // end FOR incoming simplex links
							if (found2)
							{
								admissibleChannels.push_back(sLink->m_nChannel);
								cout << " -> ch " << sLink->m_nChannel << " ";
								break;
							}
						//} // end FOR channels
					} // end IF free cap
				} // end IF validity
			} // end IF !found
		} // end FOR outgoing simplex links
	} // end FOR channels
	cout << "\n\tAdmissible channel size: " << admissibleChannels.size() << endl;

	//-B: rand() seems to be not random at all -> not used
	//-B: ------- GENERATE RANDOM NUMBER --------- (between 1 and admissibleChannels.size()) with Mersenne Twister Engine
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, admissibleChannels.size());
	ch = dis(gen);
	// -B: generate random number (between 1 and numberOfOXCNodes) with Default Random Engine
	/*std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(1, m_nNumberOfOXCNodes);
	nSrc = distribution(generator); //sorgente casuale distribuzione uniforme
	*/
	return admissibleChannels[ch - 1];
}


//-B: it is used to prefer wavelength continuity over w. conversion in a condition of equality
//	between the wavelength that would allow a wavelength path and any other wavelength
void Graph::preferWavelPath()
{
#ifdef DEBUG
	cout << "-> preferWavelPath" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink;
	for (itr = this->m_hLinkList.begin(); itr != this->m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		switch (pLink->m_eSimplexLinkType)
		{
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Lightpath:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_UniFiber:
				NULL; //nothing to do
				break;
			case SimplexLink::LT_Converter:
			case SimplexLink::LT_Grooming:
				pLink->modifyCost(0.6); 
				//-B: 0.6 and not 0.5, otherwise it could choose an already existing lightpath 
				//(assuming 1-hop lp --> cost = 0.5)
				//using another wavelength instead of activating a new lp over the same wavelength. 
				//This is important because we care about latency!
				//Monica: devo verificare che latency(grooming)>latency(lightpath 1-hop)
				break;
			default:
				DEFAULT_SWITCH;
		}
	}

#ifdef DEBUGB
	cout << endl;
#endif // DEBUGB
}


void Graph::removeSimplexLinkLightpath(Lightpath*pLightpath)
{
#ifdef DEBUG
	cout << "-> removeSimplexLinkLightpath" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink;
	bool found = false;

	//-B: UNFORTUNATELY IT IS NOT POSSIBLE TO FIND A SIMPLEX LINK LT_Lightpath WITH lookUpSimplexLink

	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		switch (pLink->getSimplexLinkType())
		{
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Converter:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_UniFiber:
				NULL;
				break;
			case SimplexLink::LT_Lightpath:
			{
				if (pLink->m_pLightpath == pLightpath)
				{

#ifdef DEBUG
					cout << "\tElimino dal grafo il simplex link " << pLink->getId()
						<< " corrispondente al lightpath " << pLink->m_pLightpath->getId() << " (";
					cout.width(3); cout << pLink->m_pLightpath->getSrc()->getId(); cout << "->";
					cout.width(3); cout << pLink->m_pLightpath->getDst()->getId(); cout << ")" << endl;
					cout << "\tSimplex Link freecap = " << pLink->m_hFreeCap << " - lightpath free cap = " << pLightpath->getFreeCapacity() << endl;
#endif // DEBUGB
					assert(pLink->m_hFreeCap == pLightpath->getFreeCapacity());
					assert(pLink->m_hFreeCap == OCLightpath);
					UINT size = this->m_hLinkList.size();
					this->removeLink(pLink); //-B: THIS IS THE RIGHT INSTRUCTION TO DELETE IT
					assert((size - 1) == this->m_hLinkList.size());
					//m_hLinkList.erase(pLink->getId()); //-B: !!! it deleted the simplex link LT_Lightpath only from m_hGraph.m_hLinkList
														//	BUT not from pSrc.m_hOLinkList or pDst.m_hILinkList !!!
					found = true;
				}
				break;
			}
			default:
				DEFAULT_SWITCH;
		}//end SWITCH
		if (found)
		{
			break;
		}
	}//end FOR
}


void Graph::removeSimplexLinkLightpath()
{
#ifdef DEBUG
	cout << "-> removeSimplexLinkLightpath" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink;

	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		switch (pLink->getSimplexLinkType())
		{
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Converter:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_UniFiber:
				NULL;
				break;
			case SimplexLink::LT_Lightpath:
			{
				if (pLink->m_hFreeCap == OCLightpath)
				{
	#ifdef DEBUG
					cout << "\tElimino dal grafo il simplex link " << pLink->getId() << " (";
					cout << "\tSimplex Link freecap = " << pLink->m_hFreeCap << endl;
	#endif // DEBUGB

					UINT size = this->m_hLinkList.size();
					this->removeLink(pLink);
					assert((size - 1) == this->m_hLinkList.size());
					//m_hLinkList.erase(pLink->getId()); //-B: !!! it deleted the simplex link LT_Lightpath only from m_hGraph.m_hLinkList
					//	BUT not from pSrc.m_hOLinkList or pDst.m_hILinkList !!!
				}
				break;
			}
			default:
				DEFAULT_SWITCH;
		}//end SWITCH
	}//end FOR
}


void Graph::releaseSimplexLinkLightpathBwd(UINT nBWToRelease, Lightpath*pLightpath)
{
#ifdef DEBUG
	cout << "-> releaseSimplexLinkLightpathBwd" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr;
	bool found = false;
	SimplexLink*pSLink;

	for (itr = this->m_hLinkList.begin(); itr != this->m_hLinkList.end(); itr++)
	{
		pSLink = (SimplexLink*)(*itr);
		if (pSLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
		{
			if (pSLink->m_pLightpath == pLightpath)
			{
				pSLink->m_hFreeCap += nBWToRelease;
				break;
			}
		}
	}
#ifdef DEBUG
	cout << "\tCHECK:" << endl;
	cout << "\t\tLightpath " << pLightpath->getId() << " (" << pLightpath->getSrc()->getId() << "->"
		<< pLightpath->getDst()->getId() << ") - free capacity = " << pLightpath->getFreeCapacity() << endl;
	cout << "\t\tSimplex Link LT_Lightpath (" << ((Vertex*)pSLink->getSrc())->m_pOXCNode->getId() << "->"
		<< ((Vertex*)pSLink->getDst())->m_pOXCNode->getId() << ") che punta al lightpath " << pSLink->m_pLightpath->getId()
		<< " - free capacity = " << pSLink->m_hFreeCap << endl;
#endif // DEBUGB
	assert(pSLink->m_hFreeCap == pLightpath->getFreeCapacity());
}


bool Graph::isSimplexLinkRemoved(Lightpath*pLightpath)
{
#ifdef DEBUGB
	//cout << "-> isSimplexLinkRemoved" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink;
	UINT index = 0;

	//scann all simplex links
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++, index++)
	{
		pLink = (SimplexLink*)(*itr);

		//if it is a LT_Lightpath simplex link
		switch (pLink->getSimplexLinkType())
		{
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Converter:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_UniFiber:
				NULL;
				break;
			case SimplexLink::LT_Lightpath:
			{
				if (pLink->m_pLightpath == pLightpath)
				{
					return false;
				}
				break;
			}
			default:
				DEFAULT_SWITCH;
		}//end SWITCH
	}//end FOR

	return true;
}


//-B: invalidate all simplex links whose type is LT_Converter or LT_Grooming
//	so that only wavel continuity is possible
// USED FOR: //-B: invalidate LT_Converter and LT_Grooming links before starting the simulation
//	i.e. when no candidate hotel nodes has BBUs active
void Graph::invalidateGrooming()
{
#ifdef DEBUG
	cout << "-> invalidateGrooming" << endl;
#endif // DEBUGB

	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;
	//Vertex*pVertex;
	//OXCNode*pNode;
	int count = 0;

	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);

		switch (pSimplexLink->m_eSimplexLinkType)
		{
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Converter:
			{
				pSimplexLink->invalidate();
				break;
			}
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Lightpath:
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Rx:
				NULL;	// Nothing to do so far
				break;
			default:
				DEFAULT_SWITCH;
		}
	}
}

//-B: if a candidate hotel node starts hosting at least a BBU we have to consider the node as active
//	and so we have to make valid its simplex link LT_Grooming and LT_Convereter
void Graph::validateGroomingHotelNode()
{
#ifdef DEBUGB
	cout << "-> validateGroomingHotelNode" << endl;
#endif // DEBUGB

	//channelsToDelete.clear();
	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;
	Vertex*pVertex;
	OXCNode*pNode;
	int count = 0;

	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);

		switch (pSimplexLink->m_eSimplexLinkType)
		{
		case SimplexLink::LT_Grooming:
		case SimplexLink::LT_Converter:
		{
			if (!pSimplexLink->getValidity())
			{
				pVertex = (Vertex*)(pSimplexLink->getSrc());
				pNode = pVertex->m_pOXCNode;
				if (pNode->m_nBBUs > 0)
				{
					pSimplexLink->validate();
				}
			}
			break;
		}
		case SimplexLink::LT_Channel:
		case SimplexLink::LT_UniFiber:
		case SimplexLink::LT_Lightpath:
		case SimplexLink::LT_Channel_Bypass:
		case SimplexLink::LT_Mux:
		case SimplexLink::LT_Demux:
		case SimplexLink::LT_Tx:
		case SimplexLink::LT_Rx:
			NULL;	// Nothing to do so far
			break;
		default:
			DEFAULT_SWITCH;
		}
	}
}

list<AbstractLink*> Graph::removeRXLink(list<AbstractLink*> firstSeg, list<AbstractLink*> secondSeg) {

	list<AbstractLink*>::const_iterator itFirst;
	list<AbstractLink*>::const_iterator itSec;
	list<AbstractLink*> newFirstSeg;
	SimplexLink* linkFirstSeg;
	bool RXpresent = false;
	for (itSec = secondSeg.begin(); itSec != secondSeg.end(); itSec++) {
		
		if (((SimplexLink*)(*itSec))->getSimplexLinkType() == SimplexLink::LT_Rx) {
			RXpresent = true;
			break;
		}
	}

	if (RXpresent) {
		for (itFirst = firstSeg.begin(); itFirst != firstSeg.end(); itFirst++) {

			linkFirstSeg = (SimplexLink*)(*itFirst);
			if (((SimplexLink*)(*itFirst))->getSimplexLinkType() == SimplexLink::LT_Rx) {
				cout << "Link RX will not be added is from " << ((Vertex*)(linkFirstSeg->getSrc()))->m_pOXCNode->getId()
					<< " to " << ((Vertex*)(linkFirstSeg->getDst()))->m_pOXCNode->getId() << endl;
				//cin.get();
				continue;
			}
			newFirstSeg.push_back((*itFirst));

		}
		return newFirstSeg;
	}
	return firstSeg;
}

list<AbstractLink*> Graph::removeTXLink(list<AbstractLink*> secondSeg, Vertex* vSrc) {

	list<AbstractLink*>::const_iterator itSecond;
	list<AbstractLink*> newSecSeg;
	SimplexLink* linkSecSeg;

	for (itSecond = secondSeg.begin(); itSecond != secondSeg.end(); itSecond++) {

		linkSecSeg = (SimplexLink*)(*itSecond);
		if (((SimplexLink*)(*itSecond))->getSimplexLinkType() == SimplexLink::LT_Tx && ((Vertex*)(linkSecSeg->getSrc()))->m_pOXCNode->getId() != vSrc->m_pOXCNode->getId()) {
			cout << "Link TX will not be added is from " << ((Vertex*)(linkSecSeg->getSrc()))->m_pOXCNode->getId()
				<< " to " << ((Vertex*)(linkSecSeg->getDst()))->m_pOXCNode->getId() << endl;
			//cin.get();
			continue;
		}
		newSecSeg.push_back((*itSecond));

	}
	return newSecSeg;
}

list<AbstractLink*> Graph::checkWavelengthsOnFibers(list<AbstractLink*> firstSeg, list<AbstractLink*> secondSeg) {

	list<AbstractLink*>::const_iterator itrF;
	list<AbstractLink*>::const_iterator itrS;
	list<AbstractLink*> newSecSeg;
	int w = -1;
	for (itrF = firstSeg.begin(); itrF != firstSeg.end(); itrF++) {

		if (((SimplexLink*)(*itrF))->m_eSimplexLinkType == SimplexLink::LT_Channel) {
			w = ((SimplexLink*)(*itrF))->m_nChannel;
			cout << "w is " << w << endl;
		}
	}

	bool freeCapNotEnough = false;

	SimplexLink* lSec;
	if (w != -1) {
		for (itrS = secondSeg.begin(); itrS != secondSeg.end(); itrS++) {

			if (((SimplexLink*)(*itrS))->m_eSimplexLinkType == SimplexLink::LT_Channel && !freeCapNotEnough) {
				lSec = (SimplexLink*)(*itrS);
				cout << "Check WAVELENGTH -> Link is from " << ((Vertex*)(lSec->getSrc()))->m_pOXCNode->getId()
					<< " to " << ((Vertex*)(lSec->getDst()))->m_pOXCNode->getId() << " on ch: " << lSec->m_nChannel << endl;
				//cin.get();

				if (((SimplexLink*)(*itrS))->m_nChannel != w) {

					cout << "Check WAVELENGTH, different w -> Link is from " << ((Vertex*)(lSec->getSrc()))->m_pOXCNode->getId()
						<< " to " << ((Vertex*)(lSec->getDst()))->m_pOXCNode->getId() << " on ch: " << lSec->m_nChannel << endl;
					//cin.get();

					UINT src = ((Vertex*)(lSec->getSrc()))->m_pOXCNode->getId();
					UINT dst = ((Vertex*)(lSec->getDst()))->m_pOXCNode->getId();
					int nS = ((Vertex*)(lSec->getSrc()))->m_nChannel;
					int nD = ((Vertex*)(lSec->getDst()))->m_nChannel;

					if (((Vertex*)(lSec->getSrc()))->m_eVType == Vertex::VertexType::VT_Channel_In || ((Vertex*)(lSec->getSrc()))->m_eVType == Vertex::VertexType::VT_Channel_Out) {
						nS = w;

						if (((Vertex*)(lSec->getDst()))->m_eVType == Vertex::VertexType::VT_Channel_In || ((Vertex*)(lSec->getDst()))->m_eVType == Vertex::VertexType::VT_Channel_Out) {
							nD = w;

							SimplexLink* newLink = lookUpSimplexLink(src, ((Vertex*)(lSec->getSrc()))->m_eVType, nS, dst, ((Vertex*)(lSec->getDst()))->m_eVType, nD);

							if ((UINT)newLink->m_hFreeCap == 1944) {
								cout << "Link to use is from " << src << " to " << dst << " on channel " << w << endl;
								newSecSeg.push_back(newLink);

							}
							else {
								cout << "Capacity is not enough; it is " << newLink->m_hFreeCap << endl;
								cin.get();
								freeCapNotEnough = true;
								newSecSeg.push_back(*itrS);
							}
						}
					}
					else {
						newSecSeg.push_back(*itrS);
					}
				}
				else {

					newSecSeg.push_back(*itrS);

				}

			}
			return newSecSeg;
		}	
		return secondSeg;
	}
	return secondSeg;
}
