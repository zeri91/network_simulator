#pragma warning(disable: 4786)
#include <stdio.h>
#include <assert.h>

#include "OchInc.h"

#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbstractPath.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "Lightpath.h"
#include "Circuit.h"
#include "Log.h"
#include "Connection.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

UINT Connection::g_pBWMap[NumberOfBWGranularity] = {OC1, OC3, OC12, OC48, OC192};

Connection::Connection(): m_eStatus(REQUEST), 
	m_pPCircuit(NULL), m_pBCircuit(NULL), m_pCircuits(NULL), m_bBlockedDueToUnreach(false),
	m_dHoldingTime(1.0), m_bBlockedDueToTx(false), m_nHopCount(0), m_dRoutingTime(0.0000),
	grooming(0), midhaul_id(NULL)
{
}

Connection::Connection(UINT nSeqNo, UINT nSrc, UINT nDst,
	SimulationTime tArrivalTime, SimulationTime tHoldingTime,
	BandwidthGranularity eBW, ProtectionClass ePC) :
	m_nSequenceNo(nSeqNo), m_nSrc(nSrc), m_nDst(nDst), grooming(0),
	m_dArrivalTime(tArrivalTime), m_dHoldingTime(tHoldingTime),
	m_eBandwidth(eBW), m_eCPRIBandwidth(OC0), m_eProtectionClass(ePC), m_eStatus(REQUEST),
	m_pPCircuit(NULL), m_pBCircuit(NULL), m_bBlockedDueToUnreach(false), m_pCircuits(NULL),
	m_bBlockedDueToTx(false), m_bBlockedDueToLatency(false), m_nHopCount(0), m_nBackhaulSaved(0),
	m_bTrafficToBeUpdatedForDep(true), midhaul_id(NULL)
{
	m_dRoutingTime = 0.0000; //-B: max latency CPRI
}

Connection::Connection(UINT nSeqNo, UINT nSrc, UINT nDst,
	SimulationTime tArrivalTime, SimulationTime tHoldingTime,
	BandwidthGranularity eBW, BandwidthGranularity CPRIbw, ProtectionClass ePC, ConnectionType connType) :
	m_nSequenceNo(nSeqNo), m_nSrc(nSrc), m_nDst(nDst), grooming(0),
	m_dArrivalTime(tArrivalTime), m_dHoldingTime(tHoldingTime),
	m_eBandwidth(eBW), m_eCPRIBandwidth(CPRIbw), m_eProtectionClass(ePC), m_eStatus(REQUEST),
	m_pPCircuit(NULL), m_pBCircuit(NULL), m_bBlockedDueToUnreach(false), m_pCircuits(NULL),
	m_bBlockedDueToTx(false), m_bBlockedDueToLatency(false), m_nHopCount(0), m_eConnType(connType), m_nBackhaulSaved(0), m_nMidhaulSaved(0),
	m_bTrafficToBeUpdatedForDep(true), midhaul_id(NULL)
{
	m_dRoutingTime = 0.0000; //-B: max latency CPRI
}

// Connection::Connection(const Connection& rhs)
// {
// 	*this = rhs;
// }
// Connection& Connection::operator=(const Connection& rhs)
// {
// 	if (this == &rhs)
// 		return (*this);
//         // devo scivere gli asssegnamenti espliciti solo per i membri che sono puntati???
//         //copy circuit



// // 	// copy nodes
// // 	list<AbstractNode*>::const_iterator itrNode;
// // 	for (itrNode=rhs.m_hNodeList.begin(); itrNode!=rhs.m_hNodeList.end(); 
// // 	itrNode++) {
// // 		OXCNode *pOXC = (OXCNode*)(*itrNode);
// // 		addNode(new OXCNode(*pOXC));
// // 	}

// // 	// copy links
// // 	list<AbstractLink*>::const_iterator itrLink;
// // 	for (itrLink=rhs.m_hLinkList.begin(); itrLink!=rhs.m_hLinkList.end();
// // 	itrLink++) {
// // 		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
// // 		UniFiber *pNewUniFiber = addUniFiber(pUniFiber->getId(), 
// // 										pUniFiber->getSrc()->getId(),
// // 										pUniFiber->getDst()->getId(),
// // 										pUniFiber->m_nW,
// // 										pUniFiber->m_hCost,
// // 										pUniFiber->m_nLength);
// // 		int w;
// // 		for (w=0; w<pUniFiber->m_nW; w++) {
// // 			pNewUniFiber->m_pChannel[w] = pUniFiber->m_pChannel[w];
// // 			pNewUniFiber->m_pChannel[w].m_pUniFiber = pNewUniFiber;
// // 		}
// // 		if (pUniFiber->m_pCSet) {
// // 			pNewUniFiber->m_pCSet = new UINT[pUniFiber->m_nCSetSize];
// // 			pNewUniFiber->m_nCSetSize = pUniFiber->m_nCSetSize;
// // 			memcpy(pNewUniFiber->m_pCSet, pUniFiber->m_pCSet, 
// // 				pUniFiber->m_nCSetSize*sizeof(UINT));
// // 		}
// // 		pNewUniFiber->m_nBChannels = pUniFiber->m_nBChannels;
// // #ifdef _OCHDEBUG9
// // 		{
// // 			int e;
// // 			for (e=0; e<pUniFiber->m_nCSetSize; e++) {
// // 				assert(pUniFiber->m_pCSet[e] <= pUniFiber->m_nBChannels);
// // 			}
// // 		}
// // #endif
// // 	}
// 	return (*this);
// }

Connection::~Connection()
{
	if (m_pPCircuit)
		delete m_pPCircuit;
	if (m_pBCircuit)
		delete m_pBCircuit;
}

//stampa le info riguardanti la connessione nella stringa che restituisce(pStr)
const char* Connection::toString() const
{
	static char pStr[80];

	char *pClass;
	switch (m_eProtectionClass) {
	case PC_NoProtection:
		pClass = "NP";
		break;
	case PC_DedicatedPath:
		pClass = "DPP";
		break;
	case PC_SharedPath:
		pClass = "SPP";
		break;
	case PC_SharedSRG:
		pClass = "SSRG";
		break;
	case PC_DedicatedLink:
		pClass = "DLP";
		break;
	case PC_SharedLink:
		pClass = "SLP";
		break;
	case PC_SharedSegment:
		pClass = "SEG";
		break;
	default:
		DEFAULT_SWITCH;
	}
	if (this->m_nDst != NULL)
	{
		sprintf(pStr, ">Conn %5d | %3d->%3d | BW = %3d | CPRIBW = %3d | ArrTime = %4.4f | HoldTime = %1.4f | RoutingTime = %1.9f | ",
			m_nSequenceNo, m_nSrc, m_nDst, m_eBandwidth, m_eCPRIBandwidth, m_dArrivalTime, m_dHoldingTime, m_dRoutingTime);
	}
	else
	{
		sprintf(pStr, ">Conn %5d | %3d->NULL | BW = %3d | CPRIBW = %3d | ArrTime = %4.4f | HoldTime = %1.4f | RoutingTime = %1.9f | ",
			m_nSequenceNo, m_nSrc, m_eBandwidth, m_eCPRIBandwidth, m_dArrivalTime, m_dHoldingTime, m_dRoutingTime);
	}

	return pStr;
}

//stampa le info riguardanti la connessione tramite il metodo toString() e aggiunge anche lo status della connessione considerata
void Connection::dump(ostream& out) const
{
	char *pStatus, *pType;
	switch (m_eStatus) {
		case REQUEST:
			pStatus = "REQUEST";
			break;
		case SETUP:
			pStatus = "SETUP";
			break;
		case DROPPED:
			pStatus = "DROPPED";
			break;
		case TORNDOWN:
			pStatus = "TORNDOWN";
			break;
		default:
			DEFAULT_SWITCH;
	}
	switch (this->m_eConnType) {
		case MOBILE_FRONTHAUL:
			pType = "MOBILE_FRONTHAUL";
			break;
		case FIXEDMOBILE_FRONTHAUL:
			pType = "FIXEDMOBILE_FRONTHAUL";
			break;
		case FIXED_BACKHAUL:
			pType = "FIXED_BACKHAUL";
			break;
		case MOBILE_BACKHAUL:
			pType = "MOBILE_BACKHAUL";
			break;
		case FIXEDMOBILE_BACKHAUL:
			pType = "FIXEDMOBILE_BACKHAUL";
			break;
		// -L
		case FIXED_MIDHAUL:
			pType = "FIXED_MIDHAUL";
			break;
		default:
			NULL;
	}
	//-B: toString() is the real CONNECTION DUMP
	out <<  toString() << "Status = " << pStatus << " | " << pType;
	
	out << " | Lightpath:";
	list<Lightpath*>::const_iterator itr;
	if (this->m_pPCircuit)
	{
		for (itr = this->m_pPCircuit->m_hRoute.begin(); itr != this->m_pPCircuit->m_hRoute.end(); itr++)
		{
			if ((*itr)->getId())
				out << " " << (*itr)->getId();
			else
				out << "/";
		}
	}

	out << endl;

	/*
	cout << "BWMap = ";
	for (int i = 0; g_pBWMap[i] != NULL; i++)
		if (g_pBWMap[i + 1] != NULL)
			cout << g_pBWMap[i] << ", ";
		else
			cout << g_pBWMap[i] << ">";
	*/
	/*
	if (SETUP == m_eStatus) {
		if (m_pPCircuit)
			m_pPCircuit->dump(out);
		if (m_pBCircuit)
			m_pBCircuit->dump(out);
	}*/
}

// -L: da modificare considerando anche il midhaul
void Connection::log(Log &hLog)
{
	if (SETUP == m_eStatus)
	{
		//hLog.m_nProvisionedCon++; //-B: commented! move this instruction in several cases to count a provisioned fronthaul+backhaul connection
									//	as a single one (to provide successfully a mobile connection we have to provide successfully 
									//	and then backhaul, not only the first -> count only when backhaul provisioning has been done
		hLog.groomingPossibilities += this->grooming; //-B: add to the total value (if there was no grooming, it simply will add 0)
		switch (this->m_eConnType)
		{
		case MOBILE_FRONTHAUL:
		{
			//hLog.m_nProvisionedMobileFrontConn++;
			//hLog.m_nProvisionedMobileFrontBW += this->m_eCPRIBandwidth;
			//hLog.m_nProvisionedBW += this->m_eCPRIBandwidth;
			break;
		}

		case FIXEDMOBILE_FRONTHAUL:
		{
			//hLog.m_nProvisionedFixMobFrontConn++;
			//hLog.m_nProvisionedFixMobFrontBW += this->m_eCPRIBandwidth;
			//hLog.m_nProvisionedBW += this->m_eCPRIBandwidth;
			break;
		}

		case FIXED_BACKHAUL:
		{
			hLog.m_nProvisionedFixedBackConn++;
			hLog.m_nProvisionedFixedBackBW += this->m_eBandwidth;
			hLog.m_nProvisionedBW += this->m_eBandwidth;
			hLog.m_nProvisionedCon++;
			break;
		}

		case MOBILE_BACKHAUL:
		{
			//-B: update fronthaul stats
			hLog.m_nProvisionedMobileFrontConn++;
			hLog.m_nProvisionedMobileFrontBW += this->m_eCPRIBandwidth;
			hLog.m_nProvisionedBW += this->m_eCPRIBandwidth;
			//-B: update backhaul stats
			hLog.m_nProvisionedMobileBackConn++;
			hLog.m_nProvisionedMobileBackBW += this->m_eBandwidth;
			hLog.m_nProvisionedBW += this->m_eBandwidth;
			hLog.m_nProvisionedCon++;
			break;
		}

		case FIXEDMOBILE_BACKHAUL:
		{
			hLog.m_nProvisionedFixMobFrontConn++;
			hLog.m_nProvisionedFixMobFrontBW += this->m_eCPRIBandwidth;
			hLog.m_nProvisionedBW += this->m_eCPRIBandwidth;
			hLog.m_nProvisionedFixMobBackConn++;
			hLog.m_nProvisionedFixMobBackBW += this->m_eBandwidth;
			hLog.m_nProvisionedBW += this->m_eBandwidth;
			hLog.m_nProvisionedCon++;
			break;
		}

		// -L
		case FIXED_MIDHAUL:
		{
			break;
		}

			default:
				assert(false);
				break; //nothing to do
		} //end switch
	}
	else if (m_eStatus == DROPPED)
	{
		hLog.m_nBlockedCon++;
		if (this->m_bBlockedDueToUnreach)
		{
			hLog.m_nBlockedConDueToUnreach++;
		}
		else if (this->m_bBlockedDueToLatency)
		{
			hLog.m_nBlockedConDueToLatency++;
		}

		switch (this->m_eConnType)
		{
			case MOBILE_FRONTHAUL:
			{
				hLog.m_nBlockedMobileFrontConn++;
				hLog.m_nBlockedBW += this->m_eCPRIBandwidth;
				hLog.m_nBlockedBW += this->m_eBandwidth;
				hLog.m_nBlockedMobileFrontBW += this->m_eCPRIBandwidth;
				hLog.m_nBlockedMobileBackBW += this->m_eBandwidth;
				hLog.m_nBlockedFHForFHBlock += this->m_eCPRIBandwidth;
				break;
			}

			case FIXEDMOBILE_FRONTHAUL:
			{
				hLog.m_nBlockedFixMobFrontConn++;
				hLog.m_nBlockedBW += this->m_eCPRIBandwidth;
				hLog.m_nBlockedBW += this->m_eBandwidth;
				hLog.m_nBlockedFixMobFrontBW += this->m_eCPRIBandwidth;
				hLog.m_nBlockedFixMobBackBW += this->m_eBandwidth;
				hLog.m_nBlockedFHForFHBlock += this->m_eCPRIBandwidth;
				break;
			}

			case FIXED_BACKHAUL:
			{
				hLog.m_nBlockedFixedBackConn++;
				hLog.m_nBlockedBW += this->m_eBandwidth;
				hLog.m_nBlockedFixedBackBW += this->m_eBandwidth;
				hLog.m_nBlockedBHForBHBlock += this->m_eBandwidth;
				break;
			}

			case MOBILE_BACKHAUL:
			{
				hLog.m_nBlockedMobileBackConn++;
				hLog.m_nBlockedBW += this->m_eBandwidth;
				hLog.m_nBlockedMobileBackBW += this->m_eBandwidth;
				hLog.m_nBlockedBHForBHBlock += this->m_eBandwidth;
				break;
			}

			case FIXEDMOBILE_BACKHAUL:
			{
				hLog.m_nBlockedFixMobBackConn++;
				hLog.m_nBlockedBW += this->m_eBandwidth;
				hLog.m_nBlockedFixMobBackBW += this->m_eBandwidth;
				hLog.m_nBlockedBHForBHBlock += this->m_eBandwidth;
				break;
			}

			//-L
			case FIXED_MIDHAUL:
			{
				hLog.m_nBlockedFixMidConn++;
				hLog.m_nBlockedBW += this->m_eBandwidth;
				hLog.m_nBlockedFixMidBW += this->m_eBandwidth;
				break;
			}

			default:
				assert(false);
				break; //nothing to do
		}//end SWITCH
	}//end ELSE IF DROPPED
	else
	{
		assert(false);
	}
}