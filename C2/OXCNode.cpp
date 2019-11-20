#pragma warning(disable: 4786)
#include <assert.h>
#include "OchInc.h"

#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "UniFiber.h"
#include "Vertex.h"
#include "AbstractPath.h"
#include "Lightpath.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "WDMNetwork.h"
#include "OXCNode.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

OXCNode::OXCNode(UINT nNodeId, WConversionCapability eConversion,
	GroomingCapability eGrooming, UINT nTx, UINT nRx, int Nchan, int nDCFlag, int nTimeZone) :
	AbstractNode(nNodeId, Nchan), m_eConversion(eConversion), m_eGrooming(eGrooming), m_dTrafficProcessed(0), m_dExtraLatency(0),
	m_nTx(nTx), m_nRx(nRx), m_nMaxW(0), m_nBBUNodeIdsAssigned(0), m_nBBUNodeIdsAssignedLast(0), m_nNetStage(UNREACHABLE), m_nBetweennessCentrality(0),
	m_nFreeTx(nTx), m_nFreeRx(nRx), m_nBBUReachCost(UNREACHABLE), m_nReachabilityDegree(0), m_nProximityDegree(0), m_uNumOfBBUSupported(0),
	m_pStateWDM(NULL), m_bNewLP(false), m_DCFlag(nDCFlag), m_TimeZone(nTimeZone), m_dCostMetric(UNREACHABLE), m_dTrafficGen(0)
{
	
	if (nTimeZone == 1) //BBU hotel node
	{
		bbuHotel = true;
		m_nBBUs = 0;
	}
	else if (nTimeZone == 0)
	{
		bbuHotel = false;
		m_nBBUs = 0; //-B: previously, it was -1, but change to 0 if we allow a node to host its own BBU as last extreme option
	}
}

OXCNode::OXCNode(UINT nNodeId, WConversionCapability eConversion,
	GroomingCapability eGrooming, UINT nTx, UINT nRx, int Nchan, int nDCFlag, int nTimeZone, int cellFlag, UrbanArea nArea) :
	AbstractNode(nNodeId, Nchan), m_eConversion(eConversion), m_eGrooming(eGrooming), m_nBetweennessCentrality(0),
	m_nTx(nTx), m_nRx(nRx), m_nMaxW(0), m_nBBUNodeIdsAssigned(0), m_nBBUNodeIdsAssignedLast(0), m_cellFlag(cellFlag), m_nNetStage(UNREACHABLE),
	m_nFreeTx(nTx), m_nFreeRx(nRx), m_nBBUReachCost(UNREACHABLE), m_nReachabilityDegree(0), m_nProximityDegree(0),
	m_pStateWDM(NULL), m_bNewLP(false), m_DCFlag(nDCFlag), m_TimeZone(nTimeZone), m_dCostMetric(UNREACHABLE),
	m_nArea(nArea), m_fDistanceFromCoreCO(0), m_dTrafficGen(0), m_dTrafficProcessed(0), m_dActivityTime(0),
	m_dExtraLatency(0), m_uNumOfBBUSupported(0), m_bBBUAlreadyChanged(false), m_nPreviousBBU(0), m_nCountBBUsChanged(0)
{
	if (cellFlag == 0)
		m_dMaxTraffic = MAXTRAFFIC_MACROCELL;
	else if (cellFlag == 1)
		m_dMaxTraffic = MAXTRAFFIC_SMALLCELL;

	if (nTimeZone == 1) //BBU hotel node
	{
		bbuHotel = true;
		m_nBBUs = 0;

	}
	else if (nTimeZone == 0)
	{
		bbuHotel = false;
		m_nBBUs = 0; //-B: previously, it was -1, but change to 0 if we allow a node to host its own BBU as last extreme option
	}
}

OXCNode::~OXCNode()
{
	if (m_bNewLP && m_pStateWDM) {
		delete m_pStateWDM;
	}
	m_pStateWDM = NULL;
}

OXCNode::OXCNode(const OXCNode& rhs)
{
	*this = rhs;
}

const OXCNode& OXCNode::operator=(const OXCNode& rhs)
{
	if (this == &rhs)
		return (*this);
	AbstractNode::operator=(rhs);
	// immutable attributes
	m_eConversion = rhs.m_eConversion;
	m_eGrooming = rhs.m_eGrooming;
	m_nTx = rhs.m_nTx;
	m_nRx = rhs.m_nRx;
	m_nMaxW = rhs.m_nMaxW;
	m_nFreeTx = rhs.m_nFreeTx;
	m_nFreeRx = rhs.m_nFreeRx;
	m_TimeZone = rhs.m_TimeZone;

	// mutable attributes
	m_pWDMNetwork = NULL;
	m_pStateWDM = NULL;
	m_bNewLP = false;

	return (*this);
}

void OXCNode::dump(ostream &out) const
{
#ifdef DEBUGB
	cout << "OXCNODE DUMP" << endl;
	cin.get();
#endif // DEBUGB

	char *pConv;
	if (FullWavelengthConversion == m_eConversion) {
		pConv = "FWC";
	} else if (PartialWavelengthConversion == m_eConversion) {
		pConv = "PWC";
	} else {
		pConv = "NWC";
	}
//	out<<m_nNodeId<<" "<<m_eConversion<<" "<<m_eGrooming<<" ";
	out<<m_nNodeId<<" "<<pConv<<" ";
//	out<<"Tx="<<m_nFreeTx<<'@'<<m_nTx<<"; ";
//	out<<"Rx="<<m_nFreeRx<<'@'<<m_nRx<<":";
//	out<<endl;

	if (UNREACHABLE != m_hCost)
		out<<" C="<<m_hCost<<" ";
	else
		out<<" C=INF ";
	if (UNREACHABLE != m_nHops)
		out<<" H="<<m_nHops<<" ";
	else
		out<<" H=INF ";
	if (m_pPrevLink) {
		out<<m_pPrevLink->m_pSrc->getId();
		out<<"(LinkID="<<m_pPrevLink->getId()<<')'<<"(prev)";
	} else {
		out<<"NULL";
	}
	out<<endl;

	list<AbstractLink*>::const_iterator iter;
	for (iter=m_hOLinkList.begin(); iter!=m_hOLinkList.end(); iter++) {
		out<<'\t';
		(*iter)->dump(out);
		out<<endl;
	}
//	for (iter=m_hILinkList.begin(); iter!=m_hILinkList.end(); iter++) {
//		out<<'\t';
//		(*iter)->dump(out);
//		out<<endl;
//	}
}

void OXCNode::addIncomingLink(AbstractLink *pILink)
{
	AbstractNode::addIncomingLink(pILink);
	UniFiber *pUniFiber = (UniFiber*)pILink;
	assert(pUniFiber);
	m_nMaxW = (pUniFiber->m_nW <= m_nMaxW)? m_nMaxW: pUniFiber->m_nW;
}

void OXCNode::addOutgoingLink(AbstractLink *pOLink)
{
	AbstractNode::addOutgoingLink(pOLink);
	UniFiber *pUniFiber = (UniFiber*)pOLink;
	assert(pUniFiber);
	m_nMaxW = (pUniFiber->m_nW <= m_nMaxW)? m_nMaxW: pUniFiber->m_nW;
}

void OXCNode::setWDMNetwork(WDMNetwork *pWDMNetwork)
{
	assert(pWDMNetwork);

	m_pWDMNetwork = pWDMNetwork;
}

void OXCNode::genStateGraphHelper(Graph& hGraph, 
								  int& nVertexId, int& nLinkId) const
{
	// Access layer
	Vertex *pAccessIn =	new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Access_In);
	Vertex *pAccessOut = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Access_Out);
	hGraph.addNode(pAccessIn);
	hGraph.addNode(pAccessOut);
	
	switch (m_eGrooming) {
		case OXCNode::FullGrooming:
			{
				SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
					pAccessIn, pAccessOut,
					0, // 0 cost
					0, NULL, SimplexLink::LT_Grooming, -1, INFINITE_CAP);
				assert(pNewLink);
			}
			break;
		case OXCNode::PartialGrooming:
			assert(false);	// todo
			break;
		case OXCNode::NoGrooming:
			//assert(false);	// todo
			break;
		default:
			DEFAULT_SWITCH;
	}

	// Lightpath layer
	 //-B: COMMENTED SINCE NOT USED
	Vertex *pLightpathIn = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Lightpath_In);
	Vertex *pLightpathOut =	new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Lightpath_Out);
	hGraph.addNode(pLightpathIn);
	hGraph.addNode(pLightpathOut);
	SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
		pLightpathIn, pAccessIn,
		0, // 0 cost
		0, NULL, SimplexLink::LT_Demux, -1, INFINITE_CAP);
	assert(pNewLink);
	pNewLink = hGraph.addSimplexLink(nLinkId++,
		pAccessOut, pLightpathOut,
		0, //0 cost
		0, NULL, SimplexLink::LT_Mux, -1, INFINITE_CAP);
	assert(pNewLink);
	

	// Wavelength layers
	int  w;
	for (w = 0; w < m_nMaxW; w++)
	{ //-B: m_nMaxW is the num of channels per fiber
		Vertex *pChannelIn = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Channel_In, w);
		Vertex *pChannelOut = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Channel_Out, w);
		hGraph.addNode(pChannelIn);
		hGraph.addNode(pChannelOut);
		SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
			pChannelIn, pChannelOut,
			0,  //0 cost
			0, NULL, SimplexLink::LT_Channel_Bypass, -1, INFINITE_CAP);
		assert(pNewLink);
		if (m_nRx > 0) {
			SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
				pChannelIn, pAccessIn,
				0, //0 cost
				0, NULL, SimplexLink::LT_Rx, -1, INFINITE_CAP);
			assert(pNewLink);
		}
		if (m_nTx > 0) {
			SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
				pAccessOut, pChannelOut,
				0, //0 cost
				0, NULL, SimplexLink::LT_Tx, -1, INFINITE_CAP);
			assert(pNewLink);
		}
	}

	// Conversion capability
	switch (m_eConversion) {
	case FullWavelengthConversion:
		{
			int  w;
			for (w = 0; w < m_nMaxW; w++) {
				int  w2;
				for (w2 = 0; w2 < m_nMaxW; w2++)
					if (w != w2) {
						SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
							m_nNodeId, Vertex::VT_Channel_In, w,
							m_nNodeId, Vertex::VT_Channel_Out, w2,
							0, //0 cost
							0, NULL, SimplexLink::LT_Converter, -1,
							INFINITE_CAP);
						assert(pNewLink);
					}
			}
		}
		break;
	case PartialWavelengthConversion:
		assert(false);	// todo
		break;
	case WavelengthContinuous:
		NULL;			// done
		break;
	default:
		DEFAULT_SWITCH
	}
}

void OXCNode::genStateGraphFullConversionEverywhereHelper(Graph& hGraph, 
									int& nVertexId, int& nLinkId) const
{
	// Access layer
	Vertex *pAccessIn = 
		new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Access_In);
	Vertex *pAccessOut = 
		new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Access_Out);
	hGraph.addNode(pAccessIn);
	hGraph.addNode(pAccessOut);
	switch (m_eGrooming) {
	case OXCNode::FullGrooming:
		{
			SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
				pAccessIn, pAccessOut,
				0, // 0 cost
				0, NULL, SimplexLink::LT_Grooming, -1, INFINITE_CAP);
			assert(pNewLink);
		}
		break;
	case OXCNode::PartialGrooming:
		assert(false);	// todo
		break;
	case OXCNode::NoGrooming:
		assert(false);	// todo
		break;
	default:
		DEFAULT_SWITCH
	}

	// Lightpath layer
	Vertex *pLightpathIn =
		new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Lightpath_In);
	Vertex *pLightpathOut =
		new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Lightpath_Out);
	hGraph.addNode(pLightpathIn);
	hGraph.addNode(pLightpathOut);
	SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
		pLightpathIn, pAccessIn,
		0, // 0 cost
		0, NULL, SimplexLink::LT_Demux, -1, INFINITE_CAP);
	assert(pNewLink);
	pNewLink = hGraph.addSimplexLink(nLinkId++,
		pAccessOut, pLightpathOut,
		0, // 0 cost
		0, NULL, SimplexLink::LT_Mux, -1, INFINITE_CAP);
	assert(pNewLink);

	// Convertion capability
	assert(FullWavelengthConversion == m_eConversion);
	// Wavelength layer. 
	// For efficiency, one layer for full wavelength conversion
	int w = -1;
	{
		Vertex *pChannelIn =
			new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Channel_In, w);
		Vertex *pChannelOut = 
			new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Channel_Out, w);
		hGraph.addNode(pChannelIn);
		hGraph.addNode(pChannelOut);
		SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
			pChannelIn, pChannelOut,
			0, // 0 cost
			0, NULL, SimplexLink::LT_Channel_Bypass, -1, INFINITE_CAP);
		assert(pNewLink);

		if (m_nRx > 0) {
			SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
				pChannelIn, pAccessIn,
				0, // 0 cost
				0, NULL, SimplexLink::LT_Rx, -1, m_nRx * OCLightpath);
			assert(pNewLink);
		}
		if (m_nTx > 0) {
			SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
				pAccessOut, pChannelOut,
				0, // 0 cost
				0, NULL, SimplexLink::LT_Tx, -1, m_nTx * OCLightpath);
			assert(pNewLink);
		}
	}
}

bool NS_OCH::OXCNode::getBBUHotel()
{
	return this->bbuHotel;
}

UINT OXCNode::getNumberOfTx() const
{
	return m_nTx;
}

UINT OXCNode::getNumberOfRx() const
{
	return m_nRx;
}

UINT OXCNode::getTimeZone() const
{
	return m_TimeZone;
}

OXCNode::WConversionCapability NS_OCH::OXCNode::getConversion()
{
	return this->m_eConversion;
}

OXCNode::GroomingCapability NS_OCH::OXCNode::getGrooming()
{
	return this->m_eGrooming;
}


UINT OXCNode::getDCFlag() const
{
	return m_DCFlag;
}


UINT OXCNode::countOutChannels() const
{
	UINT nCount = 0;
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hOLinkList.begin(); itr != m_hOLinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		assert(pUniFiber);
		nCount += pUniFiber->m_nW;
	}
	return nCount;
}

UINT OXCNode::countInChannels() const
{
	UINT nCount = 0;
	UniFiber *pUniFiber;
	list<AbstractLink*>::const_iterator itr;
	for (itr = m_hILinkList.begin(); itr != m_hILinkList.end(); itr++) {
		pUniFiber = (UniFiber*)(*itr);
		assert(pUniFiber);
		nCount += pUniFiber->m_nW;
	}
	return nCount;
}

void OXCNode::scaleTxRxWRTNodalDeg(double dTxScale)
{
	//-B: countOutChannels returns the num of outgoing channels from node that calls the method (this)
	m_nTx = m_nFreeTx = (int)(this->countOutChannels() * dTxScale);
	//-B: countInChannels returns the num of ingoing channels into the node that calls the method (this)
	m_nRx = m_nFreeRx = (int)(this->countInChannels() * dTxScale);
}


void OXCNode::genStateAuxGraphHelper(Graph& hGraph,
	int& nVertexId, int& nLinkId, UINT& w)
{
	// Access layer
	Vertex *pAccessIn = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Access_In);
	Vertex *pAccessOut = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Access_Out);
	hGraph.addNode(pAccessIn);
	hGraph.addNode(pAccessOut);

	switch (m_eGrooming)
	{
		case OXCNode::FullGrooming:
		{
			SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++, pAccessIn, pAccessOut,	0, // 0 cost
									0, NULL, SimplexLink::LT_Grooming, -1, INFINITE_CAP);
			assert(pNewLink);
		}
		break;
		case OXCNode::PartialGrooming:
			assert(false);	// todo
			break;
		case OXCNode::NoGrooming:
			assert(false);	// todo
			break;
		default:
			DEFAULT_SWITCH;
	}

	// Lightpath layer
	Vertex *pLightpathIn = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Lightpath_In);
	Vertex *pLightpathOut = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Lightpath_Out);
	hGraph.addNode(pLightpathIn);
	hGraph.addNode(pLightpathOut);
	SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++, pLightpathIn, pAccessIn, 0, // 0 cost
							0, NULL, SimplexLink::LT_Demux, -1, INFINITE_CAP);
	assert(pNewLink);
	pNewLink = hGraph.addSimplexLink(nLinkId++, pAccessOut, pLightpathOut,
				0, 0, NULL, SimplexLink::LT_Mux, -1, INFINITE_CAP);
	assert(pNewLink);

	// Wavelength layers
		Vertex *pChannelIn = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Channel_In, w);
		Vertex *pChannelOut = new Vertex((OXCNode*)this, nVertexId++, Vertex::VT_Channel_Out, w);
		hGraph.addNode(pChannelIn);
		hGraph.addNode(pChannelOut);
		pNewLink = hGraph.addSimplexLink(nLinkId++, pChannelIn, pChannelOut, 0,  //0 cost
								0, NULL, SimplexLink::LT_Channel_Bypass, -1, INFINITE_CAP);
		assert(pNewLink);
		if (m_nRx > 0) {
			SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++, pChannelIn, pAccessIn,	0, //0 cost
									0, NULL, SimplexLink::LT_Rx, -1, INFINITE_CAP);
			assert(pNewLink);
		}
		if (m_nTx > 0) {
			SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++, pAccessOut, pChannelOut, 0, //0 cost
									0, NULL, SimplexLink::LT_Tx, -1, INFINITE_CAP);
			assert(pNewLink);
		}

	// Conversion capability
	switch (m_eConversion) {
	case FullWavelengthConversion:
	{
		int  w;
		for (w = 0; w < m_nMaxW; w++) {
			int  w2;
			for (w2 = 0; w2 < m_nMaxW; w2++)
				if (w != w2) {
					SimplexLink *pNewLink = hGraph.addSimplexLink(nLinkId++,
						m_nNodeId, Vertex::VT_Channel_In, w,
						m_nNodeId, Vertex::VT_Channel_Out, w2,
						0, //0 cost
						0, NULL, SimplexLink::LT_Converter, -1,
						INFINITE_CAP);
					assert(pNewLink);
				}
		}
	}
	break;
	case PartialWavelengthConversion:
		assert(false);	// todo
		break;
	case WavelengthContinuous:
		NULL;			// done
		break;
	default:
		DEFAULT_SWITCH
	}
}

bool OXCNode::isMacroCell()
{
	if (this->m_cellFlag == 0)
		return true;
	return false;
}

bool OXCNode::isSmallCell()
{
	if (this->m_cellFlag == 1)
		return true;
	return false;
}

void OXCNode::resetCost()
{
	this->m_nBBUReachCost = UNREACHABLE;
}

//-B: higher is better -> a sort of merit cost metric
void OXCNode::updateHotelCostMetricForP2(UINT numOfNodes, UINT coreCOId)
{
	assert(this->getBBUHotel() == true);

	//-B: initialize var (constructor and resetPreProcessing() methods set it to UNREACHABLE)
	this->m_dCostMetric = 100000 * this->m_nNetStage;				//reachability = 10000
	//add to var
	this->m_dCostMetric += (numOfNodes * 10000) - (10000 * this->m_nBetweennessCentrality);				//stage = 9999
																	//netStage should be > 100 to get a negative factor (reasonably impossible)
	
	//-B: if it is not the core CO (because core CO has 0 activation cost for our model)
	//if (this->getId() != coreCOId)
	//{
		//if it is active
		//if (this->m_nBBUs == 0)
			//this->m_dCostMetric += 5000;										//activation = 5000
	//}
																		
	//-B: KIND OF COST ASSIGNMENT (!) MUST NOT (!) ALLOW TO OVERCOME THE PREFERENCE GIVEN BY "HIGHER" FACTORS -> it's a sort of preference
	this->m_dCostMetric += (numOfNodes*10) - (10 * this->m_nReachabilityDegree);				//	--> betweenness centrality should be > 500 to overcome the preference 
																			//given by the previous factor (-> reasonably impossible)

	//LAST -> to be added in calling method: path cost towards hotel
	//path cost, unlikely, will be >= 10 -> so, reasonably it should not overcome the betweenness centrality factor
}

//-B: higher is better -> a sort of merit cost metric
void OXCNode::updateHotelCostMetricForP0(UINT numOfNodes)
{
	assert(this->getBBUHotel() == true);

	//-B: initialize var (constructor and resetPreProcessing() methods set it to UNREACHABLE)
	this->m_dCostMetric = 100000 * this->m_nNetStage; 						//stage = 10000
																			//add to var
	//this->m_dCostMetric += (numOfNodes*1000) - (1000 * this->m_nReachabilityDegree);				//reachability = 9999
	
	//-B: NODE ACTIVATION IS ALREADY TAKEN INTO ACCOUNT IN THE ALGORITHM:
	//	IT WILL NOT CHECK INACTIVE HOTELS IF IT CAN FIND A FEASIBLE ACTIVE HOTEL
	//if it is active
	//if (this->m_nBBUs > 0)
		//this->m_dCostMetric += 5000;										//activation = 5000
																			//with this weighted cost assingment, to overcome the preference
																			//given by previous factor, that previous factor sholud be > 50 (reasonably impossible)
																			//else if #BBUs already active in this hotel is 0, the cost metric will remain the same

	//-B: KIND OF COST ASSIGNMENT (!) MUST NOT (!) ALLOW TO OVERCOME THE PREFERENCE GIVEN BY "HIGHER" FACTORS -> it's a sort of preference
	//this->m_dCostMetric += 10 * this->m_nBetweennessCentrality; --> NOT CONSIDERED IN POLICY #0 (but in policy #2)				

	//LAST -> to be added in calling method: path cost towards hotel
	//path cost, unlikely, will be >= 10 -> so, reasonably it should not overcome the betweenness centrality factor
}


OXCNode::UrbanArea OXCNode::getArea()
{
	return this->m_nArea;
}


double OXCNode::getTrafficProcessed()
{
	if (!(this->getBBUHotel()))
		return 0;

	return this->m_dTrafficProcessed;
}

UINT OXCNode::getNumberOfSources()
{
#ifdef DEBUGX
	cout << "-> getNumberOfSources" << endl;
#endif // DEBUGB

	UINT numOfSources = floor(this->m_dMaxTraffic / BWDGRANULARITY);
	//-B: implement small cells as sub-parts of a macro cell
	if (this->isMacroCell())
	{
		//-B: take into account that a small cell is like a 1-sector of a macro cell 3-sector
		numOfSources += SMALLCELLS_PER_MC * floor(MAXTRAFFIC_SMALLCELL / BWDGRANULARITY);
	}

#ifdef DEBUGX
	cout << "\tNode #" << this->getId() << " - Num of sources: " << numOfSources << endl;
#endif // DEBUGB

	return numOfSources;
}

UINT OXCNode::getNumberOfBusySources()
{
	UINT busySources = this->m_dTrafficGen / BWDGRANULARITY;

#ifdef DEBUGB
	//cout << "-> getNumberOfBusySources" << endl;
	//cout << "\tNode #" << this->getId() << " - Num of busy sources: " << busySources << endl;
	//cin.get();
#endif // DEBUGB

	return busySources;
}

void OXCNode::resetActivityTime()
{
	this->m_dActivityTime = 0;
}