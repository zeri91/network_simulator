#pragma warning(disable: 4786)
#pragma warning(disable: 4018)
#pragma warning(disable: 4244)
#include <assert.h>
#include <set>
#include <list> //-M
#include <algorithm>
//#include <math>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "BinaryHeap.h"
#include "AbsPath.h"
#include "AbstractPath.h"
#include "AbstractGraph.h"
#include "OXCNode.h"
#include "UniFiber.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "Channel.h"
#include "WDMNetwork.h"
#include "Log.h"
#include "Lightpath.h"
#include "Lightpath_Seg.h"
#include "ConnectionDB.h"
#include "LightpathDB.h"
#include "Circuit.h"
#include "Connection.h"
#include "NetMan.h"
#include "Event.h" //--M
#include "Log.h"

//#define DEBUGMASSIMO_
//#define _Transform_Graph //----t
//#define _Parse_Backup_Segs //----t
//#define _Compute_Route  //---t
//#define backup
//#define dump_after

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int counter =0;
using namespace NS_OCH;

//-B: set m_nChannelCapacity e m_dTxScale equal to 0. Set the full wavel conversion everywhere
WDMNetwork::WDMNetwork(): m_nChannelCapacity(0), m_dTxScale(0)
{
	// right now consider full wavelength conversion everywhere only
	if (WPFLAG)
	{
		m_bFullWConversionEverywhere = false;
		m_bWContinuous = true;
	}
	else
	{
		m_bFullWConversionEverywhere = true;
		m_bWContinuous = false;
	}
}

//-B: reference of 'this' object points to WDMNetwork object passed as parameter
WDMNetwork::WDMNetwork(const WDMNetwork& rhs)
{
	*this = rhs;
}

//-B: operator method to create a new WDMNetwork object equal to the WDMNetwork object passed as reference parameter
//	return a reference to 'this' object
WDMNetwork& WDMNetwork::operator=(const WDMNetwork& rhs)
{
	//if 'this' object points to rhs object passed as parameter, simply return this reference
	if (this == &rhs)
		return (*this);
	//-B: if 'this' object does not refer to rhs object passed as parameter, fill its attributes (WHY???)
	//FABIO 2 dic: assign values to the attributes belonging to 'this' object
	m_dEpsilon = rhs.m_dEpsilon;
	m_bFullWConversionEverywhere = rhs.m_bFullWConversionEverywhere;
	m_bWContinuous = rhs.m_bWContinuous;
	m_nChannelCapacity = rhs.m_nChannelCapacity;
	numberOfLinks = rhs.numberOfLinks;
	numberOfNodes = rhs.numberOfNodes;
	numberOfChannels = rhs.numberOfChannels;
	
	// copy nodes
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=rhs.m_hNodeList.begin(); itrNode!=rhs.m_hNodeList.end(); itrNode++)
	{
		OXCNode *pOXC = (OXCNode*)(*itrNode);
		addNode(new OXCNode(*pOXC));
	}
	m_hNodeList.m_hList.reverse();

	// copy links
	list<AbstractLink*>::const_iterator itrLink;
	for (itrLink=rhs.m_hLinkList.begin(); itrLink!=rhs.m_hLinkList.end(); itrLink++)
	{
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		UniFiber *pNewUniFiber = addUniFiber(pUniFiber->getId(), 
										pUniFiber->getSrc()->getId(),
										pUniFiber->getDst()->getId(),
										pUniFiber->m_nW,
										pUniFiber->m_hCost,
										pUniFiber->m_nLength);
		int  w;
		for (w=0; w<pUniFiber->m_nW; w++) {
			pNewUniFiber->m_pChannel[w] = pUniFiber->m_pChannel[w];
			pNewUniFiber->m_pChannel[w].m_pUniFiber = pNewUniFiber;
		}
		if (pUniFiber->m_pCSet) {
			pNewUniFiber->m_pCSet = new UINT[pUniFiber->m_nCSetSize];
			pNewUniFiber->m_nCSetSize = pUniFiber->m_nCSetSize;
			memcpy(pNewUniFiber->m_pCSet, pUniFiber->m_pCSet, 
				pUniFiber->m_nCSetSize*sizeof(UINT));
		}
		pNewUniFiber->m_nBChannels = pUniFiber->m_nBChannels;

#ifdef _OCHDEBUG9
		{
			int e;
			for (e=0; e<pUniFiber->m_nCSetSize; e++) {
				assert(pUniFiber->m_pCSet[e] <= pUniFiber->m_nBChannels);
			}
		}
#endif

	}

	m_hLinkList.m_hList.reverse();
	return (*this);
}

//-B: nothing to do
WDMNetwork::~WDMNetwork()
{
}

//-B: add node passed as parameter to the m_hNodeList of AbstractGraph object
//	and creates a new OXCNode object that refers to the WDMNetwork is in
bool WDMNetwork::addNode(AbstractNode *pNode)
{
	if (!AbstractGraph::addNode(pNode))
		return false;

	OXCNode *pOXC = (OXCNode*)pNode;
	assert(pOXC);
	pOXC->setWDMNetwork(this);
	return true;
}

//-B: return and add a new UniFiber object (derived from AbstractLink object) to the AbstractGraph object.
//	Each UniFiber object has a Channel array, whose size is the num of wavel/channels
UniFiber* WDMNetwork::addUniFiber(UINT nFiberId, UINT nSrc, UINT nDst, 
								  UINT nW, LINK_COST hCost, float nLength)
{
	assert(nW);
	UniFiber *pUniFiber = new UniFiber(nFiberId, (OXCNode*)lookUpNodeById(nSrc),
		(OXCNode*)lookUpNodeById(nDst), hCost, nLength, nW);

/*
#ifdef DEBUGB
	cout << "Acquisizione dati UniFibers: acquisita fibra num " << nFiberId << " con " << nW << " canali, ciascuno di capacità " << m_nChannelCapacity << endl;
#endif // DEBUGB
*/

	// allocate channels, all of same capacity
	Channel *pChannel = new Channel[nW];
	//Lightpath *pLightpath = new Lightpath[(OCLightpath / BWDGRANULARITY)];
	UINT  w;
	for (w = 0; w < nW; w++)
	{
		pChannel[w] = Channel(pUniFiber, w, m_nChannelCapacity, true, UP);
		pChannel[w].linkProtected = vector<bool>(numberOfLinks); //LINK VARIABILE
		pChannel[w].nodesProtected = vector<bool>(numberOfNodes); //NODI VARIABILE
		//for (i = 0; i < (OCLightpath / BWDGRANULARITY); i++)
		//{ 
			//pLightpath[i] = Lightpath(0, Lightpath::LPProtectionType::LPT_PAC_Unprotected, OCLightpath);;
			//pChannel[w].m_pLightpath = pLightpath;
		//}
	}

	pUniFiber->m_pChannel = pChannel;
	pUniFiber->m_used = 0;
	addLink(pUniFiber);
	return pUniFiber;
}

//-B: as addUniFiber, but it doesn't add the UniFiber object to the AbstractGraph.
//	This way it returns a reference to a new Unifiber object
UniFiber* WDMNetwork::addUniFiberOnServCopy(UINT nFiberId, UINT nSrc, UINT nDst, 
								  UINT nW, LINK_COST hCost, UINT nLength)
{
	assert(nW);
	UniFiber *pUniFiber = new UniFiber(nFiberId, (OXCNode*)lookUpNodeById(nSrc),
		(OXCNode*)lookUpNodeById(nDst), hCost, nLength, nW);

	// allocate channels, all of same capacity
	Channel *pChannel =  new Channel[nW];
	UINT  w;
	for (w=0; w<nW; w++)
		pChannel[w] = Channel(pUniFiber, w, m_nChannelCapacity, true, UP);
	pUniFiber->m_pChannel = pChannel;

	//addLink(pUniFiber);
	return pUniFiber;
}

//-B: simply set the related var to the value passed as parameter
void WDMNetwork::setChannelCapacity(UINT nCapacity)
{
	m_nChannelCapacity = nCapacity;
}

UINT NS_OCH::WDMNetwork::getChannelCapacity()
{
	return m_nChannelCapacity;
}

//-B: create a 3 - layer network
//-B: for each node of the network, it calls the genStateGraphHelper (very long method);
//	for each link of the network, it calls the addSimplexLink method, that calls the addLink method that
//	adds each link to the m_hLinkList of the AbstractObject (-> WDMNetwork object I guess, since it is a derived class)
void WDMNetwork::genStateGraph(Graph& hGraph) const
{
	cout << "\t-> genStateGraph..." << endl;


	int nVertexId = 0;
	int nLinkId = 0;

	hGraph.deleteContent();
	// handle nodes
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode = m_hNodeList.begin(); itrNode != m_hNodeList.end(); itrNode++)
	{
		OXCNode *pOXC = (OXCNode*)(*itrNode);
		assert(pOXC);
		pOXC->genStateGraphHelper(hGraph, nVertexId, nLinkId);
	}

	// handle wavelength-link layers
	list<AbstractLink*>::const_iterator itrLink;
	//-B: Linklist.size() == num of fibers
	for (itrLink = m_hLinkList.begin(); itrLink != m_hLinkList.end(); itrLink++)
	{
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		assert(pUniFiber);
		Channel *pChannel = pUniFiber->m_pChannel;
		assert(pChannel);
		int w;
		//switch (protectiontype = PT_BBU)
		//{
		//Lightpath *pLightpath = new Lightpath[(OCLightpath / BWDGRANULARITY)];
		//}
		//-B: m_nW = num of channels
		for (w = 0; w < pUniFiber->m_nW; w++)
			if (pChannel[w].m_bFree)
			{
				SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++, 
					pUniFiber->m_pSrc->m_nNodeId, Vertex::VT_Channel_Out, w,
					pUniFiber->m_pDst->m_nNodeId, Vertex::VT_Channel_In, w,
					pUniFiber->m_hCost, pUniFiber->m_nLength,
					pUniFiber, SimplexLink::LT_Channel, w,
					m_nChannelCapacity);
				assert(pNewLink);
				/*for (int i = 0; i < (OCLightpath / BWDGRANULARITY); i++)
				{
					pLightpath[i] = Lightpath(0, Lightpath::LPProtectionType::LPT_PAC_Unprotected, OCLightpath);
					pChannel[w].m_pLightpath = pLightpath;
				}*/
			}
	}

	cout << "\tWDMNet --> NodeList size, LinkLIst (end genstategraph): " << m_hNodeList.size() << " , " << m_hLinkList.size() << endl;
	cout << "\tGRAPH --> NodeList, LinkLIst size (end genstategraph): " << hGraph.m_hNodeList.size() << " , " << hGraph.m_hLinkList.size() << endl;
	cout << "\t...end!" << endl;
}

inline void WDMNetwork::BBU_GenAuxGraph(Graph& hAuxGraph, Circuit& pCircuit, Graph& pGraph, UINT& w)
{
#ifdef DEBUGB
	cout << "-> BBU_GenAuxGraph" << endl;
#endif // DEBUGB

	int nVertexId = 0;
	int nLinkId = 0;
	hAuxGraph.deleteContent();

	list<AbstractNode*>::const_iterator itrN;
	list<AbstractLink*>::const_iterator itrL;
	for (itrN = m_hNodeList.begin(); itrN != m_hNodeList.end(); itrN++)
	{
		OXCNode *pOXC = (OXCNode*)(*itrN);
		assert(pOXC);
		pOXC->genStateAuxGraphHelper(hAuxGraph, nVertexId, nLinkId, w);
	}

	for (itrL = m_hLinkList.begin(); itrL != m_hLinkList.end(); itrL++)
	{
		UniFiber *pUniFiber = (UniFiber*)(*itrL);
		assert(pUniFiber);
		Channel *pChannel = pUniFiber->m_pChannel;
		assert(pChannel);
		//int w;
		//switch (protectiontype = PT_BBU)
		//{
		//Lightpath *pLightpath = new Lightpath[(OCLightpath / BWDGRANULARITY)];
		//}
		//-B: m_nW = num of channels
		//for (w = 0; w < pUniFiber->m_nW; w++)
		if (pChannel[w].m_bFree)
		{
			SimplexLink *pNewLink = hAuxGraph.addSimplexLink(nLinkId++,
				pUniFiber->m_pSrc->m_nNodeId, Vertex::VT_Channel_Out, w,
				pUniFiber->m_pDst->m_nNodeId, Vertex::VT_Channel_In, w,
				pUniFiber->m_hCost, pUniFiber->m_nLength,
				pUniFiber, SimplexLink::LT_Channel, w,
				m_nChannelCapacity);
			assert(pNewLink);
			//for (i = 0; i < (OCLightpath / BWDGRANULARITY); i++)
			//{
			//pLightpath[i] = Lightpath(0, Lightpath::LPProtectionType::LPT_PAC_Unprotected, OCLightpath);
			//pChannel[w].m_pLightpath = pLightpath;
			//}
		}
	}
	return;
}

void WDMNetwork::genStateGraphFullConversionEverywhere(Graph& hGraph) const
{
	int nVertexId = 0;
	int nLinkId = 0;

	hGraph.deleteContent();

	// handle nodes
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=m_hNodeList.begin(); itrNode!=m_hNodeList.end(); 
	itrNode++) {
		OXCNode *pOXC = (OXCNode*)(*itrNode);
		assert(pOXC);
		pOXC->genStateGraphFullConversionEverywhereHelper(hGraph, 
			nVertexId, nLinkId);
	}

	// handle wavelength-link layer
	list<AbstractLink*>::const_iterator itrLink;
	for (itrLink=m_hLinkList.begin(); itrLink!=m_hLinkList.end(); itrLink++) {
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		assert(pUniFiber);
		UINT nFreeChannels = pUniFiber->countFreeChannels();
		int w = -1;	// All wavelengths treated as one link
		SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++, 
					pUniFiber->m_pSrc->m_nNodeId, Vertex::VT_Channel_Out, w,
					pUniFiber->m_pDst->m_nNodeId, Vertex::VT_Channel_In, w,
					pUniFiber->m_hCost, pUniFiber->m_nLength,
					pUniFiber, SimplexLink::LT_UniFiber, w,
					m_nChannelCapacity * nFreeChannels);
		assert(pNewLink);
	}
}

// Reachability graph: adjacency based on available wavelength links
//-B: Clear the AbstractGraph object passed as parameter and fill it with those links
//	which have at least one free channel
void WDMNetwork::genReachabilityGraphThruWL(AbstractGraph& hGraph) const
{
	hGraph.deleteContent();	//clear m_hNodeList and m_hLinkList

	// handle nodes
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode = m_hNodeList.begin(); itrNode != m_hNodeList.end(); itrNode++)
	{
		hGraph.addNode(new AbstractNode((*itrNode)->getId()));
	}

	// handle wavelength links
	int nLinkId = 0;
	AbstractLink *pNewLink;
	list<AbstractLink*>::const_iterator itrLink;
	for (itrLink = m_hLinkList.begin(); itrLink != m_hLinkList.end(); itrLink++)
	{
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		assert(pUniFiber);
		Channel *pChannel = pUniFiber->m_pChannel;
		assert(pChannel);
		int  w;
		for (w = 0; w < pUniFiber->m_nW; w++)
		{
			if (pChannel[w].m_bFree)
			{
				pNewLink = hGraph.addLink(nLinkId++,
					pUniFiber->getSrc()->getId(), pUniFiber->getDst()->getId(),
					pUniFiber->getCost(), pUniFiber->getLength());
				assert(pNewLink);
				break;
			}
		}
	}
}

//-B: it seems to count all the channels of all the link in the network
//	(la cosa strana è che la capacity of each channel credo sia 192, quindi sarebbe OC192, non OC1)
UINT WDMNetwork::countOC1Links() const
{
	UINT nCount = 0;
	list<AbstractLink*>::const_iterator itr;
	UniFiber *pUniFiber;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		assert(pUniFiber && pUniFiber->m_pChannel);
		int  w;
		for (w = 0; w < pUniFiber->m_nW; w++)
			nCount += pUniFiber->m_pChannel[w].m_nCapacity;
	}
	return nCount;
}

//-B: return the total num of tx in the whole network
UINT WDMNetwork::countTx() const
{
	UINT nCount = 0;
	list<AbstractNode*>::const_iterator itr;
	OXCNode *pOXC;
	for (itr=m_hNodeList.begin(); itr!=m_hNodeList.end(); itr++) {
		pOXC = (OXCNode*)(*itr);
		assert(pOXC);
		nCount += pOXC->getNumberOfTx();
	}
	return nCount;
}

//-B: return the total num of rx in the whole network
UINT WDMNetwork::countRx() const
{
	UINT nCount = 0;
	list<AbstractNode*>::const_iterator itr;
	OXCNode *pOXC;
	for (itr=m_hNodeList.begin(); itr!=m_hNodeList.end(); itr++) {
		pOXC = (OXCNode*)(*itr);
		assert(pOXC);
		nCount += pOXC->getNumberOfRx();
	}
	return nCount;
}

//-B: it checks, for each link of the network, if the real num of backup channels
//	is equal to the value stored in the var m_nBChannels (attribute of the lin itself)
void WDMNetwork::dumpcontrol()
{	
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++) {
		UniFiber* fibra =(UniFiber*) (*itr);
		int bCount=0;
		for (int i = 0; i < fibra->m_nW; i++)
		{
			if((fibra)->m_pChannel[i].m_backup == 1)
				bCount++;
		}
		if((fibra)->m_nBChannels!=bCount)
		{
			cout<<"controllo fallito per fibra "<<fibra->m_nLinkId ;
			assert(false);
		}
	}
}

//-B: print the network status (WDMNetwork object status) (useful for debugging purpose)
//	for each link (it means, for each unidirectional fiber) it prints:
//	fiberId, num of primary working channels, num of backup channels, connections'/lightpaths' IDs
void WDMNetwork::dump(ostream &out) const
{
	list<AbstractLink*>::const_iterator itr;

#ifdef DEBUGB
	char*wConv;
	char*wCont;
	if (this->m_bFullWConversionEverywhere == true)
		wConv = "TRUE";
	else
		wConv = "FALSE";

	if (this->m_bWContinuous == true)
		wCont = "TRUE";
	else
		wCont = "FALSE";
	out << "-> WDMNetwork DUMP: STATO RETE" << endl;
	out << "\tNum of DC: " << this->nDC;
	out << "\tDummy node ID: " << this->DummyNode;
	out << "\tWDMNetwork Channel capacity: " << this->m_nChannelCapacity << endl;
	out << "\tALTRO: "
		<< "\tFullConversion: " << wConv
		<< "\tWContinuous: " << wCont
		<< "\tNum of channels per fiber: " << this->numberOfChannels
		<< "\tNum of links: " << this->numberOfLinks
		<< "\tNum of nodes: " << this->numberOfNodes;

	UniFiber* single_link;
	//SimplexLink* singleLink;
	
	out << "\n-> UniFiber DUMP" << endl;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		single_link = (UniFiber*)(*itr);
		//singleLink = (SimplexLink*)(*itr);
		//-B: UNIFIBER DUMP (not  AbstractLink dump)
		single_link->dump(cout);
		//-B: commento la stampa successiva perchè la var non viene nè inizializzata nè utilizzata (credo)
		//out << "\tFreeCap = " << singleLink->m_hFreeCap << endl;
	}

#endif // DEBUGB
		
	//-B: for each link, print its id, num of Primary Working (PW) channels and num of Backup (B) channels
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		//-B: count primary working channels
		int work = 0;
		UniFiber* fibra = (UniFiber*)(*itr);
		for (int i = 0; i < fibra->m_nW; i++)
		{
			//occupato e non di backup
			if ((fibra->m_pChannel[i].m_bFree == false ) && (fibra->m_pChannel[i].m_backup == false))
				work++;
			//out<<endl;
		}

		int bCount = 0;
		for (int i = 0; i < fibra->m_nW; i++)
		{
			if((fibra)->m_pChannel[i].m_backup == true)
				bCount++;
		}
		//assert((fibra)->m_nBChannels==bCount);

		//-B: last cout part is commented because I don't need it
		out << fibra << " " << (fibra)->m_nLinkId << "\t" << work << "PW  " /*<< bCount << "B   "*/;
		
		//-B: ciclo per scorrere tutti i canali
		for (int i = 0; i < fibra->m_nW; i++)
		{
			//occupato e non di backup
			//if ((fibra->m_pChannel[i].m_bFree==0 ) && (fibra->m_pChannel[i].m_backup==0))
			//work++;
			//out<<" "<<fibra->m_pChannel[i].m_pLightpath<<"  ";

			//-B: if a lightpath uses this channel
			if( fibra->m_pChannel[i].m_pLightpath != NULL)
			{
				int trueConValue;
				int lptId = fibra->m_pChannel[i].m_pLightpath->m_nLinkId;
				trueConValue = lptId;

				//-B: don't know what this cycle does----------------------
				for (int ii = (truecon.size() - 1); ii >= 0; ii--)
				{
					if (truecon.size()==0)
					{
						break;
					}
					if(lptId >= truecon[ii])
					{
						trueConValue = dbggap[ii] + (lptId - truecon[ii]); //
						break;
					}//IF
				}//FOR
				//---------------------------------------------------------

				out<<" "<<(trueConValue)<<" ";
			}//IF
			else
			{
				unsigned int trueout;
				if(fibra->m_pChannel[i].m_backup == 1)//se è un backup, scrivo le connessioni protette
				{
					out<<"p"<<" ";
					map<unsigned int,Lightpath*>::iterator SGiter;
					unsigned int lpp; //lightpath protetto..
					for(SGiter = fibra->m_bSharingGroup.begin(); SGiter != fibra->m_bSharingGroup.end(); SGiter++)
					{
						lpp=(SGiter)->first;
						trueout = lpp;
						for (int i3 = (truecon.size() - 1); i3 >= 0; i3--)
						{
							if (truecon.size()==0)
							{
								break;
							}
							if(lpp>= truecon[i3])
							{
								trueout=dbggap[i3]+(lpp-truecon[i3]); //
								break;
							}//IF
						}//FOR
						out<<trueout<<"p"<<" ";
					}//for sgiter
				}
				else 
					out<<" - ";
			}
			//out << "|"; //-B: print view is better without it
		}
	out<<"\n";
	}	
}

//-B: for each node of the network, it sets the num of tx/rx (and the num of free tx/rx)
//	equal to the num of outgoing/ingoing CHANNELS (not links) * dTxScale (0 < dTxScale <= 1)
void WDMNetwork::scaleTxRxWRTNodalDeg(double dTxScale)
{
	if ((0 == dTxScale) || (dTxScale > 1))
		return;
	// if 0.0 < m_dTxScale <= 1.0, then the #of Tx(Rx) at node a is
	// m_dTxScale * a's out degree (a's in degree)
	list<AbstractNode*>::const_iterator itr;
	OXCNode *pOXC;
	for (itr=m_hNodeList.begin(); itr!=m_hNodeList.end(); itr++) {
		pOXC = (OXCNode*)(*itr);
		assert(pOXC);
		pOXC->scaleTxRxWRTNodalDeg(dTxScale);
	}
	m_dTxScale = dTxScale;
}

//-B: return the num of free channels over all the links in the network
UINT WDMNetwork::countFreeChannels() const
{
	UINT nFreeChannels = 0;
	list<AbstractLink*>::const_iterator itr;
	UniFiber *pUniFiber;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		assert(pUniFiber);
		nFreeChannels += pUniFiber->countFreeChannels();
	}
	return nFreeChannels;
}

//-B: return the num of channels of the first object (link) of the m_hLinkList list
//	(I guess buecause the num of channles is the same for every link)
UINT WDMNetwork::countChannels() const
{
	UINT nChannels = 1000; //-B: because for sure it cannot be so big for a single link
	list<AbstractLink*>::const_iterator itr;
	UniFiber *pUniFiber;
	itr = m_hLinkList.begin();
	while (nChannels==1000) { // -B: when it finds a link in which m_nW has a valid value, it exits from this cycle
	    pUniFiber = (UniFiber*)(*itr);
		assert(pUniFiber);
		nChannels = pUniFiber->m_nW;
	    itr++;
	}
	return nChannels;
}

//-B: in WDMNetwork.h the parameter passed as input is always true, so I don't know how it works
//	and above all, it seems it is never used
void WDMNetwork::setWavelengthContinuous(bool bWContinuous)
{
	if (bWContinuous) {
		m_bFullWConversionEverywhere = false;
	}
	else 
		// might be sparse
		m_bFullWConversionEverywhere = false;
		// m_bWContinuous = false;
		//-B: it could be sparse (mean partial?!) if both m_bWContinuous 
		//	and m_bFullWConversionEverywhere are false (I guess)
}


//**************************************************************************************************
//---------------------------------FUNCTIONS: CASENAME_COMPUTEROUTE--------------------------------
//**************************************************************************************************

//****************** PAL2_SP ******************
LINK_COST WDMNetwork::PAL2_SP_ComputeRoute(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst,
										   BandwidthGranularity eBW)
{
	// compute route
	reset4ShortestPathComputation();
	pSrc->m_hCost = 0;
	pSrc->m_pStateWDM = new WDMNetwork(*this);

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ;
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=m_hNodeList.begin(); itrNode!=m_hNodeList.end(); itrNode++) {
		if ((*itrNode)->valid())
			hPQ.insert(*itrNode);
	}
	UINT nNumberOfAltPaths = pNetMan->getNumberOfAltPaths();
	OXCNode *pU, *pV;
	vector<AbstractNode*>::const_iterator itr;
	while (!hPQ.empty()) {
		pU = (OXCNode*)hPQ.peekMin();
		assert(pU);
		hPQ.popMin();
		if ((UNREACHABLE == pU->m_hCost) || (pU == pDst))
			break;

		for (itr=hPQ.m_hContainer.begin(); itr!=hPQ.m_hContainer.end(); itr++) {
			pV = (OXCNode*)(*itr);
			assert(pV);
			pU->m_pStateWDM->PAL2_SP_ComputeRoute_RelaxOneHop(pNetMan,
				pU, pV, eBW);
		} //  for itr

		hPQ.buildHeap();

#ifdef _OCHDEBUG8
		cout<<endl<<endl<<"Src = "<<pU->getId()<<endl;
		for (itr=hPQ.m_hContainer.begin(); itr!=hPQ.m_hContainer.end(); itr++) {
			pV = (OXCNode*)(*itr);
			assert(pV);
			pV->dump(cout);
		}
#endif
	} // while

	if (UNREACHABLE == pDst->m_hCost) {
		PAL2_SP_ComputeRoute_ReleaseMem(pSrc);
		return UNREACHABLE;
	}

	// retrieve route
	OXCNode *pOXC = pDst;
	while (pOXC != pSrc) {
		assert(pOXC->m_pPrevLink);
		Lightpath *pLP = (Lightpath*)(pOXC->m_pPrevLink);
		if (!pOXC->m_bNewLP) {
			hCircuit.m_hRoute.push_front(pLP);
			pOXC = pLP->getSrcOXC();
			assert(pOXC);
		} else {
			Lightpath *pNewLP = new Lightpath(0, Lightpath::LPT_PAL_Shared);
			list<UniFiber*>::const_iterator itr;
			for (itr=pLP->m_hPRoute.begin(); itr!=pLP->m_hPRoute.end(); itr++) {
				UniFiber *pFiber = 
					(UniFiber*)lookUpLinkById((*itr)->getId());
				assert(pFiber 
					&& (pFiber->m_pSrc->getId() == (*itr)->m_pSrc->getId())
					&& (pFiber->m_pDst->getId() == (*itr)->m_pDst->getId()));
				pNewLP->m_hPRoute.push_back(pFiber);
			}
			for (itr=pLP->m_hBRoute.begin(); itr!=pLP->m_hBRoute.end(); itr++) {
				UniFiber *pFiber = 
					(UniFiber*)lookUpLinkById((*itr)->getId());
				assert(pFiber 
					&& (pFiber->m_pSrc->getId() == (*itr)->m_pSrc->getId())
					&& (pFiber->m_pDst->getId() == (*itr)->m_pDst->getId()));
				pNewLP->m_hBRoute.push_back(pFiber);
			}

			hCircuit.m_hRoute.push_front(pNewLP);
			pOXC = pNewLP->getSrcOXC();
#ifdef _OCHDEBUG10
			if (NULL == pOXC) {
				cout<<"pLP@"<<(void*)pLP<<endl;
				pLP->dump(cout);
				cout<<endl<<"pNewLP@"<<(void*)pNewLP<<endl;
				pNewLP->dump(cout);
			}
#endif
		assert(pOXC);
		}
	} // while

	PAL2_SP_ComputeRoute_ReleaseMem(pSrc);
	return pDst->m_hCost;
}

inline void 
WDMNetwork::PAL2_SP_ComputeRoute_ReleaseMem(OXCNode* pSrc)
{
	// release memory
	OXCNode *pOXC;
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=m_hNodeList.begin(); itrNode!=m_hNodeList.end(); itrNode++) {
		pOXC = (OXCNode*)(*itrNode);
		if (pOXC->m_bNewLP) {
			delete pOXC->m_pPrevLink;
			pOXC->m_pPrevLink = NULL;
			delete pOXC->m_pStateWDM;
			pOXC->m_pStateWDM = NULL;
			pOXC->m_bNewLP = false;
		}
	}
	delete pSrc->m_pStateWDM;
}

inline void 
WDMNetwork::PAL2_SP_ComputeRoute_RelaxOneHop(NetMan* pNetMan,
											 OXCNode* pU, 
											 OXCNode* pV, 
											 BandwidthGranularity eBW)
{
	// NB: cost of a lightpath plays a big role here.  Currently, use
	//     the following policy:
	//     If there is an existing lightpath of sufficient capacity, use it
	Lightpath *pExistingLP = pNetMan->lookUpLightpathOfMinCost(pU, pV, eBW);
	list<AbstractLink*> hPPath, hBPath;
	LINK_COST hNewLPCost = UNREACHABLE;
	LINK_COST hExistingLPCost;
	if (pExistingLP) {
		hExistingLPCost = 
			(pExistingLP->getCost() - pNetMan->PAL2_GetSlack()) * eBW;
		if (hExistingLPCost < 0)
			hExistingLPCost = SMALL_COST;
	} else {
		hExistingLPCost = UNREACHABLE;

		if ((pU->m_nFreeTx > 0) && (pV->m_nFreeRx > 0)) {
			assert(pU->m_pStateWDM);
			OXCNode *pDupU = (OXCNode*)lookUpNodeById(pU->getId());
			OXCNode *pDupV = (OXCNode*)lookUpNodeById(pV->getId());
			assert(pDupU && pDupV);
			hNewLPCost = PAL2_SP_ComputeRoute_RelaxOneHop_Aux(hPPath, hBPath,
				pDupU, pDupV, pNetMan->getNumberOfAltPaths(), hExistingLPCost, eBW);
			// NB: hNewLPCost and hExistingLPCost are not comparable because
			//     in hNewLPCost, shared backup cost is not accounted
//			if (UNREACHABLE != hNewLPCost)
//				hNewLPCost = (hPPath.size() + hBPath.size()) * eBW;
			validateAllLinks();
		}
	}

	if (hNewLPCost < hExistingLPCost) {
		// use new lightpath
		if ((pU->m_hCost + hNewLPCost) < pV->m_hCost) {
			if (pV->m_bNewLP) {
				assert(pV->m_pPrevLink && pV->m_pStateWDM);
				delete pV->m_pPrevLink;
				delete pV->m_pStateWDM;
			}

			Lightpath *pNewLP = new Lightpath(0, Lightpath::LPT_PAL_Shared);
			pV->m_bNewLP = true;
			pV->m_pPrevLink = pNewLP;
			pV->m_pStateWDM = PAL2_SP_ComputeRoute_RelaxOneHop_NewState(*pNewLP,
								pU, pV,	hPPath, hBPath);
			pV->m_hCost = pU->m_hCost + hNewLPCost;
		}
	} else if (UNREACHABLE != hExistingLPCost) {
		// use existing lightpath
		assert(pExistingLP);
		if ((pU->m_hCost + hExistingLPCost) < pV->m_hCost) {
			if (pV->m_bNewLP) {
				assert(pV->m_pPrevLink && pV->m_pStateWDM);
				delete pV->m_pPrevLink;
				delete pV->m_pStateWDM;
			}

			// new stuff
			pV->m_bNewLP = false;
			pV->m_pPrevLink = pExistingLP;
			pV->m_pStateWDM = pU->m_pStateWDM;

			pV->m_hCost = pU->m_hCost + hExistingLPCost;
		}
	}
}

inline LINK_COST 
WDMNetwork::PAL2_SP_ComputeRoute_RelaxOneHop_Aux(list<AbstractLink*>& hPPath,
												 list<AbstractLink*>& hBPath,
												 OXCNode* pU,
												 OXCNode* pV,
												 UINT nNumberOfPaths,
												 LINK_COST hPCost,
												 BandwidthGranularity eBW)
{
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pU, pV, nNumberOfPaths, 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size())
		return UNREACHABLE;

	LINK_COST hBestCost = hPCost;
	list<AbsPath*>::const_iterator itrPPath;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		validateAllLinks();
		// update link cost for computing backup
		PAL2_SP_UpdateLinkCost_Backup((*itrPPath)->m_hLinkList);

		// compute backup
		LINK_COST hCurrBCost;
		list<AbstractLink*> hCurrBPath;
		hCurrBCost = Dijkstra(hCurrBPath, pU, pV, 
						AbstractGraph::LCF_ByOriginalLinkCost);
		if (UNREACHABLE != hCurrBCost) {
			LINK_COST hCurrPCost = (*itrPPath)->getCost() * eBW;
			if ((hCurrBCost + hCurrPCost) < hBestCost) {
				hBestCost = hCurrBCost + hCurrPCost;
				hPPath = (*itrPPath)->m_hLinkList;
				hBPath = hCurrBPath;
			}
		}

		delete (*itrPPath);
		// needed, otherwise, original cost will be overwritten
		restoreLinkCost();		
	}
	return hBestCost;
}

inline WDMNetwork* WDMNetwork::PAL2_SP_ComputeRoute_RelaxOneHop_NewState(Lightpath& hNewLP,
													  OXCNode *pU, 
													  OXCNode *pV,
											const list<AbstractLink*>& hPPath,
											const list<AbstractLink*>& hBPath)
{
	WDMNetwork *pNewState = new WDMNetwork(*(pU->m_pStateWDM));
	OXCNode *pDupU = (OXCNode*)pNewState->lookUpNodeById(pU->getId());
	OXCNode *pDupV = (OXCNode*)pNewState->lookUpNodeById(pV->getId());
	assert(pDupU && pDupV);
	
	// consume grooming-add/drop port
	assert((pDupU->m_nFreeTx > 0) && (pDupV->m_nFreeRx > 0));
	pDupU->m_nFreeTx--;
	pDupV->m_nFreeRx--;

	// consume wavelength along hPPath
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++) {
		UniFiber *pFiber = 
			(UniFiber*)pNewState->lookUpLinkById((*itr)->getId());
		assert(pFiber && (pFiber->m_pSrc->getId() == (*itr)->m_pSrc->getId())
			&& (pFiber->m_pDst->getId() == (*itr)->m_pDst->getId()));
		pFiber->consumeChannel(&hNewLP, -1);
		hNewLP.m_hPRoute.push_back((UniFiber*)pFiber);
	}

	// update backup rcs along hBPath
	for (itr=hBPath.begin(); itr!=hBPath.end(); itr++) {
		UniFiber *pBFiber = 
			(UniFiber*)pNewState->lookUpLinkById((*itr)->getId());
		assert(pBFiber && (pBFiber->m_pSrc->getId() == (*itr)->m_pSrc->getId())
			&& (pBFiber->m_pDst->getId() == (*itr)->m_pDst->getId()));

		UINT nNewBChannels = pBFiber->m_nBChannels;
		list<UniFiber*>::const_iterator itrPFiber;
		for (itrPFiber=hNewLP.m_hPRoute.begin(); 
		itrPFiber!=hNewLP.m_hPRoute.end(); itrPFiber++) {
			UINT nPFId = (*itrPFiber)->getId();
			assert(pBFiber->m_pCSet[nPFId] <= pBFiber->m_nBChannels);
			pBFiber->m_pCSet[nPFId]++;
			if (pBFiber->m_pCSet[nPFId] > nNewBChannels)
				nNewBChannels++;
			assert(nNewBChannels >= pBFiber->m_pCSet[nPFId]);
		}
		if (nNewBChannels > pBFiber->m_nBChannels) {
			pBFiber->m_nBChannels++;
			pBFiber->consumeChannel(NULL, -1);
			assert(pBFiber->m_nBChannels == nNewBChannels);
		}
		hNewLP.m_hBRoute.push_back(pBFiber);
	}
	return pNewState;
}

inline void 
WDMNetwork::PAL2_SP_UpdateLinkCost_Backup(const list<AbstractLink*>& hPPath)
{
	// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++)
		(*itr)->invalidate();

	// define link cost for computing backup
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		if (!(*itr)->valid())
			continue;
		UniFiber *pBFiber = (UniFiber*)(*itr);
		assert(pBFiber && pBFiber->m_pCSet);
		bool bShareable = true;
		list<AbstractLink*>::const_iterator itrFiber;
		for (itrFiber=hPPath.begin(); 
		(itrFiber!=hPPath.end()) && bShareable; itrFiber++) {
			if (pBFiber->m_nBChannels == 
				pBFiber->m_pCSet[(*itrFiber)->getId()])
				bShareable = false;
		}
		if (bShareable)
			pBFiber->modifyCost(SMALL_COST);
		else if (pBFiber->countFreeChannels() > 0)
			pBFiber->modifyCost(1);
		else
			pBFiber->invalidate();
	}
}

//****************** SEG_SP_NO_HOP *******************
inline void 
WDMNetwork::SEG_SP_NO_HOP_UpdateLinkCost_Backup(const list<AbstractLink*>& hPPath)
{
	// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++)
		(*itr)->invalidate();

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
		itr2ndToLast--;
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if (!(*itr)->valid()) continue;
		   UniFiber *pBFiber = (UniFiber*)(*itr);
		   assert(pBFiber && pBFiber->m_pCSet);
		   bool bShareable = true;
		   list<AbstractLink*>::const_iterator itrFiber;
		   for (itrFiber=hPPath.begin();(itrFiber!=itr2ndToLast) && bShareable; itrFiber++)
			{
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
				pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	} else {
	// case 2: primary traverses only one hop
        UINT nSrc = hPPath.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if (!(*itr)->valid())
				continue;
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
				pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	}
}

inline void 
WDMNetwork::SEG_SP_NO_HOP_UpdateLinkCost_BackupInvalidate(const list<AbstractLink*>& hPPath){

	// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++)
		(*itr)->invalidate();

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
		itr2ndToLast--;
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if (!(*itr)->valid()) continue;
		   UniFiber *pBFiber = (UniFiber*)(*itr);
		   assert(pBFiber && pBFiber->m_pCSet);
		   bool bShareable = true;
		   list<AbstractLink*>::const_iterator itrFiber;
		   for (itrFiber=hPPath.begin();(itrFiber!=itr2ndToLast) && bShareable; itrFiber++)
			{
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
				//pBFiber->modifyCost(SMALL_COST);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	} else {
	// case 2: primary traverses only one hop

		UINT nSrc = hPPath.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if (!(*itr)->valid())
				continue;
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
				//pBFiber->modifyCost(SMALL_COST);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	}
}

inline void 
WDMNetwork::SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(const list<AbstractLink*>& hPPath, const list<AbstractLink*>& hLinkToBeD,
                                                 SimulationTime hDuration, bool bSaveCI)
{// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++)
		(*itr)->invalidate();

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
		itr2ndToLast--;
		list<AbstractLink*>::const_iterator itrK;
		for (itrK=hLinkToBeD.begin(); itrK!=hLinkToBeD.end(); itrK++) {//scorro i link del grafo
			if (!(*itrK)->valid()) continue;
		   UniFiber *pBFiber = (UniFiber*)(*itrK);
		   assert(pBFiber && pBFiber->m_pCSet);
		   bool bShareable = true;
		   list<AbstractLink*>::const_iterator itrFiber;
		   for (itrFiber=hPPath.begin();(itrFiber!=itr2ndToLast) && bShareable; itrFiber++)//scorro il working
			{
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
			// I have to multiply the hDuration always for original cost and then
                            // sum the various contributes 
                                LINK_COST hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M               
                                LINK_COST hTimeCost= hCostOrig * hDuration ;               //-M
				                pBFiber->modifyCostCI(hTimeCost * m_dEpsilon,bSaveCI);    //-M 
			} else if (pBFiber->countFreeChannels() > 0) {
				                LINK_COST hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M        
                                LINK_COST hTimeCost= hCostOrig * hDuration;               //-M 
				 pBFiber->modifyCostCI(hTimeCost, bSaveCI);
			} else {
				pBFiber->invalidate();
			}
		}
	} 
	else {
	// case 2: primary traverses only one hop




		UINT nSrc = hPPath.front()->getSrc()->getId();
		list<AbstractLink*>::const_iterator itrK;
		for (itrK=hLinkToBeD.begin(); itrK!=hLinkToBeD.end(); itrK++) {
			if (!(*itrK)->valid())
				continue;
			UniFiber *pBFiber = (UniFiber*)(*itrK);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
			                    LINK_COST  hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M                             
                                LINK_COST hTimeCost= hCostOrig * hDuration;                //-M
			                    pBFiber->modifyCostCI(hTimeCost * m_dEpsilon,bSaveCI); //-M 
			} else if (pBFiber->countFreeChannels() > 0) {
				                LINK_COST  hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M
                                LINK_COST hTimeCost= hCostOrig *  hDuration ;                //-M 
			                    pBFiber->modifyCostCI(hTimeCost, bSaveCI);                 //-M
			} else {
				pBFiber->invalidate();
			}
		}
	}
												 
}

LINK_COST WDMNetwork::SEG_SP_NO_HOP_ComputeRoute(Circuit& hCircuit,
												 NetMan *pNetMan,
												 OXCNode* pSrc, 
												 OXCNode* pDst)
{int np=0;
	// obey fiber capacity
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

	// compute upto k primary candidates
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size()) {
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>
	AbsPath *pPPath = NULL;
	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;
	LINK_COST hCurrPCost, hCurrBCost;
	list<AbsPath*>::iterator itrPPath;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		AbsPath& hCurrPPath = **itrPPath;
		validateAllLinks();
        //validateAllNodes();//-----

		// redefine link cost for computing link/node-disjoint backup
		//SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList);

    //---------------------------------------------------------------------------------------------------------
      UINT nTimeInf = pNetMan->getTimePolicy(); // -M
                
	  switch (nTimeInf) {
      case 0: //No Time 
		 {                   
		  SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList);     
           break;
                 }
      case 1: // CI	
		 {
			 //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList);     
         
//-t
           SimulationTime hHTInc = pNetMan->m_hHoldingTimeIncoming;
           SimulationTime hATInc = pNetMan->m_hArrivalTimeIncoming;          
#ifdef DEBUGMASSIMO_
cout <<"                ArrTime " << hATInc   <<endl; 
cout <<"                HTINC " << hHTInc   <<endl; 
#endif
		   
		   list<AbstractLink*>  hLinkToBeD;
		   ExtractListUniFiber(hLinkToBeD); 
           bool bCopyCost=1;
           SimulationTime hTimeInterval;

           if (pNetMan->m_hDepaNet.empty()){
                 hTimeInterval= hHTInc; 
#ifdef DEBUGMASSIMO_
cout<<"                ZZ " <<  hTimeInterval <<endl; 
#endif
		         SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList); 

                 bCopyCost=0;
           } 
           else 
          {
                     Event *pEvent;
                     Event *pFollowingEvent;
                     list<Event*>::const_iterator itrE;
                     SimulationTime  hCurrT =  hATInc; 
                   
                     for (itrE=pNetMan->m_hDepaNet.begin(); itrE!=pNetMan->m_hDepaNet.end() ; itrE++) {
			            pEvent = (Event*)(*itrE);
                        assert(pEvent);
                        SimulationTime InitialTime=hCurrT;
                        SimulationTime hCurrT= pEvent->m_hTime;
//SimulationTime hCurrT_T= pEvent->m_hTime;
                        double HTFraction=1;  
                        if ( hHTInc/HTFraction + hATInc > hCurrT ){

						// if ( hHTInc + hATInc > hCurrT ){
                           if (itrE==(pNetMan->m_hDepaNet.begin())) {
                              SimulationTime hFirstEventT= pEvent->m_hTime;
                              if (hHTInc + hATInc <= hFirstEventT){
                                  hTimeInterval= (hHTInc); 
cout<<"                YY "<<endl;
#ifdef DEBUGMASSIMO_
cout<<"                YY " << hTimeInterval <<endl;
     //cerr <<  pEvent->m_pConnection->m_nSequenceNo;
#endif
				                  SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList);
                                  bCopyCost=0;
				                  break;

			                   }
							  else{
		      	                  hTimeInterval= (hFirstEventT - hATInc);
#ifdef DEBUGMASSIMO_ 

cout<<"                First " << hTimeInterval <<"       Depa_size_"<<pNetMan->m_hDepaNet.size()<<endl;
#endif
                                  bool bSave=0;
				                  SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList, hLinkToBeD, hTimeInterval,bSave);
                                  Lightpath *pLS; 
                                  list<Lightpath*> ListLightpathWork =  pEvent->m_pConnection->m_pPCircuit->m_hRoute;//P or B
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                        pLS = (Lightpath*)(*itrLS);
                                        assert(pLS);
                                    } //end for
                              }//end else hHTInc
                          } //end if itrE
//if ( hHTInc + hATInc > hCurrT ){                         
                          list<Event*>::const_iterator itrE2;
                          itrE2 = itrE;
                          itrE2++;
                          // if I don`t have a next event -> incoming ht
			              if (itrE2!=(pNetMan->m_hDepaNet.end())) {
			                  pFollowingEvent= (Event*)(*itrE2);
                              assert(pFollowingEvent);
			                  SimulationTime hNextDepT = pFollowingEvent->m_hTime;
			                  if (hHTInc + hATInc <= hNextDepT){
                                  hTimeInterval= (hHTInc + hATInc - hCurrT); 
#ifdef DEBUGMASSIMO_
cout<<"                terminal " << hTimeInterval <<endl;
#endif
                                  bool bSave=1; 
			                        SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList, hLinkToBeD, hTimeInterval,bSave);
                                  break; 
			                  }
			                  else {
			                      hTimeInterval=(hNextDepT -hCurrT); 
#ifdef DEBUGMASSIMO_
cout<<"                middle " << hTimeInterval <<endl;
#endif
                                  bool bSave=1;
                                   SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList, hLinkToBeD, hTimeInterval,bSave);
			                       Lightpath *pLS;
                              //Prima della correzione 
                                  list<Lightpath*> ListLightpathWork =  pFollowingEvent->m_pConnection->m_pPCircuit->m_hRoute;
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                    pLS = (Lightpath*)(*itrLS);
                                    assert(pLS);
                                    pLS->releaseOnServCopy(hLinkToBeD);
                                   }
			                       continue;
                                 } // end else 
			                 } // end if itrE2

			              else{ 
                              hTimeInterval= hHTInc + hATInc- hCurrT;
#ifdef DEBUGMASSIMO_
cout<<"                final " << hTimeInterval <<endl;
#endif
                              bool bSave=1;
                              SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 
			                  break; 
			                  }
                       
} //end if hHTInc/HTFraction + hATInc...  limitinf time of observation  
	                    }  //end for
		  } //end if Depanet Empty (not)
                 
		   // For the cases  in which  SEG_SP_NO_HOP_UpdateLinkCost_Backup has not been used
		   if (!bCopyCost==0) {
		                 //Needed just to invalidate link
                 SEG_SP_NO_HOP_UpdateLinkCost_BackupInvalidate(hCurrPPath.m_hLinkList);                    
	     	             //Map the new link costs assigned to hLinkToBeD in the m_LinkList  
                 UniFiber *pUFLTBD, *pUFm_h;
                 list<AbstractLink*>::const_iterator itrUFLTBD ,itrUFm_h ;
                 for (itrUFLTBD=hLinkToBeD.begin(); itrUFLTBD!=hLinkToBeD.end(); itrUFLTBD++) {
		                     pUFLTBD = (UniFiber*)(*itrUFLTBD);
                             assert(pUFLTBD);
	                         for (itrUFm_h=m_hLinkList.begin(); itrUFm_h!=m_hLinkList.end(); itrUFm_h++) {
		                          pUFm_h = (UniFiber*)(*itrUFm_h);
                                  assert(pUFm_h);
                                  if ((pUFLTBD -> m_nLinkId) == (pUFm_h -> m_nLinkId))
			                      {									  
									  LINK_COST hCostCI = pUFLTBD->getCost();
                                      pUFm_h->modifyCost(hCostCI);
                                    } // end if pUFLTBD
	                              } // end for itrUFm
                                 } //end for itrUFLTBD
		   } // end if bCopyCost 
		 
           list<AbstractLink*>::const_iterator itrLinkCI2;


           for (itrLinkCI2=hLinkToBeD.begin(); itrLinkCI2!=hLinkToBeD.end();itrLinkCI2++) {
 	              UniFiber *pUniFiberZ = (UniFiber*)(*itrLinkCI2);
                  UINT nW=pUniFiberZ->m_nW;
                  if (pUniFiberZ->m_pChannel) {          
			           delete []pUniFiberZ->m_pChannel;
                       pUniFiberZ->m_pChannel=NULL;
			          }
	        	  if (pUniFiberZ->m_pCSet) {
			           delete []pUniFiberZ->m_pCSet; 
                       pUniFiberZ->m_pCSet=NULL;
             	      } 
			      delete pUniFiberZ;
                        // hLinkToBeD.pop_back();
		          }  		   
	       break;                 
		 }

      case 2:
	  default:
      DEFAULT_SWITCH;
      };






	  
	  // transform the original graph
		list<AbstractLink*> hNewLinkList;
		SEG_SP_NO_HOP_Transform_Graph(hNewLinkList, hCurrPPath); 
		//-------t

		// compute the backup path
		list<AbstractLink*> hCurrBPath;
		hCurrBCost = Dijkstra(hCurrBPath, pSrc, pDst, 
			AbstractGraph::LCF_ByOriginalLinkCost);


//--------------t
#ifdef backup
		{
			np++;
			cout<<"hCurrPPath"<<"_(k"<<np<<")"<<endl;
hCurrPPath.dump(cout);

		list<AbstractLink*>::const_iterator itrddd ;
cout.precision(12);
cout<<"hCurrBPath"<<"_(k"<<np<<")"<<" C:"<<hCurrBCost<<endl;
for (itrddd=hCurrBPath.begin(); itrddd!=hCurrBPath.end(); itrddd++){
//(*itrddd)->dump(cout);
cout<<(*itrddd)->getSrc()->getId()<<"-"<<(*itrddd)->getDst()->getId()<<" ";
}
cout<<endl;}
#endif


		// restore graph
		list<AbstractLink*>::const_iterator itrLink;
		for (itrLink=hNewLinkList.begin(); itrLink!=hNewLinkList.end(); 
		itrLink++) {
			removeLink(*itrLink);
			// these links will be used by SEG_SP_NO_HOP_Parse_Backup_Segs
			// later, delete them after that.
		}

		if (UNREACHABLE != hCurrBCost) {
			// extract backup segments
			list<AbsPath*> hCurrBSegs;
			SEG_SP_NO_HOP_Parse_Backup_Segs(hCurrBSegs, hCurrPPath, hCurrBPath);
			// calculate real cost for backup segments
			hCurrBCost = 
				SEG_SP_NO_HOP_Calculate_BSegs_Cost(hCurrPPath, hCurrBSegs);

			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCost) < hBestCost) {
				hBestCost = hCurrPCost + hCurrBCost;
				pBestP = *itrPPath;
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				hBestB = hCurrBSegs;

//----t
/*#ifdef _Compute_Route
{cout<<"*pBestP "<<endl;
pBestP->dump(cout);
}
#endif*/

/*list<AbstractLink*>::const_iterator itrddd;
for (itrddd=hCurrPPath.m_hLinkList.begin(); itrddd!=hCurrPPath.m_hLinkList.end(); itrddd++) {
(*itrddd)->dump(cout);
}*/
//---t
			} else {
				// release memory
				list<AbsPath*>::iterator itr;
				for (itr=hCurrBSegs.begin(); itr!=hCurrBSegs.end(); itr++) {
					delete (*itr);
				}
			}
		} // if

		for (itrLink=hNewLinkList.begin(); itrLink!=hNewLinkList.end(); 
		itrLink++) {
			delete (*itrLink);
		}

		// needed, otherwise, original cost will be overwritten
		restoreLinkCost();		
	}//end for itrPPath

//----t
#ifdef _Compute_Route
{cout<<"*pBestP*"<<endl;
pBestP->dump(cout);
cout<<"*hBestB*"<<endl;
list<AbsPath*>::const_iterator itrdd;
int cx = 0;
for (itrdd=hBestB.begin(); itrdd!=hBestB.end(); itrdd++) {
cout<<"seg_nr_"<<cx<<": ";
(*itrdd)->dump(cout);
cx++;
//cout.seekp(5,ios_base::beg);
}
}
#endif


	if (UNREACHABLE != hBestCost) {
		assert(pBestP);
		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);
		list<AbstractLink*>::const_iterator itr;
		for (itr=pBestP->m_hLinkList.begin(); itr!=pBestP->m_hLinkList.end();
		itr++) {
			   pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		       }
		pLPSeg->m_hCost = pBestP->getCost();
//
//assert(hBestB.size()>0); -T

		pLPSeg->m_hBackupSegs = hBestB;
		hCircuit.addVirtualLink(pLPSeg);
	}
//-----t
//----------
/*cout<<"hBestB :"<<endl;
list<AbsPath*>::const_iterator itrdd;
for (itrdd=hBestB.begin(); itrdd!=hBestB.end(); itrdd++) {
(*itrdd)->dump(cout);
}*/

//------
	// delete primary candidate
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}
	return hBestCost;
}

inline LINK_COST WDMNetwork::SEG_SP_NO_HOP_Calculate_BSegs_Cost(const AbsPath& hPPath, 
																const list<AbsPath*>& hBSegs)
{
	LINK_COST hBSegCost = 0;
	list<AbsPath*>::const_iterator itr;
	list<AbstractLink*>::const_iterator itrP, itrB;
	if (hPPath.m_hLinkList.size() > 1) {
		// case 1: primary traverses multiple (>1) hops
		list<AbstractLink*>::const_iterator itrPIndex = hPPath.m_hLinkList.begin();
		itrPIndex++;	// only consider intermediate node failures
		for (itr=hBSegs.begin(); itr!=hBSegs.end(); itr++) {//ricorro su ogni segm trovato
			LINK_COST hCurrBSegCost = 0;
			UINT nSegEndNodeId = (*itr)->m_hLinkList.back()->getDst()->getId();
			for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();itrB++) //ricorro su ogni link di 1 segm
			{	UniFiber *pBFiber = (UniFiber*)(*itrB);
				assert(pBFiber && pBFiber->m_pCSet);
				bool bShareable = true;
				for (itrP=itrPIndex; ((itrP!=hPPath.m_hLinkList.end()) //analizzo i link del PPath
					&& ((*itrP)->getSrc()->getId()!=nSegEndNodeId)); itrP++) {//caso in cui ho 1 solo segm per tutto il PPath
					assert(pBFiber->m_nBChannels >= 
						pBFiber->m_pCSet[(*itrP)->getSrc()->getId()]);
					if (pBFiber->m_nBChannels == 
						pBFiber->m_pCSet[(*itrP)->getSrc()->getId()])
						bShareable = false;
				}
				if (bShareable) {
					hCurrBSegCost += SMALL_COST;
				} else {
#ifdef _OCHDEBUGA
					if (pBFiber->countFreeChannels() <= 0) {
						cout<<endl<<"- ERROR: Segment information"<<endl;
						hPPath.dump(cout);
						for (itr=hBSegs.begin(); itr!=hBSegs.end(); itr++) {
							(*itr)->dump(cout);
						}
						cout<<"pBFiber"<<endl;
						pBFiber->dump(cout);
						cout<<"- WDM network state"<<endl;
						this->dump(cout);
					}
#endif
					assert(pBFiber->countFreeChannels() > 0);
					hCurrBSegCost += pBFiber->getCost();
				}
			} // for itrB 

			hBSegCost += hCurrBSegCost;
			itrPIndex = itrP;
		} // for itr
	} else {
		// case 2: primary traverses single hop
		UINT nSrc = hPPath.m_hLinkList.front()->getSrc()->getId();
		for (itr=hBSegs.begin(); itr!=hBSegs.end(); itr++) {
			for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
			itrB++) {
				UniFiber *pBFiber = (UniFiber*)(*itrB);
				assert(pBFiber && pBFiber->m_pCSet);
				assert(pBFiber->m_pCSet[nSrc] <= pBFiber->m_nBChannels);
				bool bShareable = 
					(pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
				if (bShareable) {
					hBSegCost += SMALL_COST;
				} else {
					assert(pBFiber->countFreeChannels() > 0);
					hBSegCost += pBFiber->getCost();
				}
			} // for itrB
		} // for itr
	}
	// prefer more segments in case of a tie
	hBSegCost -= (hBSegs.size() - 1) * SMALL_COST;
	return hBSegCost;
}


inline void WDMNetwork::SEG_SP_NO_HOP_Parse_Backup_Segs(list<AbsPath*>& hCurrBSegs, 
														const AbsPath& hPPath,
														const list<AbstractLink*>& hBPath)
{//definisce ls soglia min di hop del hPPath dopo la quale fare il dump ---t
	UINT tot_link_hPPath_ = 0;

	// extract the set of node IDs on the primary path
	set<UINT> hPPathSet;
	map<UINT, AbstractNode*> hSuccSet;
	list<AbstractLink*>::const_iterator itrPS;
	UINT nNodeId;
	for (itrPS=hPPath.m_hLinkList.begin(); itrPS!=hPPath.m_hLinkList.end();
	itrPS++) {
		nNodeId = (*itrPS)->getSrc()->getId();
		hPPathSet.insert(nNodeId);
		hSuccSet.insert(pair<UINT, AbstractNode*>(nNodeId, (*itrPS)->getDst()));
	}
	UINT nDstNodeId = hPPath.m_hLinkList.back()->getDst()->getId();
	hPPathSet.insert(nDstNodeId);//intero set ordinato dei nodi

	// parse backup segments
	list<AbstractLink*>::const_iterator itrB = hBPath.begin();

/*---prova_-----------------------------------------------------------------------------
if (hPPathSet.find((*itrB)->getSrc()->getId()) != hPPathSet.begin())
 return ;*/
#ifdef _Parse_Backup_Segs_
	{
list<AbstractLink*>::const_iterator itrBB = hBPath.end();
//list<AbstractLink*>::const_iterator itrBE;
itrBB--;
int ppp=0;
cout<<(*itrBB)->getDst()->getId()<<"<-";
for (itrBB ;(itrBB != hBPath.begin()) /*&& (hPPathSet.find((*itrBB)->getSrc()->getId()) != hPPathSet.begin())*/;itrBB--){
    cout<<(*itrBB)->getSrc()->getId()<<"<-";
	ppp++;
	/*if (hPPathSet.find((*(itrBB++))->getSrc()->getId()) == hPPathSet.begin()){
		cout<<" |"<<(*itrBB)->getSrc()->getId()<<"| ";
	}*/

	//itrBE=itrBB;
}

cout<<(*itrBB)->getSrc()->getId();


cout<<"     ("<<ppp<<")_"<<endl;}
#endif

//---prova_-----------------------------------------------------------------------------
	
	while (itrB != hBPath.end()) {//per ogni link percorso backup 
		AbsPath *pBSeg = new AbsPath(); 
		hCurrBSegs.push_back(pBSeg);//inserisco un nuovo segmento vuoto nella lista

		// forward direction
		while (hPPathSet.find((*itrB)->getDst()->getId()) == hPPathSet.end()) {//finche' fine P coincide fine B

	
	        pBSeg->m_hLinkList.push_back(*itrB);
			itrB++;
              }

		// handle chord (one-hop backup segment)
		// and the last hop of the last backup segment
		if ((hPPathSet.find((*itrB)->getSrc()->getId()) != hPPathSet.end())
		|| (nDstNodeId == (*itrB)->getDst()->getId())) {
			pBSeg->m_hLinkList.push_back(*itrB);
		} else {
			// handle the transformed links
			UINT nSrc = (*itrB)->getSrc()->getId();
			UINT nDst = hSuccSet.find((*itrB)->getDst()->getId())->second->getId();//---t
			
			


    		AbstractLink *pLink = lookUpLink(nSrc, nDst);
#ifdef _OCHDEBUGA
			if (NULL == pLink) {
				cout<<endl<<"- Backup segments so far"<<endl;
				list<AbsPath*>::const_iterator itr;
				for (itr=hCurrBSegs.begin(); itr!=hCurrBSegs.end(); itr++) {
					(*itr)->dump(cout);
				}
			}
#endif
			assert(pLink);
    		pBSeg->m_hLinkList.push_back(pLink);
		}

		// handle backward links
		UINT nUpStreamNode, nDownStreamNode, nSuccNode;
		do {
			itrB++;
			if (itrB == hBPath.end()) {
				break;
			}
			nUpStreamNode = (*itrB)->getSrc()->getId();
			nDownStreamNode = (*itrB)->getDst()->getId();
			map<UINT, AbstractNode*>::const_iterator itr = 
						hSuccSet.find(nDownStreamNode);	// backward link
			if (itr != hSuccSet.end()) {
				nSuccNode = itr->second->getId();
			} else {
				nSuccNode = nUpStreamNode + 1;	// set to unequal
			}
		} while (nUpStreamNode == nSuccNode);
	}
///
#ifdef _Parse_Backup_Segs
	{if(hPPath.m_hLinkList.size()>=tot_link_hPPath_){
	    cout<<"- After SEG_SP_NO_HOP_Parse_Backup_Segs"<<endl;
		hPPath.dump(cout);
		list<AbsPath*>::const_iterator itr;
		for (itr=hCurrBSegs.begin(); itr!=hCurrBSegs.end(); itr++) {
			(*itr)->dump(cout);
	}
	}}

#endif



}

inline void 
WDMNetwork::SEG_SP_NO_HOP_Transform_Graph(list<AbstractLink*>& hNewLinkList,
										  const AbsPath& hPPath)
{//definisce ls soglia min di hop del hPPath dopo la quale fare il dump
	UINT tot_link_hPPath = 0;
	
	// add, if necessary, links along the reverse hPPath & assign 0 cost
	list<AbstractLink*>::const_reverse_iterator itr;
	for (itr=hPPath.m_hLinkList.rbegin(); itr!=hPPath.m_hLinkList.rend();
	itr++) {
		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pV = (*itr)->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pV->m_hOLinkList.begin();//esamino dal penultimo nodo al primo i link entranti(opposite) nel nodo
		(itrO!=pV->m_hOLinkList.end()) && ((*itrO)->m_pDst!=(*itr)->m_pSrc); 
		itrO++)	NULL;          //nodo in esame e' src e dst allo stesso tempo,allora il link e'su hPPath ma opposto 
                               //per uscire dal for basta che una condizione mi diventi == 
///----t
#ifdef _Transform_Graph
{if(hPPath.m_hLinkList.size()>=tot_link_hPPath){
    if (itr==hPPath.m_hLinkList.rbegin()){
	cout<<endl;
	cout<<"hPPath:";
	hPPath.dump(cout);}

    list<AbstractLink*>::const_iterator itraac;
    cout<<"- m_hOLinkList del nodo "<<(*itr)->m_pDst->getId()<<endl;
    for (itraac=pV->m_hOLinkList.begin(); itraac!=pV->m_hOLinkList.end(); itraac++) {
(*itraac)->dump(cout);
//cout<<endl;
	}
			}}
#endif  //---t




		// no link at the opposite direction, add one
		if (itrO == pV->m_hOLinkList.end()) { //se dal for di prima sono uscito per la prima condiz (ma l'altra non soddisfatta)
			int nNewLinkId = getNextLinkId();
//----t
#ifdef _Transform_Graph
			{
				cout<<"- Add opposite link: "<<nNewLinkId<<" ";
				cout<<(*itr)->m_pDst->getId()<<"->";
				cout<<(*itr)->m_pSrc->getId()<<endl;
				this->dump(cout);
			}
#endif
			pOppositeLink = this->addLink(nNewLinkId, 
				(*itr)->m_pDst->getId(), (*itr)->m_pSrc->getId(),
				0, 1);
			assert(pOppositeLink);
			hNewLinkList.push_back(pOppositeLink);
		} else {
			pOppositeLink = *itrO;
			pOppositeLink->validate();
			pOppositeLink->restoreCost();	// cost has been modified
		}
		pOppositeLink->modifyCost(0);//

////-t
#ifdef _Transform_Graph
{if(hPPath.m_hLinkList.size()>=tot_link_hPPath){

cout<<"-opp_link_sul_hPPath"<<endl;
pOppositeLink->dump(cout);
//cout<<endl;		
}}
#endif


	}

	// the set of node IDs on the primary path
	set<UINT> hPPathSet;	
	// the set of node IDs on the primary path except the src & dst
	set<UINT> hInterSet;	
	map<UINT, AbstractNode*> hPredSet;
	list<AbstractLink*>::const_iterator itrPS, itrSecondToLast;
	UINT nNodeId;
	itrPS=hPPath.m_hLinkList.begin();
	itrSecondToLast = hPPath.m_hLinkList.end();
	itrSecondToLast--;

	for (itrPS; (itrPS!=itrSecondToLast) /*&& (itrPS!=hPPath.m_hLinkList.end())*/;

	itrPS++) {
		nNodeId = (*itrPS)->getDst()->getId();
		hPPathSet.insert(nNodeId);
		hInterSet.insert(nNodeId);
		hPredSet.insert(pair<UINT, AbstractNode*>(nNodeId, (*itrPS)->getSrc()));
	}
	hPPathSet.insert(hPPath.m_hLinkList.front()->getSrc()->getId()); // src
	hPPathSet.insert(hPPath.m_hLinkList.back()->getDst()->getId()); // dst

	// do the following:
	// for any link <u, v> s.t. v is along the primary path & u is not,
	// invalidate link <u, v> and add link <u, pred(v)> pred of v along primary
	list<AbstractLink*> hLinksToMove;
	list<AbstractLink*>::const_iterator itrLink,itrLinkM;
//---------t
#ifdef _Transform_Graph

{if(hPPath.m_hLinkList.size()>=tot_link_hPPath){
	cout<<endl;
    cout<<"nodi_da_spost"<<endl;
	}}
#endif
//----------t

	for (itrLink=m_hLinkList.begin(); itrLink!=m_hLinkList.end(); itrLink++) {
		if (!(*itrLink)->valid()) {
			continue;
		}
		AbstractNode *pU = (*itrLink)->m_pSrc;
		AbstractNode *pV = (*itrLink)->m_pDst;

		if ((hPPathSet.find(pU->getId()) == hPPathSet.end())
&& (hInterSet.find(pU->getId()) != hPPathSet.begin())
		  && (hInterSet.find(pV->getId()) != hInterSet.end())
		  ) {
			
//modifica--t
       // if ((hPPathSet.find(pU->getId()) == NULL) 
		//	&& (hInterSet.find(pV->getId()) != NULL)) {
			
		
		(*itrLink)->invalidate();

//---------t
#ifdef _Transform_Graph
	{if(hPPath.m_hLinkList.size()>=tot_link_hPPath){

    cout<<(*itrLink)->m_pSrc->getId()<<"->"<<(*itrLink)->m_pDst->getId()<<" C="<<(*itrLink)->m_hCost<<endl;
	}}
#endif
//----------t
			hLinksToMove.push_back(*itrLink);
		}
	}
	
LINK_COST ccost;

	// add link
	for (itrLinkM=hLinksToMove.begin(); itrLinkM!=hLinksToMove.end(); itrLinkM++) {
		UINT nSrc = (*itrLinkM)->m_pSrc->getId();
		UINT nDst = hPredSet.find((*itrLinkM)->m_pDst->getId())->second->getId();
//----------t
if((*itrLinkM)->m_hCost - SMALL_COST>=0){
	ccost=(*itrLinkM)->m_hCost - SMALL_COST;
}else{
	ccost=(*itrLinkM)->m_hCost;}
//----------t

		if (UNREACHABLE != (*itrLinkM)->m_hCost) {
			AbstractLink *pNewLink = this->addLink(getNextLinkId(), nSrc, nDst, 
				// minus SMALL_COST to prefer segmented backups in case of tie
				ccost, //----------t
				1);
			assert(pNewLink);
			hNewLinkList.push_back(pNewLink);
		}
	}
/////////////--t
#ifdef _Transform_Graph
	{if(hPPath.m_hLinkList.size()>=tot_link_hPPath){
	
	
	cout<<"-hNewLinkList"<<endl;
	//hPPath.dump(cout);
	list<AbstractLink*>::const_iterator itraa,itracccc;
	for (itraa=hNewLinkList.begin(); itraa!=hNewLinkList.end(); itraa++) {
		(*itraa)->dump(cout);
        //cout<<endl;
		}
	cout<<endl;
	//for (itracccc=this->m_hLinkList.begin(); itracccc!=this->m_hLinkList.end(); itracccc++) {
	//	(*itracccc)->dump(cout);
	//    }
	
	//this->dump(cout);
	
		
	}}
#endif

	
}

//****************** UNPROTECTED *******************
//-B: return the cost of the best path found connecting source and destination
//	and add it to the m_hRoute Lightpath list, attribute of a Circuit object
LINK_COST WDMNetwork::UNPROTECTED_ComputeRoute(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)
{
	int k1 = 0;
	// obey fiber capacity

	//-B: set false the boolean var that indicates the validity of the AbstractLink (-> UniFiber) object, when there are no free channels
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

// 	       UniFiber *pUF2;
//                 list<AbstractLink*>::const_iterator itrUF2;
//                 for (itrUF2=m_hLinkList.begin(); itrUF2!=m_hLinkList.end(); itrUF2++) {
// 		 pUF2 = (UniFiber*)(*itrUF2);
//                  assert(pUniFiber);
// 		 pUF2->dump(cout);
//                 // cerr <<  pUF-> m_hLinkId << endl; 
//                 // cerr <<  pUF-> m_hCost << endl; 
// 		}

	//COMPUTE UP TO K PRIMARY CANDIDATES (K set equal to 1)
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, 1, //K=1 !!! non avrebbe motivo d'essere diverso.. -> trova solo 1 path
		AbstractGraph::LCF_ByOriginalLinkCost);
	//-B: if there are no admissible paths in the list hPPathList, then UNREACHABLE
	if (0 == hPPathList.size()) {//cout<<"zzz ";
		validateAllLinks();
		return UNREACHABLE;
	}
	//-B: else, if there is at least one admissible path between source and destination, calculate the cost of the best (the first and only of the list) primary path
	LINK_COST hBestCost = UNREACHABLE;
	LINK_COST hBestCostProv = UNREACHABLE;
	int k = 0;
	LINK_COST dummycost;
	AbsPath *pBestP = NULL;
	list<AbsPath*>::iterator itrPPath;
	itrPPath = hPPathList.begin();
	pBestP = (*itrPPath); //-B: pBestP points to the first (and only) element of hPPathList, that is the best path
	assert(pBestP);
	// construct the circuit //-B: Lightpath_Seg is a class derived from LighPath, that is a class derived from AbstractLink
	Lightpath_Seg *pLPSeg = new Lightpath_Seg(0); //LEAK!
	//-B: for each link of the first (best) path
	for (itr = pBestP->m_hLinkList.begin(); itr != pBestP->m_hLinkList.end(); itr++)
	{
		pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr)); //-B: m_hPRoute is a list of UniFiber object in the class Lightpath;
														//	itr is a AbstractLink object reference;
														//	Unifiber class (as well as Lightpath) is derived from AbstractLink
		k++;
		//-B: if the destination of the link itr is the dummy node, the cost to reach the dummy node will be simply the link cost
		if ((*itr)->getDst()->getId() == pNetMan->m_hWDMNet.DummyNode)
		{
			dummycost = (*itr)->getCost();  //costo del link che collega il DC col dummynode //-B: ma poi che se ne fa???
		}
	}
	hBestCost = pLPSeg->m_hCost = pBestP->getCost(); //-B: the cost variable of the AbsPath object is assigned to the cost var of the Lightpath object (derived from Lightpath -> derived from AbstractLink), that is assigned to the var hBestCost
	hBestCostProv = pLPSeg->m_hCost = pBestP->getCost(); //-B: fa la stessa cosa dell'istruzione successiva?!?
	hBestCost = hBestCostProv; //sistemare
	hCircuit.addVirtualLink(pLPSeg); //-B: add it to the m_hRoute Lightpath list, attribute of a Circuit object
	
	// delete primary candidate from hPPathList -> the list will be empty, since it was the only element in the list -> Yen's algorithm with k=1
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}

#ifdef DEBUGB
	//stampare lo stato degli attributi di pLPSeg e pBestP per vedere se è stata assegnata qualche wavel o no, e se si quale e come
#endif // DEBUGB

	return hBestCost;
}

//****************** DEDICATED *******************
inline void WDMNetwork::DEDICATED_UpdateBackupCostWP(const list<AbstractLink*>& hPPath, set<Lightpath*>& result, bool isLinkDisjoint)
{
	list<AbstractLink*>::const_iterator itr;
	if (isLinkDisjoint)
	{
		for (itr = hPPath.begin(); itr != hPPath.end(); itr++) {
			//cout<<(*itr)->getDst()->getId()<<"_"<<endl;
			(*itr)->invalidate();//invalido i link


		}

	}
	else
	{
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
		list<AbstractLink*>::const_iterator startPoint = hPPath.begin();
		itr2ndToLast--;

		// invalidate working path

		if (hPPath.size()>1)
		{
			for (itr = hPPath.begin(); itr != itr2ndToLast; itr++) {
				//cout<<(*itr)->getDst()->getId()<<"_"<<endl;
				(*itr)->getDst()->invalidate();//nodo-digiunzione

			}
			startPoint++;
		}
		else {
			itr = hPPath.begin();
			(*itr)->invalidate();
		}


	}//END NODE DISJOINT



}



//****************** PAL_DPP *******************
LINK_COST WDMNetwork::PAL_DPP_ComputeRoute(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)
{int k1 =0;
	// obey fiber capacity

	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}
// 	       UniFiber *pUF2;
//                 list<AbstractLink*>::const_iterator itrUF2;
//                 for (itrUF2=m_hLinkList.begin(); itrUF2!=m_hLinkList.end(); itrUF2++) {
// 		 pUF2 = (UniFiber*)(*itrUF2);
//                  assert(pUniFiber);
// 		 pUF2->dump(cout);
//                 // cerr <<  pUF-> m_hLinkId << endl; 
//                 // cerr <<  pUF-> m_hCost << endl; 
// 		}
	
	list<AbsPath*> hPPathList;
	list<AbstractLink*> LinkList;
	AbstractPath path1=AbstractPath();
	AbstractPath path2=AbstractPath();;
	LINK_COST hBestCost = UNREACHABLE;
	hBestCost=this->Suurballe(path1,path2, pSrc, pDst, 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (hBestCost == UNREACHABLE) {//cout<<"zzz ";
		validateAllLinks();
		return UNREACHABLE;
	}
			// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);//LEAK!

		
		pLPSeg->backupLength=path1.m_hLinkList.size();
		if(path2.m_hLinkList.size()>pLPSeg->backupLength) 
			{
			pLPSeg->backupLength=path2.m_hLinkList.size()-1;
			pLPSeg->primaryLength=path1.m_hLinkList.size()-1;
				}
		else
			{
			pLPSeg->backupLength=path1.m_hLinkList.size()-1;
			pLPSeg->primaryLength=path2.m_hLinkList.size()-1;
				}
	list<unsigned int>::iterator itr2;
	list<unsigned int>::iterator itr2N;

	list<unsigned int>::iterator itr2Pen;//punta al penultimo elm
	itr2Pen=path1.m_hLinkList.end();
	itr2Pen--;
	itr2N=path1.m_hLinkList.begin();
	itr2N++; //punta al secondo el
	AbstractLink* link;
	AbstractNode* node;
	for(itr2=path1.m_hLinkList.begin();itr2!=itr2Pen;itr2++)

	{
		node=lookUpNodeById((*itr2));
	link=node->lookUpOutgoingLink((*(itr2N)));
	pLPSeg->m_hPRoute.push_back((UniFiber*)link);
	itr2N++;
	}
	////////

	itr2Pen=path2.m_hLinkList.end();
	itr2Pen--;
	itr2N=path2.m_hLinkList.begin();
	itr2N++; //punta al secondo el
	for(itr2=path2.m_hLinkList.begin();itr2!=itr2Pen;itr2++)

	{
		node=lookUpNodeById((*itr2));
	link=node->lookUpOutgoingLink((*(itr2N)));
	pLPSeg->m_hPRoute.push_back((UniFiber*)link);
	itr2N++;
	}

	hCircuit.addVirtualLink(pLPSeg);
	
	


	return hBestCost;
}

//****************** SEG_SPP *******************
LINK_COST WDMNetwork::SEG_SPP_ComputeRoute(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)
{int k1 =0;
	// obey fiber capacity

	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}
// 	       UniFiber *pUF2;
//                 list<AbstractLink*>::const_iterator itrUF2;
//                 for (itrUF2=m_hLinkList.begin(); itrUF2!=m_hLinkList.end(); itrUF2++) {
// 		 pUF2 = (UniFiber*)(*itrUF2);
//                  assert(pUniFiber);
// 		 pUF2->dump(cout);
//                 // cerr <<  pUF-> m_hLinkId << endl; 
//                 // cerr <<  pUF-> m_hCost << endl; 
// 		}
	// compute upto k primary candidates
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size()) {//cout<<"zzz ";
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>
	AbsPath *pPPath = NULL;
	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;
	LINK_COST hCurrPCost, hCurrBCost;
	list<AbsPath*>::iterator itrPPath;
//	int mod=0;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
	  AbsPath& hCurrPPath = **itrPPath;
	  validateAllLinks();
	  validateAllNodes();
	       
/*#ifdef _OCHDEBUGA
//		{
//			hCurrPPath.dump(cout);
//		}
#endif*/
		// redefine link cost for computing link/node-disjoint
                //    backup according to various Time policy --M
      UINT nTimeInf = pNetMan->getTimePolicy(); // -M
                
      switch (nTimeInf) {
      case 0: //No Time 
		 {                    
		   SEG_SPP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList,pNetMan->isLinkDisjointActive);     
           break;
                 }
       default:
      DEFAULT_SWITCH;
      };

		// compute the backup path
		list<AbstractLink*> hCurrBPath;
		hCurrBCost = Dijkstra(hCurrBPath, pSrc, pDst, 
			AbstractGraph::LCF_ByOriginalLinkCost);



		if (UNREACHABLE != hCurrBCost) {
			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCost) < hBestCost) {
				hBestCost = hCurrPCost + hCurrBCost;
//				mod++;
				pBestP = *itrPPath;
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				hBestB.clear();
				AbsPath *pBackup = new AbsPath();//LEAK!
				pBackup->m_hLinkList = hCurrBPath;
				hBestB.push_back(pBackup);
			}
		} // if
                //delete hLinkToBeD; //dynamic memory management for 
		        // needed, otherwise, original cost will be overwritten
		restoreLinkCost();	

	}//end for itrPPath  


//if (mod==2) cout<<"secondo!"<<endl;

	if (UNREACHABLE != hBestCost) {
		assert(pBestP);
		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);//LEAK!
	
		list<AbstractLink*>::const_iterator itr;
		for (itr=pBestP->m_hLinkList.begin(); itr!=pBestP->m_hLinkList.end();itr++) {
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		}
		pLPSeg->m_hCost = pBestP->getCost();
//
assert(hBestB.size()>0);

		pLPSeg->m_hBackupSegs = hBestB;
		hCircuit.addVirtualLink(pLPSeg);
	
	}
	// delete primary candidate
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}
	return hBestCost;
}

//****************** SPPCh *******************
LINK_COST WDMNetwork::SPPCh_ComputeRoute(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)
{int k1 =0;
	// obey fiber capacity
//ghgh
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}
// 	       UniFiber *pUF2;
//                 list<AbstractLink*>::const_iterator itrUF2;
//                 for (itrUF2=m_hLinkList.begin(); itrUF2!=m_hLinkList.end(); itrUF2++) {
// 		 pUF2 = (UniFiber*)(*itrUF2);
//                  assert(pUniFiber);
// 		 pUF2->dump(cout);
//                 // cerr <<  pUF-> m_hLinkId << endl; 
//                 // cerr <<  pUF-> m_hCost << endl; 
// 		}
	// compute upto k primary candidates
	AbsPath* hPPath=new AbsPath();
	LINK_COST hCost=SPPCh(hPPath, pSrc,pDst, numberOfChannels,false);
	if (hCost == UNREACHABLE) {//cout<<"zzz ";
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>

	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost

	list<AbsPath*> hBestB;
	LINK_COST hCurrBCost;
//	LINK_COST hCurrPCost;
	list<AbsPath*>::iterator itrPPath;

	 
	  validateAllLinks();
	  validateAllNodes();
	       
/*#ifdef _OCHDEBUGA
//		{
//			hCurrPPath.dump(cout);
//		}
#endif*/
		// redefine link cost for computing link/node-disjoint
                //    backup according to various Time policy --M
      UINT nTimeInf = pNetMan->getTimePolicy(); // -M
                
      switch (nTimeInf) {
      case 0: //No Time 
		 {                    
		   SEG_SPP_UpdateLinkCost_Backup(hPPath->m_hLinkList,pNetMan->isLinkDisjointActive);     
           break;
                 }
       default:
      DEFAULT_SWITCH;
      };

		// compute the backup path
		AbsPath* hCurrBPath=new AbsPath();
set<Lightpath*> LPList;//lista contenente tutti i lightpath presenti nel link attraversato dal primario

	SEG_SPP_UpdateBackupCostWP(hPPath->m_hLinkList,LPList,pNetMan->isLinkDisjointActive);//Lista abstract link*) DEL PRIMARIO CONSIDERATO
		
		hCurrBCost =SPPCh(hCurrBPath, pSrc,pDst, numberOfChannels,true);

SEG_SPP_RestoreBackupCostWP(LPList);

		if (UNREACHABLE != hCurrBCost) {
			// compare cost
				assert(hCurrBPath->m_hLinkList.size()>0);
			hBestB.push_back(hCurrBPath);
			
		} // if
           else return UNREACHABLE;
		   //delete hLinkToBeD; //dynamic memory management for 
		   // needed, otherwise, original cost will be overwritten
		restoreLinkCost();	
			
		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);//LEAK!
	
		list<AbstractLink*>::const_iterator itr2;
		for (itr2=hPPath->m_hLinkList.begin(); itr2!=hPPath->m_hLinkList.end();itr2++) {
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr2));
		}
		pLPSeg->m_hCost = hPPath->getCost();
//


		pLPSeg->m_hBackupSegs = hBestB;

		assert( pLPSeg->m_hBackupSegs.size()>0 );

		hCircuit.addVirtualLink(pLPSeg);
	pLPSeg->wlAssigned=hPPath->wlAssigned;
//assert(hPPath->wlAssigned<=16);
	pLPSeg->wlAssignedBCK=hCurrBPath->wlAssigned;
	

	return hCost+hCurrBCost;
}

//****************** SPPBw *******************
LINK_COST WDMNetwork::SPPBw_ComputeRoute(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)
{int k1 =0;
	// obey fiber capacity
//ghgh
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}
// 	       UniFiber *pUF2;
//                 list<AbstractLink*>::const_iterator itrUF2;
//                 for (itrUF2=m_hLinkList.begin(); itrUF2!=m_hLinkList.end(); itrUF2++) {
// 		 pUF2 = (UniFiber*)(*itrUF2);
//                  assert(pUniFiber);
// 		 pUF2->dump(cout);
//                 // cerr <<  pUF-> m_hLinkId << endl; 
//                 // cerr <<  pUF-> m_hCost << endl; 
// 		}
	// compute upto k primary candidates
	AbsPath* hPPath=new AbsPath();
	LINK_COST hCost=SPPBw(hPPath, pSrc,pDst, numberOfChannels,false);
	if (hCost == UNREACHABLE) {//cout<<"zzz ";
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>

	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost

	list<AbsPath*> hBestB;
	LINK_COST hCurrBCost;
	//LINK_COST hCurrPCost;
	list<AbsPath*>::iterator itrPPath;

	 
	  validateAllLinks();
	  validateAllNodes();
	       
/*#ifdef _OCHDEBUGA
//		{
//			hCurrPPath.dump(cout);
//		}
#endif*/
		// redefine link cost for computing link/node-disjoint
                //    backup according to various Time policy --M
      UINT nTimeInf = pNetMan->getTimePolicy(); // -M
                
      switch (nTimeInf) {
      case 0: //No Time 
		 {                    
		   SEG_SPP_UpdateLinkCost_Backup(hPPath->m_hLinkList,pNetMan->isLinkDisjointActive);     
           break;
                 }
       default:
      DEFAULT_SWITCH;
      };

		// compute the backup path
		AbsPath* hCurrBPath=new AbsPath();
set<Lightpath*> LPList;//lista contenente tutti i lightpath presenti nel link attraversato dal primario

	SEG_SPP_UpdateBackupCostWP(hPPath->m_hLinkList,LPList,pNetMan->isLinkDisjointActive);//Lista abstract link*) DEL PRIMARIO CONSIDERATO
		
		hCurrBCost =SPPBw(hCurrBPath, pSrc,pDst, numberOfChannels,true);

SEG_SPP_RestoreBackupCostWP(LPList);

		if (UNREACHABLE != hCurrBCost) {
			// compare cost
				assert(hCurrBPath->m_hLinkList.size()>0);
			hBestB.push_back(hCurrBPath);
			
		} // if
           else return UNREACHABLE;
		   //delete hLinkToBeD; //dynamic memory management for 
		   // needed, otherwise, original cost will be overwritten
		restoreLinkCost();	

	




	
		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);//LEAK!
	
		list<AbstractLink*>::const_iterator itr2;
		for (itr2=hPPath->m_hLinkList.begin(); itr2!=hPPath->m_hLinkList.end();itr2++) {
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr2));
		}
		pLPSeg->m_hCost = hPPath->getCost();
//


		pLPSeg->m_hBackupSegs = hBestB;

		assert( pLPSeg->m_hBackupSegs.size()>0 );

		hCircuit.addVirtualLink(pLPSeg);
	pLPSeg->wlAssigned=hPPath->wlAssigned;
//assert(hPPath->wlAssigned<=16);
	pLPSeg->wlAssignedBCK=hCurrBPath->wlAssigned;
	

	return hCost+hCurrBCost;
}



/*
LINK_COST WDMNetwork::myYen(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)

{
	
}*/

//****************** wpUNPROTECTED *******************
//-B: return the cost of the best path found connecting source and destination
//	and add it to the m_hRoute Lightpath list, attribute of a Circuit object
LINK_COST WDMNetwork::wpUNPROTECTED_ComputeRoute(Circuit& hCircuit,
											NetMan *pNetMan,
											OXCNode* pSrc, 
											OXCNode* pDst)
{
	AbsPath* pBestP = new AbsPath(); //leak!
	//comincia yen....
	//list<AbsPath*> listA;//percorsi definitivi
	//list<AbsPath*> listB;// percorsi parziali
	//listA.push_back(path);
	// numero k percorsi: ce l'ho in pNetMan

	//-B: use of 'myAlg' to compute paths -> how many? to be debugged or look into pNetMan object -> 1
	LINK_COST hBestCost = myAlg(*pBestP, pSrc, pDst, numberOfChannels); // -L: how myAlg works?

	//-B: I guess that, thanks to myAlg method, pBestP points to the best path
	//caso unprotected
	if (UNREACHABLE != hBestCost)
	{
		// construct the circuit	//-B: Lightpath_Seg is a class derived from LighPath, that is a class derived from AbstractLink
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0); //LEAK!
		list<AbstractLink*>::const_iterator itr;
		//-B: for each link of the pBestP path
		for (itr = pBestP->m_hLinkList.begin(); itr != pBestP->m_hLinkList.end(); itr++)
		{
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr)); //-B: m_hPRoute is a list of UniFiber object in the class Lightpath; itr is a AbstractLink object reference; Unifiber class (as well as Lightpath) is derived from AbstractLink
		}
		//-B: the lightpath will use the same wavel/channel assigned to pBestP (the assignment has been done randomly among the available wavel, by the randomChSelection method, belonging to the AbstractGraph class)
		pLPSeg->wlAssigned = pBestP->wlAssigned;
		pLPSeg->m_hCost = pBestP->getCost();

		/*assert(hBestB.size()>0);
		pLPSeg->m_hBackupSegs = hBestB;*/
		
		hCircuit.addVirtualLink(pLPSeg); //-B: add it to the m_hRoute Lightpath list, attribute of a Circuit object
	}
	//-B: case hBestCost == UNREACHABLE is not treated!!! See UNPROTECTED_ComputeRoute to know how to deal with it

	delete pBestP;
	
	// delete primary candidate
	/*for (pBestP=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}*/
	return hBestCost; //hBestCost;

}

//****************** wpSEG_SPP *******************
LINK_COST WDMNetwork::wpSEG_SPP_ComputeRoute(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)
{

//AbsPath* pBestP=new AbsPath();//leak!
int k=pNetMan->getNumberOfAltPaths();
// int lamda;////comincia yen....

//primo passo:calcolo almeno il primo percorso
//LINK_COST hBestCost=myAlg(*pBestP,pSrc,pDst,lamda);

list<AbsPath*> hPPathList;
YenWP(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
if (0 == hPPathList.size()) {//cout<<"zzz ";
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>
	AbsPath *pPPath = NULL;
	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;
	LINK_COST hCurrPCost, hCurrBCost;
	list<AbsPath*>::iterator itrPPath;
//int mod=0;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
	  AbsPath& hCurrPPath = **itrPPath;
	  validateAllLinks();// 14 marzo fabio:riaggiunti..
	  validateAllNodes();
		//FASE 2: imposto i costi per il backup
 set<Lightpath*> LPList;//lista contenente tutti i lightpath presenti nel link attraversato dal primario

	SEG_SPP_UpdateBackupCostWP((*itrPPath)->m_hLinkList,LPList,pNetMan->isLinkDisjointActive);//Lista abstract link*) DEL PRIMARIO CONSIDERATO
		//dbgpoint
		// compute the backup path
		AbsPath* hCurrBPath=new AbsPath();//possibile leak
		hCurrBCost = myAlgBCK(*hCurrBPath, pSrc, pDst, 
			numberOfChannels);//lamda è diventata inutile...



		if (UNREACHABLE != hCurrBCost) {
			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCost) < hBestCost) {
//				mod++;
				hBestCost = hCurrPCost + hCurrBCost;
				pBestP = *itrPPath;
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				
				hBestB.clear();
				hBestB.push_back(hCurrBPath); //verificare se resta anche all' uscita dalla funzione..
				/*
				AbsPath *pBackup = new AbsPath();//LEAK!
				pBackup->m_hLinkList = hCurrBPath;
				hBestB.push_back(pBackup);*/
			}
		} // if
                //delete hLinkToBeD; //dynamic memory management for 
		        // needed, otherwise, original cost will be overwritten
	//	restoreLinkCost();	
		SEG_SPP_RestoreBackupCostWP(LPList);
//	if (mod==2) cout<<"secondo!"<<endl;
	}//end for itrPPath  



	if (UNREACHABLE != hBestCost) {


		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);//LEAK!
	
		list<AbstractLink*>::const_iterator itr;
		for (itr=pBestP->m_hLinkList.begin(); itr!=pBestP->m_hLinkList.end();itr++) {
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		}
		pLPSeg->wlAssigned=pBestP->wlAssigned;
		pLPSeg->m_hCost = pBestP->getCost();
//
		

		list<AbsPath*>::iterator itr1=hBestB.begin();
		pLPSeg->wlAssignedBCK=(*itr1)->wlAssigned;
		pLPSeg->m_hBackupSegs = hBestB;
		hCircuit.addVirtualLink(pLPSeg);
	
	}
	delete pBestP;
	// delete primary candidate
/*	for (pBestP=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}*/
	return hBestCost; //hBestCost;

}

//****************** wpPAL_DPP *******************
LINK_COST WDMNetwork::wpPAL_DPP_ComputeRoute(Circuit& hCircuit,
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)

{

	//AbsPath* pBestP=new AbsPath();//leak!
int k = pNetMan->getNumberOfAltPaths();
// int lamda;////comincia yen....

//primo passo:calcolo almeno il primo percorso
//LINK_COST hBestCost=myAlg(*pBestP,pSrc,pDst,lamda);

list<AbsPath*> hPPathList;
YenWP(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
if (0 == hPPathList.size()) {//cout<<"zzz ";
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>
	AbsPath *pPPath = NULL;
	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;
	LINK_COST hCurrPCost, hCurrBCost;
	list<AbsPath*>::iterator itrPPath;

	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
	  AbsPath& hCurrPPath = **itrPPath;
//	  validateAllLinks();
//	  validateAllNodes();
		//FASE 2: imposto i costi per il backup
	  set<Lightpath*> LPList;//lista contenente tutti i lightpath presenti nel link attraversato dal primario

	DEDICATED_UpdateBackupCostWP((*itrPPath)->m_hLinkList,LPList,pNetMan->isLinkDisjointActive);//USO SOLO X ELIMINARE PROVVISORIAMENTE PRIMARIO
	//SEG_SPP_UpdateBackupCostWP((*itrPPath)->m_hLinkList,LPList,pNetMan->isLinkDisjointActive);//USO SOLO X ELIMINARE PROVVISORIAMENTE PRIMARIO		
		// compute the backup path
		AbsPath* hCurrBPath=new AbsPath();//possibile leak
		hCurrBCost = myAlg(*hCurrBPath, pSrc, pDst, 
			numberOfChannels);//lamda è diventata inutile...

		if (UNREACHABLE != hCurrBCost) {
			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCost) < hBestCost) {
				hBestCost = hCurrPCost + hCurrBCost;
				pBestP = *itrPPath;
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				
				hBestB.clear();
				hBestB.push_back(hCurrBPath); //verificare se resta anche all' uscita dalla funzione..
				/*
				AbsPath *pBackup = new AbsPath();//LEAK!
				pBackup->m_hLinkList = hCurrBPath;
				hBestB.push_back(pBackup);*/
			}
		} // if
                //delete hLinkToBeD; //dynamic memory management for 
		        // needed, otherwise, original cost will be overwritten
	//	restoreLinkCost();	
		
	}//end for itrPPath  

	if (UNREACHABLE != hBestCost) {


		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);//LEAK!
	
		list<AbstractLink*>::const_iterator itr;
		for (itr=pBestP->m_hLinkList.begin(); itr!=pBestP->m_hLinkList.end();itr++) {
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		}
		pLPSeg->wlAssigned=pBestP->wlAssigned;
		pLPSeg->m_hCost = pBestP->getCost();
//
		
		list<AbsPath*>::iterator itr1=hBestB.begin();
		pLPSeg->wlAssignedBCK=(*itr1)->wlAssigned;
		pLPSeg->m_hBackupSegs = hBestB;
		hCircuit.addVirtualLink(pLPSeg);
	//////////////////////////////7
			pLPSeg->primaryLength=pBestP->m_hLinkList.size();
			pLPSeg->backupLength=hBestB.front()->m_hLinkList.size();
	
		///////////////////////////////////
	}
	delete pBestP;
	// delete primary candidate
/*	for (pBestP=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}*/
	return hBestCost; //hBestCost;
}


//****************** SEG_SPP_B_HOP *******************
LINK_COST WDMNetwork::SEG_SPP_B_HOP_ComputeRoute(Circuit& hCircuit,
												 NetMan *pNetMan,
												 OXCNode *pSrc, 
												 OXCNode *pDst,
												 Connection *pCon)
{
	// obey fiber capacity
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

	// compute upto k primary candidates
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size()) {
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>
	AbsPath *pPPath = NULL;
	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;
	LINK_COST hCurrPCost, hCurrBCost;
	list<AbsPath*>::iterator itrPPath;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		AbsPath& hCurrPPath = **itrPPath;
		validateAllLinks();
		validateAllNodes();

		// redefine link cost for computing link/node-disjoint backup
		SEG_SPP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList,pNetMan->isLinkDisjointActive);

		// compute the backup path
		list<AbstractLink*> hCurrBPath;
		hCurrBCost = Dijkstra(hCurrBPath, pSrc, pDst, 
			AbstractGraph::LCF_ByOriginalLinkCost);

		if (UNREACHABLE != hCurrBCost) {
			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCost) < hBestCost) {
				hBestCost = hCurrPCost + hCurrBCost;
				pBestP = *itrPPath;
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				hBestB.clear();
				AbsPath *pBackup = new AbsPath();
				pBackup->m_hLinkList = hCurrBPath;
				hBestB.push_back(pBackup);
			}
		} // if

		// needed, otherwise, original cost will be overwritten
		restoreLinkCost();		
	}

	if (UNREACHABLE != hBestCost) {
		// UINT nBHopCount = pNetMan->getBHopCount();
		UINT nBHopCount = pCon->m_nHopCount;
		if (hBestB.front()->m_hLinkList.size() > nBHopCount) {
			hBestCost = UNREACHABLE;
			delete (hBestB.front());
		} else {
			assert(pBestP);
			// construct the circuit
			Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);
			list<AbstractLink*>::const_iterator itr;
			for (itr=pBestP->m_hLinkList.begin(); 
			itr!=pBestP->m_hLinkList.end(); itr++) {
				pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
			}
			pLPSeg->m_hCost = pBestP->getCost();
			pLPSeg->m_hBackupSegs = hBestB;
			hCircuit.addVirtualLink(pLPSeg);
		}
	}

	// delete primary candidate
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}

	return hBestCost;
}

//****************** SEG_SPP *******************
inline void WDMNetwork::SEG_SPP_UpdateBackupCostWP(const list<AbstractLink*>& hPPath,set<Lightpath*>& result,bool isLinkDisjoint)
{
	list<AbstractLink*>::const_iterator itr;
	if (isLinkDisjoint)
		{
		for (itr=hPPath.begin(); itr!=hPPath.end(); itr++) {
        //cout<<(*itr)->getDst()->getId()<<"_"<<endl;
		(*itr)->invalidate();//invalido i link
	

	}

			//PARTE 1: faccio il merge dei lightpath presenti
	
	
	for (itr=hPPath.begin();itr != hPPath.end();itr++)
	{
							//result:riferito in questo caso ai LP nei LINK
	// set_union(result.begin(),result.end(),(*itr)->LPinFiber.begin(),(*itr)->LPinFiber.end(),inserter(result,result.begin()));  //commentato perchè dava errore suVS10
	
	}//for hPPath

		//Ora ho in "result" tutti i puntatori ai lightpath presenti nei LINK attraversati dal primario
	
	////PARTE 2 : estraggo i link dei percorsi di backup ed aggiorno i costi (wlOccupationBCK)
	
	set<Lightpath*>::iterator itrLP;
	list<AbsPath*>::iterator itrAP;
	list<AbstractLink*>::iterator itrLINK;

	Lightpath_Seg* LPseg;
	for (itrLP=result.begin();itrLP != result.end();itrLP++)//scorro ogni LP
	{
		LPseg=(Lightpath_Seg*)(*itrLP);
		for (itrAP=LPseg->m_hBackupSegs.begin();itrAP!=LPseg->m_hBackupSegs.end();itrAP++)
		{

			for(itrLINK=(*itrAP)->m_hLinkList.begin();itrLINK!=(*itrAP)->m_hLinkList.end();itrLINK++)
			{//scorro ogni link del LP

			(*itrLINK)->wlOccupationBCK[(*itrLP)->wlAssignedBCK]=UNREACHABLE;
			}
		}
		
	}
		}
	else
{
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
		list<AbstractLink*>::const_iterator startPoint = hPPath.begin();
	itr2ndToLast--;
	
	// invalidate working path
	
if(hPPath.size()>1)
	{
	for (itr=hPPath.begin(); itr!=itr2ndToLast; itr++) {
        //cout<<(*itr)->getDst()->getId()<<"_"<<endl;
		(*itr)->getDst()->invalidate();//nodo-digiunzione
		
	}
	startPoint++;
	}
	else{
		itr=hPPath.begin();
(*itr)->invalidate();	}

	//PARTE 1: faccio il merge dei lightpath presenti
	
	
	for (itr=startPoint;itr != hPPath.end();itr++)
	{

	//	set_union(result.begin(),result.end(),(*itr)->LPinFiber.begin(),(*itr)->LPinFiber.end(),inserter(result,result.begin()));
	//	set_union(result.begin(),result.end(),(*itr)->getSrc()->LPinNode.begin(),(*itr)->getSrc()->LPinNode.end(),inserter(result,result.begin())); //commentato perchè dava errore suVS10

	
	}//for hPPath

	//Ora ho in "result" tutti i puntatori ai lightpath presenti nei NODI attraversati dal primario
	
	////PARTE 2 : estraggo i link dei percorsi di backup ed aggiorno i costi (wlOccupationBCK)
	
	set<Lightpath*>::iterator itrLP;
	list<AbsPath*>::iterator itrAP;
	list<AbstractLink*>::iterator itrLINK;

	Lightpath_Seg* LPseg;
	for (itrLP=result.begin();itrLP != result.end();itrLP++)
	{
		LPseg=(Lightpath_Seg*)(*itrLP);
		for (itrAP=LPseg->m_hBackupSegs.begin();itrAP!=LPseg->m_hBackupSegs.end();itrAP++)
		{

			for(itrLINK=(*itrAP)->m_hLinkList.begin();itrLINK!=(*itrAP)->m_hLinkList.end();itrLINK++)
			{

			(*itrLINK)->wlOccupationBCK[(*itrLP)->wlAssignedBCK]=UNREACHABLE;
			}
		}
		
	}
	}//END NODE DISJOINT

}

inline void WDMNetwork::SEG_SPP_RestoreBackupCostWP(set<Lightpath*>& result)
{

	
	set<Lightpath*>::iterator itrLP;
	list<AbsPath*>::iterator itrAP;
	list<AbstractLink*>::iterator itrLINK;

	Lightpath_Seg* LPseg;
	for (itrLP=result.begin();itrLP != result.end();itrLP++)
	{
		LPseg=(Lightpath_Seg*)(*itrLP);
		for (itrAP=LPseg->m_hBackupSegs.begin();itrAP!=LPseg->m_hBackupSegs.end();itrAP++)
		{

			for(itrLINK=(*itrAP)->m_hLinkList.begin();itrLINK!=(*itrAP)->m_hLinkList.end();itrLINK++)
			{

			(*itrLINK)->wlOccupationBCK[(*itrLP)->wlAssignedBCK]=m_dEpsilon;
			}
		}
		
	}

}

inline void 
WDMNetwork::SEG_SPP_UpdateLinkCost_Backup(const list<AbstractLink*>& hPPath,bool linkDis)
{
	if(linkDis)
		{
// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++) {
		(*itr)->invalidate();
	}

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
	     if (!(*itr)->valid())  {continue;}
	     UniFiber *pBFiber = (UniFiber*)(*itr);
	     assert(pBFiber && pBFiber->m_pCSet);
	     bool bShareable = true;
	     list<AbstractLink*>::const_iterator itrFiber;
	     for (itrFiber=hPPath.begin(); (itrFiber!=hPPath.end()) && bShareable; itrFiber++) {
//		  assert(pBFiber->m_nBChannels >= pBFiber->m_pCSet[(*itrFiber)->getId()]);
		  if (pBFiber->m_nBChannels == pBFiber->m_pCSet[(*itrFiber)->getId()])
					bShareable = false;
	     }
	     if (bShareable) {
		 pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
		 } else if (pBFiber->countFreeChannels() > 0) {
		   NULL;
		   // pBFiber->modifyCost(1);
		        } else {
			  pBFiber->invalidate();
			  }
        }
	 }
	else
		{
	list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
	itr2ndToLast--;
	// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=itr2ndToLast; itr++) {
        //cout<<(*itr)->getDst()->getId()<<"_"<<endl;
		(*itr)->getDst()->invalidate();//nodo-digiunzione
	}

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = true;
			list<AbstractLink*>::const_iterator itrFiber;
			for (itrFiber=hPPath.begin(); 
			(itrFiber!=itr2ndToLast) && bShareable; itrFiber++) {
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
				pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	} else {
		// case 2: primary traverses only one hop
		// invalidate primary path & its reverse path
		hPPath.front()->invalidate();
		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pU = hPPath.front()->m_pSrc;
		AbstractNode *pV = hPPath.front()->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pV->m_hOLinkList.begin();
		(itrO!=pV->m_hOLinkList.end()) && ((*itrO)->m_pDst!=pU); itrO++) {
			NULL;
		}
		// link at the opposite direction exists
		if (itrO != pV->m_hOLinkList.end()) {
			(*itrO)->invalidate();
		}

		UINT nSrc = hPPath.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
				pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	}
	}//END NODE DIS
}

inline void 
WDMNetwork::SEG_SPP_UpdateLinkCost_BackupInvalidate(const list<AbstractLink*>& hPPath)
{
	list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
	itr2ndToLast--;
	// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=itr2ndToLast; itr++) {
		(*itr)->getDst()->invalidate();
	}

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = true;
			list<AbstractLink*>::const_iterator itrFiber;
			for (itrFiber=hPPath.begin(); 
			(itrFiber!=itr2ndToLast) && bShareable; itrFiber++) {
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
			  //pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	} else {
		// case 2: primary traverses only one hop
		// invalidate primary path & its reverse path
		hPPath.front()->invalidate();
		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pU = hPPath.front()->m_pSrc;
		AbstractNode *pV = hPPath.front()->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pV->m_hOLinkList.begin();
		(itrO!=pV->m_hOLinkList.end()) && ((*itrO)->m_pDst!=pU); itrO++) {
			NULL;
		}
		// link at the opposite direction exists
		if (itrO != pV->m_hOLinkList.end()) {
			(*itrO)->invalidate();
		}

		UINT nSrc = hPPath.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
			  //pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	}
}

inline void 
WDMNetwork::SEG_SPP_UpdateLinkCost_BackupCI(const list<AbstractLink*>& hPPath,
                                                 SimulationTime hDuration, bool bSaveCI)
{
	list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
	itr2ndToLast--;
	// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=itr2ndToLast; itr++) {
		(*itr)->getDst()->invalidate();
	}
        //cerr << "1";
	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) { 
	  //cerr << "2";
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}  
                  
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = true;
			list<AbstractLink*>::const_iterator itrFiber;
			for (itrFiber=hPPath.begin(); 
			(itrFiber!=itr2ndToLast) && bShareable; itrFiber++) {
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			} 
			if (bShareable) {
                        	LINK_COST hCostOrig= pBFiber->getCost();
                            LINK_COST hTimeCost= hCostOrig * hDuration;     //-M
				            pBFiber->modifyCostCI(hTimeCost * m_dEpsilon,bSaveCI); //-M
			} else if (pBFiber->countFreeChannels() > 0) {
			
				            LINK_COST hCostOrig= pBFiber->getCost();        //-M
                            LINK_COST hTimeCost= hCostOrig * hDuration;    //-M 
				            pBFiber->modifyCostCI(hTimeCost, bSaveCI);
			} else {
				pBFiber->invalidate();
			}
		}
	} else { // cerr << "3";
		// case 2: primary traverses only one hop
		// invalidate primary path & its reverse path
		hPPath.front()->invalidate();
		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pU = hPPath.front()->m_pSrc;
		AbstractNode *pV = hPPath.front()->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pV->m_hOLinkList.begin();
		(itrO!=pV->m_hOLinkList.end()) && ((*itrO)->m_pDst!=pU); itrO++) {
			NULL;
		}
		// link at the opposite direction exists
		if (itrO != pV->m_hOLinkList.end()) {
			(*itrO)->invalidate();
		}

		UINT nSrc = hPPath.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
                        	LINK_COST hCostOrig= pBFiber->getCost();
                                LINK_COST hTimeCost= hCostOrig * hDuration;     //-M
			        pBFiber->modifyCostCI(hTimeCost * m_dEpsilon, bSaveCI); //-M
			} else if (pBFiber->countFreeChannels() > 0) {
				LINK_COST hCostOrig= pBFiber->getCost();        //-M
                                LINK_COST hTimeCost= hCostOrig * hDuration;    //-M 
			        pBFiber->modifyCostCI(hTimeCost, bSaveCI);
			} else {
				pBFiber->invalidate();
			}
		}
	}
}

inline void 
WDMNetwork::SEG_SPP_UpdateLinkCost_Backup_ServCI(const list<AbstractLink*>& hPPath, const list<AbstractLink*>& hLinkToBeD,
                                                 SimulationTime hDuration, bool bSaveCI)
{

  // invalidation of primary links only the first time I access this member function
  // to save computational time??
        list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();

	itr2ndToLast--;
	//   if (bSaveCI==0){
	// invalidate working path 
	// cerr << "1"; 
        list<AbstractLink*>::const_iterator itrzz;
	for (itrzz=hPPath.begin(); itrzz!=itr2ndToLast; itrzz++) {          
		(*itrzz)->getDst()->invalidate();
	}
	// } 

        //cerr << "pd3"; 

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
        int a= hPPath.size();
        //cerr << "size " << a << "--"<< endl;
 
	if (hPPath.size() > 1) {
	  //    cerr << "2";
                list<AbstractLink*>::const_iterator itrK;
		for (itrK=hLinkToBeD.begin(); itrK!=hLinkToBeD.end(); itrK++) {
			if ((!(*itrK)->valid()) || (!(*itrK)->getSrc()->valid()) ||
				(!(*itrK)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itrK);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = true;
			list<AbstractLink*>::const_iterator itrFiber;
			for (itrFiber=hPPath.begin(); 
			(itrFiber!=itr2ndToLast) && bShareable; itrFiber++) {
			  	assert(pBFiber->m_nBChannels >= 
			  	pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
			    // I have to multiply the hDuration always for original cost and then
                            // sum the various contributes 
                                LINK_COST hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M               
                                LINK_COST hTimeCost= hCostOrig * hDuration;     //-M
				  pBFiber->modifyCostCI(hTimeCost * m_dEpsilon,bSaveCI); //-M
			} 
			else if (pBFiber->countFreeChannels() > 0) 
			{		            LINK_COST hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M        
                                LINK_COST hTimeCost= hCostOrig * hDuration;    //-M 
				 pBFiber->modifyCostCI(hTimeCost, bSaveCI);
			} else {
				pBFiber->invalidate();
			}
		}
	} else {
          //      cerr << "3";
		// case 2: primary traverses only one hop
		// invalidate primary path & its reverse path
		hPPath.front()->invalidate();
		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pU = hPPath.front()->m_pSrc;
		AbstractNode *pV = hPPath.front()->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pV->m_hOLinkList.begin();
		(itrO!=pV->m_hOLinkList.end()) && ((*itrO)->m_pDst!=pU); itrO++) {
			NULL;
		}
		// link at the opposite direction exists
		if (itrO != pV->m_hOLinkList.end()) {
			(*itrO)->invalidate();
		}

		UINT nSrc = hPPath.front()->getSrc()->getId();
                     list<AbstractLink*>::const_iterator itr;
		for (itr=hLinkToBeD.begin(); itr!=hLinkToBeD.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itr);//MODIFICA DEI COSTI DEI LINK IN BASE AL TEMPO
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
			                    LINK_COST  hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M                             
                                LINK_COST hTimeCost= hCostOrig * hDuration;                //-M
			                    pBFiber->modifyCostCI(hTimeCost * m_dEpsilon, bSaveCI);    //-M
			} else if (pBFiber->countFreeChannels() > 0) {
			                    LINK_COST  hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M
                                LINK_COST hTimeCost= hCostOrig * hDuration;                //-M 
			                    pBFiber->modifyCostCI(hTimeCost, bSaveCI);                 //-M
			} else {
				pBFiber->invalidate();
			}
		}
	}
  
	//cerr << "pd4"; 


}

//****************** SEG_SP_B_HOP *******************
LINK_COST WDMNetwork::SEG_SP_B_HOP_ComputeRoute(Circuit& hCircuit,
												NetMan *pNetMan,
												OXCNode* pSrc, 
												OXCNode* pDst,
												Connection *pCon)
{
	// obey fiber capacity
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

	// compute upto k primary candidates
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size()) {
		validateAllLinks();
		return UNREACHABLE;
	}

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;

	list<AbsPath*>::iterator itrPPath;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {

#ifdef _OCHDEBUGA
	//	cout<<"- Working path"<<endl;
	//	(*itrPPath)->dump(cout);
#endif

		validateAllLinks();

		list<AbsPath*> hCurrB;
		LINK_COST hCurrPCost = (*itrPPath)->getCost();
		LINK_COST hCurrBCost = 
			SEG_SP_B_HOP_ComputeBackup(**itrPPath, hCurrB, pNetMan, pSrc, pDst, pCon);

		if (hBestCost > (hCurrPCost + hCurrBCost)) {
			hBestCost = hCurrPCost + hCurrBCost;
			pBestP = *itrPPath;
			list<AbsPath*>::iterator itr;
			for (itr=hBestB.begin(); itr!=hBestB.end(); itr++)
				delete (*itr);
			hBestB = hCurrB;
		} else {
			list<AbsPath*>::iterator itr;
			for (itr=hCurrB.begin(); itr!=hCurrB.end(); itr++)
				delete (*itr);
		}
	}

	if (UNREACHABLE != hBestCost) {
		assert(pBestP);
		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);
		list<AbstractLink*>::const_iterator itr;
		for (itr=pBestP->m_hLinkList.begin(); itr!=pBestP->m_hLinkList.end();
		itr++) {
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		}
		pLPSeg->m_hCost = pBestP->getCost();
		pLPSeg->m_hBackupSegs = hBestB;
		hCircuit.addVirtualLink(pLPSeg);
	}

	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}

	return hBestCost;
}

inline LINK_COST WDMNetwork::SEG_SP_B_HOP_ComputeBackup(const AbsPath& hPrimary,
									   list<AbsPath*>& hBSegs,
									   NetMan* pNetMan,
									   OXCNode* pSrc, 
									   OXCNode* pDst,
									   Connection *pCon)
{
	list<AbstractLink*> hPSeg;   
	list<AbstractNode*> hSrcList; //set {S}

#ifdef _OCHDEBUGA
//	cout<<"- Primary"<<endl;
//	hPrimary.dump(cout);
//	cout<<"- Before SEG_SP_B_HOP_ComputeBackup"<<endl;
//	this->dump(cout);
#endif

	hSrcList.push_back(pSrc);
	hPSeg = hPrimary.m_hLinkList;
	bool bSuccess = true;
UINT nTimeInf = pNetMan->getTimePolicy(); // -M
      
    while (bSuccess && (!hPSeg.empty())) {
		// redefine link cost for computing link/node-disjoint backup
	//	SEG_SP_B_HOP_UpdateLinkCost_Backup(hPSeg);
switch (nTimeInf) {      // -t
      case 0: //No Time 
		 {                   
		  SEG_SP_B_HOP_UpdateLinkCost_Backup(hPSeg);     
           break;
                 }
      case 1: // CI	
		 {
			 //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList);     
         
//-t
           SimulationTime hHTInc = pNetMan->m_hHoldingTimeIncoming;
           SimulationTime hATInc = pNetMan->m_hArrivalTimeIncoming;          
#ifdef DEBUGMASSIMO_
cout <<"                ArrTime " << hATInc   <<endl; 
cout <<"                HTINC " << hHTInc   <<endl; 
#endif
		   
		   list<AbstractLink*>  hLinkToBeD;
		   ExtractListUniFiber(hLinkToBeD); 
           bool bCopyCost=1;
           SimulationTime hTimeInterval;

           if (pNetMan->m_hDepaNet.empty()){
                 hTimeInterval= hHTInc; 
#ifdef DEBUGMASSIMO_
cout<<"                ZZ " <<  hTimeInterval <<endl; 
#endif
		         SEG_SP_B_HOP_UpdateLinkCost_Backup(hPSeg); 

                 bCopyCost=0;
           } 
           else 
          {
                     Event *pEvent;
                     Event *pFollowingEvent;
                     list<Event*>::const_iterator itrE;
                     SimulationTime  hCurrT =  hATInc; 
                   
                     for (itrE=pNetMan->m_hDepaNet.begin(); itrE!=pNetMan->m_hDepaNet.end() ; itrE++) {
			            pEvent = (Event*)(*itrE);
                        assert(pEvent);
                        SimulationTime InitialTime=hCurrT;
                        SimulationTime hCurrT= pEvent->m_hTime;
//SimulationTime hCurrT_T= pEvent->m_hTime;
                        double HTFraction=1;  
                        if ( hHTInc/HTFraction + hATInc > hCurrT ){

						// if ( hHTInc + hATInc > hCurrT ){
                           if (itrE==(pNetMan->m_hDepaNet.begin())) {
                              SimulationTime hFirstEventT= pEvent->m_hTime;
                              if (hHTInc + hATInc <= hFirstEventT){
                                  hTimeInterval= (hHTInc); 
cout<<"                YY "<<endl;
#ifdef DEBUGMASSIMO_
cout<<"                YY " << hTimeInterval <<endl;
     //cerr <<  pEvent->m_pConnection->m_nSequenceNo;
#endif
				                  SEG_SP_B_HOP_UpdateLinkCost_Backup(hPSeg);
                                  bCopyCost=0;
				                  break;

			                   }
							  else{
		      	                  hTimeInterval= (hFirstEventT - hATInc);
#ifdef DEBUGMASSIMO_ 

cout<<"                First " << hTimeInterval <<"       Depa_size_"<<pNetMan->m_hDepaNet.size()<<endl;
#endif
                                  bool bSave=0;
				                  SEG_SP_B_HOP_UpdateLinkCost_Backup_ServCI(hPSeg, hLinkToBeD, hTimeInterval,bSave);
                                  Lightpath *pLS; 
                                  list<Lightpath*> ListLightpathWork =  pEvent->m_pConnection->m_pPCircuit->m_hRoute;//P or B
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                        pLS = (Lightpath*)(*itrLS);
                                        assert(pLS);
                                    } //end for
                              }//end else hHTInc
                          } //end if itrE
//if ( hHTInc + hATInc > hCurrT ){                         
                          list<Event*>::const_iterator itrE2;
                          itrE2 = itrE;
                          itrE2++;
                          // if I don`t have a next event -> incoming ht
			              if (itrE2!=(pNetMan->m_hDepaNet.end())) {
			                  pFollowingEvent= (Event*)(*itrE2);
                              assert(pFollowingEvent);
			                  SimulationTime hNextDepT = pFollowingEvent->m_hTime;
			                  if (hHTInc + hATInc <= hNextDepT){
                                  hTimeInterval= (hHTInc + hATInc - hCurrT); 
#ifdef DEBUGMASSIMO_
cout<<"                terminal " << hTimeInterval <<endl;
#endif
                                  bool bSave=1; 
			                        SEG_SP_B_HOP_UpdateLinkCost_Backup_ServCI(hPSeg, hLinkToBeD, hTimeInterval,bSave);
                                  break; 
			                  }
			                  else {
			                      hTimeInterval=(hNextDepT -hCurrT); 
#ifdef DEBUGMASSIMO_
cout<<"                middle " << hTimeInterval <<endl;
#endif
                                  bool bSave=1;
                                   SEG_SP_B_HOP_UpdateLinkCost_Backup_ServCI(hPSeg, hLinkToBeD, hTimeInterval,bSave);
			                       Lightpath *pLS;
                              //Prima della correzione 
                                  list<Lightpath*> ListLightpathWork =  pFollowingEvent->m_pConnection->m_pPCircuit->m_hRoute;
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                    pLS = (Lightpath*)(*itrLS);
                                    assert(pLS);
                                    pLS->releaseOnServCopy(hLinkToBeD);
                                   }
			                       continue;
                                 } // end else 
			                 } // end if itrE2

			              else{ 
                              hTimeInterval= hHTInc + hATInc- hCurrT;
#ifdef DEBUGMASSIMO_
cout<<"                final " << hTimeInterval <<endl;
#endif
                              bool bSave=1;
                              SEG_SP_B_HOP_UpdateLinkCost_Backup_ServCI(hPSeg, hLinkToBeD, hTimeInterval,bSave); 
			                  break; 
			                  }
                       
} //end if hHTInc/HTFraction + hATInc...  limitinf time of observation  
	                    }  //end for
		  } //end if Depanet Empty (not)
                 
		   // For the cases  in which  SEG_SP_NO_HOP_UpdateLinkCost_Backup has not been used
		   if (!bCopyCost==0) {
		                 //Needed just to invalidate link
                 SEG_SP_B_HOP_UpdateLinkCost_BackupInvalidate(hPSeg);                    
	     	             //Map the new link costs assigned to hLinkToBeD in the m_LinkList  
                 UniFiber *pUFLTBD, *pUFm_h;
                 list<AbstractLink*>::const_iterator itrUFLTBD ,itrUFm_h ;
                 for (itrUFLTBD=hLinkToBeD.begin(); itrUFLTBD!=hLinkToBeD.end(); itrUFLTBD++) {
		                     pUFLTBD = (UniFiber*)(*itrUFLTBD);
                             assert(pUFLTBD);
	                         for (itrUFm_h=m_hLinkList.begin(); itrUFm_h!=m_hLinkList.end(); itrUFm_h++) {
		                          pUFm_h = (UniFiber*)(*itrUFm_h);
                                  assert(pUFm_h);
                                  if ((pUFLTBD -> m_nLinkId) == (pUFm_h -> m_nLinkId))
			                      {									  
									  LINK_COST hCostCI = pUFLTBD->getCost();
                                      pUFm_h->modifyCost(hCostCI);
                                    } // end if pUFLTBD
	                              } // end for itrUFm
                                 } //end for itrUFLTBD
		   } // end if bCopyCost 
		 
           list<AbstractLink*>::const_iterator itrLinkCI2;


           for (itrLinkCI2=hLinkToBeD.begin(); itrLinkCI2!=hLinkToBeD.end();itrLinkCI2++) {
 	              UniFiber *pUniFiberZ = (UniFiber*)(*itrLinkCI2);
                  UINT nW=pUniFiberZ->m_nW;
                  if (pUniFiberZ->m_pChannel) {          
			           delete []pUniFiberZ->m_pChannel;
                       pUniFiberZ->m_pChannel=NULL;
			          }
	        	  if (pUniFiberZ->m_pCSet) {
			           delete []pUniFiberZ->m_pCSet; 
                       pUniFiberZ->m_pCSet=NULL;
             	      } 
			      delete pUniFiberZ;
                        // hLinkToBeD.pop_back();
		          }  		   
	       break;                 
		 }

      case 2:
	  default:
      DEFAULT_SWITCH;
      };


		// compute the backup segment
		AbsPath* pBSeg = new AbsPath;
		LINK_COST hBSegCost;
		hBSegCost = SEG_SP_B_HOP_ComputeBSeg(pBSeg->m_hLinkList, 
				hSrcList, hPSeg, pCon);
		if (UNREACHABLE != hBSegCost) {
			hBSegs.push_back(pBSeg);
		} else {
			delete pBSeg;
			bSuccess = false;
		}

		restoreLinkCost();
		// next segment
		if (!bSuccess) 
			break;
		AbstractNode *pInterNode = pBSeg->m_hLinkList.back()->m_pDst;
		if (pInterNode == pDst)
			break;	// done

		// reserve wavelengths along backup segment for better sharing
		SEG_SP_HOP_Reserve_BSeg(hPSeg, pBSeg->m_hLinkList);

		hSrcList.clear();
		list<AbstractLink*>::const_iterator itr = hPSeg.begin();
		while ((itr!=hPSeg.end()) && ((*itr)->m_pDst != pInterNode)) {
			hSrcList.push_back((*itr)->m_pDst);
			itr++;
		}
		hPSeg.clear();
		itr = hPrimary.m_hLinkList.begin();
		while ((*itr)->m_pDst != pInterNode) {
			itr++;
		}
		hPSeg.insert(hPSeg.end(), itr, hPrimary.m_hLinkList.end());
		if (hSrcList.empty()) {
			bSuccess = false;
		}
	}

#ifdef _OCHDEBUGA
//	cout<<endl<<"- Middle"<<endl;
//	this->dump(cout);
#endif

	LINK_COST hBCost = UNREACHABLE;
	if (bSuccess) {
		list<AbsPath*> hDupBSegs;
		list<AbsPath*>::iterator itr=hBSegs.end();
		itr--;
		hDupBSegs.insert(hDupBSegs.end(), hBSegs.begin(), itr);
		SEG_SP_HOP_Release_BSegs(hPrimary.m_hLinkList, hDupBSegs);

		hBCost = SEG_SP_NO_HOP_Calculate_BSegs_Cost(hPrimary, hBSegs);
	} else {
		SEG_SP_HOP_Release_BSegs(hPrimary.m_hLinkList, hBSegs);
		list<AbsPath*>::iterator itr;
		for (itr=hBSegs.begin(); itr!=hBSegs.end(); itr++)
			delete (*itr);
		hBSegs.clear();
	}

#ifdef _OCHDEBUGA
//	cout<<endl<<"- After"<<endl;
//	this->dump(cout);
#endif

	return hBCost;
}

inline LINK_COST WDMNetwork::SEG_SP_B_HOP_ComputeBSeg(list<AbstractLink*>& hBSeg, 
									 const list<AbstractNode*>& hSrcList,
									 const list<AbstractLink*>& hPSeg,
									 Connection *pCon)
{
	// Dijkstra's algorith with multiple sources
	reset4ShortestPathComputation();
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode=hSrcList.begin(); itrNode!=hSrcList.end(); itrNode++) {
		(*itrNode)->m_hCost = 0;
		(*itrNode)->m_nHops = 0;
	}

//#ifdef _OCHDEBUGA
//	this->dump(cout);
//#endif

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ; //set di nodi V'
	for (itrNode=m_hNodeList.begin(); itrNode!=m_hNodeList.end(); itrNode++) {
		if ((*itrNode)->valid())
			hPQ.insert(*itrNode);
	}
	list<AbstractLink*>::const_iterator itr;
	
	set<UINT> hPNodes;
	for (itr=hPSeg.begin(); itr!=hPSeg.end(); itr++) {
		hPNodes.insert((*itr)->m_pDst->getId());
	}

	while (!hPQ.empty()) {//finche' V' non e' vuoto
		AbstractNode *pLinkSrc = hPQ.peekMin();//scelta di {u} con PC(u) minimo
		hPQ.popMin();// V'=V'-{u}
		if (UNREACHABLE == pLinkSrc->m_hCost)
			break;
		if (hPNodes.end() != hPNodes.find(pLinkSrc->getId()))
			continue;	// ensure node-disjointness

		for (itr=pLinkSrc->m_hOLinkList.begin(); 
		itr!=pLinkSrc->m_hOLinkList.end(); itr++) 
		{
			// the link has been invalidated
			if ((!(*itr)->valid()) || (UNREACHABLE == (*itr)->getCost()))
				continue;	

			LINK_COST hLinkCost = (*itr)->getCost();
			if ((*itr)->m_pDst->m_hCost > pLinkSrc->m_hCost + hLinkCost) {//  ---t
				(*itr)->m_pDst->m_hCost = pLinkSrc->m_hCost + hLinkCost;//    PC(v)<--PC(u)+C(u,v)
				(*itr)->m_pDst->m_nHops = pLinkSrc->m_nHops + 1;//            HC(v)<--HC(u)+1
				// use previous link instead of node
				(*itr)->m_pDst->m_pPrevLink = *itr;	 //                       PH(v)<--u
			}
		} // for
		hPQ.buildHeap();
	} // while

//#ifdef _OCHDEBUGA
//	this->dump(cout);
//#endif

	UINT nBHopCount = pCon->m_nHopCount;
	AbstractNode *pInterNode = NULL;
	list<AbstractLink*>::const_reverse_iterator ritr;
	for (ritr=hPSeg.rbegin(); ritr!=hPSeg.rend(); ritr++) {//partendo da dest a ritroso sul working
		if ((*ritr)->m_pDst->m_nHops <= nBHopCount) { // se HC(v) < Hb
			pInterNode = (*ritr)->m_pDst;
			break;
		}
	}
	if (NULL == pInterNode)
		return UNREACHABLE;
	assert(UNREACHABLE != pInterNode->m_hCost);
	return recordMinCostPath(hBSeg, pInterNode);
}

inline void WDMNetwork::SEG_SP_B_HOP_UpdateLinkCost_Backup(const list<AbstractLink*>& hPPath)
{
	// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++)
		(*itr)->invalidate();

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
		itr2ndToLast--;
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if (!(*itr)->valid())
				continue;
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = true;
			list<AbstractLink*>::const_iterator itrFiber;
			for (itrFiber=hPPath.begin(); 
			(itrFiber!=itr2ndToLast) && bShareable; itrFiber++) {
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
				pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	} else {
	// case 2: primary traverses only one hop
		UINT nSrc = hPPath.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if (!(*itr)->valid())
				continue;
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
				pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	}
}

inline void WDMNetwork::SEG_SP_B_HOP_UpdateLinkCost_BackupInvalidate(const list<AbstractLink*>& hPPath){

	// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++)
		(*itr)->invalidate();

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
		itr2ndToLast--;
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if (!(*itr)->valid()) continue;
		   UniFiber *pBFiber = (UniFiber*)(*itr);
		   assert(pBFiber && pBFiber->m_pCSet);
		   bool bShareable = true;
		   list<AbstractLink*>::const_iterator itrFiber;
		   for (itrFiber=hPPath.begin();(itrFiber!=itr2ndToLast) && bShareable; itrFiber++)
			{
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
				//pBFiber->modifyCost(SMALL_COST);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	} else {
	// case 2: primary traverses only one hop

		UINT nSrc = hPPath.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if (!(*itr)->valid())
				continue;
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
				//pBFiber->modifyCost(SMALL_COST);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {
				pBFiber->invalidate();
			}
		}
	}
}




inline void WDMNetwork::SEG_SP_B_HOP_UpdateLinkCost_Backup_ServCI(const list<AbstractLink*>& hPPath, const list<AbstractLink*>& hLinkToBeD,
                                                 SimulationTime hDuration, bool bSaveCI)
{// invalidate working path
	list<AbstractLink*>::const_iterator itr;
	for (itr=hPPath.begin(); itr!=hPPath.end(); itr++)
		(*itr)->invalidate();

	// define link cost for computing backup
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath.end();
		itr2ndToLast--;
		list<AbstractLink*>::const_iterator itrK;
		for (itrK=hLinkToBeD.begin(); itrK!=hLinkToBeD.end(); itrK++) {
			if (!(*itrK)->valid()) continue;
		   UniFiber *pBFiber = (UniFiber*)(*itrK);
		   assert(pBFiber && pBFiber->m_pCSet);
		   bool bShareable = true;
		   list<AbstractLink*>::const_iterator itrFiber;
		   for (itrFiber=hPPath.begin();(itrFiber!=itr2ndToLast) && bShareable; itrFiber++)
			{
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
			// I have to multiply the hDuration always for original cost and then
                            // sum the various contributes 
                                LINK_COST hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M               
                                LINK_COST hTimeCost= hCostOrig *  hDuration ;               //-M
				                pBFiber->modifyCostCI(hTimeCost * m_dEpsilon,bSaveCI);    //-M 
			} else if (pBFiber->countFreeChannels() > 0) {
				                LINK_COST hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M        
                                LINK_COST hTimeCost= hCostOrig * hDuration;               //-M 
				 pBFiber->modifyCostCI(hTimeCost, bSaveCI);
			} else {
				pBFiber->invalidate();
			}
		}
	} 
	else {
	// case 2: primary traverses only one hop




		UINT nSrc = hPPath.front()->getSrc()->getId();
		list<AbstractLink*>::const_iterator itrK;
		for (itrK=hLinkToBeD.begin(); itrK!=hLinkToBeD.end(); itrK++) {
			if (!(*itrK)->valid())
				continue;
			UniFiber *pBFiber = (UniFiber*)(*itrK);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
			                    LINK_COST  hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M                             
                                LINK_COST hTimeCost= hCostOrig * hDuration;                //-M
			                    pBFiber->modifyCostCI(hTimeCost * m_dEpsilon,bSaveCI); //-M 
			} else if (pBFiber->countFreeChannels() > 0) {
				                LINK_COST  hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M
                                LINK_COST hTimeCost= hCostOrig *  hDuration ;                //-M 
			                    pBFiber->modifyCostCI(hTimeCost, bSaveCI);                 //-M
			} else {
				pBFiber->invalidate();
			}
		}
	}
												 
}

//****************** SEG_SP_PB_HOP *******************
LINK_COST WDMNetwork::SEG_SP_PB_HOP_ComputeRoute(Circuit& hCircuit,
												NetMan *pNetMan,
												OXCNode* pSrc, 
												OXCNode* pDst,
												Connection *pCon)
{
	// obey fiber capacity
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

	// compute upto k primary candidates
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size()) {
		validateAllLinks();
		return UNREACHABLE;
	}

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;

	list<AbsPath*>::iterator itrPPath;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		validateAllLinks();

		list<AbsPath*> hCurrB;
		LINK_COST hCurrPCost = (*itrPPath)->getCost();
		LINK_COST hCurrBCost = 
			SEG_SP_PB_HOP_ComputeBackup(**itrPPath, hCurrB, pSrc, pDst, pCon);

		if (hBestCost > (hCurrPCost + hCurrBCost)) {
			hBestCost = hCurrPCost + hCurrBCost;
			pBestP = *itrPPath;
			list<AbsPath*>::iterator itr;
			for (itr=hBestB.begin(); itr!=hBestB.end(); itr++)
				delete (*itr);
			hBestB = hCurrB;
		} else {
			list<AbsPath*>::iterator itr;
			for (itr=hCurrB.begin(); itr!=hCurrB.end(); itr++)
				delete (*itr);
		}
	}

//----t
/*#ifdef _Compute_Route
{cout<<"*pBestP*"<<endl;
pBestP->dump(cout);
cout<<"*hBestB*"<<endl;
list<AbsPath*>::const_iterator itrdd;
int cx = 0;
for (itrdd=hBestB.begin(); itrdd!=hBestB.end(); itrdd++) {
cout<<"seg_nr_"<<cx<<": ";
(*itrdd)->dump(cout);
cx++;
//cout.seekp(5,ios_base::beg);
}
}
#endif*/

	if (UNREACHABLE != hBestCost) {
		assert(pBestP);
		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);
		list<AbstractLink*>::const_iterator itr;
		for (itr=pBestP->m_hLinkList.begin(); itr!=pBestP->m_hLinkList.end();
		itr++) {
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		}
		pLPSeg->m_hCost = pBestP->getCost();
		pLPSeg->m_hBackupSegs = hBestB;
		hCircuit.addVirtualLink(pLPSeg);
	}

	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}

	return hBestCost;
}

inline LINK_COST WDMNetwork::SEG_SP_PB_HOP_ComputeBackup(const AbsPath& hPrimary,
									   list<AbsPath*>& hBSegs,
									   OXCNode* pSrc, 
									   OXCNode* pDst,
									   Connection *pCon)
{
	list<AbstractLink*> hPSeg;
	list<AbstractNode*> hSrcList;

	hSrcList.push_back(pSrc);
	hPSeg = hPrimary.m_hLinkList;
	bool bSuccess = true;
	while (bSuccess && (!hPSeg.empty())) {
		// redefine link cost for computing link/node-disjoint backup
		SEG_SP_B_HOP_UpdateLinkCost_Backup(hPSeg);

		// compute the backup segment
		AbsPath* pBSeg = new AbsPath;
		LINK_COST hBSegCost;
		hBSegCost = SEG_SP_PB_HOP_ComputeBSeg(pBSeg->m_hLinkList, 
				hSrcList, hPSeg, hPrimary.m_hLinkList, pCon);
		if (UNREACHABLE != hBSegCost) {
			hBSegs.push_back(pBSeg);
		} else {
			delete pBSeg;
			bSuccess = false;
		}

		restoreLinkCost();
		// next segment
		if (!bSuccess) 
			break;
		AbstractNode *pInterNode = pBSeg->m_hLinkList.back()->m_pDst;
		if (pInterNode == pDst)
			break;	// done

		// reserve wavelengths along backup segment for better sharing
		SEG_SP_HOP_Reserve_BSeg(hPSeg, pBSeg->m_hLinkList);
		
		hSrcList.clear();
		list<AbstractLink*>::const_iterator itr = hPSeg.begin();
		while ((itr!=hPSeg.end()) && ((*itr)->m_pDst != pInterNode)) {
			hSrcList.push_back((*itr)->m_pDst);
			itr++;
		}
		hPSeg.clear();
		itr = hPrimary.m_hLinkList.begin();
		while ((*itr)->m_pDst != pInterNode) {
			itr++;
		}
		hPSeg.insert(hPSeg.end(), itr, hPrimary.m_hLinkList.end());
		if (hSrcList.empty()) {
			bSuccess = false;
		}
	}

	LINK_COST hBCost = UNREACHABLE;
	if (bSuccess) {
		list<AbsPath*> hDupBSegs;
		list<AbsPath*>::iterator itr=hBSegs.end();
		itr--;
		hDupBSegs.insert(hDupBSegs.end(), hBSegs.begin(), itr);
		SEG_SP_HOP_Release_BSegs(hPrimary.m_hLinkList, hDupBSegs);

		hBCost = SEG_SP_NO_HOP_Calculate_BSegs_Cost(hPrimary, hBSegs);
	} else {
		SEG_SP_HOP_Release_BSegs(hPrimary.m_hLinkList, hBSegs);
		list<AbsPath*>::iterator itr;
		for (itr=hBSegs.begin(); itr!=hBSegs.end(); itr++)
			delete (*itr);
		hBSegs.clear();
	}

	return hBCost;
}

inline LINK_COST WDMNetwork::SEG_SP_PB_HOP_ComputeBSeg(list<AbstractLink*>& hBSeg, 
									 const list<AbstractNode*>& hSrcList,
									 const list<AbstractLink*>& hPSeg,
									 const list<AbstractLink*>& hPrimary,
									 Connection *pCon)
{
	// Dijkstra's algorith with multiple sources
	reset4ShortestPathComputation();
	list<AbstractNode*>::const_iterator itrNode;
//hSrcList set di nodi S
	for (itrNode=hSrcList.begin(); itrNode!=hSrcList.end(); itrNode++) {
		(*itrNode)->m_hCost = 0;
		(*itrNode)->m_nHops = 0;
	}

	BinaryHeap<AbstractNode*, vector<AbstractNode*>, PAbstractNodeComp> hPQ;//set di nodi V'
	for (itrNode=m_hNodeList.begin(); itrNode!=m_hNodeList.end(); itrNode++) {
		if ((*itrNode)->valid())
			hPQ.insert(*itrNode);
	}
	list<AbstractLink*>::const_iterator itr;
	
	set<UINT> hPNodes;
	for (itr=hPSeg.begin(); itr!=hPSeg.end(); itr++) {
		hPNodes.insert((*itr)->m_pDst->getId());
	}

	while (!hPQ.empty()) {
		AbstractNode *pLinkSrc = hPQ.peekMin();
		hPQ.popMin();
		if (UNREACHABLE == pLinkSrc->m_hCost)
			break;
		if (hPNodes.end() != hPNodes.find(pLinkSrc->getId()))
			continue;	// ensure node-disjointness

		for (itr=pLinkSrc->m_hOLinkList.begin(); 
		itr!=pLinkSrc->m_hOLinkList.end(); itr++) 
		{
			// the link has been invalidated
			if ((!(*itr)->valid()) || (UNREACHABLE == (*itr)->getCost()))
				continue;	

			LINK_COST hLinkCost = (*itr)->getCost();
			if ((*itr)->m_pDst->m_hCost > pLinkSrc->m_hCost + hLinkCost) {
				(*itr)->m_pDst->m_hCost = pLinkSrc->m_hCost + hLinkCost;
				(*itr)->m_pDst->m_nHops = pLinkSrc->m_nHops + 1;
				// use previous link instead of node
				(*itr)->m_pDst->m_pPrevLink = *itr;	
			}
		} // for
		hPQ.buildHeap();
	} // while

	// hop count constraint on (p + b)

// Original buggy code
//	UINT nHopCount = pCon->m_nHopCount;
//	UINT nPHops = hPSeg.size();
//	AbstractNode *pInterNode = NULL;
//	list<AbstractLink*>::const_reverse_iterator ritr;
//	for (ritr=hPSeg.rbegin(); ritr!=hPSeg.rend(); ritr++) {
//		if (((*ritr)->m_pDst->m_nHops + nPHops) <= nHopCount) {
//			pInterNode = (*ritr)->m_pDst;
//			break;
//		}
//		assert(nPHops > 0);
//		nPHops--;
//	}
//	if (NULL == pInterNode)
//		return UNREACHABLE;
//	assert(UNREACHABLE != pInterNode->m_hCost);
//	return recordMinCostPath(hBSeg, pInterNode);


// New code
	LINK_COST hBSegCost = UNREACHABLE;
	UINT nHopCount = pCon->m_nHopCount;
	AbstractNode *pSegDst = NULL;
	list<AbstractLink*>::const_reverse_iterator ritr;

	for (ritr=hPSeg.rbegin(); ritr!=hPSeg.rend(); ritr++) {
		pSegDst = (*ritr)->m_pDst;
		if (pSegDst->m_nHops >= nHopCount) {
			continue;
		} else {
			AbstractNode *pSegSrc;
			list<AbstractLink*> hSeg;

			assert(UNREACHABLE != pSegDst->m_hCost);
			hBSegCost = recordMinCostPath(hSeg, pSegDst);
			pSegSrc = hSeg.front()->m_pSrc;

			list<AbstractLink*>::const_iterator itr = hPrimary.begin();
			while ((itr != hPrimary.end()) && ((*itr)->m_pSrc != pSegSrc)) {
				itr++;
			}
			assert(itr != hPrimary.end());
			UINT nPSegHops = 1;
			while ((itr != hPrimary.end()) && ((*itr)->m_pDst != pSegDst)) {
				itr++;
				nPSegHops++;
			}

			if ((nPSegHops + pSegDst->m_nHops) <= nHopCount) {
				hBSeg = hSeg;
				break;
			} else {
				hBSegCost = UNREACHABLE;
			}
		}
	}



	return hBSegCost;
}

LINK_COST WDMNetwork::SEG_SPP_PB_HOP_ComputeRoute(Circuit& hCircuit,
												  NetMan *pNetMan,
												  OXCNode* pSrc, 
												  OXCNode* pDst,
												  Connection *pCon)
{
	// obey fiber capacity
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

	// compute upto k primary candidates
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size()) {
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>
	AbsPath *pPPath = NULL;
	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;
	LINK_COST hCurrPCost, hCurrBCost;
	list<AbsPath*>::iterator itrPPath;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		AbsPath& hCurrPPath = **itrPPath;
		validateAllLinks();
		validateAllNodes();

		// redefine link cost for computing link/node-disjoint backup
		SEG_SPP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList,pNetMan->isLinkDisjointActive);

		// compute the backup path
		list<AbstractLink*> hCurrBPath;
		hCurrBCost = Dijkstra(hCurrBPath, pSrc, pDst, 
			AbstractGraph::LCF_ByOriginalLinkCost);

		if (UNREACHABLE != hCurrBCost) {
			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCost) < hBestCost) {
				hBestCost = hCurrPCost + hCurrBCost;
				pBestP = *itrPPath;
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				hBestB.clear();
				AbsPath *pBackup = new AbsPath();
				pBackup->m_hLinkList = hCurrBPath;
				hBestB.push_back(pBackup);
			}
		} // if

		// needed, otherwise, original cost will be overwritten
		restoreLinkCost();		
	}

	if (UNREACHABLE != hBestCost) {
		// UINT nHopCount = pNetMan->getBHopCount();
		UINT nHopCount = pCon->m_nHopCount;
		UINT nPHops = pBestP->m_hLinkList.size();
		UINT nBHops = hBestB.front()->m_hLinkList.size();
		if ( (nPHops + nBHops) > nHopCount) {
			hBestCost = UNREACHABLE;
			delete (hBestB.front());
		} else {
			assert(pBestP);
			// construct the circuit
			Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);
			list<AbstractLink*>::const_iterator itr;
			for (itr=pBestP->m_hLinkList.begin(); 
			itr!=pBestP->m_hLinkList.end(); itr++) {
				pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
			}
			pLPSeg->m_hCost = pBestP->getCost();
			pLPSeg->m_hBackupSegs = hBestB;
			hCircuit.addVirtualLink(pLPSeg);
		}
	}

	// delete primary candidate
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}

	return hBestCost;
}

//****************** SEG_SP_HOP *******************
// Reserve wavelength along one backup segment
void WDMNetwork::SEG_SP_HOP_Reserve_BSeg(const list<AbstractLink*>& hPSeg, 
										 const list<AbstractLink*>& hBSeg)
{
	list<AbstractLink*>::const_iterator itrB;
	list<AbstractLink*>::const_iterator itrP;
	AbstractNode *pSegDst = hBSeg.back()->m_pDst;
	UINT nSegEndNodeId = pSegDst->getId();
	for (itrB=hBSeg.begin(); itrB!=hBSeg.end();	itrB++) {
		UniFiber *pBFiber = (UniFiber*)(*itrB);
		assert(pBFiber && pBFiber->m_pCSet);
		UINT nNewBChannels = pBFiber->m_nBChannels;
		itrP = hPSeg.begin();
		for (++itrP; ((itrP!=hPSeg.end()) && ((*itrP)->getSrc()!=pSegDst));
		itrP++) {
			UINT nPNodeId = (*itrP)->getSrc()->getId();
			pBFiber->m_pCSet[nPNodeId]++;
			if (pBFiber->m_pCSet[nPNodeId] > nNewBChannels)
				nNewBChannels++;
			assert(nNewBChannels >= pBFiber->m_pCSet[nPNodeId]);
		}
		if (nNewBChannels > pBFiber->m_nBChannels) {
			pBFiber->m_nBChannels++;
			pBFiber->consumeChannel(NULL, -1);
			assert(pBFiber->m_nBChannels == nNewBChannels);
		}
	} // for itrB
}

void WDMNetwork::SEG_SP_HOP_Release_BSegs(const list<AbstractLink*>& hPrimary,
										  const list<AbsPath*>& hBSegs)
{
	list<AbsPath*>::const_iterator itr;
	list<AbstractLink*>::const_iterator itrP;
	list<AbstractLink*>::const_iterator itrB;
	list<AbstractLink*>::const_iterator itrPIndex = hPrimary.begin();
	itrPIndex++;	// only consider intermediate node failures
	for (itr=hBSegs.begin(); itr!=hBSegs.end(); itr++) {
		UINT nSegEndNodeId = (*itr)->m_hLinkList.back()->getDst()->getId();
		for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
		itrB++) {
			UniFiber *pBFiber = (UniFiber*)(*itrB);
			assert(pBFiber && pBFiber->m_pCSet);
			for (itrP=itrPIndex; ((itrP!=hPrimary.end()) 
				&& ((*itrP)->getSrc()->getId()!=nSegEndNodeId)); itrP++) {
				UINT nPNodeId = (*itrP)->getSrc()->getId();
				assert(pBFiber->m_pCSet[nPNodeId] > 0);
				pBFiber->m_pCSet[nPNodeId]--;
			}
			UINT nNodeId;
			UINT nNewBChannels = 0; 
			for (nNodeId=0; nNodeId<pBFiber->m_nCSetSize; nNodeId++) {
				if (pBFiber->m_pCSet[nNodeId] > nNewBChannels) {
					nNewBChannels = pBFiber->m_pCSet[nNodeId];
				}
			}
			if (nNewBChannels < pBFiber->m_nBChannels) {
				pBFiber->m_nBChannels--;
				pBFiber->releaseChannel(NULL);
				assert(pBFiber->m_nBChannels == nNewBChannels);
			}
		} // for itrB
		itrPIndex = itrP;
	} // for itr
}

//-B: build as many UniFiber object as the num of link in m_hLinkList and put them in the hLinkToBeDecr list passed as parameter
void WDMNetwork::ExtractListUniFiber(list<AbstractLink*>& hLinkToBeDecr)
{
	//list<UniFiber*> hLinkToBeDecr;
	list<AbstractLink*>::const_iterator itrLinkCI;
	//-B: for each link (AbstractLink object) of the list m_hLinkList (-> belonging to an AbstractGraph object)
	for (itrLinkCI = m_hLinkList.begin(); itrLinkCI != m_hLinkList.end(); itrLinkCI++)
	{
		//reference to UniFiber object pointing to each link (AbstractLink object) -> UniFiber is derived from AbstractLink class
 	    UniFiber *pUniFiber = (UniFiber*)(*itrLinkCI);
        
		// 	list<AbstractNode*>::const_iterator itrNode;
//                 UINT src=pUniFiber->getSrc()->getId();
//         	list<AbstractNode*>::const_iterator itrNode2;
//                 UINT dst=pUniFiber->getDst()->getId();
// 		OXCNode *pOXCSrc;
//                 OXCNode *pOXCDst;
//         	for (itrNode=m_hNodeList.begin(); itrNode!=m_hNodeList.end(); 
//         	itrNode++) {
// 	        	OXCNode *pOXC = (OXCNode*)(*itrNode);
// 	        	if (src==pOXC->getId())
//                                pOXCSrc= pOXC; 
// 	        }
//         	for (itrNode2=m_hNodeList.begin(); itrNode2!=m_hNodeList.end(); 
//         	itrNode2++) {
// 	        	OXCNode *pOXC2 = (OXCNode*)(*itrNode2);
// 	        	if (dst==pOXC2->getId())
//                                pOXCDst= pOXC2; 
// 	        }
// 		// AbstractNode *pOXCSrc=pUniFiber->getSrc();
// 		// AbstractNode *pOXCDst=pUniFiber->getDst();
		
		//-B: save source, destination and num of wavel of the considered link
		UINT nSrc = pUniFiber->getSrc()->getId();
		UINT nDst = pUniFiber->getDst()->getId();
		UINT nW = pUniFiber->m_nW;
		UniFiber  *pNewUniFiber = new UniFiber(pUniFiber->getId(),(OXCNode*)lookUpNodeById(nSrc),
    							       			(OXCNode*)lookUpNodeById(nDst),
        										pUniFiber->m_hCost,
					        					pUniFiber->m_nLength,
												pUniFiber->m_nW);
		
		//UniFiber *pNewUniFiber = addUniFiberOnServCopy(pUniFiber->getId(), 
														//pUniFiber->getSrc()->getId(),
														//pUniFiber->getDst()->getId(),
														//pUniFiber->m_nW,
														//pUniFiber->m_hCost,
														//pUniFiber->m_nLength);

	    Channel *pChannel = new Channel[nW];
         
		 //   UniFiber *pNewUniFiber = new UniFiber(nFiberId, (OXCNode*)lookUpNodeById(nSrc),
		 //  (OXCNode*)lookUpNodeById(nDst), hCost, nLength, nW);

	    UINT  w;
		for (w = 0; w < nW; w++)
			pChannel[w] = Channel(pNewUniFiber, w, m_nChannelCapacity, true, UP);
        pNewUniFiber->m_pChannel = pChannel; //m_pChannel is reference, in a UniFiber object, to an array of channel object -> this array has as many element as the num of wavel

		//UniFiber *pNewUniFiber;
 		//pNewUniFiber=&NewUniFiber;

        int w1;
		for (w1 = 0; w1 < pUniFiber->m_nW; w1++) {
 		   	pNewUniFiber->m_pChannel[w1] = pUniFiber->m_pChannel[w1];
	        pNewUniFiber->m_pChannel[w1].m_pLightpath = pUniFiber->m_pChannel[w1].m_pLightpath;
 		    pNewUniFiber->m_pChannel[w1].m_pUniFiber = pNewUniFiber;
 	    }
		//-B: what is m_pCSet???
	    if (pUniFiber->m_pCSet) {
			pNewUniFiber->m_pCSet = new UINT[pUniFiber->m_nCSetSize];
			pNewUniFiber->m_nCSetSize = pUniFiber->m_nCSetSize;
 		    memcpy(pNewUniFiber->m_pCSet, pUniFiber->m_pCSet, 
 			pUniFiber->m_nCSetSize*sizeof(UINT));
		}
		//-B: backup channels
 	    pNewUniFiber->m_nBChannels = pUniFiber->m_nBChannels;
        //-B: insert the new UniFiber object in the list passed as parameter to the method itself
		hLinkToBeDecr.push_back(pNewUniFiber);  
		//delete  pNewUniFiber;                                      
	}    
}

//****************** SEG_SP_AGBSP *******************
LINK_COST WDMNetwork::SEG_SP_AGBSP_ComputeRoute(Circuit& hCircuit,
												 NetMan *pNetMan,
												 OXCNode* pSrc, 
												 OXCNode* pDst)
{int K_=1;
	// obey fiber capacity
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

	// compute upto k primary candidates
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size()) {//cout<<"ZZZZ";
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>
	AbsPath *pPPath = NULL;
	//list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;
	LINK_COST hCurrPCost;//, hCurrBCost;
	list<AbsPath*>::iterator itrPPath;
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
      //(*itrPPath)->dump(cout);//
	  K_++;//
		AbsPath& hCurrPPath = **itrPPath;
		validateAllLinks();
        validateAllNodes();//-----

	
      list<AbsPath*> hCurrBSegsL;
	  list<AbsPath*> hCurrPSegsL;

     LINK_COST hCurrBCostL=SEG_SP_AGBSP_aux(hCurrPSegsL,hCurrBSegsL,hCurrPPath,pNetMan);//-t
//cout<<"s:"<<hCurrPSegsL.size()<<endl;
//list<AbstractLink*>::iterator hcpp;
//list<AbsPath*>::iterator hcpP;
/*for (hcpP=hCurrPSegsL.begin(); hcpP!=hCurrPSegsL.begin(); hcpP++) {
for (hcpp=(*hcpP)->m_hLinkList.begin(); hcpp!=(*hcpP)->m_hLinkList.end(); hcpp++) {}
}*/

#ifdef backup
	 {list<AbsPath*>::const_iterator itrBBS=hCurrBSegsL.begin(); 
     for(itrBBS;itrBBS!=hCurrBSegsL.end();itrBBS++){(*itrBBS)->dump(cout);}
	 }
#endif


	  if (UNREACHABLE != hCurrBCostL) {	
        UINT nTimeInf = pNetMan->getTimePolicy(); 
        switch (nTimeInf) {
        case 0: 
		  { // cout<<"s:"<<hCurrBSegsL.size()<<endl;
			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCostL) < hBestCost) {
				hBestCost = hCurrPCost + hCurrBCostL;
				pBestP = *itrPPath;                        
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				hBestB = hCurrBSegsL;
			} else {
				// release memory
				list<AbsPath*>::iterator itr;
				for (itr=hCurrBSegsL.begin(); itr!=hCurrBSegsL.end(); itr++) {
					delete (*itr);
				}
			} //if 
		    break; 
			   }

	    case 1:  // CI	
		  {
restoreLinkCost();
validateAllLinks();
validateAllNodes();//-----
hCurrBCostL=0;
list<AbsPath*> hCurrBSegsL_;
list<AbsPath*>::const_iterator  itCPS;
for(itCPS=hCurrPSegsL.begin();itCPS!=hCurrPSegsL.end();itCPS++){
//cout<<"sl:"<<(*itCPS)->m_hLinkList.size()<<endl;
		   SimulationTime hHTInc = pNetMan->m_hHoldingTimeIncoming;
           SimulationTime hATInc = pNetMan->m_hArrivalTimeIncoming;          
     	   list<AbstractLink*>  hLinkToBeD;
		   ExtractListUniFiber(hLinkToBeD); 
           bool bCopyCost=1;
           SimulationTime hTimeInterval;
           if (pNetMan->m_hDepaNet.empty()){
                 hTimeInterval= hHTInc; 
		        //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList); 
		        SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList,(*itCPS)->m_hLinkList);     
                bCopyCost=0;
           } 
           else 
          {
                     Event *pEvent;
                     Event *pFollowingEvent;
                     list<Event*>::const_iterator itrE;
                     SimulationTime  hCurrT =  hATInc; 
                   
                     for (itrE=pNetMan->m_hDepaNet.begin(); itrE!=pNetMan->m_hDepaNet.end() ; itrE++) {
			            pEvent = (Event*)(*itrE);
                        assert(pEvent);
                        SimulationTime InitialTime=hCurrT;
                        SimulationTime hCurrT= pEvent->m_hTime;
                        double HTFraction=1;  
                        if ( hHTInc/HTFraction + hATInc > hCurrT ){
                            if (itrE==(pNetMan->m_hDepaNet.begin())) {
                              SimulationTime hFirstEventT= pEvent->m_hTime;
                              if (hHTInc + hATInc <= hFirstEventT){
                                  hTimeInterval= (hHTInc); 
//SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList); 
SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList,(*itCPS)->m_hLinkList);  
                                  bCopyCost=0;
				                  break;
			                   }
							  else{
		      	                  hTimeInterval= (hFirstEventT - hATInc);
                                 bool bSave=0;
//SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 	
SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList,(*itCPS)->m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 
                                  Lightpath *pLS; 
                                  list<Lightpath*> ListLightpathWork =  pEvent->m_pConnection->m_pPCircuit->m_hRoute;//P or B
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                        pLS = (Lightpath*)(*itrLS);
                                        assert(pLS);
                                    } //end for
                              }//end else hHTInc
                          } //end if itrE
                          list<Event*>::const_iterator itrE2;
                          itrE2 = itrE;
                          itrE2++;
                          // if I don`t have a next event -> incoming ht
			              if (itrE2!=(pNetMan->m_hDepaNet.end())) {
			                  pFollowingEvent= (Event*)(*itrE2);
                              assert(pFollowingEvent);
			                  SimulationTime hNextDepT = pFollowingEvent->m_hTime;
			                  if (hHTInc + hATInc <= hNextDepT){
                                  hTimeInterval= (hHTInc + hATInc - hCurrT); 
                                 bool bSave=1; 
//SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 	
SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList,(*itCPS)->m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 
                                  break; 
			                  }
			                  else {
			                      hTimeInterval=(hNextDepT -hCurrT); 
                                  bool bSave=1;
// SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 	
SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList,(*itCPS)->m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 
			                       Lightpath *pLS;
                              //Prima della correzione 
                                  list<Lightpath*> ListLightpathWork =  pFollowingEvent->m_pConnection->m_pPCircuit->m_hRoute;
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                    pLS = (Lightpath*)(*itrLS);
                                    assert(pLS);
                                    pLS->releaseOnServCopy(hLinkToBeD);
                                   }
			                       continue;
                                 } // end else 
			                 } // end if itrE2
			              else{ 
                              hTimeInterval=( hHTInc + hATInc- hCurrT);
                              bool bSave=1;
//SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 	
SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hCurrPPath.m_hLinkList,(*itCPS)->m_hLinkList, hLinkToBeD, hTimeInterval,bSave); 
			                  break; 
			                  }
                 } //end if hHTInc/HTFraction + hATInc...  limitinf time of observation  
	                    }  //end for
		  } //end if Depanet Empty (not)                 
		   // For the cases  in which  SEG_SP_NO_HOP_UpdateLinkCost_Backup has not been used
		   if (!bCopyCost==0) {
		                 //Needed just to invalidate link
SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_BackupInvalidate(hCurrPPath.m_hLinkList,(*itCPS)->m_hLinkList); 
//SEG_SP_NO_HOP_UpdateLinkCost_BackupInvalidate(hCurrPPath.m_hLinkList);                    
	     	             //Map the new link costs assigned to hLinkToBeD in the m_LinkList  
                 UniFiber *pUFLTBD, *pUFm_h;
                 list<AbstractLink*>::const_iterator itrUFLTBD ,itrUFm_h ;
                 for (itrUFLTBD=hLinkToBeD.begin(); itrUFLTBD!=hLinkToBeD.end(); itrUFLTBD++) {
		                     pUFLTBD = (UniFiber*)(*itrUFLTBD);
                             assert(pUFLTBD);
	                         for (itrUFm_h=m_hLinkList.begin(); itrUFm_h!=m_hLinkList.end(); itrUFm_h++) {
		                          pUFm_h = (UniFiber*)(*itrUFm_h);
                                  assert(pUFm_h);
                                  if ((pUFLTBD -> m_nLinkId) == (pUFm_h -> m_nLinkId))
			                      {									  
									  LINK_COST hCostCI = pUFLTBD->getCost();
                                      pUFm_h->modifyCost(hCostCI);
                                    } // end if pUFLTBD
	                              } // end for itrUFm
                                 } //end for itrUFLTBD
		   } // end if bCopyCost 		 
           list<AbstractLink*>::const_iterator itrLinkCI2;
           for (itrLinkCI2=hLinkToBeD.begin(); itrLinkCI2!=hLinkToBeD.end();itrLinkCI2++) {
 	              UniFiber *pUniFiberZ = (UniFiber*)(*itrLinkCI2);
                  UINT nW=pUniFiberZ->m_nW;
                  if (pUniFiberZ->m_pChannel) {          
			           delete []pUniFiberZ->m_pChannel;
                       pUniFiberZ->m_pChannel=NULL;
			          }
	        	  if (pUniFiberZ->m_pCSet) {
			           delete []pUniFiberZ->m_pCSet; 
                       pUniFiberZ->m_pCSet=NULL;
             	      } 
			      delete pUniFiberZ;
                        // hLinkToBeD.pop_back();
		   } 


//fine -M
 list<AbstractLink*> hCurrBPathL;
 list<AbstractLink*>::iterator itrhCurrBPathL;
//(*itCPS)->m_hLinkList.begin().

list<AbstractLink*>::const_iterator it=(*itCPS)->m_hLinkList.begin();
AbstractNode *pSrcL=(*it)->m_pSrc;//cout<<pSrcL->getId()<<"-";
for (it; it!=(*itCPS)->m_hLinkList.end(); it++) {}
it--;
AbstractNode *pDstL=(*it)->m_pDst;//cout<<pDstL->getId()<<endl;
AbsPath *spPPath= new AbsPath();
//spPPath.m_hLinkList.clear();
//cout<<endl<<"C_parz:"<<hCurrBCostL<<endl;
hCurrBCostL += Dijkstra(hCurrBPathL,pSrcL, pDstL, AbstractGraph::LCF_ByOriginalLinkCost);
for(itrhCurrBPathL=hCurrBPathL.begin();itrhCurrBPathL!=hCurrBPathL.end();itrhCurrBPathL++){
//cout<<(*itrhCurrBPathL)->getSrc()->getId()<<"-"<<(*itrhCurrBPathL)->getDst()->getId()<<" ";
spPPath->m_hLinkList.push_back((*itrhCurrBPathL));}//cout<<endl;
//cout<<"siz:"<<spPPath->m_hLinkList.size()<<endl;
//spPPath->dump(cout);
hCurrBSegsL_.push_back(spPPath);
validateAllLinks();
validateAllNodes();//-----
restoreLinkCost();

          } //for	
//cout<<endl<<"C:"<<hCurrBCostL<<endl;

list<AbsPath*>::iterator hcpP;
for (hcpP=hCurrPSegsL.begin(); hcpP!=hCurrPSegsL.begin(); hcpP++) {
delete (*hcpP);
}

hCurrPSegsL.clear();
hCurrBSegsL.clear();     
if (hCurrBCostL<UNREACHABLE) {
 if (hCurrBCostL > 0) {	
			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCostL) < hBestCost) {
				hBestCost = hCurrPCost + hCurrBCostL;
				pBestP = *itrPPath;                        
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				hBestB = hCurrBSegsL_;
			} else {
				// release memory
				list<AbsPath*>::iterator itr;
				for (itr=hCurrBSegsL_.begin(); itr!=hCurrBSegsL_.end(); itr++) {
					delete (*itr);
				}
			}}//if
 }
hCurrBSegsL_.clear(); 
  break;                 
		 }

	    case 2:
   	    default:
        DEFAULT_SWITCH;
        };
      }
             	
	}//end for itrPPath

#ifdef dump_after
this->dump(cout);
#endif

	if (UNREACHABLE != hBestCost) {
		assert(pBestP);
		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);
		list<AbstractLink*>::const_iterator itr;
		for (itr=pBestP->m_hLinkList.begin(); itr!=pBestP->m_hLinkList.end();
		itr++) {
			   pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		       }
		pLPSeg->m_hCost = pBestP->getCost();
//assert(hBestB.size()>0);// -T
		pLPSeg->m_hBackupSegs = hBestB;
		hCircuit.addVirtualLink(pLPSeg);
	}
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}
	return hBestCost;
}


inline void WDMNetwork::SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(const list<AbstractLink*>& hPPath_,const list<AbstractLink*>& hPPath, const list<AbstractLink*>& hLinkToBeD, SimulationTime hDuration, bool bSaveCI)
{// invalidate working path hPPath_
list<AbstractLink*>::const_iterator itr;

list<AbstractLink*>::const_iterator itr22 = hPPath_.begin();
list<AbstractLink*>::const_iterator itr22ndToLast = hPPath_.end();
    (*itr22)->invalidate();
	itr22ndToLast--;
	for (itr22; itr22!=itr22ndToLast; itr22++){
		(*itr22)->invalidate();
		//if(itr22!=hPPath_.end()) 
			(*itr22)->getDst()->invalidate();
	}
(*itr22)->invalidate();
(*itr22)->getDst()->invalidate();
	//if (hPPath_.size() > 1){
	list<AbstractLink*>::const_reverse_iterator itr_;
	for (itr_=hPPath_.rbegin(); itr_!=hPPath_.rend();
	itr_++) {
		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pV = (*itr_)->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pV->m_hOLinkList.begin();//esamino dal penultimo nodo al primo i link entranti(opposite) nel nodo
		(itrO!=pV->m_hOLinkList.end()) && ((*itrO)->m_pDst!=(*itr_)->m_pSrc); 
		itrO++)	NULL;         //nodo in esame e' src e dst allo stesso tempo,allora il link e'su hPPath ma opposto 
                               //per uscire dal for basta che una condizione mi diventi == 
		if (itrO != pV->m_hOLinkList.end()) { //se dal for di prima sono uscito per la prima condiz (ma l'altra non soddisfatta)	
			pOppositeLink = *itrO;
			pOppositeLink->invalidate();
            //pOppositeLink->dump(cout);
		}
	}
	//}
//da qui uso il segmento hPPath
//if (hPPath_.size() > 1){
//if (hPPath.size() > 1){
    list<AbstractLink*>::const_iterator itrrt=hPPath.begin();
        (*itrrt)->getSrc()->validate();//cout<<"A";
        (*itrrt)->invalidate();
		for (itrrt; itrrt!=hPPath.end(); itrrt++){}
        itrrt--;
		(*itrrt)->getDst()->validate();//cout<<"B";
	
//}
//}
	// case 1: primary traverses multiple (>1) hops
	if (hPPath.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath_.end();
		itr2ndToLast--;
		list<AbstractLink*>::const_iterator itrK;
		for (itrK=hLinkToBeD.begin(); itrK!=hLinkToBeD.end(); itrK++) {//scorro i link del grafo
			if (!(*itrK)->valid()) continue;
		   UniFiber *pBFiber = (UniFiber*)(*itrK);
		   assert(pBFiber && pBFiber->m_pCSet);
		   bool bShareable = true;
		   list<AbstractLink*>::const_iterator itrFiber;
		   for (itrFiber=hPPath_.begin();(itrFiber!=itr2ndToLast) && bShareable; itrFiber++)//scorro il working
			{
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
			// I have to multiply the hDuration always for original cost and then
                            // sum the various contributes 
                                LINK_COST hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M               
                                LINK_COST hTimeCost= hCostOrig * hDuration ;               //-M
				                pBFiber->modifyCostCI(hTimeCost * m_dEpsilon,bSaveCI);    //-M 
			} else if (pBFiber->countFreeChannels() > 0) {
				                LINK_COST hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M        
                                LINK_COST hTimeCost= hCostOrig * hDuration;               //-M 
				 pBFiber->modifyCostCI(hTimeCost, bSaveCI);
			} else {
				pBFiber->invalidate();
			}
		}
	} 
	else {
	// case 2: primary traverses only one hop
		UINT nSrc = hPPath_.front()->getSrc()->getId();
		list<AbstractLink*>::const_iterator itrK;
		for (itrK=hLinkToBeD.begin(); itrK!=hLinkToBeD.end(); itrK++) {
			if (!(*itrK)->valid())
				continue;
			UniFiber *pBFiber = (UniFiber*)(*itrK);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
			                    LINK_COST  hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M                             
                                LINK_COST hTimeCost= hCostOrig * hDuration;                //-M
			                    pBFiber->modifyCostCI(hTimeCost * m_dEpsilon,bSaveCI);     //-M 
			} else if (pBFiber->countFreeChannels() > 0) {
				                LINK_COST  hCostOrig= pBFiber->getOriginalCost(bSaveCI);   //-M
                                LINK_COST hTimeCost= hCostOrig *  hDuration ;               //-M 
			                    pBFiber->modifyCostCI(hTimeCost, bSaveCI);                 //-M
			} else {
				pBFiber->invalidate();
			}
		}
	}
												 
}

inline void WDMNetwork::SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_BackupInvalidate(const list<AbstractLink*>& hPPath_,const list<AbstractLink*>& hPPath)
{
	// invalidate working path hPPath_
list<AbstractLink*>::const_iterator itr;

list<AbstractLink*>::const_iterator itr22 = hPPath_.begin();
list<AbstractLink*>::const_iterator itr22ndToLast = hPPath_.end();
    (*itr22)->invalidate();
	itr22ndToLast--;
	for (itr22; itr22!=itr22ndToLast; itr22++){
		(*itr22)->invalidate();
		//if(itr22!=hPPath_.end()) 
			(*itr22)->getDst()->invalidate();
	}
(*itr22)->invalidate();
(*itr22)->getDst()->invalidate();
	//if (hPPath_.size() > 1){
	list<AbstractLink*>::const_reverse_iterator itr_;
	for (itr_=hPPath_.rbegin(); itr_!=hPPath_.rend();
	itr_++) {
		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pV = (*itr_)->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pV->m_hOLinkList.begin();//esamino dal penultimo nodo al primo i link entranti(opposite) nel nodo
		(itrO!=pV->m_hOLinkList.end()) && ((*itrO)->m_pDst!=(*itr_)->m_pSrc); 
		itrO++)	NULL;         //nodo in esame e' src e dst allo stesso tempo,allora il link e'su hPPath ma opposto 
                               //per uscire dal for basta che una condizione mi diventi == 
		if (itrO != pV->m_hOLinkList.end()) { //se dal for di prima sono uscito per la prima condiz (ma l'altra non soddisfatta)	
			pOppositeLink = *itrO;
			pOppositeLink->invalidate();
            //pOppositeLink->dump(cout);
		}
	}
	//}
//da qui uso il segmento hPPath
//if (hPPath_.size() > 1){
//if (hPPath.size() > 1){
    list<AbstractLink*>::const_iterator itrrt=hPPath.begin();
        (*itrrt)->getSrc()->validate();//cout<<"A";
        (*itrrt)->invalidate();
		for (itrrt; itrrt!=hPPath.end(); itrrt++){}
        itrrt--;
		(*itrrt)->getDst()->validate();//cout<<"B";
	
//}
//}


	// case 1: segmento primary traverses multiple (>1) hops
	if (hPPath_.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath_.end();
		itr2ndToLast--;
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
		   UniFiber *pBFiber = (UniFiber*)(*itr);
		   assert(pBFiber && pBFiber->m_pCSet);
		   bool bShareable = true;
		   list<AbstractLink*>::const_iterator itrFiber;
		   for (itrFiber=hPPath_.begin();(itrFiber!=itr2ndToLast) && bShareable; itrFiber++)
			{
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
				//pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {//pBFiber->dump(cout);
				pBFiber->invalidate();
			}
		}
	} else {
	// case 2: segmento primary traverses only one hop
        UINT nSrc = hPPath_.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
				//pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {//pBFiber->dump(cout);
				pBFiber->invalidate();
			}
		}
	}
	
	
}

inline void WDMNetwork::SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(const list<AbstractLink*>& hPPath_,const list<AbstractLink*>& hPPath)
{
	// invalidate working path hPPath_
list<AbstractLink*>::const_iterator itr;

list<AbstractLink*>::const_iterator itr22 = hPPath_.begin();
list<AbstractLink*>::const_iterator itr22ndToLast = hPPath_.end();
    (*itr22)->invalidate();
	itr22ndToLast--;
	for (itr22; itr22!=itr22ndToLast; itr22++){
		(*itr22)->invalidate();
		//if(itr22!=hPPath_.end()) 
			(*itr22)->getDst()->invalidate();
	}
(*itr22)->invalidate();
(*itr22)->getDst()->invalidate();
	//if (hPPath_.size() > 1){
	list<AbstractLink*>::const_reverse_iterator itr_;
	for (itr_=hPPath_.rbegin(); itr_!=hPPath_.rend();
	itr_++) {
		AbstractLink *pOppositeLink = NULL;
		AbstractNode *pV = (*itr_)->m_pDst;
		// look up the link at the opposite direction
		list<AbstractLink*>::const_iterator itrO;
		for (itrO=pV->m_hOLinkList.begin();//esamino dal penultimo nodo al primo i link entranti(opposite) nel nodo
		(itrO!=pV->m_hOLinkList.end()) && ((*itrO)->m_pDst!=(*itr_)->m_pSrc); 
		itrO++)	NULL;         //nodo in esame e' src e dst allo stesso tempo,allora il link e'su hPPath ma opposto 
                               //per uscire dal for basta che una condizione mi diventi == 
		if (itrO != pV->m_hOLinkList.end()) { //se dal for di prima sono uscito per la prima condiz (ma l'altra non soddisfatta)	
			pOppositeLink = *itrO;
			pOppositeLink->invalidate();
            //pOppositeLink->dump(cout);
		}
	}
	//}
//da qui uso il segmento hPPath
//if (hPPath_.size() > 1){
//if (hPPath.size() > 1){
    list<AbstractLink*>::const_iterator itrrt=hPPath.begin();
        (*itrrt)->getSrc()->validate();//cout<<"A";
        (*itrrt)->invalidate();
		for (itrrt; itrrt!=hPPath.end(); itrrt++){}
        itrrt--;
		(*itrrt)->getDst()->validate();//cout<<"B";
	
//}
//}


	// case 1: segmento primary traverses multiple (>1) hops
	if (hPPath_.size() > 1) {
		list<AbstractLink*>::const_iterator itr2ndToLast = hPPath_.end();
		itr2ndToLast--;
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
		   UniFiber *pBFiber = (UniFiber*)(*itr);
		   assert(pBFiber && pBFiber->m_pCSet);
		   bool bShareable = true;
		   list<AbstractLink*>::const_iterator itrFiber;
		   for (itrFiber=hPPath_.begin();(itrFiber!=itr2ndToLast) && bShareable; itrFiber++)
			{
				assert(pBFiber->m_nBChannels >= 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()]);
				if (pBFiber->m_nBChannels == 
					pBFiber->m_pCSet[(*itrFiber)->getDst()->getId()])
					bShareable = false;
			}
			if (bShareable) {
				pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {//pBFiber->dump(cout);
				pBFiber->invalidate();
			}
		}
	} else {
	// case 2: segmento primary traverses only one hop
        UINT nSrc = hPPath_.front()->getSrc()->getId();
		for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
			if ((!(*itr)->valid()) || (!(*itr)->getSrc()->valid()) ||
				(!(*itr)->getDst()->valid())) {
				continue;
			}
			UniFiber *pBFiber = (UniFiber*)(*itr);
			assert(pBFiber && pBFiber->m_pCSet);
			bool bShareable = (pBFiber->m_pCSet[nSrc] < pBFiber->m_nBChannels);
			if (bShareable) {
				pBFiber->modifyCost(pBFiber->getCost() * m_dEpsilon);
			} else if (pBFiber->countFreeChannels() > 0) {
				NULL;
				// pBFiber->modifyCost(1);
			} else {//pBFiber->dump(cout);
				pBFiber->invalidate();
			}
		}
	}
	
	
}

inline LINK_COST WDMNetwork::SEG_SP_AGBSP_aux(list<AbsPath*>& hCurrPSegsLL,list<AbsPath*>& hCurrBSegsLL, const AbsPath& hPPath, NetMan *pNetMan)
{     
	//Creazione Grafo Ausiliario
      AbstractGraph AGBSP;
      UINT nTimeInf_ = pNetMan->getTimePolicy(); //cout<<"\n";
      list<AbstractLink*> hCurrPSegment,hCurrPSegment1;
     
      ///inseriscoOppositeWorking ///
      map<UINT, list<AbstractLink*> > PBSS;
	  map<UINT, list<AbstractLink*> > PWSS;
      int id=0;//int id=this->getNextLinkId();
      list<AbstractLink*>::const_iterator itrCPPath=hPPath.m_hLinkList.begin();
      AGBSP.addNode(new AbstractNode((*itrCPPath)->getSrc()->m_nNodeId));//cout<<(*itrCPPath)->getSrc()->m_nNodeId<<"-";
      for (itrCPPath; itrCPPath!=hPPath.m_hLinkList.end(); itrCPPath++) {
            AGBSP.addNode(new AbstractNode((*itrCPPath)->getDst()->m_nNodeId));//cout<<(*itrCPPath)->getDst()->m_nNodeId<<"-";
            AGBSP.addLink(new AbstractLink(id++,AGBSP.lookUpNodeById((*itrCPPath)->getDst()->m_nNodeId),
                           AGBSP.lookUpNodeById((*itrCPPath)->getSrc()->m_nNodeId),0,1));
            list<AbstractLink*> pBSegLL;
	        pBSegLL.push_back(AGBSP.lookUpLinkById(id-1));
            PBSS.insert(pair<UINT, list<AbstractLink*> >(id-1, pBSegLL));
            PWSS.insert(pair<UINT, list<AbstractLink*> >(id-1, pBSegLL));
      }		
      list<AbstractLink*> hCurrBPathL;//,hCurrBPathLnew ,hCurrBPathLAGBSP;  
      LINK_COST hCurrBCostL;
      list<AbstractLink*>::const_iterator itrLp,itrLLp;
      list<AbstractLink*>::iterator ip,ip_;
      list<AbstractLink*>::const_iterator ipp=hPPath.m_hLinkList.begin();;
      list<AbstractLink*>::const_iterator ipp_=hPPath.m_hLinkList.begin();;
       ///inseriscoPBSS ///
      id=hPPath.m_hLinkList.size()+2;
      for(itrLp=hPPath.m_hLinkList.begin(); itrLp!=hPPath.m_hLinkList.end(); itrLp++){
	        AbstractNode *pSrcL=(*itrLp)->getSrc();//cout<<(*pSrcL).getId()<<"-";
            AbstractNode *pDstL=(*itrLp)->getDst();//cout<<(*pDstL).getId()<<" ";//(*itrLp)->dump(cout);
            ipp_=itrLp;
            hCurrPSegment.push_back((*itrLp));
            validateAllLinks();
            validateAllNodes();//-----
//double psize=hPPath.m_hLinkList.size();
//psize=(double) ((psize)*(psize+1))/2;
            int psize=1;
            switch (nTimeInf_) {
              case 0: //No Time 
		      {   //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPSegment); 
		     SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hPPath.m_hLinkList,hCurrPSegment);     
           break;
                 }
	          case 1:  // CI	
		/*  {
		   SimulationTime hHTInc = pNetMan->m_hHoldingTimeIncoming;
           SimulationTime hATInc = pNetMan->m_hArrivalTimeIncoming;          
     	   list<AbstractLink*>  hLinkToBeD;
		   ExtractListUniFiber(hLinkToBeD); 
           bool bCopyCost=1;
           SimulationTime hTimeInterval;
           if (pNetMan->m_hDepaNet.empty()){
                 hTimeInterval= hHTInc; 
		        //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPSegment); 
		        SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hPPath.m_hLinkList,hCurrPSegment);     
                bCopyCost=0;
           } 
           else 
          {
                     Event *pEvent;
                     Event *pFollowingEvent;
                     list<Event*>::const_iterator itrE;
                     SimulationTime  hCurrT =  hATInc; 
                   
                     for (itrE=pNetMan->m_hDepaNet.begin(); itrE!=pNetMan->m_hDepaNet.end() ; itrE++) {
			            pEvent = (Event*)(*itrE);
                        assert(pEvent);
                        SimulationTime InitialTime=hCurrT;
                        SimulationTime hCurrT= pEvent->m_hTime;
                        double HTFraction=1;  
                        if ( hHTInc/HTFraction + hATInc > hCurrT ){
                            if (itrE==(pNetMan->m_hDepaNet.begin())) {
                              SimulationTime hFirstEventT= pEvent->m_hTime;
                              if (hHTInc + hATInc <= hFirstEventT){
                                  hTimeInterval= (hHTInc); 
				                  //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPSegment); 
		        SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hPPath.m_hLinkList,hCurrPSegment);  
                                  bCopyCost=0;
				                  break;
			                   }
							  else{
		      	                  hTimeInterval= (hFirstEventT - hATInc);
                                 bool bSave=0;
				                 SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hPPath.m_hLinkList,hCurrPSegment, hLinkToBeD, hTimeInterval,bSave);
                                  Lightpath *pLS; 
                                  list<Lightpath*> ListLightpathWork =  pEvent->m_pConnection->m_pPCircuit->m_hRoute;//P or B
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                        pLS = (Lightpath*)(*itrLS);
                                        assert(pLS);
                                    } //end for
                              }//end else hHTInc
                          } //end if itrE
                          list<Event*>::const_iterator itrE2;
                          itrE2 = itrE;
                          itrE2++;
                          // if I don`t have a next event -> incoming ht
			              if (itrE2!=(pNetMan->m_hDepaNet.end())) {
			                  pFollowingEvent= (Event*)(*itrE2);
                              assert(pFollowingEvent);
			                  SimulationTime hNextDepT = pFollowingEvent->m_hTime;
			                  if (hHTInc + hATInc <= hNextDepT){
                                  hTimeInterval= (hHTInc + hATInc - hCurrT); 
                                 bool bSave=1; 
			                        SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hPPath.m_hLinkList,hCurrPSegment, hLinkToBeD, hTimeInterval,bSave);
                                  break; 
			                  }
			                  else {
			                      hTimeInterval=(hNextDepT -hCurrT); 
                                  bool bSave=1;
                                   SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hPPath.m_hLinkList,hCurrPSegment, hLinkToBeD, hTimeInterval,bSave);
			                       Lightpath *pLS;
                              //Prima della correzione 
                                  list<Lightpath*> ListLightpathWork =  pFollowingEvent->m_pConnection->m_pPCircuit->m_hRoute;
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                    pLS = (Lightpath*)(*itrLS);
                                    assert(pLS);
                                    pLS->releaseOnServCopy(hLinkToBeD);
                                   }
			                       continue;
                                 } // end else 
			                 } // end if itrE2
			              else{ 
                              hTimeInterval=( hHTInc + hATInc- hCurrT);
                              bool bSave=1;
                              SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hPPath.m_hLinkList,hCurrPSegment, hLinkToBeD, hTimeInterval,bSave); 
			                  break; 
			                  }
                 } //end if hHTInc/HTFraction + hATInc...  limitinf time of observation  
	                    }  //end for
		  } //end if Depanet Empty (not)
                 
		   // For the cases  in which  SEG_SP_NO_HOP_UpdateLinkCost_Backup has not been used
		   if (!bCopyCost==0) {
		                 //Needed just to invalidate link
SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_BackupInvalidate(hPPath.m_hLinkList,hCurrPSegment); 
                 //SEG_SP_NO_HOP_UpdateLinkCost_BackupInvalidate(hCurrPSegment);                    
	     	             //Map the new link costs assigned to hLinkToBeD in the m_LinkList  
                 UniFiber *pUFLTBD, *pUFm_h;
                 list<AbstractLink*>::const_iterator itrUFLTBD ,itrUFm_h ;
                 for (itrUFLTBD=hLinkToBeD.begin(); itrUFLTBD!=hLinkToBeD.end(); itrUFLTBD++) {
		                     pUFLTBD = (UniFiber*)(*itrUFLTBD);
                             assert(pUFLTBD);
	                         for (itrUFm_h=m_hLinkList.begin(); itrUFm_h!=m_hLinkList.end(); itrUFm_h++) {
		                          pUFm_h = (UniFiber*)(*itrUFm_h);
                                  assert(pUFm_h);
                                  if ((pUFLTBD -> m_nLinkId) == (pUFm_h -> m_nLinkId))
			                      {									  
									  LINK_COST hCostCI = pUFLTBD->getCost();
                                      pUFm_h->modifyCost(hCostCI);
                                    } // end if pUFLTBD
	                              } // end for itrUFm
                                 } //end for itrUFLTBD
		   } // end if bCopyCost 
		 
           list<AbstractLink*>::const_iterator itrLinkCI2;
           for (itrLinkCI2=hLinkToBeD.begin(); itrLinkCI2!=hLinkToBeD.end();itrLinkCI2++) {
 	              UniFiber *pUniFiberZ = (UniFiber*)(*itrLinkCI2);
                  UINT nW=pUniFiberZ->m_nW;
                  if (pUniFiberZ->m_pChannel) {          
			           delete []pUniFiberZ->m_pChannel;
                       pUniFiberZ->m_pChannel=NULL;
			          }
	        	  if (pUniFiberZ->m_pCSet) {
			           delete []pUniFiberZ->m_pCSet; 
                       pUniFiberZ->m_pCSet=NULL;
             	      } 
			      delete pUniFiberZ;
                        // hLinkToBeD.pop_back();
		          }  		   
	       break;                 
		 }
*/
              {    SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hPPath.m_hLinkList,hCurrPSegment);     
           break; }
	          case 2:
   	          default:
              DEFAULT_SWITCH;
               };
            hCurrBCostL = Dijkstra(hCurrBPathL, pSrcL, pDstL, AbstractGraph::LCF_ByOriginalLinkCost);
            restoreLinkCost();
            //for(ip=hCurrPSegment.begin();ip!=hCurrPSegment.end();ip++){cout<<"{"<<(*ip)->getSrc()->getId()<<"-"<<(*ip)->getDst()->getId()<<"}";}cout<<":-:";
            //for(ip_=hCurrBPathL.begin();ip_!=hCurrBPathL.end();ip_++){cout<<"{"<<(*ip_)->getSrc()->getId()<<"-"<<(*ip_)->getDst()->getId()<<"}";}cout.precision(12);cout<<"   C:"<<hCurrBCostL<<endl;      
            if(hCurrBCostL!=UNREACHABLE){
            list<AbstractLink*> pBSegL;
		    list<AbstractLink*>::const_iterator itrBL = hCurrBPathL.begin();
            while (itrBL != hCurrBPathL.end()) {//per ogni link percorso backup 
	            pBSegL.push_back(*itrBL);//inserisco il link nel segmento
			    itrBL++;}
            hCurrBPathL.clear();
            PBSS.insert(pair<UINT, list<AbstractLink*> >(id, pBSegL));//map<UINT, AbsPath*>::iterator itrBBLB = PBSS.find(id);//(*itrBBLB).second->dump(cout);cout<<"a "<<endl;
            PWSS.insert(pair<UINT, list<AbstractLink*> >(id, hCurrPSegment));//map<UINT, AbsPath*>::iterator itrBBLB = PBSS.find(id);//(*itrBBLB).second->dump(cout);cout<<"a "<<endl;
			AGBSP.addLink(new AbstractLink(id++, AGBSP.lookUpNodeById(pSrcL->getId()),
				             AGBSP.lookUpNodeById(pDstL->getId()), hCurrBCostL,1));}

            hCurrPSegment.clear();
            itrLLp=itrLp;
		    itrLLp++;
            while(itrLLp!=hPPath.m_hLinkList.end()){
               AbstractNode *pDstLnew=(*itrLLp)->getDst();//cout<<"nuovo"<<(*itrLLp)->getDst()->getId()<<" ";//
               ipp=ipp_;
               while(ipp!=itrLLp){
               hCurrPSegment1.push_back((*ipp));
               ipp++;}
               hCurrPSegment1.push_back((*ipp));
               psize++;//--;
               validateAllLinks();
               validateAllNodes();//-----
               switch (nTimeInf_) {
      case 0: //No Time 
		 {  //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPSegment1);  
		    SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hPPath.m_hLinkList,hCurrPSegment1);     
           break;}
       case 1:  // CI	
		/*  {
		   SimulationTime hHTInc = pNetMan->m_hHoldingTimeIncoming;
           SimulationTime hATInc = pNetMan->m_hArrivalTimeIncoming;          
     	   list<AbstractLink*>  hLinkToBeD;
		   ExtractListUniFiber(hLinkToBeD); 
           bool bCopyCost=1;
           SimulationTime hTimeInterval;
           if (pNetMan->m_hDepaNet.empty()){
                 hTimeInterval= hHTInc; 
		        //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPSegment); 
		        SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hPPath.m_hLinkList,hCurrPSegment1);     
                bCopyCost=0;
           } 
           else 
          {
                     Event *pEvent;
                     Event *pFollowingEvent;
                     list<Event*>::const_iterator itrE;
                     SimulationTime  hCurrT =  hATInc; 
                   
                     for (itrE=pNetMan->m_hDepaNet.begin(); itrE!=pNetMan->m_hDepaNet.end() ; itrE++) {
			            pEvent = (Event*)(*itrE);
                        assert(pEvent);
                        SimulationTime InitialTime=hCurrT;
                        SimulationTime hCurrT= pEvent->m_hTime;
                        double HTFraction=1;  
                        if ( hHTInc/HTFraction + hATInc > hCurrT ){
                            if (itrE==(pNetMan->m_hDepaNet.begin())) {
                              SimulationTime hFirstEventT= pEvent->m_hTime;
                              if (hHTInc + hATInc <= hFirstEventT){
                                  hTimeInterval= (hHTInc); 
				                  //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPSegment); 
		        SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hPPath.m_hLinkList,hCurrPSegment1);  
                                  bCopyCost=0;
				                  break;
			                   }
							  else{
		      	                  hTimeInterval= (hFirstEventT - hATInc);
                                 bool bSave=0;
				                 SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hPPath.m_hLinkList,hCurrPSegment1, hLinkToBeD, hTimeInterval,bSave);
                                  Lightpath *pLS; 
                                  list<Lightpath*> ListLightpathWork =  pEvent->m_pConnection->m_pPCircuit->m_hRoute;//P or B
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                        pLS = (Lightpath*)(*itrLS);
                                        assert(pLS);
                                    } //end for
                              }//end else hHTInc
                          } //end if itrE
                          list<Event*>::const_iterator itrE2;
                          itrE2 = itrE;
                          itrE2++;
                          // if I don`t have a next event -> incoming ht
			              if (itrE2!=(pNetMan->m_hDepaNet.end())) {
			                  pFollowingEvent= (Event*)(*itrE2);
                              assert(pFollowingEvent);
			                  SimulationTime hNextDepT = pFollowingEvent->m_hTime;
			                  if (hHTInc + hATInc <= hNextDepT){
                                  hTimeInterval= (hHTInc + hATInc - hCurrT); 
                                 bool bSave=1; 
			                        SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hPPath.m_hLinkList,hCurrPSegment1, hLinkToBeD, hTimeInterval,bSave);
                                  break; 
			                  }
			                  else {
			                      hTimeInterval=(hNextDepT -hCurrT); 
                                  bool bSave=1;
                                   SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hPPath.m_hLinkList,hCurrPSegment1, hLinkToBeD, hTimeInterval,bSave);
			                       Lightpath *pLS;
                              //Prima della correzione 
                                  list<Lightpath*> ListLightpathWork =  pFollowingEvent->m_pConnection->m_pPCircuit->m_hRoute;
                                  list<Lightpath*>::const_iterator itrLS;
                                  for (itrLS=ListLightpathWork.begin(); itrLS!=ListLightpathWork.end(); itrLS++) {
                                    pLS = (Lightpath*)(*itrLS);
                                    assert(pLS);
                                    pLS->releaseOnServCopy(hLinkToBeD);
                                   }
			                       continue;
                                 } // end else 
			                 } // end if itrE2
			              else{ 
                              hTimeInterval=( hHTInc + hATInc- hCurrT);
                              bool bSave=1;
                              SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(hPPath.m_hLinkList,hCurrPSegment1, hLinkToBeD, hTimeInterval,bSave); 
			                  break; 
			                  }
                 } //end if hHTInc/HTFraction + hATInc...  limitinf time of observation  
	                    }  //end for
		  } //end if Depanet Empty (not)
                 
		   // For the cases  in which  SEG_SP_NO_HOP_UpdateLinkCost_Backup has not been used
		   if (!bCopyCost==0) {
		                 //Needed just to invalidate link
             SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_BackupInvalidate(hPPath.m_hLinkList,hCurrPSegment1); 
                 //SEG_SP_NO_HOP_UpdateLinkCost_BackupInvalidate(hCurrPSegment);                    
	     	             //Map the new link costs assigned to hLinkToBeD in the m_LinkList  
                 UniFiber *pUFLTBD, *pUFm_h;
                 list<AbstractLink*>::const_iterator itrUFLTBD ,itrUFm_h ;
                 for (itrUFLTBD=hLinkToBeD.begin(); itrUFLTBD!=hLinkToBeD.end(); itrUFLTBD++) {
		                     pUFLTBD = (UniFiber*)(*itrUFLTBD);
                             assert(pUFLTBD);
	                         for (itrUFm_h=m_hLinkList.begin(); itrUFm_h!=m_hLinkList.end(); itrUFm_h++) {
		                          pUFm_h = (UniFiber*)(*itrUFm_h);
                                  assert(pUFm_h);
                                  if ((pUFLTBD -> m_nLinkId) == (pUFm_h -> m_nLinkId))
			                      {									  
									  LINK_COST hCostCI = pUFLTBD->getCost();
                                      pUFm_h->modifyCost(hCostCI);
                                    } // end if pUFLTBD
	                              } // end for itrUFm
                                 } //end for itrUFLTBD
		   } // end if bCopyCost 
		 
           list<AbstractLink*>::const_iterator itrLinkCI2;
           for (itrLinkCI2=hLinkToBeD.begin(); itrLinkCI2!=hLinkToBeD.end();itrLinkCI2++) {
 	              UniFiber *pUniFiberZ = (UniFiber*)(*itrLinkCI2);
                  UINT nW=pUniFiberZ->m_nW;
                  if (pUniFiberZ->m_pChannel) {          
			           delete []pUniFiberZ->m_pChannel;
                       pUniFiberZ->m_pChannel=NULL;
			          }
	        	  if (pUniFiberZ->m_pCSet) {
			           delete []pUniFiberZ->m_pCSet; 
                       pUniFiberZ->m_pCSet=NULL;
             	      } 
			      delete pUniFiberZ;
                        // hLinkToBeD.pop_back();
		          }  		   
	       break;                 
		 }
*/
{    SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hPPath.m_hLinkList,hCurrPSegment1);
break;}
	  case 2:
	  default:
      DEFAULT_SWITCH;
      };

               hCurrBCostL = Dijkstra(hCurrBPathL, pSrcL, pDstLnew, AbstractGraph::LCF_ByOriginalLinkCost);
               restoreLinkCost();
                //for(ip=hCurrPSegment1.begin();ip!=hCurrPSegment1.end();ip++){cout<<"{"<<(*ip)->getSrc()->getId()<<"-"<<(*ip)->getDst()->getId()<<"}";}cout<<":-:";
            // for(ip_=hCurrBPathL.begin();ip_!=hCurrBPathL.end();ip_++){cout<<"{"<<(*ip_)->getSrc()->getId()<<"-"<<(*ip_)->getDst()->getId()<<"}";}cout.precision(12);cout<<"   C:"<<hCurrBCostL<<endl;          
			   if(hCurrBCostL!=UNREACHABLE){
                list<AbstractLink*> pBSegL_;//creo nuovo segmento
		        list<AbstractLink*>::const_iterator itrBL = hCurrBPathL.begin();
                while (itrBL != hCurrBPathL.end()) {//per ogni link percorso backup 
	              pBSegL_.push_back(*itrBL);//inserisco il link nel segmento
			      itrBL++;}
                hCurrBPathL.clear();
                PBSS.insert(pair<UINT, list<AbstractLink*> >(id, pBSegL_));//map<UINT, AbsPath*>::iterator itrBBLB = PBSS.find(id);//(*itrBBLB).second->dump(cout);cout<<"b "<<endl;
        	    PWSS.insert(pair<UINT, list<AbstractLink*> >(id, hCurrPSegment1));
				AGBSP.addLink(new AbstractLink(id++,AGBSP.lookUpNodeById(pSrcL->getId()),
                             AGBSP.lookUpNodeById(pDstLnew->getId()), hCurrBCostL,1));
		 }			
               hCurrPSegment1.clear();
	           itrLLp++;
		 }//cout<<"ok"<<(*itrLp)->getDst()->getId()<<" ";//
      }//for(itrLp...
      //AGBSP.dump(cout);
      itrCPPath--;
      list<AbstractLink*>::const_iterator itrCPPa_t_h=hPPath.m_hLinkList.begin();
      hCurrBCostL=AGBSP.Dijkstra(hCurrBPathL, AGBSP.lookUpNodeById((*itrCPPa_t_h)->getSrc()->m_nNodeId), 
						   AGBSP.lookUpNodeById((*itrCPPath)->getDst()->m_nNodeId), AbstractGraph::LCF_ByOriginalLinkCost);
	  // cout<<"   CostoTOT:"<<hCurrBCostL+hPPath.getCost()<<endl;//nlinkBackupinAGBSP:"<<hCurrBPathLAGBSP.size()<<endl; 
	  if(hCurrBCostL!=UNREACHABLE){//rimappo nel grafo originale//
	     list<AbstractLink*>::iterator itrBBL = hCurrBPathL.begin();
         while (itrBBL != hCurrBPathL.end()) {//per ogni link percorso backup in AGBSP
   	       int idd=(*itrBBL)->getId();// cout<<"idd"<<idd<<" ";//map<UINT, AbsPath*>::const_iterator itrBBLB = PBSS.find(idd);
            //AbsPath*  SSPath=PBSS.find(idd)->second; //cout<<"segmento"<<q++<<endl;//if((q-1)>1)cout<<" *"<<endl;//(*itrBBLB).second->dump(cout);//cout<<(*itrBBLB).first<<" ";	  
     	   assert(idd>0);
           AbsPath *pBS = new AbsPath();
		   if(idd>hPPath.m_hLinkList.size()){
           switch (nTimeInf_) {
            case 0: 
			{list<AbstractLink*> SSPath=PBSS.find(idd)->second;	
             list<AbstractLink*>::const_iterator itraaz = SSPath.begin();
             while (itraaz!= SSPath.end()) {//per ogni link percorso backup 
				    pBS->m_hLinkList.push_back(*itraaz);//inserisco il link nel segmento
	                itraaz++;}
             hCurrBSegsLL.push_back(pBS);
			 hCurrPSegsLL.clear();
			 break;
			}
	        case 1:  // CI	
			{list<AbstractLink*> WSPath=PWSS.find(idd)->second;
             list<AbstractLink*>::const_iterator itraazw = WSPath.begin();
             while (itraazw!= WSPath.end()) {//per ogni link segmento working 
			        pBS->m_hLinkList.push_back(*itraazw);//inserisco il link nel segmento
	                itraazw++;}     
             hCurrPSegsLL.push_back(pBS);
             hCurrBSegsLL.clear();
             break;
			}
	        case 2:
   	        default:
            DEFAULT_SWITCH;
         };             
		  }//if(idd>hPPath
			 // hCurrBSegsLL.splice(hCurrBSegsLL.end(),SSPath);}//inserisco nella lista solo i segmenti non opposti 
          itrBBL++;
        }//while
	  }//if(hCurrBCostL!=UNREACHABLE)	  
	  hCurrBPathL.clear();
	  PBSS.clear();
      PWSS.clear();
	  AGBSP.deleteContent(); 
      validateAllLinks();
      validateAllNodes();//-----
	  return hCurrBCostL; 

}

//****************** SEG_SP_L_NO_HOP *******************
LINK_COST WDMNetwork::SEG_SP_L_NO_HOP_ComputeRoute(Circuit& hCircuit,
												 NetMan *pNetMan,
												 OXCNode* pSrc, 
												 OXCNode* pDst,
												 Connection *pCon)
{int np=0;
	// obey fiber capacity
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		if (0 == pUniFiber->countFreeChannels())
			pUniFiber->invalidate();
	}

	// compute upto k primary candidates
	list<AbsPath*> hPPathList;
	this->Yen(hPPathList, pSrc, pDst, pNetMan->getNumberOfAltPaths(), 
				AbstractGraph::LCF_ByOriginalLinkCost);
	if (0 == hPPathList.size()) {
		validateAllLinks();
		return UNREACHABLE;
	}

	// store the best <primary, backup segments>
	AbsPath *pPPath = NULL;
	list<AbsPath*> hBSegs;

	// compute the best backup for each primary candidate
	// pick the pair <working, backup> of minimum cost
	LINK_COST hBestCost = UNREACHABLE;
	AbsPath *pBestP = NULL;
	list<AbsPath*> hBestB;
	LINK_COST hCurrPCost;
	LINK_COST hCurrBCost=0;
    LINK_COST hCurrBCostL;
	list<AbsPath*>::iterator itrPPath;
    list<AbstractLink*> hCurrBPath;//
	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
      //(*itrPPath)->dump(cout);
	  AbsPath& hCurrPPath = **itrPPath;
	  validateAllLinks();
      validateAllNodes();//-----
		// redefine link cost for computing link/node-disjoint backup
		//SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList);
    //---------------------------------------------------------------------------------------------------------
      UINT nTimeInf = pNetMan->getTimePolicy(); 
	  ///inizio link-protection ///
      list<AbstractLink*> hCurrBPathL;  
      list<AbsPath*> hCurrBSegsL;
      LINK_COST hBestCostL=UNREACHABLE;
      list<AbstractLink*>::const_iterator itrLp;
      int aa=1; //cout<<endl<<hCurrPPath.m_hLinkList.size()<<"_ ";     
      for(itrLp=hCurrPPath.m_hLinkList.begin(); itrLp!=hCurrPPath.m_hLinkList.end(); itrLp++){
        list<AbstractLink*> hCurrPSegment;
	    if (aa!=0){
	      AbstractNode *pSrcL=(*itrLp)->getSrc();
    	  int nn=1;
    	  if (hCurrPPath.m_hLinkList.size()>1){
            if (itrLp!=hCurrPPath.m_hLinkList.end()) hCurrPSegment.push_back(*itrLp);//cout<<"A";
            itrLp++;//if (itrLp!=hCurrPPath.m_hLinkList.end()) hCurrPSegment.push_back(*itrLp);cout<<"B";
       	    while((itrLp!=hCurrPPath.m_hLinkList.end()) && (nn<(pCon->m_nHopCount))){
          	    nn++;
                if (itrLp!=hCurrPPath.m_hLinkList.end()) hCurrPSegment.push_back(*itrLp);//cout<<"C";
                itrLp++;
		    }//while //if (itrLp!=hCurrPPath.m_hLinkList.end()) hCurrPSegment.pop_back();cout<<"D";
            itrLp--;
            SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList,hCurrPSegment);
            //SEG_SP_NO_HOP_UpdateLinkCost_Backup(hCurrPSegment);
		  }else{
            if (itrLp!=hCurrPPath.m_hLinkList.end()) hCurrPSegment.push_back(*itrLp);//cout<<"E";
            SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(hCurrPPath.m_hLinkList,hCurrPSegment);
		  }//if(hCurrPPath.m_hLinkList.size()>1)	

	      /*list<AbstractLink*>::iterator itrth=hCurrPSegment.begin();
          int ss=hCurrPSegment.size();//cout<<ss;         
          while(itrth!=hCurrPSegment.end()){cout<<(*itrth)->getSrc()->getId()<<"-"<<(*itrth)->getDst()->getId()<<" ";		  
          itrth++;} cout<<" "<<endl;*/

       hCurrPSegment.clear();
       AbstractNode *pDstL=(*itrLp)->getDst();//cout<<"nuovo"<<(*itrLp)->getDst()->getId()<<" ";//
	   hCurrBCostL = Dijkstra(hCurrBPathL, pSrcL, pDstL,AbstractGraph::LCF_ByOriginalLinkCost); 
       restoreLinkCost();
       validateAllLinks();
       validateAllNodes();
       if (UNREACHABLE != hCurrBCostL){//il costo di un segmento
            AbsPath *pBSegL = new AbsPath(); //creo nuovo segmento
		    hCurrBSegsL.push_back(pBSegL); //inserisco un nuovo segmento vuoto nella lista
            list<AbstractLink*>::const_iterator itrBL = hCurrBPathL.begin();
            while (itrBL != hCurrBPathL.end()) {//per ogni link percorso backup 
	               pBSegL->m_hLinkList.push_back(*itrBL);//inserisco il link nel segmento
			       itrBL++;
              }
            hCurrBCost=hCurrBCost+hCurrBCostL;
            hCurrBPathL.clear();
          }else{aa=0;}
	   }//if (aa=1)
      }//for(itrLp...
 
	  if (UNREACHABLE != hCurrBCostL) {
			// compare cost
			hCurrPCost = hCurrPPath.getCost();
			if ((hCurrPCost + hCurrBCost) < hBestCost) {
				hBestCost = hCurrPCost + hCurrBCost;
				pBestP = *itrPPath;                        
				list<AbsPath*>::iterator itr;
				for (itr=hBestB.begin(); itr!=hBestB.end(); itr++) {
					delete (*itr);
				}
				hBestB = hCurrBSegsL;
			} else {
				// release memory
				list<AbsPath*>::iterator itr;
				for (itr=hCurrBSegsL.begin(); itr!=hCurrBSegsL.end(); itr++) {
					delete (*itr);
				}
			}
		} // if

	  restoreLinkCost();	
		   
	}//end for itrPPath



	if (UNREACHABLE != hBestCost) {//cout<<"c"<<endl;
		assert(pBestP);
		// construct the circuit
		Lightpath_Seg *pLPSeg = new Lightpath_Seg(0);
		list<AbstractLink*>::const_iterator itr;
		for (itr=pBestP->m_hLinkList.begin(); itr!=pBestP->m_hLinkList.end();
		itr++) {
			   pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		       }
		pLPSeg->m_hCost = pBestP->getCost();


		pLPSeg->m_hBackupSegs = hBestB;
		hCircuit.addVirtualLink(pLPSeg);
	}

	for (itrPPath=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
		delete (*itrPPath);
	}
	return hBestCost;
}


//////MIE FUNZIONI//////////////////////////////////////////////////////////////

double WDMNetwork::EstimateGreenPower(int NodeID,int TimeZone,int time)
{
int DCtype=0;// vale 1 se DC-Wind, 2 se DC-Solar, 3 se DC-Hydro
int j;
//int k; // contatore per verifica vettori 
double power_disp;

// Verifica conteuto vettori
//for (k=0;k!=nDC;k++) {
//cout<<DCvettW[k]<<"W\n";   }
//for (k=0;k!=nDC;k++) {
//cout<<DCvettS[k]<<"S\n";  }
//for (k=0;k!=nDC;k++) {
//cout<<DCvettH[k]<<"H\n";  }
//cin.get();


for (j=0;j!=nDC;j++) 
	{
		if  (DCvettW[j]==NodeID) 
		 DCtype=1;
		if  (DCvettS[j]==NodeID)
		 DCtype=2;
		if  (DCvettH[j]==NodeID)
		 DCtype=3;
}

if  (DCtype==0)  // se non è un nodo con DC
{
 cout<<"Error - No DC in node"<<NodeID;
 cin.get();
 power_disp=0;
}

//cout<<"TYPE"<<DCtype;  // verifica tipo di energia
//cin.get();
switch(DCtype)

{
	case 1: 
		{
		power_disp=WindPower(time,TimeZone);
	   //cout<<"Potenza disponibile: "<<power_disp<<" W\n";
	break;
		}
	case 2: 
		{
		 power_disp=SolarPower(time,TimeZone);
		 //cout<<"Potenza disponibile: "<<power_disp<<" W\n";
	break;
		}
	case 3:
		{
		  power_disp=BrownPower(time,TimeZone);
		  //cout<<"Potenza disponibile: "<<power_disp<<" W\n";
	break;
		}
	default:
	 DEFAULT_SWITCH;
}

return power_disp; 
}

double WDMNetwork::SolarPower(int istant,int TimeZone)

{ 
	double SolarDistribution[]={0,0,0,0,0,0,0,2000,6000,8500,13000,15000,17500,17500,17500,15000,13000,11000,7500,5000,3500,1000,0,0}; // distribuzione presa da progetto INTERNET 153kW totali
    //double SolarDistribution[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	//double SolarDistribution[]={1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000};
	//double SolarDistribution[]={1000,1000,1000,1000,1000,1000,1000,20000,60000,85000,130000,150000,175000,175000,175000,150000,130000,110000,75000,50000,35000,10000,0,0};
	double power = 2000*SolarDistribution[(istant+TimeZone+24)%24]; // aggiungo 24 perchè il % non funziona con i negativi
	//double power = 1000*SolarDistribution[(istant+TimeZone+24)%24]; //  per rete ITALYcon arrivi 80/s
return power;
}

double WDMNetwork::WindPower(int istant,int TimeZone)

{
	//int istant=ceil(time);
	//istant=1+istant%24; // verificare, per ora tempo di simulazione breve.. 
	//double WindDistribution[]={23500,21000,2000,15000,3000,1000,10000,14000,16000,13000,9000,7000,1000,0,5000,17000,19000,16000,13000,8000,12000,14000,20000,23000}; //305 kWatt
	//double WindDistribution[]={0,0,0,0,0,0,0,0,0,1000,1000,0,0,0,0,0,0,0,0,0,0,0,0,0};
	double WindDistribution[]={12000,11500,10000,7500,1500,500,5000,7000,8000,6500,4500,3500,500,0,2500,8500,9500,8000,6500,4000,6000,7000,10000,11500}; //  163.5 kWprodotti in 24 ore, valore per ora casuali
	double power = 2000*WindDistribution[(istant+TimeZone+24)%24];
	//double power = 1000*WindDistribution[(istant+TimeZone+24)%24]; //per rete ITALYcon arrivi 80/s

return power; 
}


double WDMNetwork::BrownPower(int istant,int TimeZone)

{
	//int istant=ceil(time);
	//istant=istant%24; // per ora tempo di simulazione breve.. 
	double BrownDistribution[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // Watt prodotti in 24 ore, valore per ora casuali
	double power = BrownDistribution[(istant+TimeZone+24)%24];
   
return power;

}

//-----------------------------------------------------------------
//****************** UNPROTECTED_GREEN *******************
//-----------------------------------------------------------------
//*********** QUASI 1000 RIGHE DI CODICE!!! **********
LINK_COST WDMNetwork::UNPROTECTED_ComputeRouteGreen(Circuit& hCircuit,	//Mirko
										   NetMan *pNetMan,
										   OXCNode* pSrc, 
										   OXCNode* pDst)
{
	int k1 = 0;
	int algorithm_type = 0; // scelta algoritmo:        0--> Shortest Path // 1-->GEAR // 2-->SWEAR// 3-->BestGreenDC
	int WDM_type = 1; // scelta configurazione IPoWDM:   0-->OPACA // 1-->IP basic// 2-->SDH
	// obey fiber capacity
	//int k=0;
   
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	int M = 100;		//Big Number
	double En = 150;	//Electronic control power at each node
	double Es = 1.5;	//3DMems switch
	double Exy = 0;
	LINK_COST hBestCost = UNREACHABLE;

	switch(algorithm_type)
	{
		case 0: 
		{

// //------------------------------------ALGORITMO SHORTEST PATH--------------------------------------------
// //-------------------------------------------------------------------------------------------------------

		for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++) {
			pUniFiber = (UniFiber*)(*itr);
			AbstractNode *a=pUniFiber->getSrc();
			AbstractNode *b=pUniFiber->getDst();
			int sorg=a->getId();
			int dest=b->getId();
			pUniFiber->modifyCost(1);
			if (0 == pUniFiber->countFreeChannels()) 
			{
				pUniFiber->invalidate();
			}
			//stampa pesi sui link 			 				 
			//LINK_COST aaa =pUniFiber->getCost();//verifica restoreCost()
			//cout<<"fibra n."<<pUniFiber->getId()<<":" <<sorg<<"-"<<dest<<" cost: "<<aaa;
			//cin.get();
		} //chiusura for
		
		// calcolo rotta	
		list<AbsPath*> hPPathList1;
		this->YenGreen(hPPathList1, pSrc, pDst, 1, //K=1 !!! non avrebbe motivo d'essre diverso.. -> 1 solo percorso nella lista
						AbstractGraph::LCF_ByOriginalLinkCost);
		if (0 == hPPathList1.size()) {//cout<<"zzz ";
			validateAllLinks();
			//cout<<"\n----- PathList.Size = 0 ????? -----\n";
			return UNREACHABLE;
		}
		LINK_COST hBestCost1 = UNREACHABLE;
		AbsPath *pBestP1 = NULL;
		list<AbsPath*>::iterator itrPPath1;
		itrPPath1 = hPPathList1.begin();
		pBestP1 = (*itrPPath1);
		assert(pBestP1);

		// construct the circuit
		//-B: create the new Lightpath with Id = 0
		Lightpath_Seg *pLPSeg1 = new Lightpath_Seg(0);//LEAK!
		LINK_COST dummycost1=0;
		int Hop1=0;
		int DCusato1;
		int già_usato1=0;
		double EDFACost1=0;
		double NodeProcessingCost1=0;
        //cout<<"\nRotta1: ";
		for (itr = pBestP1->m_hLinkList.begin(); itr != pBestP1->m_hLinkList.end(); itr++)
		{
			pUniFiber = (UniFiber*)(*itr);
			pLPSeg1->m_hPRoute.push_back((UniFiber*)(*itr));
			Hop1++;
			if ((*itr)->getDst()->getId() == pNetMan->m_hWDMNet.DummyNode){
				dummycost1 = (*itr)->getCost();  //costo del link che collega il DC col dummynode			
			}
			//Stampa rotta
			//cout<<(*itr)->getSrc()->getId()<<"-";
			//stabilisce se EDFA è in uso o meno e quindi se il costo va considerato
			if ((*itr)->m_used!=1)
				già_usato1=1;
			else
				già_usato1=0;

			EDFACost1 = EDFACost1 + ceil((*itr)->getLength() / EDFAspan)*EDFApower*già_usato1;
			DCusato1 = (*itr)->getSrc()->getId();
	   	   //pNetMan->m_hWDMNet.DCused=DCusato1;
 		} 
		// cout<<pNetMan->m_hWDMNet.DummyNode;
		// cin.get();
		if (già_usato1==1) {            //è il "già_usato" dell'ultimo link del perorso, quello com la dest...
			EDFACost1=(EDFACost1-8)*pNetMan->m_hHoldingTimeIncoming;
		}
		else {
		  EDFACost1=EDFACost1*pNetMan->m_hHoldingTimeIncoming;
		}
		
		// scelta configurazione architettura IPoWDM
	  switch(WDM_type)
	  {
		case 0:  // configurazione OPACA
		{
			if (Hop1==1) {                // se il nodo sorgente è già vicino al DC, non contatno i WDM trasponders e conto solo una Short Reach interface + Optical Switch
				NodeProcessingCost1=(OpticalSwitchCost+ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
			}else {
				NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost)+Hop1*OpticalSwitchCost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; 
			}
			break;
		}
		case 1:  // configurazione IPbasic
		{
			if (Hop1==1) {              
				NodeProcessingCost1=(TransponderCost2+IPCost)*pNetMan->m_hHoldingTimeIncoming;
			}
			// NodeProcessingCost1=(TransponderCost2+IPCost)*pNetMan->m_hHoldingTimeIncoming; }
			else {
				NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost2)+(Hop1-1)*IPCost)*pNetMan->m_hHoldingTimeIncoming;
			}
			break;
		}
		case 2:  // configurazione SDH
		{
			if (Hop1==1) {              
				NodeProcessingCost1=(TransponderCost+DXCcost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
			}else {
				NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost)+Hop1*DXCcost+4*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		}
		break;	
	  } //-B: end switch(WDM_type)
		hBestCost1=EDFACost1+NodeProcessingCost1; // costo di amplificatori+trasporto-costo link dummynode
		//hBestCost=EDFACost+(HopCost-1)*TransportCost-dummycost;		 //  per energy-aware
		hBestCost=hBestCost1;
		hCircuit.addVirtualLink(pLPSeg1);
		pNetMan->m_hWDMNet.DCused=DCusato1;
		pNetMan->m_hWDMNet.EDFACost=EDFACost1;
	    pNetMan->m_hWDMNet.NodeProcessingCost=NodeProcessingCost1;
		// delete primary candidate
		for (itrPPath1=hPPathList1.begin(); itrPPath1!=hPPathList1.end(); itrPPath1++) {
			delete (*itrPPath1);
		}
		break;
	} //-B: end case 0 (shortest path) -> switch(algorithm_type)
	
	case 1: 
	{
	// //------------------------------------ALGORITMO	GEAR-----------------------------------------------------------------------
	//  //-------------------------------------------------------------------------------------------------------------------------
	  for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++)
	  {
		pUniFiber = (UniFiber*)(*itr);
		AbstractNode *a=pUniFiber->getSrc();
		AbstractNode *b=pUniFiber->getDst();
		int sorg=a->getId();
		int dest=b->getId();
		if (0 == pUniFiber->countFreeChannels()) 
		{
			pUniFiber->invalidate();
		}
		// LINK DELLA RETE
		if ((dest!=pNetMan->m_hWDMNet.DummyNode)&&(sorg!=pNetMan->m_hWDMNet.DummyNode))
		{	
			switch(WDM_type) // scelta configurazione architettura IPoWDM
			{ 
				case 0:  // configurazione OPACA
				{
					pUniFiber->modifyCost((2 * TransponderCost + OpticalSwitchCost)*pNetMan->m_hHoldingTimeIncoming);
					break;
				}
				case 1:  // configurazione IPbasic
				{
					pUniFiber->modifyCost((2*TransponderCost2+IPCost)*pNetMan->m_hHoldingTimeIncoming) ;
					break;
				}
				case 2:  // configurazione SDH
				{
					pUniFiber->modifyCost((2*TransponderCost+DXCcost)*pNetMan->m_hHoldingTimeIncoming);
					break;
				}
			}
			pUniFiber->modifyCost((2*TransponderCost2+IPCost)*pNetMan->m_hHoldingTimeIncoming) ;
		}
		// LINK GRAFO AUSILIARIO DI ANYCAST      
		if (dest==pNetMan->m_hWDMNet.DummyNode)
		{  
			int yy;
			for (yy=0;yy!=pNetMan->m_hWDMNet.nDC;yy++)
			{
				if (pNetMan->m_hWDMNet.DCvettALL[yy]==pUniFiber->getSrc()->getId())
				{
					//cout<<"\nresidual"<<pNetMan->m_hWDMNet.g_en_residual[yy];
					// cin.get();
					if ((pNetMan->m_hWDMNet.g_en_residual[yy]<ComputingCost*pNetMan->m_hHoldingTimeIncoming) && (pNetMan->m_hWDMNet.g_en_residual[yy]>0))
					{ //energia rinnovabile non sufficiente in parte
						//cout<<"\nNodo: "<<pNetMan->m_hWDMNet.DCvettALL[yy]<<" GP: "<<pNetMan->m_hWDMNet.gp[yy]; 
						pUniFiber->modifyCost(ComputingCost*pNetMan->m_hHoldingTimeIncoming-pNetMan->m_hWDMNet.g_en_residual[yy]);
					}
					if ((pNetMan->m_hWDMNet.g_en_residual[yy]<ComputingCost*pNetMan->m_hHoldingTimeIncoming) && (pNetMan->m_hWDMNet.g_en_residual[yy]<=0))
					{ //energia rinnovabile non sufficiente del tutto
						//cout<<"\nNodo: "<<pNetMan->m_hWDMNet.DCvettALL[yy]<<" GP: "<<pNetMan->m_hWDMNet.gp[yy]; 
						pUniFiber->modifyCost(ComputingCost*pNetMan->m_hHoldingTimeIncoming);
					}
					if ((pNetMan->m_hWDMNet.g_en_residual[yy]>ComputingCost*pNetMan->m_hHoldingTimeIncoming))
					{ //energia rinnovabile sufficiente
						//cout<<"\nNodo: "<<pNetMan->m_hWDMNet.DCvettALL[yy]<<" GP: "<<pNetMan->m_hWDMNet.gp[yy]; 
						pUniFiber->modifyCost(0);
					}
				}
			}
			//stampa pesi sui link 			 				 
			//LINK_COST aaa =pUniFiber->getCost();//verifica restoreCost()
			//cout<<"fibra n."<<pUniFiber->getId()<<":" <<sorg<<"-"<<dest<<" cost: "<<aaa;
			//cin.get();
		} 
	  } //chiusura for
		// calcolo rotta	
		list<AbsPath*> hPPathList1;
		this->YenGreen(hPPathList1, pSrc, pDst, 1, //K=1 !!! non avrebbe motivo d'essre diverso.. -> 1 solo path nella lista
					AbstractGraph::LCF_ByOriginalLinkCost);
		
		if (0 == hPPathList1.size())
		{
			//cout<<"zzz ";
			validateAllLinks();
			//cout<<"\n----- PathList.Size = 0 ????? -----\n";
			return UNREACHABLE;
		}
		LINK_COST hBestCost1 = UNREACHABLE;
		AbsPath *pBestP1 = NULL;
		list<AbsPath*>::iterator itrPPath1;
		itrPPath1=hPPathList1.begin();
		pBestP1=(*itrPPath1);
		assert(pBestP1);
		// construct the circuit
		Lightpath_Seg *pLPSeg1 = new Lightpath_Seg(0);//LEAK!
		LINK_COST dummycost1=0;
		int Hop1=0;
		int DCusato1;
		int già_usato1=0;
		double EDFACost1=0;
		double NodeProcessingCost1=0;
	    //cout<<"\nRotta1: ";
		for (itr=pBestP1->m_hLinkList.begin(); itr!=pBestP1->m_hLinkList.end();itr++)
		{
			pUniFiber = (UniFiber*)(*itr);
			pLPSeg1->m_hPRoute.push_back((UniFiber*)(*itr));
			Hop1++;
			if ((*itr)->getDst()->getId()== pNetMan->m_hWDMNet.DummyNode)
			{
				 dummycost1=(*itr)->getCost();  //costo del link che collega il DC col dummynode			
			}
			//Stampa rotta
			//cout<<(*itr)->getSrc()->getId()<<"-";
			//stabilisce se EDFA è in uso o meno e quindi se il costo va considerato
			if ((*itr)->m_used!=1)
				già_usato1=1;
			else
				già_usato1=0;
			EDFACost1=EDFACost1+ceil((*itr)->getLength()/EDFAspan)*EDFApower*già_usato1;		  
		   	DCusato1=(*itr)->getSrc()->getId();
	   		//pNetMan->m_hWDMNet.DCused=DCusato1; /////////////////////
		} 
        // cout<<pNetMan->m_hWDMNet.DummyNode;
	    // cin.get();
		if (già_usato1==1) //è il "già_usato" dell'ultimo link del perorso, quello com la dest...
		{
			EDFACost1=(EDFACost1-8)*pNetMan->m_hHoldingTimeIncoming;
		}else {
			EDFACost1=EDFACost1*pNetMan->m_hHoldingTimeIncoming;
		}

		switch(WDM_type) // scelta configurazione architettura IPoWDM
		{
			case 0:  // configurazione OPACA
			{
				if (Hop1==1) {                // se il nodo sorgente è già vicino al DC, non contatno i WDM trasponders e conto solo una Short Reach interface + Optical Switch
					NodeProcessingCost1=(OpticalSwitchCost+ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
				}else {
					NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost)+Hop1*OpticalSwitchCost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
				}
				break;
			}
			case 1:  // configurazione IPbasic
			{
				if (Hop1==1){              
					NodeProcessingCost1=(TransponderCost2)*pNetMan->m_hHoldingTimeIncoming;
				}else{
					NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost2)+(Hop1-1)*IPCost)*pNetMan->m_hHoldingTimeIncoming;
				}
				break;
			}
			case 2:  // configurazione SDH
			{
				if (Hop1==1) {              
					NodeProcessingCost1=(TransponderCost+DXCcost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
				}else{
					NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost)+Hop1*DXCcost+4*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
				}
				break;
			}
		}
		hBestCost1=EDFACost1+NodeProcessingCost1; // costo di amplificatori+trasporto-costo link dummynode
		//hBestCost=EDFACost+(HopCost-1)*TransportCost-dummycost;		 //  per energy-aware 
		hBestCost=hBestCost1;
		hCircuit.addVirtualLink(pLPSeg1);
		pNetMan->m_hWDMNet.DCused=DCusato1; 
		pNetMan->m_hWDMNet.EDFACost=EDFACost1;
		pNetMan->m_hWDMNet.NodeProcessingCost=NodeProcessingCost1;
		// delete primary candidate
		for (itrPPath1=hPPathList1.begin(); itrPPath1!=hPPathList1.end(); itrPPath1++) {
			delete (*itrPPath1);
		}
		break;
	} //-B: end case 1 -> switch(algorithm_type)

//*************************** INDENTATO CORRETTAMENTE FINO A QUI *******************************
	case 2:
	{

	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		AbstractNode *a=pUniFiber->getSrc();
		AbstractNode *b=pUniFiber->getDst();
		int sorg=a->getId();
		int dest=b->getId();

//  //---------------------------------------ALGORITMO SWEAR-------------------------------------------------------------------
//  //-------------------------------------------------------------------------------------------------------------------------

	//LINK DELLA RETE	

		double T=0.625; //Load Threshold -->circa 10 utilizzate
		double Wxy=pUniFiber->countFreeChannels();  // Free WaveLnghts
		double W=pUniFiber->m_nW; // Total WaveLenghts
		double alfa=10;		   //
		double beta=1;
		double Lxy=(W-Wxy)/W;  // Link Load --> Wavelengths utilizzate
        double axy=ceil(pUniFiber->getLength()/EDFAspan)*EDFApower; // EDFA cost

				if (pUniFiber->m_used!=1) {
					//cout<<"link:"<<sorg<<"-"<<dest<<"non usato !";
					//cin.get();
				 Exy=axy+Es; }  //Exy=axy+En+Es;
				else  {
					//cout<<"link:"<<sorg<<"-"<<dest<<"usato !";
					//cin.get();
					Exy=Es; }
			  //}
	if ((dest!=pNetMan->m_hWDMNet.DummyNode)&&(sorg!=pNetMan->m_hWDMNet.DummyNode)){
			if (Lxy>T) {
				pUniFiber->modifyCost(alfa*Lxy) ;}
			else {
		        pUniFiber->modifyCost(1) ; }
		//LINK_COST aaa =pUniFiber->getCost();//verifica restoreCost()
		//cout<<"fibra n."<<pUniFiber->getId()<<":" <<sorg<<"-"<<dest<<" cost: "<<aaa;
		//cin.get();
	}
// LINK GRAFO AUSILIARIO DI ANYCAST  

		//if ((sorg!=pNetMan->m_hWDMNet.BestGreenNode) && (dest==pNetMan->m_hWDMNet.DummyNode)){  // solo link tra DC con meno energia e dummynode
		if (dest==pNetMan->m_hWDMNet.DummyNode){  //solo link tra DC e dummynode	
		int yy;
        for (yy=0;yy!=pNetMan->m_hWDMNet.nDC;yy++) {
			 if (pNetMan->m_hWDMNet.DCvettALL[yy]==pUniFiber->getSrc()->getId()) {
				 //cout<<"\nresidual"<<pNetMan->m_hWDMNet.g_en_residual[yy];
				// cin.get();
				if ((pNetMan->m_hWDMNet.g_en_residual[yy])<ComputingCost*pNetMan->m_hHoldingTimeIncoming) { //se l'energia rinnovabile di quel nodo è minore di ..
					//cout<<"\nNodo: "<<pNetMan->m_hWDMNet.DCvettALL[yy]<<" GP: "<<pNetMan->m_hWDMNet.gp[yy]; 
					//pUniFiber->modifyCost(LARGE_COST); }
					pUniFiber->modifyCost(10); }
				 else {
					 double Dxy=(pNetMan->m_hWDMNet.gp[yy]-pNetMan->m_hWDMNet.g_en_residual[yy])/pNetMan->m_hWDMNet.gp[yy]; //energia utilizzata su energia disponibile --> più ne ho utilizzata maggiore è il costo del link
					 // cout<<"\nNodo: "<<pNetMan->m_hWDMNet.DCvettALL[yy]<<" GP: "<<pNetMan->m_hWDMNet.gp[yy]; 
					 // cout<<"\nNodo: "<<pNetMan->m_hWDMNet.DCvettALL[yy]<<" GPres: "<<pNetMan->m_hWDMNet.g_en_residual[yy];
					 //cout<<"\ncosto: "<<Dxy;
					 //cin.get();
					 //pUniFiber->modifyCost(0); }
					 pUniFiber->modifyCost(beta*Dxy); }}
					 //cout<<"\ncosto1: "<<(pNetMan->m_hWDMNet.gp[yy]-pNetMan->m_hWDMNet.g_en_residual[yy])/pNetMan->m_hWDMNet.gp[yy];
				
//cout<<"Link "<<sorg<<"-"<<pNetMan->m_hWDMNet.DummyNode<<" cambia costo\n";
	    	}
		}

					
		if (0 == pUniFiber->countFreeChannels()) 
		{
			pUniFiber->invalidate();
		}


	} // chiusura for

	// calcolo rotta
	//percorso 1-------------------------------------------------------------------------------

	list<AbsPath*> hPPathList1;
	this->YenGreen(hPPathList1, pSrc, pDst, 1, //K=1 !!! non avrebbe motivo d'essre diverso..
				AbstractGraph::LCF_ByOriginalLinkCost);

	
	if (0 == hPPathList1.size()) {//cout<<"zzz ";
		validateAllLinks();
		//cout<<"\n----- PathList.Size = 0 ????? -----\n";
		return UNREACHABLE;
}
	
	LINK_COST hBestCost1 = UNREACHABLE;
	AbsPath *pBestP1 = NULL;
	list<AbsPath*>::iterator itrPPath1;

	itrPPath1=hPPathList1.begin();
	pBestP1=(*itrPPath1);
		assert(pBestP1);
		// construct the circuit
		Lightpath_Seg *pLPSeg1 = new Lightpath_Seg(0);//LEAK!
		LINK_COST dummycost1=0;
		int Hop1=0;
		int DCusato1;
		int già_usato1=0;
		double EDFACost1=0;
		double NodeProcessingCost1=0;

        //cout<<"\nRotta1: ";
		for (itr=pBestP1->m_hLinkList.begin(); itr!=pBestP1->m_hLinkList.end();itr++) {
			pUniFiber = (UniFiber*)(*itr);
			pLPSeg1->m_hPRoute.push_back((UniFiber*)(*itr));
			Hop1++;
			if ((*itr)->getDst()->getId()== pNetMan->m_hWDMNet.DummyNode){
				 dummycost1=(*itr)->getCost();  //costo del link che collega il DC col dummynode			
				}
		   //Stampa rotta
		   //cout<<(*itr)->getSrc()->getId()<<"-";

		   //stabilisce se EDFA è in uso o meno e quindi se il costo va considerato
		   if ((*itr)->m_used!=1)
			   già_usato1=1;
		   else
			   già_usato1=0;

		   EDFACost1=EDFACost1+ceil((*itr)->getLength()/EDFAspan)*EDFApower*già_usato1;		  
		   
		   DCusato1=(*itr)->getSrc()->getId();
	   	   //pNetMan->m_hWDMNet.DCused=DCusato1; /////////////////////
		 		} 
           // cout<<pNetMan->m_hWDMNet.DummyNode;
	      // cin.get();

		if (già_usato1==1) {            //è il "già_usato" dell'ultimo link del perorso, quello com la dest...
		  EDFACost1=(EDFACost1-8)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  EDFACost1=EDFACost1*pNetMan->m_hHoldingTimeIncoming; }

		switch(WDM_type) { // scelta configurazione architettura IPoWDM

case 0:  // configurazione OPACA
		{
		if (Hop1==1) {                // se il nodo sorgente è già vicino al DC, non contatno i WDM trasponders e conto solo una Short Reach interface + Optical Switch
		  NodeProcessingCost1=(OpticalSwitchCost+ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost)+Hop1*OpticalSwitchCost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;
		}
case 1:  // configurazione IPbasic
		{
			if (Hop1==1) {              
		  NodeProcessingCost1=(TransponderCost2)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost2)+(Hop1-1)*IPCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;
		}
case 2:  // configurazione SDH
		{
			if (Hop1==1) {              
		  NodeProcessingCost1=(TransponderCost+DXCcost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost)+Hop1*DXCcost+4*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;
		}
	}

		hBestCost1=EDFACost1+NodeProcessingCost1; // costo di amplificatori+trasporto-costo link dummynode
		//hBestCost=EDFACost+(HopCost-1)*TransportCost-dummycost;		 //  per energy-aware 
		
		hBestCost1=EDFACost1+NodeProcessingCost1; // costo di amplificatori+trasporto-costo link dummynode
		//hBestCost=EDFACost+(HopCost-1)*TransportCost-dummycost;		 //  per energy-aware 

//-----------------------------------------------------------------------------
		// modifica pesi sui link per confrontare il percorso calcolato
	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		pUniFiber->modifyCost(1);
}
//-----------------------------------------------------------------------------
 //percorso 2
    list<AbsPath*> hPPathList2;
	this->YenGreen(hPPathList2, pSrc, pDst, 1, //K=1 !!! non avrebbe motivo d'essre diverso..
				AbstractGraph::LCF_ByOriginalLinkCost);

	
	if (0 == hPPathList2.size()) {//cout<<"zzz ";
		validateAllLinks();
		//cout<<"\n----- PathList.Size = 0 ????? -----\n";
		return UNREACHABLE; }
	
	LINK_COST hBestCost2 = UNREACHABLE;
	AbsPath *pBestP2 = NULL;
	list<AbsPath*>::iterator itrPPath2;
	itrPPath2=hPPathList2.begin();
	pBestP2=(*itrPPath2);
		assert(pBestP2);
		// construct the circuit
		Lightpath_Seg *pLPSeg2 = new Lightpath_Seg(0);//LEAK!
		LINK_COST dummycost2=0;
		int Hop2=0;
		int DCusato2;
		int già_usato2=0;
		double EDFACost2=0;
		double NodeProcessingCost2=0;
			

        //cout<<"\nRotta2: ";
		for (itr=pBestP2->m_hLinkList.begin(); itr!=pBestP2->m_hLinkList.end();itr++) {
			pUniFiber = (UniFiber*)(*itr);
			pLPSeg2->m_hPRoute.push_back((UniFiber*)(*itr));
			Hop2++;
			if ((*itr)->getDst()->getId()== pNetMan->m_hWDMNet.DummyNode){
				 dummycost2=(*itr)->getCost();  //costo del link che collega il DC col dummynode			
				}
		   //Stampa rotta
		   //cout<<(*itr)->getSrc()->getId()<<"-";

		   //stabilisce se EDFA è in uso o meno e quindi se il costo va considerato
		   if ((*itr)->m_used!=1)
			   già_usato2=1;
		   else
			   già_usato2=0;
			
		   EDFACost2=EDFACost2+ceil((*itr)->getLength()/EDFAspan)*EDFApower*già_usato2;		  
		   
		   DCusato2=(*itr)->getSrc()->getId();
	   	   //pNetMan->m_hWDMNet.DCused=DCusato2;  ///////////////////////////
		 		} 
		  //cout<<pNetMan->m_hWDMNet.DummyNode;
	      //cin.get();

		if (già_usato2==1) {            //è il "già_usato" dell'ultimo link del perorso, quello com la dest...
		  EDFACost2=(EDFACost2-8)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  EDFACost2=EDFACost2*pNetMan->m_hHoldingTimeIncoming; }

		switch(WDM_type) { // scelta configurazione architettura IPoWDM

case 0:  // configurazione OPACA
		{
		if (Hop2==1) {                // se il nodo sorgente è già vicino al DC, non contatno i WDM trasponders e conto solo una Short Reach interface + Optical Switch
		  NodeProcessingCost2=(OpticalSwitchCost+ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost2=(2*(Hop2-1)*(TransponderCost)+Hop2*OpticalSwitchCost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;
		}
case 1:  // configurazione IPbasic
		{
			if (Hop2==1) {              
		  NodeProcessingCost2=(TransponderCost2)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost2=(2*(Hop2-1)*(TransponderCost2)+(Hop2-1)*IPCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;
		}
case 2:  // configurazione SDH
		{
			if (Hop2==1) {              
		  NodeProcessingCost2=(TransponderCost+DXCcost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost2=(2*(Hop2-1)*(TransponderCost)+Hop2*DXCcost+4*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;
		}
	}

		hBestCost2=EDFACost2+NodeProcessingCost2; // costo di amplificatori+trasporto-costo link dummynode


		double NP=NodeProcessingCost1-NodeProcessingCost2;
		//cout<<"diff"<<NodeProcessingCost1-NodeProcessingCost2;
		//cin.get();
	
if (NP<=ComputingCost*pNetMan->m_hHoldingTimeIncoming) {

	hBestCost=hBestCost1;
	hCircuit.addVirtualLink(pLPSeg1);
	delete(pLPSeg2);
	pNetMan->m_hWDMNet.DCused=DCusato1; /////////////////////
	pNetMan->m_hWDMNet.EDFACost=EDFACost1;
    pNetMan->m_hWDMNet.NodeProcessingCost=NodeProcessingCost1;

}
else {

	 hBestCost=hBestCost2;
     hCircuit.addVirtualLink(pLPSeg2);
	 delete(pLPSeg1);
	 pNetMan->m_hWDMNet.DCused=DCusato2; /////////////////////
	 pNetMan->m_hWDMNet.EDFACost=EDFACost2;
	 pNetMan->m_hWDMNet.NodeProcessingCost=NodeProcessingCost2;
}


	// delete primary candidate
	for (itrPPath1=hPPathList1.begin(); itrPPath1!=hPPathList1.end(); itrPPath1++) {
		delete (*itrPPath1);
	}

		// delete primary candidate
	for (itrPPath2=hPPathList2.begin(); itrPPath2!=hPPathList2.end(); itrPPath2++) {
		delete (*itrPPath2);
	}

	break;
	}  // chiusura case 2 -> switch algorithm_type
	

case 3: 
		{
//   //-----------------------------------ALGORITMO BEST GREEN DC--------------------------------------------------------------
//   //------------------------------------------------------------------------------------------------------------------------

	for (itr=m_hLinkList.begin(); itr!=m_hLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		AbstractNode *a=pUniFiber->getSrc();
		AbstractNode *b=pUniFiber->getDst();
		int sorg=a->getId();
		int dest=b->getId();

	//LINK DEI DATA CENTER
	if ((sorg!=pNetMan->m_hWDMNet.BestGreenNode) && (dest==pNetMan->m_hWDMNet.DummyNode)){  // solo link tra DC con meno energia e dummynode
					pUniFiber->modifyCost(LARGE_COST); }

				
		if (0 == pUniFiber->countFreeChannels()) 
		{
			pUniFiber->invalidate();
		}

	} // chiusura for

	// calcolo rotta
		list<AbsPath*> hPPathList1;
	this->YenGreen(hPPathList1, pSrc, pDst, 1, //K=1 !!! non avrebbe motivo d'essre diverso..
				AbstractGraph::LCF_ByOriginalLinkCost);

	if (0 == hPPathList1.size()) {//cout<<"zzz ";
		validateAllLinks();
		//cout<<"\n----- PathList.Size = 0 ????? -----\n";
		return UNREACHABLE;
}
	
	LINK_COST hBestCost1 = UNREACHABLE;
	AbsPath *pBestP1 = NULL;
	list<AbsPath*>::iterator itrPPath1;

	itrPPath1=hPPathList1.begin();
	pBestP1=(*itrPPath1);
		assert(pBestP1);
		// construct the circuit
		Lightpath_Seg *pLPSeg1 = new Lightpath_Seg(0);//LEAK!
		LINK_COST dummycost1=0;
		int Hop1=0;
		int DCusato1;
		int già_usato1=0;
		double EDFACost1=0;
		double NodeProcessingCost1=0;

        //cout<<"\nRotta1: ";
		for (itr=pBestP1->m_hLinkList.begin(); itr!=pBestP1->m_hLinkList.end();itr++) {
			pUniFiber = (UniFiber*)(*itr);
			pLPSeg1->m_hPRoute.push_back((UniFiber*)(*itr));
			Hop1++;
			if ((*itr)->getDst()->getId()== pNetMan->m_hWDMNet.DummyNode){
				 dummycost1=(*itr)->getCost();  //costo del link che collega il DC col dummynode			
				}
		   //Stampa rotta
		   //cout<<(*itr)->getSrc()->getId()<<"-";

		   //stabilisce se EDFA è in uso o meno e quindi se il costo va considerato
		   if ((*itr)->m_used!=1)
			   già_usato1=1;
		   else
			   già_usato1=0;

		   EDFACost1=EDFACost1+ceil((*itr)->getLength()/EDFAspan)*EDFApower*già_usato1;		  
		   
		   DCusato1=(*itr)->getSrc()->getId();
	   	   //pNetMan->m_hWDMNet.DCused=DCusato1; /////////////////////
		 		} 
           // cout<<pNetMan->m_hWDMNet.DummyNode;
	      // cin.get();

		if (già_usato1==1) {            //è il "già_usato" dell'ultimo link del perorso, quello com la dest...
		  EDFACost1=(EDFACost1-8)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  EDFACost1=EDFACost1*pNetMan->m_hHoldingTimeIncoming; }

		switch(WDM_type) { // scelta configurazione architettura IPoWDM

case 0:  // configurazione OPACA
		{
		if (Hop1==1) {                // se il nodo sorgente è già vicino al DC, non contatno i WDM trasponders e conto solo una Short Reach interface + Optical Switch
		  NodeProcessingCost1=(OpticalSwitchCost+ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost)+Hop1*OpticalSwitchCost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;	
		}
case 1:  // configurazione IPbasic
		{
			if (Hop1==1) {              
		  NodeProcessingCost1=(TransponderCost2)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost2)+(Hop1-1)*IPCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;	
		}
case 2:  // configurazione SDH
		{
			if (Hop1==1) {              
		  NodeProcessingCost1=(TransponderCost+DXCcost+2*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		else {
		  NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost)+Hop1*DXCcost+4*ShortReachCost)*pNetMan->m_hHoldingTimeIncoming; }
		break;	
		}
	}


		if (Hop1==1) {              
     NodeProcessingCost1=(TransponderCost2+IPCost)*pNetMan->m_hHoldingTimeIncoming; }
	else {
	  NodeProcessingCost1=(2*(Hop1-1)*(TransponderCost2)+(Hop1-1)*IPCost)*pNetMan->m_hHoldingTimeIncoming; }

		hBestCost1=EDFACost1+NodeProcessingCost1; // costo di amplificatori+trasporto-costo link dummynode
		//hBestCost=EDFACost+(HopCost-1)*TransportCost-dummycost;		 //  per energy-aware 

hBestCost = hBestCost1;
hCircuit.addVirtualLink(pLPSeg1);

pNetMan->m_hWDMNet.DCused = DCusato1; /////////////////////
pNetMan->m_hWDMNet.EDFACost = EDFACost1;
pNetMan->m_hWDMNet.NodeProcessingCost = NodeProcessingCost1;

// delete primary candidate
for (itrPPath1 = hPPathList1.begin(); itrPPath1 != hPPathList1.end(); itrPPath1++) {
	delete (*itrPPath1);
}

break;
//  //-----------------------------------------------------------------------------------------------------------------------------------------------
} //-B: end case 3 -> switch(algorithm_type)

  default:
	  DEFAULT_SWITCH;
} // end switch(algorithm_type)

//	if (pNetMan->m_hLog.m_nProvisionedCon>17000) {
//	    cout<<"\n h: "<<pNetMan->m_hLog.m_nPHopDistance;
//		cout<<"\n c: "<<pNetMan->m_hLog.m_nProvisionedCon;
//		cin.get();
//	}
		// 	       UniFiber *pUF2;
//                 list<AbstractLink*>::const_iterator itrUF2;
//                 for (itrUF2=m_hLinkList.begin(); itrUF2!=m_hLinkList.end(); itrUF2++) {
// 		 pUF2 = (UniFiber*)(*itrUF2);
//                  assert(pUniFiber);
// 		 pUF2->dump(cout);
//                 // cerr <<  pUF-> m_hLinkId << endl; 
//                 // cerr <<  pUF-> m_hCost << endl; 
// 		}
	// compute upto k primary candidates
	////stampa costo percorso
	//cout<<"\nPathCost: "<<hBestCost1<<" [Watt]";
	//cout<<"\n    EDFA: "<<EDFACost1<<" [Watt]";
	//cout<<"\n    PROCESSING NODO: "<<pNetMan->m_hWDMNet.NodeProcessingCost<<" [Watt]"; 
	//cin.get();
	//

return hBestCost;
}

//-B: so far, it is is a simply copy paste of a small part of UNPROTECTED_ComputeRouteGreen with some modifications
LINK_COST WDMNetwork::BBU_ComputeRoute(Circuit*hCircuit, NetMan *pNetMan, OXCNode* pSrc, OXCNode* pDst, int algorithm_type)
{
	int k1 = 0;
	int WDM_type = 1; // scelta configurazione IPoWDM:   0-->OPACA // 1-->IP basic// 2-->SDH
					  // obey fiber capacity
					  //int k=0;

	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	int M = 100;		//Big Number
	double En = 150;	//Electronic control power at each node
	double Es = 1.5;	//3DMems switch
	double Exy = 0;
	LINK_COST hBestCost = UNREACHABLE;

	switch (algorithm_type)
	{
	case 0:
	{

		// //------------------------------------ALGORITMO SHORTEST PATH--------------------------------------------
		// //-------------------------------------------------------------------------------------------------------

		for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
		{
			pUniFiber = (UniFiber*)(*itr);
			AbstractNode *a = pUniFiber->getSrc();
			AbstractNode *b = pUniFiber->getDst();
			int sorg = a->getId();
			int dest = b->getId();
			
			//-B: FIBER COST: set to 1 the var m_hCost and save the previous value in m_hCostSaved --> WHY???? commented!
			//pUniFiber->modifyCost(1);
			
			if (0 == pUniFiber->countFreeChannels())
			{
				pUniFiber->invalidate();
				cout << "\tNO FREE CHANNELS ON FIBER " << pUniFiber->getId() << "!!!!!!!!!!";
			}
			//stampa pesi sui link 			 				 
			//LINK_COST aaa = pUniFiber->getCost();//verifica restoreCost()
			//cout<<"fibra n."<<pUniFiber->getId()<<":" <<sorg<<"-"<<dest<<" cost: "<<aaa;
			//cin.get();
		} //chiusura for

		  // calcolo rotta	
		list<AbsPath*> hPPathList1;
		this->Yen(hPPathList1, pSrc, pDst, 1, //K=1 !!! non avrebbe motivo d'essere diverso.. -> 1 solo percorso nella lista
			AbstractGraph::LCF_ByOriginalLinkCost);
		if (0 == hPPathList1.size()) {//cout<<"zzz ";
			validateAllLinks();
			cout<<" ----- PathList.Size = 0 ????? -----";
			hBestCost = UNREACHABLE;
			return hBestCost;
		}

#ifdef DEBUGB
		/*
		list<AbsPath*>::iterator itrBestPath;
		itrBestPath = hPPathList1.begin();
		UniFiber *uniFiber = (UniFiber*)(*itrBestPath);
		SimplexLink *uniLink = (SimplexLink*)(*itrBestPath);
		cout << "\nYENGREEN DONE - LINKLIST DUMP\nWavelAssigned: " << (*itrBestPath)->wlAssigned;
		for (itr = (*itrBestPath)->m_hLinkList.begin(); itr != (*itrBestPath)->m_hLinkList.end(); itr++)
		{
			cout << "\nLinkID: " << uniFiber->m_nLinkId;
			cout << "\tLength: " << uniFiber->m_nLength;
			cout << "\tUsed: " << uniFiber->m_used;
			cout << "\tNum channel on fiber: " << uniFiber->m_nW;
			cout << "\tLink type: " << uniLink->m_eSimplexLinkType;
			cout << "\tLink cap: " << uniLink->m_hFreeCap;
			cout << "\tBandw to be allocated: " << uniLink->m_nBWToBeAllocated << endl;
		}
		*/
#endif // DEBUGB

		LINK_COST hBestCost1 = UNREACHABLE;
		AbsPath *pBestP1 = NULL;
		list<AbsPath*>::iterator itrPPath1;
		itrPPath1 = hPPathList1.begin();
		pBestP1 = (*itrPPath1);
		assert(pBestP1);

		//-B: assegno la capacità al lightpath prendendola da quella del canale. Essendo un nuovo lightpath la capacità è quella totale
		itr = pBestP1->m_hLinkList.begin();
		pUniFiber = (UniFiber*)(*itr);
		int LPCap = pUniFiber->m_pChannel->m_nCapacity;
		//Lightpath::LPProtectionType pType;
		// construct the circuit
		//-B: create the new Lightpath with Id = 0 and ProtectionType = LPT_PAL_Unprotected
		Lightpath_Seg *pLPSeg1 = new Lightpath_Seg(0, Lightpath::LPProtectionType::LPT_PAL_Unprotected, LPCap);
		//cout << "Lightpath: CAP = " << pLPSeg1->m_nCapacity << " FREECAP = " << pLPSeg1->m_nFreeCapacity << " PT = " << pLPSeg1->m_eProtectionType << " ********************" << endl;

		LINK_COST dummycost1 = 0;
		int Hop1 = 0;
		int DCusato1;
		int già_usato1 = 0;
		double EDFACost1 = 0;
		double NodeProcessingCost1 = 0;
		//cout<<"\nRotta1: ";
		for (itr = pBestP1->m_hLinkList.begin(); itr != pBestP1->m_hLinkList.end(); itr++) {
			pUniFiber = (UniFiber*)(*itr);
			pLPSeg1->m_hPRoute.push_back((UniFiber*)(*itr));
			Hop1++;
			if ((*itr)->getDst()->getId() == pNetMan->m_hWDMNet.DummyNode) {
				dummycost1 = (*itr)->getCost();  //costo del link che collega il DC col dummynode			
			}
			//Stampa rotta
			//cout<<(*itr)->getSrc()->getId()<<"-";
			//stabilisce se EDFA è in uso o meno e quindi se il costo va considerato
			if ((*itr)->m_used != 1)
				già_usato1 = 1;
			else
				già_usato1 = 0;

			EDFACost1 = EDFACost1 + ceil((*itr)->getLength() / EDFAspan)*EDFApower*già_usato1;
			DCusato1 = (*itr)->getSrc()->getId();
			//pNetMan->m_hWDMNet.DCused=DCusato1;
		}
		// cout<<pNetMan->m_hWDMNet.DummyNode;
		// cin.get();
		if (già_usato1 == 1) {            //è il "già_usato" dell'ultimo link del perorso, quello com la dest...
			EDFACost1 = (EDFACost1 - 8)*pNetMan->m_hHoldingTimeIncoming;
		}
		else {
			EDFACost1 = EDFACost1*pNetMan->m_hHoldingTimeIncoming;
		}

		// scelta configurazione architettura IPoWDM
		switch (WDM_type)
		{
		case 0:  // configurazione OPACA
		{
			if (Hop1 == 1) {	// se il nodo sorgente è già vicino al DC, non contatno i WDM trasponders e conto solo una Short Reach interface + Optical Switch
				NodeProcessingCost1 = (OpticalSwitchCost + ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
			}
			else {
				NodeProcessingCost1 = (2 * (Hop1 - 1)*(TransponderCost)+Hop1*OpticalSwitchCost + 2 * ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
			}
			break;
		}
		case 1:  // configurazione IPbasic
		{
			if (Hop1 == 1) {
				NodeProcessingCost1 = (TransponderCost2 + IPCost)*pNetMan->m_hHoldingTimeIncoming;
			}
			// NodeProcessingCost1=(TransponderCost2+IPCost)*pNetMan->m_hHoldingTimeIncoming; }
			else {
				NodeProcessingCost1 = (2 * (Hop1 - 1)*(TransponderCost2)+(Hop1 - 1)*IPCost)*pNetMan->m_hHoldingTimeIncoming;
			}
			break;
		}
		case 2:  // configurazione SDH
		{
			if (Hop1 == 1) {
				NodeProcessingCost1 = (TransponderCost + DXCcost + 2 * ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
			}
			else {
				NodeProcessingCost1 = (2 * (Hop1 - 1)*(TransponderCost)+Hop1*DXCcost + 4 * ShortReachCost)*pNetMan->m_hHoldingTimeIncoming;
			}
		}
		break;
		} //-B: end switch(WDM_type)

		//-B: compute lightpath's cost
		hBestCost1 = EDFACost1 + NodeProcessingCost1; // costo di amplificatori + trasporto - costo link dummynode
													  //hBestCost = EDFACost + (HopCost-1) * TransportCost - dummycost;		 //  per energy-aware
		hBestCost = hBestCost1;

		//-B: save lightpath's cost
		pLPSeg1->m_hCost = hBestCost;

		//-B: add lightpath to hCircuit's m_hRoute list
		hCircuit->addVirtualLink(pLPSeg1);
		
		pNetMan->m_hWDMNet.DCused = DCusato1;
		pNetMan->m_hWDMNet.EDFACost = EDFACost1;
		pNetMan->m_hWDMNet.NodeProcessingCost = NodeProcessingCost1;
		// delete primary candidate
		for (itrPPath1 = hPPathList1.begin(); itrPPath1 != hPPathList1.end(); itrPPath1++) {
			delete (*itrPPath1);
		}
		break;
	} //-B: end case 0 (shortest path) -> switch(algorithm_type)

	default:
		DEFAULT_SWITCH;
	
} // end switch(algorithm_type)

  //	if (pNetMan->m_hLog.m_nProvisionedCon>17000) {
  //	    cout<<"\n h: "<<pNetMan->m_hLog.m_nPHopDistance;
  //		cout<<"\n c: "<<pNetMan->m_hLog.m_nProvisionedCon;
  //		cin.get();
  //	}
  // 	       UniFiber *pUF2;
  //                 list<AbstractLink*>::const_iterator itrUF2;
  //                 for (itrUF2=m_hLinkList.begin(); itrUF2!=m_hLinkList.end(); itrUF2++) {
  // 		 pUF2 = (UniFiber*)(*itrUF2);
  //                  assert(pUniFiber);
  // 		 pUF2->dump(cout);
  //                 // cerr <<  pUF-> m_hLinkId << endl; 
  //                 // cerr <<  pUF-> m_hCost << endl; 
  // 		}
  // compute upto k primary candidates
  ////stampa costo percorso
  //cout<<"\nPathCost: "<<hBestCost1<<" [Watt]";
  //cout<<"\n    EDFA: "<<EDFACost1<<" [Watt]";
  //cout<<"\n    PROCESSING NODO: "<<pNetMan->m_hWDMNet.NodeProcessingCost<<" [Watt]"; 
  //cin.get();
  //

	return hBestCost;
}

//****************** wpBBU *******************
//-B: so far, it's a simply copy paste of wpUNPROTECTED_ComputeRoute
//	return the cost of the best path found connecting source and destination
//	and add it to the m_hRoute Lightpath list, attribute of a Circuit object
LINK_COST WDMNetwork::wpBBU_ComputeRoute(Circuit*hCircuit, NetMan *pNetMan, OXCNode* pSrc, OXCNode* pDst)
{
	AbsPath* pBestP = new AbsPath(); //leak!
									 //comincia yen....
									 //list<AbsPath*> listA;//percorsi definitivi
									 //list<AbsPath*> listB;// percorsi parziali
									 //listA.push_back(path);
									 // numero k percorsi: ce l'ho in pNetMan
	
	//-B: use of 'myAlg' to compute paths -> how many? to be debugged or look into pNetMan object (I guess 1 path)
	LINK_COST hBestCost = myAlg(*pBestP, pSrc, pDst, numberOfChannels);
	
	// -B: if we would have use YenWP's algorithm to compute the shortest path, we should have done something similar (it doesn't work, throw an error):
	/*
	list<AbsPath*> hPPathList1;
	YenWP(hPPathList1, pSrc, pDst, 1, AbstractGraph::LCF_ByOriginalLinkCost);
	LINK_COST hBestCost = (*hPPathList1.begin())->calculateCost();
	*/

	//-B: pBestP points to the best path
	// caso unprotected
	if (UNREACHABLE != hBestCost)
	{
		list<AbstractLink*>::const_iterator itr;
		itr = pBestP->m_hLinkList.begin();

		//-B: create the new Lightpath with Id = 0 and ProtectionType = LPT_PAL_Unprotected and capacity equal to channel's cap
		//	Lightpath_Seg is a class derived from LighPath, that is a class derived from AbstractLink
		// construct the circuit
		Lightpath *pLPSeg = new Lightpath(0, Lightpath::LPProtectionType::LPT_PAL_Unprotected);
		//-B: for each link of the pBestP path
		for (itr = pBestP->m_hLinkList.begin(); itr != pBestP->m_hLinkList.end(); itr++)
		{
			//-B: m_hPRoute is a list of UniFiber object in the class Lightpath; itr is a AbstractLink object reference;
			//	Unifiber class (as well as Lightpath) is derived from AbstractLink
			pLPSeg->m_hPRoute.push_back((UniFiber*)(*itr));
		}
		
		//-B: DECIDO SE USARE UN NUOVO LIGHTPATH O CONSUMARE LA BANDA LIBERA DI UNO GIA' ESISTENTE
		/*
		if (!checkPathExist(pNetMan, pLPSeg)) //-B: if there is no path along this route, we have created a new one
		{
			//-B: the lightpath will use the same wavel/channel assigned to pBestP (the assignment has been done randomly
			//	among the available wavel, by the randomChSelection method, belonging to the AbstractGraph class)
			pLPSeg->wlAssigned = pBestP->wlAssigned;
		}
		else //-B: ---------------************** LEGGERE!!!!!! *****************------------------
			 //	if there is an already existing lightpath between those source and destination with enough free capacity, we'll consume its free bandwidth
			 //	questo tipo di consumo di banda va bene però solo per lightapth che sono esattamente coincidenti.
			 //	Se invece avessimo un lightpath che per es. fa 2->3->4 e uno che fa 1->2->3->4->5 non funziona
		{
			list<Lightpath*>::iterator itr;
			itr = hCircuit->m_hRoute.begin();
			pLPSeg->m_nLinkId++; //-B: I've already incremented LinkId of the other lightpath in checkPathExist method (see above)
			pLPSeg->m_nBWToBeAllocated = hCircuit->m_eBW; //valore utilizzato nel metodo consume all'interno del metodo Lightpath.BBU_WPsetUp
			//(*itr)->m_nLinkId++;
			//pLPSeg->m_nLinkId = (*itr)->m_nLinkId;
			cout << "\nBw to be allocated " << pLPSeg->m_nBWToBeAllocated << endl;
		}*/

		pLPSeg->wlAssigned = pBestP->wlAssigned;
		pLPSeg->m_nBWToBeAllocated = hCircuit->m_eBW;
		pLPSeg->m_hCost = pBestP->getCost();
		//assert(pLPSeg->m_hCost == pBestP->getCost());
		//-B: add it to the m_hRoute Lightpath list, attribute of a Circuit object
		hCircuit->addVirtualLink(pLPSeg);

#ifdef DEBUGB
		cout << "\nFINE COMPUTE ROUTE. CANALE ASSEGNATO: " << pLPSeg->wlAssigned << endl;
#endif // DEBUGB

	} //-B: ELSE caso hBestCost == UNREACHABLE viene gestito da BBU_Provision quando si ritorna da questo metodo

	delete pBestP;

	// delete primary candidate
	/*for (pBestP=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
	delete (*itrPPath);
	}*/

	return hBestCost; //hBestCost;
}

bool NS_OCH::WDMNetwork::checkPathExist(NetMan *pNetMan, Lightpath* lp)
{
	bool flag = false;
	bool exit = false;
	list<UniFiber*>::const_iterator itr;
	list<UniFiber*>::const_iterator itr2;
	list<Lightpath*>::const_iterator itrLP;
	if (pNetMan->m_hLightpathDB.m_hLightpathList.size() > 0)
	{
		cout << "\tControllo nella lista di lightpathdb" << endl;
		for (itrLP = pNetMan->m_hLightpathDB.m_hLightpathList.begin(); itrLP != pNetMan->m_hLightpathDB.m_hLightpathList.end() && exit == false; itrLP++)
		{
			for (itr2 = (*itrLP)->m_hPRoute.begin(), itr = lp->m_hPRoute.begin(), flag = true;
				itr != lp->m_hPRoute.end() && itr2 != (*itrLP)->m_hPRoute.end();
				itr++, itr2++)
			{
				if ((*itr)->m_nLinkId != (*itr2)->m_nLinkId)
				{
					flag = false;
					break;
				}
			}
			if (flag)
				exit = true;
		}
		itrLP--;
	}

	if (flag)
	{
		lp->wlAssigned = (*itrLP)->wlAssigned;
		//lp->m_nBWToBeAllocated = ;
		//(*itrLP)->m_nLinkId++;
	}
	

#ifdef DEBUGB
	if (flag)
	{
		cout << "\nI LIGHTPATH " << (*itrLP)->m_nLinkId << " E " << lp->m_nLinkId << " COINCIDONO:\t";
		for (itr2 = (*itrLP)->m_hPRoute.begin(), itr = lp->m_hPRoute.begin();
			itr != lp->m_hPRoute.end() && itr2 != (*itrLP)->m_hPRoute.end();
			itr++, itr2++)
		{
			cout << (*itr2)->m_nLinkId << "," << (*itr)->m_nLinkId << "->";
		}
	}
#endif // DEBUGB
	return flag;
}

void WDMNetwork::setVectNodes(UINT numOfNodes)
{
	MobileNodes = new UINT[numOfNodes];
	FixedNodes = new UINT[numOfNodes];
	FixMobNodes = new UINT[numOfNodes];
}


// Reachability graph: adjacency based on available wavelength links
//-B: Clear the AbstractGraph object passed as parameter and fill it with those links
//	which have at least one free channel
//	originally taken from genReachabilityGraphThruWL
void WDMNetwork::genReachabilityGraphOCLightpath(AbstractGraph& hGraph, NetMan*pNetMan) const
{
#ifdef DEBUGB
	cout << "-> genReachabilityGraphOCLightpath" << endl;
#endif // DEBUGB

	hGraph.deleteContent();	//clear m_hNodeList and m_hLinkList

							// handle nodes
	list<AbstractNode*>::const_iterator itrNode;
	for (itrNode = m_hNodeList.begin(); itrNode != m_hNodeList.end(); itrNode++)
	{
		hGraph.addNode(new AbstractNode((*itrNode)->getId()));
	}

	cout << "\tnum of nodes: " << hGraph.m_hNodeList.size();

	// handle wavelength links
	int nLinkId = 0;
	AbstractLink *pNewLink;
	list<AbstractLink*>::const_iterator itrLink;
	for (itrLink = m_hLinkList.begin(); itrLink != m_hLinkList.end(); itrLink++) {
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		assert(pUniFiber);
		Channel *pChannel = pUniFiber->m_pChannel;
		assert(pChannel);
		int  w;
		for (w = 0; w < pUniFiber->m_nW; w++)
			if (pNetMan->checkFreedomSimplexLink(pUniFiber->getSrc()->getId(), pUniFiber->getDst()->getId(), w))
			{
				pNewLink = hGraph.addLink(nLinkId++,
					pUniFiber->getSrc()->getId(), pUniFiber->getDst()->getId(),
					pUniFiber->getCost(), pUniFiber->getLength());
				assert(pNewLink);
				break;
			}
	}
	cout << "\tnum of links: " << hGraph.m_hLinkList.size();
	cout << endl;
}

//****************** BBU_UNPROTECTED *******************
//-B: return the cost of the best path found connecting source and destination
//	and add it to the m_hRoute Lightpath list, attribute of a Circuit object
//	-------------------- originally taken from wpUNPROTECTED_ComputeRoute -------------------------

LINK_COST WDMNetwork::BBU_UNPROTECTED_ComputeRoute(list<AbstractLink*> hPPath, OXCNode* pSrc, OXCNode* pDst, UINT&channel)
{
#ifdef DEBUGB
	cout << "->BBU_UNPROTECTED_ComputeRoute" << endl;
#endif // DEBUGB

	AbsPath* pBestP = new AbsPath(); //leak!
									 //comincia yen....
									 //list<AbsPath*> listA;//percorsi definitivi
									 //list<AbsPath*> listB;// percorsi parziali
									 //listA.push_back(path);
									 // numero k percorsi: ce l'ho in pNetMan

									 //-B: use of 'myAlg' to compute paths -> how many? to be debugged or look into pNetMan object -> 1
	LINK_COST hBestCost = myAlg(*pBestP, pSrc, pDst, numberOfChannels);

	//-B: I guess that, thanks to myAlg method, pBestP points to the best path
	//caso unprotected
	if (UNREACHABLE != hBestCost)
	{
		// construct the circuit	//-B: Lightpath_Seg is a class derived from LighPath, that is a class derived from AbstractLink
		list<AbstractLink*>::const_iterator itr;
		//-B: for each link of the pBestP path
		for (itr = pBestP->m_hLinkList.begin(); itr != pBestP->m_hLinkList.end(); itr++)
		{
			hPPath.push_back((*itr)); //-B: m_hPRoute is a list of UniFiber object in the class Lightpath; itr is a AbstractLink object reference; Unifiber class (as well as Lightpath) is derived from AbstractLink
		}
		//-B: the lightpath will use the same wavel/channel assigned to pBestP (the assignment has been done randomly among the available wavel, by the randomChSelection method, belonging to the AbstractGraph class)
		channel = pBestP->wlAssigned;
		//pLPSeg->m_hCost = pBestP->getCost();

#ifdef DEBUGB
		cout << "\tmyAlg andato! Num di link che compongono il percorso: " << hPPath.size();
		cout << "\tCANALE SELEZIONATO CASUALMENTE: " << channel << endl;
#endif // DEBUGB

		/*assert(hBestB.size()>0);
		pLPSeg->m_hBackupSegs = hBestB;*/

		//hCircuit.addVirtualLink(pLPSeg); //-B: add it to the m_hRoute Lightpath list, attribute of a Circuit object
	}
	//-B: case hBestCost == UNREACHABLE is not treated!!! See UNPROTECTED_ComputeRoute to know how to deal with it

	delete pBestP;

	// delete primary candidate
	/*for (pBestP=hPPathList.begin(); itrPPath!=hPPathList.end(); itrPPath++) {
	delete (*itrPPath);
	}*/

	return hBestCost; //hBestCost;
}


//-B: print the network status (WDMNetwork object status) (useful for debugging purpose)
//	for each link (it means, for each unidirectional fiber) it prints:
//	fiberId, num of primary working channels, num of backup channels, connections'/lightpaths' IDs
void WDMNetwork::BBU_WDMNetdump(ostream &out) const
{
	list<AbstractLink*>::const_iterator itr;
	char*wConv;
	char*wCont;
	if (this->m_bFullWConversionEverywhere == true)
		wConv = "TRUE";
	else
		wConv = "FALSE";

	if (this->m_bWContinuous == true)
		wCont = "TRUE";
	else
		wCont = "FALSE";
	out << "-> WDMNetwork DUMP: STATO RETE" << endl;
	out << "\tCore CO ID: " << this->DummyNode;
	out << " | WDMNetwork Channel capacity: " << this->m_nChannelCapacity
		<< " | FullConversion: " << wConv
		<< " | WContinuous: " << wCont 
		<< " | TxScale: " << m_dTxScale << endl;
	out	<< "\tNum of channels/fiber: " << this->numberOfChannels
		<< " | Num of links: " << this->numberOfLinks
		<< " | Num of nodes: " << this->numberOfNodes
		<< " | LinkList size: " << this->m_hLinkList.size()
		<< " | NodeList size: " << this->m_hNodeList.size() << endl;

	//SimplexLink* singleLink;
	/*
	out << "\n-> UniFiber DUMP" << endl;
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		single_link = (UniFiber*)(*itr);
		//singleLink = (SimplexLink*)(*itr);
		//-B: UNIFIBER DUMP (not  AbstractLink dump)
		single_link->dump(cout);
		//-B: commento la stampa successiva perchè la var non viene nè inizializzata nè utilizzata (credo)
		//out << "\tFreeCap = " << singleLink->m_hFreeCap << endl;
	}
	cout << endl;
	*/
}

bool NS_OCH::WDMNetwork::isMobileNode(UINT nodeId)
{
	int i;
	bool nFlag;
	//-B: identify if source node is mobile
	for (i = 0, nFlag = false; MobileNodes[i] != NULL; i++)
	{
		if (nodeId == MobileNodes[i])
		{
			nFlag = true;
			break;
		}
	}
	return nFlag;
}

bool NS_OCH::WDMNetwork::isFixMobNode(UINT nodeId)
{
	int i;
	bool nFlag;
	//-B: identify if source node is fixed-mobile
	for (i = 0, nFlag = false; FixMobNodes[i] != NULL; i++)
	{
		if (nodeId == FixMobNodes[i])
		{
			nFlag = true;
			break;
		}
	}
	return nFlag;
}

bool NS_OCH::WDMNetwork::isFixedNode(UINT nodeId)
{
	int i;
	bool nFlag;
	//-B: identify if source node is fixed-mobile
	for (i = 0, nFlag = false; FixedNodes[i] != NULL; i++)
	{
		if (nodeId == FixedNodes[i])
		{
			nFlag = true;
			break;
		}
	}
	return nFlag;
}


void WDMNetwork::invalidateWlOccupationLinks(Circuit*pCircuit, Graph&pGraph)
{
#ifdef DEBUGB
	cout << "-> invalidateWlOccupationLinks" << endl;
#endif // DEBUGB
	
	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink;
	channelsToDelete.clear();

	//-B: scorro tutti i simplex link del grafo
	for (itr = pGraph.m_hLinkList.begin(); itr != pGraph.m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		//-B: se il simplex link corrispondente al canale è stato invalidato
		if (!(pLink->getValidity()))
		{
			//-B: imposto un costo uguale ad UNREACHABLE all'array dei costi del canale della fibra a cui appartiene
			assert(pLink->m_pUniFiber);
			pLink->m_pUniFiber->wlOccupation[pLink->m_nChannel] = UNREACHABLE;
			channelsToDelete.push_back(pLink->m_nChannel);
			//cout << endl << "\tEHI TIO MIRA! EL CANAL " << pLink->m_nChannel
				//	<< " DE LA FIBRA " << pLink->m_pUniFiber->getId() << " NO SE PUEDE LLEGAR" << endl;
		}
	}
	
	
	/*
	int w;
	UniFiber*pUniFiber;
	//-B: scorro tutte le fibre della rete WDMNetPast
	for (itr = m_hLinkList.begin(); itr != m_hLinkList.end(); itr++)
	{
		pUniFiber = (UniFiber*)(*itr);
		//-B: scorro tutti i canali di quella fibra
		for (w = 0; w < pUniFiber->m_nW; w++)
		{
			//-B: identifico il simplex link corrispondente al canale considerato
			pLink = pGraph.lookUpSimplexLink(pUniFiber->getSrc()->getId(), Vertex::VT_Channel_In, w,
					pUniFiber->getDst()->getId(), Vertex::VT_Channel_Out, w);
			//-B: se il canale ha capacità insufficiente per la connessione che stiamo instradando
			if (pLink->m_hFreeCap < pCircuit->m_eBW)
			{
				//-B: pongo il costo di quel canale uguale ad UNREACHABLE
				pUniFiber->wlOccupation[w] = UNREACHABLE;
				cout << endl << "\tEHI TIO MIRA! EL CANAL " << w << " NO SE PUEDE LLEGAR" << endl;
			}
		}
	}*/
}

//-B: reset BBUsReachCost = UNREACHABLE for all nodes
void WDMNetwork::resetBBUsReachabilityCost()
{
#ifdef DEBUGB
	cout << "-> resetBBUsReachabilityCost" << endl;
#endif // DEBUGB

	list<AbstractNode*>::const_iterator itrNode;
	OXCNode* pOXCNode;
	for (itrNode = this->m_hNodeList.begin(); itrNode != this->m_hNodeList.end(); itrNode++)
	{
		pOXCNode = (OXCNode*)(*itrNode);
		assert(pOXCNode);
		if (pOXCNode->getBBUHotel())
			pOXCNode->m_nBBUReachCost = UNREACHABLE;
	}
}

//-B: method called only if connection type is FIXMOB_FRONTHAUL or MOBILE_FRONTHAUL
void WDMNetwork::updateBBUsUseAfterBlock(Connection*pCon, ConnectionDB&connDB)
{
#ifdef DEBUGB
	cout << "-> updateBBUsUseAfterBlock" << endl;
#endif // DEBUGB

	int numOfConn;
	OXCNode*pOXCDst = (OXCNode*)this->lookUpNodeById(pCon->m_nDst);
	OXCNode*pOXCSrc = (OXCNode*)this->lookUpNodeById(pCon->m_nSrc);
	assert(pOXCSrc);
	assert(pOXCDst);

	//-B: it should be always a BBU hotel, right?! Not if we allow a node to host its own bbu in its cell site as last option
	//if (pOXCDst->getBBUHotel())
	//{
		//-B: count num of (!) FRONTHAUL (!) connections routed between the same source and destination
		numOfConn = countConnections(pOXCSrc, pOXCDst, connDB);
		//-B: *******************************************************************
		//	IF THIS SOURCE DOESN'T HAVE ANY OTHER CONNECTION BESIDE THIS CONNECTION THAT WAS JUST BLOCKED
		//***********************************************************************
		if (numOfConn == 0)
		{
#ifdef DEBUGB
			cout << "\tIl source node " << pOXCSrc->getId() << " non ha altre connessioni. Rimuovo la BBU assegnata" << endl;
#endif // DEBUGB
			//reset to default value
			pOXCSrc->m_nBBUNodeIdsAssigned = 0;
			//decrease num of active BBUs in this hotel node
			pOXCDst->m_nBBUs--;
			assert(pOXCDst->m_nBBUs >= 0);
			//if this hotel node does not have active BBU in itself anymore
			if (pOXCDst->m_nBBUs == 0)
			{
				if (pOXCDst->getBBUHotel())
				{
#ifdef DEBUGB
					cout << "\tL'hotel node " << pOXCDst->getId() << " non ha piu' BBU attive. Lo disattivo" << endl;
#endif // DEBUGB
					removeActiveBBUs(pOXCDst);
				}
				else
				{
					;
#ifdef DEBUGB
					cout << "\tIl nodo " << pOXCDst->getId() << " torna ad essere un semplice mobile node" << endl;
#endif // DEBUGB
				}
			}
		}
		else
		{
			NULL;
#ifdef DEBUGB
			cout << "\tIl nodo sorgente " << pCon->m_nSrc << " ha altre "
				<< (numOfConn) << " connessioni aventi come destinazione il nodo " << pCon->m_nDst << endl;
#endif // DEBUGB
		}
	//} //end IF
}

//-B: method called only if connection type is FIXMOB_FRONTHAUL or MOBILE_FRONTHAUL
void WDMNetwork::updatePoolsUse(Connection*pCon, ConnectionDB&connDB)
{
#ifdef DEBUGB
	cout << "-> updatePoolsUse" << endl;
#endif // DEBUGB

	OXCNode*pOXCDst = (OXCNode*)this->lookUpNodeById(pCon->m_nDst);
	OXCNode*pOXCSrc = (OXCNode*)this->lookUpNodeById(pCon->m_nSrc);
	assert(pOXCSrc);
	assert(pOXCDst);

	//-B: it should be always a BBU hotel, right?! Not if we allow a node to host its own bbu in its cell site as last option
	//if (pOXCDst->getBBUHotel())

	if (pOXCSrc->m_nBBUNodeIdsAssigned == pOXCDst->getId())
	{
		//reset to default value
		pOXCSrc->m_nBBUNodeIdsAssigned = 0;
	}
	//decrease num of active connections in this pooling node
	pOXCDst->m_nBBUs--;
	assert(pOXCDst->m_nBBUs >= 0);
		
	//if this hotel node does not have any active connection to process
	if (pOXCDst->m_nBBUs == 0)
	{
		this->computeTrafficProcessed_BBUPooling(pOXCDst, connDB.m_hConList);
#ifdef DEBUGB
		cout << "\tNode #" << pOXCDst->getId() << " - traffic processed: " << pOXCDst->m_dTrafficProcessed
			<< " - bbus (connections): " << pOXCDst->m_nBBUs << endl;
#endif // DEBUGB
		//-B: check: we decrease the traffic value by m_eBandwidth because we haven't already deprovided the connection
		assert((pOXCDst->m_dTrafficProcessed - pCon->m_eBandwidth) == 0);
		if (pOXCDst->getBBUHotel())
		{
#ifdef DEBUGB
			cout << "\tL'hotel node " << pOXCDst->getId() << " non ha piu' BBU attive. Lo disattivo" << endl;
#endif // DEBUGB
			removeActiveBBUs(pOXCDst);
		}
		else
		{
			;
#ifdef DEBUGB
			cout << "\tIl nodo " << pOXCDst->getId() << " torna ad essere un semplice mobile node" << endl;
#endif // DEBUGB
		}
	}
	else
	{
		NULL;
#ifdef DEBUGB
		cout << "\tCi sono altre " << pOXCDst->m_nBBUs << " connessioni aventi come destinazione il nodo " << pCon->m_nDst << endl;
#endif // DEBUGB
	}
	//} //end IF
}

//-B: method called only if connection type is FIXMOB_FRONTHAUL or MOBILE_FRONTHAUL
void WDMNetwork::updateBBUsUseAfterDeparture(Connection*pCon, ConnectionDB&connDB)
{
#ifdef DEBUGC
	cout << "-> updateBBUsUseAfterDeparture" << endl;
#endif // DEBUGB

	int numOfConn;
	OXCNode*pOXCDst = (OXCNode*)this->lookUpNodeById(pCon->m_nDst);
	OXCNode*pOXCSrc = (OXCNode*)this->lookUpNodeById(pCon->m_nSrc);
	assert(pOXCSrc);
	assert(pOXCDst);

	//-B: it should be always a BBU hotel, right?! Not if we allow a node to host its own bbu in its cell site as last alternative
	//if (pOXCDst->getBBUHotel())
	//{
		//-B: count num of (!) FRONTHAUL (!) connections routed between the same source and destination
		numOfConn = countConnections(pOXCSrc, pOXCDst, connDB);
		//-B: *******************************************************************
		//	IF THIS SOURCE HAS ONLY THIS CONNECTION THAT IT IS GOING TO DEPROVIDE
		//***********************************************************************
		if (numOfConn == 1)
		{
			if (pOXCSrc->m_nBBUNodeIdsAssignedLast > 0) {
#ifdef DEBUGC
				cout << "\tIl source node " << pOXCSrc->getId() << " non ha altre connessioni sulla vecchia BBU; rimuovo la vecchia BBU associata" << endl;
#endif // DEBUB
				pOXCSrc->m_nPreviousBBU = pOXCSrc->m_nBBUNodeIdsAssigned;
				pOXCSrc->m_nBBUNodeIdsAssigned = pOXCSrc->m_nBBUNodeIdsAssignedLast;
				pOXCSrc->m_nBBUNodeIdsAssignedLast = 0;
				//cin.get();

				//decrease num of active BBUs in this hotel node
				if(pCon->m_eConnType == Connection::FIXED_MIDHAUL)
					pOXCDst->m_nCUs--;
				else
					pOXCDst->m_nBBUs--;

				//-L: ????? no sense, lo commento
				//assert(pOXCDst->m_nBBUs >= 0);

				//if this hotel node does not have active BBU in itself anymore
				if (pOXCDst->m_nBBUs == 0 && pOXCDst->m_nCUs == 0)
				{
					if (pOXCDst->getBBUHotel())
					{
#ifdef DEBUGC
						cout << "\tL'hotel node " << pOXCDst->getId() << " non ha piu' BBU attive. Lo disattivo" << endl;
#endif // DEBUGB
						removeActiveBBUs(pOXCDst);
					}
					else
					{
						;
#ifdef DEBUGC
						cout << "\tIl nodo " << pOXCDst->getId() << " torna ad essere un semplice mobile node" << endl;
						//cin.get();
#endif // DEBUGB
					}
				}
			}
			else {

#ifdef DEBUGC
				cout << "\tIl source node " << pOXCSrc->getId() << " non ha altre connessioni. Rimuovo la BBU assegnata" << endl;
#endif // DEBUGB
				//reset to default value
				pOXCSrc->m_nBBUNodeIdsAssigned = 0;

				//decrease num of active BBUs in this hotel node
				if (pCon->m_eConnType == Connection::FIXED_MIDHAUL)
					pOXCDst->m_nCUs--;
				else
					pOXCDst->m_nBBUs--;

				//-L: ????? no sense, lo commento
				// assert(pOXCDst->m_nBBUs >= 0);

				//if this hotel node does not have active BBU in itself anymore
				if (pOXCDst->m_nBBUs == 0 && pOXCDst->m_nCUs == 0)
				{
					if (pOXCDst->getBBUHotel())
					{
#ifdef DEBUGC
						cout << "\tL'hotel node " << pOXCDst->getId() << " non ha piu' BBU attive. Lo disattivo" << endl;
#endif // DEBUGB
						removeActiveBBUs(pOXCDst);
					}
					else
					{
						;
#ifdef DEBUGC
						cout << "\tIl nodo " << pOXCDst->getId() << " torna ad essere un semplice mobile node" << endl;
						//cin.get();
#endif // DEBUGB
					}
				}
			}
		}
		else
		{
			NULL;
	#ifdef DEBUG
				cout << "\tIl nodo sorgente " << pCon->m_nSrc << " ha altre "
				<< (numOfConn - 1) << " connessioni, oltre a quella appena terminata, aventi come destinazione il nodo " << pCon->m_nDst << endl;
	#endif // DEBUGB
		}
	//} //end IF
}

//-B
void WDMNetwork::removeActiveBBUs(OXCNode*pOXCBBUNode)
{
	int i;
	for (i = 0; i < this->BBUs.size(); i++)
	{
		if (pOXCBBUNode->getId() == this->BBUs[i]->getId())
		{
			break; //i will be the position's index of the given OXC BBU node
		}
	}
	
	if (i < this->BBUs.size())
	{
		this->BBUs.erase(this->BBUs.begin() + i);
	}
	//else it means that pOXCBBUNode is a simple mobile node that was hosting its own BBU --> it is not present in BBUs list

#ifdef DEBUGC
	cout << "\tBBU Hotel node " << pOXCBBUNode->getId() << " non piu' utilizzato. Lo disattivo" << endl;
#endif // DEBUGB
}


void WDMNetwork::printHotelNodes()
{
#ifdef DEBUGC
	cout << "-> printHotelNodes" << endl;
	cout << "\t#" << BBUs.size() << " BBU hotel nodes:" << endl;
#endif // DEBUGB

	int i;
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;
	int count = 0;
	
	for (i = 0; i < this->BBUs.size(); i++)
	{

#ifdef DEBUGC
		cout << "\tHotel node " << BBUs[i]->getId() << ": " << BBUs[i]->m_nBBUs << endl;
#endif	
		count++;
	}

	//-B: scan all node of the networks
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		//select only NO candidate hotel nodes
		if (!pNode->getBBUHotel())
		{
			//if it hosts its own BBU
			if (pNode->m_nBBUs > 0)
			{
#ifdef DEBUGC
				cout << "\tHotel node " << pNode->getId() << ": " << pNode->m_nBBUs << endl;
#endif			
				count++;
			}
		}
	}
	cout << "Number of active hotels: " << count << endl;
}

void WDMNetwork::printBBUs()
{
#ifdef DEBUGX
	cout << "-> printBBUs" << endl;
#endif // DEBUGB

	list<AbstractNode*>::const_iterator itr;
	OXCNode*pOXC;
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pOXC = (OXCNode*)(*itr);
		assert(pOXC);
		if (pOXC->m_nBBUNodeIdsAssigned > 0)
		{
#ifdef DEBUGX
			cout << "Node " << pOXC->getId() << "--> BBU nel nodo " << pOXC->m_nBBUNodeIdsAssigned << endl;
#endif
		}

		if (pOXC->m_nBBUNodeIdsAssignedLast > 0)
		{
#ifdef DEBUGX
			cout << "Node " << pOXC->getId() << "--> second BBU nel nodo " << pOXC->m_nBBUNodeIdsAssignedLast << endl;
			//cin.get();
#endif
		}
	}
}

UINT WDMNetwork::countBBUs()
{
#ifdef DEBUG
	cout << "-> countBBUs" << endl;
#endif // DEBUGB
	
	UINT count = 0;
	int i;
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	for (i = 0; i < this->BBUs.size(); i++)
	{
		count += BBUs[i]->m_nBBUs;
	}
	//-B: scan all nodes
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		//select only NO candidate hotel nodes
		if (!pNode->getBBUHotel())
		{
			//pNode->m_nBBUs will be: = 0, if the node doesn't host its own BBU;
			//						  = 1, if the node hosts its own BBU.
			assert(pNode->m_nBBUs == 0 || pNode->m_nBBUs == 1);
			count += pNode->m_nBBUs;
		}
	}

	return count;
}

void WDMNetwork::logBBUInHotel()
{
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pOXC;
	bool already;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pOXC = (OXCNode*)(*itr);
		already = false;

		if (pOXC->m_nBBUNodeIdsAssigned > 0)
		{
			if (pOXC->m_vLogHotels.size() > 0)
			{
				//consider last hotel logged
				int i = pOXC->m_vLogHotels.size() - 1;
				if (pOXC->m_vLogHotels[i] == pOXC->m_nBBUNodeIdsAssigned)
				{
					already = true;
				}
				
				if (!already)
				{
					pOXC->m_vLogHotels.push_back(pOXC->m_nBBUNodeIdsAssigned);
				}
			}
			else //first hotel to be logged
			{
				pOXC->m_vLogHotels.push_back(pOXC->m_nBBUNodeIdsAssigned);
			}
		}
		//else no hotel node assigned for this source node
	}
}

int WDMNetwork::countConnections(OXCNode*pOXCSrc, OXCNode * pOXCDst, ConnectionDB&connDB)
{
	int count = 0;
	list<Connection*>::const_iterator itr;
	//-B: scan all the FRONTHAUL connections
	for (itr = connDB.m_hConList.begin(); itr != connDB.m_hConList.end(); itr++)
	{
		if ((*itr)->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL
			|| (*itr)->m_eConnType == Connection::MOBILE_FRONTHAUL)
		{
			//-B: if the selected connection has the given (source) node as destination node
			if ((*itr)->m_nSrc == pOXCSrc->getId() && (*itr)->m_nDst == pOXCDst->getId())
			{
				count++;
			}
		}
	}//end FOR
	return count;
}


void WDMNetwork::fillHotelsList()
{	
	list<AbstractNode*>::const_iterator itrNode;
	OXCNode*pOXCNode;

	for (itrNode = this->m_hNodeList.begin(); itrNode != this->m_hNodeList.end(); itrNode++)
	{
		pOXCNode = (OXCNode*)(*itrNode);
		if (pOXCNode->getBBUHotel())
		{
			this->hotelsList.push_back(pOXCNode);
#ifdef DEBUG
			cout << "\t----Nodo #" << pOXCNode->getId() << " inserito " << endl;
#endif // DEBUGB
		}
	}
}

//sort the candidate hotel nodes by cost and, for those having the same cost, by reachability degree
void WDMNetwork::sortHotelsList(Graph&m_hGraph)
{
	cout << "\tSort hotel nodes list\n..." << endl;

	int i;
	Vertex*pVSrc, *pVDst;
	OXCNode*pNode, *pOXC; //*pAuxNode, *pDebug;
	list<AbstractLink*> hPrimaryPath;
	list<AbstractNode*>::const_iterator itrNode;
	BinaryHeap<OXCNode*, vector<OXCNode*>, POXCNodeStageComp> sortedHotelsList;
	BinaryHeap<OXCNode*, vector<OXCNode*>, POXCNodeReachComp> reachHotelsList;
	BinaryHeap<OXCNode*, vector<OXCNode*>, POXCNodeDistanceComp> partialHotelsList;
	vector<OXCNode*> auxHotelsList;
	vector<OXCNode*>::iterator itr;
	list<OXCNode*>::const_iterator itr2, itr3, itrDebug;

	for (i = 0; i < this->hotelsList.size(); i++)
	{
		
		pNode = hotelsList[i];
		pVSrc = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_Out, -1);
		pVDst = m_hGraph.lookUpVertex(this->DummyNode, Vertex::VertexType::VT_Access_In, -1);
		//-B: here the var netStage is used as a measure of the distance between one among the hotel node and the core CO
		pNode->m_nNetStage = m_hGraph.Dijkstra(hPrimaryPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
		pNode->m_fDistanceFromCoreCO = computeLength(hPrimaryPath);
		sortedHotelsList.insert(pNode);
	}

	while (!sortedHotelsList.empty())
	{
		pNode = sortedHotelsList.peekMin();
		sortedHotelsList.popMin();
#ifdef DEBUG
		cout << "\tNodo #" << pNode->getId() << " - stage = " << pNode->m_nNetStage
			<< " - distanza dal core CO in km = " << pNode->m_fDistanceFromCoreCO << endl; 
#endif // DEBUGB
		auxHotelsList.push_back(pNode);
	}
#ifdef DEBUG
#endif // DEBUGB

	//-B: PRE-PROCESSING
	//empty list
	hotelsList.clear();
	//prefer wavel path over wavel conversion
	m_hGraph.preferWavelPath();
	
	//------------------------- NEXT STEPS -----------------------------
	//-B: fill again the list with the sorted elements
	//-B: compute the number of nodes that can reach each hotel node
	//------------------------------------------------------------------
	LINK_COST savedCost = UNREACHABLE, referenceCost = UNREACHABLE, distCost = UNREACHABLE;
	// scan all hotels nodes, sorted by cost: they will be the destination nodes
	for (itr = auxHotelsList.begin(); itr != auxHotelsList.end(); itr++)
	{
		pNode = ((OXCNode*)(*itr));
		pVDst = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_In, -1);
		
		//cost between the hotel and the core CO
		savedCost = pNode->m_nNetStage;
#ifdef DEBUG
		cout << "\nCONSIDERO L'HOTEL NODE #" << pNode->getId() << " - saved stage = " << savedCost << endl;
		cout << "Calcolo il suo grado di reachability" << endl;
		//cin.get();
#endif // DEBUGB
		//scan all nodes of the network
		for (itrNode = this->m_hNodeList.m_hList.begin(); itrNode != this->m_hNodeList.m_hList.end(); itrNode++)
		{
			pOXC = (OXCNode*)(*itrNode);
			// consider only mobile or fixed-mobile nodes
			if (this->isMobileNode(pOXC->getId()) || this->isFixMobNode(pOXC->getId()))
			{
				pVSrc = m_hGraph.lookUpVertex(pOXC->getId(), Vertex::VertexType::VT_Access_Out, -1);
				referenceCost = m_hGraph.DijkstraLatency(hPrimaryPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
				//if it the dst is reachable from this selected node
				if (referenceCost < UNREACHABLE)
				{
					//if (computeLatency(hPrimaryPath) <= LATENCYBUDGET) //-B: no need to perform this check because we use DijkstraLatency
					//{
						//-B: increase node's reachability degree
						pNode->m_nReachabilityDegree++;
#ifdef DEBUG
						cout << "\tADMISSIBLE!" << endl;
#endif // DEBUGB
					//}
				} //end IF reachable
				  //else: nothing to do
			} //end IF isMobile OR isFixMob
			  //else: nothing to do
		} //end FOR node list
		  
		//-B: insert pNode in the list sorted by reachability degree
		reachHotelsList.insert(pNode);
#ifdef DEBUG
		cout << "INSERISCO L'HOTEL NODE nella reach hotels list. Grado di reachability: " << pNode->m_nReachabilityDegree << endl;
#endif // DEBUGB
		//consider next node
		itr++;
		//-B: examinate all the nodes having the same netstage
		while (itr != auxHotelsList.end())
		{
			pNode = ((OXCNode*)(*itr));
			pVDst = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_In, -1);
				// if it has the same cost of the previous one
				if (pNode->m_nNetStage == savedCost)
				{
#ifdef DEBUG
					cout << "\nCONSIDERO L'HOTEL NODE #" << pNode->getId() << " - stage = " << pNode->m_nNetStage << endl;
					cout << "Calcolo il suo grado di reachability" << endl;
					//cin.get();
#endif // DEBUGB
					//scan all nodes of the network
					for (itrNode = this->m_hNodeList.m_hList.begin(); itrNode != this->m_hNodeList.m_hList.end(); itrNode++)
					{
						pOXC = (OXCNode*)(*itrNode);
						// consider only mobile or fixed-mobile nodes
						if (this->isMobileNode(pOXC->getId()) || this->isFixMobNode(pOXC->getId()))
						{
							pVSrc = m_hGraph.lookUpVertex(pOXC->getId(), Vertex::VertexType::VT_Access_Out, -1);
							distCost = m_hGraph.DijkstraLatency(hPrimaryPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
							//if it the dst is reachable from this selected node
							if (distCost < UNREACHABLE)
							{
								//if (computeLatency(hPrimaryPath) <= LATENCYBUDGET) //-B: no need to perform this check because we use DijkstraLatency
								//{
									//increase node's reachability degree
									pNode->m_nReachabilityDegree++;
#ifdef DEBUG
									cout << "\tADMISSIBLE!" << endl;
#endif // DEBUGB
								//}
							} //end IF reachable
							  //else: nothing to do
						} //end IF isMobile OR isFixMob
						  //else: nothing to do
					} //end FOR node list
					//-B: insert pNode in the list sorted by reachability degree
					reachHotelsList.insert(pNode);
#ifdef DEBUG
					cout << "INSERISCO L'HOTEL NODE nella reach hotels list. Grado di reachability: " << pNode->m_nReachabilityDegree << endl;
#endif // DEBUGB
				} //end IF same cost
				else
				{
#ifdef DEBUG
					cout << "\nL'HOTEL NODE #" << pNode->getId() << " HA STAGE DIVERSO - ReachCost = " << pNode->m_nNetStage << endl;
					//cin.get();
#endif // DEBUGB
					itr--;
					//exit from WHILE
					break;
				}
			itr++;
		} //end while

		//put all the elements of reachHotelsList in hotelsList, sorted by their reachability degree
		while (!reachHotelsList.empty())
		{
			pOXC = reachHotelsList.peekMin();
			reachHotelsList.popMin();
#ifdef DEBUG
			cout << "\tINSERISCO IN HOTELS LIST: Nodo #" << pOXC->getId() << ": stage = " << pOXC->m_nNetStage << " - reach degree = " << pOXC->m_nReachabilityDegree << endl;
			//cin.get();
#endif // DEBUGB
			hotelsList.push_back(pOXC);
		}
		//-B: FONDAMENTALE!!!
		if (itr == auxHotelsList.end())
		{
			break;
		}
	} //end FOR auxHotelsList

	//-B: WE WANT TO ADD A PASSAGE, BUTI IT REQUIRE A BIT INTRICATE PROCEDURE
	//this->sortHotelsByDistance(auxHotelsList); //-B: don't know if it is really useful

	cout << "Sorted hotels list by cost + reachability:" << endl; //+ distance (km)
	for (itr = hotelsList.begin(); itr != hotelsList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		cout << pNode->getId() << " (" << pNode->m_nNetStage << "s, " << pNode->m_nReachabilityDegree << "r, " 
			<< pNode->m_fDistanceFromCoreCO << "km)" << endl;
	}
}

//-B: sort the candidate hotel nodes by cost and, for those having the same cost, by proximity degree
//	similar to previous method but a different way of computing the reachability degree
void WDMNetwork::sortHotelsList_Evolved(Graph&m_hGraph)
{
#ifdef DEBUG
	cout << "\tSort hotel nodes list\n..." << endl;
#endif // DEBUGB

	int i;
	Vertex*pVSrc, *pVDst;
	OXCNode*pNode, *pOXC;
	list<AbstractLink*> hPrimaryPath;
	list<AbstractNode*>::const_iterator itrNode;
	BinaryHeap<OXCNode*, vector<OXCNode*>, POXCNodeStageComp> sortedHotelsList;
	BinaryHeap<OXCNode*, vector<OXCNode*>, POXCNodeReachComp> reachHotelsList;
	vector<OXCNode*> auxHotelsList, partialHotelsList;
	vector<OXCNode*>::iterator itr;


	//-B: build hotels list, sorted by their m_hCost
	for (i = 0; i < this->hotelsList.size(); i++)
	{
		pNode = hotelsList[i];
		pVSrc = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_Out, -1);
		pVDst = m_hGraph.lookUpVertex(this->DummyNode, Vertex::VertexType::VT_Access_In, -1);
		//-B: var m_nNetStage is used as measure of the distance between one of the hotel node and the core CO
		//	it is computed here and can be used during all the simulations, if needed
		pNode->m_nNetStage = m_hGraph.Dijkstra(hPrimaryPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
#ifdef DEBUGB
		cout << "\tNodo " << pNode->getId() << " - distanza dal core CO: " << pNode->m_nNetStage << endl;
#endif // DEBUGB
		sortedHotelsList.insert(pNode);
	}

	//build auxiliary hotels list (already sorted)
	while (!sortedHotelsList.empty())
	{
		pNode = sortedHotelsList.peekMin();
		sortedHotelsList.popMin();
#ifdef DEBUGB
		cout << "\tNodo " << pNode->getId() << ": stage = " << pNode->m_nNetStage << endl;
#endif // DEBUGB
		auxHotelsList.push_back(pNode);
	}
#ifdef DEBUGB
	cin.get();
#endif // DEBUGB

	//-B: PRE-PROCESSING
	//empty list
	hotelsList.clear();
	//prefer wavel path over wavel conversion
	m_hGraph.preferWavelPath();

	//------------------------- NEXT STEPS -----------------------------
	//-B: fill again the list with the sorted elements
	//-B: compute the number of nodes that can reach each hotel node
	//------------------------------------------------------------------
	LINK_COST referenceCost = UNREACHABLE, distCost = UNREACHABLE;
	//-B: scan all network's nodes
	for (itrNode = this->m_hNodeList.begin(); itrNode != this->m_hNodeList.end(); itrNode++)
	{
		//reset reference cost
		referenceCost = UNREACHABLE;
		//SOURCE NODE
		pOXC = (OXCNode*)(*itrNode);
		// consider only mobile or fixed-mobile nodes
		if (this->isMobileNode(pOXC->getId()) || this->isFixMobNode(pOXC->getId()))
		{
			//SOURCE VERTEX
			pVSrc = m_hGraph.lookUpVertex(pOXC->getId(), Vertex::VertexType::VT_Access_Out, -1);
			//-B: scan all hotel nodes
			for (itr = auxHotelsList.begin(); itr != auxHotelsList.end(); itr++)
			{
				//DESTINATION NODE
				pNode = (OXCNode*)(*itr);
				//if it is not the same source node
				if (pNode->getId() != pOXC->getId())
				{
					//DST VERTEX
					pVDst = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_In, -1);
					//-B: here the var BBUReachCost is used as measure of the distance between one of the node and one of the hotel
					distCost = m_hGraph.Dijkstra(hPrimaryPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
					//if it the dst is reachable from this selected node
					if (distCost < UNREACHABLE)
					{
						//if the latency constraint is respected
						if (computeLatency(hPrimaryPath) <= LATENCYBUDGET)
						{
							//save cost in dst node
							pNode->m_nBBUReachCost = distCost;
							//if it is lower than the minimum cost
							if (distCost < referenceCost)
							{
								//save it as minimum cost
								referenceCost = distCost;
							}
						}
					}
				}
			}//end FOR hotels
			//-B: scan again all hotels to add the reachability degree
			for (itr = auxHotelsList.begin(); itr != auxHotelsList.end(); itr++)
			{
				pNode = (OXCNode*)(*itr);
				if (pNode->getId() != pOXC->getId())
				{
					//if node reach. cost is the minimum
					if (pNode->m_nBBUReachCost == referenceCost)
					{
						pNode->m_nReachabilityDegree++;
#ifdef DEBUGB
						cout << "\tNODO SORGENTE #" << pOXC->getId() << " - Incremento il reachability degree del nodo " << pNode->getId()
							<< " con costo " << pNode->m_nBBUReachCost << ": " << pNode->m_nReachabilityDegree << "r" << endl;
						//cin.get();
#endif // DEBUGB
					}
				}
			}
		}
		//else: go to the next node
	}//end FOR nodes

	LINK_COST savedCost = UNREACHABLE;
	// scan all hotels nodes, sorted by cost: they will be the destination nodes
	for (itr = auxHotelsList.begin(); itr != auxHotelsList.end(); itr++)
	{
		pNode = ((OXCNode*)(*itr));
		pVDst = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_In, -1);

		//cost between the hotel and the core CO
		savedCost = pNode->m_nNetStage;
#ifdef DEBUGB
		cout << "\nCONSIDERO L'HOTEL NODE #" << pNode->getId() << " - savedCost = " << savedCost << endl;
		cin.get();
#endif // DEBUGB
		  //-B: insert pNode in the list sorted by reachability degree
		reachHotelsList.insert(pNode);
#ifdef DEBUGB
		cout << "INSERISCO L'HOTEL NODE nella reach hotels list. Grado di reachability: " << pNode->m_nReachabilityDegree << endl;
#endif // DEBUGB
		//consider next node
		itr++;
		//look for hotels with the same m_hCost, i.e. the same distance from core CO
		while (itr != auxHotelsList.end())
		{
			pNode = ((OXCNode*)(*itr));
			pVDst = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_In, -1);
			// if it has the same cost of the previous one
			if (pNode->m_nNetStage == savedCost)
			{
#ifdef DEBUGB
				cout << "\nCONSIDERO L'HOTEL NODE #" << pNode->getId() << " - stage = " << pNode->m_nNetStage << endl;
				//cin.get();
#endif // DEBUGB
				  //-B: insert pNode in the list sorted by reachability degree
				reachHotelsList.insert(pNode);
#ifdef DEBUGB
				cout << "INSERISCO L'HOTEL NODE nella reach hotels list. Grado di reachability: " << pNode->m_nReachabilityDegree << endl;
#endif // DEBUGB
			} //end IF same cost
			else
			{
#ifdef DEBUGB
				cout << "\nL'HOTEL NODE #" << pNode->getId() << " HA COSTO DIVERSO - stage = " << pNode->m_nNetStage << endl;
				//cin.get();
#endif // DEBUGB
				itr--;
				//exit from WHILE
				break;
			}
			itr++;
		} //end while

		  //put all the elements of reachHotelsList in hotelsList, sorted by their reachability degree
		while (!reachHotelsList.empty())
		{
			pOXC = reachHotelsList.peekMin();
			reachHotelsList.popMin();
#ifdef DEBUGB
			cout << "\tINSERISCO IN HOTELS LIST: Nodo #" << pOXC->getId() << ": stage = " << pOXC->m_nNetStage 
				<< " - reach degree = " << pOXC->m_nReachabilityDegree << endl;
			//cin.get();
#endif // DEBUGB
			hotelsList.push_back(pOXC);
		}
		//-B: FONDAMENTALE!!!
		if (itr == auxHotelsList.end())
		{
			break;
		}
	} //end FOR auxHotelsList


	cout << "Sorted hotels list by cost + reachability:" << endl;
	for (itr = hotelsList.begin(); itr != hotelsList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		cout << pNode->getId() << " (" << pNode->m_nReachabilityDegree << " r)" << endl;
	}
	//cin.get();
}


void WDMNetwork::logPeriodical(Log&hLog, SimulationTime hTimeSpan)
{
	list<AbstractLink*>::const_iterator itr;
	UniFiber*pUniFiber;
	for (itr = this->m_hLinkList.begin(); itr != this->m_hLinkList.end(); itr++)
	{
		double busyChannels = 0;
		pUniFiber = (UniFiber*)(*itr);
		if (pUniFiber->getUsedStatus() == 1)
		{
			for (int w = 0; w < pUniFiber->m_nW; w++)
			{
				if (pUniFiber->m_pChannel[w].m_bFree == false)
				{
					busyChannels++;
				}
			}
		}
		else
		{
			busyChannels = 0;
		}
		pUniFiber->m_dLinkLoad += (busyChannels*hTimeSpan);
	}
}

UINT WDMNetwork::countCandidateHotels()
{
	UINT count = 0;
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		if (pNode->getBBUHotel())
		{
			count++;
		}
	}
	return count;
}

float WDMNetwork::computeLatency(list<AbstractLink*>&path)
{
#ifdef DEBUGB
	cout << "-> computeLatency" << endl;
#endif // DEBUGB

	float latency = 0;
	list<AbstractLink*>::const_iterator itr = path.begin();

	//-B: scan every simplex link of path
	while (itr != path.end())
	{
		SimplexLink *pLink = (SimplexLink*)(*itr);
		assert(pLink);

		switch (pLink->m_eSimplexLinkType)
		{
			case SimplexLink::LT_Channel:
			{
#ifdef DEBUGB

				cout << "\t+ Fiber's length = " << pLink->m_pUniFiber->getLength() << endl;
#endif // DEBUGB
				latency += pLink->m_pUniFiber->m_latency;
				break;
			}
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Lightpath:
			{
#ifdef DEBUGB
				cout << "\t+ Lightpath's length = " << pLink->getLength() << endl;
#endif // DEBUGB
				latency += pLink->m_latency;
				break;
			}
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Converter:
			{
#ifdef DEBUGB
				cout << "\t+ Electronic switch!" << endl;
#endif // DEBUGB
				latency += (float)ELSWITCHLATENCY;
				break;
			}
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Rx:
				NULL;   // No need to do anything for these cases
				break;
			default:
				DEFAULT_SWITCH;
		} // end switch
		itr++;
	} // end while
#ifdef DEBUGB
	cout << "\tPath latency = " << latency << endl;
#endif // DEBUGB
	return latency;
}

float WDMNetwork::computeLength(list<AbstractLink*>&path)
{
#ifdef DEBUGB
	cout << "-> computeLatency" << endl;
#endif // DEBUGB

	float length = 0;
	list<AbstractLink*>::const_iterator itr = path.begin();

	//-B: scan every simplex link of path
	while (itr != path.end())
	{
		SimplexLink *pLink = (SimplexLink*)(*itr);
		assert(pLink);

		switch (pLink->m_eSimplexLinkType)
		{
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Lightpath:
			{
#ifdef DEBUGB
				if (pLink->m_eSimplexLinkType == SimplexLink::LT_Channel)
				{
					cout << "\t+ Fiber's length = " << pLink->m_pUniFiber->getLength() << endl;
				}
				else if (pLink->m_eSimplexLinkType == SimplexLink::LT_Lightpath)
				{
					cout << "\t+ Lightpath's length = " << pLink->getLength() << endl;
				}
				else
				{
					cout << "LT_UniFiber??????" << endl;
				}
#endif // DEBUGB
				length += pLink->getLength();
				break;
			}
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Converter:
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Rx:
				NULL;   // No need to do anything for these cases
				break;
			default:
				DEFAULT_SWITCH;
		} // end switch
		itr++;
	} // end while

#ifdef DEBUGB
	cout << "\tPath length = " << length << endl;
#endif // DEBUGB
	return length;
}

UINT WDMNetwork::countActiveNodes()
{
	UINT count = this->BBUs.size();
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	//-B: placeBBUs in core CO does not cost anything
	//-B: if core CO has BBUs, it will be present in BBUs list --> decrease count var by 1
	if (((OXCNode*)(this->lookUpNodeById(this->DummyNode)))->m_nBBUs > 0)
		count--;

	//-B: count all nodes that have their BBU directly in its cell site
	//-B: scan all node of the networks
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		//select only NO candidate hotel nodes
		if (!pNode->getBBUHotel())
		{
			//if it hosts its own BBU
			if (pNode->m_nBBUs > 0)
			{
				//increase the value of the num of active nodes
				count++;
			}
		}
	}
	return count;
}

UINT WDMNetwork::countActivePools(ConnectionDB & connDB)
{
	UINT count = this->BBUs.size();
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	//-B: placeBBUs in core CO does not cost anything
	//-B: if core CO has BBUs, it will be present in BBUs list --> decrease count var by 1
	if (((OXCNode*)(this->lookUpNodeById(this->DummyNode)))->m_nBBUs > 0)
		count--;

	//-B: count all nodes that have their BBU directly in its cell site
	//-B: scan all node of the networks
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		//select only NO candidate hotel nodes
		if (!pNode->getBBUHotel())
		{
			//if it hosts its own BBU
			if (pNode->m_nBBUs > 0)
			{
				//increase the value of the num of active nodes
				count++;
			}
		}
	}

	UINT count2 = 0;
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		//-B: core CO does not cost anything
		if (pNode->getId() == this->DummyNode)
			continue;

		this->computeTrafficProcessed_BBUPooling(pNode, connDB.m_hConList);
		if (pNode->m_dTrafficProcessed > 0)
			count2++;
	}
	
	assert(count == count2);
	return count;
}


void WDMNetwork::restoreNodeCost()
{
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		pNode->resetCost();
	}
}

void WDMNetwork::computeShortestPaths(Graph&m_hGraph)
{
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;
	Vertex*pVSrc, *pVDst;
	LINK_COST cost = UNREACHABLE;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		pVSrc = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_Out, -1);
		pVDst = m_hGraph.lookUpVertex(this->DummyNode, Vertex::VertexType::VT_Access_In, -1);
		//it is computed here and can be used during all the simulations, if needed
		m_hGraph.Dijkstra(pNode->pPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
#ifdef DEBUGB
		cout << "\tCosto " << pNode->getId() << "->" << this->DummyNode << " = " << cost << endl;
#endif // DEBUGB
	}
}

//-B: for each node, it computes its network stage as distance from the core CO
//	it exploits the work done by computeShortestPaths method
void WDMNetwork::setNodesStage(Graph&m_hGraph)
{
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;
	Vertex*pVSrc, *pVDst;
	list<AbstractLink*> hPrimaryPath;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
			pNode = (OXCNode*)(*itr);
			pVSrc = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_Out, -1);
			pVDst = m_hGraph.lookUpVertex(this->DummyNode, Vertex::VertexType::VT_Access_In, -1);
			//-B: var m_nNetStage is used as measure of the distance between one of the hotel node and the core CO
			//	it is computed here and can be used during all the simulations, if needed
			pNode->m_nNetStage = m_hGraph.Dijkstra(pNode->pPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost); 
			pNode->m_fDistanceFromCoreCO = computeLength(hPrimaryPath);
			//recordMinCostPath(pNode->pPath, pVDst); //pPath computed in computeShortestPaths method --> IT DOESN'T WORK, OBVIOUSLY
	}

#ifdef DEBUGB
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		cout << "\tNodo #" << pNode->getId() << " - stage: " << pNode->m_nNetStage << endl;
	}
	cin.get();
#endif // DEBUGB
}

//-B: for each node, it computes its degree of reachability
void WDMNetwork::setReachabilityDegree(Graph&m_hGraph)
{
	/*
		while (!sortedHotelsList.empty())
	{
		pNode = sortedHotelsList.peekMin();
		sortedHotelsList.popMin();
#ifdef DEBUGB
		cout << "\tNodo " << pNode->getId() << ": costo = " << pNode->m_nBBUReachCost << endl;
#endif // DEBUGB
		auxHotelsList.push_back(pNode);
	}
	*/

	list<AbstractNode*>::const_iterator itrNode;
	vector<OXCNode*>::iterator itr;
	OXCNode*pNode, *pOXC;
	Vertex*pVSrc, *pVDst;
	list<AbstractLink*> hPrimaryPath;

	//-B: PRE-PROCESSING
	//empty list
	//hotelsList.clear();
	//prefer wavel path over wavel conversion
	m_hGraph.preferWavelPath();

	//------------------------- NEXT STEPS -----------------------------
	//-B: fill again the list with the sorted elements
	//-B: compute the number of nodes that can reach each hotel node
	//------------------------------------------------------------------
	LINK_COST savedCost = UNREACHABLE, referenceCost = UNREACHABLE, distCost = UNREACHABLE;
	// scan all hotels nodes: they will be the destination nodes
	for (itr = this->hotelsList.begin(); itr != this->hotelsList.end(); itr++)
	{
		pNode = ((OXCNode*)(*itr));
		pVDst = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_In, -1);

		//cost between the hotel and the core CO
		//savedCost = pNode->m_nBBUReachCost;
#ifdef DEBUGB
		cout << "\nCONSIDERO L'HOTEL NODE #" << pNode->getId() << endl;
		cout << "Calcolo il suo grado di reachability" << endl;
		//cin.get();
#endif // DEBUGB
		//scan all nodes of the network
		for (itrNode = this->m_hNodeList.m_hList.begin(); itrNode != this->m_hNodeList.m_hList.end(); itrNode++)
		{
			pOXC = (OXCNode*)(*itrNode);
			// consider only mobile or fixed-mobile nodes
			if (this->isMobileNode(pOXC->getId()) || this->isFixMobNode(pOXC->getId()))
			{
				pVSrc = m_hGraph.lookUpVertex(pOXC->getId(), Vertex::VertexType::VT_Access_Out, -1);
				//-B: here we cannot use pNode->pPath (already computed in computeShortestPaths) because we are not computing the distance from the corec CO
				referenceCost = m_hGraph.DijkstraLatency(hPrimaryPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
				//referenceCost = m_hGraph.Dijkstra(hPrimaryPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
				//if it the dst is reachable from this selected node
				if (referenceCost < UNREACHABLE)
				{
					if (computeLatency(hPrimaryPath) <= LATENCYBUDGET)
					{
						//increase node's reachability degree
						pNode->m_nReachabilityDegree++;
#ifdef DEBUGB
						cout << "\tADMISSIBLE LATENCY!" << endl;
#endif // DEBUGB
					}
				} //end IF reachable
				  //else: nothing to do
			} //end IF isMobile OR isFixMob
			  //else: nothing to do
		} //end FOR network nodes list

#ifdef DEBUGB
		cout << "\tGrado di reachability: " << pNode->m_nReachabilityDegree << endl;
#endif // DEBUGB
		
	} //end FOR auxHotelsList

#ifdef DEBUGB
	for (itr = hotelsList.begin(); itr != hotelsList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		cout << pNode->getId() << " (" << pNode->m_nReachabilityDegree << " r)" << endl;
	}
	cin.get();
#endif // DEBUGB

}

//reset some vars after preprocessing
void WDMNetwork::resetPreProcessing()
{
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;
	list<AbstractLink*> hPrimaryPath;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		//pNode->m_nReachabilityDegree --> REMAINS AS IT IS
		//pNode->m_nProximityDegree --> REMAINS AS IT IS --> OBSOLETE
		//pNode->m_nNetStage --> REMAINS AS IT IS
		//pNode->m_nBetweennessCentrality--> REMAINS AS IT IS
		pNode->m_dCostMetric = UNREACHABLE;
		pNode->m_nBBUReachCost = UNREACHABLE;
		pNode->pPath.clear();
	}
}

//-B: for each node, it computes its degree of "proximity"
void WDMNetwork::setProximityDegree(Graph&m_hGraph)
{
	Vertex*pVSrc, *pVDst;
	OXCNode*pNode, *pOXC;
	list<AbstractLink*> hPrimaryPath;
	list<AbstractNode*>::const_iterator itrNode;
	vector<OXCNode*>::iterator itr;

	//-B: PRE-PROCESSING
	//empty list
	//hotelsList.clear();
	//prefer wavel path over wavel conversion
	m_hGraph.preferWavelPath();

	//------------------------- NEXT STEPS -----------------------------
	//-B: fill again the list with the sorted elements
	//-B: compute the number of nodes that can reach each hotel node
	//------------------------------------------------------------------
	LINK_COST referenceCost = UNREACHABLE, distCost = UNREACHABLE;
	//-B: scan all network's nodes
	for (itrNode = this->m_hNodeList.begin(); itrNode != this->m_hNodeList.end(); itrNode++)
	{
		//reset reference cost
		referenceCost = UNREACHABLE;
		//SOURCE NODE
		pOXC = (OXCNode*)(*itrNode);
		// consider only mobile or fixed-mobile nodes
		if (this->isMobileNode(pOXC->getId()) || this->isFixMobNode(pOXC->getId()))
		{
			//SOURCE VERTEX
			pVSrc = m_hGraph.lookUpVertex(pOXC->getId(), Vertex::VertexType::VT_Access_Out, -1);

			//-B: scan all hotel nodes
			for (itr = hotelsList.begin(); itr != hotelsList.end(); itr++)
			{
				//DESTINATION NODE
				pNode = (OXCNode*)(*itr);
				//if it is not the same source node
				if (pNode->getId() != pOXC->getId())
				{
					//DST VERTEX
					pVDst = m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_In, -1);
					//-B: here the var BBUReachCost is used as measure of the distance between one of the node and one of the hotel
					distCost = m_hGraph.Dijkstra(hPrimaryPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
					//if it the dst is reachable from this selected node
					if (distCost < UNREACHABLE)
					{
						//if the latency constraint is respected
						if (computeLatency(hPrimaryPath) <= LATENCYBUDGET)
						{
							//save cost in dst node
							pNode->m_nBBUReachCost = distCost;
							//if it is lower than the minimum cost
							if (distCost < referenceCost)
							{
								//save it as minimum cost
								referenceCost = distCost;
							}
						}
					}
				}
			}//end FOR hotels
			 //-B: scan again all hotels to add the proximity degree
			for (itr = hotelsList.begin(); itr != hotelsList.end(); itr++)
			{
				pNode = (OXCNode*)(*itr);
				if (pNode->getId() != pOXC->getId())
				{
					//if node reach. cost is the minimum
					if (pNode->m_nBBUReachCost == referenceCost)
					{
						pNode->m_nProximityDegree++;
#ifdef DEBUGB
						cout << "\tNODO SORGENTE #" << pOXC->getId() << " - Incremento il reachability degree del nodo " << pNode->getId()
							<< " con costo " << pNode->m_nBBUReachCost << ": " << pNode->m_nProximityDegree << "r" << endl;
						//cin.get();
#endif // DEBUGB
					}
				}
			}
		}
		//else: go to the next node
	}//end FOR nodes

#ifdef DEBUGB
	for (itr = hotelsList.begin(); itr != hotelsList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		cout << pNode->getId() << " (" << pNode->m_nProximityDegree << "p)" << endl;
	}
	cin.get();
#endif // DEBUGB
}

//
//	it exploits the work done by computeShortestPaths method
void WDMNetwork::setBetweennessCentrality(Graph&m_hGraph)
{
	vector<OXCNode*>::const_iterator itr;
	OXCNode*pOXC, *pNetNode;
	Vertex*pVertex;
	list<AbstractNode*>::const_iterator itrNode;
	list<AbstractLink*>::const_iterator itrL;
	SimplexLink *pLink;
	AbstractNode*pAbstract;
	this->m_nAvgNodesCrossed = 0;
	UINT nodesCrossed = 0;

	Vertex*pVDst = m_hGraph.lookUpVertex(this->DummyNode, Vertex::VT_Access_In, -1);

	for (itrNode = this->m_hNodeList.begin(); itrNode != this->m_hNodeList.end(); itrNode++)
	{
		pNetNode = (OXCNode*)(*itrNode);

		if (pNetNode->getId() == this->DummyNode)
		{
			pNetNode->m_nBetweennessCentrality = this->numberOfNodes;
			continue;
		}
		UINT hop = 0;
		Vertex*pVSrc = m_hGraph.lookUpVertex(pNetNode->getId(), Vertex::VT_Access_Out, -1);
		hop = m_hGraph.Dijkstra(pNetNode->pPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost); //cost not needed
		nodesCrossed += (hop - 1);
		//-B: scan all hotel nodes
		//for (itr = hotelsList.begin(); itr != this->hotelsList.end(); itr++)
		//{
			//pNode = (OXCNode*)(*itr);
			//-B: scan all links included in the shortest path going from pNode to the coreCO (already computed)
			for (itrL = pNetNode->pPath.begin(); itrL != pNetNode->pPath.end(); itrL++)
			{
				pLink = (SimplexLink*)(*itrL);
				switch (pLink->getSimplexLinkType())
				{
					case SimplexLink::LT_Channel_Bypass:
					case SimplexLink::LT_Converter:
					case SimplexLink::LT_Tx:
					case SimplexLink::LT_Grooming:
					case SimplexLink::LT_Rx:
						break;
					case SimplexLink::LT_Lightpath:
					case SimplexLink::LT_Channel:
					case SimplexLink::LT_UniFiber:
					{
						pAbstract = pLink->getSrc();
						//get soure vertex
						pVertex = (Vertex*)(pAbstract);
						//get source OXC node
						pOXC = pVertex->m_pOXCNode;
						if (pOXC->getBBUHotel())
							pOXC->m_nBetweennessCentrality++;
						break;
					}

					default:
						DEFAULT_SWITCH;
				} //end SWITCH
			} //end FOR links
		//} //end FOR hotel nodes
	}//end FOR network nodes
	this->m_nAvgNodesCrossed = ((double)nodesCrossed / ((double)(this->numberOfNodes - 1)));
	if ((ceil(this->m_nAvgNodesCrossed)) - this->m_nAvgNodesCrossed <= (this->m_nAvgNodesCrossed - floor(this->m_nAvgNodesCrossed)))
		this->m_nAvgNodesCrossed = ceil(this->m_nAvgNodesCrossed);
	else
		this->m_nAvgNodesCrossed = floor(this->m_nAvgNodesCrossed);

#ifdef DEBUGB
	for (itr = this->hotelsList.begin(); itr != this->hotelsList.end(); itr++)
	{
		pNetNode = (OXCNode*)(*itr);
		cout << "\tNodo #" << pNetNode->getId() << " (" << pNetNode->m_nBetweennessCentrality << "bc)" << endl;
	}
	cout << endl << "crossedNodes = " << nodesCrossed << endl;
	cout << "Avg num of nodes crossed = " << this->m_nAvgNodesCrossed << endl;
	//cin.get();
#endif // DEBUGB
}

void WDMNetwork::genAreasOfNodes()
{
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		switch (pNode->getArea())
		{
			case OXCNode::UrbanArea::Office:
			{
				this->OfficeNodes.push_back(pNode);
				break;
			}
			case OXCNode::UrbanArea::Residential:
			{
				this->ResidentialNodes.push_back(pNode);
				break;
			}
			default:
				DEFAULT_SWITCH;
		} //end SWITCH
	}//end FOR
}

void WDMNetwork::extractSameStageNodes(vector<OXCNode*>&auxHotelsList, list<OXCNode*>&sameStageNodes, LINK_COST nStage)
{
	vector<OXCNode*>::const_iterator itr;
	list<OXCNode*>::const_iterator itr2;
	OXCNode*pNode;

	for (itr = auxHotelsList.begin(); itr != auxHotelsList.end(); itr = auxHotelsList.begin())
	{
		pNode = (OXCNode*)(*itr);
		if (pNode->m_nNetStage == nStage)
		{
			sameStageNodes.push_back(pNode);
			auxHotelsList.erase(itr);
		}
		else
		{
			break;
		}
	}
}

void WDMNetwork::extractSameReachDegreeNodes(list<OXCNode*>&sameStageNodes, list<OXCNode*>&sameReachDegreeNodes, int reachDegree)
{
	list<OXCNode*>::const_iterator itr;
	OXCNode*pNode;

	for (itr = sameStageNodes.begin(); itr != sameStageNodes.end(); itr = sameStageNodes.begin())
	{
		pNode = (OXCNode*)(*itr);
		if (pNode->m_nReachabilityDegree == reachDegree)
		{
			sameReachDegreeNodes.push_back(pNode);
			sameStageNodes.erase(itr);
		}
		else
		{
			break;
		}
	}
}

void WDMNetwork::sortHotelsByDistance(vector<OXCNode*>&auxHotelsList)
{
	OXCNode*pNode, *pOXC, *pAuxNode, *pDebug;
	BinaryHeap<OXCNode*, vector<OXCNode*>, POXCNodeDistanceComp> partialHotelsList;
	vector<OXCNode*>::iterator itr;
	list<OXCNode*>::const_iterator itr2, itr3, itrDebug;

	//reset auxiliary hotels list
	auxHotelsList.clear();
	//fill it with hotels list's elements
	for (itr = hotelsList.begin(); itr != hotelsList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		auxHotelsList.push_back(pNode);
	}

	list<OXCNode*> sameStageNodes, sameReachDegreeNodes;
	//empty hotels list
	hotelsList.clear();

	for (itr = auxHotelsList.begin(); itr != auxHotelsList.end(); )
	{
		pNode = (OXCNode*)(*itr);
		sameStageNodes.clear();
		extractSameStageNodes(auxHotelsList, sameStageNodes, pNode->m_nNetStage);
#ifdef DEBUGB
		cout << "\n\t+++++++Same stage nodes list:" << endl;
		for (itrDebug = sameStageNodes.begin(); itrDebug != sameStageNodes.end(); itrDebug++)
		{
			pDebug = (OXCNode*)(*itrDebug);
			cout << "\t#" << pDebug->getId() << " - stage = " << pDebug->m_nNetStage << endl;
		}
#endif // DEBUGB
		for (itr2 = sameStageNodes.begin(); itr2 != sameStageNodes.end(); )
		{
			pOXC = (OXCNode*)(*itr2);
			sameReachDegreeNodes.clear();
			extractSameReachDegreeNodes(sameStageNodes, sameReachDegreeNodes, pOXC->m_nReachabilityDegree);
#ifdef DEBUGB
			cout << "\n\t++++++++Same reach degree nodes list:" << endl;
			for (itrDebug = sameReachDegreeNodes.begin(); itrDebug != sameReachDegreeNodes.end(); itrDebug++)
			{
				pDebug = (OXCNode*)(*itrDebug);
				cout << "\t#" << pDebug->getId() << " - reach degree = " << pDebug->m_nReachabilityDegree << endl;
			}
#endif // DEBUGB
			for (itr3 = sameReachDegreeNodes.begin(); itr3 != sameReachDegreeNodes.end(); itr3++)
			{
				pAuxNode = (OXCNode*)(*itr3);
				partialHotelsList.insert(pAuxNode);
			}
			while (!partialHotelsList.empty())
			{
				pAuxNode = partialHotelsList.peekMin();
				partialHotelsList.popMin();
#ifdef DEBUGB
				cout << "\tINSERISCO IN HOTELS LIST: Nodo #" << pAuxNode->getId() << ": stage = " << pAuxNode->m_nNetStage
					<< " - reach degree = " << pAuxNode->m_nReachabilityDegree << " - distance from core CO = " << pAuxNode->m_fDistanceFromCoreCO << endl;
				//cin.get();
#endif // DEBUGB
				hotelsList.push_back(pAuxNode);
			}
			itr2 = sameStageNodes.begin();
		}
		itr = auxHotelsList.begin();
	} //end FOR auxHotelsList

}

//-B: method called after a backhaul arrival (and successfully provisioned!) OR after a fronthaul departure
void WDMNetwork::updateTrafficPerNode(Event*pEvent)
{
	OXCNode*pSrc;
	if (pEvent->m_hEvent == Event::EVT_ARRIVAL)
	{
		if (pEvent->m_pConnection->m_eConnType == Connection::MOBILE_BACKHAUL || pEvent->m_pConnection->m_eConnType == Connection::FIXEDMOBILE_BACKHAUL)
		{
			//-B: be sure that is a backhaul event
			//-L: check also the midhaul
			assert(pEvent->fronthaulEvent && pEvent->midhaulEvent);
			//-B: original source node is the source of the corresponding fronthaul connection
			pSrc = (OXCNode*)(this->lookUpNodeById(pEvent->fronthaulEvent->m_pConnection->m_nSrc));
		}
		else if (pEvent->m_pConnection->m_eConnType == Connection::FIXED_BACKHAUL)
		{
			pSrc = (OXCNode*)(this->lookUpNodeById(pEvent->m_pConnection->m_nSrc));
		}
		//-L: midhaul case
		else if (pEvent->m_pConnection->m_eConnType == Connection::FIXED_MIDHAUL)
		{
			//-L: to be sure it's a midhaul 
			assert(pEvent->fronthaulEvent);
			//-B: original source node is the source of the corresponding fronthaul connection
			pSrc = (OXCNode*)(this->lookUpNodeById(pEvent->fronthaulEvent->m_pConnection->m_nSrc));
		}

		//-B: increase traffic generated by this source node, taking into account only the backhaul bandwith (the "payload")
		pSrc->m_dTrafficGen += pEvent->m_pConnection->m_eBandwidth; //pEvent->m_pConnection->m_eBandwidth == pEvent->fronthaulEvent->m_pConnection->m_eBandwidth
	}
	else //EVT_DEPARTURE
	{
		//-B: be sure that is a fronthaul event
		assert(pEvent->fronthaulEvent == NULL);
		//-B: original source node is the source of this fronthaul connection
		pSrc = (OXCNode*)(this->lookUpNodeById(pEvent->m_pConnection->m_nSrc));
		//-B: increase traffic generated by this source node, taking into account only the backhaul bandwith (the "payload")
		pSrc->m_dTrafficGen -= pEvent->m_pConnection->m_eBandwidth;
	}

	//-B: CHECK: following lines don't do anything, just control
	/*
		if (pSrc->isSmallCell())
	{
		if (pSrc->m_dTrafficGen < 0 || pSrc->m_dTrafficGen > MAXTRAFFIC_SMALLCELL)
			cout << "\tATTENTION! Traffico generato dalla SMALL cell #" << pSrc->getId() << " = " << pSrc->m_dTrafficGen << endl;
		assert(pSrc->m_dTrafficGen >= 0);
		assert(pSrc->m_dTrafficGen <= MAXTRAFFIC_SMALLCELL);
	}
	else //macro-cell
	{
		if (pSrc->m_dTrafficGen < 0 || pSrc->m_dTrafficGen > MAXTRAFFIC_MACROCELL)
			cout << "\tATTENTION! Traffico generato dalla MACRO cell #" << pSrc->getId() << " = " << pSrc->m_dTrafficGen << endl;
		assert(pSrc->m_dTrafficGen >= 0);
		assert(pSrc->m_dTrafficGen <= MAXTRAFFIC_MACROCELL);
	}
	*/

	//-B: after changing the way of representing small cells, we have to modify PREVIOUS check
	if (pSrc->isMacroCell() && this->isMobileNode(pSrc->getId()))
	{
		if (pSrc->m_dTrafficGen < 0 || pSrc->m_dTrafficGen > (MAXTRAFFIC_MACROCELL + (SMALLCELLS_PER_MC * MAXTRAFFIC_SMALLCELL)))
			cout << "\tATTENTION! Traffico generato dalla MACRO cell #" << pSrc->getId() << " = " << pSrc->m_dTrafficGen << endl;
		if (pSrc->m_dTrafficGen < 0)
			cin.get();
		assert(pSrc->m_dTrafficGen >= 0);
		assert(pSrc->m_dTrafficGen <= (MAXTRAFFIC_MACROCELL + (SMALLCELLS_PER_MC * MAXTRAFFIC_SMALLCELL)));
	}
}

void WDMNetwork::extractPotentialSourcesMC_Nconn(bool officeSource, vector<UINT>&potentialSources, BandwidthGranularity bwdMC,
	double bwdSC)
{
#ifdef DEBUGB
	cout << "-> extractPotentialSources_MIDHAUL" << endl;
#endif // DEBUGB
	//-B: reset list
	potentialSources.clear();

	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;
	vector<OXCNode*>::const_iterator itr2;
	list<Connection*>::const_iterator itrCon;

	//-B: scan all network nodes
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);

		//-B: assuming that core CO cannot be a source node of any connection
		if (pNode->getId() == this->DummyNode)
			continue;

		//-B: if officeSource is true, it means that only macro cells belonging to office area can be chosen
		if (officeSource)
		{
			//-B: only consider macro cells belonging to OFFICE area
			if (pNode->getArea() == OXCNode::UrbanArea::Office)
			{
				//-B: if this node is a macro cell
				if (pNode->isMacroCell())
				{
					//-B: if currently it doesn't generate too much traffic to host the following connection
					if (pNode->m_dTrafficGen <= MAXTRAFFIC_MACROCELL - bwdMC)
					{
						potentialSources.push_back(pNode->getId());
					}
					else //it could be chosen as source node only if at least one of its small cells is not a source
					{
						//macro cell that are hotel nodes don't have small cells, but macro cells
						if (!pNode->getBBUHotel())
						{
							int countSourceSC = 0;
							//check for its small cells
							for (itr2 = pNode->m_vSmallCellCluster.begin(); itr2 != pNode->m_vSmallCellCluster.end(); itr2++)
							{
								//-B: if currently it hasn't reached its maximum capacity yet
								if ((*itr2)->m_dTrafficGen < MAXTRAFFIC_SMALLCELL)
								{
									///-B: increase the number of small cells not having overcome their maximum capacity
									countSourceSC++;
								}
							} //end FOR small cells
							if (countSourceSC > 0)
							{
								potentialSources.push_back(pNode->getId());
							}
						}
					}
				}
				//-B: if this node is a small cell
				else
				{
					continue;
				}
			}
		}
		else
		{
			//-B: only consider macro cells belonging to RESIDENTIAL area
			if (pNode->getArea() == OXCNode::UrbanArea::Residential)
			{
				//-B: if this node is a macro cell
				if (pNode->isMacroCell())
				{
					//-B: if currently it doesn't generate too much traffic to host the following connection
					if (pNode->m_dTrafficGen <= MAXTRAFFIC_MACROCELL - bwdMC)
					{
						potentialSources.push_back(pNode->getId());
					}
					else //it could be chosen as source node only if at least one of its small cells is not a source
					{
						//macro cell that are hotel nodes don't have small cells, but macro cells
						if (!pNode->getBBUHotel())
						{
							int countSourceSC = 0;
							//check for its small cells
							for (itr2 = pNode->m_vSmallCellCluster.begin(); itr2 != pNode->m_vSmallCellCluster.end(); itr2++)
							{
								//-B: if currently it hasn't reached its maximum capacity yet
								if ((*itr2)->m_dTrafficGen < MAXTRAFFIC_SMALLCELL)
								{
									///-B: increase the number of small cells not having overcome their maximum capacity
									countSourceSC++;
								}
							} //end FOR small cells
							if (countSourceSC > 0)
							{
								potentialSources.push_back(pNode->getId());
							}
						}
					}
				}
				//-B: if this node is a small cell
				else
				{
					continue;
				}
			}
		}
	} //end FOR nodes

	if (potentialSources.size() == 0)
	{
		cout << endl << "\tATTENTION! NETWORK FULL! ALL NODES HAVE REACHED THEIR MAXIMUM CAPACITY" << endl;
		for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
		{
			pNode = (OXCNode*)(*itr);
			cout << "\tNodo #" << pNode->getId() << " - traffico generato: " << pNode->m_dTrafficGen << endl;
		}
		cin.get();
		cin.get();
	}

#ifdef DEBUGB
	cout << "\tLista dei possibili source nodes: ";
	int i;
	for (i = 0; i < potentialSources.size(); i++)
	{
		cout << potentialSources[i] << ", ";
	}
	cout << endl;
#endif // DEBUGB
}

//-B: difference with previous method: a node that is already source of a current connection cannot be source of the new one
void WDMNetwork::extractPotentialSourcesMC_1conn(bool officeSource, vector<UINT>&potentialSources, MappedLinkList<UINT, Connection*>&connectionList)
{
#ifdef DEBUGB
	cout << "-> extractPotentialSources_FRONTHAUL" << endl;
#endif // DEBUGB

	//-B: reset list
	potentialSources.clear();

	list<AbstractNode*>::const_iterator itr;
	vector<OXCNode*>::const_iterator itr2;
	list<Connection*>::const_iterator itrCon;
	OXCNode*pNode;

	//-B: scan all network nodes
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);

		//-B: assuming that core CO cannot be a source node of any connection
		if (pNode->getId() == this->DummyNode)
			continue;

		bool alreadySource = false;

		//-B: if officeSource is true, it means that only macro cells belonging to office area can be chosen
		if (officeSource)
		{
			//-B: only consider macro cells belonging to OFFICE area
			if (pNode->getArea() == OXCNode::UrbanArea::Office)
			{
				//-B: only consider macro cell nodes
				if (pNode->isMacroCell())
				{
					//if this macro cell is not a source, it can be chosen as potential source
					if (!(this->isAlreadySource(pNode->getId(), connectionList)))
					{
						assert(pNode->m_dTrafficGen == 0);
						potentialSources.push_back(pNode->getId());
					}
					else //it could be chosen as source node only if at least one of its small cells is not a source
					{
						//-B: macro cell that are hotel nodes don't have any small cells, but macro cells
						if (!pNode->getBBUHotel())
						{
							int countSourceSC = 0;
							//-B: check for its small cells
							for (itr2 = pNode->m_vSmallCellCluster.begin(); itr2 != pNode->m_vSmallCellCluster.end(); itr2++)
							{
								//-B: if its small cell is already source of a FRONTHAUL connection
								if (this->isAlreadySource((*itr2)->getId(), connectionList))
								{
									//-B: increase the number of small cells already source of a connection
									countSourceSC++;
								}
							} //end FOR small cells
							  //-B: if there is at least 1 small cell without any connection
							if (countSourceSC < pNode->m_vSmallCellCluster.size())
							{
								//-B: the macro cell could be selected (-> one of its small cells could be selected as source)
								potentialSources.push_back(pNode->getId());
							}
						}
					}
				}
				else //if small cell
				{
					continue;
				}
			}
		}
		//source must belong to office area
		else
		{
			//-B: only consider macro cells belonging to RESIDENTIAL area
			if (pNode->getArea() == OXCNode::UrbanArea::Residential)
			{
				//-B: only consider macro cell nodes
				if (pNode->isMacroCell())
				{
					//if this macro cell is not a source, it can be chosen as potential source
					if (!(this->isAlreadySource(pNode->getId(), connectionList)))
					{
						assert(pNode->m_dTrafficGen == 0);
						potentialSources.push_back(pNode->getId());
					}
					else //it could be chosen as source node only if at least one of its small cells is not a source
					{
						//-B: macro cell that are hotel nodes don't have any small cells, but macro cells
						if (!pNode->getBBUHotel())
						{
							int countSourceSC = 0;
							//-B: check for its small cells
							for (itr2 = pNode->m_vSmallCellCluster.begin(); itr2 != pNode->m_vSmallCellCluster.end(); itr2++)
							{
								//-B: if its small cell is already source of a FRONTHAUL connection
								if (this->isAlreadySource((*itr2)->getId(), connectionList))
								{
									//-B: increase the number of small cells already source of a connection
									countSourceSC++;
								}
							} //end FOR small cells
							  //-B: if there is at least 1 small cell without any connection
							if (countSourceSC < pNode->m_vSmallCellCluster.size())
							{
								//-B: the macro cell could be selected (-> one of its small cells could be selected as source)
								potentialSources.push_back(pNode->getId());
							}
						}
					}
				}
				else //if small cell
				{
					continue;
				}
			}
		}
	} //end FOR nodes

	if (potentialSources.size() == 0)
	{
		cout << endl << "\tATTENTION! NETWORK FULL! ALL NODES HAVE REACHED THEIR MAXIMUM CAPACITY" << endl;
		for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
		{
			pNode = (OXCNode*)(*itr);
			cout << "\tNodo #" << pNode->getId() << " - traffico generato: " << pNode->m_dTrafficGen << endl;
		}
		//-B: scan all the FRONTHAUL connections
		cout << "FRONTHAUL connections' list" << endl;
		for (itrCon = connectionList.begin(); itrCon != connectionList.end(); itrCon++)
		{
			if ((*itrCon)->m_eConnType == Connection::MOBILE_FRONTHAUL || (*itrCon)->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
			{
				(*itrCon)->dump(cout);
			}
		}
		cin.get();
		cin.get();
	}

#ifdef DEBUGB
	cout << "\tLista dei possibili source nodes: ";
	int i;
	for (i = 0; i < potentialSources.size(); i++)
	{
		cout << potentialSources[i] << ", ";
	}
	cout << endl;
#endif // DEBUGB
}


void WDMNetwork::computeTrafficProcessed_BBUStacking(OXCNode*pNode)
{
	//-B: reset traffic value
	pNode->m_dTrafficProcessed = 0;
	
	if (!(pNode->getBBUHotel()))
		return;

	list<AbstractNode*>::const_iterator itr;
	OXCNode*pOXC;

	//-B: scan all nodes
	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pOXC = (OXCNode*)(*itr);
		//-B: if node pOXC has a valid BBU hotel node ID assigned and it is the selected node pNode
		if (pOXC->m_nBBUNodeIdsAssigned == pNode->getId())
		{
			//-B: increase traffic processed, only considering BACKHAUL traffic
			pNode->m_dTrafficProcessed += pOXC->m_dTrafficGen;
		}
	}
}


void WDMNetwork::computeTrafficProcessed_BBUPooling(OXCNode*pNode, MappedLinkList<UINT, Connection*>&connectionList)
{
	//-B: reset traffic value
	pNode->m_dTrafficProcessed = 0;

	if (!(pNode->getBBUHotel()))
		return;

	list<Connection*>::const_iterator itr;
	Connection*pCon;

	//-B: scan all connections
	for (itr = connectionList.begin(); itr != connectionList.end(); itr++)
	{
		pCon = (Connection*)(*itr);
		switch (pCon->m_eConnType)
		{
			case Connection::FIXEDMOBILE_BACKHAUL:
			case Connection::FIXED_BACKHAUL:
			case Connection::MOBILE_BACKHAUL:
				break;
			case Connection::FIXEDMOBILE_FRONTHAUL:
			case Connection::MOBILE_FRONTHAUL:
			{
				//if dst node of this fronthaul connection is the selected node pNode
				if (pCon->m_nDst == pNode->getId())
				{
					//-B: increase traffic processed, only considering BACKHAUL traffic
					pNode->m_dTrafficProcessed += pCon->m_eBandwidth;
				}
				break;
			}
		} //end SWITCH connection type
	} //end FOR connections
}


void WDMNetwork::extractPotentialSourcesSC_1conn(vector<UINT>&potentialSources, MappedLinkList<UINT, Connection*>&connectionList, UINT macroCellID)
{
#ifdef DEBUGB
	cout << "-> extractPotentialSources_SmallCell" << endl;
#endif // DEBUGB

	//-B: reset list
	potentialSources.clear();

	OXCNode*pNode = (OXCNode*)this->lookUpNodeById(macroCellID);
	vector<OXCNode*>::const_iterator itr;

	for (itr = pNode->m_vSmallCellCluster.begin(); itr != pNode->m_vSmallCellCluster.end(); itr++)
	{
		if (this->isAlreadySource((*itr)->getId(), connectionList))
		{
			continue;
		}
		else //free small cell -> potential source
		{
			potentialSources.push_back((*itr)->getId());
		}
	}
}

void WDMNetwork::extractPotentialSourcesSC_Nconn(vector<UINT>&potentialSources, double bwdSC, UINT macroCellID)
{
#ifdef DEBUGB
	cout << "-> extractPotentialSources_SmallCell" << endl;
#endif // DEBUGB

	//-B: reset list
	potentialSources.clear();

	OXCNode*pNode = (OXCNode*)this->lookUpNodeById(macroCellID);
	vector<OXCNode*>::const_iterator itr;

	for (itr = pNode->m_vSmallCellCluster.begin(); itr != pNode->m_vSmallCellCluster.end(); itr++)
	{
		if ((*itr)->m_dTrafficGen >= MAXTRAFFIC_SMALLCELL)
		{
			continue;
		}
		else //free small cell -> potential source
		{
			potentialSources.push_back((*itr)->getId());
		}
	}
}

bool WDMNetwork::isAlreadySource(UINT nodeID, MappedLinkList<UINT, Connection*>&connectionList)
{
	//-B: CHECK IF THS MACRO CELL SELECTED AS SOURCE IS ALREADY A SOURCE NODE
	list<Connection*>::const_iterator itrCon;
	bool alreadySource = false;

	for (itrCon = connectionList.begin(); itrCon != connectionList.end(); itrCon++)
	{
		if ((*itrCon)->m_eConnType == Connection::MOBILE_FRONTHAUL || (*itrCon)->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
		{
			if ((*itrCon)->m_nSrc == nodeID)
			{
				alreadySource = true;
				break;
			}
		}
	} //end FOR connections
	return alreadySource;
}

UINT WDMNetwork::countActiveSmallCells()
{
	UINT activeSC = 0;
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		for (int threshold = MAXTRAFFIC_MACROCELL; threshold < (MAXTRAFFIC_MACROCELL + (MAXTRAFFIC_SMALLCELL*SMALLCELLS_PER_MC));
			threshold += MAXTRAFFIC_SMALLCELL)
		{
			if (pNode->m_dTrafficGen > threshold)
			{
				activeSC++;
			}
		}
	}
	return activeSC;
}

void WDMNetwork::logHotelsActivation(SimulationTime hTimeSpan)
{
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		if (pNode->m_nBBUs > 0)
		{
			pNode->m_dActivityTime += hTimeSpan;
		}
	}
}

void WDMNetwork::resetHotelStats()
{
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;

	for (itr = this->m_hNodeList.begin(); itr != this->m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		pNode->resetActivityTime();
		pNode->m_vLogHotels.clear();
	}
}

void WDMNetwork::computeNumOfBBUSupported(UINT offeredTraffic_MC)
{
	vector<OXCNode*>::const_iterator itr;
	OXCNode*pNode;
	list<AbstractLink*>::const_iterator itrLink;
	UniFiber*pFiber;

	//-B: assuming monofiber link betweeen nodes and assuming each fiber has the same number of wavelenghts
	UINT fiberCapacity = this->numberOfChannels * this->m_nChannelCapacity;

	//-B: compute average number of nodes under (in the network hierarchy) a candidate hotel node
	//-B: assuming a ring and spur topology with a certain number of trees and a certain number of trees for each ring
	UINT numOfNodesUnderHotel = floor((double)this->numberOfNodes / (double)this->m_nNumOfRings) - 1;
	double offeredTraffic_Tree = numOfNodesUnderHotel*offeredTraffic_MC;

	for (itr = this->hotelsList.begin(); itr != this->hotelsList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		UINT numOfIngoingFibers = 0;

		//-B: count all ingoing fibers coming from another hotel node
		for (itrLink = pNode->m_hILinkList.begin(); itrLink != pNode->m_hILinkList.end(); itrLink++)
		{
			pFiber = (UniFiber*)(*itrLink);
			//-B: if the ingloing fiber comes from another candidate hotel node
			if (((OXCNode*)pFiber->getSrc())->getBBUHotel() == true)
			{
				numOfIngoingFibers++;
			}
		}
		//-B: ingoing fibers are not all going towards the core CO on the shortest path --> on average only half of them
		//	(it is important to consider the shortest path since we have to cope with a strict latency constraint)
		numOfIngoingFibers = numOfIngoingFibers / 2;
		
		double maxIngoingTraffic = 0;
		double MC_per_wavel = 0;

		if (MIDHAUL)
		{
			//maxIngoingTraffic = (numOfIngoingFibers * fiberCapacity);
			UINT wavel_per_tree = ceil((double)offeredTraffic_Tree / (double)this->m_nChannelCapacity);
			UINT tree_per_fiber = floor((double)this->numberOfChannels / (double)wavel_per_tree);
			pNode->m_uNumOfBBUSupported = numOfIngoingFibers * tree_per_fiber*numOfNodesUnderHotel;
			//pNode->m_uNumOfBBUSupported = floor((double)maxIngoingTraffic / (double)offeredTraffic_Tree);
#ifdef DEBUGB
			cout << "\n\tnumOfNodesUnderHotel = " << numOfNodesUnderHotel << endl;
			cout << "\tofferedTraffic_MC = " << offeredTraffic_MC << endl;
			cout << "\tmaxIngoingTraffic = " << maxIngoingTraffic << endl;
			cout << "\tofferedTraffic_Tree = " << offeredTraffic_Tree << endl;
#endif // DEBUGB

			//-B: for each candidate hotel node, the number of BBU supported is the number of nodes whose average traffic can be held by the hotel node
			//pNode->m_uNumOfBBUSupported = floor(maxIngoingTraffic / (double)offeredTraffic_MC);
			//pNode->m_uNumOfBBUSupported = this->numberOfChannels * numOfIngoingFibers;
		}
		else //FRONTHAUL
		{
			MC_per_wavel = floor(this->m_nChannelCapacity / offeredTraffic_MC);
			if (MC_per_wavel == 0)
			{
				cout << "\tChannel capacity too small for FRONTHAUL!!!" << endl;
				cin.get();
				DEFAULT_SWITCH;
			}
			else
			{
				pNode->m_uNumOfBBUSupported = MC_per_wavel * numOfIngoingFibers * this->numberOfChannels;
			}
		}


		//-B: add this number to the previously computed one
		//pNode->m_uNumOfBBUSupported += numOfNodesUnderHotel;

#ifdef DEBUGB
		cout << "\tIl nodo #" << pNode->getId() << " ha " << numOfIngoingFibers << " fibre entranti e provenienti da altri hotel nodes" << endl;
		if (MIDHAUL)
			cout << "\t\t--> supporta un traffico massimo proveniente dagli altri hotels di " << maxIngoingTraffic << " OC1" << endl;
		else
			cout << "\t\t--> supporta il fronthaul di " << MC_per_wavel << " MC per ogni wavelength" << endl;
		cout << "\t\t--> supporta un numero massimo di BBU pari a " << pNode->m_uNumOfBBUSupported << endl;
		//cin.get();
#endif // DEBUGB

	}
}


void WDMNetwork::computeNumOfBBUSupported_CONSERVATIVE(UINT offeredTraffic_MC)
{
	vector<OXCNode*>::const_iterator itr;
	OXCNode*pNode;
	list<AbstractLink*>::const_iterator itrLink;
	UniFiber*pFiber;

	cout << "Fiber capacity used by G: " << this->numberOfChannels * this->m_nChannelCapacity << endl;

	//-B: assuming monofiber link betweeen nodes and assuming each fiber has the same number of wavelenghts
	double fiberCapacity = LINKOCCUPANCY_PERC * this->numberOfChannels * this->m_nChannelCapacity;

	cout << "Fiber capacity updated: " << fiberCapacity << endl;

	cin.get();

	//-B: compute average number of nodes under (in the network hierarchy) a candidate hotel node
	//-B: assuming a ring and spur topology with a certain number of trees and a certain number of trees for each ring
	//UINT numOfNodesUnderHotel = floor((double)this->numberOfNodes / (double)this->m_nNumOfRings) - 1;
	//double offeredTraffic_Tree = numOfNodesUnderHotel*offeredTraffic_MC;

	for (itr = this->hotelsList.begin(); itr != this->hotelsList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		UINT numOfIngoingFibers = 0;

		//-B: count all ingoing fibers coming from another hotel node
		for (itrLink = pNode->m_hILinkList.begin(); itrLink != pNode->m_hILinkList.end(); itrLink++)
		{
			pFiber = (UniFiber*)(*itrLink);
			//-B: if the ingloing fiber comes from another candidate hotel node
			if (((OXCNode*)pFiber->getSrc())->getBBUHotel() == true)
			{
				numOfIngoingFibers++;
			}
		}
		//-B: ingoing fibers are not all going towards the core CO on the shortest path --> on average only half of them
		//	(it is important to consider the shortest path since we have to cope with a strict latency constraint)
		numOfIngoingFibers = numOfIngoingFibers / 2;

		double maxIngoingTraffic = 0;
		//double MC_per_wavel = 0;

		if (MIDHAUL)
		{
			double MCs_per_fiber = floor(fiberCapacity / (double)offeredTraffic_MC);
			cout << "MCs per fiber: " << MCs_per_fiber << endl;
			cin.get();

			pNode->m_uNumOfBBUSupported = numOfIngoingFibers * MCs_per_fiber;
			//-B: ONLY FOR 4G CASE where grooming capabilities is present only in active nodes
			if (ONLY_ACTIVE_GROOMING == true)
				pNode->m_uNumOfBBUSupported = pNode->m_uNumOfBBUSupported / 2;
			//pNode->m_uNumOfBBUSupported = floor((double)maxIngoingTraffic / (double)offeredTraffic_Tree);
#ifdef DEBUGB
			//cout << "\n\tnumOfNodesUnderHotel = " << numOfNodesUnderHotel << endl;
			//cout << "\tofferedTraffic_MC = " << offeredTraffic_MC << endl;
			//cout << "\tmaxIngoingTraffic = " << maxIngoingTraffic << endl;
			//cout << "\tofferedTraffic_Tree = " << offeredTraffic_Tree << endl;
#endif // DEBUGB

			//-B: for each candidate hotel node, the number of BBU supported is the number of nodes whose average traffic can be held by the hotel node
			//pNode->m_uNumOfBBUSupported = floor(maxIngoingTraffic / (double)offeredTraffic_MC);
			//pNode->m_uNumOfBBUSupported = this->numberOfChannels * numOfIngoingFibers;
		}
		else //FRONTHAUL
		{
			double MCs_per_wavel = floor((double)this->m_nChannelCapacity / (double)offeredTraffic_MC);
			double MCs_per_fiber = MCs_per_wavel * this->numberOfChannels;
			pNode->m_uNumOfBBUSupported = numOfIngoingFibers * MCs_per_fiber;
		}


		//-B: add this number to the previously computed one
		//pNode->m_uNumOfBBUSupported += numOfNodesUnderHotel;

#ifdef DEBUGB
		cout << "\tIl nodo #" << pNode->getId() << " ha " << numOfIngoingFibers << " fibre entranti e provenienti da altri hotel nodes" << endl;
		if (MIDHAUL)
			cout << "\t\t--> supporta un traffico massimo proveniente dagli altri hotels di " << maxIngoingTraffic << " OC1" << endl;
		else
			//cout << "\t\t--> supporta il fronthaul di " << MC_per_wavel << " MC per ogni wavelength" << endl;
		cout << "\t\t--> supporta un numero massimo di BBU pari a " << pNode->m_uNumOfBBUSupported << endl;
		//cin.get();
#endif // DEBUGB

	}
}


void WDMNetwork::sortHotelsListByMetric()
{
	BinaryHeap<OXCNode*, vector<OXCNode*>, POXCNodeMetricComp> sortedHotelsList;
	vector<OXCNode*>::const_iterator itr;
	OXCNode*pNode;

	for (int i = 0; i < this->hotelsList.size(); i++)
	{
		pNode = hotelsList[i];
		pNode->updateHotelCostMetricForP2(this->numberOfNodes, this->DummyNode);
		sortedHotelsList.insert(pNode);
	}

	hotelsList.clear();

	while (!sortedHotelsList.empty())
	{
		pNode = sortedHotelsList.peekMin();
		sortedHotelsList.popMin();
#ifdef DEBUGB
		cout << "\tNodo #" << pNode->getId() << " - stage = " << pNode->m_nNetStage
			<< " - distanza dal core CO in km = " << pNode->m_fDistanceFromCoreCO << endl;
#endif // DEBUGB
		hotelsList.push_back(pNode);
	}

}