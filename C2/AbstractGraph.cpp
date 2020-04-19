#pragma warning(disable: 4786)
#pragma warning(disable: 4018)
#pragma warning(disable: 4996)
#define WAVE 16
#include <assert.h>
#include <iostream>
#include <stack>
#include <vector>
#include <algorithm>
#include <math.h>
#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "BinaryHeap.h"
#include "AbstractPath.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "OXCNode.h"
#include "OchMemDebug.h"
#include "Vertex.h"
#include "Graph.h"
#include "Lightpath.h"
#include "Circuit.h"
#include "LightpathDB.h"
#include "SimplexLink.h"
#include "Connection.h"
#include "UniFiber.h"
#include "OXCNode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

long int linkvisited=0;
using namespace NS_OCH;

///////////////////////////////////////////////////////////////////////////////
//
AbstractNode::AbstractNode(): m_bValid(false),forceUpdate(true),costiParzialiFlag(false)
{
}

bool AbstractNode::updateOutLinks(set<int>& chan)
{
#ifdef DEBUGB
	//cout << "-> updateOutLinks" << endl;
#endif // DEBUGB

	list<AbstractLink*>::iterator itrLink;
	//int i;
	int ii;
	int dstId;
	int srcId;
	bool	out=false;
	set<int> updates2;
	//ciclo sui link d'uscita
	for (itrLink = m_hOLinkList.m_hList.begin(); itrLink != m_hOLinkList.m_hList.end(); itrLink++)
	{
		//cout << "Fiber " << (*itrLink)->getId() << endl;
		//updates2 = updates;
		srcId = (*itrLink)->getSrc()->m_nNodeId;
		dstId = (*itrLink)->getDst()->m_nNodeId;
		if (!(*itrLink)->valid())
			continue;
		if( !(*itrLink)->getDst()->valid())
			continue;
		out = false;
		//while (updates2.size()){
		//ii=(*updates2.begin());
		//updates2.erase(updates2.begin());
		for (ii = 0; ii < updates.size(); ii++)
		{
			if(updates[ii] == false)
				continue;
			//ii=updates[i];
			//cout << "costo ch " << ii << ": " << (*itrLink)->wlOccupation[ii] << "\t";
			LINK_COST newCost = costi[ii] + ((*itrLink)->getCost()*((*itrLink)->wlOccupation[ii]));
			//aggiornamento costi
			if (newCost < (*itrLink)->getDst()->costi[ii])
			{ //AGGIORNO
				(*itrLink)->getDst()->costi[ii] = newCost;
				(*itrLink)->getDst()->prev[ii] = (*itrLink);
				chan.insert(dstId);
				(*itrLink)->getDst()->updates[ii] = true;
			}
		}
		//cout << endl;
	} //ciclo sui link
	//updates.clear();
	return out;
}

bool AbstractNode::updateOutLinksPartial(set<int>& chan)
{
	list<AbstractLink*>::iterator itrLink;
	//int i;
	int ii;
	int dstId;
	int srcId;
	set<int> updates2;
bool	out=false;
	//ciclo sui link d'uscita
	for(itrLink=m_hOLinkList.m_hList.begin();itrLink!=m_hOLinkList.m_hList.end();itrLink++)
	{
		srcId=(*itrLink)->getSrc()->m_nNodeId;
		dstId=(*itrLink)->getDst()->m_nNodeId;
		if (!(*itrLink)->valid())
			continue;
		if( !(*itrLink)->getDst()->valid()) continue;
//		updates2=updates;
		
		out=false;
	
//		for (i=0;i<updates.size();i++){
//			ii=updates[i];
		
//				while (updates2.size()){
//ii=(*updates2.begin());
//		updates2.erase(updates2.begin());

		for (ii=0;ii<updates.size();ii++){

		if(updates[ii]==false) continue;
			if(prevParziali[ii]!=NULL)
			if(dstId==prevParziali[ii]->getSrc()->getId()) 
				continue; //FABIO 16 marzo: per evitare loop durante yen con k>1
		
				LINK_COST newCost= costiParziali[ii]+((*itrLink)->getCost()*((*itrLink)->wlOccupation[ii]));
		
			 

			//aggiornamento costi
			
			 if (  newCost<(*itrLink)->getDst()->costiParziali[ii])
			{//AGGIORNO
				
				
				(*itrLink)->getDst()->costiParziali[ii]=newCost;
				
				(*itrLink)->getDst()->prevParziali[ii]=(*itrLink);
				
				
				chan.insert(dstId);

				
				(*itrLink)->getDst()->updates[ii]=true;
			}
			

		}

	
	}//ciclo sui link

//	updates.clear();
	return out;

}

bool AbstractNode::updateOutLinksBCK(set<int>& chan)//versione per il calcolo backup
{
	list<AbstractLink*>::iterator itrLink;
//	int i;
	int ii;
	int dstId;
	int srcId;
	
bool	out=false;
//set<int> updates2;
	//ciclo sui link d'uscita
	for(itrLink=m_hOLinkList.m_hList.begin();itrLink!=m_hOLinkList.m_hList.end();itrLink++)
	{
		srcId=(*itrLink)->getSrc()->m_nNodeId;
		dstId=(*itrLink)->getDst()->m_nNodeId;
		if (!(*itrLink)->valid()) continue;
		if( !(*itrLink)->getDst()->valid()) continue;
		//updates2=updates;
		
				//ii=updates[i];
	/*while (updates2.size()){
ii=(*updates2.begin());
		updates2.erase(updates2.begin());*/
		for (ii=0;ii<updates.size();ii++){

		if(updates[ii]==false) continue;
				LINK_COST newCost= costi[ii]+((*itrLink)->getCost()*((*itrLink)->wlOccupationBCK[ii]));
		
			 

			//aggiornamento costi
			
			 if (  newCost<(*itrLink)->getDst()->costi[ii])
			{//AGGIORNO
				out=true;
				
				(*itrLink)->getDst()->costi[ii]=newCost;
				
				(*itrLink)->getDst()->prev[ii]=(*itrLink);
				chan.insert(dstId);
				(*itrLink)->getDst()->updates[ii]=true;
			}
			

		}

	
	}//ciclo sui link

//	updates.clear();
	return out;

}

AbstractNode::AbstractNode(UINT nNodeId): m_nNodeId(nNodeId), 
	m_pPrevLink(NULL), m_hCost(5), m_bValid(true), m_dLatency(0),
	m_nBackupVHops(0), m_pBackupCost(NULL),forceUpdate(true),costiParzialiFlag(false)
{
	int g=7;
}

AbstractNode::AbstractNode(UINT nNodeId,int Nchan): m_nNodeId(nNodeId), 
	m_pPrevLink(NULL), m_hCost(5), m_bValid(true), m_dLatency(0),
	m_nBackupVHops(0), m_pBackupCost(NULL),forceUpdate(true),costiParzialiFlag(false)
{
	prevParziali = vector<AbstractLink*>(Nchan);
	costiParziali = valarray<LINK_COST> (UNREACHABLE,Nchan);
}

AbstractNode::~AbstractNode()
{
	if (m_pBackupCost) {
		delete []m_pBackupCost;
		m_pBackupCost = NULL;
	}
}

AbstractNode::AbstractNode(const AbstractNode& rhs)
{
	*this = rhs;
}

// NB: only the immutable attributes are copied
const AbstractNode& AbstractNode::operator=(const AbstractNode& rhs)
{
	if (this == &rhs)
		return (*this);
	
	m_nNodeId = rhs.m_nNodeId;
	m_bValid = rhs.m_bValid;

	m_hCost = UNREACHABLE;
	m_pPrevLink = NULL;
	m_pBackupCost = NULL;
	return (*this);
}

void AbstractNode::dump(ostream& out) const
{
#ifdef DEBUGB
	cout << "ABSTRACTNODE DUMP" << endl;
	cin.get();
#endif // DEBUGB
	out.width(4);
	out<<m_nNodeId;
	if (UNREACHABLE != m_hCost) {
		out<<" C=";
		out.precision(5);
		out.setf(ios::fixed, ios::floatfield); //http://www.cplusplus.com/reference/ios/ios_base/precision/?kw=precision
		out<<m_hCost<<" ";
	} else
		out<<" C=INF ";
	if (m_pPrevLink) {
		out<<m_pPrevLink->m_pSrc->getId();
		out<<"(LinkID="<<m_pPrevLink->getId()<<')'<<"(Prev)";
	} else {
		out<<"NULL";
	}
	out<<endl;
	list<AbstractLink*>::const_iterator iter;
	for (iter=m_hOLinkList.begin(); iter!=m_hOLinkList.end(); iter++) {
		out<<'\t';
		//-B: ABSTRACTLINK DUMP
		(*iter)->dump(out);
		out<<endl;
	}
}

UINT AbstractNode::getId() const
{
	return m_nNodeId;
}

LINK_COST AbstractNode::getCost()
{
	return m_hCost;
}


AbstractLink* AbstractNode::lookUpIncomingLink(UINT nSrc) const
{
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hILinkList.begin(); itr!=m_hILinkList.end(); itr++)
		if ((*itr)->m_pSrc->getId() == nSrc)
			return (*itr);
	return (AbstractLink*)NULL;
}

AbstractLink* AbstractNode::lookUpOutgoingLink(UINT nDst) const
{
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hOLinkList.begin(); itr != m_hOLinkList.end(); itr++)
		if ((*itr)->m_pDst->getId() == nDst)
			return (*itr);
	return (AbstractLink*)NULL;
}

UINT AbstractNode::inDegree() const
{
	return m_hILinkList.size();
}

UINT AbstractNode::outDegree() const
{
	return m_hOLinkList.size();
}

void AbstractNode::addIncomingLink(AbstractLink *pILink)
{
	assert(pILink);
	assert(pILink->m_pDst->m_nNodeId == m_nNodeId);
	m_hILinkList.push_front(pILink->getId(), pILink);
}

void AbstractNode::addOutgoingLink(AbstractLink *pOLink)
{
	assert(pOLink);
	assert(pOLink->m_pSrc->m_nNodeId == m_nNodeId);
	m_hOLinkList.push_front(pOLink->getId(), pOLink);
}

void AbstractNode::invalidate()
{
	m_bValid = false;
}

void AbstractNode::validate()
{
	m_bValid = true;
}

bool AbstractNode::valid() const
{
	return m_bValid;
}

///////////////////////////////////////////////////////////////////////////////
// ABSTRACT LINK
AbstractLink::AbstractLink(): m_pSrc(NULL), m_pDst(NULL), m_bValid(true),
	m_hCost(UNREACHABLE), m_nLength(0), m_bCostSaved(false),
	m_nBackupVHops(0), m_pBackupCost(NULL), m_dLinkLoad(0),
	m_pCSet(NULL), m_nCSetSize(0), m_nBChannels(0), m_used(0)
{
	m_latency = m_nLength * (float)PROPAGATIONLATENCY;
}

AbstractLink::AbstractLink(int nId, AbstractNode* pSrc, AbstractNode* pDst,
			 LINK_COST hCost, float nLength): m_nLinkId(nId), 
			 m_pSrc(pSrc), m_pDst(pDst), m_hCost(hCost), m_nLength(nLength),
			 m_bValid(true), m_bCostSaved(false), m_dLinkLoad(0),
			 m_nBackupVHops(0), m_pBackupCost(NULL),
			 m_pCSet(NULL), m_nCSetSize(0), m_nBChannels(0), m_used(0)
{
	m_latency = m_nLength * (float)PROPAGATIONLATENCY;
}									



AbstractLink::~AbstractLink()
{
	if (m_pBackupCost) {
		delete []m_pBackupCost;
		m_pBackupCost = NULL;
	}
	if (m_pCSet) {
		delete []m_pCSet;
		m_pCSet = NULL;
	}
}

void AbstractLink::dump(ostream& out) const
{
#ifdef DEBUGB
	out << "ABSTRACTLINK DUMP" << endl;
	cin.get();
#endif // DEBUGB
	out<<"AbsLink "<<m_nLinkId<<": "<<m_pSrc->m_nNodeId<<"->"<<m_pDst->m_nNodeId;
	//out<<" C="<<m_hCost;//<<" Length="<<m_nLength;
	if (m_bValid)
		out<< " VALID";
	else
		out<<" INVALID";
    out<< " Cost = " << m_hCost;
	out << " Length (km) = " << m_nLength;
	out<<endl;
}

AbstractLink::LinkType AbstractLink::getLinkType() const
{
	return LT_Abstract;
}

int AbstractLink::getId() const
{
	return m_nLinkId;
}

AbstractNode* AbstractLink::getSrc() const
{
	assert(m_pSrc);
	return m_pSrc;
}

AbstractNode* AbstractLink::getDst() const
{
	assert(m_pDst);
	return m_pDst;
}

LINK_COST AbstractLink::getCost() const
{
	return m_hCost;
}

LINK_COST AbstractLink::getSavedCost() const
{
	return m_hSavedCost;
}

//Complete Information cost evaluation  -M

LINK_COST AbstractLink::getOriginalCost(bool bSaveCI2) const
{           LINK_COST hCostOrig;
	     if  (bSaveCI2==0) 
	       {hCostOrig= m_hCost; }
	       // cerr << "a"; }
	       else   {hCostOrig= m_hSavedCost;}
	       //     cerr << "b"; }
	return hCostOrig;
}

float AbstractLink::getLength() const
{
	return m_nLength;
}


void AbstractLink::invalidate()
{
	m_bValid = false;
}

void AbstractLink::validate()
{
	m_bValid = true;
}

bool AbstractLink::valid() const
{
	return m_bValid;
}

void AbstractLink::modifyCost(LINK_COST hNewCost)
{
	m_hSavedCost = m_hCost;
	m_bCostSaved = true;
	m_hCost = hNewCost;
}


int AbstractLink::getUsedStatus()
{
	return m_used;
}

void AbstractLink::restoreStatus()
{
	m_used=0; 
}

bool NS_OCH::AbstractLink::getValidity()
{
	return m_bValid;
}


void AbstractLink::modifyCostCI(LINK_COST hNewCost, bool bSavedCI2)
{       if ( bSavedCI2==0)
	  {  m_hSavedCost = m_hCost;
	    m_bCostSaved = true;
	    m_hCost = hNewCost;
	    //cerr << "b"; 
}
 else { m_hCost += hNewCost;}
 // cerr << "c";} 
}

bool AbstractLink::costModifed() const
{
	return m_bCostSaved;
}

void AbstractLink::restoreCost()
{
	if (m_bCostSaved)
	{
		m_hCost = m_hSavedCost;
		m_bCostSaved = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
AbstractGraph::AbstractGraph(): m_nNextLinkId(0)
{
}

AbstractGraph::~AbstractGraph()
{
	list<AbstractNode*>::iterator itr;
	for (itr=m_hNodeList.begin(); itr!=m_hNodeList.end(); itr++)
		delete (*itr);

	list<AbstractLink*>::iterator itrLink;
	for (itrLink=m_hLinkList.begin(); itrLink!=m_hLinkList.end(); itrLink++) 
		delete (*itrLink);
// NB: design has changed
//		lightpaths will NOT serve as links in other graph as this has given
//		me endless hassle.
//	{
//		AbstractLink *pLink = *itrLink;
//		AbstractLink::LinkType hLinkType = pLink->getLinkType();
//		// existing lightpath can be a link
//		switch (hLinkType) {
//		case AbstractLink::LT_Abstract:
//		case AbstractLink::LT_Simplex:
//		case AbstractLink::LT_UniFiber:
//			delete pLink;
//			break;
//		case AbstractLink::LT_Lightpath:
//			NULL;	// leave it, and it will be deleted in LightpathDB
//			break;
//		default:
//			DEFAULT_SWITCH;
//		}
//	}
}

AbstractGraph::AbstractGraph(const AbstractGraph& rhs)
{
	*this = rhs;
}

const AbstractGraph& AbstractGraph::operator=(const AbstractGraph& rhs)
{
	if (&rhs == this) return *this;

	m_nNextLinkId = rhs.m_nNextLinkId;
	// copy nodes
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=rhs.m_hNodeList.begin(); itrNode!=rhs.m_hNodeList.end(); 
	itrNode++)
		this->addNode(new AbstractNode((*itrNode)->getId(),numberOfChannels));

	// copy links
	list<AbstractLink*>::const_iterator itrLink;
	for (itrLink=rhs.m_hLinkList.begin(); itrLink!=rhs.m_hLinkList.end();
	itrLink++)
		this->addLink((*itrLink)->getId(), (*itrLink)->getSrc()->getId(),
		(*itrLink)->getDst()->getId(), (*itrLink)->getCost(),
		(*itrLink)->getLength());

	return (*this);
}

void AbstractGraph::dump(ostream& out) const
{
	out<<"AbstractGraph: Nodes = "<<m_hNodeList.size();
	out<<", Links = "<<m_hLinkList.size();
	out<<", NextLinkId = "<<m_nNextLinkId;
	out<<endl;
	list<AbstractNode*>::const_iterator itr;
	for (itr=m_hNodeList.begin(); itr!=m_hNodeList.end(); itr++) {
		(*itr)->dump(out);
		out<<endl;
	}
}

void AbstractGraph::deleteContent()
{
	list<AbstractNode*>::iterator itr;
	for (itr=m_hNodeList.begin(); itr!=m_hNodeList.end(); itr++)
		delete (*itr);
	m_hNodeList.clear();

	list<AbstractLink*>::iterator itrLink;
	for (itrLink=m_hLinkList.begin(); itrLink!=m_hLinkList.end(); itrLink++)
		delete (*itrLink);
	m_hLinkList.clear();
	m_nNextLinkId = 0;
}

UINT AbstractGraph::getNumberOfNodes() const
{
	return m_hNodeList.size();
}

UINT AbstractGraph::getNumberOfLinks() const
{
	return m_hLinkList.size();
}

AbstractNode* AbstractGraph::lookUpNodeById(UINT nNodeId) const
{
	return m_hNodeList.find(nNodeId);
}

AbstractLink* AbstractGraph::lookUpLinkById(UINT nLinkId) const
{
	return m_hLinkList.find(nLinkId);
}

AbstractLink* AbstractGraph::lookUpLink(UINT nSrc, UINT nDst) const
{
	AbstractNode *pSrc = m_hNodeList.find(nSrc);
	if (NULL == pSrc)
		return NULL;
	else
		return pSrc->lookUpOutgoingLink(nDst);
}

bool AbstractGraph::addNode(AbstractNode *pNode)
{
	assert(pNode);

	if (NULL != m_hNodeList.find(pNode->getId()))
		return false;	// already exist

	m_hNodeList.push_front(pNode->getId(), pNode);
	return true;
}

bool AbstractGraph::addLink(AbstractLink *pLink)
{
	assert(pLink);

	AbstractNode *pSrc = m_hNodeList.find(pLink->getSrc()->getId());
	AbstractNode *pDst = m_hNodeList.find(pLink->getDst()->getId());
	if ((NULL == pSrc) || (NULL == pDst))
		return false;	// at least one end-node does not exist
	if (NULL != m_hLinkList.find(pLink->getId()))
		return false;	// the link already exists

	pSrc->addOutgoingLink(pLink);
	pDst->addIncomingLink(pLink);
	m_hLinkList.push_front(pLink->getId(), pLink);

	if (pLink->getId() >= m_nNextLinkId)
		m_nNextLinkId = pLink->getId() + 1;
	return true;
}

AbstractLink* AbstractGraph::addLink(int nId, UINT nSrc, UINT nDst, 
							LINK_COST hCost, float nLength)
{
	AbstractNode *pSrc = m_hNodeList.find(nSrc);
	AbstractNode *pDst = m_hNodeList.find(nDst);
	if ((NULL == pSrc) || (NULL == pDst))
		return NULL;	// at least one end-node does not exist
	if (NULL != m_hLinkList.find(nId))
		return NULL;

	AbstractLink *pLink = new AbstractLink(nId, pSrc, pDst, hCost, nLength);
	pSrc->addOutgoingLink(pLink);
	pDst->addIncomingLink(pLink);
	m_hLinkList.push_front(pLink->getId(), pLink);

	if (nId >= m_nNextLinkId)
		m_nNextLinkId = nId + 1;
	return pLink;
}

int AbstractGraph::getNextLinkId()
{
	return (m_nNextLinkId++);
}

//-B: reset everything (?) about all the nodes in the network
void AbstractGraph::reset4ShortestPathComputation()
{
	list<AbstractNode*>::const_iterator itr;
	for (itr = m_hNodeList.m_hList.begin(); itr != m_hNodeList.m_hList.end(); itr++)
	{
		(*itr)->m_pPrevLink = NULL;
		(*itr)->m_pPrevLightpath = NULL;
		(*itr)->m_hCost = UNREACHABLE;
		(*itr)->m_dLatency = 0; //-B: needed only for DijkstraLatency (and DijkstraHelperLatency) method
		(*itr)->m_nHops = (UINT)UNREACHABLE;
		if ((*itr)->m_pBackupCost) {
			delete []((*itr)->m_pBackupCost);
			(*itr)->m_pBackupCost = NULL;
		}
		(*itr)->m_nBackupVHops = 0;
	}
}

LINK_COST AbstractGraph::recordMinCostPath(AbstractPath& hPath, UINT nDst)
{
#ifdef DEBUGB
	cout << "-> recordMinCostPath_int" << endl;
#endif // DEBUGB

	AbstractNode *pDst = m_hNodeList.find(nDst);
	if (NULL == pDst)
		return UNREACHABLE;
	return recordMinCostPath(hPath, pDst);
}

/*inline*/ LINK_COST AbstractGraph::recordMinCostPath(AbstractPath& hPath, 
										   AbstractNode* pDst)
{
#ifdef DEBUGB
	cout << "-> recordMinCostPath&" << endl;
#endif // DEBUGB

	if (UNREACHABLE == pDst->m_hCost)
		return UNREACHABLE;

	hPath.deleteContent();
	
	UINT nMaxCount = m_hLinkList.size() + 1;
	UINT nCount = 0;	// to avoid infinite loop in while
	AbstractNode *pNode = pDst;
	while ((pNode->m_pPrevLink) && (nCount++ < nMaxCount)) {
		hPath.m_hLinkList.push_front(pNode->getId());
// NB: design has changed
//		lightpaths will NOT serve as links in other graph as this has given
//		me endless hassle.
		// this may be different from the network lightpaths point to,
		// so look up those nodes
//		if (AbstractLink::LT_Lightpath == pNode->m_pPrevLink->getLinkType()) {
//			pNode = lookUpNodeById(pNode->m_pPrevLink->m_pSrc->getId());
//		} else {
//			pNode = pNode->m_pPrevLink->m_pSrc;
//		}

		pNode = pNode->m_pPrevLink->m_pSrc;
		assert(pNode);
	}
	hPath.m_hLinkList.push_front(pNode->getId());	// src node

	// See if anything went wrong
	if (nCount == nMaxCount) {	//-B: why in the other Dijkstra's function is: if (nCount == nMaxCount+1) ????????
		this->dump(cout);
		THROW_OCH_EXEC("Loop exists in shortest path.");
	}

	hPath.setSrc(hPath.m_hLinkList.front());
	hPath.setDst(pDst->getId());
	return pDst->m_hCost;
}

/*inline*/ LINK_COST AbstractGraph::recordMinCostPath(list<AbstractLink*>& hPath,
	AbstractNode *pDst)
{
#ifdef DEBUGB
	cout << "->recordMinCostPath*&" << endl;
#endif // DEBUGB

	//-B: if destination node's cost is unreachable, it means that no path was found -> the cost wasn't overwritten
	if (UNREACHABLE == pDst->m_hCost)
	{
		return UNREACHABLE;
	}

	//-B: free the AbstractLink list
	hPath.clear();

	UINT nMaxCount = m_hLinkList.size() + 1; //-B: LinkList belongs to m_hGraph
	UINT nCount = 0; 	// to avoid infinite loop in while

						//-B: +++++++++++++ BUILD THE PATH: from destination to source, according to previous link poitner ++++++++++++++
	AbstractLink *pPrevLink = pDst->m_pPrevLink;
	while (pPrevLink && (nCount++ < nMaxCount))
	{
		SimplexLink*pSLink = (SimplexLink*)(pPrevLink);
		hPath.push_front(pPrevLink);
#ifdef DEBUGX
		cout << "\tInserisco simplex link -> ch: " << pSLink->m_nChannel
			<< " - cost: " << pPrevLink->getCost() << " - type: " << pSLink->getSimplexLinkType();
		cout << " - link: ";
		if (pSLink->getSimplexLinkType() == SimplexLink::LT_Channel)
		{
			Vertex*pVertex = (Vertex*)pSLink->getSrc();
			cout << pVertex->m_pOXCNode->getId() << "->";
			Vertex* pVertex2 = (Vertex*)pSLink->getDst();
			cout << pVertex2->m_pOXCNode->getId();
			cout << " (fiber " << pSLink->m_pUniFiber->getSrc()->getId()
				<< "->" << pSLink->m_pUniFiber->getDst()->getId() << ")" << endl;
		}
		else if (pSLink->getSimplexLinkType() == SimplexLink::LT_Lightpath)
		{
			Vertex*pVertex = (Vertex*)pSLink->getSrc();
			cout << pVertex->m_pOXCNode->getId() << "->";
			pVertex = (Vertex*)pSLink->getDst();
			cout << pVertex->m_pOXCNode->getId();
			cout << " (lightpath " << pSLink->m_pLightpath->getSrc()->getId()
				<< "->" << pSLink->m_pLightpath->getDst()->getId() << ")" << endl;
		}
		else {
			cout << endl;
		}
#endif // DEBUGB
		pPrevLink = pPrevLink->getSrc()->m_pPrevLink;
	}

	// See if anything went wrong
	if (nCount == nMaxCount + 1)
	{	//-B: why in the other Dijkstra's function is: if (nCount == nMaxCount) ????????
		this->AbstractGraph::dump(cout);
		THROW_OCH_EXEC("Loop exists in shortest path.");
	}

	//-B: we already have computed the cost in DijkstraHelper method, so it simply returns it
	return pDst->m_hCost;
}

LINK_COST AbstractGraph::Dijkstra(AbstractPath& hMinCostPath, 
								  UINT nSrc, UINT nDst, 
								  LinkCostFunction hLCF)
{
	AbstractNode *pSrc = m_hNodeList.find(nSrc);
	AbstractNode *pDst = m_hNodeList.find(nDst);
	if ((NULL == pSrc) || (NULL == pDst))
		return UNREACHABLE; 

	DijkstraHelper(pSrc,pDst, hLCF);//FABIO 4 dic: modifico 

	// record minimum-cost path
	return recordMinCostPath(hMinCostPath, pDst);
}

LINK_COST AbstractGraph::Dijkstra(list<AbstractLink*>& hMinCostPath, 
			AbstractNode* pSrc, AbstractNode* pDst, LinkCostFunction hLCF)
{
	DijkstraHelper(pSrc, pDst, hLCF); //FABIO 4 dic: modifico 

	// record minimum-cost path
	return recordMinCostPath(hMinCostPath, pDst);
}

//-B: same as Dijkstra but this method is specifically for fronthaul connections:
//	it checks latency constraint during shortest path computation
LINK_COST AbstractGraph::DijkstraLatency(list<AbstractLink*>& hMinCostPath,
	AbstractNode* pSrc, AbstractNode* pDst, LinkCostFunction hLCF)
{
	DijkstraHelperLatency(pSrc, pDst, hLCF);

	// record minimum-cost path
	return recordMinCostPath(hMinCostPath, pDst);
}

//-B: originally taken from the previous Dijkstra. Added a pointer to the connection
LINK_COST AbstractGraph::Dijkstra_BBU(BandwidthGranularity bw, Connection*pCon, list<AbstractLink*>& hMinCostPath,
	AbstractNode* pSrc, AbstractNode* pDst, LinkCostFunction hLCF)
{
	DijkstraHelper_BBU(bw, pCon, pSrc, pDst, hLCF); //FABIO 4 dic: modifico 

									  // record minimum-cost path
	return recordMinCostPath(hMinCostPath, pDst);
}

 void AbstractGraph::DijkstraHelper(AbstractNode* pSrc, 
										  LinkCostFunction hLCF)
{}//FABIO 4 dic: trucco... 

 inline void AbstractGraph::DijkstraHelper(AbstractNode* pSrc, AbstractNode* pDst,
	 LinkCostFunction hLCF)
 {
#ifdef DEBUGB
	 cout << "-> DijkstraHelper" << endl;
#endif // DEBUGB

	 //-B: for each VERTEX, it sets: (*itr)->m_pPrevLink = NULL; (*itr)->m_hCost = UNREACHABLE;
	 //	it does not modify m_bValid attribute obviously
	 reset4ShortestPathComputation();

	 //-B: source node has zero reachability cost
	 pSrc->m_hCost = 0;


	 BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ;
	 list<AbstractNode*>::const_iterator itrNode;


	 //-B: INSERT ALL m_hGraph's NODES IN A LIST, SORTED BY NODE'S COST (not network's nodes! They are graph's vertexes)
	
	 for (itrNode = m_hNodeList.m_hList.begin(); itrNode != this->m_hNodeList.m_hList.end(); itrNode++)
	 {

		 if ((*itrNode)->valid())
		 {
			 //-B: insert function is able to insert the item according to increasing nodes' cost
			
			 hPQ.insert(*itrNode);
		 }
		 else {
		 
			 cout << "Node not valid is from" << ((Vertex*)(*itrNode))->m_pOXCNode->getId() << endl;
		 
		 }
	 }

	 //SimplexLink*pLink;
	 Vertex* pNode = (Vertex*)pSrc;
	 Vertex* pNode2 = (Vertex*)pDst;
	 if (pNode->m_pOXCNode->getId() == pNode2->m_pOXCNode->getId())
	 {
#ifdef DEBUGB
		 cout << "\tvertex src e vertex dst sono nello stesso nodo -> TROVATO!" << endl;
#endif // DEBUGB
		 pDst->m_hCost = 0;
		 return;
	 }

	 list<AbstractLink*>::const_iterator itr;
	 //-B: till this list is not empty
	 while (!hPQ.empty())
	 {
		 int dim = hPQ.m_hContainer.size(); //-B: never used
											//-B: choose vertex with the minimum distance/weight/cost with respect to source node
											//	(the node with the minimum node cost)
		 AbstractNode *pLinkSrc = hPQ.peekMin();

		 //-B: remove this vertex from the list
		 hPQ.popMin();
		 //-B: if this vertex is the destination node, we have found what we are looking for, so return
		 if (pLinkSrc == pDst)
		 {
#ifdef DEBUGB
			 cout << "\tvertex considerato = vertex destinazione -> TROVATO!" << endl;
#endif // DEBUGB
			 return; // FABIO 4 dic
		 }
		 //-B: if this vertex has an infinite cost, go to next iteration of while cycle -> consider next cheapest node
		 if (UNREACHABLE == pLinkSrc->m_hCost)
			 break;

		 //-B: consider all the adjacent outgoing links from selected node
		 for (itr = pLinkSrc->m_hOLinkList.begin(); itr != pLinkSrc->m_hOLinkList.end(); itr++)
		 {
			 
			 //-B: if the link has been invalidated or has an infinite cost, go to the next adjacent link
			 if ((!(*itr)->valid()) || UNREACHABLE == (*itr)->getCost()) {
				 continue;
			 }
		
			 // NB: design has changed
			 //		lightpaths will NOT serve as links in other graph as this has given
			 //		me endless hassle.
			 // this may be different from the network lightpaths point to,
			 // so look up those nodes
			 //			if (AbstractLink::LT_Lightpath == (*itr)->getLinkType()) {
			 //				pRealSrc = lookUpNodeById((*itr)->m_pSrc->getId());
			 //				pRealDst = lookUpNodeById((*itr)->m_pDst->getId());
			 //			}

			 //-B: save selected link cost
			 LINK_COST hLinkCost;
			 switch (hLCF)
			 {
			 case LCF_ByHop:
				 //to count only effective channel links
				 if ((*itr)->getLinkType() == SimplexLink::LT_Channel)
					 hLinkCost = 1;
				 else if ((*itr)->getLinkType() == SimplexLink::LT_Lightpath) //-B: added by me to count all hops done by a lightpath already existing
					 hLinkCost = ((SimplexLink*)(*itr))->m_pLightpath->m_hPRoute.size();
				 else
					 hLinkCost = 0;
				 break;
			 case LCF_ByOriginalLinkCost:
				 hLinkCost = (*itr)->getCost();
				 assert(hLinkCost >= 0); //-B: it was just > 0, I've changed it
				 break;
			 default:
				 DEFAULT_SWITCH;
			 }

			 //-B: save cost of the destination node of the link
			 double c1 = (*itr)->m_pDst->m_hCost;
			 //-B: compute partial cost: reachability cost = source node cost + outgoing link cost
			 double c2 = pLinkSrc->m_hCost + hLinkCost;
			 //-B: if destination node's cost > reachability cost
			

			 if (c1 > c2)
			 {
				 //-B: set destination node's cost = reachability cost
				 (*itr)->m_pDst->m_hCost = pLinkSrc->m_hCost + hLinkCost;
				 //-B: set this outgoing link as (unique) previous link of the destination node of the selected outgoing link
				 (*itr)->m_pDst->m_pPrevLink = *itr; //use previous link instead of node
			 }
		 } //end FOR outgoing links

		   //-B: Sort again the destination node in the list according to the new cost
		 hPQ.buildHeap();

	 } // end WHILE

#ifdef DEBUGB
	 cout << "\tNO PATH AVAILABLE???" << endl;
#endif // DEBUGB

 }
 inline void AbstractGraph::DijkstraHelperYen(AbstractNode* pSrc, AbstractNode* pDst, LinkCostFunction hLCF) {
 
#ifdef DEBUGB
	 cout << "-> DijkstraHelperYen" << endl;
#endif // DEBUGB

	 //-B: for each VERTEX, it sets: (*itr)->m_pPrevLink = NULL; (*itr)->m_hCost = UNREACHABLE;
	 //	it does not modify m_bValid attribute obviously
	 reset4ShortestPathComputation();

	 //-B: source node has zero reachability cost
	 pSrc->m_hCost = 0;

	 BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ;
	 list<AbstractNode*>::const_iterator itrNode;



	 //-B: INSERT ALL m_hGraph's NODES IN A LIST, SORTED BY NODE'S COST (not network's nodes! They are graph's vertexes)
	 for (itrNode = m_hNodeList.begin(); itrNode != m_hNodeList.end(); itrNode++)
	 {
		 if ((*itrNode)->valid())
		 {
			 //-B: insert function is able to insert the item according to increasing nodes' cost
			 hPQ.insert(*itrNode);
		 }
	 }

	 //SimplexLink*pLink;
	 Vertex* pNode = (Vertex*)pSrc;
	 Vertex* pNode2 = (Vertex*)pDst;
	 if (pNode->m_pOXCNode->getId() == pNode2->m_pOXCNode->getId())
	 {
#ifdef DEBUGB
		 cout << "\tvertex src e vertex dst sono nello stesso nodo -> TROVATO!" << endl;
#endif // DEBUGB
		 pDst->m_hCost = 0;
		 return;
	 }

	 list<AbstractLink*>::const_iterator itr;
	 //-B: till this list is not empty
	 while (!hPQ.empty())
	 {
		 int dim = hPQ.m_hContainer.size(); //-B: never used
											//-B: choose vertex with the minimum distance/weight/cost with respect to source node
											//	(the node with the minimum node cost)
		 AbstractNode *pLinkSrc = hPQ.peekMin();
		 //-B: remove this vertex from the list
		 hPQ.popMin();
		 //-B: if this vertex is the destination node, we have found what we are looking for, so return
		 if (pLinkSrc == pDst)
		 {
#ifdef DEBUGB
			 cout << "\tvertex considerato = vertex destinazione -> TROVATO!" << endl;
#endif // DEBUGB
			 return; // FABIO 4 dic
		 }
		 //-B: if this vertex has an infinite cost, go to next iteration of while cycle -> consider next cheapest node
		 if (UNREACHABLE == pLinkSrc->m_hCost)
			 break;

		 //-B: consider all the adjacent outgoing links from selected node
		 for (itr = pLinkSrc->m_hOLinkList.begin(); itr != pLinkSrc->m_hOLinkList.end(); itr++)
		 {
			 //-B: if the link has been invalidated or has an infinite cost, go to the next adjacent link
			 if ((!(*itr)->valid()) || (UNREACHABLE == (*itr)->getCost()))
				 continue;

			 // NB: design has changed
			 //		lightpaths will NOT serve as links in other graph as this has given
			 //		me endless hassle.
			 // this may be different from the network lightpaths point to,
			 // so look up those nodes
			 //			if (AbstractLink::LT_Lightpath == (*itr)->getLinkType()) {
			 //				pRealSrc = lookUpNodeById((*itr)->m_pSrc->getId());
			 //				pRealDst = lookUpNodeById((*itr)->m_pDst->getId());
			 //			}

			 //-B: save selected link cost
			 LINK_COST hLinkCost;
			 switch (hLCF)
			 {
			 case LCF_ByHop:
				 //to count only effective channel links
				 if ((*itr)->getLinkType() == SimplexLink::LT_Channel)
					 hLinkCost = 1;
				 else if ((*itr)->getLinkType() == SimplexLink::LT_Lightpath) //-B: added by me to count all hops done by a lightpath already existing
					 hLinkCost = ((SimplexLink*)(*itr))->m_pLightpath->m_hPRoute.size();
				 else
					 hLinkCost = 0;
				 break;
			 case LCF_ByOriginalLinkCost:
				 hLinkCost = (*itr)->getCost();
				 assert(hLinkCost >= 0); //-B: it was just > 0, I've changed it
				 break;
			 default:
				 DEFAULT_SWITCH;
			 }

			 //-B: save cost of the destination node of the link
			 double c1 = (*itr)->m_pDst->m_hCost;
			 //-B: compute partial cost: reachability cost = source node cost + outgoing link cost
			 double c2 = pLinkSrc->m_hCost + hLinkCost;
			 //-B: if destination node's cost > reachability cost
			 if (c1 > c2)
			 {
				 //-B: set destination node's cost = reachability cost
				 (*itr)->m_pDst->m_hCost = pLinkSrc->m_hCost + hLinkCost;
				 //-B: set this outgoing link as (unique) previous link of the destination node of the selected outgoing link
				 (*itr)->m_pDst->m_pPrevLink = *itr; //use previous link instead of node
			 }
		 } //end FOR outgoing links

		   //-B: Sort again the destination node in the list according to the new cost
		 hPQ.buildHeap();

	 } // end WHILE

#ifdef DEBUGB
	 cout << "\tNO PATH AVAILABLE???" << endl;
#endif // DEBUGB
 
 }


inline void AbstractGraph::DijkstraHelperLatency(AbstractNode* pSrc, AbstractNode* pDst,
	LinkCostFunction hLCF)
{
#ifdef DEBUG
	cout << "-> DijkstraHelperLatency" << endl;
#endif // DEBUGB

	//-B: for each VERTEX, it sets: (*itr)->m_pPrevLink = NULL; (*itr)->m_hCost = UNREACHABLE;
	//	it does not modify m_bValid attribute obviously
	reset4ShortestPathComputation();

	//-B: source node has zero reachability cost
	pSrc->m_hCost = 0;

	//-B: if source node is a small cell belonging to a macro cell, we should add
	//	the extra latency (due to the "hidden" path between sC and MC) to the source vertex (of the MC)
	if (((Vertex*)pSrc)->m_pOXCNode->m_dTrafficGen > MAXTRAFFIC_MACROCELL)
	{
		pSrc->m_dLatency = ((Vertex*)pSrc)->m_pOXCNode->m_dExtraLatency;
	}

#ifdef DEBUGX
	cout << "\tNODO SORGENTE = " << ((Vertex*)pSrc)->m_pOXCNode->getId() << endl;
#endif // DEBUGB

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ;
	list<AbstractNode*>::const_iterator itrNode;

	//-B: INSERT ALL m_hGraph's NODES IN A LIST, SORTED BY NODE'S COST (not network's nodes! They are graph's vertexes)
	for (itrNode = m_hNodeList.begin(); itrNode != m_hNodeList.end(); itrNode++)
	{

		if ((*itrNode)->valid())
		{
			//-B: insert function is able to insert the item according to increasing nodes' cost
			hPQ.insert(*itrNode);
			//cout << "\tHo inserito nodo " << (*itrNode)->getId() << endl;
		}
	}

	//SimplexLink*pLink;
	Vertex*pNode = (Vertex*)pSrc;
	Vertex*pNode2 = (Vertex*)pDst;
	if (pNode->m_pOXCNode->getId() == pNode2->m_pOXCNode->getId())
	{
#ifdef DEBUGX
		cout << "\tvertex src e vertex dst sono nello stesso nodo -> TROVATO!" << endl;
#endif // DEBUG
		pDst->m_hCost = 0;
		return;
	}

	list<AbstractLink*>::const_iterator itr;
	//-B: till this list is not empty
	while (!hPQ.empty())
	{
		int dim = hPQ.m_hContainer.size(); //-B: never used
										   //-B: choose vertex with the minimum distance/weight/cost with respect to source node
										   //	(the node with the minimum node cost)
										   //	-> FOR THE FIRST ITERATION, IT IS THE SOURCE NODE (because it is the only one with m_hCost = 0)
		AbstractNode *pLinkSrc = hPQ.peekMin();
		//-B: remove this vertex from the list
		hPQ.popMin();

		//-B: LATENCY CONSTRAINT ONLY FOR FRONTHAUL CONNECTIONS
		//-B: if this vertex has a latency > than the latency budget, we should not consider it
		// (THIS CONTROL MUST BE DONE BEFORE CHECKING IF pLinkSrc == pDst, ELSE IT CAN ACCEPT A TOO LONG PATH!)
		if (pLinkSrc->m_dLatency > LATENCYBUDGET)
		{
#ifdef DEBUGX
			//cout << "\n\tTOO LONG PATH!!!!!";
			Vertex*pVSrc = (Vertex*)(pLinkSrc);
			//cout << "\n\tlatency per arrivare al nodo " << pVSrc->m_pOXCNode->getId() << " troppo alta: " << pLinkSrc->m_dLatency << endl;
			Vertex*pVLinkSrc, *pVLinkDst;
			pVLinkSrc = (Vertex*)(pLinkSrc->m_pPrevLink->getSrc());
			pVLinkDst = (Vertex*)(pLinkSrc->m_pPrevLink->getDst());
			cout << "\tIl previous link di questo nodo e' il link " << pVLinkSrc->m_pOXCNode->getId() << "->" << pVLinkDst->m_pOXCNode->getId()
			<< " sul canale " << ((SimplexLink*)(pLinkSrc->m_pPrevLink))->m_nChannel << endl;
#endif // DEBUGB
			//this node has been removed from hPQ (--> there is no need to set its cost to UNREACHABLE), go to the next cheapest node
			continue;
		}

		//-B: if this vertex is the destination node, we have found what we are looking for, so return
		if (pLinkSrc == pDst)
		{
#ifdef DEBUGX
			cout << "\tvertex considerato = vertex destinazione -> TROVATO!" << endl;
#endif // DEBUG
			return; // FABIO 4 dic
		}
		//-B: if this vertex (that should be the minimum cost vertwx) has an infinite cost, 
		//	it means that there is no available path -> exit from while cycle
		if (UNREACHABLE == pLinkSrc->m_hCost)
			break;

		//-B: consider all the adjacent outgoing links from selected node
		for (itr = pLinkSrc->m_hOLinkList.begin(); itr != pLinkSrc->m_hOLinkList.end(); itr++)
		{	

			//-B: if the link has been invalidated or has an infinite cost or overcomes the latency budget, go to the next adjacent link
			if ((!(*itr)->valid()) || (UNREACHABLE == (*itr)->getCost()))
				continue;

			// NB: design has changed
			//		lightpaths will NOT serve as links in other graph as this has given
			//		me endless hassle.
			// this may be different from the network lightpaths point to,
			// so look up those nodes
			//			if (AbstractLink::LT_Lightpath == (*itr)->getLinkType()) {
			//				pRealSrc = lookUpNodeById((*itr)->m_pSrc->getId());
			//				pRealDst = lookUpNodeById((*itr)->m_pDst->getId());
			//			}

			//-B: save selected link cost
			LINK_COST hLinkCost;
			switch (hLCF)
			{
				case LCF_ByHop:
					//to count only effective channel links
					if ((*itr)->getLinkType() == SimplexLink::LT_Channel)
						hLinkCost = 1;
					else if ((*itr)->getLinkType() == SimplexLink::LT_Lightpath) //-B: added by me to count all hops done by a lightpath already existing
						hLinkCost = ((SimplexLink*)(*itr))->m_pLightpath->m_hPRoute.size();
					else
						hLinkCost = 0;
					break;
				case LCF_ByOriginalLinkCost:
					hLinkCost = (*itr)->getCost();
					assert(hLinkCost >= 0); //-B: it was just > 0, I've changed it
					break;
				default:
					DEFAULT_SWITCH;
			}

			//-B: save cost of the destination node of the link
			double c1 = (*itr)->m_pDst->m_hCost;
			//-B: compute partial cost: reachability cost = source node cost + outgoing link cost
			double c2 = pLinkSrc->m_hCost + hLinkCost;
			//-B: compute partial latency: reachability latency = source node latency + outgoing link latency ################ -B: ADDED BY ME
			double l1 = pLinkSrc->m_dLatency + ((*itr)->m_latency);

			//-B: if destination node's cost > reachability cost
			if (c1 > c2)
			{
				//-B: set destination node's cost = reachability cost
				(*itr)->m_pDst->m_hCost = pLinkSrc->m_hCost + hLinkCost;
				//-B: set dst node's latency = reachability latency ######################### -B: ADDED BY ME
				(*itr)->m_pDst->m_dLatency = l1;
				//-B: set this outgoing link as (unique) previous link of the destination node of the selected outgoing link
				(*itr)->m_pDst->m_pPrevLink = *itr; //use previous link instead of node
			}
		} //end FOR outgoing links

		  //-B: Sort again the destination node in the list according to the new cost
		hPQ.buildHeap();

	} // end WHILE

#ifdef DEBUG
	cout << "\tNO PATH AVAILABLE???" << endl;
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//-B: it is possible that a the algorithm exits from this function because of latency, and not for cost
	//	consequently, we have to check not only cost, but also latency: OR in recordMinCostPath OR after returning from DijkstraLatency
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	//cin.get();
#endif // DEBUG

}

inline double AbstractGraph::getLinkLatency(float linkLength)
{
	return linkLength*PROPAGATIONLATENCY;
}

//-B: originally taken from previous DijkstraHelper. Added a pointer to the connection
inline void AbstractGraph::DijkstraHelper_BBU(BandwidthGranularity bw, Connection*pCon, AbstractNode* pSrc, AbstractNode* pDst,
	LinkCostFunction hLCF)
{
#ifdef DEBUGB
	cout << "-> DijkstraHelper_BBU" << endl;
#endif // DEBUGB

	//-B: for each VERTEX, it sets: (*itr)->m_pPrevLink = NULL; (*itr)->m_hCost = UNREACHABLE;
	//	it does not modify m_bValid attribute obviously
	reset4ShortestPathComputation();

	//-B: source node has zero reachability cost
	pSrc->m_hCost = 0;

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ;
	list<AbstractNode*>::const_iterator itrNode;

	//-B: INSERT ALL m_hGraph's NODES IN A LIST, SORTED BY NODE'S COST (not network's nodes! They are graph's vertexes)
	for (itrNode = m_hNodeList.begin(); itrNode != m_hNodeList.end(); itrNode++)
	{
		if ((*itrNode)->valid())
		{
			//-B: insert function is able to insert the item according to increasing nodes' cost
			hPQ.insert(*itrNode);
		}
	}

	//SimplexLink*pLink;
	//Vertex*pNode;

	list<AbstractLink*>::const_iterator itr;
	//-B: till this list is not empty
	while (!hPQ.empty())
	{
		int dim = hPQ.m_hContainer.size(); //-B: never used
		//-B: choose vertex with the minimum cost (in the first iteration it corresponds with the source vertex
		//	(the node with the minimum node cost)
		AbstractNode *pLinkSrc = hPQ.peekMin();
		//-B: remove this vertex from the list
		hPQ.popMin();
		//-B: if this vertex is the destination node, we have found what we are looking for, so return
		if (pLinkSrc == pDst)
		{
#ifdef DEBUG
			cout << "\tvertex considerato = vertex destinazione -> TROVATO!" << endl;
#endif // DEBUG
			return; // FABIO 4 dic
		}
		//-B: if this vertex has an infinite cost, go to next iteration of while cycle -> consider next cheapest node
		if (UNREACHABLE == pLinkSrc->m_hCost)
			break;

		//-B: consider all the adjacent outgoing links from selected node
		for (itr = pLinkSrc->m_hOLinkList.begin(); itr != pLinkSrc->m_hOLinkList.end(); itr++)
		{
			//-B: if the link has been invalidated or has an infinite cost, go to the next adjacent link
			if ((!(*itr)->valid()) || (UNREACHABLE == (*itr)->getCost()))
				continue;

			// NB: design has changed
			//		lightpaths will NOT serve as links in other graph as this has given
			//		me endless hassle.
			// this may be different from the network lightpaths point to,
			// so look up those nodes
			//			if (AbstractLink::LT_Lightpath == (*itr)->getLinkType()) {
			//				pRealSrc = lookUpNodeById((*itr)->m_pSrc->getId());
			//				pRealDst = lookUpNodeById((*itr)->m_pDst->getId());
			//			}

			//-B: save selected link cost
			LINK_COST hLinkCost;
			switch (hLCF)
			{
			case LCF_ByHop:
				//to count only effective channel links
				if ((*itr)->getLinkType() == SimplexLink::LT_Channel)
					hLinkCost = 1;
				else
					hLinkCost = 0;
				break;
			case LCF_ByOriginalLinkCost:
				hLinkCost = (*itr)->getCost();
				assert(hLinkCost >= 0); //-B: it was just > 0, I've changed it
				break;
			default:
				DEFAULT_SWITCH;
			}

			//-B: save cost of the destination node of the link
			double c1 = (*itr)->m_pDst->m_hCost;
			//-B: compute partial cost: reachability cost = source node cost + outgoing link cost
			double c2 = pLinkSrc->m_hCost + hLinkCost;
			//-B: if destination node's cost > reachability cost
			if (c1 > c2)
			{
				//-B: set destination node's cost = reachability cost
				(*itr)->m_pDst->m_hCost = pLinkSrc->m_hCost + hLinkCost;
				//-B: set this outgoing link as (unique) previous link of the destination node of the selected outgoing link
				(*itr)->m_pDst->m_pPrevLink = *itr; //use previous link instead of node
			}
		} //end FOR outgoing links

		  //-B: Sort again the destination node in the list according to the new cost
		hPQ.buildHeap();

	} // end WHILE

#ifdef DEBUG
	cout << "\tNO PATH AVAILABLE???" << endl;
#endif // DEBUG

}

//-B: taken from the same method in NetMan
void AbstractGraph::invalidateSimplexLinkDueToFreeStatus(MappedLinkList<UINT, AbstractLink*>pOutLinkList)
{
	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;
	for (itr = pOutLinkList.begin(); itr != pOutLinkList.end(); itr++)
	{
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);
		if (pSimplexLink->m_hFreeCap < OCLightpath)
		{
			pSimplexLink->invalidate();
			//cout << " " << pSimplexLink->m_nChannel;
		}
	}
}

//-B: taken from the same method in Graph class
// Invalidate those SimplexLinks that don't have enough bandwidth
void AbstractGraph::invalidateOutSimplexLinkDueToCap(MappedLinkList<UINT, AbstractLink*>pOutLinkList, UINT nBW)
{
#ifdef DEBUGB
	//cout << "-> invalidateSimplexLinkDueToCap" << endl;
	//cout << "\tInvalido i ch:";
#endif // DEBUGB

	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;

	for (itr = pOutLinkList.begin(); itr != pOutLinkList.end(); itr++) {
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);
		if (pSimplexLink->m_hFreeCap < nBW)
		{
			pSimplexLink->invalidate();
			cout << "\tDueToCap: fiber " << pSimplexLink->m_pUniFiber->getId() <<" - Invalido il ch: " << pSimplexLink->m_nChannel << " - freecap: " << pSimplexLink->m_hFreeCap << endl;
		}
	}
	//cout << endl << endl;
}


// compute minimum-cost path
LINK_COST AbstractGraph::BellmanFord(AbstractPath& hMinCostPath, 
							UINT nSrc, UINT nDst, LinkCostFunction hLCF)
{
	AbstractNode *pSrc = m_hNodeList.find(nSrc);
	AbstractNode *pDst = m_hNodeList.find(nDst);
	if ((NULL == pSrc) || (NULL == pDst))
		return UNREACHABLE;

	BellmanFordHelper(pSrc, hLCF);

	// record minimum-cost path
	return recordMinCostPath(hMinCostPath, pDst);
}

LINK_COST AbstractGraph::BellmanFord(list<AbstractLink*>& hMinCostPath, 
			AbstractNode* pSrc, AbstractNode* pDst, LinkCostFunction hLCF)
{
#ifdef DEBUGB
	cout << "-> BellmanFord" << endl;
#endif // DEBUGB

	BellmanFordHelper(pSrc, hLCF);

	// record minimum-cost path
	return recordMinCostPath(hMinCostPath, pDst);
}

inline void AbstractGraph::BellmanFordHelper(AbstractNode *pSrc, 
											 LinkCostFunction hLCF)
{
#ifdef DEBUGB
	cout << "-> BellmanFordHelper" << endl;
	cout << "\tNumber of nodes: " << this->numberOfNodes;
	cout << "\tLink list size (m_hGraph): " << m_hLinkList.size() << endl;
	cout << "\tNode List size: " << m_hNodeList.size() << endl;
	cin.get();
#endif // DEBUGB

	reset4ShortestPathComputation();
	pSrc->m_hCost = 0;

	list<AbstractLink*>::iterator itrLink;
	int N = this->getNumberOfNodes();
	int i;
	for (i = 1; i < N; i++)
	{
		for (itrLink = m_hLinkList.begin(); itrLink != m_hLinkList.end(); itrLink++)
		{
			// the link has been invalidated
			if (!(*itrLink)->valid())
				continue;

			AbstractNode *pSrc = (*itrLink)->m_pSrc;
			AbstractNode *pDst = (*itrLink)->m_pDst;
			assert(pSrc && pDst);
			// NB: design has changed
			//		lightpaths will NOT serve as links in other graph as this has given
			//		me endless hassle.
						// this may be different from the network lightpaths point to,
						// so look up those nodes
			//			if (AbstractLink::LT_Lightpath == (*itrLink)->getLinkType()) {
			//				pSrc = lookUpNodeById((*itrLink)->m_pSrc->getId());
			//				pDst = lookUpNodeById((*itrLink)->m_pDst->getId());
			//			}
			assert(pSrc && pDst);

			if ((UNREACHABLE == pSrc->m_hCost) || (UNREACHABLE == (*itrLink)->getCost()))
				continue;
			LINK_COST hLinkCost;
			switch (hLCF) {
			case LCF_ByHop:
				hLinkCost = 1;
				break;
			case LCF_ByOriginalLinkCost:
				hLinkCost = (*itrLink)->getCost();
				break;
			default:
				DEFAULT_SWITCH;
			}
			if (pDst->m_hCost > pSrc->m_hCost + hLinkCost)
			{
				pDst->m_hCost = pSrc->m_hCost + hLinkCost;
				pDst->m_pPrevLink = *itrLink;	// use previous link instead of previous node
			}
		}
	}
}

LINK_COST AbstractGraph::Suurballe(AbstractPath& hPath1, AbstractPath& hPath2, 
							  UINT nSrc, UINT nDst, LinkCostFunction hLCF)
{
	AbstractNode *pSrc = m_hNodeList.find(nSrc);
	AbstractNode *pDst = m_hNodeList.find(nDst);
	if ((NULL == pSrc) || (NULL == pDst))
		return UNREACHABLE;
	
	return Suurballe(hPath1, hPath2, pSrc, pDst, hLCF);
}

LINK_COST AbstractGraph::Suurballe(AbstractPath& hPath1, 
								   AbstractPath& hPath2, 
								   AbstractNode* pSrc, AbstractNode* pDst,
								   LinkCostFunction hLCF)
{
	return SuurballeHelper(hPath1, hPath2, pSrc, pDst, hLCF);
}

inline LINK_COST AbstractGraph::SuurballeHelper(AbstractPath& hPath1, 
										  AbstractPath& hPath2, 
										  AbstractNode* pSrc, AbstractNode* pDst,
										  LinkCostFunction hLCF)
{
	LINK_COST hTotalCost, hPathCost;
	list<AbstractLink*> hNewLinkList;
//	BellmanFordHelper(pSrc, hLCF);
	try {
		DijkstraHelper(pSrc,pDst, hLCF);//fabio 8 dic
	} catch (...) {
		cerr<<"- OUT OF MEMORY: "<<__FILE__<<":"<<__LINE__<<endl;
	}
	try {
		hTotalCost = recordMinCostPath(hPath1, pDst);
	} catch (...) {
		cerr<<"- OUT OF MEMORY: "<<__FILE__<<":"<<__LINE__<<endl;
	}
	if (UNREACHABLE == hTotalCost) return UNREACHABLE;

	invalidateMinCostPath(pDst);
	reverseMinCostPath(hNewLinkList, pDst);
	// NB: here use LCF_ByOriginalLinkCost mandantorily
	BellmanFordHelper(pSrc, LCF_ByOriginalLinkCost);
	try {
		hPathCost = recordMinCostPath(hPath2, pDst);
	} catch (...) {
		cerr<<"- OUT OF MEMORY: "<<__FILE__<<":"<<__LINE__<<endl;
	}
	if (UNREACHABLE == hPathCost) {
		SuurballeRestorer(hNewLinkList);
		return UNREACHABLE;
	}
	hTotalCost += hPathCost;

	groupTwoPaths(hPath1, hPath2);

	SuurballeRestorer(hNewLinkList);
	return hTotalCost;
}

// NB: assume simple graph, should invalidate all the links in the same SRG as
//     the min-cost path if not simple graph
inline void AbstractGraph::invalidateMinCostPath(AbstractNode* pDst)
{
	assert(UNREACHABLE != pDst->m_hCost);

	AbstractLink *pPrevLink = pDst->m_pPrevLink;
	while (pPrevLink) {
		pPrevLink->invalidate();
		pPrevLink = pPrevLink->m_pSrc->m_pPrevLink;
	}
}

inline void AbstractGraph::SuurballeRestorer(list<AbstractLink*>& hNewLinkList)
{
	list<AbstractLink*>::const_iterator itr;
	for (itr=hNewLinkList.begin(); itr!=hNewLinkList.end(); itr++) {
		this->removeLink(*itr);
		delete (*itr);
	}

	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		(*itr)->restoreCost();
		(*itr)->validate();
	}
}

inline void AbstractGraph::reverseMinCostPath(list<AbstractLink*>& hNewLinkList,
											  AbstractNode* pDst)
{
	assert(UNREACHABLE != pDst->m_hCost);
	
	AbstractLink *pPrevLink = pDst->m_pPrevLink;
	while (pPrevLink) {
		// remove original one, assuming simple graph
//		list<AbstractLink*>::iterator itrI, itrO;
//		for (itrI=pCurr->m_hILinkList.begin(); 
//		(itrI!=pCurr->m_hILinkList.end()) && ((*itrI)->m_pSrc!=pPrev); itrI++)
//			NULL;
//		assert(itrI!=pCurr->m_hILinkList.end());
//		pLink = *itrI;
//		pCurr->m_hILinkList.erase((*itrI)->getId());
//
//		for (itrO=pPrev->m_hOLinkList.begin();
//		(itrO!=pPrev->m_hOLinkList.end()) && ((*itrO)->m_pDst!=pCurr); itrO++)
//			NULL;
//		assert(itrO!=pPrev->m_hOLinkList.end());
//		pPrev->m_hOLinkList.erase((*itrO)->getId());

		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pNode = pPrevLink->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pNode->m_hOLinkList.begin();
		(itrO!=pNode->m_hOLinkList.end()) && ((*itrO)->m_pDst!=pPrevLink->m_pSrc); 
		itrO++)	NULL;
		// no link at the opposite direction, add one
		if (itrO == pNode->m_hOLinkList.end()) {
			int nNewLinkId = getNextLinkId();
#ifdef _OCHDEBUG5
			{
				cout<<"- Add opposite link: "<<nNewLinkId<<" ";
				cout<<pPrevLink->m_pDst->getId()<<"->";
				cout<<pPrevLink->m_pSrc->getId()<<endl;
				this->dump(cout);
			}
#endif
			pOppositeLink = this->addLink(nNewLinkId, 
				pPrevLink->m_pDst->getId(), pPrevLink->m_pSrc->getId(),
				pPrevLink->m_hCost, 1);
			assert(pOppositeLink);
			hNewLinkList.push_back(pOppositeLink);
		} else {
			pOppositeLink = *itrO;
		}
		LINK_COST hCost = pOppositeLink->m_hCost;
		LINK_COST hNewCost;
		if (hCost > 0)
			hNewCost = 0 - hCost;
		else if (hCost < 0)
			hNewCost = 2 * hCost;
		else
			hNewCost = -1;
		pOppositeLink->modifyCost(hNewCost);

		pPrevLink = pPrevLink->m_pSrc->m_pPrevLink;
	}
}

// tested in testGroupTwoPaths in GPMain.cpp
void AbstractGraph::groupTwoPaths(AbstractPath& hPath1, AbstractPath& hPath2)
{
	list<UINT>::iterator itr1 = hPath1.m_hLinkList.begin();
	while (itr1 != hPath1.m_hLinkList.end()) {
		list<UINT>::iterator itr1Next = itr1;
		itr1Next++;
		if (itr1Next == hPath1.m_hLinkList.end())
			break;
		list<UINT>::iterator itr2 = hPath2.m_hLinkList.begin();
		while ((itr2 != hPath2.m_hLinkList.end()) && (*itr2 != *itr1Next))
			itr2++;
		if (itr2 == hPath2.m_hLinkList.end()) {
			itr1++;
			continue;
		}
		list<UINT>::iterator itr2Next = itr2;
		itr2Next++;
		if ((itr2Next == hPath2.m_hLinkList.end()) || (*itr2Next != *itr1)) {
			itr1++;
			continue;
		}
		
		// handling situation like <7, 5, 6> & <6, 5, 7>
		while ((itr1Next != hPath1.m_hLinkList.end())
			&& (itr2 != hPath2.m_hLinkList.begin())
			&& (*itr1Next == *itr2)) {
			itr1Next++;
			itr2--;
		}
//		assert((NULL != itr1Next) && (NULL != itr2));
		itr1Next--;
		itr2++;
		
		list<UINT>::iterator itrImmediateNext = itr1;
		itrImmediateNext++;
		if (itrImmediateNext == itr1Next) {	// erase one link each
			itr1 = hPath1.m_hLinkList.erase(itr1Next);
			itr2 = hPath2.m_hLinkList.erase(itr2Next);
		} else {
			itr1 = hPath1.m_hLinkList.erase(itrImmediateNext, ++itr1Next);
			itr2 = hPath2.m_hLinkList.erase(++itr2, ++itr2Next);
		}

		list<UINT> temp;
		// NB: 
		// Original code (one line)
		// temp.splice(temp.end(), hPath1, itr1);
		// Modified code
		temp.splice(temp.end(), hPath1.m_hLinkList, itr1, hPath1.m_hLinkList.end());
		hPath1.m_hLinkList.splice(hPath1.m_hLinkList.end(), hPath2.m_hLinkList,
			itr2, hPath2.m_hLinkList.end());
		hPath2.m_hLinkList.splice(hPath2.m_hLinkList.end(), temp,
			temp.begin(), temp.end());

		list<UINT>::iterator itrTemp = itr1;
		itr1 = itr2;
		itr2 = itrTemp;
	}
}

void AbstractGraph::validateAllLinks()
{
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
		(*itr)->validate();
}

void AbstractGraph::validateAllNodes()
{
	list<AbstractNode*>::const_iterator itr;
	for (itr = m_hNodeList.begin(); itr != m_hNodeList.end(); itr++)
	{
		(*itr)->costiParzialiFlag = false; //FABIO 15 marzo
		(*itr)->validate();
	}
}

void AbstractGraph::restoreLinkCost()
{
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
		(*itr)->restoreCost();
}

void AbstractGraph::resetLinks()
{
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		(*itr)->validate();
		(*itr)->restoreCost();
	}
}

void AbstractGraph::removeLink(AbstractLink *pLink)
{
#ifdef DEBUGB
	cout << "-> removeLink" << endl;
#endif // DEBUGB

	assert(pLink);

	UINT nLinkId = pLink->getId();
	pLink->getSrc()->m_hOLinkList.erase(nLinkId);
	pLink->getDst()->m_hILinkList.erase(nLinkId);
	m_hLinkList.erase(nLinkId);
}

// Generalized Floyd algorithm
// compute k-shortest loop path lengths between every node pair
void AbstractGraph::Floyd(UINT nNumberOfPaths, LinkCostFunction hLCF)
{
	FloydHelper(nNumberOfPaths, hLCF);
}

inline void AbstractGraph::FloydHelper(UINT nNumberOfPaths, 
									   LinkCostFunction hLCF)
{
	int N = this->getNumberOfNodes();
	int  u, i, j;
	UINT k;
	
	LINK_COST ***pppD1;
	LINK_COST ***pppD2;
	pppD1 = new LINK_COST**[N];
	for (i=0; i<N; i++) {
		pppD1[i] = new LINK_COST*[N];
		for (j=0; j<N; j++)
			pppD1[i][j] = new LINK_COST[nNumberOfPaths];
	}
	pppD2 = new LINK_COST**[N];
	for (i=0; i<N; i++) {
		pppD2[i] = new LINK_COST*[N];
		for (j=0; j<N; j++)
			pppD2[i][j] = new LINK_COST[nNumberOfPaths];
		for (k=0; k<nNumberOfPaths; k++)
			pppD2[i][i][k] = 0;
	}

	genCostMatrix(pppD1, nNumberOfPaths, hLCF);
	


	LINK_COST ***pTempMatrix;
	LINK_COST hTempCost;
	LINK_COST *pTempCost = new LINK_COST[nNumberOfPaths];
	UINT  k1, k2;
	for (u=0; u<N; u++) {
		for (i=0; i<N; i++) for (j=0; j<N; j++) {
			if (i == j)
				continue;
			for (k=0; k<nNumberOfPaths; k++)
				pTempCost[k] = UNREACHABLE;
			for (k1=0; (k1<nNumberOfPaths) && (pppD1[i][u][k1]<UNREACHABLE); 
			k1++) {
				for (k2=0; (k2<nNumberOfPaths) && (pppD1[u][j][k2]<UNREACHABLE);
				k2++) {
					hTempCost = pppD1[i][u][k1] + pppD1[u][j][k2];
					for (k=0; (k<nNumberOfPaths) && (pTempCost[k]<=hTempCost);
					k++) NULL;
					if (hTempCost == pTempCost[k-1])
						continue;	// cost should be distinct
					if (k == (nNumberOfPaths - 1))
						pTempCost[k] = hTempCost;
					else if (k < (nNumberOfPaths - 1)) {
						UINT  nIndex;
						for (nIndex=nNumberOfPaths-1; nIndex>k; nIndex--)
							pTempCost[nIndex] = pTempCost[nIndex-1];
						pTempCost[k] = hTempCost;
					}
				} // for k2
			} // for k1
			k1 = 0;
			k2 = 0;
			k=0;
			while (k<nNumberOfPaths) {
				if (pTempCost[k1] < pppD1[i][j][k2]) {
					pppD2[i][j][k] = pTempCost[k1];
					k1++;
				} else if (pTempCost[k1] == pppD1[i][j][k2]) {
					pppD2[i][j][k] = pTempCost[k1];
					k1++;
					k2++;	// cost should be distinct
				} else {
					pppD2[i][j][k] = pppD1[i][j][k2];
					k2++;
				}
				k++;
			}
		} // for i j
		pTempMatrix = pppD1;
		pppD1 = pppD2;
		pppD2 = pTempMatrix;


	}

	if (pppD1) {
		for (i=0; i<N; i++) {
			for (j=0; j<N; j++)
				delete []pppD1[i][j];
			delete []pppD1[i];
		}
		delete []pppD1;
	}
	if (pppD2) {
		for (i=0; i<N; i++) {
			for (j=0; j<N; j++)
				delete []pppD2[i][j];
			delete []pppD2[i];
		}
		delete []pppD2;
	}
}

void AbstractGraph::genCostMatrix(LINK_COST ***pppCost, 
								  UINT nNumberOfPaths, 
								  LinkCostFunction hLCF)
{
	int N = this->getNumberOfNodes();
	int i, j;
	UINT k;
	for (i=0; i<N; i++) {
		for (j=0; j<N; j++)
			for (k=0; k<nNumberOfPaths; k++) {
				if (i == j)
					pppCost[i][j][k] = 0;
				else
					pppCost[i][j][k] = UNREACHABLE;
			}
	}

	list<AbstractLink*>::iterator itrLink;
	for (itrLink=m_hLinkList.begin(); itrLink!=m_hLinkList.end(); 
	itrLink++) {
		// the link has been invalidated
		if (!(*itrLink)->valid()) continue;	

		AbstractNode *pSrc = (*itrLink)->m_pSrc;
		AbstractNode *pDst = (*itrLink)->m_pDst;
		assert(pSrc && pDst);

		if (UNREACHABLE == (*itrLink)->getCost())
			continue;
		LINK_COST hLinkCost;
		switch (hLCF) {
		case LCF_ByHop:
			hLinkCost = 1;
			break;
		case LCF_ByOriginalLinkCost:
			hLinkCost = (*itrLink)->getCost();
			break;
		default:
			DEFAULT_SWITCH;
		}

		LINK_COST *pCost = pppCost[pSrc->getId()][pDst->getId()];
		for (k=0; (k<nNumberOfPaths) && (pCost[k]<=hLinkCost); k++) NULL;
		if (k == (nNumberOfPaths - 1))
			pCost[k] = hLinkCost;
		else if (k < (nNumberOfPaths - 1)) {
			for (i=nNumberOfPaths-1; i>k; i--)
				pCost[i] = pCost[i-1];
			pCost[k] = hLinkCost;
		}
	}
}

// compute K-shortest loopless paths
void AbstractGraph::Yen(list<AbsPath*>& hKPaths, 
						UINT nSrc, UINT nDst, 
						UINT nNumberOfPaths, LinkCostFunction hLCF)
{
	AbstractNode *pSrc = m_hNodeList.find(nSrc);
	AbstractNode *pDst = m_hNodeList.find(nDst);
	if ((NULL == pSrc) || (NULL == pDst))
		return;

	YenHelper(hKPaths, pSrc, pDst, nNumberOfPaths, hLCF);
}

void AbstractGraph::Yen(list<AbsPath*>& hKPaths, 
						AbstractNode* pSrc, AbstractNode* pDst, 
						UINT nNumberOfPaths, LinkCostFunction hLCF)
{
	YenHelper(hKPaths, pSrc, pDst, nNumberOfPaths, hLCF);
}

void AbstractGraph::YenWP(list<AbsPath*>& hKPaths, 
						AbstractNode* pSrc, AbstractNode* pDst, 
						UINT nNumberOfPaths, LinkCostFunction hLCF)
{
	YenHelperWP(hKPaths, pSrc, pDst, nNumberOfPaths, hLCF);
}

inline void AbstractGraph::YenHelper(list<AbsPath*>& hKPaths, 
									 AbstractNode* pSrc, AbstractNode* pDst, 
									 UINT nNumberOfPaths, LinkCostFunction hLCF)
{
	BinaryHeap<AbsPath*, vector<AbsPath*>, PAbsPathComp> hPQ;
	LINK_COST hCost;
	list<AbstractLink*> hMinCostPath;
	
	DijkstraHelperYen(pSrc, pDst, hLCF); //FABIO 4 dic: modificato

	hCost = recordMinCostPath(hMinCostPath, pDst);

	if (UNREACHABLE == hCost)
		return;	// graph disconnected
	hPQ.insert(new AbsPath(hMinCostPath));

	list<AbstractLink*>::const_iterator itr, itrSeg;
	AbsPath *pCurrentPath;
	UINT  k=0;
	while (k<nNumberOfPaths) {
		pCurrentPath = hPQ.peekMin();
		hPQ.popMin();

		if ((k+1) == nNumberOfPaths) {
			hKPaths.push_back(pCurrentPath);
			break;
		}

		// a minor optimization
		if ((k+2) == nNumberOfPaths) {
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
		for (itr=pCurrentPath->m_hLinkList.begin(); 
		itr!=pCurrentPath->m_hLinkList.end(); itr++) {
			list<AbstractNode*> hDirtyNodes;	// nodes to invalidate/validate
			list<AbstractLink*> hDirtyLinks;
			// disable first segment nodes to ensure loopless
			for (itrSeg=hFirstSeg.begin(); itrSeg!=hFirstSeg.end(); itrSeg++) {
				(*itrSeg)->m_pSrc->invalidate();
				hDirtyNodes.push_back((*itrSeg)->m_pSrc);
			}
			// no branch
			(*itr)->invalidate();
			hDirtyLinks.push_back(*itr);
			list<AbsPath*>::const_iterator itrKPaths;
			for (itrKPaths=hKPaths.begin(); itrKPaths!=hKPaths.end(); 
			itrKPaths++) {
				bool bSameRoot = true;
				list<AbstractLink*>::const_iterator itrK;
				for (itrSeg=hFirstSeg.begin(), 
					itrK=(*itrKPaths)->m_hLinkList.begin(); 
				(itrSeg!=hFirstSeg.end()) &&
					(itrK!=(*itrKPaths)->m_hLinkList.end()) && bSameRoot;
				itrSeg++, itrK++) {
					if ((*itrSeg) != (*itrK))
						bSameRoot = false;
				}
				if (bSameRoot && (itrSeg == hFirstSeg.end()) 
					&& (itrK != (*itrKPaths)->m_hLinkList.end())) {
					(*itrK)->invalidate();
					hDirtyLinks.push_back(*itrK);
				}
			}
			
			DijkstraHelperLatency((*itr)->m_pSrc,pDst, hLCF);//FABIO 4 dic
			// enable nodes/links
			list<AbstractNode*>::const_iterator itrDNodes;
			list<AbstractLink*>::const_iterator itrDLinks;
			for (itrDNodes=hDirtyNodes.begin(); itrDNodes!=hDirtyNodes.end();
			itrDNodes++) {
				(*itrDNodes)->validate();
			}
			for (itrDLinks=hDirtyLinks.begin(); itrDLinks!=hDirtyLinks.end();
			itrDLinks++) {
				(*itrDLinks)->validate();
			}

			hCost = recordMinCostPath(hSecondSeg, pDst);
			if (UNREACHABLE != hCost) {
				AbsPath *pPath = new AbsPath();
				pPath->m_hLinkList = hFirstSeg;
				pPath->m_hLinkList.splice(pPath->m_hLinkList.end(), hSecondSeg);
				pPath->calculateCost();

				// added on May 6th, 2003 to handle duplicate paths
				bool bDuplicate = false;
				vector<AbsPath*>::const_iterator itrTmp;
				for (itrTmp=hPQ.m_hContainer.begin(); 
				((itrTmp!=hPQ.m_hContainer.end()) && (!bDuplicate));
				itrTmp++) {
					if ((**itrTmp) == *pPath) {
						bDuplicate = true;
					}
				}
				// insert to binary heap if it's not a duplicate
				if (!bDuplicate) {
					hPQ.insert(pPath);
				} else {
					delete pPath;
				}
			}

			hFirstSeg.push_back(*itr);
		} // for itr

		hKPaths.push_back(pCurrentPath);
		k++;
		if (hPQ.empty())
			break;	// done
	}

	// release memory
	vector<AbsPath*>::const_iterator itrTmp;
	for (itrTmp=hPQ.m_hContainer.begin(); itrTmp!=hPQ.m_hContainer.end();
	itrTmp++) {
		delete (*itrTmp);
	}
}


LINK_COST AbstractGraph::SPPBw(AbsPath* path,AbstractNode* pSrc,AbstractNode* pDst,int nchannels,bool isBackup){

	/////FASE 1: ricerco percorso con Dijkstra
LINK_COST hCost;
list<AbstractLink*> hMinCostPath;
int selectedLambda;
DijkstraHelper(pSrc,pDst, AbstractGraph::LCF_ByOriginalLinkCost);//FABIO 2 marzo
	hCost = recordMinCostPath(hMinCostPath, pDst);
if(hCost==UNREACHABLE) return UNREACHABLE;
	///FASE 2: verifico il vicolo di continuita delle lambda sul percorso trovato

	list<AbstractLink*> ::iterator linkItr;
	valarray<double> contResult(1,nchannels);//Verfica
	valarray<bool> boolResult(true,nchannels);//Verfica per calcolo backup

	if (isBackup==false)
	{
		for(linkItr=hMinCostPath.begin();linkItr!=hMinCostPath.end();linkItr++)//ricerco il percorso minimo
		{
			contResult=(*linkItr)->wlOccupation*contResult;
		}
		
		//scelgo una lamda valida tra la label disponibile (random)
		int i;
		vector<int> validLambdas;
		for(i=0;i<contResult.size() ;i++)
			if (contResult[i]==1) validLambdas.push_back(i);
			
			int sz=validLambdas.size();
			if(sz==0)
				return UNREACHABLE;
			double val=double(rand()) / (RAND_MAX+1);
			int selectedIndex=(int)(val * (validLambdas.size())) ;
			 selectedLambda=validLambdas.at(selectedIndex);
	}
	else//calcolo sul backup...
	{
		for(linkItr=hMinCostPath.begin();linkItr!=hMinCostPath.end();linkItr++)//ricerco il percorso minimo
		{
			boolResult=((*linkItr)->wlOccupationBCK<UNREACHABLE)&&boolResult;//diventa false se moltiplico per infinito
			contResult=(*linkItr)->wlOccupationBCK*contResult;
		}
		int i;
		for (i=0;i<contResult.size();i++)
			if (boolResult[i]==false) contResult[i]=UNREACHABLE;
		//creo due gruppi di lambda valide: sharabili(costo<1) e non(costi=1)
		
		vector<int> validLambdasSH;
		vector<int> validLambdas;
		for(i=0;i<contResult.size() ;i++)
			if (contResult[i]<1) validLambdasSH.push_back(i);
			
			int szSH=validLambdasSH.size();
			if(szSH==0)
			{
				for(i=0;i<contResult.size() ;i++)
					if (contResult[i]==1) validLambdas.push_back(i);
					
					int sz=validLambdas.size();
					if(sz==0) 
						return UNREACHABLE;
					else
					{
						double val=double(rand()) / (RAND_MAX+1);
						int selectedIndex=(int)(val * (validLambdas.size())) ;
						selectedLambda=validLambdas.at(selectedIndex);
					}
			}
			else
			{
				double val=double(rand()) / (RAND_MAX+1);
				int selectedIndex=(int)(val * (validLambdasSH.size())) ;
				selectedLambda=validLambdasSH.at(selectedIndex);
			}
	}//fine else

///creazione percorso primario



path->m_hLinkList=hMinCostPath; //POSSIBILE LEAK

path->wlAssigned=selectedLambda;

return hCost;


}


LINK_COST AbstractGraph::SPPCh(AbsPath* path,AbstractNode* pSrc,AbstractNode* pDst,int nchannels,bool isBackup){

	/////FASE 1: ricerco percorso con Dijkstra
LINK_COST hCost;
list<AbstractLink*> hMinCostPath;
int selectedLambda;
DijkstraHelper(pSrc,pDst, AbstractGraph::LCF_ByOriginalLinkCost);//FABIO 2 marzo
	hCost = recordMinCostPath(hMinCostPath, pDst);
if(hCost==UNREACHABLE) return UNREACHABLE;
	///FASE 2: verifico il vicolo di continuita delle lambda sul percorso trovato

	list<AbstractLink*> ::iterator linkItr;
	valarray<double> contResult(1,nchannels);//Verfica
	valarray<bool> boolResult(true,nchannels);//Verfica per calcolo backup

	int i;
bool verifyFlag=false;

	if (isBackup==false)
	{
	
StartFor:
			for(linkItr=hMinCostPath.begin();linkItr!=hMinCostPath.end();linkItr++)//ricerco il percorso minimo
			{
				double sum=0;
				boolResult=((*linkItr)->wlOccupation<UNREACHABLE)&&boolResult;//diventa false se moltiplico per infinito
				contResult=(*linkItr)->wlOccupation*contResult;
				for (i=0;i<boolResult.size();i++)
				{
					sum+=boolResult[i];
				}
				
				if (sum==0){ //DEVO ELIMINARE LINK DALLA COMPUTAZIONE
					(*linkItr)->invalidate();
					hMinCostPath.clear();
					DijkstraHelper(pSrc,pDst, AbstractGraph::LCF_ByOriginalLinkCost);//FABIO 2 marzo
					hCost = recordMinCostPath(hMinCostPath, pDst);
					if(hCost==UNREACHABLE) 
							{
				//		cout<<"SF"<<endl;
					return UNREACHABLE;
				}
						
					 boolResult=valarray<bool> (true,nchannels);//Verfica per calcolo backup
					 contResult=valarray<double>(1,nchannels);
					goto StartFor;
				}
			
			}
			
		
		//scelgo una lamda valida tra la label disponibile (random)
		
		vector<int> validLambdas;
		for(i=0;i<contResult.size() ;i++)
			if (contResult[i]==1) validLambdas.push_back(i);
			
			int sz=validLambdas.size();
			if(sz==0)
						{
						cout<<"noValidLamsPRIM"<<endl;
					return UNREACHABLE;
				}
			double val=double(rand()) / (RAND_MAX+1);
			int selectedIndex=(int)(val * (validLambdas.size())) ;
			 selectedLambda=validLambdas.at(selectedIndex);
	}
	else//calcolo sul backup...
	{
	/*	for(linkItr=hMinCostPath.begin();linkItr!=hMinCostPath.end();linkItr++)//ricerco il percorso minimo
		{
			boolResult=((*linkItr)->wlOccupationBCK<UNREACHABLE)&&boolResult;//diventa false se moltiplico per infinito
			contResult=(*linkItr)->wlOccupationBCK*contResult;
		}
		int i;
		for (i=0;i<contResult.size();i++)
			if (boolResult[i]==false) contResult[i]=UNREACHABLE;*/
		//creo due gruppi di lambda valide: sharabili(costo<1) e non(costi=1)
		

StartFor2:
			for(linkItr=hMinCostPath.begin();linkItr!=hMinCostPath.end();linkItr++)//ricerco il percorso minimo
			{
				double sum=0;
				valarray<bool> boolResultLink(true,nchannels);
				bool shFlag=false;
				//LOGICA: per il link sharabili,i candidati sono SOLO i canali sharabili
					for (i=0;i<boolResultLink.size();i++)
					{
							
						if ((*linkItr)->wlOccupationBCK[i]<1) 
							{
								boolResultLink[i]=true;
								shFlag=true;
							}


					}
				if(shFlag==false)
				{
				   for (i=0;i<boolResultLink.size();i++)
					{
							
						if ((*linkItr)->wlOccupationBCK[i]==1) 
							{
								boolResultLink[i]=true;
								
							}


					}
				}
				boolResult=boolResultLink&&boolResult;//diventa false se moltiplico per infinito
				contResult=(*linkItr)->wlOccupationBCK*contResult;
				
				for (i=0;i<boolResult.size();i++)
				{
					sum+=boolResult[i];
				}
				
				if (sum==0){ //DEVO ELIMINARE LINK DALLA COMPUTAZIONE
					(*linkItr)->invalidate();
					hMinCostPath.clear();
					DijkstraHelper(pSrc,pDst, AbstractGraph::LCF_ByOriginalLinkCost);//FABIO 2 marzo
					hCost = recordMinCostPath(hMinCostPath, pDst);
					if(hCost==UNREACHABLE)
							{
	//					cout<<"SF2"<<endl;
					return UNREACHABLE;
				}
					 boolResult=valarray<bool> (true,nchannels);//Verfica per calcolo backup
					   contResult=valarray<double>(1,nchannels);
					goto StartFor2;
				}//IF SUM==0
			
			}


	for (i=0;i<contResult.size();i++)
			if (boolResult[i]==false) contResult[i]=UNREACHABLE;
		vector<int> validLambdasSH;
		vector<int> validLambdas;
		for(i=0;i<contResult.size() ;i++)
			if (contResult[i]<1) validLambdasSH.push_back(i);
			
			int szSH=validLambdasSH.size();
			if(szSH==0)
			{
				for(i=0;i<contResult.size() ;i++)
					if (contResult[i]==1) validLambdas.push_back(i);
					
					int sz=validLambdas.size();
					if(sz==0) 
						{
				//		cout<<"noValidLamsBACK"<<endl;
					return UNREACHABLE;
				}
					else
					{
						double val=double(rand()) / (RAND_MAX+1);
						int selectedIndex=(int)(val * (validLambdas.size())) ;
						selectedLambda=validLambdas.at(selectedIndex);
					}
			}
			else
			{
				double val=double(rand()) / (RAND_MAX+1);
				int selectedIndex=(int)(val * (validLambdasSH.size())) ;
				selectedLambda=validLambdasSH.at(selectedIndex);
			}
	}//fine else

///creazione percorso primario



path->m_hLinkList=hMinCostPath; //POSSIBILE LEAK

path->wlAssigned=selectedLambda;

return hCost;


}


LINK_COST AbstractGraph::myAlg(AbsPath&path, AbstractNode* pSrc, AbstractNode* pDst, int nchannels)
{
#ifdef DEBUGB
	cout << "-> myAlg" << endl;
#endif // DEBUGB

	bool changeFlag = true;
	//vector<bool> changes(this->m_hNodeList.	m_hList.size());
	set<int> changes;

	//reset della status list di tutti i nodi, tranne che per il sorgente;
	list<AbstractNode*>::iterator itrnodo;

	for (itrnodo = m_hNodeList.m_hList.begin(); itrnodo != m_hNodeList.m_hList.end(); itrnodo++)
	{
		(*itrnodo)->costi = valarray<LINK_COST>(UNREACHABLE, nchannels);
		(*itrnodo)->prev = vector<AbstractLink*>(nchannels);
		(*itrnodo)->updates = vector<bool>(nchannels, false);
	}
	pSrc->costi = valarray<LINK_COST>(0.0, nchannels);
	pSrc->prev = vector<AbstractLink*>(nchannels, NULL); //verificare se resta effettivamente in lista...
	int i;
	//for (i=0;i<nchannels;i++)				
	//pSrc->updates.insert(i);
	pSrc->updates = vector<bool>(nchannels, true);
	changes.insert(pSrc->getId()); //primo nodo su cui iniziare la scansione
	int dstId = pDst->getId();
	AbstractNode* pNode;
	while (changes.size()) //finchè changes.size > 0
	{
		//changeFlag=false;
		//int i;
		//for(i=0;i<changes.size();i++)
		//{
		i = (*changes.begin());
		changes.erase(changes.begin());
		if (i == dstId)
			continue;
		//if(changes[i])//se changes[i]==true
		//{
		pNode = lookUpNodeById(i);
		pNode->updateOutLinks(changes);
		//}
		//}//for changes
	}//while
	 //cout<<endl<<"passaggi necessari:"<<counter<<endl;
	 ///////////////ora ricavo  percorso e lo inserisco////////
	int j;
	LINK_COST min = UNREACHABLE;
	int index = -1;
	vector<int> lambdas;
	for (j = 0; j < pDst->costi.size(); j++) //ricerco il percorso minimo
	{
		if (pDst->costi[j] < min)
		{
			index = j;
			min = pDst->costi[j];
		}
	}
	if (index == -1)
	{
#ifdef DEBUGB
		cout << "\tMi sa che si ferma qui!" << endl;
#endif // DEBUGB
		return UNREACHABLE;
	}

	//scelgo una lamda valida tra la label
	index = randomChSelection(pDst->costi); //costi is valarray<LINK_COST>, i.e. valarray<double>, and belongs to AbstractNode class (the class pDst belongs to) 

#ifdef DEBUGB
	cout << "\tCanale selezionato casualmente: " << index << endl;
#endif // DEBUGB

	///creazione percorso primario
	AbstractLink* pLink = pDst->prev[index];

	while (true)
	{
		/*
		cout<<"Node  "<<pLink->getSrc()->getId()+1<<endl;
		for (int ii=0;ii<16;ii++)
		{
		if(pLink->getSrc()->prev[ii]==NULL)
		cout<<"NULL"<<endl;
		else
		cout<<pLink->getSrc()->prev[ii]->getSrc()->getId()+1<<endl;
		}
		cout<<"=================================="<<endl;
		*/
		path.m_hLinkList.push_front(pLink);
		pLink->getSrc()->costiParzialiFlag = true; //FABIO 15 marzo: segnalo che i costi di questo nodo van mantenuti per le complutazioni
												   //dei k percorsi di yen
		pLink = pLink->getSrc()->prev[index];
		if (pLink == NULL)
			break;
	}
	path.wlAssigned = index;
	return path.calculateCost();
}


int AbstractGraph::randomChSelection(valarray<double> values)
{
#ifdef DEBUGB
	cout << "-> randomChSelection" << endl;
	//cout << "\tchannels to delete size: " << channelsToDelete.size() << endl;
#endif // DEBUGB

	bool flag = false;
	vector<int> lam;
	double min = values.min();

	for (int i = 0; i < values.size(); i++)
		if (values[i] == min)
		{
				//for (z = 0, flag = false; z < channelsToDelete.size() ; z++)
				//{
					//if (i == channelsToDelete[z])
					//{
						//flag = true;
						//break;
					//}
				//}
				//if (!flag)
					lam.push_back(i); //inserisco la lambda nella lista di canali selezionabili
			//cout << "\tmetto dentro il ch " << i << " con costo " << values[i] << endl;
		}

	double val = double(rand()) / (RAND_MAX + 1);
	int selectedIndex = (int)(val * (lam.size()));

#ifdef DEBUGB
	cout << "CANALI DISPONIBILI: " << lam.size() << endl;
#endif // DEBUGB

	return lam[selectedIndex];
}


void AbstractGraph::WPinvalidateRootPaths(AbstractNode* pSrc,AbstractNode* pDst)//Invalido tutti i root( ponendo i costi = 0) su ogni lambda
{
int i;
AbstractNode* pNode;


for(i=0;i<pDst->prev.size();i++)

{
	pNode = pDst;
	pNode->costiParziali[i]=0;
		pNode->prevParziali[i]=pNode->prev[i];
	while(pNode->prev[i]!=NULL)
	{
		
		
		pNode->costiParziali[i]=0;
		pNode->prevParziali[i]=pNode->prev[i];
	
	pNode=pNode->prev[i]->getSrc();
	}		
	
}//FOR
}

LINK_COST AbstractGraph::myAlgPartial(AbsPath& path,AbstractNode* pSrc,AbstractNode* pDst,int nchannels){

	bool changeFlag= true;
//vector<bool> changes(this->m_hNodeList.	m_hList.size());
set<int> changes;
int i;
	//reset della status list di tutti i nodi, tranne che per il sorgente;
	list<AbstractNode*>::iterator itrnodo=m_hNodeList.m_hList.begin();

	for (itrnodo=m_hNodeList.m_hList.begin();itrnodo!=m_hNodeList.m_hList.end();itrnodo++)
	{
			(*itrnodo)->	prevParziali=vector<AbstractLink*>(nchannels);
		(*itrnodo)->	costiParziali=valarray<LINK_COST> (UNREACHABLE,nchannels);
	/*	for(i=0;i<nchannels;i++)
			//	(*itrnodo)->costi=valarray<LINK_COST> (UNREACHABLE,nchannels);
			//	(*itrnodo)->prev=vector<AbstractLink*>(nchannels);
	
		if((*itrnodo)->costiParziali[i]!=0)	
			{
				(*itrnodo)->costiParziali[i]=UNREACHABLE;
				(*itrnodo)->prevParziali[i]=NULL;
			}*/
		
			(*itrnodo)->updates=vector<bool>(nchannels,false);
	}
WPinvalidateRootPaths(NULL,pSrc);//pongo costiParziali= 0  x i predecessori e prevParziali=prev per tutti i root
	 
	
//pSrc->costi=valarray<LINK_COST> (0.0,nchannels);
//pSrc->prev=vector<AbstractLink*>(nchannels,NULL);//FABIO 15 MARZO: se non inizializzo il src,dovrebbe mantenere
	
//for (i=0;i<nchannels;i++)				
//pSrc->updates.insert(i);
pSrc->updates=vector<bool>(nchannels,true);
changes.insert(pSrc->getId());//primo nodo su cui iniziare la scansione
int dstId=pDst->getId();
AbstractNode* pNode;
while (changes.size())
{
//changeFlag=false;
//int i;

//for(i=0;i<changes.size();i++)
//{
	i=(*changes.begin());
		changes.erase(changes.begin());

	if(i==dstId) continue;
//if(changes[i])//se changes[i]==true
//{
	
pNode=lookUpNodeById(i);
pNode->updateOutLinksPartial(changes);

//}

//}//for changes

}//while
//cout<<endl<<"passaggi necessari:"<<counter<<endl;
///////////////ora ricavo  percorso e lo inserisco////////

int j;
 LINK_COST min=UNREACHABLE;
 int index=-1;
for(j=0;j<pDst->costiParziali.size();j++)//ricerco il percorso minimo
{
	if(pDst->costiParziali[j]<min)
	{
		index=j;
		min=pDst->costiParziali[j];
	}
}
if (index==-1) return UNREACHABLE;
//scelgo una lamda valida tra la label

///creazione percorso primario
AbstractLink* pLink=pDst->prevParziali[index];

while(true)
{
path.m_hLinkList.push_front(pLink);

//pLink->getSrc()->costiParzialiFlag=true;//FABIO 15 marzo: segnalo che i costi di questo nodo van mantenuti per le complutazioni
										//dei k percorsi di yen
pLink=pLink->getSrc()->prevParziali[index];
if (pLink==NULL) break;

}

path.wlAssigned=index;
return path.calculateCost();


}

LINK_COST AbstractGraph::myAlgBCK(AbsPath& path,AbstractNode* pSrc,AbstractNode* pDst,int nchannel){

	bool changeFlag= true;
//vector<bool> changes(this->m_hNodeList.	m_hList.size());
set<int> changes;

	//reset della status list di tutti i nodi, tranne che per il sorgente;
	list<AbstractNode*>::iterator itrnodo=m_hNodeList.m_hList.begin();
int i;
	for (itrnodo=m_hNodeList.m_hList.begin();itrnodo!=m_hNodeList.m_hList.end();itrnodo++)
	{
(*itrnodo)->costi=valarray<LINK_COST> (UNREACHABLE,nchannel);
(*itrnodo)->prev=vector<AbstractLink*>(nchannel);
(*itrnodo)->updates=vector<bool>(nchannel,false);
	}

	
pSrc->costi=valarray<LINK_COST> (0.0,nchannel);
pSrc->prev=vector<AbstractLink*>(nchannel,NULL);//verificare se resta effettivamente in lista...
	
//for (i=0;i<nchannel;i++)				//leak 
//pSrc->updates.insert(i);
pSrc->updates=vector<bool>(nchannel,true);
changes.insert(pSrc->getId());//primo nodo su cui iniziare la scansione
int dstId=pDst->getId();
AbstractNode* pNode;
while (changes.size())
{
//changeFlag=false;
//int i;

//for(i=0;i<changes.size();i++)
//{
i=(*changes.begin());
		changes.erase(changes.begin());

	if(i==dstId) continue;
//if(changes[i])//se changes[i]==true
//{
	
pNode=lookUpNodeById(i);
pNode->updateOutLinksBCK(changes);

//}

//}//for changes

}//while
//cout<<endl<<"passaggi necessari:"<<counter<<endl;
///////////////ora ricavo  percorso e lo inserisco////////

int j;
 LINK_COST min=UNREACHABLE;
 int index=-1;
for(j=0;j<pDst->costi.size();j++)//ricerco il percorso minimo
{
	if(pDst->costi[j]<min)
	{
		index=j;
		min=pDst->costi[j];
	}
}
if (index==-1) return UNREACHABLE;
//scelgo una lamda valida tra la label
index= randomChSelection(pDst->costi);
///creazione percorso primario
AbstractLink* pLink=pDst->prev[index];

while(true)
{
path.m_hLinkList.push_front(pLink);
pLink=pLink->getSrc()->prev[index];
if (pLink==NULL) break;

}

path.wlAssigned=index;
return min;


}

inline void AbstractGraph::YenHelperWP(list<AbsPath*>& hKPaths, 
									 AbstractNode* pSrc, AbstractNode* pDst, 
									 UINT nNumberOfPaths, LinkCostFunction hLCF)
{
#ifdef DEBUGB
	cout << "-> YenHelperWP" << endl;
#endif // DEBUGB

	BinaryHeap<AbsPath*, vector<AbsPath*>, PAbsPathComp> hPQ;
	LINK_COST hCost;
	list<AbstractLink*> hMinCostPath;
	
	AbsPath* ABP = new AbsPath();//f
	hCost = myAlg(*ABP, pSrc, pDst, numberOfChannels);      //f
	//DijkstraHelper(pSrc,pDst, hLCF);//FABIO 4 dic: modificato
	//hCost = recordMinCostPath(hMinCostPath, pDst);
	if (UNREACHABLE == hCost)
		return;	// graph disconnected
	//hPQ.insert(new AbsPath(hMinCostPath));
	hPQ.insert(ABP);//f

	list<AbstractLink*>::const_iterator itr, itrSeg;
	AbsPath *pCurrentPath;
	UINT  k = 0;
//	if(nNumberOfPaths>1) WPinvalidateRootPaths(pSrc,pDst);
	while (k<nNumberOfPaths) {
		pCurrentPath = hPQ.peekMin();
		hPQ.popMin();

		if ((k+1) == nNumberOfPaths) {
			hKPaths.push_back(pCurrentPath);
			break;
		}
		// a minor optimization
		if ((k+2) == nNumberOfPaths) {
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
		for (itr=pCurrentPath->m_hLinkList.begin(); 
		itr!=pCurrentPath->m_hLinkList.end(); itr++) {
			list<AbstractNode*> hDirtyNodes;	// nodes to invalidate/validate
			list<AbstractLink*> hDirtyLinks;
			// disable first segment nodes to ensure loopless
			for (itrSeg=hFirstSeg.begin(); itrSeg!=hFirstSeg.end(); itrSeg++) {
				(*itrSeg)->m_pSrc->invalidate();
				hDirtyNodes.push_back((*itrSeg)->m_pSrc);
			}
			// no branch
			(*itr)->invalidate();
			hDirtyLinks.push_back(*itr);
			list<AbsPath*>::const_iterator itrKPaths;
			for (itrKPaths=hKPaths.begin(); itrKPaths!=hKPaths.end(); 
			itrKPaths++) {
				bool bSameRoot = true;
				list<AbstractLink*>::const_iterator itrK;
				for (itrSeg=hFirstSeg.begin(), 
					itrK=(*itrKPaths)->m_hLinkList.begin(); 
				(itrSeg!=hFirstSeg.end()) &&
					(itrK!=(*itrKPaths)->m_hLinkList.end()) && bSameRoot;
				itrSeg++, itrK++) {
					if ((*itrSeg) != (*itrK))
						bSameRoot = false;
				}
				if (bSameRoot && (itrSeg == hFirstSeg.end()) 
					&& (itrK != (*itrKPaths)->m_hLinkList.end())) {
					(*itrK)->invalidate();
					hDirtyLinks.push_back(*itrK);
				}
			}
			AbsPath * pPath = new AbsPath();
			hCost=myAlgPartial(*pPath,(*itr)->m_pSrc,pDst,numberOfChannels); //4 genn: lamda non serve piu...
			//DijkstraHelper((*itr)->m_pSrc,pDst, hLCF);//FABIO 4 dic
			// enable nodes/links
			list<AbstractNode*>::const_iterator itrDNodes;
			list<AbstractLink*>::const_iterator itrDLinks;
			for (itrDNodes=hDirtyNodes.begin(); itrDNodes!=hDirtyNodes.end();
			itrDNodes++) {
				(*itrDNodes)->validate();
			}
			for (itrDLinks=hDirtyLinks.begin(); itrDLinks!=hDirtyLinks.end();
			itrDLinks++) {
				(*itrDLinks)->validate();
			}

			//hCost = recordMinCostPath(hSecondSeg, pDst);
			if (UNREACHABLE != hCost) {
			//	AbsPath *pPath = new AbsPath();
			//	pPath->m_hLinkList = hFirstSeg;
			//	pPath->m_hLinkList.splice(pPath->m_hLinkList.end(), hSecondSeg);
			//	pPath->m_hLinkList.splice(pPath->m_hLinkList.end(), hFirstSeg);//F: in teoria, quello che era il sevond seg l'ho gia calcolato con my alg..
				//RIMOSSO 15 marzo: probabilmente non serve piu..

				pPath->calculateCost();

				// added on May 6th, 2003 to handle duplicate paths
				bool bDuplicate = false;
				vector<AbsPath*>::const_iterator itrTmp;
				for (itrTmp=hPQ.m_hContainer.begin(); 
				((itrTmp!=hPQ.m_hContainer.end()) && (!bDuplicate));
				itrTmp++) {
					if ((**itrTmp) == *pPath) {
						bDuplicate = true;
					}
				}
				// insert to binary heap if it's not a duplicate
				if (!bDuplicate) {
					hPQ.insert(pPath);
				} else {
					delete pPath;
				}
			}

			hFirstSeg.push_back(*itr);
		} // for itr

		hKPaths.push_back(pCurrentPath);
//		list<AbstractLink*>::iterator iterN;
//		for(iterN=pCurrentPath->m_hLinkList.begin();iterN!=pCurrentPath->m_hLinkList.end();iterN++)
//		(*iterN)->getSrc()->costiParzialiFlag=true;//tengo fissi i costi dei nuovi root trovati
		
		k++;
		if (hPQ.empty())
			break;	// done
	}

	// release memory
	vector<AbsPath*>::const_iterator itrTmp;
	for (itrTmp=hPQ.m_hContainer.begin(); itrTmp!=hPQ.m_hContainer.end();
	itrTmp++) {
		delete (*itrTmp);
	}
}

void AbstractGraph::allocateConflictSet(UINT nSize)
{
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		if ((*itr)->m_pCSet)
			delete [](*itr)->m_pCSet;
		(*itr)->m_nCSetSize = nSize;
		(*itr)->m_pCSet = new UINT[nSize];
		memset((*itr)->m_pCSet, 0, nSize*sizeof(UINT));
	}
}


///////////////////////////////////////////////////////////////////////
// Mie Funzioni /////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


void AbstractGraph::YenGreen(list<AbsPath*>& hKPaths, 
						AbstractNode* pSrc, AbstractNode* pDst, 
						UINT nNumberOfPaths, LinkCostFunction hLCF)
{
	
	YenHelperGreen(hKPaths, pSrc, pDst, nNumberOfPaths, hLCF);
}


inline void AbstractGraph::YenHelperGreen(list<AbsPath*>& hKPaths, 
									 AbstractNode* pSrc, AbstractNode* pDst, 
									 UINT nNumberOfPaths, LinkCostFunction hLCF)
{
	BinaryHeap<AbsPath*, vector<AbsPath*>, PAbsPathComp> hPQ;
	LINK_COST hCost;
	list<AbstractLink*> hMinCostPath;
		
	DijkstraHelperGreen(pSrc, pDst, hLCF); //FABIO 4 dic: modificato

	hCost = recordMinCostPath(hMinCostPath, pDst);
	if (UNREACHABLE == hCost)
		return;	// graph disconnected
	hPQ.insert(new AbsPath(hMinCostPath));
   	list<AbstractLink*>::const_iterator itr, itrSeg;
	AbsPath *pCurrentPath;
	UINT  k = 0;

	while (k < nNumberOfPaths) {
		pCurrentPath = hPQ.peekMin();
		hPQ.popMin();

		if ((k+1) == nNumberOfPaths) {
			hKPaths.push_back(pCurrentPath);
			break;
		}
		// a minor optimization
		if ((k+2) == nNumberOfPaths) {
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
		for (itr = pCurrentPath->m_hLinkList.begin(); itr != pCurrentPath->m_hLinkList.end(); itr++) {
			list<AbstractNode*> hDirtyNodes;	// nodes to invalidate/validate
			list<AbstractLink*> hDirtyLinks;
			// disable first segment nodes to ensure loopless
			for (itrSeg = hFirstSeg.begin(); itrSeg != hFirstSeg.end(); itrSeg++) {
				(*itrSeg)->m_pSrc->invalidate();
				hDirtyNodes.push_back((*itrSeg)->m_pSrc);
			}
			// no branch
			(*itr)->invalidate();
			hDirtyLinks.push_back(*itr);
			list<AbsPath*>::const_iterator itrKPaths;
			for (itrKPaths = hKPaths.begin(); itrKPaths != hKPaths.end(); itrKPaths++) {
				bool bSameRoot = true;
				list<AbstractLink*>::const_iterator itrK;
				for (itrSeg = hFirstSeg.begin(), itrK = (*itrKPaths)->m_hLinkList.begin(); 
					(itrSeg != hFirstSeg.end()) && (itrK != (*itrKPaths)->m_hLinkList.end()) && bSameRoot;
					itrSeg++, itrK++)
				{
					if ((*itrSeg) != (*itrK))
						bSameRoot = false;
				}
				if (bSameRoot && (itrSeg == hFirstSeg.end()) 
					&& (itrK != (*itrKPaths)->m_hLinkList.end())) {
					(*itrK)->invalidate();
					hDirtyLinks.push_back(*itrK);
				}
			}
			
	
			DijkstraHelperGreen((*itr)->m_pSrc,pDst, hLCF);//FABIO 4 dic
			// enable nodes/links
			list<AbstractNode*>::const_iterator itrDNodes;
			list<AbstractLink*>::const_iterator itrDLinks;
			for (itrDNodes = hDirtyNodes.begin(); itrDNodes != hDirtyNodes.end(); itrDNodes++) {
				(*itrDNodes)->validate();
			}
			for (itrDLinks = hDirtyLinks.begin(); itrDLinks != hDirtyLinks.end(); itrDLinks++) {
				(*itrDLinks)->validate();
			}

			hCost = recordMinCostPath(hSecondSeg, pDst);

			if (UNREACHABLE != hCost) {
				AbsPath *pPath = new AbsPath();
				pPath->m_hLinkList = hFirstSeg;
				pPath->m_hLinkList.splice(pPath->m_hLinkList.end(), hSecondSeg);
				pPath->calculateCost();

				// added on May 6th, 2003 to handle duplicate paths
				bool bDuplicate = false;
				vector<AbsPath*>::const_iterator itrTmp;
				for (itrTmp = hPQ.m_hContainer.begin();
					((itrTmp != hPQ.m_hContainer.end()) && (!bDuplicate)); itrTmp++) {
					if ((**itrTmp) == *pPath) {
						bDuplicate = true;
					}
				}
				// insert to binary heap if it's not a duplicate
				if (!bDuplicate) {
					hPQ.insert(pPath);
				} else {
					delete pPath;
				}
			}

			hFirstSeg.push_back(*itr);
		} // for itr

		hKPaths.push_back(pCurrentPath);
		k++;
		if (hPQ.empty())
			break;	// done
	}

	// release memory
	vector<AbsPath*>::const_iterator itrTmp;
	for (itrTmp=hPQ.m_hContainer.begin(); itrTmp!=hPQ.m_hContainer.end();
	itrTmp++) {
		delete (*itrTmp);
	}
}




inline void AbstractGraph::DijkstraHelperGreen(AbstractNode* pSrc, AbstractNode* pDst,
										  LinkCostFunction hLCF)
{
	reset4ShortestPathComputation();
	pSrc->m_hCost = 0;

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ;
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode = m_hNodeList.begin(); itrNode != m_hNodeList.end(); itrNode++) {
		if ((*itrNode)->valid())
			hPQ.insert(*itrNode);
	}
	list<AbstractLink*>::const_iterator itr;
	while (!hPQ.empty()) {
	  int dim = hPQ.m_hContainer.size();
	  AbstractNode *pLinkSrc = hPQ.peekMin();
	  hPQ.popMin();
		 if(pLinkSrc == pDst) 
			return;// FABIO 4 dic
		 if (UNREACHABLE == pLinkSrc->m_hCost)
		    break;

		for (itr = pLinkSrc->m_hOLinkList.begin(); itr != pLinkSrc->m_hOLinkList.end(); itr++) 
		{
			// the link has been invalidated
			if ((!(*itr)->valid()) || (UNREACHABLE == (*itr)->getCost()))
			{
				cout << "\nLINK NON VALIDO, VADO AL PROSSIMO";
				continue;
			}	

// NB: design has changed
//		lightpaths will NOT serve as links in other graph as this has given
//		me endless hassle.
			// this may be different from the network lightpaths point to,
			// so look up those nodes
//			if (AbstractLink::LT_Lightpath == (*itr)->getLinkType()) {
//				pRealSrc = lookUpNodeById((*itr)->m_pSrc->getId());
//				pRealDst = lookUpNodeById((*itr)->m_pDst->getId());
//			}

			LINK_COST hLinkCost;
			switch (hLCF) {
			  case LCF_ByHop:
				hLinkCost = 1;
				break;
			  case LCF_ByOriginalLinkCost:
				hLinkCost = (*itr)->getCost();
				assert(hLinkCost > 0);
				break;
			  default:
				DEFAULT_SWITCH;
			}//itr = itera su tutti i link uscenti dal nodo considerato
			double c1 = (*itr)->m_pDst->m_hCost;
			double c2 = pLinkSrc->m_hCost + hLinkCost;
			if ((*itr)->m_pDst->m_hCost > pLinkSrc->m_hCost + hLinkCost) {
				(*itr)->m_pDst->m_hCost = pLinkSrc->m_hCost + hLinkCost;
				// use previous link instead of node
			(*itr)->m_pDst->m_pPrevLink = *itr;
		
			}
		} // for
		hPQ.buildHeap();
	} // while
}


LINK_COST AbstractGraph::Dijkstra(AbstractPath& hMinCostPath,
	AbstractNode*pSrc, AbstractNode*pDst, LinkCostFunction hLCF)
{
	DijkstraHelper(pSrc, pDst, hLCF); //FABIO 4 dic: modifico 

									 // record minimum-cost path
	return recordMinCostPath(hMinCostPath, pDst);
}


/*inline*/ LINK_COST AbstractGraph::recordMinCostPath(AbstractPath* hPath,
	AbstractNode* pDst)
{
#ifdef DEBUGB
	cout << "-> recordMinCostPath*" << endl;
#endif // DEBUGB

	if (UNREACHABLE == pDst->m_hCost)
		return UNREACHABLE;

	hPath->deleteContent();

	UINT nMaxCount = m_hLinkList.size() + 1;
	UINT nCount = 0;	// to avoid infinite loop in while
	AbstractNode *pNode = pDst;
	while ((pNode->m_pPrevLink) && (nCount++ < nMaxCount)) {
		hPath->m_hLinkList.push_front(pNode->getId());
		// NB: design has changed
		//		lightpaths will NOT serve as links in other graph as this has given
		//		me endless hassle.
		// this may be different from the network lightpaths point to,
		// so look up those nodes
		//		if (AbstractLink::LT_Lightpath == pNode->m_pPrevLink->getLinkType()) {
		//			pNode = lookUpNodeById(pNode->m_pPrevLink->m_pSrc->getId());
		//		} else {
		//			pNode = pNode->m_pPrevLink->m_pSrc;
		//		}

		pNode = pNode->m_pPrevLink->m_pSrc;
		assert(pNode);
	}
	hPath->m_hLinkList.push_front(pNode->getId());	// src node

													// See if anything went wrong
	if (nCount == nMaxCount) {	//-B: why in the other Dijkstra's function is: if (nCount == nMaxCount+1) ????????
		this->dump(cout);
		THROW_OCH_EXEC("Loop exists in shortest path.");
	}

	hPath->setSrc(hPath->m_hLinkList.front());
	hPath->setDst(pDst->getId());
	return pDst->m_hCost;
}

//-B: for shortest lightpaths computation
LINK_COST AbstractGraph::DijkstraLightpath(OXCNode* pSrc, OXCNode* pDst, Circuit&pCircuit, Graph&hGraph)
{
	DijkstraHelperLP(pSrc, pDst, hGraph);

	// record minimum-cost path
	return recordMinCostPathLP(pDst, pCircuit);
}


inline void AbstractGraph::DijkstraHelperLP(OXCNode* pSrc, OXCNode* pDst, Graph&hGraph)
{
#ifdef DEBUGB
	cout << "-> DijkstraHelperLP" << endl;
#endif // DEBUGB

	//-B: for each VERTEX, it sets: (*itr)->m_pPrevLightpath = NULL; (*itr)->m_hCost = UNREACHABLE;
	this->reset4ShortestPathComputation(); //this = WDMNet
	
	OXCNode*auxSrc;
	//Vertex*src = hGraph.lookUpVertex(pSrc->getId(), Vertex::VT_Access_In, -1);
	//OXCNode*pOXCSrc = (OXCNode*)lookUpNodeById(src->m_pOXCNode->getId());
	//OXCNode*pOXCDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);


	//-B: source node has zero reachability cost
	pSrc->m_hCost = 0;

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ;
	list<AbstractNode*>::const_iterator itrNode;

	//-B: INSERT ALL m_hGraph's NODES IN A LIST, SORTED BY NODE'S COST (not network's nodes! They are graph's vertexes)
	for (itrNode = m_hNodeList.begin(); itrNode != m_hNodeList.end(); itrNode++)
	{
		if ((*itrNode)->valid())
		{
			//-B: insert function is able to insert the item according to increasing nodes' cost
			hPQ.insert(*itrNode);
		}
	}

	list<Lightpath*>::const_iterator itr;
	//-B: till this nodes' list is not empty
	while (!hPQ.empty())
	{
		//-B: choose vertex with the minimum distance/weight/cost with respect to source node
		//	(I think the node with the minimum node cost)
		AbstractNode*pLinkSrc = hPQ.peekMin();
		//-B: remove this vertex from the list
		hPQ.popMin();
		//-B: if this vertex is the destination node, we have found what we are looking for, so return
		if (pLinkSrc == pDst)
		{
#ifdef DEBUGB
			cout << "\tnodo considerato = nodo destinazione -> TROVATO!" << endl;
#endif // DEBUGB
			return; // FABIO 4 dic
		}
		//-B: if this vertex has an infinite cost, go to next iteration of while cycle -> consider next cheapest node
		if (UNREACHABLE == pLinkSrc->m_hCost)
			break;

		//It should be as equal as: auxSrc = (OXCNode*)(*pLinkSrc)
		Vertex*src = (Vertex*)hGraph.lookUpVertex(pLinkSrc->getId(), Vertex::VT_Access_In, -1);
		auxSrc = (OXCNode*)lookUpNodeById(src->m_pOXCNode->getId());
		//-B: build auxiliary lightpaths' list with all lightpaths outgoing from selected node
		genAuxOutLightpathsList(auxSrc);

		//-B: consider all the adjacent outgoing links from selected node
		for (itr = auxSrc->m_hAuxOutgoingLpList.begin(); itr != auxSrc->m_hAuxOutgoingLpList.end(); itr++)
		{
			//-B: if the link has been invalidated or has an infinite cost, go to the next adjacent link
			if ((!(*itr)->valid()) || (UNREACHABLE == (*itr)->getCost()))
				continue;

			/*
			//-B: save selected link cost
			LINK_COST hLinkCost;
			switch (hLCF)
			{
			case LCF_ByHop:
			hLinkCost = 1;
			break;
			case LCF_ByOriginalLinkCost:
			hLinkCost = (*itr)->getCost();
			assert(hLinkCost >= 0); //-B: it was just > 0, I've changed it
			break;
			default:
			DEFAULT_SWITCH;
			}*/

			//-B: save cost of the destination node of the lightpath
			double c1 = (*itr)->getDst()->getCost();
			//-B: compute partial cost: reachability cost = source node cost + outgoing link cost
			double c2 = auxSrc->getCost() + (*itr)->getCost();
			//-B: if destination node's cost > reachability cost
			if (c1 > c2)
			{
				//-B: set destination node's cost = reachability cost
				(*itr)->m_pDst->m_hCost = c2;
				//-B: set this outgoing lightpath as (unique) previous lightpath of the destination node of the selected outgoing lightpath
				(*itr)->m_pDst->m_pPrevLightpath = *itr; //assign previous lightpath
			}
		} //end FOR outgoing links

		  //-B: Sort again the destination node in the list according to the new cost
		hPQ.buildHeap();

	} // end WHILE

#ifdef DEBUGB
	cout << "\tNO LIGHTPATH AVAILABLE???" << endl;
#endif // DEBUGB

}


/*inline*/ LINK_COST AbstractGraph::recordMinCostPathLP(OXCNode *pDst, Circuit&pCircuit)
{
#ifdef DEBUGB
	cout << "->recordMinCostPathLP" << endl;
#endif // DEBUGB

	//-B: if destination node's cost is unreachable, it means that no path was found -> the cost wasn't overwritten
	if (UNREACHABLE == pDst->getCost()) {
		return UNREACHABLE;
	}

	UINT nMaxCount = this->m_hLinkList.size() + 1; //-B: this = WDMNet
	UINT nCount = 0; 	// to avoid infinite loop in while

	//-B: +++++++++++++ BUILD THE PATH: from dst to src, according to previous link poitner ++++++++++++++
	Lightpath*pPrevLp = pDst->m_pPrevLightpath;
	while (pPrevLp && (nCount++ < nMaxCount))
	{
#ifdef DEBUGB
		cout << "\tSto aggiungendo il lightpath " << pPrevLp->getId() << "(" << pPrevLp->getSrc()->getId()
			<< "->" << pPrevLp->getDst()->getId()  << ") al circuito" << endl;
#endif // DEBUGB
		pCircuit.addFrontVirtualLink(pPrevLp);
		pPrevLp = pPrevLp->getSrc()->m_pPrevLightpath;
	}

	// See if anything went wrong
	if (nCount == nMaxCount + 1)
	{	//-B: why in the other Dijkstra's function is: if (nCount == nMaxCount) ????????
		THROW_OCH_EXEC("Loop exists in shortest path.");
	}

	//-B: we have already computed the path's cost in DijkstraHelper method, so it simply returns it
	return pDst->m_hCost;
}


void AbstractGraph::genAuxOutLightpathsList(OXCNode*pNode)
{
	//reset outgoing lightpaths' list
	pNode->m_hAuxOutgoingLpList.clear();

	list<Lightpath*>::const_iterator itr1;
	//-B: browse lightpaths' list having enough capacity to host the connection
	for (itr1 = auxLightpathsList.begin(); itr1 != auxLightpathsList.end(); itr1++)
	{
		//-B: if lightpath's source node == selected node
		if ((*itr1)->getSrc()->getId() == pNode->getId())
		{
			pNode->m_hAuxOutgoingLpList.push_back((*itr1));
		}
	}
}

//-B: build auxiliary lightpaths' list with all lightpaths having enpough capacity
void AbstractGraph::genAuxLightpathsList(BandwidthGranularity&bwd, LightpathDB&hLightpathDB)
{
#ifdef DEBUGB
	cout << "-> genAuxLightpathList" << endl;
#endif // DEBUGB

	//-B: reset auxiliary lightpaths' list
	auxLightpathsList.clear();

	list<Lightpath*>::const_iterator itr;
	for (itr = hLightpathDB.m_hLightpathList.begin(); itr != hLightpathDB.m_hLightpathList.end(); itr++)
	{
		if ((*itr)->m_nFreeCapacity >= (UINT)bwd)
		{
			auxLightpathsList.push_back((*itr));
		}
	}
}


void AbstractGraph::setSimplexLinkCostForGrooming(MappedLinkList<UINT, AbstractLink*>pOutLinkList)
{	
	SimplexLink*pSLink;
	list<AbstractLink*>::const_iterator itrLink;
	BinaryHeap<SimplexLink*, vector<SimplexLink*>, PSimplexLinkComp> simplexLinkSorted;

	//STEP 1 - BUILD A SORTED SIMPLEX LINK LIST
	//-B: scann all the simplex links of the extended graph
	for (itrLink = pOutLinkList.begin(); itrLink != pOutLinkList.end(); itrLink++)
	{
		pSLink = (SimplexLink*)(*itrLink);
		//-B: if it has not been invalidated during invalidateSimplexLinkDueToCap or invalidateSimplexLinkDueToFreeStatus
		if (pSLink->getValidity())
		{
			//-B: control to not consider simplex link of type LT_Mux, LT_Converter, LT_Grooming, ecc.
			//	that have infinite capacity
			if (pSLink->m_hFreeCap < OCLightpath)
			{
				//-B: insert simplex link in links' list, sorted by increasing free capacity
				simplexLinkSorted.insert(pSLink);
			}
		}
	}

	LINK_COST costAssigned = 0;
	LINK_CAPACITY cap = 0;
	//-B: STEP 2 - ASSING INCREASING COSTS TO CHANNELS SELECTED FOR GROOMING
	while (!simplexLinkSorted.empty())
	{
		pSLink = simplexLinkSorted.peekMin();
		simplexLinkSorted.popMin();
		if (pSLink->m_hFreeCap > cap)
		{
			//increase link's cost only if its free capacity is effectively greater than previous link's free capacity
			costAssigned++;
		}//else: they will have the same cost

		//-------- ASSIGN AN INCREASING COST --------
		pSLink->modifyCost(costAssigned); //-B: !!!Ricorda che qui viene modificata anche la var bool m_bCostSaved, nel caso la utilizzassi!!!
#ifdef DEBUGB
		cout << "\tFiber " << pSLink->m_pUniFiber->getId() << " - SimplexLink ch " << pSLink->m_nChannel
			<< " - freecap: " << pSLink->m_hFreeCap << " - cost = " << pSLink->getCost() << endl;
#endif // DEBUGB
		//simplexLinkSorted.buildHeap(); //maybe useless, because I don't modify anything in this cycle
		cap = pSLink->m_hFreeCap;
	}

	//STEP 3 - SET ALL THE OTHER SIMPLEX LINKS' COSTS EQUAL TO A BIG NUMBER (>= link's cost with some free capacity < OCLightpath)
	for (itrLink = pOutLinkList.begin(); itrLink != pOutLinkList.end(); itrLink++)
	{
		pSLink = (SimplexLink*)(*itrLink);
		//-B: if it has not been invalidated during invalidateSimplexLinkDueToCap or invalidateSimplexLinkDueToFreeStatus
		if (pSLink->getValidity()) //just a check, since we are going to consider only those having free cap == OCLightpath
		{
			//-B: control to not consider simplex link of type LT_Mux, LT_Converter, LT_Grooming, ecc.
			//	that have infinite capacity
			if (pSLink->m_hFreeCap == OCLightpath)
			{
				pSLink->modifyCost(costAssigned + 1);
			}
		}//else: it doesn't care
	}

	//STEP 4 - ASSIGN A BIG COST ALSO TO THE CHANNEL BYPASS SIMPLEX LINK
	//	-> we'll do it in DijkstraHelper function

#ifdef DEBUGB
	//cout << endl;
#endif // DEBUGB
}