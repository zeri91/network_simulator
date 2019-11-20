#pragma warning(disable: 4786)
#pragma warning(disable: 4018)
#include <assert.h>
#include <list>
#include <set>

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
#include "ConnectionDB.h"
#include "Connection.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "Log.h"
#include "LightpathDB.h"
#include "NetMan.h"
#include "Circuit.h"
#include "Lightpath.h"
#include "OchMemDebug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

NS_OCH::Lightpath::Lightpath():
	AbstractLink(),
	m_bStateUpdated(false), m_hLifeTime(0), m_hBusyTime(0), m_pCSet(NULL), m_nCSetSize(0),
	m_nBackupCapacity(0), m_nBWToBeAllocated(0), m_nBWToBeReleased(0), wlAssigned(-1), backupLength(-2), primaryLength(-3)
{
}

Lightpath::Lightpath(UINT nId, LPProtectionType hType, UINT nCap):
    AbstractLink(),
    m_eProtectionType(hType), m_nCapacity(nCap), m_nFreeCapacity(nCap),
    m_bStateUpdated(false), m_hLifeTime(0), m_hBusyTime(0),
    m_pCSet(NULL), m_nCSetSize(0), 
    m_nBackupCapacity(0), m_nBWToBeAllocated(0), m_nBWToBeReleased(0), wlAssigned(-1), backupLength(-2), primaryLength(-3)
{
    m_nLinkId = nId;
	m_groomingAlreadydone = false;
}

Lightpath::~Lightpath()
{
    if (m_pCSet) {
        delete []m_pCSet;
        m_pCSet = NULL;
    }
}

const Lightpath & NS_OCH::Lightpath::operator=(const Lightpath & rhs)
{
		if (&rhs == this)
			return (*this);
		m_nLinkId = rhs.m_nLinkId;
		m_pSrc = rhs.m_pSrc;
		m_pDst = rhs.m_pDst;
		//m_hCost = rhs.m_hCost;
		//m_nLength = rhs.m_nLength;
		m_eProtectionType = rhs.m_eProtectionType;
		m_nCapacity = rhs.m_nCapacity;
		m_nFreeCapacity = rhs.m_nFreeCapacity;
		m_hCircuitList = rhs.m_hCircuitList;
		m_hPPhysicalPath = rhs.m_hPPhysicalPath;
		m_hPRoute = rhs.m_hPRoute;
		wlAssigned = wlAssigned;

		// todo: other stuff

		return (*this);
}

void Lightpath::dump(ostream& out) const
{
#ifdef DEBUGB
	char*protectionType;
	if (m_eProtectionType == 0)
	{
		protectionType = "LPT_PAL_Unprotected";
		//out << "\tLp protection Type: " << protectionType << endl;
	}
	else if (m_eProtectionType == 3)
	{
		protectionType = "LPT_PAL_Shared";
		//out << "\tLp protection Type: " << protectionType << endl;
	}
	else
	{
		out << "\tLp protection Type: " << m_eProtectionType << endl;
	}
#endif // DEBUGB
    
	out << "LP ";
	if (m_nLinkId < 10)
		out << " " << m_nLinkId << ": ";
	else
		out << m_nLinkId << ": ";
    if (m_pSrc && m_pDst)
        out<<m_pSrc->getId()<<"->"<<m_pDst->getId();
	out << "\tcost = ";
	//out.width(5);
	if (UNREACHABLE != m_hCost)
	{
		out.precision(5);
		out << m_hCost;
	}
	else
	{
		out << "\tINF";   // NO OUTPUT afterwards if out<<UNREACHABLE
	}
	out << "\tlength = ";
	out.width(7);
	out << m_nLength;
	out << "\tfreecap/cap = ";
	out.width(3);
	out << m_nFreeCapacity;
	out << "/";
	out.width(3);
	out << m_nCapacity;
	
	if (this->wlAssigned > -1)
		out << "\twl = " << this->wlAssigned + 1; //-B: +1 since wavelenght's range is 0-39, instead of 1-40
	if (this->wlAssigned == -1)
		out << "\twl = " << this->wlAssigned;
	out << "\tBWAlloc = " << m_nBWToBeAllocated;
	out << "\tBWRelease = " << m_nBWToBeReleased;
	out << "\tLifeTime = ";
	out.width(10);
	out << m_hLifeTime;
	out << "\tBusyTime = ";
	out.width(10);
	out << m_hBusyTime;
	out << "\tCircuitList size: " << this->m_hCircuitList.size();
	
	//-B: print path
	out << "\t[";
	list<UniFiber*>::const_iterator itr;
	int i;
	for (i = 0, itr = m_hPRoute.begin(); itr != m_hPRoute.end(); i++, itr++)
	{
		if ((i + 1) < m_hPRoute.size())
			out << (*itr)->getSrc()->getId() << " "; //-B: STRANO: in Lightpath_seg.dump questa linea di codice è: out >> m_pSrc->getId() + 1;
		else if ((i + 1) == m_hPRoute.size())
			out << (*itr)->getSrc()->getId();
	}
    if (m_pDst) {
        out.width(3);
		out << m_pDst->getId(); //-B: STRANO: in Lightpath_seg.dump questa linea di codice è: out >> m_pDst->getId() + 1;
    }

	//-B: if there is protection and so a backup path exists
    if ((LPT_PAL_Dedicated == m_eProtectionType) || (LPT_PAL_Shared == m_eProtectionType)) { // PAL2
        out<<", ";
		for (itr = m_hBRoute.begin(); itr != m_hBRoute.end(); itr++) {
            out.width(3);
            out<<(*itr)->getSrc()->getId();
        }
        if (m_pDst) {
            out.width(3);
            out<<m_pDst->getId();
        }
    }
    if (LPT_PAL_Shared == m_eProtectionType) { // PAL
        out<<", ";
        list<AbstractLink*>::const_iterator itr;
        for (itr=m_hBRouteInSG.begin(); itr!=m_hBRouteInSG.end(); itr++) {
            SimplexLink *pSLink = (SimplexLink*)(*itr);
            if (SimplexLink::LT_UniFiber == pSLink->getSimplexLinkType()) {
                out.width(3);
                out<<pSLink->m_pUniFiber->m_pSrc->getId();
            }
        }
        assert(m_pDst);
        if (m_pDst) {
            out.width(3);
            out<<m_pDst->getId();
        }
    }
	if (LPT_PAC_SharedPath == m_eProtectionType) {
		// output conflict set
		out << endl << "\tCS=";
		out << '{';
		UINT  i;
		for (i = 0; i<m_nCSetSize; i++) {
			out.width(3);
			out << m_pCSet[i];
			if (i<(m_nCSetSize - 1))
				out << ", ";
		}
		out << '}' << endl;
	}
    out<<"]";

}

AbstractLink::LinkType Lightpath::getLinkType() const
{
    return LT_Lightpath;
}

UINT Lightpath::getFreeCapacity() const
{
    return m_nFreeCapacity;
}

//-B: return the total num of hops over both primary and backup lightpaths
UINT Lightpath::getPhysicalHops() const
{
//  UINT nTotal = m_hPPhysicalPath.getPhysicalHops();
//  nTotal += m_hBPhysicalPath.getPhysicalHops();
    UINT nTotal = m_hPRoute.size() + m_hBRoute.size();
    return nTotal;
}

inline void Lightpath::attachToWDM(list<UniFiber*>& hPRoute,
                                   const AbstractPath& hPath,
                                   WDMNetwork* pWDM)
{
    list<UINT>::const_iterator itrCurr, itrNext;
    itrNext = hPath.m_hLinkList.begin();
    itrCurr = itrNext;
	
    while ((++itrNext) != hPath.m_hLinkList.end()) {
        hPRoute.push_back((UniFiber*)pWDM->lookUpLink(*itrCurr, *itrNext));
        itrCurr = itrNext;
    }

}

void Lightpath::WPsetUp(NetMan* pNetMan, Circuit *pCircuit){
	
	LINK_COST eps = pNetMan->m_hWDMNet.m_dEpsilon;
	if (0 == m_nLinkId)
		m_nLinkId = LightpathDB::getNextId();
	if (NULL == m_pSrc)
		m_pSrc = m_hPRoute.front()->getSrc();

	if (NULL == m_pDst)
		m_pDst = m_hPRoute.back()->getDst();
	//-B: sottraggo dalla capacità disponibile invece che da OCLightpath.
	//	Tanto essendo un nuovo Lightpath, la cap disponibile corrisponde a quella totale
	m_nFreeCapacity = m_nFreeCapacity - pCircuit->m_eBW;
	//m_nFreeCapacity = OCLightpath - pCircuit->m_eBW;

	// Update wavelength usage along the primary

	list<AbsPath*>::const_iterator itr;
	list<AbstractLink*>::const_iterator itrB;
	list<UniFiber*>::const_iterator itrC;
	list<UniFiber*>::const_iterator startPoint;
	updateChannelUsage(m_hPRoute);


	if (pNetMan->m_eProvisionType == NetMan::PT_wpUNPROTECTED || pNetMan->m_eProvisionType == NetMan::PT_BBU)
	{
		if (pCircuit->m_eState == Circuit::CT_Setup)
			pNetMan->appendLightpath(this, pCircuit->m_eBW);
		return; //-B: evito il resto del codice di questo metodo che tratta il caso in cui ci sia da fare protection
	}
	return;
	//assert(false);
}

void Lightpath::WPrelease(bool& bToDelete, NetMan *pNetMan,
							Circuit *pCircuit, bool bLog)
{
#ifdef DEBUGB
	cout << "-> WPrelease Lightpath" << endl;
#endif // DEBUGB

	m_nFreeCapacity = m_nCapacity;
	if (bLog)
		logFinal(pNetMan->m_hLog); //FABIO TOLTO 7 genn
	pNetMan->removeLightpath(this);
	tearDownOneRoute(m_hPRoute);
	bToDelete = true;

	//assert(false);
}

//-B: setup a NEW Lightpath
void Lightpath::setUp(NetMan* pNetMan, Circuit *pCircuit)
{
#ifdef DEBUGB
	cout << "LIGHTPAHT SETUP" << endl;
#endif // DEBUGB
    assert(pNetMan);
	
    // Attach WDM route
    attachToWDM(m_hPRoute, m_hPPhysicalPath, pNetMan->getWDMNetwork());
    attachToWDM(m_hBRoute, m_hBPhysicalPath, pNetMan->getWDMNetwork());

    if ((NULL == m_pSrc) || (NULL == m_pDst)) {
        AbstractLink *pLink = m_hPRoute.front();
        assert(pLink);
        m_pSrc = pLink->getSrc();
        pLink = m_hPRoute.back();
        assert(pLink);
        m_pDst = pLink->getDst();
    }
    
	// Update resource usage
    updateChannelUsage(m_hPRoute);
    updateChannelUsage(m_hBRoute);
    int nTxRxToConsume;
    if (LPT_PAL_Dedicated == m_eProtectionType)
        nTxRxToConsume = -2;
    else
        nTxRxToConsume = -1;
    // consume Tx/Rx, may need to update state graph
    pNetMan->consumeTx((OXCNode*)m_pSrc, nTxRxToConsume);
    pNetMan->consumeRx((OXCNode*)m_pDst, nTxRxToConsume);

    // Assign unique id if it's not assigned
    if (0 == m_nLinkId)
        m_nLinkId = LightpathDB::getNextId();
    // Update cost of this lightpath, minus SMALL_COST such that, when there
    // is a tie between lightpath & new wavelength-links, use lightpath
    m_hCost = getPhysicalHops() - SMALL_COST;
    assert(m_hCost >= 0);
    m_nLength = 1;
    assert(m_nFreeCapacity >= pCircuit->m_eBW);
    m_nFreeCapacity -= pCircuit->m_eBW;
    m_hCircuitList.push_back(pCircuit);

    // Add to lightpath db, and change network-state graph if necessary
    pNetMan->appendLightpath(this, pCircuit->m_eBW);

    switch (pCircuit->m_eCircuitType) {
    case Circuit::CT_PAC_SPP_Backup:
        increaseConflictSet(*pCircuit);
        break;
    case Circuit::CT_PAL_SPP:
        NULL;
        break;
    case Circuit::CT_Unprotected:
    case Circuit::CT_PAL_DPP:
    case Circuit::CT_PAC_DPP_Primary:
    case Circuit::CT_PAC_DPP_Backup:
    case Circuit::CT_PAC_SPP_Primary:
    case Circuit::CT_SPAC_SPP:
        NULL;           // nothing to do
        break;
    default:
        DEFAULT_SWITCH;
    }
    // reset
    m_nBWToBeAllocated = 0;
}

/*inline*/ void Lightpath::updateChannelUsage(list<UniFiber*>& hRoute)
{
  list<UniFiber*>::iterator itr;
  for (itr = hRoute.begin(); itr != hRoute.end(); itr++) {
		//di default wlAssigned è -1,cosi
		//se non lo specifico, il comportamento è
		//sempre come nel caso vwp
	    //-B: viene impostata a -1 nel costruttore Lightpath
        (*itr)->consumeChannel(this, 0, this->wlAssigned); // consume a free channel
	
		if( this->wlAssigned != -1) 
		{
			//-B: se siamo nel caso wp e quindi un certo canale può essere utilizzato solo una volta -> PERCHE' MAI??? Così non viene permesso il multiplexing!
			//	imposta a UNREACHABLE il costo di tale canale in modo tale che non si possa più riutlizzare
			//	questo viene fatto per tutti i link del lightpath di routing
			((*itr)->wlOccupation[this->wlAssigned]) = UNREACHABLE;
			((*itr)->wlOccupationBCK[this->wlAssigned]) = UNREACHABLE; //devo modificare anche costi di backup
		}
  }
}

//-B: cancellata per sbaglio -> rimando a quella modificata da me
void NS_OCH::Lightpath::tearDownOneRoute(list<UniFiber*>&hRoute)
{
	muxTearDownOneRoute(hRoute);
}

void Lightpath::consume(NetMan* pNetMan, Circuit* pCircuit)
{
#ifdef DEBUGB
	cout << "-> Lightpath consume" << endl;
#endif // DEBUGB

	cout << "BWD to be allocated " << m_nBWToBeAllocated << endl;
    assert(m_nFreeCapacity >= m_nBWToBeAllocated);
    m_hCircuitList.push_back(pCircuit);
    if (m_nBWToBeAllocated > 0) {
		//-B:---------- UPDATE LIGHTPATH FREE CAPACITY --------------
        m_nFreeCapacity -= m_nBWToBeAllocated;
        // PAC & PAL_SPP: need to update state graph
        pNetMan->consumeLightpathBandwidth(this, m_nBWToBeAllocated);
    }

    switch (pCircuit->m_eCircuitType) {
    case Circuit::CT_PAC_SPP_Backup:
        increaseConflictSet(*pCircuit);
        break;
    case Circuit::CT_PAL_SPP:
    case Circuit::CT_Unprotected:
    case Circuit::CT_PAL_DPP:
    case Circuit::CT_PAC_DPP_Primary:
    case Circuit::CT_PAC_DPP_Backup:
    case Circuit::CT_PAC_SPP_Primary:
    case Circuit::CT_SPAC_SPP:
        NULL;           // nothing to do
        break;
    default:
        DEFAULT_SWITCH;
    }
    // reset
    m_nBWToBeAllocated = 0;
}

void Lightpath::release(bool& bToDelete, NetMan *pNetMan, 
                        Circuit *pCircuit, bool bLog)
{
#ifdef DEBUGB
	cout << "-> Lightpath release" << endl;
#endif // DEBUGB
    UINT nBWToRelease;
    switch (pCircuit->m_eCircuitType)
	{
		case Circuit::CT_PAC_SPP_Backup:
			decreaseConflictSet(*pCircuit);
			nBWToRelease = m_nBWToBeReleased;
			break;
		case Circuit::CT_PAL_SPP:
		case Circuit::CT_Unprotected:
		case Circuit::CT_PAL_DPP:
		case Circuit::CT_PAC_DPP_Primary:
		case Circuit::CT_PAC_DPP_Backup:
		case Circuit::CT_PAC_SPP_Primary:
		case Circuit::CT_SPAC_SPP:
			nBWToRelease = pCircuit->m_eBW;
			break;
		default:
			DEFAULT_SWITCH;
    }
    if (0 == nBWToRelease) {
        bToDelete = false;
        return;
    }

    m_nFreeCapacity += nBWToRelease;
    assert(m_nFreeCapacity <= m_nCapacity);
    
	// no connection uses me anymore?
    if (m_nCapacity == m_nFreeCapacity)
	{
		cout << "1-I'M HERE!!!!!!" << endl;
        // For PAL_SPP
        if (Circuit::CT_PAL_SPP == pCircuit->m_eCircuitType)
            detachBackup();

        UINT nTxRxToRelease;
        if (LPT_PAL_Dedicated == m_eProtectionType)
            nTxRxToRelease = 2;
        else
            nTxRxToRelease = 1;

        pNetMan->consumeTx((OXCNode*)m_pSrc, nTxRxToRelease);
        pNetMan->consumeRx((OXCNode*)m_pDst, nTxRxToRelease);
        // In PAC, primary may be set up temporarily before computing backup.
        // The primary will be torn down if no backup found.
        // No need to log in that case
        if (bLog)   
            logFinal(pNetMan->m_hLog);
		cout << "2-I'M HERE!!!!!!" << endl;
		pNetMan->removeLightpath(this);
        // This MUST be the last thing to do.
        tearDownOneRoute(m_hPRoute);
        tearDownOneRoute(m_hBRoute);
        bToDelete = true;
    }
	else
	{
		cout << "3-I'M HERE!!!!!!" << endl;
        bToDelete = false;
        pNetMan->releaseLightpathBandwidth(this, nBWToRelease);
        // reset
        m_nBWToBeReleased = 0;
    }
}

void Lightpath::releaseOnServCopy(list<AbstractLink*>& hLinkToBeD2)
{
}

/*inline*/ void Lightpath::muxTearDownOneRoute(list<UniFiber*>& hRoute)
{
#ifdef DEBUGB
	cout << "-> muxTearDownOneRoute" << endl;
#endif // DEBUGB

	//-B: scorro tutte le fibre che compongono il lightpath
	list<UniFiber*>::iterator itr;
	for (itr = hRoute.begin(); itr != hRoute.end(); itr++)
	{
		// release a channel
		(*itr)->muxReleaseChannel(this);
		//cout<<"link "<<(*itr)->getId()<<" rimesso a 0";
		//cin.get();
	}
}

/*inline*/ void Lightpath::muxTearDownOneRoute(list<UniFiber*>& hRoute, NetMan*pNetMan)
{
#ifdef DEBUG
	cout << "-> muxTearDownOneRoute" << endl;
#endif // DEBUGB

	//-B: scorro tutte le fibre che compongono il lightpath
    list<UniFiber*>::iterator itr;
#ifdef DEBUG
	cout << endl << "\tScorro le fibre del lightpath" << endl;
#endif // DEBUGB
    for (itr = hRoute.begin(); itr != hRoute.end(); itr++)
	{
		// release a channel
        (*itr)->muxReleaseChannel(this, pNetMan);
		//cout<<"link "<<(*itr)->getId()<<" rimesso a 0";
		//cin.get();
	}
}

//Fabio 9 Ott:per il tear down temporaneo connessioni
/*void Lightpath::tDORTemp(list<UniFiber*>& hRoute)
{
    list<UniFiber*>::iterator itr;
    for (itr=hRoute.begin(); itr!=hRoute.end(); itr++)
        (*itr)->releaseChannelTemp(this);   // release a channel
}*/

void Lightpath::logPeriodical(Log &hLog, SimulationTime hTimeSpan)
{
    assert(m_nCapacity >= m_nFreeCapacity);
    m_hLifeTime += hTimeSpan;
    m_hBusyTime += (double)(m_nCapacity - m_nFreeCapacity) * hTimeSpan;
}


//-B: lifetime and busytime are computed every iteration.  On the contrary, this function is called whenever
//	a lightpath is removed from the graph (or when the simulation ends)
void Lightpath::logFinal(Log &hLog)
{
//  UINT nPHops = m_hPPhysicalPath.getPhysicalHops();
//  UINT nBHops = m_hBPhysicalPath.getPhysicalHops();
    UINT nPHops = m_hPRoute.size()-1;
    UINT nBHops = m_hBRoute.size();
    if (LPT_PAL_Shared == m_eProtectionType)    // for PAL2_SP
        nBHops = 0;
    hLog.m_nLightpath++; //-B: num of lightpaths activated during the simulation
    hLog.m_nPHopDistance += nPHops;
    hLog.m_nBHopDistance += nBHops;
    
	// statistic after it's done
    if (m_hLifeTime > 0) {
		//-B: somma del lifetime di tutti i lightpath attivati durante la simulazione
        hLog.m_dSumLightpathHoldingTime += m_hLifeTime;
		//-B: somma della capacità utilizzata durante tutta la simulazione / capacità disponibile durante tutta la simulazione
        hLog.m_dSumLightpathLoad += (double)(m_hBusyTime / ((double)m_nCapacity * m_hLifeTime));
		//-B: carico sulla rete durante la simulazione in termini di hop = link
        hLog.m_dSumLinkLoad += (double)(m_hLifeTime * (double)(nPHops + nBHops) * m_nCapacity);

      switch (m_eProtectionType)
	  {
        case LPT_PAL_Unprotected:
        case LPT_PAL_Dedicated:
            hLog.m_dSumTxLoad += (2 * m_hLifeTime);
            break;
        case LPT_PAL_Shared:
            hLog.m_dSumTxLoad += m_hLifeTime;
            break;
        case LPT_PAC_Unprotected:
        case LPT_PAC_DedicatedPath:
        case LPT_PAC_SharedPath:
        case LPT_SPAC_SharedPath:
            hLog.m_dSumTxLoad += m_hLifeTime;
            break;
        default:
            DEFAULT_SWITCH;
      }
    }

    hLog.m_dTotalUsedOC1Time += m_hBusyTime;
}

// test if the primary traverses UniFiber pUniFiber
bool Lightpath::traverseUniFiber(const UniFiber *pUniFiber) const
{
    list<UniFiber*>::const_iterator itr;
    for (itr=m_hPRoute.begin(); itr!=m_hPRoute.end(); itr++)
        if ((*itr) == pUniFiber)
            return true;

    return false;
}

OXCNode* Lightpath::getSrcOXC() const
{
    if (m_hPRoute.empty())
        return NULL;
    AbstractLink *pAbsLink = m_hPRoute.front();
    return (OXCNode*)pAbsLink->getSrc();
}

OXCNode* Lightpath::getDstOXC() const
{
    if (m_hPRoute.empty())
        return NULL;
    AbstractLink *pAbsLink = m_hPRoute.back();
    return (OXCNode*)pAbsLink->getDst();
}

bool Lightpath::allocateConflictSet(UINT nSize)
{
    assert(NULL == m_pCSet);

    m_nCSetSize = nSize;
    m_pCSet = new LINK_CAPACITY[nSize];
    memset(m_pCSet, 0, nSize * sizeof(LINK_CAPACITY));

    // NB: should check if memory allocation is successful or not
    return true;
}

void Lightpath::attachBackup(list<AbstractLink*>& hBackup)
{
    assert(0 == m_hBRouteInSG.size());

    SimplexLink *pSLink;
    list<AbstractLink*>::const_iterator itr;
    for (itr=hBackup.begin(); itr!=hBackup.end(); itr++) {
        pSLink = (SimplexLink*)(*itr);
        if (SimplexLink::LT_UniFiber == pSLink->getSimplexLinkType()) {
            assert(pSLink && pSLink->m_pUniFiber);

            list<UniFiber*>::const_iterator itr2;
            for (itr2=m_hPRoute.begin(); itr2!=m_hPRoute.end(); itr2++) {
                UINT nFiberId = (*itr2)->getId();
                pSLink->m_pCSet[nFiberId]++;
                if (pSLink->m_nBChannels < pSLink->m_pCSet[nFiberId]) {
                    pSLink->m_nBChannels++;
                    // needs to consume one more channel for backup
                    pSLink->m_pUniFiber->consumeChannel(NULL, -1);
                    // reflect to state graph
                    pSLink->consumeBandwidth(OCLightpath);
                }
            }
        }
    }
    m_hBRouteInSG = hBackup;
}

void Lightpath::detachBackup()
{
    SimplexLink *pSLink;
    list<AbstractLink*>::const_iterator itr;
    for (itr=m_hBRouteInSG.begin(); itr!=m_hBRouteInSG.end(); itr++) {
        pSLink = (SimplexLink*)(*itr);
        if (SimplexLink::LT_UniFiber == pSLink->getSimplexLinkType()) {
            assert(pSLink && pSLink->m_pUniFiber);

            list<UniFiber*>::const_iterator itr2;
            for (itr2=m_hPRoute.begin(); itr2!=m_hPRoute.end(); itr2++) {
                assert(pSLink->m_pCSet[(*itr2)->getId()] > 0);
                pSLink->m_pCSet[(*itr2)->getId()]--;
            }
            bool bReleaseChannel = true;
            UINT nE;
            for (nE=0; (nE<pSLink->m_nCSetSize) && bReleaseChannel; nE++) {
                if (pSLink->m_nBChannels == pSLink->m_pCSet[nE])
                    bReleaseChannel = false;
            }
            if (bReleaseChannel) {
                pSLink->m_nBChannels--;
                // needs to release a channel for backup
                pSLink->m_pUniFiber->releaseChannel(NULL);
                // reflect to state graph
                pSLink->releaseBandwidth(OCLightpath);
            }
        }
    }
}

void Lightpath::increaseConflictSet(const Circuit& hBCircuit)
{
    assert(Circuit::CT_PAC_SPP_Backup == hBCircuit.m_eCircuitType);
    assert(hBCircuit.m_pCircuit);
    assert(m_pCSet);
    // update the amount of backup resources
    m_nBackupCapacity += m_nBWToBeAllocated;

    BandwidthGranularity eBW = hBCircuit.m_pCircuit->m_eBW;
    set<UINT> hFIdSet;  // set of UniFiber Ids
    list<Lightpath*>::const_iterator itr;
    for (itr=hBCircuit.m_pCircuit->m_hRoute.begin(); 
    itr!=hBCircuit.m_pCircuit->m_hRoute.end(); itr++) {
        // unifibers traversed by the primary
        list<UniFiber*>::const_iterator itrUniFiber;
        for (itrUniFiber=(*itr)->m_hPRoute.begin(); 
        itrUniFiber!=(*itr)->m_hPRoute.end(); itrUniFiber++) {
            UINT nFId = (*itrUniFiber)->getId();
            if (hFIdSet.find(nFId) == hFIdSet.end()) {
                hFIdSet.insert(nFId);
                m_pCSet[nFId] += eBW;
                assert(m_pCSet[(*itrUniFiber)->getId()] <= m_nBackupCapacity);
            } else {
#ifdef _OCHDEBUG8
//              cout<<"- WARNING: primary traverses a fiber multiple times.";
//              cout<<endl<<"- Primary circuit: "<<endl;
//              hBCircuit.m_pCircuit->dump(cout);
//              cout<<"- Backup circuit: "<<endl;
//              hBCircuit.dump(cout);
#endif
            }
        }
    }
}

void Lightpath::decreaseConflictSet(const Circuit& hBCircuit)
{
    assert(Circuit::CT_PAC_SPP_Backup == hBCircuit.m_eCircuitType);
    assert(hBCircuit.m_pCircuit);
    assert(m_pCSet);

    BandwidthGranularity eBW = hBCircuit.m_pCircuit->m_eBW;
    set<UINT> hFIdSet;  // set of UniFiber Ids
    list<Lightpath*>::const_iterator itr;
    for (itr=hBCircuit.m_pCircuit->m_hRoute.begin(); 
    itr!=hBCircuit.m_pCircuit->m_hRoute.end(); itr++) {
        // unifibers traversed by the primary
        list<UniFiber*>::const_iterator itrUniFiber;
        for (itrUniFiber=(*itr)->m_hPRoute.begin(); 
        itrUniFiber!=(*itr)->m_hPRoute.end(); itrUniFiber++) {
            UINT nFId = (*itrUniFiber)->getId();
            if (hFIdSet.find(nFId) == hFIdSet.end()) {
                hFIdSet.insert(nFId);
                m_pCSet[nFId] -= eBW;
            }
        }
    }

    UINT nMaxBackupCap = 0;
    UINT i;
    for (i=0; i<m_nCSetSize; i++) {
        if (m_pCSet[i] > nMaxBackupCap)
            nMaxBackupCap = m_pCSet[i];
    }
    assert(m_nBackupCapacity >= nMaxBackupCap);
    m_nBWToBeReleased = m_nBackupCapacity - nMaxBackupCap;
    m_nBackupCapacity = nMaxBackupCap;
}

// for debug purpose
void Lightpath::checkSanity() const
{
    assert(0 == m_nBackupCapacity);
    if (m_pCSet) {
        UINT  nE;
        for (nE=0; nE<m_nCSetSize; nE++) {
            assert(0 == m_pCSet[nE]);
        }
    }
}

void Lightpath::MPAC_SetUp(NetMan* pNetMan, Circuit *pCircuit)
{
    assert(pNetMan);
    assert((Circuit::CT_PAC_SPP_Backup == pCircuit->m_eCircuitType) ||
        (Circuit::CT_PAC_SPP_Primary == pCircuit->m_eCircuitType));

    if ((NULL == m_pSrc) || (NULL == m_pDst)) {
        AbstractLink *pLink = m_hPRoute.front();
        assert(pLink);
        m_pSrc = pLink->getSrc();
        pLink = m_hPRoute.back();
        assert(pLink);
        m_pDst = pLink->getDst();
    }

    // Update resource usage
//  updateChannelUsage(m_hPRoute);
//  updateChannelUsage(m_hBRoute);

    // consume Tx/Rx, may need to update state graph
    pNetMan->consumeTx((OXCNode*)m_pSrc, -1);
    pNetMan->consumeRx((OXCNode*)m_pDst, -1);

    // Assign unique id if it's not assigned
//  if (0 == m_nLinkId)
//      m_nLinkId = LightpathDB::getNextId();

    // Update cost of this lightpath, minus SMALL_COST such that, when there
    // is a tie between lightpath & new wavelength-links, use lightpath
    m_hCost = getPhysicalHops();
//  m_hCost = getPhysicalHops() - SMALL_COST;
//  assert(m_hCost >= 0);
    m_nLength = 1;
    assert(m_nFreeCapacity >= pCircuit->m_eBW);
//  m_nFreeCapacity -= pCircuit->m_eBW;
    m_hCircuitList.push_back(pCircuit);

    // Add to lightpath db, and change network-state graph if necessary
//  pNetMan->appendLightpath(this, pCircuit->m_eBW);
}

void Lightpath::MPAC_Release(bool& bToDelete, 
                             NetMan *pNetMan, 
                             bool bLog)
{
    if (0 == m_nLinkId) {
        pNetMan->consumeTx((OXCNode*)m_pSrc, 1);
        pNetMan->consumeRx((OXCNode*)m_pDst, 1);
        // In PAC, primary may be set up temporarily before computing backup.
        // The primary will be torn down if no backup found.
        // No need to log in that case
        if (bLog)   
            logFinal(pNetMan->m_hLog);
//      pNetMan->removeLightpath(this);
        // This MUST be the last thing to do.
//      tearDownOneRoute(m_hPRoute);
//      tearDownOneRoute(m_hBRoute);
        bToDelete = true;
    } else {
        bToDelete = false;
    }
}

//-B: added by me to allow multiplexing
//	originally taken from updateChannelUsage
/*inline*/ void Lightpath::muxUpdateChannelUsage(list<UniFiber*>& hRoute, NetMan* pNetMan)
{
#ifdef DEBUGC
	cout << "-> muxUpdateChannelUsage" << endl;
#endif // DEBUGB

	//assert(hRoute.size() == wlsAssigned.size());
	int i;
	//-B: scorro tutte le fibre che compongono il lightpath
	list<UniFiber*>::iterator itr;
	for (i = 0, itr = hRoute.begin(); itr != hRoute.end() && i < wlsAssigned.size(); itr++, i++)
	{
		//-B: se fino ad adesso la fibra non è mai stata utilizzata
		if ((*itr)->m_used == 0)
		{
			(*itr)->m_used = 1; //-B: at least one channel is routed along it
		}
		//-B: viene impostata a -1 nel costruttore Lightpath
		(*itr)->muxConsumeChannel(this, this->wlsAssigned[i], pNetMan); // consume a free channel
	}
#ifdef DEBUGC
	cout << endl;
#endif // DEBUGB
}


//-B: originally taken from WPsetUp
void Lightpath::BBU_WPsetUpLp(NetMan* pNetMan, Circuit *pCircuit) {
#ifdef DEBUGB
	cout << "-> BBU_WPsetUpLp" << endl;
#endif // DEBUGB

	//-B: actually, part is commented, so it does nothing --> commented
	// Attach WDM route
	//attachToWDM(m_hPRoute, m_hPPhysicalPath, pNetMan->getWDMNetwork())

	//-B: not used, so commented
	//LINK_COST eps = pNetMan->m_hWDMNet.m_dEpsilon;

	// -------------- ASSIGN UNIQUE ID (if it's not assigned) -------------
	if (0 == m_nLinkId)
		m_nLinkId = LightpathDB::getNextId();
	if (NULL == m_pSrc)
		m_pSrc = m_hPRoute.front()->getSrc();
	if (NULL == m_pDst)
		m_pDst = m_hPRoute.back()->getDst();

	// ----------- UPDATE RESOURCE USAGE ALONG THE PATH -------------
	muxUpdateChannelUsage(m_hPRoute, pNetMan);

	// ------------------ CONSUME TX/RX --------------------
	int nTxRxToConsume;
	nTxRxToConsume = -1;
	// consume Tx/Rx, may need to update state graph
	pNetMan->consumeTx((OXCNode*)m_pSrc, nTxRxToConsume);
	pNetMan->consumeRx((OXCNode*)m_pDst, nTxRxToConsume);

	// --------------------- UPDATE LIGHTPATH'S COST : simply given by physical hops ---------------------------------
	// minus SMALL_COST such that, when there is a tie between lightpath & new wavelength-links, use lightpath
	m_hCost = getPhysicalHops() - SMALL_COST;
	assert(m_hCost >= 0);

	// -------------------- UPDATE LIGHTPATH LENGTH --------------------
	//-B: why should the lightpath's length (in km) be equal to 1, always??? commented!
	//m_nLength = 1;
	m_nLength = this->calculateLpLength();

	//------------------- UPDATE LIGHTPATH FREE_CAPACITY --------------------
	//-B: m_nBWToBeAllocated's value set in BBU_NewCircuit equal to pCircuit->m_eBW
	assert(m_nFreeCapacity >= m_nBWToBeAllocated);
	m_nFreeCapacity -= m_nBWToBeAllocated;
	//m_nFreeCapacity = m_nFreeCapacity - pCircuit->m_eBW;
	// reset value
	m_nBWToBeAllocated = 0;
#ifdef DEBUG
	cout << "\tLightpath free capacity: " << m_nFreeCapacity << endl;
#endif // DEBUGB

	// ------------------- ADD LIGHTPATH TO DB --------------------
	// Add to lightpath db, and change network-state graph if necessary
	pNetMan->appendLightpath(this, pCircuit->m_eBW);

	//-------------------- ADD CIRCUIT -------------------------
	m_hCircuitList.push_back(pCircuit);

	return;
}

void Lightpath::updatePhysicalPath(UniFiber* pUniFiber) {
	list<UniFiber*>::const_iterator first, last;
	first = this->m_hPRoute.begin();	
	if (pUniFiber->getId() == (*first)->getId())
	{		
		this->m_pSrc = pUniFiber->getSrc();		
		this->m_hPPhysicalPath.m_nSrc = this->m_pSrc->getId();
	}
	this->m_pDst = pUniFiber->getDst();			
	this->m_hPPhysicalPath.m_nDst = this->m_pDst->getId();
	this->m_hPPhysicalPath.m_hLinkList.push_back(pUniFiber->getId());
}

//-B: setup a NEW Lightpath
//	originally taken from setUp
//	************ USED BOTH FOR VWP AND WP CASES *************
void Lightpath::Unprotected_setUpLightpath(NetMan* pNetMan, Circuit *pCircuit)
{
#ifdef DEBUGC
	cout << "-> Unprotected_setUpLightpath" << endl;
	cout << "\tLightpath route size: " << m_hPRoute.size() << endl;
#endif // DEBUGB

	//-B: actually, part is commented, so it does nothing --> commented
	// Attach WDM route
	//attachToWDM(m_hPRoute, m_hPPhysicalPath, pNetMan->getWDMNetwork());
	
	// -------------- ASSIGN UNIQUE ID (if it's not assigned) -------------
	if (0 == m_nLinkId)
		m_nLinkId = LightpathDB::getNextId();
	if (NULL == m_pSrc)
		m_pSrc = m_hPRoute.front()->getSrc();
	if (NULL == m_pDst)
		m_pDst = m_hPRoute.back()->getDst();

	// -------------- UPDATE RESOURCE USAGE ALONG THE PATH ----------------
	muxUpdateChannelUsage(m_hPRoute, pNetMan);

	// ------------------ CONSUME TX/RX --------------------
	int nTxRxToConsume;
	nTxRxToConsume = 0;
	// consume Tx/Rx, may need to update state graph
	pNetMan->consumeTx((OXCNode*)m_pSrc, nTxRxToConsume);
	pNetMan->consumeRx((OXCNode*)m_pDst, nTxRxToConsume);

	// --------------------- UPDATE LIGHTPATH'S COST : simply given by physical hops ---------------------------------
	// minus SMALL_COST such that, when there is a tie between lightpath & new wavelength-links, use lightpath
	m_hCost = getPhysicalHops() - SMALL_COST;
	assert(m_hCost >= 0);
	
	// -------------------- UPDATE LIGHTPATH LENGTH --------------------
	//-B: why should the lightpath's length (in km) be equal to 1, always??? commented!
	//m_nLength = 1;
	m_nLength = this->calculateLpLength();
	
	//---------------- UPDATE LIGHTPATH FREE_CAPACITY ----------------
	//-B: m_nBWToBeAllocated's value set in BBU_NewCircuit equal to pCircuit->m_eBW
	assert(m_nFreeCapacity >= m_nBWToBeAllocated);
	//-B: find the smallest free capacity among the links that the lightpath is made of
	//LINK_CAPACITY minCap = this->findLeastCapacityLinkInLightpath(pNetMan);
#ifdef DEBUG
	cout << "\tLightpath #" << this->getId() << " free capacity before updating: " << m_nFreeCapacity << endl << endl;
#endif
	m_nFreeCapacity -= m_nBWToBeAllocated; //m_nFreeCapacity = minCap; //also: m_nFreeCapacity -= pCircuit->m_eBW
#ifdef DEBUG
	cout << "\tLightpath #" << this->getId() << " free capacity after updating: " << m_nFreeCapacity << endl << endl;
#endif // DEBUGB
	updateSimplexLinkLpCapacity(this, pNetMan, m_nBWToBeAllocated);
	// reset value
	m_nBWToBeAllocated = 0;

#ifdef DEBUGC
	if (m_nFreeCapacity < 29) {
	
		cout << "Lightpath from " << getSrcOXC()->getId() << " to " << getDstOXC()->getId()
			<< " has free capacity " << m_nFreeCapacity << endl;
		//cin.get();

	}
#endif
	// ------------------- ADD CIRCUIT TO LIGHTPATH'S CIRCUIT LIST --------------------
	m_hCircuitList.push_back(pCircuit);

	// ------------------- ADD LIGHTPATH TO LIGHTPATH'S DB -------------------
	if (!(this->isPresentInLightpathDB(pNetMan->getLightpathDB())))
		pNetMan->appendLightpath(this, pCircuit->m_eBW);

	return;
}

void Lightpath::updateSimplexLinkLpCapacity(Lightpath*pLightpath, NetMan*pNetMan, UINT BWToBeAllocated)
{
	pNetMan->consumeBandwOnSimplexLink(pLightpath, BWToBeAllocated);
}

//-B: find the smallest free capacity among the links that the lightpath is made of
LINK_CAPACITY Lightpath::findLeastCapacityLinkInLightpath(NetMan*pNetMan)
{
	list<UniFiber*>::const_iterator itr;
	SimplexLink*pLink;
	LINK_CAPACITY min = OCLightpath;
	int i = 0;
	for (itr = this->m_hPRoute.begin(); itr != this->m_hPRoute.end(); i++, itr++)
	{
		for (int w = 0; w < (*itr)->m_nW; w++)
		{
			if (w == this->wlsAssigned[i])
			{
				pLink = (SimplexLink*)pNetMan->lookUpSimplexLink((*itr)->getSrc(), (*itr)->getDst(), w);
				if (pLink->m_hFreeCap < min)
				{
					min = pLink->m_hFreeCap;
				}
			}
		}
	}
	return min;
}


//-B: originally taken from Lightpath.release to distinguish it from Lightpath_Seg.release
void Lightpath::releaseLp(bool& bToDelete, NetMan *pNetMan,
	Circuit *pCircuit, bool bLog)
{
#ifdef DEBUGB
	cout << "-> Lightpath releaseLp" << endl;
#endif // DEBUGB

	UINT nBWToRelease;
	switch (pCircuit->m_eCircuitType)
	{
		case Circuit::CT_PAC_SPP_Backup:
			decreaseConflictSet(*pCircuit);
			nBWToRelease = m_nBWToBeReleased;
			break;
		case Circuit::CT_PAL_SPP:
		case Circuit::CT_Unprotected:
		case Circuit::CT_PAL_DPP:
		case Circuit::CT_PAC_DPP_Primary:
		case Circuit::CT_PAC_DPP_Backup:
		case Circuit::CT_PAC_SPP_Primary:
		case Circuit::CT_SPAC_SPP:
			nBWToRelease = pCircuit->m_eBW; //-B: my case
			break;
		default:
			DEFAULT_SWITCH;
	}
	if (0 == nBWToRelease) {
		bToDelete = false;
		return;
	}

	//-B: --------------------- RELEASE LIGHTPATH CAPACITY ------------------------
	//	ON LIGHTPATH...
	m_nFreeCapacity += nBWToRelease;
	//	...AND ON SIMPLEX LINK LT_Lightpath
	pNetMan->releaseVirtualLinkBandwidth(nBWToRelease, this);

#ifdef DEBUGC
	cout << "\tFreeCap del lightpath " << this->getId() << " = " << m_nFreeCapacity << " (ho rilasciato " << nBWToRelease << ")" << endl;
#endif // DEBUGB
	assert(m_nFreeCapacity <= m_nCapacity);

	//-B: RELEASE CAPACITY ON SIMPLEX LINKS LT_Channel
	//pNetMan->releaseLightpathBandwidth(this, nBWToRelease);
	pNetMan->releaseLinkBandwidth(this, nBWToRelease);
	//-B: if a fiber is not used anymore, restore m_used variable
	restoreUsedStatus(m_hPRoute, pNetMan);
	// This MUST be the last thing to do //-B: I cannot see the reason why it should be
	muxTearDownOneRoute(m_hPRoute, pNetMan);
	// reset value (in case it deletes the lightpath, there shoul be unnecessary)
	m_nBWToBeReleased = 0;

	// no connection uses me (lightpath) anymore? -B: it means that it allows more than 1 connection using the same lightpath
	if (m_nFreeCapacity == OCLightpath) //-B: (m_nCapacity == m_nFreeCapacity) should be used instead for a more general case
										//	but it would require to check if it is fine with the rest of the code
	{
#ifdef DEBUGC
		cout << "\tLightpath vuoto! Lo elimino" << endl;
#endif // DEBUGB

		UINT nTxRxToRelease;
		nTxRxToRelease = 0;
		pNetMan->consumeTx((OXCNode*)m_pSrc, nTxRxToRelease);
		pNetMan->consumeRx((OXCNode*)m_pDst, nTxRxToRelease);
		// In PAC, primary may be set up temporarily before computing backup.
		// The primary will be torn down if no backup found.
		// No need to log in that case
		if (bLog) //-B: bLog = true by method's declaration
			logFinal(pNetMan->m_hLog); //-B: to do when a lightpath is removed
		
		//remove simplex link LT_Lightpath from graph
		pNetMan->removeSimplexLinkFromGraph(this);

#ifdef DEBUGB
		//check is the removal has been done correctly
		pNetMan->checkSimplexLinkRemoval(this);
#endif // DEBUGB

		//remove lightpath from lightpath db
		pNetMan->removeLightpath(this); //-B: since the lightpath is deleted, there's no need to reset m_nBWToBeReleased's value	
		
		bToDelete = true;
	}
	else
	{
		bToDelete = false;
	}

	//-B: delete circuit from lightpath's circuitlist, if there is more than 1 circuit
	//if (this->m_hCircuitList.size() > 1)
	//{
		//this->deleteCircuit(pCircuit);
	//}

	// This MUST be the last thing to do
	//muxTearDownOneRoute(m_hPRoute);
}

void Lightpath::deleteCircuit(Circuit*pCircuit)
{
	list<Circuit*>::iterator itr;
	Circuit*pCir;
	bool found = false;

	for (itr = this->m_hCircuitList.begin(); found = false; itr++)
	{
		pCir = (Circuit*)(*itr);
		if (pCir == pCircuit)
		{
			this->m_hCircuitList.erase(itr);
			found = true;
		}
	}
}

void Lightpath::restoreUsedStatus(list<UniFiber*>& hPRoute, NetMan *pNetMan)
{
#ifdef DEBUG
	cout << "-> restoreUsedStatus" << endl;
#endif // DEBUGB

	int i, flag;
	list<UniFiber*>::const_iterator itrFiber;
	//-B: scorro tutte le fibre del lightpath
	for (itrFiber = hPRoute.begin(); itrFiber != hPRoute.end(); itrFiber++)
	{
#ifdef DEBUGC
		cout << "\tFiber " << (*itrFiber)->getId();
#endif // DEBUGB
		//-B: scorro tutti i canali della fibra
		for (i = 0, flag = 0; i < (*itrFiber)->m_nW; i++)
		{
			if (!(pNetMan->checkFreedomSimplexLink((*itrFiber)->getSrc()->getId(), (*itrFiber)->getDst()->getId(), i)))
				flag = 1;
		}
		if (flag == 0)
		{
			(*itrFiber)->m_used = 0;
#ifdef DEBUGC
			cout << " non piu' utilizzata!" << endl;
#endif // DEBUGB
		}
		else
		{
#ifdef DEBUGC
			cout << " utilizzata da altri lightpath" << endl;
#endif // DEBUGB
		}
	}
}

//-B: setup a NEW Lightpath
//	originally taken from setUp
void Lightpath::Unprotected_setUpOCLightpath(NetMan* pNetMan, Circuit *pCircuit)
{
#ifdef DEBUGB
	cout << "-> Unprotected_setUpOCLightpath" << endl;
#endif // DEBUGB

	//-B: actually, part is commented, so it does nothing --> commented
	// Attach WDM route
	//attachToWDM(m_hPRoute, m_hPPhysicalPath, pNetMan->getWDMNetwork());
	
	// -------------- ASSIGN UNIQUE ID (if it's not assigned) -------------
	if (0 == m_nLinkId)
		m_nLinkId = LightpathDB::getNextId();
	if (NULL == m_pSrc)
		m_pSrc = m_hPRoute.front()->getSrc();
	if (NULL == m_pDst)
		m_pDst = m_hPRoute.back()->getDst();

	// ----------- UPDATE RESOURCE USAGE ALONG THE PATH -------------
	muxUpdateChannelUsage(m_hPRoute, pNetMan);
	
	// ------------------ CONSUME TX/RX --------------------
	int nTxRxToConsume;
	nTxRxToConsume = -1;
	// consume Tx/Rx, may need to update state graph
	pNetMan->consumeTx((OXCNode*)m_pSrc, nTxRxToConsume);
	pNetMan->consumeRx((OXCNode*)m_pDst, nTxRxToConsume);

	// --------------------- UPDATE LIGHTPATH'S COST : simply given by physical hops ---------------------------------
	// minus SMALL_COST such that, when there is a tie between lightpath & new wavelength-links, use lightpath
	m_hCost = getPhysicalHops() - SMALL_COST;
	assert(m_hCost >= 0);

	//-B: why should the lightpath's length (in km) be equal to 1, always??? commented!
	//m_nLength = 1;

	//---------------- UPDATE LIGHTPATH FREE_CAPACITY ----------------
	//-B: m_nBWToBeAllocated's value set in BBU_NewCircuit equal to pCircuit->m_eBW
	assert(m_nFreeCapacity >= m_nBWToBeAllocated);
	m_nFreeCapacity -= m_nBWToBeAllocated; //-B: also m_nFreeCapacity -= pCircuit->m_eBW
	// reset value
	m_nBWToBeAllocated = 0;

	// ------------------- ADD LIGHTPATH TO DB --------------------
	// Add to lightpath db, and change network-state graph if necessary
	pNetMan->appendLightpath(this, pCircuit->m_eBW);

	//-------------------- ADD CIRCUIT -------------------
	m_hCircuitList.push_back(pCircuit);

	return;
}

float NS_OCH::Lightpath::calculateLpLength()
{
	list<UniFiber*>::const_iterator itr;
	float length = 0;
	for (itr = m_hPRoute.begin(); itr != m_hPRoute.end(); itr++)
	{
		length += (*itr)->getLength();
	}
	return length;
}

UINT Lightpath::countConnectionsDoingGrooming(NetMan* pNetMan) 
{
#ifdef DEBUG
	cout << "-> countConnectionsDoingGrooming" << endl;
#endif // DEBUGB
	UINT connectionsDoingGrooming = 0;
	list<Connection*>::const_iterator itrConn;
	Connection* pConDB;

	for (itrConn = this->m_hConnectionsUsingLightpath.begin(); itrConn != this->m_hConnectionsUsingLightpath.end(); itrConn++) {

		pConDB = (Connection*)(*itrConn);

		if (pConDB->m_nSrc != this->getSrcOXC()->getId()) {
#ifdef DEBUGX
			cout << "Connection from " << pConDB->m_nSrc << " with id " << pConDB->m_nSequenceNo
				<< " is doing grooming on the lightpath from "
				<< this->getSrcOXC()->getId() << " to " << this->getDstOXC()->getId() << endl;
#endif					
			connectionsDoingGrooming += 1;
		}

	}

#ifdef DEBUGC
	cout << "Connections doing grooming on this lightpath: " << connectionsDoingGrooming << endl;
#endif
	return connectionsDoingGrooming;
}


bool Lightpath::isPresentInLightpathDB(LightpathDB&hLightpathDB)
{
	list<Lightpath*>::iterator itr;
	for (itr = hLightpathDB.m_hLightpathList.begin(); itr != hLightpathDB.m_hLightpathList.end(); itr++)
	{
		if (this->getId() == (*itr)->getId())
			return true;
	}
	return false;
}


