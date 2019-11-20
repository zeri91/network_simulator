#pragma warning(disable: 4786)
#pragma warning(disable: 4018)
#include <assert.h>
#include <list>
#include <set>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbstractPath.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "Connection.h"
#include "ConnectionDB.h"
#include "Lightpath.h"
#include "Lightpath_Seg.h"
#include "LightpathDB.h"
#include "Log.h"
#include "OXCNode.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "NetMan.h"
#include "Circuit.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

Circuit::Circuit(BandwidthGranularity eBW, CircuitType eCT, Circuit *pCircuit):
	m_eBW(eBW), m_eCircuitType(eCT), m_pCircuit(pCircuit),
	m_eState(CT_Ready), latency(0)
{
	m_hRouteA = list<Lightpath*>(0);
}


Circuit::~Circuit()
{
	// Lightpaths in m_hRoute are deleted in 
	// Circuit::tearDown & LightpathDB::~LightpathDB.
	// It causes dangling pointers if deleting here.
}

void Circuit::dump(ostream& out) const
{
	if (m_hRoute.empty())
		return;

#ifdef DEBUGB
	char*circuitType;
	if (m_eCircuitType == 0)
		circuitType = "CT_Unprotected";
	cout << "\n-> CIRCUIT DUMP: num di lightpath appartenenti al circuito, # = " << m_hRoute.size();
	cout <<"\tCircuit type: " << circuitType << endl;
#endif // DEBUGB

//	These info has been dumped in Connection::dump
//	AbstractNode *pNode = getSrc();
//	if (pNode)
//		out<<pNode->getId()<<"->";
//	pNode = getDst();
//	if (pNode)
//		out<<pNode->getId()<<" ";
//
//	out<<m_eBW<<endl;

	//-B: comment this part to not print every lightpath belonging to the circuit (it is equivalent to do LightpathDB.dump)
	/*
	list<Lightpath*>::const_iterator itr;
	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++)
	{
		assert(*itr);
		//-B: ATTENZIONE!!! Esegue la Lightpath_seg dump (NON Lightpath dump!)
		(*itr)->dump(out);
		out<<endl;
	}
	*/

	if ((CT_SPAC_SPP == m_eCircuitType) && (m_hBRouteInSG.size() > 0)) {
		SimplexLink *pSLink;
		list<AbstractLink*>::const_iterator itr;
		for (itr = m_hBRouteInSG.begin(); itr != m_hBRouteInSG.end(); itr++) {
			pSLink = (SimplexLink*)(*itr);
			switch (pSLink->m_eSimplexLinkType) {
			case SimplexLink::LT_UniFiber:
				assert(pSLink->m_pUniFiber);
				out.width(3);
				out<<pSLink->m_pUniFiber->m_pSrc->getId();
				break;
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Rx:
				NULL;
				break;
			default:
				(*itr)->dump(out);
				DEFAULT_SWITCH;
			}
		}
		out.width(3);
		out<<this->getDst()->getId()<<endl;
	}
}

void Circuit::setUp(NetMan* pNetMan)
{
	// Fabio 6 ott : m_hRoute è data dal routing per le nuove connessioni
	//				 ed è gia presente per le connessioni sospese 
	//				 temporaneamente


	//Fabio 27 Nov: devo modificare m:hRoute in modo che si riferisca ai link della rete originale

	
	//assert(CT_Setup != m_eState);
	assert(m_eBW > 0);
	assert(m_hRoute.size() > 0);
	list<Lightpath*>::iterator itr;

#ifdef DEBUGB
	cout << "Circuit state: " << m_eState << endl; // 0 = CT_Ready
#endif // DEBUGB

	//seleziono quale route usare in base al flag
	if (CT_SetupB == m_eState)
		m_hRoute = m_hRouteB;
	else if (CT_Setup == m_eState)
		m_hRoute = m_hRouteA;
	//else
		//assert(false);
	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++) {
		// new lightpath to be set up?
		if (0 == (*itr)->getId() || CT_SetupB == m_eState || CT_Setup == m_eState)
		{
			//-B: ATTENZIONE!!!!! go to Lightpath_Seg.setUp (NON Lightpath.setUp)
			(*itr)->setUp(pNetMan, this); //-B: LEGGI IL COMMENTO SOPRA!!!
		}
		else
		{
			(*itr)->consume(pNetMan, this);
		}
	}
	m_eState = CT_Setup;
}

void Circuit::WPsetUp(NetMan* pNetMan)
{
	// Fabio 6 ott : m_hRoute è data dal routing per le nuove connessioni
	//				 ed è gia presente per le connessioni sospese 
	//				 temporaneamente


	//Fabio 27 Nov: devo modificare m_hRoute in modo che si riferisca ai link della rete originale

	
//	assert(CT_Setup != m_eState);
	assert(m_eBW > 0);
	assert(m_hRoute.size() > 0);
	list<Lightpath*>::iterator itr;

	//seleziono quale route usare in base al flag
	if (CT_SetupB == m_eState)
		m_hRoute = m_hRouteB;
	else if (CT_Setup == m_eState)
		m_hRoute = m_hRouteA;
	
	/*
	else if (CT_Setup == m_eState && pNetMan->m_eProvisionType == NetMan::PT_BBU) //seleziono quale route usare in base al flag
		;	//-B: nothing to do. Let m_hRoute unchanged
			//	(altrimenti succede un casino per colpa del metodo RouteConverterUNPR che è una merda)
	else if (CT_Setup == m_eState && pNetMan->m_eProvisionType != NetMan::PT_BBU)
		m_hRoute = m_hRouteA;
	*/
	
	else
		assert(false);

	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++) {
		//-B: ATTENZIONE!!!!! go to Lightpath_Seg.WPsetUp (NON Lightpath.setUp)
		(*itr)->WPsetUp(pNetMan, this);
	}
	m_eState = CT_Setup;
}

void Circuit::tearDown(NetMan* pNetMan, bool bLog)
{
#ifdef DEBUGB
	cout << "->tearDown" << endl;
#endif // DEBUGB
	//assert(CT_Setup == m_eState );
	if (CT_SPAC_SPP == m_eCircuitType)
	{
		SPAC_DetachBackup(pNetMan);
	}
	if (CT_TorndownB == m_eState)
		m_hRoute = m_hRouteB;
	else if(CT_Torndown == m_eState)				//seleziono quale route usare in base al flag
		m_hRoute = m_hRouteA;
	else
		assert(false);

	bool bToDelete;
	list<Lightpath*>::iterator itr;
	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++) 
	{
		(*itr)->release(bToDelete, pNetMan, this, bLog);
		//(*itr)->restoreStatus();
	}
}

void Circuit::WPtearDown(NetMan* pNetMan, bool bLog)
{
#ifdef DEBUGB
	cout << "-> WPtearDown" << endl;
#endif // DEBUGB

	assert(CT_Setup == m_eState );
	
	if (CT_TorndownB == m_eState)
		m_hRoute = m_hRouteB;
	else if (CT_Torndown == m_eState)				//seleziono quale route usare in base al flag
		m_hRoute = m_hRouteA;
	else
		assert(false);
	
	bool bToDelete;
	list<Lightpath*>::iterator itr;
	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++)
	{
		//-B: LIGHTPATH_SEG WPRELEASE (not Lightpath.WPrelease!)
		(*itr)->WPrelease(bToDelete, pNetMan, this, bLog);
	}
}

//FABIO 29 Sett:
//Identica a tear down, però conserva la variabile m_hRoute...
//in modo da poter riattivare il circuito in modo identico

void Circuit::addVirtualLink(Lightpath *pLink)
{
	assert(pLink);
// NB: the assertion doesn't hold cause pLink->getSrc() return NULL before it
//		is set up
// #ifdef _OCHDEBUG
//	if (m_hRoute.size() > 0) {
//		assert(m_hRoute.back()->getDst() == pLink->getSrc());
//	}
// #endif
	m_hRoute.push_back(pLink);
}

void Circuit::addFrontVirtualLink(Lightpath *pLink)
{
	assert(pLink);
	// NB: the assertion doesn't hold cause pLink->getSrc() return NULL before it
	//		is set up
	// #ifdef _OCHDEBUG
	//	if (m_hRoute.size() > 0) {
	//		assert(m_hRoute.back()->getDst() == pLink->getSrc());
	//	}
	// #endif
	m_hRoute.push_front(pLink);
}

AbstractNode* Circuit::getSrc() const
{
	if (m_hRoute.empty())
		return NULL;
	else
		return m_hRoute.front()->getSrc();
}

AbstractNode* Circuit::getDst() const
{
	if (m_hRoute.empty())
		return NULL;
	else
		return m_hRoute.back()->getDst();
}

UINT Circuit::getLightpathHops() const
{
	return m_hRoute.size();
}

void Circuit::deleteTempLightpaths()
{
	list<Lightpath*>::iterator itr;
	for (itr=m_hRoute.begin(); itr!=m_hRoute.end(); itr++) {
		if (0 == (*itr)->getId())	// temp lightpath?
			delete (*itr);			// if yes, delete it
	}
}

UINT Circuit::getPhysicalHops() const
{
	UINT nPhysicalHops = 0;
	list<Lightpath*>::const_iterator itr;
	for (itr=m_hRoute.begin(); itr!=m_hRoute.end(); itr++)
		nPhysicalHops += (*itr)->getPhysicalHops();
	return nPhysicalHops;
}

/*
void Circuit::increaseConflictSet()
{
	assert(CT_PAC_SPP_Backup == m_eCircuitType);
	assert(m_pCircuit);

	BandwidthGranularity eBW = m_pCircuit->m_eBW;
	list<Lightpath*>::const_iterator itrLP;	// lightpaths traversed by the backup
	for (itrLP=m_hRoute.begin(); itrLP!=m_hRoute.end(); itrLP++) {
		UINT *pCSet = (*itrLP)->m_pCSet;
		assert(pCSet);

		list<Lightpath*>::const_iterator itr;
		for (itr=m_pCircuit->m_hRoute.begin(); itr!=m_pCircuit->m_hRoute.end(); 
		itr++) {
			// unifibers traversed by the primary
			list<UniFiber*>::const_iterator itrUniFiber;
			for (itrUniFiber=(*itr)->m_hPRoute.begin(); 
			itrUniFiber!=(*itr)->m_hPRoute.end(); itrUniFiber++) {
				pCSet[(*itrUniFiber)->getId()] += eBW;
			}
		}
	}
}

void Circuit::decreaseConflictSet()
{
	assert(CT_PAC_SPP_Backup == m_eCircuitType);
	assert(m_pCircuit);

	BandwidthGranularity eBW = m_pCircuit->m_eBW;
	list<Lightpath*>::const_iterator itrLP;	// lightpaths traversed by the backup
	// release backup resources for the primary
	for (itrLP=m_hRoute.begin(); itrLP!=m_hRoute.end(); itrLP++) {
		UINT *pCSet = (*itrLP)->m_pCSet;
		assert(pCSet);

		list<Lightpath*>::const_iterator itr;
		for (itr=m_pCircuit->m_hRoute.begin(); itr!=m_pCircuit->m_hRoute.end(); 
		itr++) {
			// unifibers traversed by the primary
			list<UniFiber*>::const_iterator itrUniFiber;
			for (itrUniFiber=(*itr)->m_hPRoute.begin(); 
			itrUniFiber!=(*itr)->m_hPRoute.end(); itrUniFiber++) {
				pCSet[(*itrUniFiber)->getId()] -= eBW;
			}
		}
	}

	// update max backup resources for each lightpath
	for (itrLP=m_hRoute.begin(); itrLP!=m_hRoute.end(); itrLP++) {
		UINT *pCSet = (*itrLP)->m_pCSet;
		assert(pCSet);

		UINT nMaxBackupCap = 0;
		UINT i;
		for (i=0; i<(*itrLP)->m_nCSetSize; i++) {
			if (pCSet[i] > nMaxBackupCap)
				nMaxBackupCap = pCSet[i];
		}
		assert((*itrLP)->m_nBackupCapacity >= nMaxBackupCap);
		(*itrLP)->m_nBWToBeReleased = 
				(*itrLP)->m_nBackupCapacity - nMaxBackupCap;
		(*itrLP)->m_nBackupCapacity = nMaxBackupCap;
	}
}
*/

void Circuit::SPAC_AttachBackup(NetMan *pNetMan, list<AbstractLink*>& hBackup)
{
	assert((0 == m_hBRouteInSG.size()) && (hBackup.size() > 0));

	SimplexLink *pSLink;
	list<AbstractLink*>::const_iterator itr;
	for (itr=hBackup.begin(); itr!=hBackup.end(); itr++) {
		pSLink = (SimplexLink*)(*itr);
		if (SimplexLink::LT_UniFiber == pSLink->getSimplexLinkType()) {
			assert(pSLink->m_pUniFiber);
			pSLink->m_nBackupCap += pSLink->m_nBWToBeAllocated;

			set<UINT> hFIdSet;	// set of UniFiber Ids
			list<Lightpath*>::const_iterator itrLP;	
			for (itrLP=m_hRoute.begin(); itrLP!=m_hRoute.end(); itrLP++) {
				list<UniFiber*>::const_iterator itrUniFiber;
				for (itrUniFiber=(*itrLP)->m_hPRoute.begin(); 
				itrUniFiber!=(*itrLP)->m_hPRoute.end(); itrUniFiber++) {
					UINT nFId = (*itrUniFiber)->getId();
					if (hFIdSet.find(nFId) == hFIdSet.end()) {
						hFIdSet.insert(nFId);
						pSLink->m_pCSet[nFId] += m_eBW;
						assert(pSLink->m_pCSet[nFId] <= pSLink->m_nBackupCap);
					}
				}
			}

			if (pSLink->m_nBChannels * OCLightpath < pSLink->m_nBackupCap) {
				pSLink->m_nBChannels++;
				assert(pSLink->m_nBChannels * OCLightpath>=pSLink->m_nBackupCap);
				// need to consume one more channel for backup
				pSLink->m_pUniFiber->consumeChannel(NULL, -1);
				// reflect to state graph
				pSLink->consumeBandwidth(OCLightpath);

				// need to consume one more grooming port
				OXCNode *pSrcOXC, *pDstOXC;
				pSrcOXC = ((Vertex*)pSLink->m_pSrc)->m_pOXCNode;
				pDstOXC = ((Vertex*)pSLink->m_pDst)->m_pOXCNode;
				assert(pSrcOXC && pDstOXC);
				pNetMan->consumeTx(pSrcOXC, -1);
				pNetMan->consumeRx(pDstOXC, -1);
			}
			m_hBRouteInSG.push_back(pSLink);
		} // if
	} // for
	// WRONG: TxE & RxE may be no longer there, save LT_UniFiber edges only
//	m_hBRouteInSG = hBackup;
}

void Circuit::SPAC_DetachBackup(NetMan *pNetMan)
{
	SimplexLink *pSLink;
	list<AbstractLink*>::const_iterator itr;
	for (itr=m_hBRouteInSG.begin(); itr!=m_hBRouteInSG.end(); itr++) {
		pSLink = (SimplexLink*)(*itr);
		if (SimplexLink::LT_UniFiber == pSLink->getSimplexLinkType()) {
			assert(pSLink && pSLink->m_pUniFiber);

			set<UINT> hFIdSet;	// set of UniFiber Ids
			list<Lightpath*>::const_iterator itrLP;	
			for (itrLP=m_hRoute.begin(); itrLP!=m_hRoute.end(); itrLP++) {
				list<UniFiber*>::const_iterator itrUniFiber;
				for (itrUniFiber=(*itrLP)->m_hPRoute.begin(); 
				itrUniFiber!=(*itrLP)->m_hPRoute.end(); itrUniFiber++) {
					UINT nFId = (*itrUniFiber)->getId();
					if (hFIdSet.find(nFId) == hFIdSet.end()) {
						hFIdSet.insert(nFId);
						pSLink->m_pCSet[nFId] -= m_eBW;
					}
				}
			}

			UINT nNewBackupCap = 0;
			UINT  nW;
			for (nW=0; nW<pSLink->m_nCSetSize; nW++) {
				if (nNewBackupCap < pSLink->m_pCSet[nW])
					nNewBackupCap = pSLink->m_pCSet[nW];
			}

			pSLink->m_nBackupCap = nNewBackupCap;
			assert(pSLink->m_nBChannels > 0);
			if ((pSLink->m_nBChannels - 1) * OCLightpath >= nNewBackupCap) {
				pSLink->m_nBChannels--;
				assert((int)(pSLink->m_nBChannels - 1) * (int)OCLightpath
					<= (int)nNewBackupCap);
				// need to release one more channel for backup
				pSLink->m_pUniFiber->releaseChannel(NULL);
				// reflect to state graph
				pSLink->releaseBandwidth(OCLightpath);

				// need to release one grooming port
				OXCNode *pSrcOXC, *pDstOXC;
				pSrcOXC = ((Vertex*)pSLink->m_pSrc)->m_pOXCNode;
				pDstOXC = ((Vertex*)pSLink->m_pDst)->m_pOXCNode;
				assert(pSrcOXC && pDstOXC);
				pNetMan->consumeTx(pSrcOXC, 1);
				pNetMan->consumeRx(pDstOXC, 1);
			}
		} // if
	} // for
	m_hBRouteInSG.clear();
}

void Circuit::MPAC_SetUp(NetMan* pNetMan)
{
	assert(CT_Setup != m_eState);
	assert(m_eBW > 0);

	list<Lightpath*>::iterator itr;
	for (itr=m_hRoute.begin(); itr!=m_hRoute.end(); itr++) {
		// new lightpath to be set up?
		if (0 == (*itr)->getId())
			(*itr)->MPAC_SetUp(pNetMan, this);
	}
	m_eState = CT_Setup;
}

void Circuit::MPAC_TearDown(NetMan* pNetMan, bool bLog)
{
	assert(CT_Setup == m_eState);
	assert((CT_PAC_SPP_Primary == m_eCircuitType) 
		|| (CT_PAC_SPP_Backup == m_eCircuitType));

	bool bToDelete;
	list<Lightpath*>::iterator itr;
	for (itr=m_hRoute.begin(); itr!=m_hRoute.end(); itr++) {
		(*itr)->MPAC_Release(bToDelete, pNetMan, bLog);
		if (bToDelete) {
			delete (*itr);
		}
	}

	m_hRoute.clear();
	m_eState = CT_Torndown;
}

void Circuit::PAL2_SetUp(NetMan* pNetMan)
{
	assert(CT_Setup != m_eState);
	assert(m_eBW > 0);

	list<Lightpath*>::iterator itr;
	for (itr=m_hRoute.begin(); itr!=m_hRoute.end(); itr++) {
		// new lightpath to be set up?
		if (0 == (*itr)->getId()) {
			pNetMan->PAL2_SP_SetUpLightpath(**itr);
		}
		assert((*itr)->m_nFreeCapacity >= m_eBW);
		(*itr)->m_nFreeCapacity -= m_eBW;
		(*itr)->m_hCircuitList.push_back(this);
	}
	m_eState = CT_Setup;
}

void Circuit::PAL2_TearDown(NetMan* pNetMan)
{
	assert(CT_Setup == m_eState);

	bool bToDelete;
	list<Lightpath*>::iterator itr;
	for (itr=m_hRoute.begin(); itr!=m_hRoute.end(); itr++) {
		pNetMan->PAL2_SP_ReleaseLightpath(**itr, bToDelete, *this);
		if (bToDelete) {
			// The lightpath has been removed in Lightpath::release
#ifdef _OCHDEBUG8
//			(*itr)->checkSanity();
#endif
			delete (*itr);
		}
	}

	m_hRoute.clear();
	m_eState = CT_Torndown;
}

void Circuit::BBU_setUpCir(NetMan* pNetMan)
{
	// Fabio 6 ott : m_hRoute è data dal routing per le nuove connessioni
	//				 ed è gia presente per le connessioni sospese 
	//				 temporaneamente


	//Fabio 27 Nov: devo modificare m:hRoute in modo che si riferisca ai link della rete originale


	//	assert(CT_Setup != m_eState);
	assert(m_eBW > 0);
	assert(m_hRoute.size() > 0);
	list<Lightpath*>::iterator itr;
	//seleziono quale route usare in base al flag
	if (CT_SetupB == m_eState)
		m_hRoute = m_hRouteB;
	else if (CT_Setup == m_eState)
		m_hRoute = m_hRouteA;
	else
		assert(false);

	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++) {
		// new lightpath to be set up?
		if (0 == (*itr)->getId() || CT_SetupB == m_eState || CT_Setup == m_eState)
		{
			//-B: ATTENZIONE!!!!! go to Lightpath_Seg.setUp (NON Lightpath.setUp)
			(*itr)->BBU_WPsetUpLp(pNetMan, this); //-B: LEGGI IL COMMENTO SOPRA!!!
		}
		else
		{
			(*itr)->consume(pNetMan, this);
		}
	}
	m_eState = CT_Setup;
}

void Circuit::BBU_WPsetUpCir(NetMan* pNetMan)
{
#ifdef DEBUGB
	cout << "CIRCUIT WPSETUP" << endl;
#endif // DEBUGB

	// Fabio 6 ott : m_hRoute è data dal routing per le nuove connessioni
	//				 ed è gia presente per le connessioni sospese 
	//				 temporaneamente


	//Fabio 27 Nov: devo modificare m_hRoute in modo che si riferisca ai link della rete originale


	//	assert(CT_Setup != m_eState);
	assert(m_eBW > 0);
	assert(m_hRoute.size() > 0);
	list<Lightpath*>::iterator itr;

	//seleziono quale route usare in base al flag
	if (CT_SetupB == m_eState)
		m_hRoute = m_hRouteB;
	else if (CT_Setup == m_eState)
		m_hRoute = m_hRouteA;
	else
		assert(false);

	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++)
	{
		cout << "LP ID: " << (*itr)->getId() << endl;
		if (0 == (*itr)->getId() && CT_Setup == m_eState)
		{
			cout << "NUOVO LIGHTPATH\n";
			//-B: ATTENZIONE!!!!! go to Lightpath_Seg.setUp (NON Lightpath.setUp)
			(*itr)->Unprotected_setUpLightpath(pNetMan, this);
		
		}
		else //if Lightpath id > 0
		{
			//-B: PER ORA CREDO CHE CONSUMO SOLO LA CAPACITA' DEL LIGHTPATH CHE CHIAMA IL METODO consume
			//	MA DEVO SOTTRARLA ANCHE A TUTTI GLI ALTRI LIGHTPAH COINCIDENTI!
			(*itr)->consume(pNetMan, this);
			cout << "LO CONSUMO. Banda disponibile: " << (*itr)->m_nFreeCapacity << endl;
		}

	}
	m_eState = CT_Setup;
}

void Circuit::Unprotected_setUpCircuit(NetMan* pNetMan)
{
#ifdef DEBUG
	cout << "-> Unprotected_setUpCircuit" << endl;
#endif // DEBUGB

	// Fabio 6 ott : m_hRoute è data dal routing per le nuove connessioni
	//				 ed è gia presente per le connessioni sospese 
	//				 temporaneamente

	//Fabio 27 Nov: devo modificare m:hRoute in modo che si riferisca ai link della rete originale

	assert(m_eBW >= 0); //-B: it was > only, I added = for a particular case
	assert(m_hRoute.size() > 0);
	list<Lightpath*>::iterator itr;

	//-B: since at this point m_eState == CT_Ready, it should not go into this
	//seleziono quale route usare in base al flag
	//if (CT_Setup == m_eState)
		//m_hRoute = m_hRouteA;

	//-B: scorro tutti i lightpath del circuito
	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++)
	{
		// --------------------- LIGHTPATH SETUP ---------------------
		(*itr)->Unprotected_setUpLightpath(pNetMan, this);
	}

	//-B: ---------------- UPDATE CIRCUIT STATUS -----------------
	this->m_eState = CT_Setup;
}

void Circuit::tearDownCircuit(NetMan* pNetMan, Connection* connectionDeparting, bool bLog)
{
#ifdef DEBUGC
	cout << "-> tearDownCircuit" << endl;
#endif // DEBUGB

	bool bToDelete;
	list<Lightpath*>::iterator itr;

	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++)
	{
		Lightpath* lightpathOnCircuit = (Lightpath*)(*itr);
#ifdef DEBUGC
		cout << "Lightpath from " << lightpathOnCircuit->getSrcOXC()->getId()
			<< " to " << lightpathOnCircuit->getDstOXC()->getId() << endl;
#endif
	}


	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++)
	{
		Lightpath* lightpathOnCircuit = (Lightpath*)(*itr);
		//if grooming has been done on the lightpath considered
		if (lightpathOnCircuit->m_groomingAlreadydone) {
#ifdef DEBUGC

			cout << "\tLightpath from " << lightpathOnCircuit->getSrcOXC()->getId() <<
				" to " << lightpathOnCircuit->getDstOXC()->getId() << " is doing grooming" << endl;
#endif
			//if the connection's source and the lightpath's source are equal,
			//it means that the connection departing is not involved in the grooming.
			if (connectionDeparting->m_nSrc == lightpathOnCircuit->getSrcOXC()->getId()) {
#ifdef DEBUGC

				cout << "\t-Connection from " << connectionDeparting->m_nSrc
					<< " to " << connectionDeparting->m_nDst
					<< " departing is not doing grooming on this lightpath" << endl;
				//cin.get();
#endif
			}
			else {
				//if the number of connections doing grooming is just 1 (which corresponds
				//to the connection departing), I have to update the latency for all the connections
				//using this lightpath and set the variable m_groomingAlreadydone related to the lightpaths to false
				if (lightpathOnCircuit->countConnectionsDoingGrooming(pNetMan) == 1) {
					list<Connection*>::const_iterator itr2;
					Connection* pConDB;

					for (itr2 = lightpathOnCircuit->m_hConnectionsUsingLightpath.begin(); itr2 != lightpathOnCircuit->m_hConnectionsUsingLightpath.end(); itr2++) {
						pConDB = (Connection*)(*itr2);
						pConDB->m_dRoutingTime -= (float)ELSWITCHLATENCY;
						if (pConDB->m_dRoutingTime < 0) {
							pConDB->m_dRoutingTime = 0;
						}
					
#ifdef DEBUGC
						cout << "\t+ Connection with id " << pConDB->m_nSequenceNo << " has to update its routing time" << endl;
						cout << "\t++After updating: " << pConDB->m_dRoutingTime << endl;
						//cin.get();
#endif
					}

#ifdef DEBUGC
					cout << "\tLightpath from " << lightpathOnCircuit->getSrcOXC()->getId() <<
						" to " << lightpathOnCircuit->getDstOXC()->getId() << " is NO MORE doing grooming" << endl;
#endif
					lightpathOnCircuit->m_groomingAlreadydone = false;
				}


			}
		}
		(*itr)->m_hConnectionsUsingLightpath.erase(connectionDeparting->m_nSequenceNo);

#ifdef DEBUGC	
		cout << "Connection from " << connectionDeparting->m_nSrc << " to " << connectionDeparting->m_nDst
			<< " deleted from lightpath from " << (*itr)->getSrcOXC()->getId() << 
			" to " << (*itr)->getDstOXC()->getId()<< endl;
		//-B: go to Lightpath.releaseLp (copy of Lightpath.release to distinguish it from Lightpath_Seg.release)
#endif
		(*itr)->releaseLp(bToDelete, pNetMan, this, bLog);
	}
}

void Circuit::Unprotected_setUpCircuitOCLightpath(NetMan* pNetMan)
{
#ifdef DEBUGB
	cout << "-> Unprotected_setUpCircuit" << endl;
	cout << "\tCircuit " << " state: " << m_eState << endl; // 0 = CT_Ready
	cout << "\tQUI. Circuit.m_hRoute size: " << m_hRoute.size() << " lightpath nel circuit" << endl;
#endif // DEBUGB

	// Fabio 6 ott : m_hRoute è data dal routing per le nuove connessioni
	//				 ed è gia presente per le connessioni sospese 
	//				 temporaneamente

	//Fabio 27 Nov: devo modificare m:hRoute in modo che si riferisca ai link della rete originale

	assert(m_eBW > 0);
	assert(m_hRoute.size() > 0);
	list<Lightpath*>::iterator itr;

	//-B: since at this point m_eState == CT_Ready, it should not go into this
	//seleziono quale route usare in base al flag
	//if (CT_Setup == m_eState)
	//m_hRoute = m_hRouteA;

	//-B: scorro tutti i lightpath del circuito (credo sempre e solo 1)
	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++) {
		// new lightpath to be set up?
		if (0 == (*itr)->getId())
		{
#ifdef DEBUGB
			cout << "\tVado a fare il setup del lightpath " << (*itr)->getId() << ": ";
			cout << (*itr)->getSrc()->getId() << "->" << (*itr)->getDst()->getId() << endl;
#endif // DEBUGB
			//-B
			if (WPFLAG)
			{
				(*itr)->BBU_WPsetUpLp(pNetMan, this);
			}
			else
			{
				(*itr)->Unprotected_setUpOCLightpath(pNetMan, this);
			}
		}
		//-B: don't need it anymore, it's all included in the methods above
		/*
		else
		{
			(*itr)->consume(pNetMan, this);
		}*/
	}
	m_eState = CT_Setup;

}

//-B: originally taken from WPtearDown
void Circuit::WPtearDownCircuit(NetMan* pNetMan, bool bLog)
{
#ifdef DEBUGB
	cout << "-> WPtearDownCircuit" << endl;
#endif // DEBUGB

	bool bToDelete;
	list<Lightpath*>::iterator itr;
	for (itr = m_hRoute.begin(); itr != m_hRoute.end(); itr++)
	{
		//-B: LIGHTPATH_SEG WPRELEASE (not Lightpath.WPrelease!)
		(*itr)->WPrelease(bToDelete, pNetMan, this, bLog);
	}
}

//-B: compute circuit latency: propagation + wavel conversion
void Circuit::computeLatency()
{
#ifdef DEBUGB
	cout << "-> computeLatency" << endl;
	cout << "\tCircuit route size: " << this->m_hRoute.size() << endl;
#endif // DEBUGB

	list<Lightpath*>::iterator itr1;
	list<UniFiber*>::iterator itr2;
	//int w, i;

	//-B: scan all the lightpaths in the circuit
	for (itr1 = this->m_hRoute.begin(); itr1 != this->m_hRoute.end(); itr1++)
	{ 
		//if lightpath's latency has not been computed yet
		if ((*itr1)->m_latency == 0)
		{
			for (itr2 = (*itr1)->m_hPRoute.begin(); itr2 != (*itr1)->m_hPRoute.end(); itr2++)
			{
#ifdef DEBUGB
				cout << "\tLength: " << (*itr2)->getLength()
					<< " - lat: " << (*itr2)->m_latency;
#endif // DEBUGB
				//compute lightpath's propagation latency
				(*itr1)->m_latency += (*itr2)->m_latency;
			}
			/*
			//scan all the fibers along the lightpath
			for (w = -1, i = 0, itr2 = (*itr1)->m_hPRoute.begin(); itr2 != (*itr1)->m_hPRoute.end()
				&& i < (*itr1)->wlsAssigned.size(); i++, itr2++) //wlsAssigned.size() == m_hPRoute.size() -> tante wl (= o != tra loro) quante fibre
			{
#ifdef DEBUGB
				cout << "\tLength: " << (*itr2)->getLength()
					<< " - lat: " << (*itr2)->m_latency;
#endif // DEBUGB
				//compute lightpath's propagation latency
				(*itr1)->m_latency += (*itr2)->m_latency;

				//check if there was a wavel conversion along the different fibers of the lightpath
				//if a wavel has already been saved
				if (w >= 0)
				{
					//if saved wavel != current wavel <==> wavel used in previous fiber != wavel used in current fiber
					if (w != (*itr1)->wlsAssigned[i])
					{
#ifdef DEBUGB
						cout << " + electronic switch - lat: " << (float)ELSWITCHLATENCY;
#endif // DEBUGB
						//compute lightpath's conversion wl latency
						(*itr1)->m_latency += (float)ELSWITCHLATENCY;
					}
					else
					{
						; //nothing to do
#ifdef DEBUGB
						cout << " + no wavel conversion";
#endif // DEBUGB
					}
				}
				//save last used wavelength
				w = (*itr1)->wlsAssigned[i];

#ifdef DEBUGB
				cout << endl;
#endif // DEBUGB
			}*/ //end FOR fibers
		} //end IF
		//compute circuit's latency
		this->latency += (*itr1)->m_latency;
	} //end FOR lightpaths
	
	//-B: if a circuit is composed by 2 or more lightpaths, independently of wether they have the same or different wavel,
	//	we have to take into account that there is a path termination in the destination node of the first lightpath
	this->latency += (this->m_hRoute.size() - 1) * ELSWITCHLATENCY;

#ifdef DEBUGB
	cout << " + " << this->m_hRoute.size() 
		<< " cambi di lunghezza d'onda <==> nodi attivi (con electronic switch) attraversati (no bypass)" << endl;
	cout << "\tCircuit latency: " << this->latency;
#endif // DEBUGB
}

void Circuit::assignBandwidthToAllocate()
{
	list<Lightpath*>::iterator itr;

	for (itr = this->m_hRoute.begin(); itr != this->m_hRoute.end(); itr++)
	{
		(*itr)->m_nBWToBeAllocated = this->m_eBW;
	}
}

bool Circuit::checkGrooming()
{
	list<Lightpath*>::const_iterator itr;
	Lightpath*pLp;

	itr = this->m_hRoute.begin();
	//-B: do not take into account the first lp, since it could be a case of multiplexing, but not of grooming
	//	grooming can happen only after the first lightpath of the path over which the connection is routed
	itr++;
	while (itr != this->m_hRoute.end())
	{
		pLp = (Lightpath*)(*itr);
		//-B: if in this lightpath is routed another circuit beside the current one
		if (pLp->m_hCircuitList.size() > 1)
		{
			return true;
		}
		itr++;
	}
	return false;
}