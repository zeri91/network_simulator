#pragma warning(disable: 4786)
#pragma warning(disable: 4018)
#include <assert.h>
#include <vector>
#include <algorithm>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "OXCNode.h"
#include "AbstractPath.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "TopoReader.h"
#include "SimplexLink.h"
#include "Vertex.h"
#include "Graph.h"
#include "Connection.h"
#include "ConnectionDB.h"
#include "Lightpath.h"
#include "Lightpath_Seg.h"
#include "LightpathDB.h"
#include "Circuit.h"
#include "EventList.h"
#include "Event.h"
#include "Log.h"
#include "NetMan.h"
#include "Channel.h"
#include "OchMemDebug.h"
#include "ConstDef.h"
#include "BinaryHeap.h"
#include "OXCNode.h"


//#define DEBUGMASSIMO
//#define Con_Prov
//#define DEBUG_FABIO
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bool dbgactive;
using namespace NS_OCH;

//-B: builder modified to set specific parameters
NetMan::NetMan(): m_dTxThreshold(1.0), m_nNumberOfPaths(1), m_hCostSlack(0),
    m_bWithOptimization(false),pc(0),bc(0),fc(0),bothc(0),pcPer(0),bcPer(0),fcPer(0),bothcPer(0),pcTot(0),bcTot(0),fcTot(0),bothcTot(0),n(0)
{	
	p_block = new Sstat();	// Add by ANDREA
	stop_perc = 5.0;		// Add by ANDREA
    conf = 0.95;			// Add by ANDREA
	isLinkDisjointActive=true; //FABIO
	pathTotCost=0;

	list<Connection*>::iterator itr;
	for (itr = groomingConnections.begin(); itr != groomingConnections.end(); itr++)
		delete (*itr);
}

NetMan::~NetMan()
{
}

//-B: method to print stats: mean, variance, stdDev, Error (= confidence percentage error -> margin of error)
//	-> taken from the Stat object referenced by p_block
//	(p_block is an attribute of 'this' object, that is a NetMan class object)
void NetMan::printStat(ostream& out) const{
	cout << endl << "\n-> printStat" << endl;

	this->p_block->StatLog(this->p_block, conf, "BLOCKING PROB", out);

	cout << "---------------ESCO printStat---------------" << endl;

}

//-B: update stats about blocking prob and conflict's stats
//	if a true value is returned for okstat var the while cycle (in the run method of Simulator class) stops
// Add by Andrea
bool NetMan::updateStat(UINT connection, UINT nRunConnections) {
	double block = m_runLog.m_nBlockedCon; //FABIO ANDREA
	double prov = m_runLog.m_nProvisionedCon;
	bool okstat = false; 
	//*p_block += (double)(block/prov); 
	*p_block += (double)(block/nRunConnections);	//-B: since stats are computed every N_RUN_CONNECTIONS (m_nRunConnections is set in Simulator class)
	n++;			//-B: num of times stats are updated???
	//-B: update conflict count
	pcTot += pc;
	fcTot += fc;
	bcTot += bc;
	bothcTot += bothc;
	//-B: update conflict stats
	pcPer = pcTot / 100 / n;
	bcPer = bcTot / 100 / n;
	fcPer = fcTot / 100 / n;
	bothcPer = bothcTot / 100 / n;
	//-B: reset temporary stat values
	pc = 0; bc = 0; fc = 0; bothc = 0;

	//*wl_num += (double)m_runLog.m_dSumLinkLoad;		// #link*192
	//*tx_num += (double)m_runLog.m_dSumTxLoad; 		// #tx
	
	//-B: useless switch?!
	switch (m_eStatActive){
		case PBLOCK:
		{												//ALMENO 300000 connessioni..
			//cout << endl << "******************* IS CONFIDENCE SATISFIED???" << endl;
			//if((p_block->isconfsatisfied(stop_perc , conf) == true /*&& (connection >= 300000)*/) || (p_block->mean() == 0.0 && (connection) == 1000000))   //tolto per evitare che una volta raggiunta la confidenza si fermi la simulazione
			//{
				//connReached = connection;
				//okstat = true;
				//cout << "******************************* CONFIDENZA SODDISFATTA!!! Premi invio" << endl << endl;
				//cin.get();
			//}
			break;
		}
		default:
			DEFAULT_SWITCH;
    }
	return okstat; //-B: read comments above about the value returned
}

//-B: it should print NetMan object status (useful for debugging purpose; to be tried)
//	it also dump LightpathDB and Graph objects (ConnectionDB.dump and WDMNet.dump were commented)
void NetMan::dump(ostream& out)
{
	//-B: per il dump del WDMNetwork object è stata fatta una funzione a parte
	// m_hWDMNet.dump(out);
    // out<<endl;

#ifdef DEBUGB
	out << "\n-> NetMan DUMP" << endl;
#endif // DEBUGB

	//out<<"- NetMan @ "<<(void*)this<<':'<<endl;
	out << " \t#OfPaths in Yen's algorithm = " << m_nNumberOfPaths; //-B: it simply is the num of k shortest paths to be computed in the Yen's algorithm
	out << " | conf: " << conf
		<< " | TxReq per lp: " << m_nTxReqPerLightpath
		<< " | Time policy: " << m_nTimePolicy;
	out<<" | ProvisionType = ";
    char *pProvisionType;
    switch (m_eProvisionType)
	{
		case PT_SEG_SP_NO_HOP:
		   pProvisionType = "SEG_NO_HOP";
		  break;
		 case PT_SEG_SP_L_NO_HOP:
			pProvisionType = "SEG_L_NO_HOP";
			break;
		case PT_SEG_SPP:
			pProvisionType = "SEG_SPP";
			break;
		case PT_SEG_SP_B_HOP:
			pProvisionType = "SEG_B_HOP";
		case PT_wpSEG_SPP:
			pProvisionType = "wpSEG_SPP";
			break;
		case PT_wpPAL_DPP:
			pProvisionType = "DED wp";
			break;
		case PT_PAL_DPP:
			pProvisionType = "DED ";
			break;
		case PT_wpUNPROTECTED:
			pProvisionType = "UNPR wp";
			break;
		case PT_UNPROTECTED_GREEN:
			pProvisionType = "UNPR GREEN";
			break;
		case PT_BBU:
			pProvisionType = "BBU_HOTEL";
			break;
		default:
			DEFAULT_SWITCH;
    }
	out << pProvisionType;
	out << " | GroomingPolicy: " << m_eGroomingPolicy;
	out << endl;
	//cin.get();
	//-B: CONNECTIONDB DUMP
	m_hConnectionDB.dump(out);
	out<<endl;
	//-B: LIGHTPATHDB DUMP
	m_hLightpathDB.dump(out);
	out<<endl;
	//-B: WDMNet DUMP
	if (pProvisionType = "BBU_HOTEL")
	{
		m_hWDMNet.BBU_WDMNetdump(out);
		this->simplexLinkDump();
	}
	else
		m_hWDMNet.dump(out);

	///////////////////////////////////////////-B: WDM network -> Graph Simplex Link dump
#ifdef DEBUGB
	//cout << "\n-> 'simplexLinkDump'" << endl;
#endif // DEBUGB
	this->linkCapacityDump();

	//////////////////////////////////////////////
	//out << endl;
	//out << -> WDMNetPast DUMP" << endl;
	//m_hWDMNetPast.dump(out);
	//cin.get();
	//out << endl;
	//-B: GRAPH DUMP (too long)
    //m_hGraph.dump(out); //Graph object should represent the network state
}

//-B: it should print the WDMNetwork object status (useful for debugging purpose; to be tried)
void NetMan::WDMNetdump(ostream& out) const
{
	  m_hWDMNet.dump(out);
      out<<endl;
	
	/*out<<"- NetMan @"<<(void*)this<<':'<<endl;
    out<<"  #OfPaths = "<<m_nNumberOfPaths<<endl;
    out<<"  ProvisionType = ";
    char *pProvisionType;
    switch (m_eProvisionType) {
		case PT_SEG_SP_NO_HOP:
			pProvisionType = "SEG_NO_HOP";
			break;
		case PT_SEG_SP_L_NO_HOP:
			pProvisionType = "SEG_L_NO_HOP";
			break;
		case PT_SEG_SPP:
			pProvisionType = "SEG_SPP";
			break;
		case PT_SEG_SP_B_HOP:
			pProvisionType = "SEG_B_HOP";
			break;
		default:
			DEFAULT_SWITCH;
    }
    out<<pProvisionType<<endl;
    m_hConnectionDB.dump(out);
    out<<endl;
    m_hLightpathDB.dump(out);
    out<<endl;
	//m_hWDMNet.dump(out);
	//out<<endl;
    m_hGraph.dump(out);
    out<<"- NetMan end of dump"<<endl;*/
}

//-B: return true if the initialization of the NetMan object has been done correctly (with all the input parameters)
//	this method is called in ConProvisionMain.cpp and it is called before the initialization of Simulator object
//	this method calls 2 main methods: readTopo (-> readTopoHelper) and genInitialStateGraph
bool NetMan::initialize(const char* pTopoFile, 
                        const char* pProtectionType, 
                        int nNumberOfPaths,
                        double dEpsilon,
                        int nTimePolicy,
						const char* UnAv) // M//ANDREA
{

#ifdef DEBUGB
	cout << "\nINIZIALIZZO hNetMan - PREMI INVIO" << endl;
	//cin.get();
#endif // DEBUGB
	
    assert(pTopoFile && pProtectionType);
    TopoReader hTopoReader;
	if (!hTopoReader.readTopo(m_hWDMNet, pTopoFile))
	{
		cerr<<"- Error reading topo file "<<pTopoFile<<endl;
		return false;
	}

    // if (!hTopoReader.readTopo(m_hServWDMNet, pTopoFile)) {
    //    cerr
    //}
    // Parse protection type<<"- Error reading topo Serv file "<<pTopoFile<<endl;
    //    return false;

	//-B: set the provisioning type
    if (0 == strcmp(pProtectionType, "SEG_SP_NO_HOP")) {
        m_eProvisionType = PT_SEG_SP_NO_HOP;
    } else if (0 == strcmp(pProtectionType, "wpSEG_SPP")) {
        m_eProvisionType = PT_wpSEG_SPP;
	} else if (0 == strcmp(pProtectionType, "wpUNPROTECTED")) {
        m_eProvisionType = PT_wpUNPROTECTED;
	} else if (0 == strcmp(pProtectionType, "wpDEDICATED")) {
        m_eProvisionType = PT_wpPAL_DPP;
	} else if (0 == strcmp(pProtectionType, "SPPBw")) {
        m_eProvisionType = PT_SPPBw;
	} else if (0 == strcmp(pProtectionType, "SPPCh")) {
        m_eProvisionType = PT_SPPCh;
	} else if (0 == strcmp(pProtectionType, "UNPROTECTED")) {
        m_eProvisionType = PT_UNPROTECTED;
	} else if (0 == strcmp(pProtectionType, "UNPROTECTED_GREEN")) {
        m_eProvisionType = PT_UNPROTECTED_GREEN;
	} else if (0 == strcmp(pProtectionType, "DEDICATED")) {
        m_eProvisionType = PT_PAL_DPP;
	} else if (0 == strcmp(pProtectionType, "SEG_SP_B_HOP")) {
        m_eProvisionType = PT_SEG_SP_B_HOP;
    } else if (0 == strcmp(pProtectionType, "SEG_SP_L_NO_HOP")) {
        m_eProvisionType = PT_SEG_SP_L_NO_HOP;
    } else if (0 == strcmp(pProtectionType, "SEG_SP_PB_HOP")) {
        m_eProvisionType = PT_SEG_SP_PB_HOP;
    } else if (0 == strcmp(pProtectionType, "SEG_SPP")) {
        m_eProvisionType = PT_SEG_SPP;
    } else if (0 == strcmp(pProtectionType, "SEG_SPP_B_HOP")) {
        m_eProvisionType = PT_SEG_SPP_B_HOP;
	} else if (0 == strcmp(pProtectionType, "SEG_SPP_PB_HOP")) {
		m_eProvisionType = PT_SEG_SPP_PB_HOP;
	} else if (0 == strcmp(pProtectionType, "SEG_SP_AGBSP")) {
		m_eProvisionType = PT_SEG_SP_AGBSP;
	} else if (0 == strcmp(pProtectionType, "BBU_HOTEL")) {
		m_eProvisionType = PT_BBU;
    } else {
        TRACE1("- ERROR: invalid protection type: %s\n", pProtectionType);
        return false;
    }

    if (nNumberOfPaths <= 0) {
        TRACE1("- ERROR: invalid #OfPaths: %d\n", nNumberOfPaths);
        return false;
    }
    m_nNumberOfPaths = nNumberOfPaths;
    m_hWDMNet.m_dEpsilon = dEpsilon;
    //m_hServWDMNet.m_dEpsilon = dEpsilon; //-M
    m_nTimePolicy = nTimePolicy; // M

    // if (m_nTimePolicy==1){
    //  cerr <<"The value of TimePolicy is" << m_nTimePolicy  <<endl;
    // }

    m_UnAv=false;
    if(0 == strcmp(UnAv, "A"))
		m_UnAv=true;
	// Added ANDREA
    m_eStatActive = PBLOCK;
    // generate initial state graph
    genInitialStateGraph();
	m_hWDMNetPast = m_hWDMNet;
    return true;
}

//-B: it should provide an array useful to deal with link/node disjointness
inline void NetMan::genInitialStateGraph()
{
  switch (m_eProvisionType) {
    case PT_SEG_SP_NO_HOP:
    case PT_SEG_SP_L_NO_HOP://-t
	case PT_SEG_SP_AGBSP://-t
    case PT_SEG_SP_B_HOP:
    case PT_SEG_SPP:
    case PT_SEG_SPP_B_HOP:
    case PT_SEG_SP_PB_HOP:
    case PT_SEG_SPP_PB_HOP:
	case PT_UNPROTECTED:
	case PT_PAL_DPP:
	case PT_wpUNPROTECTED:
	case PT_UNPROTECTED_GREEN:
	case PT_wpSEG_SPP:
	case PT_wpPAL_DPP:
	case PT_SPPBw:
	case PT_SPPCh:
	case PT_BBU:
        // single link/node failure
		if(isLinkDisjointActive)
			m_hWDMNet.allocateConflictSet(m_hWDMNet.getNumberOfLinks());
		else
			m_hWDMNet.allocateConflictSet(m_hWDMNet.getNumberOfNodes());
			//m_hServWDMNet.allocateConflictSet(m_hServWDMNet.getNumberOfNodes());
        break;
    default:
        DEFAULT_SWITCH;
  }
}

//-B: reset attributes/info about connections, nodes (also links???)
void NetMan::resetAll()
{
	//-B: delete/reset/release everything (?) about connections -> before resetting everything, it seems to update the stats of each lighpath
    deprovisionAll();
	//-B: reset everything (?) about all the nodes in the network
    m_hGraph.reset4ShortestPathComputation();
    SimplexLink *pSLink;
    list<AbstractLink*>::const_iterator itr;
	for (itr = m_hGraph.m_hLinkList.begin(); itr != m_hGraph.m_hLinkList.end(); itr++)
	{
        pSLink = (SimplexLink*)(*itr); //-B: SimplexLink class derived from AbstractLink class
        //-B: if SimplexLinkType of the pSLink object is set to LT_UniFiber, then set the cost atttribute equal to 1
		if (SimplexLink::LT_UniFiber == pSLink->getSimplexLinkType())
            pSLink->modifyCost(1);
    }
	//-B: why does it call again the same method???
    m_hWDMNet.reset4ShortestPathComputation();
}

//-B: delete/reset/release everything (?) about connections/lightpaths
void NetMan::deprovisionAll()
{
    cout<<m_hConnectionDB.m_hConList.size()<<" connection(s) remaining"<<endl;
    Connection *pCon;
    while (!m_hConnectionDB.m_hConList.m_hList.empty()) {
        pCon = m_hConnectionDB.m_hConList.m_hList.front();
        deprovisionConnection(pCon);
        delete pCon;
    }
}

//-B: it simply returns the num of OXC nodes in the network (stored in m_hNodeList)
UINT NetMan::getNumberOfOXCNodes() const
{
    return m_hWDMNet.getNumberOfNodes();
}

//-B: it simply returns the num of unidirectional fibers in the network (stored in m_hLinkList)
UINT NetMan::getNumberOfUniFibers() const
{
    return m_hWDMNet.getNumberOfLinks();
}

//-B: return a reference to the WDMNetwork object, attribute of NetMan object
WDMNetwork* NetMan::getWDMNetwork()
{
    return (&m_hWDMNet);
}

//-B: return true if the provisioning of the connection passed as parameter has been done correctly
//	it calls the provisionConnectionHelper method, which calls a different method CASENAME_Provision
//	depending on the provisioning type of the connection (these methods are implemented in this class, see below)
//	ATTENTION: not used in while cycle of run method (class Simulator) -> provisionConnectionxTime is used instead
bool NetMan::provisionConnection(Connection *pCon)
{
    bool bSuccess;
    try {
        bSuccess = provisionConnectionHelper(pCon);
    } catch (OchException) {
        cerr<<OchException::m_pErrorMsg<<endl;
        this->dump(cout);
        bSuccess = false;
    }
    return bSuccess;
}

//-B: return true if the provisioning of the connection passed as parameter has been done correctly
//	it calls the provisionConnectionHelper method, which calls a different method CASENAME_Provision
//	depending on the provisioning type of the connection (these methods are implemented in this class, see below)
//	also, set the departure event list equal to the dep event list passed as parameter (DepaPC)
//	(EventList evList is not used)
//	[in sostanza, rispetto a provisionConnection (vedi sopra) imposta i valori delle var m_HoldingTimeIncoming
//	e m_HArrivalTimeIncoming (appartenenti all'oggetto NetMan) uguali a quelli della connessione passata come parametro]
// -M List Event added
bool NetMan::provisionConnectionxTime(Connection *pCon, list<Event*>& DepaPC, EventList& evList)
{
// int a=0;
//   Event *pEvent;
//   list<Event*>::const_iterator itrE;
//      for (itrE=DepaPC.begin(); itrE!=DepaPC.end(); itrE++) {
//          pEvent = (Event*)(*itrE);
//          assert(pEvent);
//         cerr <<  pEvent-> m_hTime <<endl; 
//         a++;
//      }
//      cerr << "dimensione lista" << a << endl;
//      a=0;
    
	m_hDepaNet = DepaPC;
//     int a=0;
//     Event *pEvent;
//     list<Event*>::const_iterator itrE;
//     for (itrE=m_hDepaNet.begin(); itrE!=m_hDepaNet.end(); itrE++) {
//          pEvent = (Event*)(*itrE);
//          assert(pEvent);
//         cerr <<  pEvent-> m_hTime <<endl; 
//         a++;
//      }
//      cerr << "dimensione lista" << a << endl;
//      a=0;

	DepaPC.erase(DepaPC.begin(), DepaPC.end());
    //m_hDepaNet.erase(m_hDepaNet.begin(), m_hDepaNet.end());
	m_hHoldingTimeIncoming = pCon->m_dHoldingTime;
	m_hArrivalTimeIncoming = pCon->m_dArrivalTime;
    bool bSuccess;
    try {
        bSuccess = provisionConnectionHelper(pCon);
    } catch (OchException) {
        cerr<<OchException::m_pErrorMsg<<endl;
        this->dump(cout);
        bSuccess = false;
    }
    return bSuccess;
}

//-B: return true if the provisioning of the connection passed as parameter has been done correctly
//	call a different method CASENAME_Provision. Log the provisioning of the connection passed as parameter
//	(to count the provisioned/blocked connections) and add it to the Connection DB (a Connection list)
inline bool NetMan::provisionConnectionHelper(Connection *pCon)
{
  assert(pCon);
  bool bSuccess = false;

  switch (m_eProvisionType) {

	case PT_wpSEG_SPP:
        bSuccess = wpSEG_SPP_Provision(pCon);
        break;
	case PT_wpUNPROTECTED:
        bSuccess = wpUNPROTECTED_Provision(pCon);
        break;
	case PT_UNPROTECTED:
        bSuccess = UNPROTECTED_Provision(pCon);
        break;
	case PT_UNPROTECTED_GREEN:
        bSuccess = UNPROTECTED_ProvisionGreen(pCon);
        break;
	case PT_PAL_DPP: //fabio 8 dic..avrò fatto bene ad usare questo nome gia assegnato?
        bSuccess = PAL_DPP_Provision(pCon);
        break;
	case PT_wpPAL_DPP: //fabio 8 dic..avrò fatto bene ad usare questo nome gia assegnato?
        bSuccess = wpPAL_DPP_Provision(pCon);
        break;
	case PT_SPPBw: 
        bSuccess = SPPBw_Provision(pCon);
        break;
	case PT_SPPCh: 
        bSuccess = SPPCh_Provision(pCon);
        break;
    case PT_SEG_SP_NO_HOP:
        bSuccess = SEG_SP_NO_HOP_Provision(pCon);
        break;
    case PT_SEG_SP_L_NO_HOP:
        bSuccess = SEG_SP_L_NO_HOP_Provision(pCon);//-t
        break;
	case PT_SEG_SP_AGBSP:
        bSuccess = SEG_SP_AGBSP_Provision(pCon);//-t
        break;
    case PT_SEG_SPP:
        bSuccess = SEG_SPP_Provision(pCon);
        break;
    case PT_SEG_SP_B_HOP:
        bSuccess = SEG_SP_B_HOP_Provision(pCon);
        break;
    case PT_SEG_SPP_B_HOP:
        bSuccess = SEG_SPP_B_HOP_Provision(pCon);
        break;
    case PT_SEG_SP_PB_HOP:
        bSuccess = SEG_SP_PB_HOP_Provision(pCon);
        break;
    case PT_SEG_SPP_PB_HOP:
        bSuccess = SEG_SPP_PB_HOP_Provision(pCon);
        break;
	case PT_BBU:
		bSuccess = BBU_ProvisionNew(pCon); //-B: both VWP and WP cases
		break;
    default:
        DEFAULT_SWITCH;
  };

#ifdef DEBUGB
  UINT weird = 0;
  //-B: check if there is something strange
  weird = this->checkEmptyLightpaths();
  if (weird > 0)
  {
	  cout << "CI SONO " << weird << " LIGHTPATHS CON FREECAP = OCLIGHTPATH. WEIRD!" << endl;
	  cin.get();
	  this->m_hLightpathDB.dump(cout);
	  cin.get();
	  //this->removeEmptyLinkFromGraph();
	  //assert(this->checkEmptyLightpaths() == 0);
  }
#endif // DEBUGB



	//if (bSuccess)
	//{
		//this->checkGrooming(pCon);	/-B: NOT NEEDED ANYMORE
	//}

    // log this provisioning, increasing counter of accepted or blocked connections
    pCon->log(m_hLog);

	//*************************************************************
    pCon->log(m_runLog);	// Statistical Log Add by Andrea  *
    //*************************************************************

    // add to connection db if provisioned
    if (bSuccess)
        m_hConnectionDB.addConnection(pCon);

	return bSuccess;
}

//-B: delete/reset/release everything (?) about connections
//	depending on the connection provisioning type, it calls a different method CASENAME_Deprovision
void NetMan::deprovisionConnection(Connection* pCon)
{
	switch (m_eProvisionType)
	{
	case PT_wpSEG_SPP:
		wpSEG_SPP_Deprovision(pCon);
		break;
	case PT_wpUNPROTECTED:
		wpUNPROTECTED_Deprovision(pCon);
		break;
	case PT_SPPBw:
		SPPBw_Deprovision(pCon);
		break;
	case PT_SPPCh:
		SPPCh_Deprovision(pCon);
		break;
	case PT_wpPAL_DPP:
		wpPAL_DPP_deprovision(pCon);
		break;
	case PT_SEG_SP_NO_HOP:
		SEG_SP_NO_HOP_Deprovision(pCon);
		break;
	case PT_SEG_SP_L_NO_HOP:
		SEG_SP_L_NO_HOP_Deprovision(pCon);
		break;
	case PT_SEG_SP_AGBSP:
		SEG_SP_AGBSP_Deprovision(pCon);
		break;
	case PT_SEG_SPP:
		SEG_SPP_Deprovision(pCon);
		break;
	case PT_UNPROTECTED:
		UNPROTECTED_Deprovision(pCon);
		break;
	case PT_UNPROTECTED_GREEN:
		UNPROTECTED_Deprovision(pCon);
		break;
	case PT_PAL_DPP:
		PAL_DPP_Deprovision(pCon);
		break;
	case PT_SEG_SP_B_HOP:
		SEG_SP_B_HOP_Deprovision(pCon);
		break;
	case PT_SEG_SPP_B_HOP:
		SEG_SPP_B_HOP_Deprovision(pCon);
		break;
	case PT_SEG_SP_PB_HOP:
		SEG_SP_PB_HOP_Deprovision(pCon);
		break;
	case PT_SEG_SPP_PB_HOP:
		SEG_SPP_PB_HOP_Deprovision(pCon);
		break;
	case PT_BBU:
		BBU_Deprovision(pCon);
		break;
	default:
		DEFAULT_SWITCH;
	}
	pCon->m_eStatus = Connection::TORNDOWN;
	m_hConnectionDB.removeConnection(pCon);
}

//**************************** PAL_DPP ****************************
inline bool NetMan::PAL_DPP_Provision_OCLightpath(Connection *pCon)
{
    assert(OCLightpath == pCon->m_eBandwidth);

    // check if there are enough Tx (Rx) at Src (Dst)
    OXCNode *pOXCSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pOXCDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pOXCSrc && pOXCDst);
    if ((pOXCSrc->m_nFreeTx < m_nTxReqPerLightpath) ||
        (pOXCDst->m_nFreeRx < m_nTxReqPerLightpath)) {
        pCon->m_eStatus = Connection::DROPPED;
        pCon->m_bBlockedDueToUnreach = true;
        pCon->m_bBlockedDueToTx = true;
        return false;
    }

    Lightpath *pLP = new Lightpath(0, Lightpath::LPT_PAL_Dedicated);
    // apply Suurballe's algorithm to the reachability graph
    AbstractGraph hWLGraph;
    m_hWDMNet.genReachabilityGraphThruWL(hWLGraph);
    
    // NB: may need to set the shorter one as primary
    LINK_COST hCost = 
        hWLGraph.Suurballe(pLP->m_hPPhysicalPath, pLP->m_hBPhysicalPath,
                        pCon->m_nSrc, pCon->m_nDst, 
                        AbstractGraph::LCF_ByOriginalLinkCost);
    if (UNREACHABLE == hCost) {
        delete pLP;
        pCon->m_eStatus = Connection::DROPPED;
        pCon->m_bBlockedDueToUnreach = true;
        return false;
    }

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_PAL_DPP, NULL);
    pLP->m_nBWToBeAllocated = pCon->m_eBandwidth;
    pCircuit->addVirtualLink(pLP);
    pCircuit->setUp(this);

    pCon->m_pPCircuit = pCircuit;
    pCon->m_eStatus = Connection::SETUP;

    return true;
}


bool NetMan::PAL_DPP_MinTHP_Provision(Connection *pCon)
{
// NB: wrong, should provision OCLightpath the same way as others
//  if (OCLightpath == pCon->m_eBandwidth)
//      return PAL_DPP_Provision_OCLightpath(pCon);

    AbstractGraph hAuxGraph;
    vector<Lightpath*> hLPList;
    list<AbstractLink*> hMinCostPath;

    // STEP 1: Generate auxilary graph
    try {
        PAL_DPP_MinTHP_GenAuxGraph(hAuxGraph, hLPList, pCon);
    } catch (...) {
        cerr<<"- OUT OF MEMORY: "<<__FILE__<<":"<<__LINE__<<endl;
    }

    // STEP 2: Apply min-cost path computation
    AbstractNode *pSrc = hAuxGraph.lookUpNodeById(pCon->m_nSrc);
    AbstractNode *pDst = hAuxGraph.lookUpNodeById(pCon->m_nDst);
    LINK_COST hMinCost = hAuxGraph.Dijkstra(hMinCostPath, pSrc, pDst,
                    AbstractGraph::LCF_ByOriginalLinkCost);

    if (UNREACHABLE == hMinCost) {
        pCon->m_eStatus = Connection::DROPPED;
        pCon->m_bBlockedDueToUnreach = true;

        // delete temp lightpaths
        vector<Lightpath*>::const_iterator itr;
        for (itr=hLPList.begin(); itr!=hLPList.end(); itr++) {
            if (0 == (*itr)->getId()) delete (*itr);
        }
        return false;
    }
    
    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_PAL_DPP, NULL);
    // STEP 3: Check min-cost path
    bool bRcAvailable;
    try {
        bRcAvailable = PAL_DPP_MinTHP_NewCircuit(*pCircuit, hAuxGraph, 
                            hLPList, hMinCostPath);
    } catch (...) {
        cerr<<"- OUT OF MEMORY: "<<__FILE__<<":"<<__LINE__<<endl;
    }

    // STEP 4: If okay then set up this new circuit
    if (bRcAvailable) {
        try {
            pCircuit->setUp(this);
        } catch (...) {
            cerr<<"- OUT OF MEMORY: "<<__FILE__<<":"<<__LINE__<<endl;
        }

        // STEP 5: Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // STEP 6: Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        pCon->m_eStatus = Connection::DROPPED;
        delete pCircuit;
    }

    // delete temp lightpaths
    vector<Lightpath*>::const_iterator itr;
    for (itr=hLPList.begin(); itr!=hLPList.end(); itr++) {
        if (0 == (*itr)->getId()) delete (*itr);
    }

    return bRcAvailable;
}

inline bool NetMan::PAL_DPP_MinTHP_NewCircuit(Circuit& hCircuit, 
                                   const AbstractGraph& hAuxGraph, 
                                   const vector<Lightpath*>& hLPList, 
                                   const list<AbstractLink*>& hMinCostPath)
{
    UINT nUniFibers = m_hWDMNet.getNumberOfLinks();
    int *pChannelReqOnLink = new int[nUniFibers];
    memset(pChannelReqOnLink, 0, sizeof(int)*nUniFibers);

    AbstractLink *pAuxLink;
    Lightpath *pLP;
    list<AbstractLink*>::const_iterator itr;
    for (itr=hMinCostPath.begin(); itr!=hMinCostPath.end(); itr++) {
        pAuxLink = *itr;
        assert(pAuxLink);
        // link ID is the index of that lightpath in hLPList
        pLP = hLPList[pAuxLink->getId()];
        if (0 == pLP->getId()) {    // new lightpath, check rc availability
            PAL_DPP_MinTHP_NewCircuit_Helper(pChannelReqOnLink,
                                        pLP->m_hPPhysicalPath);
            PAL_DPP_MinTHP_NewCircuit_Helper(pChannelReqOnLink,
                                        pLP->m_hBPhysicalPath);
        }
        pLP->m_nBWToBeAllocated = hCircuit.m_eBW;
        hCircuit.addVirtualLink(pLP);
    }

    // resource available?
    bool bRcAvailable = true;
    list<AbstractLink*>::const_iterator itrUniFiber;
    UniFiber *pUniFiber;
    for (itrUniFiber=m_hWDMNet.m_hLinkList.begin(); 
    itrUniFiber!=m_hWDMNet.m_hLinkList.end(); itrUniFiber++) {
        pUniFiber = (UniFiber*)(*itrUniFiber);
        assert(pUniFiber);
        if (pUniFiber->countFreeChannels() < 
            pChannelReqOnLink[pUniFiber->getId()]) {
            bRcAvailable = false;   // not enough available wavelengths
            break;
        }
    }

    delete []pChannelReqOnLink;
    return bRcAvailable;
}

inline void NetMan::PAL_DPP_MinTHP_NewCircuit_Helper(int* pChannelReqOnLink, 
                                                     const AbstractPath& hPath)
{
    list<UINT>::const_iterator itrNext1 = hPath.m_hLinkList.begin();
    assert(itrNext1 != hPath.m_hLinkList.end());
    list<UINT>::const_iterator itrCurr1 = itrNext1++;
    AbstractLink *pLink;
    while (itrNext1 != hPath.m_hLinkList.end()) {
        pLink = m_hWDMNet.lookUpLink(*itrCurr1, *itrNext1);
        assert(pLink);
        pChannelReqOnLink[pLink->getId()]++;
        itrCurr1 = itrNext1++;
    }
}


inline void NetMan::PAL_DPP_MinTHP_GenAuxGraph(AbstractGraph& hAuxGraph,
                                               vector<Lightpath*>& hLPList,
                                               const Connection* pCon)
{
    list<AbstractNode*>::const_iterator itrSrc;
    list<AbstractNode*>::const_iterator itrDst;

    hAuxGraph.deleteContent();
    for (itrSrc=m_hWDMNet.m_hNodeList.begin(); 
    itrSrc!=m_hWDMNet.m_hNodeList.end(); itrSrc++)
        hAuxGraph.addNode(new AbstractNode((*itrSrc)->getId()));

    AbstractGraph hWLGraph;
    m_hWDMNet.genReachabilityGraphThruWL(hWLGraph);

    // link ID is the index of that lightpath in hLPList
    UINT nLinkId = 0;
    for (itrSrc = m_hWDMNet.m_hNodeList.begin(); itrSrc != m_hWDMNet.m_hNodeList.end(); itrSrc++)
        for (itrDst = m_hWDMNet.m_hNodeList.begin(); itrDst!=m_hWDMNet.m_hNodeList.end(); itrDst++)
		{
            if (itrSrc == itrDst)
				continue;

            // search for existing lightpath w/ min # of WLs
            Lightpath *pTargetLP = NULL;
            if (pCon->m_eBandwidth < OCLightpath) {
                pTargetLP = lookUpLightpathOfMinChannels((*itrSrc)->getId(), 
                                (*itrDst)->getId(), pCon->m_eBandwidth);
            }

            LINK_COST hNewCost = UNREACHABLE;
            Lightpath *pNewLP = NULL;
            if ((NULL == pTargetLP) || (GP_MinTHP == m_eGroomingPolicy)) {
                // constraints on Tx & Rx
                OXCNode *pOXCSrc = (OXCNode*)(*itrSrc);
                OXCNode *pOXCDst = (OXCNode*)(*itrDst);
                assert(pOXCSrc && pOXCDst);
                if ((pOXCSrc->m_nFreeTx >= m_nTxReqPerLightpath) 
                    && (pOXCDst->m_nFreeRx >= m_nTxReqPerLightpath)) {
                    // apply Suurballe's algorithm for each s-d pair on hWLGraph
                    pNewLP = new Lightpath(0, Lightpath::LPT_PAL_Dedicated);
                    // NB: may need to set the shorter one as primary
                    hNewCost = hWLGraph.Suurballe(pNewLP->m_hPPhysicalPath, 
                                    pNewLP->m_hBPhysicalPath, 
                                    (*itrSrc)->getId(), 
                                    (*itrDst)->getId(), 
                                    AbstractGraph::LCF_ByOriginalLinkCost);
                }
            }
           bool bUseExistingLP = false;
            bool bUseNewLP = false;
			//m_eGroomingPolicy=GP_MinTHP;//levare
            switch (m_eGroomingPolicy) {
            case GP_MinNTHP:
                if (pTargetLP) bUseExistingLP = true;
                else if (UNREACHABLE != hNewCost)
                    bUseNewLP = true;
                break;
            case GP_MinTHP:
                if (pTargetLP) {    
                    if (UNREACHABLE != hNewCost) {
                        if (pTargetLP->getCost() <= hNewCost)
                            bUseExistingLP = true;  // use existing LP
                        else
                            bUseNewLP = true;       // use new LP
                    } else {
                        bUseExistingLP = true;      // use existing LP
                    }
                } else {    // no existing LP
                    if (UNREACHABLE != hNewCost)
                        bUseNewLP = true;           // use new LP
                }
                break;
            default:
                DEFAULT_SWITCH;
            }

            assert(!(bUseExistingLP && bUseNewLP));
            if (bUseExistingLP) {
                assert(pTargetLP->getCost() >= 0);
                // to encourage grooming, use hUseCost instead of hRealCost
                LINK_COST hRealCost = pTargetLP->getCost();
                LINK_COST hUseCost = hRealCost - 1; 
                hAuxGraph.addLink(nLinkId++, (*itrSrc)->getId(), 
                    (*itrDst)->getId(), hUseCost, 1);
                // use existing lightpath
                hLPList.push_back(pTargetLP);
                if (pNewLP) delete pNewLP;
            } else if (bUseNewLP) {
                assert(hNewCost >= 0);
                hAuxGraph.addLink(nLinkId++, (*itrSrc)->getId(), 
                    (*itrDst)->getId(), hNewCost, 1);
                // use new lightpath
                hLPList.push_back(pNewLP);
            } else {
                // no existing/new lightpath connect these two nodes
                if (pNewLP) delete pNewLP;
            }
        }
}

inline void NetMan::PAL_DPP_MinTHP_Deprovision(Connection* pCon)
{
    if (pCon->m_pPCircuit)
        pCon->m_pPCircuit->tearDown(this);
    if (pCon->m_pBCircuit)
        pCon->m_pBCircuit->tearDown(this);
}

//-B: Log some info about lightpaths, count num of backup channels and other logs
void NetMan::logPeriodical(SimulationTime hTimeSpan)
{
	//-B: log peak and average network cost
	this->logNetCostPeriodical(hTimeSpan);

	//-B: log peak and average number of lightpaths
	this->logActiveLpPeriodical(hTimeSpan);

	//-B: log peak and average number of active hotels/nodes
	this->logActiveHotelsPeriodical(hTimeSpan);

	//-B: log peak and average number of BBUs (count only macro cells)
	this->logActiveBBUs(hTimeSpan);

	//-B: log peak and average number of active small cells
	this->logActiveSmallCells(hTimeSpan);
	//////////////////////////////////////////////////////////////////////////////////

	//-B: log time of activation of hotel nodes
	this->m_hWDMNet.logHotelsActivation(hTimeSpan);

	//-B: for each node, log which hotel host its BBU
	this->m_hWDMNet.logBBUInHotel();

	//-B: log fiber load
	this->m_hWDMNet.logPeriodical(m_hLog, hTimeSpan);
	//per nRunConnections connections
	//this->m_hWDMNet.logPeriodical(m_runLog, hTimeSpan);

	 //-B: log life time e busy time of each lightpath
	m_hLightpathDB.logPeriodical(m_hLog, hTimeSpan);
	//per nRunConnections
	//m_hLightpathDB.logPeriodical(m_runLog, hTimeSpan);

	//-B: route-computation-effectiveness ratio
	UINT backBwd = 0;

	//------------ TO COMPUTE SUMCARRIEDLOAD, USE OPTION 1 IF EACH CONNECTION REQUIRE THE SAME AMOUNT OF BWD, OTHERWISE USE OPTION 2 ----------------
	//-B: ORIGINAL OPTION
	//m_hLog.m_dSumCarriedLoad += m_hConnectionDB.m_hConList.size() * hTimeSpan; //pare che venga incrementato solo qui, quindi nel logPeriodical fatto ad ogni iterazione
	
	//-B: OPTION 1
	//viene anche chiamato route-computation-effectiveness-ratio -> ???
	//Carried load already there
	//m_hLog.m_dSumCarriedLoad += m_hConnectionDB.countBackConn(backBwd) * hTimeSpan;

	double totalResources = CHANNEL_CAPACITY * getNumberOfUniFibers() * (this->m_hWDMNet.numberOfChannels);
	double resourcesUsed = m_hConnectionDB.countBackConn(backBwd)*6;
	float fractionRes = resourcesUsed / totalResources;
	m_hLog.m_dSumCarriedLoadMonica += fractionRes * hTimeSpan;

	//-B: OPTION 2
	//-B: return the number of backhaul connections and modify the input parameter backBwd computing the total carried bh bwd
	//m_hConnectionDB.countBackConn(backBwd);
	//m_hLog.m_dSumCarriedLoad += backBwd * hTimeSpan;

	switch (m_eProvisionType) {
    case PT_SEG_SP_NO_HOP:
    case PT_SEG_SP_L_NO_HOP://-t
	case PT_SEG_SP_AGBSP://-t
    case PT_SEG_SP_B_HOP:
    case PT_SEG_SPP:
    case PT_SEG_SPP_B_HOP:
    case PT_SEG_SP_PB_HOP:
    case PT_SEG_SPP_PB_HOP:
	case PT_UNPROTECTED:
	case PT_UNPROTECTED_GREEN:	
	case PT_wpSEG_SPP:
	case PT_wpUNPROTECTED:
	case PT_SPPBw:
	case PT_SPPCh:
	{
		//-B: count num of backup channels
		UINT nBackupChannels = 0;
        list<AbstractLink*>::const_iterator itr;
		for (itr = m_hWDMNet.m_hLinkList.begin(); itr != m_hWDMNet.m_hLinkList.end(); itr++) {
			nBackupChannels += (*itr)->m_nBChannels;
        }
        m_hLog.m_dSumLinkLoad += hTimeSpan * (double)nBackupChannels;
		//	cout<<"BackupChannels= "<< nBackupChannels<<endl;
	}
	break;
	case PT_BBU:
	case PT_PAL_DPP:
	case PT_wpPAL_DPP:
		break;
    default:
        DEFAULT_SWITCH;
  }
}

UINT NetMan::computeNetworkCost()
{
	/*
	UINT numActiveHotels = this->m_hWDMNet.BBUs.size();
	UINT n = 0;
	vector<OXCNode*>::iterator itr;

	for (itr = this->m_hWDMNet.hotelsList.begin(); itr != this->m_hWDMNet.hotelsList.end(); itr++)
	{
		if ((*itr)->m_nBBUs > 0)
			n++;
	}
	if (n != numActiveHotels)
	{
		cout << "ATTENZIONE! n = " << n << " ; numActiveHotels = " << numActiveHotels << endl;
		this->m_hWDMNet.printBBUs();
		this->m_hWDMNet.printHotelNodes();
		for (itr = this->m_hWDMNet.hotelsList.begin(); itr != this->m_hWDMNet.hotelsList.end(); itr++)
		{
			if ((*itr)->m_nBBUs > 0)
				cout << "Il nodo " << (*itr)->getId() << " pare avere " << (*itr)->m_nBBUs << " attive" << endl;
		}
		cin.get();
	}*/

	UINT numActiveHotels = 0;

	if (BBUSTACKING == true && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == false)
	{
		numActiveHotels = this->m_hWDMNet.countActiveNodes();
	}
	else if (BBUSTACKING == false && INTRA_BBUPOOLING == true && INTER_BBUPOOLING == false)
	{
		; //to do
	}
	else if (BBUSTACKING == false && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == true)
	{
		numActiveHotels = this->m_hWDMNet.countActivePools(this->getConnectionDB());
	}
	else
	{
		assert(false);
	}

	return (100 * numActiveHotels) + (1 * this->getLightpathDB().m_hLightpathList.size());
}

//-B: it calls different log methods -> see these method's description
void NetMan::logFinal()
{
    m_hConnectionDB.logFinal(m_hLog); //-B: nothing to do
    m_hLightpathDB.logFinal(m_hLog); //-B: for each lightpath, it logs: primary/backup hop distance
									//	holding time, lightpath load, link load, tx load
	//*************************************************************************************
    //-B: see above, but the Log object passed as parameter is for running stats (durante l'esecuzione della simulazione)
	m_hConnectionDB.logFinal(m_runLog);			// Statistical Log Add by Andrea  *
    m_hLightpathDB.logFinal(m_runLog);			// Statistical Log Add by Andrea  *
    //*************************************************************************************
}

//-B: print in ostream object passed as parameter some stats logged previously:
//	num of provisioned/blocked connections, primary/backup distance, link load, num of conflicts
void NetMan::showLog(ostream &out) const
{
    m_hLog.output(out);
	cout<<"\nConflitti primario: "<< pcPer;
	cout<<"\nConflitti backup: "<<bcPer;
	cout<<"\nConflitti Prim & Bak: "<<bothcPer;
	cout<<"\nFalsi conflitti: "<<fcPer;
}

//-B: return the searched lightpath (source and destination) having the minimum amount of hops
//	but having an amount of bandw >= than the required amount of bandw
//	(a parità di num di hops, prende quello con minore capacità libera)
Lightpath* NetMan::lookUpLightpathOfMinChannels(UINT nSrc, UINT nDst, UINT nBW) const
{
    // search for existing lightpath w/ min # of WLs
    Lightpath *pCandidateLP = NULL;
    UINT nMinPhysicalHops = INT_MAX;
    list<Lightpath*>::const_iterator itrLP;
	for (itrLP = m_hLightpathDB.m_hLightpathList.begin(); itrLP != m_hLightpathDB.m_hLightpathList.end(); itrLP++) {
        //-B: look for the lightpath 
		if (((*itrLP)->getSrc()->getId() != nSrc) || ((*itrLP)->getDst()->getId() != nDst))
            continue; //-B: go to the next iteration of for loop
        if ((*itrLP)->getFreeCapacity() < nBW)
            continue; //-B: go to the next iteration of for loop
        //-B: if it is the lightpath we are looking for (source, destination and required amount of banndwidth)
		UINT nPhysicalHops = (*itrLP)->getPhysicalHops(); //-B: return the total num of hops over both primary and backup lightpaths
        if (nPhysicalHops < nMinPhysicalHops) {
            nMinPhysicalHops = nPhysicalHops;
            pCandidateLP = *itrLP;
        } else if (nPhysicalHops == nMinPhysicalHops) {
            if ((*itrLP)->getFreeCapacity() < pCandidateLP->getFreeCapacity())
                pCandidateLP = *itrLP;
        }
    }
    return pCandidateLP;
}


void NetMan::setTxThreshold(double dTxThreshold)
{
    m_dTxThreshold = dTxThreshold;
}

//**************************** PAC_DPP ****************************
inline void NetMan:: PAC_DPP_Provision_OCLightpath_Helper(list<AbstractLink*>& hPath, 
                                                         const AbstractPath& hAbsPath)
{
    SimplexLink *pLink;
    list<UINT>::const_iterator itrNext = hAbsPath.m_hLinkList.begin();
    list<UINT>::const_iterator itrCurr = itrNext++;
    while (itrNext != hAbsPath.m_hLinkList.end())
	{
        pLink = m_hGraph.lookUpSimplexLink(*itrCurr, Vertex::VT_Channel_Out, -1, *itrNext, Vertex::VT_Channel_In, -1);
        assert(pLink);
        hPath.push_back(pLink);
        itrCurr = itrNext++;
    }
}

inline bool NetMan::PAC_DPP_Provision_OCLightpath(Connection *pCon)
{
    assert(OCLightpath == pCon->m_eBandwidth);

    // check if there are enough Tx (Rx) at Src (Dst)
    OXCNode *pOXCSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pOXCDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pOXCSrc && pOXCDst);
    if ((pOXCSrc->m_nFreeTx < 2 * m_nTxReqPerLightpath) ||
        (pOXCDst->m_nFreeRx < 2 * m_nTxReqPerLightpath)) {
        pCon->m_eStatus = Connection::DROPPED;
        pCon->m_bBlockedDueToUnreach = true;
        return false;
    }

    // apply Suurballe's algorithm to the reachability graph
    AbstractGraph hWLGraph;
    m_hWDMNet.genReachabilityGraphThruWL(hWLGraph);
    
    AbstractPath hPath1, hPath2;
    LINK_COST hCost = hWLGraph.Suurballe(hPath1, hPath2, 
                        pCon->m_nSrc, pCon->m_nDst, 
                        AbstractGraph::LCF_ByOriginalLinkCost);
    if (UNREACHABLE == hCost) {
        pCon->m_eStatus = Connection::DROPPED;
        pCon->m_bBlockedDueToUnreach = true;
        return false;
    }

    // convert to paths in the state graph to feed to PAC_DPP_NewCircuit
    list<AbstractLink*> hPPath, hBPath;
    if (hPath1.getPhysicalHops() > hPath2.getPhysicalHops()) {
        PAC_DPP_Provision_OCLightpath_Helper(hPPath, hPath2);
        PAC_DPP_Provision_OCLightpath_Helper(hBPath, hPath1);
    } else {
        PAC_DPP_Provision_OCLightpath_Helper(hPPath, hPath1);
        PAC_DPP_Provision_OCLightpath_Helper(hBPath, hPath2);
    }

    // create/setup primary & backup circuits
    Circuit *pPCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_PAC_DPP_Primary, NULL);
    Circuit *pBCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_PAC_DPP_Backup, pPCircuit);
    pPCircuit->m_pCircuit = pBCircuit;

    PAC_DPP_NewCircuit(*pPCircuit, hPPath);
    PAC_DPP_NewCircuit(*pBCircuit, hBPath);
    pPCircuit->setUp(this);
    pBCircuit->setUp(this);

    pCon->m_pPCircuit = pPCircuit;
    pCon->m_pBCircuit = pBCircuit;
    pCon->m_eStatus = Connection::SETUP;

    return true;
}

bool NetMan::PAC_DPP_Provision(Connection *pCon)
{
    // Provision OC-192 connections separately
    if (OCLightpath == pCon->m_eBandwidth)
        return PAC_DPP_Provision_OCLightpath(pCon);

	//-B: else if pCon->m_eBandwidth != OCLightpath provide connection in a different way
    // STEP 1: Compute primary path
    Vertex *pSrc = m_hGraph.lookUpVertex(pCon->m_nSrc, Vertex::VT_Access_Out, -1);
    Vertex *pDst = m_hGraph.lookUpVertex(pCon->m_nDst, Vertex::VT_Access_In, -1);
    assert(pSrc && pDst);

    Circuit *pPCircuit = new Circuit(pCon->m_eBandwidth, Circuit::CT_PAC_DPP_Primary, NULL);
    Circuit *pBCircuit = new Circuit(pCon->m_eBandwidth, Circuit::CT_PAC_DPP_Backup, pPCircuit);
    pPCircuit->m_pCircuit = pBCircuit;

    bool bSuccess = PAC_DPP_ProvisionHelper(pCon, *pPCircuit, *pBCircuit, true, pSrc, pDst);
    if (bSuccess) {
        // STEP 5: Associate the circuit w/ the connection
        pCon->m_pPCircuit = pPCircuit;
        pCon->m_pBCircuit = pBCircuit;

        // STEP 6: Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pPCircuit;
        delete pBCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hGraph.validateAllLinks();
    return bSuccess;
}

inline bool NetMan::PAC_DPP_ProvisionHelper(Connection *pCon, Circuit& hPCircuit, 
                                 Circuit& hBCircuit, bool bCutSet,
                                 Vertex *pSrc, Vertex *pDst)
{
    // Preprocess: Invalidate those links that don't have enough capacity
    m_hGraph.invalidateSimplexLinkDueToCap(pCon->m_eBandwidth);

    list<AbstractLink*> hPrimaryPath, hBackupPath;
    LINK_COST hPrimaryCost, hBackupCost;
    hPrimaryCost = m_hGraph.Dijkstra(hPrimaryPath, pSrc, pDst,
                        AbstractGraph::LCF_ByOriginalLinkCost);

    if (UNREACHABLE == hPrimaryCost) {
        assert(bCutSet);
        pCon->m_bBlockedDueToUnreach = true;
        return false;
    }
    // NB: set up the primary circuit because the computation of a 
    // link-disjoint path depends on the UPDATED (w.r.t the primary circuit)
    // network state
    PAC_DPP_NewCircuit(hPCircuit, hPrimaryPath);

// 11/05/2002
    // Tx/Rx links may be deleted in Circuit::setUp
    // use hPSafePath after invokation of Circuit::setUp
    list<AbstractLink*> hPSafePath;
    {
        list<AbstractLink*>::const_iterator itr = hPrimaryPath.begin();
        while (itr != hPrimaryPath.end()) {
            SimplexLink *pLink = (SimplexLink*)(*itr);
            assert(pLink);
            switch (pLink->m_eSimplexLinkType) {
            case SimplexLink::LT_Channel:
            case SimplexLink::LT_UniFiber:
            case SimplexLink::LT_Lightpath:
                hPSafePath.push_back(pLink);
                break;
            case SimplexLink::LT_Channel_Bypass:
            case SimplexLink::LT_Grooming:
            case SimplexLink::LT_Mux:
            case SimplexLink::LT_Demux:
            case SimplexLink::LT_Tx:
            case SimplexLink::LT_Rx:
            case SimplexLink::LT_Converter:
                NULL;   // No need to do anything for these cases
                break;
            default:
                DEFAULT_SWITCH;
            }
            if (itr != hPrimaryPath.end()) itr++;
        }
    }

    hPCircuit.setUp(this);

    // STEP 2: Compute a link-disjoint shortest path as backup
//  PAC_InvalidatePath(m_hGraph, hPrimaryPath, pSrc, pDst);
    PAC_InvalidatePath(m_hGraph, hPSafePath);

    hBackupCost = m_hGraph.Dijkstra(hBackupPath, pSrc, pDst, 
                        AbstractGraph::LCF_ByOriginalLinkCost);
    if (UNREACHABLE == hBackupCost)
	{   
        hPCircuit.tearDown(this, false);    // do NOT log
        bool bSuccess = false;

        if (bCutSet) {
            // STEP 3: apply cut-set if STEP 2 fails
//          if (PAC_DPP_CutSet_HandleBackHaulLink(hPrimaryPath)) {
            if (PAC_DPP_CutSet_HandleBackHaulLink(hPSafePath)) {
                m_hGraph.validateAllLinks();
                bSuccess = PAC_DPP_ProvisionHelper(pCon, hPCircuit, hBCircuit,
                                    false, pSrc, pDst);
                // restore link cost
                m_hGraph.restoreLinkCost();
            } else {
                // No backhaul link, there is NO link-disjoint paths
                pCon->m_bBlockedDueToUnreach = true;
            }
        }
        return bSuccess;
    }

    // STEP 4: set up circuits when primary & backup are all set
    PAC_DPP_NewCircuit(hBCircuit, hBackupPath);
    hBCircuit.setUp(this);

    return true;
}

// return true if there is backhaul link; false otherwise
// increase the cost of backhaul links if any
bool NetMan::PAC_DPP_CutSet_HandleBackHaulLink(const list<AbstractLink*>& hPath)
{
    map<UINT, OXCNode*> hCutSetSrc;
    m_hGraph.computeCutSet(hCutSetSrc);

    bool bBackHaulLinkExists = false;
    bool bBackHaul;
    SimplexLink *pSimplexLink;
    list<AbstractLink*>::const_iterator itr;
    for (itr=hPath.begin(); itr!=hPath.end(); itr++) {
        pSimplexLink = (SimplexLink*)(*itr);
        assert(pSimplexLink);

        switch (pSimplexLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
        case SimplexLink::LT_UniFiber:
            assert(pSimplexLink->m_pUniFiber);
            bBackHaul = PAC_DPP_CutSet_HandleBackHaulPhyicalLink(hCutSetSrc,
                            pSimplexLink->m_pUniFiber);
            if (bBackHaul) {
                // increase the cost of ALL, not only this one, the links
                // that are in the same SRG as this one
                m_hGraph.modifyBackHaulLinkCost(pSimplexLink->m_pUniFiber,
                            LARGE_COST);
                bBackHaulLinkExists = true;
            }
            break;
        case SimplexLink::LT_Lightpath:
            {
                assert(pSimplexLink->m_pLightpath);
                bBackHaul = false;
                list<UniFiber*>::const_iterator itr;
                for (itr=pSimplexLink->m_pLightpath->m_hPRoute.begin();
                itr!=pSimplexLink->m_pLightpath->m_hPRoute.end(); itr++) {
// 11/04/2002 for debug
                    {
                        if (NULL == *itr) {
                            cout<<endl<<"- Troublesome lightpath:"<<endl;
                            pSimplexLink->m_pLightpath->dump(cout);
                            cout<<endl<<"- Troublesome link:"<<endl;
                            pSimplexLink->dump(cout);
                            cout<<endl;
                            this->dump(cout);
                        }
                    }
                    assert(*itr);
                    bBackHaul = 
                        PAC_DPP_CutSet_HandleBackHaulPhyicalLink(hCutSetSrc, *itr);
                    // increase the cost of ALL, not only this one, the links
                    // that are in the same SRG as this one
                    if (bBackHaul) {
                        m_hGraph.modifyBackHaulLinkCost(*itr, LARGE_COST);
                        bBackHaulLinkExists = true;
                    }
                }
            }
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter: 
            NULL;   // No need to do anyhting
            break;
        default:
            DEFAULT_SWITCH;
        }
    }

    return bBackHaulLinkExists;
}

inline bool NetMan::PAC_DPP_CutSet_HandleBackHaulPhyicalLink(
                            const map<UINT, OXCNode*>& hCutSetSrc,
                            const UniFiber *pUniFiber) const
{
    assert(pUniFiber);
    UINT nSrcOXC = pUniFiber->getSrc()->getId();
    UINT nDstOXC = pUniFiber->getDst()->getId();
    bool bSrcInCutSet = (hCutSetSrc.find(nSrcOXC) != hCutSetSrc.end());
    bool bDstInCutSet = (hCutSetSrc.find(nDstOXC) != hCutSetSrc.end());
    if (!bSrcInCutSet && bDstInCutSet)
        return true;
    return false;
}

// Construct a circuit and consume bandwidth along hLinkList 
void NetMan::PAC_DPP_NewCircuit(Circuit& hCircuit, 
                                const list<AbstractLink*>& hLinkList)
{
    list<AbstractLink*>::const_iterator itr = hLinkList.begin();
    while (itr != hLinkList.end())
	{
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
        case SimplexLink::LT_UniFiber:
            {
            // 0 indicates it's a new lightpath to be set up
            Lightpath *pLightpath = new Lightpath(0, Lightpath::LPT_PAC_DedicatedPath);
            pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            while (itr != hLinkList.end())
			{
                pLink = (SimplexLink*)(*itr);
                assert(pLink);
                SimplexLink::SimplexLinkType eSLType = pLink->m_eSimplexLinkType;
                if ((SimplexLink::LT_Channel == eSLType) || (SimplexLink::LT_UniFiber == eSLType))
				{
                    // consume bandwidth along this physical link
                    pLink->consumeBandwidth(OCLightpath);
                    assert(pLink->m_pUniFiber);
                    // construct lightpath route
                    pLightpath->m_hPRoute.push_back(pLink->m_pUniFiber);
                    // if it's OCLightpath, then there is no need to break
                    if (OCLightpath > hCircuit.m_eBW) {
                        OXCNode *pNode = (OXCNode*)pLink->m_pUniFiber->getDst();
                        assert(pNode);
                        if ((pNode->m_nFreeTx > 0) && (pNode->m_nFreeRx > 0) &&
                            ((pNode->m_nFreeTx - 1) >= m_dTxThreshold * pNode->getNumberOfTx()) &&
                            ((pNode->m_nFreeRx - 1) >= m_dTxThreshold * pNode->getNumberOfRx())) {
                            break;  // Sufficient # of Tx & Rx, break the lightpath
                        }
                    }
                } else if (SimplexLink::LT_Lightpath == eSLType) {
                    itr--;
                    break;
                }
                itr++;
            }
            // appendLightpath will add to the state graph a link corresponding
            // to this lightpath
            hCircuit.addVirtualLink(pLightpath);
            }
            break;
        case SimplexLink::LT_Lightpath:
            assert(pLink->m_pLightpath);
            pLink->m_pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            hCircuit.addVirtualLink(pLink->m_pLightpath);
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter:
            NULL;   // No need to do anything for these cases
            break;
        default:
            DEFAULT_SWITCH;
        }
        if (itr != hLinkList.end()) 
			itr++;
    }
}

void NetMan::PAC_InvalidatePath(Graph& hGraph, list<AbstractLink*>& hPath)
{
    list<AbstractLink*>::const_iterator itr;
    for (itr=hPath.begin(); itr!=hPath.end(); itr++) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
        case SimplexLink::LT_UniFiber:
            assert(pLink->m_pUniFiber);
            // invalidate this link and lightpaths that traverses this link
            hGraph.invalidateSimplexLinkDueToSRG(pLink->m_pUniFiber);
            break;
        case SimplexLink::LT_Lightpath:
            {
                assert(pLink->m_pLightpath);
                // do the above cases for each UniFiber the lightpath traverses
                list<UniFiber*>::const_iterator itr;
                for (itr=pLink->m_pLightpath->m_hPRoute.begin(); 
                itr!=pLink->m_pLightpath->m_hPRoute.end(); itr++)
                    hGraph.invalidateSimplexLinkDueToSRG(*itr);
            }
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter:
            NULL;   // Nothing to do so far
            break;
        default:
#ifdef _OCHDEBUG8
            (*itr)->dump(cout);
#endif
            DEFAULT_SWITCH;
        }
    }
}

inline void NetMan::PAC_DPP_Deprovision(Connection* pCon)
{
	PAL_DPP_MinTHP_Deprovision(pCon);
}

//-B: add a lightpath to the lightpaths' list/db (eBW unused)
void NetMan::appendLightpath(Lightpath *pLightpath, BandwidthGranularity eBW)
{
    m_hLightpathDB.appendLightpath(pLightpath);
}

//-B: remove a lightpath from the lightpaths' list/db
void NetMan::removeLightpath(Lightpath *pLightpath)
{
#ifdef DEBUGB
	cout << "-> removeLightpath" << endl;
#endif // DEBUGB
    m_hLightpathDB.removeLightpath(pLightpath);
}

//-B: don't know what it does
//	so far, for case PT_BBU: nothing to do
void NetMan::consumeTx(OXCNode *pOXC, int nTx)
{
    assert(pOXC);
    int nNewFreeTx = (int)pOXC->m_nFreeTx + nTx;
	assert(0 <= nNewFreeTx);
	assert(nNewFreeTx <= pOXC->getNumberOfTx());
    SimplexLink *pTxSLink;
    switch (m_eProvisionType) {
		case PT_BBU:
		case PT_PAL_DPP:    // No need to do anything
			NULL;   
			break;
		case PT_PAC_DPP:    // update state graph when necessary
		case PT_PAL_SPP:
		case PT_PAC_SPP:
		case PT_SPAC_SPP:
		// Original
		//      if (0 == pOXC->m_nFreeTx) { // add to state graph TxE
		//          assert(nTx > 0);
		//          SimplexLink *pTxLink = 
		//              m_hGraph.addSimplexLink(m_hGraph.getNextLinkId(),
		//                  pOXC->getId(), Vertex::VT_Access_Out, -1,
		//                  pOXC->getId(), Vertex::VT_Channel_Out, -1,
		//                  0, // 0 cost
		//                  0, NULL, SimplexLink::LT_Tx, -1, INFINITE_CAP);
		//          assert(pTxLink);
		//      } else if (0 == nNewFreeTx) { // remove from state graph TxE
		//          SimplexLink *pTxLink = m_hGraph.lookUpSimplexLink(
		//                                  pOXC->getId(), Vertex::VT_Access_Out, -1,
		//                                  pOXC->getId(), Vertex::VT_Channel_Out, -1);
		//          assert(pTxLink);
		//          m_hGraph.removeLink(pTxLink);
		//          delete pTxLink;
		//      }
			pTxSLink = m_hGraph.lookUpSimplexLink(
						pOXC->getId(), Vertex::VT_Access_Out, -1,
						pOXC->getId(), Vertex::VT_Channel_Out, -1);
			assert(pTxSLink);
			pTxSLink->m_hFreeCap = nNewFreeTx * OCLightpath; //-B: è una typedef -> OCLightpath = 192
			break;
		default:
			DEFAULT_SWITCH;
    };
    pOXC->m_nFreeTx = nNewFreeTx;
}

//-B: don't know what it does
//	so far, for case PT_BBU: nothing to do
void NetMan::consumeRx(OXCNode *pOXC, int nRx)
{
    assert(pOXC);

    int nNewFreeRx = (int)pOXC->m_nFreeRx + nRx;
    assert((0 <= nNewFreeRx) && (nNewFreeRx <= pOXC->getNumberOfRx()));

    SimplexLink *pRxSLink;
    switch (m_eProvisionType) {
	case PT_BBU:
	case PT_PAL_DPP:    // No need to do anything
        NULL;   
        break;
    case PT_PAL_SPP:
    case PT_PAC_DPP:    // update state graph when necessary
    case PT_PAC_SPP:
    case PT_SPAC_SPP:
// Original
//      if (0 == pOXC->m_nFreeRx) { // add to state graph RxE
//          assert(nRx > 0);
//          SimplexLink *pRxLink = 
//              m_hGraph.addSimplexLink(m_hGraph.getNextLinkId(),
//                  pOXC->getId(), Vertex::VT_Channel_In, -1,
//                  pOXC->getId(), Vertex::VT_Access_In, -1,
//                  0, // 0 cost
//                  0, NULL, SimplexLink::LT_Rx, -1, INFINITE_CAP);
//          assert(pRxLink);
//      } else if (0 == nNewFreeRx) { // remove from state graph RxE
//          SimplexLink *pRxLink = m_hGraph.lookUpSimplexLink(
//                                  pOXC->getId(), Vertex::VT_Channel_In, -1,
//                                  pOXC->getId(), Vertex::VT_Access_In, -1);
//          assert(pRxLink);
//          m_hGraph.removeLink(pRxLink);
//          delete pRxLink;
//      }
        pRxSLink = m_hGraph.lookUpSimplexLink(
                    pOXC->getId(), Vertex::VT_Channel_In, -1,
                    pOXC->getId(), Vertex::VT_Access_In, -1);
        assert(pRxSLink);
        pRxSLink->m_hFreeCap = nNewFreeRx * OCLightpath; //-B: è una typedef -> OCLightpath = 192
        break;
    default:
        DEFAULT_SWITCH;
    }
    pOXC->m_nFreeRx = nNewFreeRx;
}

//-B: reduce the capacity (of an amount of bandw eBW passed as parameter) of the simplx link object
//	identified by the reference to the lightpath passed as parameter
//	ATTENTION: only for some provisioning type! Check it!
void NetMan::consumeLightpathBandwidth(const Lightpath *pLightpath, 
                                       UINT eBW)
{
    consumeLightpathBandwidthHelper(pLightpath, 0 - (int)eBW);
}

//-B: increase the capacity (of an amount of bandw eBW passed as parameter) of the simplx link object
//	identified by the reference to the lightpath passed as parameter
//	ATTENTION: only for some provisioning type! Check it!
void NetMan::releaseLightpathBandwidth(const Lightpath *pLightpath, 
                                       UINT eBW)
{
    consumeLightpathBandwidthHelper(pLightpath, eBW);
}

//-B: basic method for consumeLightpathBandwidth and releaseLightpathBandwidth
//	ATTENTION: only for some provisioning type! Check it!
inline void NetMan::consumeLightpathBandwidthHelper(const Lightpath *pLightpath,
                                             int nBW)
{
    switch (m_eProvisionType) {
		case PT_PAL_DPP:    // No need to do anything
			NULL;   
			break;
		case PT_PAL_SPP:
		case PT_PAC_DPP:    
		case PT_PAC_SPP:
		case PT_SPAC_SPP:
		case PT_BBU:
			// Update state graph: 
			// consume bandwidth on the corresponding SimplexLink
			{
				assert(pLightpath->getId() > 0);
				LP2SimplexLinkMap::const_iterator itr = m_hLP2SLinkMap.find(pLightpath->getId());
				assert(itr != m_hLP2SLinkMap.end());
				assert(itr->second);
				itr->second->consumeBandwidthHelper(nBW);
			}
			break;
		default:
			DEFAULT_SWITCH;
    }
}

//**************************** PAC_SPP ****************************
bool NetMan::PAC_SPP_Provision(Connection *pCon)
{
    assert(PT_PAC_SPP == m_eProvisionType);
    // STEP 1: Compute primary path
    Vertex *pSrc = 
        m_hGraph.lookUpVertex(pCon->m_nSrc, Vertex::VT_Access_Out, -1);
    Vertex *pDst =
        m_hGraph.lookUpVertex(pCon->m_nDst, Vertex::VT_Access_In, -1);
    assert(pSrc && pDst);

    Circuit *pPCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_PAC_SPP_Primary, NULL);
    Circuit *pBCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_PAC_SPP_Backup, pPCircuit);
    pPCircuit->m_pCircuit = pBCircuit;

    bool bSuccess = 
            PAC_SPP_ProvisionHelper(pCon, *pPCircuit, *pBCircuit, pSrc, pDst);
    if (bSuccess) {
        // STEP 5: Associate the circuit w/ the connection
        pCon->m_pPCircuit = pPCircuit;
        pCon->m_pBCircuit = pBCircuit;

        // STEP 6: Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pPCircuit;
        delete pBCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hGraph.restoreLinkCost();
    m_hGraph.validateAllLinks();
    return bSuccess;
}

inline bool NetMan::PAC_SPP_ProvisionHelper(Connection *pCon, 
                                            Circuit& hPCircuit, 
                                            Circuit& hBCircuit, 
                                            Vertex *pSrc,
                                            Vertex *pDst)
{
    // Preprocess: Invalidate those links that don't have enough capacity
    m_hGraph.invalidateSimplexLinkDueToCap(pCon->m_eBandwidth);

    AbsPath *pPPath = NULL;
    list<AbstractLink*> hBPath;
    list<AbsPath*> hPPathList;
    LINK_COST hBestCost = UNREACHABLE;

    //m_hGraph.Yen(hPPathList, pSrc, pDst, m_nNumberOfPaths, this,
	//                AbstractGraph::LCF_ByOriginalLinkCost);

    if (0 == hPPathList.size()) {
        pCon->m_bBlockedDueToUnreach = true;
        return false;
    }
    list<AbsPath*>::const_iterator itr;
    for (itr=hPPathList.begin(); itr!=hPPathList.end(); itr++) {
        LINK_COST hCurrBCost;
        list<AbstractLink*> hCurrBPath;

        Circuit hTmpCircuit(pCon->m_eBandwidth, Circuit::CT_PAC_SPP_Primary, NULL);
        // NB: set up the primary circuit because the computation of a 
        // link-disjoint path depends on the UPDATED (w.r.t the primary circuit)
        // network state
        PAC_SPP_NewCircuit(hTmpCircuit, (*itr)->m_hLinkList);
        hTmpCircuit.setUp(this);

        // Validate those links previously invalidated due to capacity as
        // backups can be shared
        m_hGraph.validateAllLinks();
        PAC_InvalidatePath(m_hGraph, (*itr)->m_hLinkList);
        PAC_SPP_UpdateLinkCost_Backup(m_hGraph, hTmpCircuit);
        hCurrBCost = m_hGraph.Dijkstra(hCurrBPath, pSrc, pDst, 
                        AbstractGraph::LCF_ByOriginalLinkCost);

        hTmpCircuit.tearDown(this, false);  // do NOT log
        if (UNREACHABLE != hCurrBCost) {
            LINK_COST hCurrPCost = (*itr)->getCost() * pCon->m_eBandwidth;
            if (m_bWithOptimization) {
                MPAC_Optimize((*itr)->m_hLinkList, hCurrBPath, 
                    hCurrPCost, hCurrBCost, *pCon, pSrc, pDst);
            }
            if ((hCurrBCost + hCurrPCost) < hBestCost) {
                hBestCost = hCurrBCost + hCurrPCost;
                pPPath = (*itr);    // NB: cost & path not consistent here
                hBPath = hCurrBPath;
            }
        }
        
        // needed, otherwise, original cost will be overwritten
        m_hGraph.restoreLinkCost();     
    }

    if (NULL == pPPath)
        return false;

    // STEP 4: set up circuits when primary & backup are all set
    PAC_SPP_NewCircuit(hPCircuit, pPPath->m_hLinkList);
    hPCircuit.setUp(this);
    PAC_SPP_NewBackupCircuit(hBCircuit, hPCircuit, m_hGraph, pCon, hBPath);
    hBCircuit.setUp(this);

#ifdef _OCHDEBUG8
    cout<<"- After provisioning"<<endl;
    this->dump(cout);
    cout<<endl<<"- Working circuit"<<endl;
    hPCircuit.dump(cout);
    cout<<endl<<"- Backup circuit"<<endl;
    hBCircuit.dump(cout);
#endif


/*
    // original
    list<AbstractLink*> hPrimaryPath, hBackupPath;
    LINK_COST hPrimaryCost, hBackupCost;
    hPrimaryCost = m_hGraph.Dijkstra(hPrimaryPath, pSrc, pDst,
                        AbstractGraph::LCF_ByOriginalLinkCost);

    if (UNREACHABLE == hPrimaryCost) {
        pCon->m_bBlockedDueToUnreach = true;
        return false;
    }
    // NB: set up the primary circuit because the computation of a 
    // link-disjoint path depends on the UPDATED (w.r.t the primary circuit)
    // network state
    PAC_SPP_NewCircuit(hPCircuit, hPrimaryPath);
    hPCircuit.setUp(this);

    // Validate those links previously invalidated due to capacity as
    // backups can be shared
    m_hGraph.validateAllLinks();
    // STEP 2: Compute a link-disjoint min-cost path as backup
    // STEP 2.1: Invalidate those non-SRG-disjoint links
    PAC_InvalidatePath(m_hGraph, hPrimaryPath, pSrc, pDst);
    // STEP 2.2: Define link cost for computing backup
    PAC_SPP_UpdateLinkCost_Backup(m_hGraph, hPCircuit);

    hBackupCost = m_hGraph.Dijkstra(hBackupPath, pSrc, pDst, 
                        AbstractGraph::LCF_ByOriginalLinkCost);
    if (UNREACHABLE == hBackupCost) {   
        hPCircuit.tearDown(this, false);    // do NOT log
        return false;
    }

    // STEP 4: set up circuits when primary & backup are all set
    PAC_SPP_NewBackupCircuit(hBCircuit, m_hGraph, pCon, hBackupPath);
    hBCircuit.setUp(this);
*/
    return true;
}

// Construct a circuit and consume bandwidth along hLinkList 
void NetMan::PAC_SPP_NewCircuit(Circuit& hCircuit, 
                                const list<AbstractLink*>& hLinkList)
{
    list<AbstractLink*>::const_iterator itr = hLinkList.begin();
    while (itr != hLinkList.end()) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
        case SimplexLink::LT_UniFiber:
            {
            // 0 indicates it's a new lightpath to be set up
            Lightpath *pLightpath = new Lightpath(0, 
                                        Lightpath::LPT_PAC_SharedPath);
            pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            while (itr != hLinkList.end()) {
                pLink = (SimplexLink*)(*itr);
                assert(pLink);
                SimplexLink::SimplexLinkType eSLType = 
                    pLink->m_eSimplexLinkType;
                if ((SimplexLink::LT_Channel == eSLType) ||
                    (SimplexLink::LT_UniFiber == eSLType)) {
                    // consume bandwidth along this physical link
                    pLink->consumeBandwidth(OCLightpath);
                    assert(pLink->m_pUniFiber);
                    // construct lightpath route
                    pLightpath->m_hPRoute.push_back(pLink->m_pUniFiber);
                    // if it's OCLightpath, then there is no need to break
                    if (OCLightpath > hCircuit.m_eBW) {
                        OXCNode *pNode = (OXCNode*)pLink->m_pUniFiber->getDst();
                        assert(pNode);
                        if ((pNode->m_nFreeTx > 0) && (pNode->m_nFreeRx > 0) &&
                            ((pNode->m_nFreeTx - 1) >= 
                                m_dTxThreshold * pNode->getNumberOfTx()) &&
                            ((pNode->m_nFreeRx - 1) >=
                                m_dTxThreshold * pNode->getNumberOfRx())) {
                            break;  // Sufficient # of Tx & Rx, break the lightpath
                        }
                    }
                } else if (SimplexLink::LT_Lightpath == eSLType) {
                    itr--;
                    break;
                }
                itr++;
            }
            // appendLightpath will add to the state graph a link corresponding
            // to this lightpath
            hCircuit.addVirtualLink(pLightpath);
            }
            break;
        case SimplexLink::LT_Lightpath:
            assert(pLink->m_pLightpath);
            pLink->m_pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            hCircuit.addVirtualLink(pLink->m_pLightpath);
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter:
            NULL;   // No need to do anything for these cases
            break;
        default:
            DEFAULT_SWITCH;
        }
        if (itr != hLinkList.end()) itr++;
    }
}

inline void NetMan::PAC_SPP_UpdateLinkCost_Backup(Graph& hGraph,
                                                  const Circuit& hPCircuit)
{
    BandwidthGranularity eBW = hPCircuit.m_eBW;
    list<AbstractLink*>::const_iterator itr;
    for (itr=hGraph.m_hLinkList.begin(); itr!=hGraph.m_hLinkList.end(); itr++) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        if (!pLink->valid()) continue;

        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
            assert(false);  // for wavelength-continuous case, to do
            break;
        case SimplexLink::LT_UniFiber:
            assert(0 == pLink->m_hFreeCap % OCLightpath);
            if (pLink->m_hFreeCap >= eBW)
                pLink->modifyCost(eBW);
            else
                pLink->invalidate();
            break;
        case SimplexLink::LT_Lightpath:
            PAC_SPP_UpdateLinkCost_Backup_Lightpath(hPCircuit, *pLink);
            break;
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
            if (pLink->m_hFreeCap < hPCircuit.m_eBW)
                pLink->invalidate();
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Converter:
            // pLink->modifyCost(0);    // it's always 0, leave it as it is
            break;
        default:
            DEFAULT_SWITCH;
        }
    }
}

inline void NetMan::PAC_SPP_UpdateLinkCost_Backup_Lightpath(const Circuit& hPCircuit,
                                                            SimplexLink& hSLink)
{
    // hSLink corresponds to a lightpath
    assert(hSLink.m_pLightpath);
    Lightpath *pLP = hSLink.m_pLightpath;
    BandwidthGranularity eBW = hPCircuit.m_eBW;
    UINT nUniFibers = m_hWDMNet.getNumberOfLinks();
    LINK_COST hLPCost = pLP->getCost();
    UINT nBWAvailable = pLP->getFreeCapacity();
    assert(hLPCost > 0);
    
    // first time to allocate bw on this lightpath?
    if (NULL == pLP->m_pCSet) {
        if (nBWAvailable < eBW) {
            hSLink.invalidate();
        } else {
            pLP->allocateConflictSet(nUniFibers);
            
            hSLink.modifyCost(hLPCost * eBW);
            pLP->m_nBWToBeAllocated = eBW;
        }
        return;
    }

    // not the first time
    UINT nBWToBeShared = eBW;   // upper limit
    list<Lightpath*>::const_iterator itrLP;
    for (itrLP=hPCircuit.m_hRoute.begin(); itrLP!=hPCircuit.m_hRoute.end(); 
    itrLP++) {
        UINT  nDiff;
        list<UniFiber*>::const_iterator itrUniFiber;
        for (itrUniFiber=(*itrLP)->m_hPRoute.begin(); 
        itrUniFiber!=(*itrLP)->m_hPRoute.end(); itrUniFiber++) {
            assert(pLP->m_nBackupCapacity >= pLP->m_pCSet[(*itrUniFiber)->getId()]);
            nDiff = 
                pLP->m_nBackupCapacity - pLP->m_pCSet[(*itrUniFiber)->getId()];
            if (nBWToBeShared > nDiff) {
                nBWToBeShared = nDiff;
                if (0 == nBWToBeShared) break;
            }
        }
        if (0 == nBWToBeShared) break;
    }
    if ((nBWToBeShared + nBWAvailable) < eBW)
        hSLink.invalidate();
    else if (nBWToBeShared >= eBW) {
        hSLink.modifyCost(hLPCost * SMALL_COST);
        pLP->m_nBWToBeAllocated = 0;
    } else {
        hSLink.modifyCost(hLPCost * (eBW - nBWToBeShared));
        pLP->m_nBWToBeAllocated = (eBW - nBWToBeShared);
    }
}

// Construct a backup circuit (can be shared) and 
// consume bandwidth along hLinkList 
void NetMan::PAC_SPP_NewBackupCircuit(Circuit& hCircuit, 
                                      const Circuit& hPCircuit,
                                      Graph& hGraph, 
                                      Connection *pCon, 
                                      const list<AbstractLink*>& hLinkList)
{
    list<AbstractLink*>::const_iterator itr = hLinkList.begin();
    while (itr != hLinkList.end()) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
        case SimplexLink::LT_UniFiber:
            {
            // 0 indicates it's a new lightpath to be set up
            Lightpath *pLightpath = new Lightpath(0, 
                                        Lightpath::LPT_PAC_SharedPath);
            pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            pLightpath->allocateConflictSet(m_hWDMNet.getNumberOfLinks());

            while (itr != hLinkList.end()) {
                pLink = (SimplexLink*)(*itr);
                assert(pLink);
                SimplexLink::SimplexLinkType eSLType = 
                    pLink->m_eSimplexLinkType;
                if ((SimplexLink::LT_Channel == eSLType) ||
                    (SimplexLink::LT_UniFiber == eSLType)) {
                    // consume bandwidth along this physical link
                    pLink->consumeBandwidth(OCLightpath);
                    assert(pLink->m_pUniFiber);
                    // construct lightpath route
                    pLightpath->m_hPRoute.push_back(pLink->m_pUniFiber);
                    // NB: even for OCLightpath, we can break its backup
                    // to increase backup sharing
                    // if (OCLightpath > pCon->m_eBandwidth) 
                    {
                        OXCNode *pNode = (OXCNode*)pLink->m_pUniFiber->getDst();
                        assert(pNode);
                        if ((pNode->m_nFreeTx > 0) && (pNode->m_nFreeRx > 0) &&
                            ((pNode->m_nFreeTx - 1) >= 
                                m_dTxThreshold * pNode->getNumberOfTx()) &&
                            ((pNode->m_nFreeRx - 1) >=
                                m_dTxThreshold * pNode->getNumberOfRx())) {
                            break;  // Sufficient # of Tx & Rx, break the lightpath
                        }
                    }
                } else if (SimplexLink::LT_Lightpath == eSLType) {
                    itr--;
                    break;
                }
                itr++;
            }
            // appendLightpath will add to the state graph a link corresponding
            // to this lightpath
            hCircuit.addVirtualLink(pLightpath);
            }
            break;
        case SimplexLink::LT_Lightpath:
            assert(pLink->m_pLightpath);
            // NB: m_nBWToBeAllocated is set in 
            // PAC_SPP_UpdateLinkCost_Backup_Lightpath
            // However, this does NOT work for K>1
            PAC_SPP_UpdateLinkCost_Backup_Lightpath(hPCircuit, *pLink);
            hCircuit.addVirtualLink(pLink->m_pLightpath);
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter:
            NULL;   // No need to do anything for these cases
            break;
        default:
            DEFAULT_SWITCH;
        }
        if (itr != hLinkList.end()) itr++;
    }
}

void NetMan::PAC_SPP_Deprovision(Connection* pCon)
{
#ifdef _OCHDEBUG5
    {
        cout<<"- Deprovisioning connection"<<endl;
        pCon->dump(cout);
    }
#endif

    // first release the backup cause backup depends on primary
    if (pCon->m_pBCircuit)
        pCon->m_pBCircuit->tearDown(this);
    if (pCon->m_pPCircuit)
        pCon->m_pPCircuit->tearDown(this);
}

//**************************** PAL_SPP ****************************
inline void NetMan::PAL_SPP_Deprovision(Connection* pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

inline bool NetMan::PAL_SPP_Provision(Connection* pCon)
{
    assert(PT_PAL_SPP == m_eProvisionType);
    Vertex *pSrc = 
        m_hGraph.lookUpVertex(pCon->m_nSrc, Vertex::VT_Access_Out, -1);
    Vertex *pDst =
        m_hGraph.lookUpVertex(pCon->m_nDst, Vertex::VT_Access_In, -1);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_PAL_SPP, NULL);

    bool bSuccess = 
            PAL_SPP_Helper(pCon, *pCircuit, pSrc, pDst);
    if (bSuccess) {
        // STEP 5: Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // STEP 6: Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hGraph.restoreLinkCost();
    m_hGraph.validateAllLinks();
    return bSuccess;
}

inline bool NetMan::PAL_SPP_Helper(Connection* pCon, 
                                   Circuit& hCircuit, 
                                   Vertex* pSrc, 
                                   Vertex* pDst)
{
    // Preprocess: Invalidate those links that don't have enough capacity
    m_hGraph.invalidateSimplexLinkDueToCap(pCon->m_eBandwidth);

    list<AbstractLink*> hPrimaryPath;
    LINK_COST hPrimaryCost;
    hPrimaryCost = m_hGraph.Dijkstra(hPrimaryPath, pSrc, pDst,
                        AbstractGraph::LCF_ByOriginalLinkCost);

    if (UNREACHABLE == hPrimaryCost) {
        // assert(bCutSet);
        pCon->m_bBlockedDueToUnreach = true;
        return false;
    }
    // NB: set up the primary circuit because the computation of a 
    // link-disjoint path depends on the UPDATED (w.r.t the primary circuit)
    // network state
    list<Lightpath*> hNewLPList;
    PAL_SPP_New_Circuit(hCircuit, hNewLPList, hPrimaryPath);
    hCircuit.setUp(this);

    list<Lightpath*>::const_iterator itr;
    for (itr=hNewLPList.begin(); itr!=hNewLPList.end(); itr++) {
        // Validate those links previously invalidated because capacity as
        // backups can be shared
        m_hGraph.validateAllLinks();
        m_hGraph.restoreLinkCost();
        // invalidate the fibers used by the primary lightpath
        list<UniFiber*>::const_iterator itrUniFiber;
        for (itrUniFiber=(*itr)->m_hPRoute.begin(); 
        itrUniFiber!=(*itr)->m_hPRoute.end(); itrUniFiber++) {
            m_hGraph.invalidateUniFiberSimplexLink(*itrUniFiber);
        }
//      list<AbstractLink*>::const_iterator itrLink;
//      for (itrLink=hPrimaryPath.begin(); itrLink!=hPrimaryPath.end();
//      itrLink++) {
//          if (SimplexLink::LT_UniFiber == 
//              ((SimplexLink*)(*itrLink))->getSimplexLinkType())
//              (*itrLink)->invalidate();
//      }

        // STEP 2.2: Define link cost for computing backup
        PAL_SPP_UpdateLinkCost_Backup(m_hGraph, (*itr)->m_hPRoute);

        Vertex *pLPSrc = m_hGraph.lookUpVertex((*itr)->getSrc()->getId(),
                                        Vertex::VT_Access_Out, -1);
        Vertex *pLPDst = m_hGraph.lookUpVertex((*itr)->getDst()->getId(), 
                                        Vertex::VT_Access_In, -1);
        assert(pLPSrc && pLPDst);
        list<AbstractLink*> hBackupPath;
        LINK_COST hBackupCost;
        hBackupCost = m_hGraph.Dijkstra(hBackupPath, pLPSrc, pLPDst,
                            AbstractGraph::LCF_ByOriginalLinkCost);
        if (UNREACHABLE == hBackupCost) {   
            hCircuit.tearDown(this, false); // do NOT log
            return false;
        }
        (*itr)->attachBackup(hBackupPath);
    }
    return true;
}

inline void NetMan::PAL_SPP_New_Circuit(Circuit& hCircuit, 
                                        list<Lightpath*>& hNewLPList,
                                        const list<AbstractLink*>& hLinkList)
{
    list<AbstractLink*>::const_iterator itr = hLinkList.begin();
    while (itr != hLinkList.end()) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
        case SimplexLink::LT_UniFiber:
            {
            // 0 indicates it's a new lightpath to be set up
            Lightpath *pLightpath = new Lightpath(0, 
                                        Lightpath::LPT_PAL_Shared);
            hNewLPList.push_back(pLightpath);
            pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            while (itr != hLinkList.end()) {
                pLink = (SimplexLink*)(*itr);
                assert(pLink);
                SimplexLink::SimplexLinkType eSLType = 
                    pLink->m_eSimplexLinkType;
                if ((SimplexLink::LT_Channel == eSLType) ||
                    (SimplexLink::LT_UniFiber == eSLType)) {
                    // consume bandwidth along this physical link
                    pLink->consumeBandwidth(OCLightpath);
                    assert(pLink->m_pUniFiber);
                    // construct lightpath route
                    pLightpath->m_hPRoute.push_back(pLink->m_pUniFiber);
                    // if it's OCLightpath, then there is no need to break
                    if (OCLightpath > hCircuit.m_eBW) {
                        OXCNode *pNode = (OXCNode*)pLink->m_pUniFiber->getDst();
                        assert(pNode);
                        if ((pNode->m_nFreeTx > 0) && (pNode->m_nFreeRx > 0) &&
                            ((pNode->m_nFreeTx - 1) >= 
                                m_dTxThreshold * pNode->getNumberOfTx()) &&
                            ((pNode->m_nFreeRx - 1) >=
                                m_dTxThreshold * pNode->getNumberOfRx())) {
                            break;  // Sufficient # of Tx & Rx, break the lightpath
                        }
                    }
                } else if (SimplexLink::LT_Lightpath == eSLType) {
                    itr--;
                    break;
                }
                itr++;
            }
            // appendLightpath will add to the state graph a link corresponding
            // to this lightpath
            hCircuit.addVirtualLink(pLightpath);
            }
            break;
        case SimplexLink::LT_Lightpath:
            assert(pLink->m_pLightpath);
            pLink->m_pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            hCircuit.addVirtualLink(pLink->m_pLightpath);
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter:
            NULL;   // No need to do anything for these cases
            break;
        default:
            DEFAULT_SWITCH;
        }
        if (itr != hLinkList.end()) itr++;
    }
}

inline void NetMan::PAL_SPP_UpdateLinkCost_Backup(Graph& hGraph, 
                                                  const list<UniFiber*>& hPPath)
{
    list<AbstractLink*>::const_iterator itr;
    for (itr=hGraph.m_hLinkList.begin(); itr!=hGraph.m_hLinkList.end(); itr++) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        if (!pLink->valid()) continue;

        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
            // for wavelength-continuous case, to do
            assert(false);  
            break;
        case SimplexLink::LT_UniFiber:
            assert(0 == pLink->m_hFreeCap % OCLightpath);
            assert(pLink->m_pUniFiber && pLink->m_pCSet);
            {
                bool bShareable = true;
                list<UniFiber*>::const_iterator itrFiber;
                for (itrFiber=hPPath.begin(); 
                (itrFiber!=hPPath.end()) && bShareable; itrFiber++) {
                    if (pLink->m_nBChannels == 
                        pLink->m_pCSet[(*itrFiber)->getId()])
                        bShareable = false;
                }
                if (bShareable)
                    pLink->modifyCost(SMALL_COST);
                else if (pLink->m_hFreeCap > 0)
                    pLink->modifyCost(1);
                else
                    pLink->invalidate();
            }
            break;
        case SimplexLink::LT_Lightpath:
            pLink->invalidate();    // backup should not be set up
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter:
            // pLink->modifyCost(0);    // it's always 0, leave it as it is
            break;
        default:
            DEFAULT_SWITCH;
        }
    }
}

//-B: return the num of free channels over all the links in the network
UINT NetMan::countFreeChannels() const
{
    return m_hWDMNet.countFreeChannels();
}

//**************************** SPAC_SPP ****************************
inline void NetMan::SPAC_SPP_Deprovision(Connection* pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
    pCon->m_eStatus = Connection::TORNDOWN;
}

inline bool NetMan::SPAC_SPP_Provision(Connection *pCon)
{
    assert(PT_SPAC_SPP == m_eProvisionType);
    // STEP 1: Compute primary path
    Vertex *pSrc = 
        m_hGraph.lookUpVertex(pCon->m_nSrc, Vertex::VT_Access_Out, -1);
    Vertex *pDst =
        m_hGraph.lookUpVertex(pCon->m_nDst, Vertex::VT_Access_In, -1);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SPAC_SPP, NULL);

    bool bSuccess = 
            SPAC_SPP_Helper(pCon, *pCircuit, pSrc, pDst);
    if (bSuccess) {
        // STEP 5: Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // STEP 6: Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hGraph.restoreLinkCost();
    m_hGraph.validateAllLinks();
    return bSuccess;
}

inline bool NetMan::SPAC_SPP_Helper(Connection *pCon, 
                                    Circuit& hPCircuit, 
                                    Vertex *pSrc, 
                                    Vertex *pDst)
{
    // Preprocess: Invalidate those links that don't have enough capacity
    m_hGraph.invalidateSimplexLinkDueToCap(pCon->m_eBandwidth);

    AbsPath *pPPath = NULL;
    list<AbstractLink*> hBPath;
    list<AbsPath*> hPPathList;
    LINK_COST hBestCost = UNREACHABLE;

    //m_hGraph.Yen(hPPathList, pSrc, pDst, m_nNumberOfPaths, this,
	//                AbstractGraph::LCF_ByOriginalLinkCost);

    if (0 == hPPathList.size()) {
        pCon->m_bBlockedDueToUnreach = true;
        return false;
    }
    list<AbsPath*>::const_iterator itr;
    for (itr=hPPathList.begin(); itr!=hPPathList.end(); itr++) {
        LINK_COST hCurrBCost;
        list<AbstractLink*> hCurrBPath;
        Circuit hTmpCircuit(pCon->m_eBandwidth, Circuit::CT_SPAC_SPP, NULL);
        // NB: set up the primary circuit because the computation of a 
        // link-disjoint path depends on the UPDATED (w.r.t the primary circuit)
        // network state
        SPAC_SPP_NewCircuit(hTmpCircuit, (*itr)->m_hLinkList);
        hTmpCircuit.setUp(this);

        // Validate those links previously invalidated due to capacity as
        // backups can be shared
        m_hGraph.validateAllLinks();
        // STEP 2: Compute a link-disjoint min-cost path as backup
        // STEP 2.1: Invalidate those non-SRG-disjoint links
        SPAC_InvalidatePath(m_hGraph, hTmpCircuit);
        // STEP 2.2: Define link cost for computing backup
        SPAC_SPP_UpdateLinkCost_Backup(m_hGraph, hTmpCircuit, pCon->m_eBandwidth);

        hCurrBCost = m_hGraph.Dijkstra(hCurrBPath, pSrc, pDst, 
                            AbstractGraph::LCF_ByOriginalLinkCost);
        hTmpCircuit.tearDown(this, false);  // do NOT log
        if (UNREACHABLE != hCurrBCost) {
            LINK_COST hCurrPCost = (*itr)->getCost() * pCon->m_eBandwidth;
            if (m_bWithOptimization) {
                SPAC_Optimize((*itr)->m_hLinkList, hCurrBPath, 
                    hCurrPCost, hCurrBCost, *pCon, pSrc, pDst);
            }
            if ((hCurrBCost + hCurrPCost) < hBestCost) {
                hBestCost = hCurrBCost + hCurrPCost;
                pPPath = (*itr);    // NB: cost & path not consistent here
                hBPath = hCurrBPath;
            }
        }
        
        // needed, otherwise, original cost will be overwritten
        m_hGraph.restoreLinkCost();     
    }

    if (UNREACHABLE != hBestCost) {
        SPAC_SPP_NewCircuit(hPCircuit, pPPath->m_hLinkList);
        hPCircuit.setUp(this);
        m_hGraph.validateAllLinks();
        SPAC_InvalidatePath(m_hGraph, hPCircuit);
        SPAC_SPP_UpdateLinkCost_Backup(m_hGraph, hPCircuit, pCon->m_eBandwidth);
        // STEP 4: attach the backup
        hPCircuit.SPAC_AttachBackup(this, hBPath);
        return true;
    } else {
        return false;
    }

// NB: neat idea, does not work though
        // distinguish Tx or wavelength blocking
//      m_hGraph.restoreLinkCost();
//      m_hGraph.validateAllLinks();
//      SPAC_InvalidatePath(m_hGraph, hPCircuit);
//      SPAC_SPP_UpdateLinkCost_Backup(m_hGraph, hPCircuit, 
//          pCon->m_eBandwidth, true);
//
//      hBackupCost = m_hGraph.Dijkstra(hBackupPath, pSrc, pDst, 
//                          AbstractGraph::LCF_ByOriginalLinkCost);
//      if (UNREACHABLE == hBackupCost)
//          pCon->m_bBlockedDueToTx = false;
//      else
//          pCon->m_bBlockedDueToTx = true;
}

void NetMan::SPAC_InvalidatePath(Graph& hGraph, Circuit& hCircuit)
{
    list<Lightpath*>::const_iterator itr;
    for (itr=hCircuit.m_hRoute.begin(); itr!=hCircuit.m_hRoute.end(); itr++) {
        list<UniFiber*>::const_iterator itrFiber;
        for (itrFiber=(*itr)->m_hPRoute.begin(); 
        itrFiber!=(*itr)->m_hPRoute.end(); itrFiber++)
            hGraph.invalidateSimplexLinkDueToSRG(*itrFiber);
    }
}

// Construct a circuit and consume bandwidth along hLinkList 
void NetMan::SPAC_SPP_NewCircuit(Circuit& hCircuit, 
                                 const list<AbstractLink*>& hLinkList)
{
    list<AbstractLink*>::const_iterator itr = hLinkList.begin();
    while (itr != hLinkList.end()) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
        case SimplexLink::LT_UniFiber:
            {
            // 0 indicates it's a new lightpath to be set up
            Lightpath *pLightpath = new Lightpath(0, 
                                        Lightpath::LPT_SPAC_SharedPath);
            pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            while (itr != hLinkList.end()) {
                pLink = (SimplexLink*)(*itr);
                assert(pLink);
                SimplexLink::SimplexLinkType eSLType = 
                    pLink->m_eSimplexLinkType;
                if ((SimplexLink::LT_Channel == eSLType) ||
                    (SimplexLink::LT_UniFiber == eSLType)) {
                    // consume bandwidth along this physical link
                    pLink->consumeBandwidth(OCLightpath);
                    assert(pLink->m_pUniFiber);
                    // construct lightpath route
                    pLightpath->m_hPRoute.push_back(pLink->m_pUniFiber);
                    // if it's OCLightpath, then there is no need to break
                    if (OCLightpath > hCircuit.m_eBW) {
                        OXCNode *pNode = (OXCNode*)pLink->m_pUniFiber->getDst();
                        assert(pNode);
                        if ((pNode->m_nFreeTx > 0) && (pNode->m_nFreeRx > 0) &&
                            ((pNode->m_nFreeTx - 1) >= 
                                m_dTxThreshold * pNode->getNumberOfTx()) &&
                            ((pNode->m_nFreeRx - 1) >=
                                m_dTxThreshold * pNode->getNumberOfRx())) {
                            break;  // Sufficient # of Tx & Rx, break the lightpath
                        }
                    }
                } else if (SimplexLink::LT_Lightpath == eSLType) {
                    itr--;
                    break;
                }
                itr++;
            }
            // appendLightpath will add to the state graph a link corresponding
            // to this lightpath
            hCircuit.addVirtualLink(pLightpath);
            }
            break;
        case SimplexLink::LT_Lightpath:
            assert(pLink->m_pLightpath);
            pLink->m_pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
            hCircuit.addVirtualLink(pLink->m_pLightpath);
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter:
            NULL;   // No need to do anything for these cases
            break;
        default:
            DEFAULT_SWITCH;
        }
        if (itr != hLinkList.end()) itr++;
    }
}

inline void NetMan::SPAC_SPP_UpdateLinkCost_Backup(Graph& hGraph, 
                                                   Circuit& hCircuit,
                                                   BandwidthGranularity eBW,
                                                   bool bIgnoreTx)
{
    list<AbstractLink*>::const_iterator itr;
    for (itr=hGraph.m_hLinkList.begin(); itr!=hGraph.m_hLinkList.end(); itr++) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        if (!pLink->valid()) continue;

        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
            // for wavelength-continuous case, to do
            assert(false);  
            break;
        case SimplexLink::LT_UniFiber:
            assert(0 == pLink->m_hFreeCap % OCLightpath);
            assert(pLink->m_pUniFiber && pLink->m_pCSet);
            {
                LINK_CAPACITY hShareableCap = eBW;
                LINK_CAPACITY hDiff;
                list<Lightpath*>::const_iterator itrLP;
                for (itrLP=hCircuit.m_hRoute.begin(); 
                itrLP!=hCircuit.m_hRoute.end(); itrLP++) {
                    list<UniFiber*>::const_iterator itrFiber;
                    for (itrFiber=(*itrLP)->m_hPRoute.begin(); 
                    itrFiber!=(*itrLP)->m_hPRoute.end(); itrFiber++) {
                        hDiff = pLink->m_nBackupCap 
                              - pLink->m_pCSet[(*itrFiber)->getId()];
                        if (hDiff < hShareableCap) {
                            hShareableCap = hDiff;
                            if (0 == hShareableCap)
                                break;
                        }
                    } // for
                    if (0 == hShareableCap)
                        break;
                } // for
                if (eBW == hShareableCap) {
                    pLink->modifyCost(SMALL_COST);
                    pLink->m_nBWToBeAllocated = 0;
                } else if (pLink->m_nBChannels * OCLightpath 
                    >= (pLink->m_nBackupCap + eBW - hShareableCap)) {
                    pLink->modifyCost(eBW - hShareableCap);
                    pLink->m_nBWToBeAllocated = eBW - hShareableCap;
                } else if (pLink->m_hFreeCap > 0) {
                    if (bIgnoreTx) {
                        pLink->modifyCost(eBW - hShareableCap);
                        pLink->m_nBWToBeAllocated = eBW - hShareableCap;
                    } else {
                        // need to allocate an add port at src & a drop port at dst
                        OXCNode *pSrcOXC, *pDstOXC;
                        pSrcOXC = ((Vertex*)pLink->m_pSrc)->m_pOXCNode;
                        pDstOXC = ((Vertex*)pLink->m_pDst)->m_pOXCNode;
                        assert(pSrcOXC && pDstOXC);
                        if ((0 == pSrcOXC->m_nFreeTx) 
                            || (0 == pDstOXC->m_nFreeRx)) {
                            pLink->invalidate();
                        } else {
                            pLink->modifyCost(eBW - hShareableCap);
                            pLink->m_nBWToBeAllocated = eBW - hShareableCap;
                        }
                    }
                } else {
                    pLink->invalidate();
                }
            }
            break;
        case SimplexLink::LT_Lightpath:
            pLink->invalidate();    // backup should not be set up
            break;
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
            // already been checked in LT_UniFiber case
//          if (pSLink->m_hFreeCap < eBW)
//              pSLink->invalidate();
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Converter:
            // pLink->modifyCost(0);    // it's always 0, leave it as it is
            break;
        default:
            DEFAULT_SWITCH;
        }
    }
}

//**************************** MPAC ****************************
void NetMan::MPAC_Optimize(list<AbstractLink*>& hPPath, 
                           list<AbstractLink*>& hBPath, 
                           LINK_COST& hPCost,
                           LINK_COST& hBCost,
                           const Connection& hCon,
                           Vertex *pSrc,
                           Vertex *pDst)
{
    Circuit hPCircuit(hCon.m_eBandwidth, Circuit::CT_PAC_SPP_Primary, NULL);
    Circuit hBCircuit(hCon.m_eBandwidth, Circuit::CT_PAC_SPP_Backup, NULL);

    LINK_COST hPCandidateCost, hBCandidateCost;
    list<AbstractLink*> hPCandidate;
    list<AbstractLink*> hBCandidate;
    int nCount = 0;
    while (nCount++ < 10) {
        m_hGraph.restoreLinkCost();
        m_hGraph.validateAllLinks();

        // set up backup circuit
        MPAC_Opt_NewBCircuit(hBCircuit, m_hGraph, hBPath);
        hBCircuit.MPAC_SetUp(this);

        // compute l_w given l_b
        PAC_InvalidatePath(m_hGraph, hBPath);
        MPAC_Opt_UpdateLinkCost_Primary(hBCircuit);
        m_hGraph.MPAC_Opt_ComputePrimaryGivenBackup(hPCandidate, 
            hPCandidateCost, hBCandidateCost, hBCircuit, pSrc, pDst);

        hBCircuit.MPAC_TearDown(this, false);   // release rc
        if ((hPCandidateCost + hBCandidateCost) >= (hPCost + hBCost)) {
            // done, use previous one
            break;
        }
        m_hGraph.restoreLinkCost();
        m_hGraph.validateAllLinks();
        hPPath = hPCandidate;
        hPCost = hPCandidateCost;
        hBCost = hBCandidateCost;
        
        // For primary circuit
        PAC_SPP_NewCircuit(hPCircuit, hPPath);
        hPCircuit.setUp(this);

        // compute l_b given l_w
        PAC_InvalidatePath(m_hGraph, hPPath);
        PAC_SPP_UpdateLinkCost_Backup(m_hGraph, hPCircuit);

        hBCandidateCost = m_hGraph.Dijkstra(hBCandidate, pSrc, pDst, 
                            AbstractGraph::LCF_ByOriginalLinkCost);
        hPCircuit.tearDown(this, false);    // do NOT log
        if (hBCandidateCost >= hBCost) {
            break;
        }
        hBPath = hBCandidate;
        hBCost = hBCandidateCost;

#ifdef _OCHDEBUG9
        {
            if (51 == hCon.m_nSequenceNo) {
                cout<<endl<<"P: C="<<hPCandidateCost<<" ";
                dumpAbsLinkList(hPPath);
                cout<<"B: C="<<hBCandidateCost<<" ";
                dumpAbsLinkList(hBPath);
//              m_hGraph.dump(cout);
            }
        }
#endif

        hPCircuit.m_hRoute.clear();
        hBCircuit.m_hRoute.clear();
    }
}

// NB: m_nBWToBeAllocated is not needed to be set
void NetMan::MPAC_Opt_NewBCircuit(Circuit& hBCircuit,
                                  Graph& hGraph, 
                                  const list<AbstractLink*>& hLinkList)
{
    list<AbstractLink*>::const_iterator itr = hLinkList.begin();
    while (itr != hLinkList.end()) {
        SimplexLink *pLink = (SimplexLink*)(*itr);
        assert(pLink);
        switch (pLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
        case SimplexLink::LT_UniFiber:
            {
            // 0 indicates it's a new lightpath to be set up
            Lightpath *pLightpath = new Lightpath(0, 
                                        Lightpath::LPT_PAC_SharedPath);
//          pLightpath->allocateConflictSet(m_hWDMNet.getNumberOfLinks());

            while (itr != hLinkList.end()) {
                pLink = (SimplexLink*)(*itr);
                assert(pLink);
                SimplexLink::SimplexLinkType eSLType = 
                    pLink->m_eSimplexLinkType;
                if ((SimplexLink::LT_Channel == eSLType) ||
                    (SimplexLink::LT_UniFiber == eSLType)) {
                    // consume bandwidth along this physical link
//                  pLink->consumeBandwidth(OCLightpath);
                    assert(pLink->m_pUniFiber);
                    // construct lightpath route
                    pLightpath->m_hPRoute.push_back(pLink->m_pUniFiber);
                    // NB: even for OCLightpath, we can break its backup
                    // to increase backup sharing
                    // if (OCLightpath > pCon->m_eBandwidth) 
                    {
                        OXCNode *pNode = (OXCNode*)pLink->m_pUniFiber->getDst();
                        assert(pNode);
                        if ((pNode->m_nFreeTx > 0) && (pNode->m_nFreeRx > 0) &&
                            ((pNode->m_nFreeTx - 1) >= 
                                m_dTxThreshold * pNode->getNumberOfTx()) &&
                            ((pNode->m_nFreeRx - 1) >=
                                m_dTxThreshold * pNode->getNumberOfRx())) {
                            break;  // Sufficient # of Tx & Rx, break the lightpath
                        }
                    }
                } else if (SimplexLink::LT_Lightpath == eSLType) {
                    itr--;
                    break;
                }
                itr++;
            }
            // appendLightpath will add to the state graph a link corresponding
            // to this lightpath
            hBCircuit.addVirtualLink(pLightpath);
            }
            break;
        case SimplexLink::LT_Lightpath:
            assert(pLink->m_pLightpath);
            hBCircuit.addVirtualLink(pLink->m_pLightpath);
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
        case SimplexLink::LT_Converter:
            NULL;   // No need to do anything for these cases
            break;
        default:
            DEFAULT_SWITCH;
        }
        if (itr != hLinkList.end()) itr++;
    }
}

void NetMan::MPAC_Opt_UpdateLinkCost_Primary(const Circuit& hBCircuit)
{
    BandwidthGranularity eBW = hBCircuit.m_eBW;
    UINT nBackupVHops = hBCircuit.m_hRoute.size();

    list<AbstractLink*>::const_iterator itr;
    for (itr=m_hGraph.m_hLinkList.begin(); itr!=m_hGraph.m_hLinkList.end();
    itr++) {
        SimplexLink *pSLink = (SimplexLink*)(*itr);
        assert(pSLink);
        if (!pSLink->valid()) continue;

        if (pSLink->m_pBackupCost) {
            delete [](pSLink->m_pBackupCost);
            pSLink->m_pBackupCost = NULL;
            pSLink->m_nBackupVHops = 0;
        }
        switch (pSLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
            assert(false);  // for wavelength-continuous case, to do
            break;
        case SimplexLink::LT_UniFiber:
            assert(pSLink->m_pUniFiber);
            assert(0 == pSLink->m_hFreeCap % OCLightpath);
            if (pSLink->m_hFreeCap < eBW) {
                pSLink->invalidate();
            } else {
                LINK_CAPACITY nDiff;
                bool bValid = true;
                pSLink->m_nBackupVHops = nBackupVHops;
                pSLink->m_pBackupCost = new LINK_COST[nBackupVHops];
                memset(pSLink->m_pBackupCost, 0, nBackupVHops * sizeof(LINK_COST));
                UINT nHop = 0;
                list<Lightpath*>::const_iterator itrLP;
                for (itrLP=hBCircuit.m_hRoute.begin(); 
                (itrLP!=hBCircuit.m_hRoute.end()) && (bValid); itrLP++) {
                    if (NULL == (*itrLP)->m_pCSet) {
                        if ((*itrLP)->getFreeCapacity() < eBW) {
                            bValid = false;
                        } else {
                            pSLink->m_pBackupCost[nHop] = 
                                eBW * (*itrLP)->getCost();
                        }
                    } else {
                        assert((*itrLP)->m_nBackupCapacity 
                            >= (*itrLP)->m_pCSet[pSLink->m_pUniFiber->getId()]);
                        nDiff = (*itrLP)->m_nBackupCapacity 
                              - (*itrLP)->m_pCSet[pSLink->m_pUniFiber->getId()];
                        if (nDiff >= eBW) {
                            pSLink->m_pBackupCost[nHop] = 
                                (*itrLP)->getCost() * SMALL_COST;
                        } else if ((nDiff + (*itrLP)->getFreeCapacity()) < eBW) {
                            bValid = false;
                        } else {
                            pSLink->m_pBackupCost[nHop] = 
                                (*itrLP)->getCost() * (eBW - nDiff);
                        }
                    }
                    nHop++;
                } // for itrLP
                if (bValid) {
                    // NB: since we jointly compute C(p) + C(b), and C(b)
                    // is a function of eBW, C(p) needs to be a function of
                    // eBW too.
                    pSLink->modifyCost(eBW);
                } else
                    pSLink->invalidate();
            }
            break;
        case SimplexLink::LT_Lightpath:
            MPAC_Opt_UpdateLinkCost_Primary_Lightpath(*pSLink, hBCircuit);
            break;
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
            if (pSLink->m_hFreeCap < eBW)
                pSLink->invalidate();
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Converter:
            // pSLink->modifyCost(0);   // it's always 0, leave it as it is
            break;
        default:
            DEFAULT_SWITCH;
        }
    }
}

inline void 
NetMan::MPAC_Opt_UpdateLinkCost_Primary_Lightpath(SimplexLink& hSLink, 
                                                  const Circuit& hBCircuit)
{
    assert(hSLink.m_pLightpath);

    BandwidthGranularity eBW = hBCircuit.m_eBW;
    if (hSLink.m_pLightpath->getFreeCapacity() < eBW) {
        hSLink.invalidate();
        return;
    }

    UINT nBackupVHops = hBCircuit.getLightpathHops();
    hSLink.m_nBackupVHops = nBackupVHops;
    hSLink.m_pBackupCost = new LINK_COST[nBackupVHops];
    memset(hSLink.m_pBackupCost, 0, nBackupVHops * sizeof(LINK_COST));

    bool bValid = true;
    list<UniFiber*>::const_iterator itr;
    for (itr=hSLink.m_pLightpath->m_hPRoute.begin();
    (itr!=hSLink.m_pLightpath->m_hPRoute.end()) && bValid; itr++) {
        UINT nHop = 0;
        UINT nDiff;
        list<Lightpath*>::const_iterator itrLP;
        for (itrLP=hBCircuit.m_hRoute.begin(); 
        (itrLP!=hBCircuit.m_hRoute.end()) && (bValid); itrLP++) {
            if (NULL == (*itrLP)->m_pCSet) {
                if ((*itrLP)->getFreeCapacity() < eBW) {
                    bValid = false;
                } else {
                    if (hSLink.m_pBackupCost[nHop]<eBW * (*itrLP)->getCost())
                        hSLink.m_pBackupCost[nHop] = eBW * (*itrLP)->getCost();
                }
            } else {
                assert((*itrLP)->m_nBackupCapacity 
                    >= (*itrLP)->m_pCSet[(*itr)->getId()]);
                nDiff = (*itrLP)->m_nBackupCapacity 
                      - (*itrLP)->m_pCSet[(*itr)->getId()];
                if (nDiff >= eBW) {
                    if (hSLink.m_pBackupCost[nHop] <
                        (*itrLP)->getCost() * SMALL_COST)
                        hSLink.m_pBackupCost[nHop] =
                            (*itrLP)->getCost() * SMALL_COST;
                } else if ((nDiff + (*itrLP)->getFreeCapacity()) < eBW) {
                    bValid = false;
                } else {
                    if (hSLink.m_pBackupCost[nHop] <
                        (*itrLP)->getCost() * (eBW - nDiff))
                        hSLink.m_pBackupCost[nHop] =
                            (*itrLP)->getCost() * (eBW - nDiff);
                }
            } // if
            nHop++;
        } // for itrLP
    } // for itr
    if (bValid) {
        hSLink.modifyCost(eBW * hSLink.m_pLightpath->getCost());
    } else {
        hSLink.invalidate();
    }
}

//-B: for each link of the list passed as parameter, print the source node id
//	and also print the destination node id of tha last link present in the list
void NetMan::dumpAbsLinkList(const list<AbstractLink*>& hList) const
{
    if (0 == hList.size()) {
        cout<<"NULL";
        return;
    }
    list<AbstractLink*>::const_iterator itr;
    for (itr=hList.begin(); itr!=hList.end(); itr++) {
        cout<<(*itr)->m_pSrc->m_nNodeId<<' ';
    }
    cout<<hList.back()->m_pDst->m_nNodeId<<endl;
}

//**************************** SPAC ****************************
void NetMan::SPAC_Optimize(list<AbstractLink*>& hPPath, 
                           list<AbstractLink*>& hBPath, 
                           LINK_COST& hPCost,
                           LINK_COST& hBCost,
                           const Connection& hCon,
                           Vertex *pSrc,
                           Vertex *pDst)
{
    Circuit hPCircuit(hCon.m_eBandwidth, Circuit::CT_SPAC_SPP, NULL);
    LINK_COST hPCandidateCost, hBCandidateCost;
    list<AbstractLink*> hPCandidate;
    list<AbstractLink*> hBCandidate;
    int nCount = 0;
    while (nCount++ < 10) {
        m_hGraph.restoreLinkCost();
        m_hGraph.validateAllLinks();

        // compute l_w given l_b
        PAC_InvalidatePath(m_hGraph, hBPath);
        // aux links do not matter
        list<AbstractLink*>::const_iterator itr;
        list<AbstractLink*> hDupBPath;  // backup path w/o aux links
        for (itr=hBPath.begin(); itr!=hBPath.end(); itr++) {
            SimplexLink *pSLink = (SimplexLink*)(*itr);
            assert(pSLink);
            if (SimplexLink::LT_UniFiber == pSLink->m_eSimplexLinkType)
                hDupBPath.push_back(*itr);
        }
        SPAC_Opt_UpdateLinkCost_Primary(hDupBPath, hCon.m_eBandwidth);
        m_hGraph.SPAC_Opt_ComputePrimaryGivenBackup(hPCandidate, 
            hPCandidateCost, hBCandidateCost, hDupBPath, pSrc, pDst);

        if ((hPCandidateCost + hBCandidateCost) >= (hPCost + hBCost)) {
            // done, use previous one
            break;
        }
        m_hGraph.restoreLinkCost();
        m_hGraph.validateAllLinks();
        hPPath = hPCandidate;
        hPCost = hPCandidateCost;
        hBCost = hBCandidateCost;
        

        SPAC_SPP_NewCircuit(hPCircuit, hPPath);
        hPCircuit.setUp(this);

        // Validate those links previously invalidated due to capacity as
        // backups can be shared
        m_hGraph.validateAllLinks();
        // STEP 2: Compute a link-disjoint min-cost path as backup
        // STEP 2.1: Invalidate those non-SRG-disjoint links
        SPAC_InvalidatePath(m_hGraph, hPCircuit);
        // STEP 2.2: Define link cost for computing backup
        SPAC_SPP_UpdateLinkCost_Backup(m_hGraph, hPCircuit, hCon.m_eBandwidth);

        hBCandidateCost = m_hGraph.Dijkstra(hBCandidate, pSrc, pDst, 
                            AbstractGraph::LCF_ByOriginalLinkCost);
        hPCircuit.tearDown(this, false);    // do NOT log
        if (hBCandidateCost >= hBCost) {
            break;
        }
        hBPath = hBCandidate;
        hBCost = hBCandidateCost;

#ifdef _OCHDEBUG9
        {
            if (51 == hCon.m_nSequenceNo) {
                cout<<endl<<"P: C="<<hPCandidateCost<<" ";
                dumpAbsLinkList(hPPath);
                cout<<"B: C="<<hBCandidateCost<<" ";
                dumpAbsLinkList(hBPath);
//              m_hGraph.dump(cout);
            }
        }
#endif

        hPCircuit.m_hRoute.clear();
    }
}

void NetMan::SPAC_Opt_UpdateLinkCost_Primary(const list<AbstractLink*>& hBPath,
                                             BandwidthGranularity eBW)
{
    UINT nBackupHops = hBPath.size();
    list<AbstractLink*>::const_iterator itr;
    for (itr=m_hGraph.m_hLinkList.begin(); itr!=m_hGraph.m_hLinkList.end(); 
    itr++) {
        SimplexLink *pSLink = (SimplexLink*)(*itr);
        assert(pSLink);
        if (!pSLink->valid()) continue;

        if (pSLink->m_pBackupCost) {
            delete [](pSLink->m_pBackupCost);
            pSLink->m_pBackupCost = NULL;
            pSLink->m_nBackupVHops = 0;
        }
        switch (pSLink->m_eSimplexLinkType) {
        case SimplexLink::LT_Channel:
            // for wavelength-continuous case, to do
            assert(false);  
            break;
        case SimplexLink::LT_UniFiber:
            assert(0 == pSLink->m_hFreeCap % OCLightpath);
            assert(pSLink->m_pUniFiber);
            if (pSLink->m_hFreeCap < eBW) {
                pSLink->invalidate();
            } else {
                LINK_CAPACITY nDiff;
                bool bValid = true;
                pSLink->m_nBackupVHops = nBackupHops;
                pSLink->m_pBackupCost = new LINK_COST[nBackupHops];
                memset(pSLink->m_pBackupCost, 0, nBackupHops * sizeof(LINK_COST));
                UINT nHop = 0;
                SimplexLink *pSBHop;
                list<AbstractLink*>::const_iterator itrAbsLink;
                for (itrAbsLink=hBPath.begin(); 
                (itrAbsLink!=hBPath.end()) && bValid; itrAbsLink++) {
                    pSBHop = (SimplexLink*)(*itrAbsLink);
                    assert(pSBHop && pSBHop->m_pUniFiber);
                    assert(SimplexLink::LT_UniFiber == pSBHop->getSimplexLinkType());
                    UniFiber *pPFiber = pSLink->m_pUniFiber;
                    if (NULL == pSBHop->m_pCSet) {
                        assert(pSBHop->m_hFreeCap > 0);
                        pSLink->m_pBackupCost[nHop] = eBW;
                    } else {
                        assert(pSBHop->m_nBackupCap >= 
                            pSBHop->m_pCSet[pPFiber->getId()]);
                        nDiff = pSBHop->m_nBackupCap 
                            - pSBHop->m_pCSet[pPFiber->getId()];
                        if (nDiff >= eBW)
                            pSLink->m_pBackupCost[nHop] = SMALL_COST;
                        else if ((nDiff + (pSBHop->m_nBChannels*OCLightpath - pSBHop->m_nBackupCap))
                                >= eBW)
                            pSLink->m_pBackupCost[nHop] = (eBW - nDiff);
                        else {
                            if (0 == pSBHop->m_hFreeCap)
                                bValid = false;
                            else {
                                OXCNode *pOXCSrc = 
                                    (OXCNode*)pSBHop->m_pUniFiber->m_pSrc;
                                OXCNode *pOXCDst = 
                                    (OXCNode*)pSBHop->m_pUniFiber->m_pDst;
                                assert(pOXCSrc && pOXCDst);
                                if ((0 == pOXCSrc->m_nFreeTx)
                                    || (0 == pOXCDst->m_nFreeRx))
                                    bValid = false;
                                else
                                    pSLink->m_pBackupCost[nHop] = (eBW - nDiff);
                            }
                        }
                    } // if (NULL == ...)
                    nHop++;
                } // for itrAbsLink
                if (bValid)
                    pSLink->modifyCost(eBW);
                else
                    pSLink->invalidate();
            }
            break;
        case SimplexLink::LT_Lightpath:
            SPAC_Opt_UpdateLinkCost_Primary_Lightpath(*pSLink, hBPath, eBW);
            break;
        case SimplexLink::LT_Tx:
        case SimplexLink::LT_Rx:
            if (pSLink->m_hFreeCap < eBW)
                pSLink->invalidate();
            break;
        case SimplexLink::LT_Channel_Bypass:
        case SimplexLink::LT_Grooming:
        case SimplexLink::LT_Mux:
        case SimplexLink::LT_Demux:
        case SimplexLink::LT_Converter:
            // pSLink->modifyCost(0);   // it's always 0, leave it as it is
            break;
        default:
            DEFAULT_SWITCH;
        }
    }
}

inline void 
NetMan::SPAC_Opt_UpdateLinkCost_Primary_Lightpath(SimplexLink& hSLink, 
                                                  const list<AbstractLink*>& hBPath,
                                                  BandwidthGranularity eBW)
{
    assert(hSLink.m_pLightpath);

    if (hSLink.m_pLightpath->getFreeCapacity() < eBW) {
        hSLink.invalidate();
        return;
    }

    UINT nBackupHops = hBPath.size();
    hSLink.m_nBackupVHops = nBackupHops;
    hSLink.m_pBackupCost = new LINK_COST[nBackupHops];
    memset(hSLink.m_pBackupCost, 0, nBackupHops * sizeof(LINK_COST));

    bool bValid = true;
    list<UniFiber*>::const_iterator itr;
    for (itr=hSLink.m_pLightpath->m_hPRoute.begin();
    (itr!=hSLink.m_pLightpath->m_hPRoute.end()) && bValid; itr++) {
        UINT nHop = 0;
        UINT nDiff;
        SimplexLink *pSBHop;
        list<AbstractLink*>::const_iterator itrAbsLink;
        for (itrAbsLink=hBPath.begin(); (itrAbsLink!=hBPath.end()) && bValid;
        itrAbsLink++) {
            pSBHop = (SimplexLink*)(*itrAbsLink);
            assert(pSBHop && pSBHop->m_pUniFiber);
            assert(SimplexLink::LT_UniFiber == pSBHop->getSimplexLinkType());
            UniFiber *pPFiber = (*itr);
            if (NULL == pSBHop->m_pCSet) {
                assert(pSBHop->m_hFreeCap > 0);
                if (hSLink.m_pBackupCost[nHop] < eBW)
                    hSLink.m_pBackupCost[nHop] = eBW;
            } else {
                assert(pSBHop->m_nBackupCap >= 
                    pSBHop->m_pCSet[pPFiber->getId()]);
                nDiff = pSBHop->m_nBackupCap 
                    - pSBHop->m_pCSet[pPFiber->getId()];
                if (nDiff >= eBW) {
                    if (hSLink.m_pBackupCost[nHop] < SMALL_COST)
                        hSLink.m_pBackupCost[nHop] = SMALL_COST;
                } else if ((nDiff + (pSBHop->m_nBChannels*OCLightpath - pSBHop->m_nBackupCap))
                    >= eBW) {
                    if (hSLink.m_pBackupCost[nHop] < (eBW - nDiff))
                        hSLink.m_pBackupCost[nHop] = (eBW - nDiff);
                } else {
                    if (0 == pSBHop->m_hFreeCap)
                        bValid = false;
                    else {
                        OXCNode *pOXCSrc = 
                            (OXCNode*)pSBHop->m_pUniFiber->m_pSrc;
                        OXCNode *pOXCDst = 
                            (OXCNode*)pSBHop->m_pUniFiber->m_pDst;
                        assert(pOXCSrc && pOXCDst);
                        if ((0 == pOXCSrc->m_nFreeTx)
                            || (0 == pOXCDst->m_nFreeRx))
                            bValid = false;
                        else {
                            if (hSLink.m_pBackupCost[nHop] < (eBW - nDiff))
                                hSLink.m_pBackupCost[nHop] = (eBW - nDiff);
                        }
                    }
                }
            } // if (NULL == ...)
            nHop++;
        } // for itrAbsLink
    } // for itr
    if (bValid)
        hSLink.modifyCost(eBW * hSLink.m_pLightpath->getCost());
    else
        hSLink.invalidate();
}

inline void NetMan::PAL2_SP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->PAL2_TearDown(this);
}

inline bool NetMan::PAL2_SP_Provision(Connection *pCon)
{
    assert(PT_PAL2_SP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_PAL_SPP, NULL);

    LINK_COST hCost = m_hWDMNet.PAL2_SP_ComputeRoute(*pCircuit, this,
                        pSrc, pDst, pCon->m_eBandwidth);
    
    if (UNREACHABLE != hCost) {
        pCircuit->PAL2_SetUp(this);
        // STEP 5: Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // STEP 6: Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    
    return (UNREACHABLE != hCost);
}

//-B: return the searched lightpath (source and destination) having the minimum cost
//	but having an amount of bandw >= than the required amount of bandw
//	(a parità di costo, prendi quello con banda libera minore)
Lightpath* NetMan::lookUpLightpathOfMinCost(OXCNode *pSrc, 
                                            OXCNode *pDst, 
                                            BandwidthGranularity nBW) const
{
    // search for existing lightpath w/ min # of WLs
    Lightpath *pCandidateLP = NULL;
    LINK_COST hMinCost = UNREACHABLE;
    list<Lightpath*>::const_iterator itr;
	for (itr = m_hLightpathDB.m_hLightpathList.begin(); itr != m_hLightpathDB.m_hLightpathList.end(); itr++) {
        if (((OXCNode*)((*itr)->getSrc()) != pSrc) || ((OXCNode*)((*itr)->getDst()) != pDst))
            continue; //-B: go to the next iteration of for loop
        if ((*itr)->getFreeCapacity() < nBW)
            continue; //-B: go to the next iteration of for loop
        LINK_COST hCost = (*itr)->getCost();
        if (hCost < hMinCost) {
            hMinCost = hCost;
            pCandidateLP = *itr;
        } else if (hCost == hMinCost) {
            if ((*itr)->getFreeCapacity() < pCandidateLP->getFreeCapacity())
                pCandidateLP = *itr;
        }
    }
    return pCandidateLP;
}

//-B: return the number of paths enumerate (for first k paths, I guess)
UINT NetMan::getNumberOfAltPaths() const
{
    return m_nNumberOfPaths;
}

//-B: return time policy: 0 = No Time; 1 = CI; 2 = PI
UINT NetMan::getTimePolicy() const
{
    return m_nTimePolicy;
}

//**************************** PAL2_SP ****************************
void NetMan::PAL2_SP_SetUpLightpath(Lightpath& hNewLP)
{
    if ((NULL == hNewLP.m_pSrc) || (NULL == hNewLP.m_pDst)) {
        AbstractLink *pLink = hNewLP.m_hPRoute.front();
        assert(pLink);
        hNewLP.m_pSrc = pLink->getSrc();
        pLink = hNewLP.m_hPRoute.back();
        assert(pLink);
        hNewLP.m_pDst = pLink->getDst();
    }

    // consume Tx/Rx
    OXCNode *pSrcOXC = (OXCNode*)(hNewLP.m_pSrc);
    OXCNode *pDstOXC = (OXCNode*)(hNewLP.m_pDst);
    assert(pSrcOXC && pDstOXC);
    assert((pSrcOXC->m_nFreeTx > 0) && (pDstOXC->m_nFreeRx > 0));
    pSrcOXC->m_nFreeTx--;
    pDstOXC->m_nFreeRx--;

    // consume wavelength along primary path
    list<UniFiber*>::iterator itr;
    for (itr=hNewLP.m_hPRoute.begin(); itr!=hNewLP.m_hPRoute.end(); itr++)
        (*itr)->consumeChannel(&hNewLP, -1);    // consume a free channel

    // update backup rcs along hBPath
    for (itr=hNewLP.m_hBRoute.begin(); itr!=hNewLP.m_hBRoute.end(); itr++) {
        UniFiber *pBFiber = (*itr);

        UINT nNewBChannels = pBFiber->m_nBChannels;
        list<UniFiber*>::const_iterator itrPFiber;
        for (itrPFiber=hNewLP.m_hPRoute.begin(); 
        itrPFiber!=hNewLP.m_hPRoute.end(); itrPFiber++) {
            UINT nPFId = (*itrPFiber)->getId();
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
    }

    // Update cost of this lightpath, backup is not counted for PAL2_SP
    hNewLP.m_hCost = hNewLP.m_hPRoute.size();
    assert(hNewLP.m_hCost >= 0);
    hNewLP.m_nLength = 1;

    // Assign unique id
    hNewLP.m_nLinkId = LightpathDB::getNextId();
    // Add to lightpath db
    m_hLightpathDB.appendLightpath(&hNewLP);
}

void NetMan::PAL2_SP_ReleaseLightpath(Lightpath& hLP, 
                                      bool& bToDelete, 
                                      const Circuit& hCircuit)
{
    assert(Circuit::CT_PAL_SPP == hCircuit.m_eCircuitType);

    hLP.m_nFreeCapacity += hCircuit.m_eBW;
    assert(hLP.m_nFreeCapacity <= hLP.m_nCapacity);
    // no connection uses me?
    if (hLP.m_nCapacity == hLP.m_nFreeCapacity) {
        hLP.logFinal(m_hLog);

        // release Tx/Rx
        OXCNode *pSrcOXC = (OXCNode*)(hLP.m_pSrc);
        OXCNode *pDstOXC = (OXCNode*)(hLP.m_pDst);
        assert(pSrcOXC && pDstOXC);
        assert((pSrcOXC->m_nFreeTx < pSrcOXC->getNumberOfTx()) 
            && (pDstOXC->m_nFreeRx < pDstOXC->getNumberOfRx()));
        pSrcOXC->m_nFreeTx++;
        pDstOXC->m_nFreeRx++;

        // update backup rcs along the backup
        list<UniFiber*>::const_iterator itr;
        for (itr=hLP.m_hBRoute.begin(); itr!=hLP.m_hBRoute.end(); itr++) {
            UniFiber *pBFiber = (*itr);

            list<UniFiber*>::const_iterator itrPFiber;
            for (itrPFiber=hLP.m_hPRoute.begin(); 
            itrPFiber!=hLP.m_hPRoute.end(); itrPFiber++) {
                pBFiber->m_pCSet[(*itrPFiber)->getId()]--;
            }
            UINT nNewBChannels = 0;
            UINT  e;
            for (e=0; e<pBFiber->m_nCSetSize; e++) {
                if (pBFiber->m_pCSet[e] > nNewBChannels)
                    nNewBChannels = pBFiber->m_pCSet[e];
            }

            if (nNewBChannels < pBFiber->m_nBChannels) {
                pBFiber->m_nBChannels--;
                pBFiber->releaseChannel(NULL);
                assert(pBFiber->m_nBChannels == nNewBChannels);
            }
        }

        // remove this lightpath
        m_hLightpathDB.removeLightpath(&hLP);
        
        // release wavelength along the primary
        // This MUST be the last thing to do.
        for (itr=hLP.m_hPRoute.begin(); itr!=hLP.m_hPRoute.end(); itr++)
            (*itr)->releaseChannel(&hLP);   // release a channel
        bToDelete = true;
    } else {
        bToDelete = false;
    }

}

void NetMan::PAL2_SetSlack(LINK_COST hCostSlack)
{
    m_hCostSlack = hCostSlack;
}

LINK_COST NetMan::PAL2_GetSlack() const
{
    return m_hCostSlack;
}

//**************************** SEG_SP_NO_HOP ****************************
inline void NetMan::SEG_SP_NO_HOP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

inline bool NetMan::SEG_SP_NO_HOP_Provision(Connection *pCon)
{
    assert(PT_SEG_SP_NO_HOP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);

    LINK_COST hCost = 
        m_hWDMNet.SEG_SP_NO_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst);
    
    if (UNREACHABLE != hCost) {
        pCircuit->setUp(this);
        // Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    
    return (UNREACHABLE != hCost);
}

//**************************** SEG_SP_AGBSP ****************************
inline void NetMan::SEG_SP_AGBSP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

inline bool NetMan::SEG_SP_AGBSP_Provision(Connection *pCon)
{
    assert(PT_SEG_SP_AGBSP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);

    LINK_COST hCost = 
        m_hWDMNet.SEG_SP_AGBSP_ComputeRoute(*pCircuit, this, pSrc, pDst);
    
    if (UNREACHABLE != hCost) {
        pCircuit->setUp(this);
        // Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    
    return (UNREACHABLE != hCost);
}

//**************************** SEG_SP_L_NO_HOP ****************************
inline void NetMan::SEG_SP_L_NO_HOP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

inline bool NetMan::SEG_SP_L_NO_HOP_Provision(Connection *pCon)
{
    assert(PT_SEG_SP_L_NO_HOP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);

    LINK_COST hCost = 
        m_hWDMNet.SEG_SP_L_NO_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst,pCon);
    
    if (UNREACHABLE != hCost) {
        pCircuit->setUp(this);
        // Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    
    return (UNREACHABLE != hCost);
}

//**************************** SEG_SPP ****************************
inline void NetMan::SEG_SPP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

//**************************** UNPROTECTED ****************************
//-B: tear down the primary circuit referenced by the connection passed as parameter
//	the tearDown method is in charge of releasing everything about the circuit/connection
inline void NetMan::UNPROTECTED_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

//**************************** wpSEG_SPP ****************************
inline void NetMan::wpSEG_SPP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->WPtearDown(this);
}

//**************************** wpUNPROTECTED ****************************
//-B: tear down the primary circuit referenced by the connection passed as parameter
inline void NetMan::wpUNPROTECTED_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->WPtearDown(this);
}

inline void NetMan::wpPAL_DPP_deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->WPtearDown(this);
}

inline void NetMan::SPPBw_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->WPtearDown(this);
}

inline void NetMan::SPPCh_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->WPtearDown(this);
}

inline void NetMan::PAL_DPP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

//-B: don't know what it does. It seems to create a new Lightpath object and put it in the attribute m_hRouteA 
//assegno m_hRouteA a partire da m_hRouteB
//HP: m_hRoute ha gli elementi della rete B(passata)
void NetMan::RouteConverter(Circuit* cir){
	list<Lightpath*>::iterator itrLP;
	list<UniFiber*>::iterator itr;
	UniFiber* fib;
	list<AbstractLink*>::iterator itrBackupLink;
	Lightpath_Seg *LPSeg;
	list<AbsPath*>::iterator itrAbsPath;
	//-B: PERCHE' A QUESTA FUNZIONE VIENE PASSATO IL VALORE 5, CHE VA AD IMPOSTARSI COME LiknId???
	//	IN TUTTI GLI ALTRI CASI IN CUI VIENE USATO IL COSTRUTTORE DI Lightpath_Seg VIENE PASSATO 0
	Lightpath_Seg* nuovoLP = new Lightpath_Seg(0); //se non lo creo con new, mi da errore all'uscita della funzione
	AbsPath* nuovoABP;
	for (itrLP = cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end(); itrLP++) //-B: m_hRoute, attribute of a Circuit object, is a Lightpath list
	{
		//-B: itero sui lightpath ddel circuito passato come parametro alla funzione
		//	Lightpath_Seg* nuovoLP=new Lightpath_Seg((*itrLP)->m_nLinkId);
		//	nuovoLP((*itrLP)->m_nLinkId);
		for (itr = (*itrLP)->m_hPRoute.begin(); itr != (*itrLP)->m_hPRoute.end(); itr++) //-B: m_PRoute, attribute of a Lightpath object, is a UniFiber list
		{	
			//-B: itero sulle fibre di un primary lightpath
			fib = (UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());
			nuovoLP->m_hPRoute.push_back(fib);	
		}
		LPSeg=(Lightpath_Seg *)(*itrLP);
		for (itrAbsPath = (LPSeg->m_hBackupSegs.begin()); itrAbsPath != (LPSeg->m_hBackupSegs.end()); itrAbsPath++)
		{
			//itero sui link di un segmento di backup						
			nuovoABP=new AbsPath();
			for(itrBackupLink=(*itrAbsPath)->m_hLinkList.begin();itrBackupLink!=(*itrAbsPath)->m_hLinkList.end();itrBackupLink++)
			{
				 fib=(UniFiber*)m_hWDMNet.lookUpLinkById((*itrBackupLink)->getId());				
				 nuovoABP->m_hLinkList.push_back(fib);								
			}
			nuovoLP->m_hBackupSegs.push_back(nuovoABP);			
		}
		nuovoLP->m_nLinkId=(*itrLP)->m_nLinkId;
		nuovoLP->wlAssigned=(*itrLP)->wlAssigned;
		nuovoLP->wlAssignedBCK=(*itrLP)->wlAssignedBCK;
		nuovoLP->backupLength=(*itrLP)->backupLength;
		nuovoLP->primaryLength=(*itrLP)->primaryLength;
	}
	cir->m_hRouteA.push_back(nuovoLP);
}

//FABIO 8 dic
void NetMan::RouteConverterUNPR(Circuit* cir){
#ifdef DEBUGB
	cout << "-> RouteConverterUNPR" << endl;
#endif // DEBUGB

	list<Lightpath*>::iterator itrLP;
	list<UniFiber*>::iterator itr;
	UniFiber* fib;
	list<AbstractLink*>::iterator itrBackupLink;
	list<AbsPath*>::iterator itrAbsPath;

	/*
	itr = pBestP1->m_hLinkList.begin();
		pUniFiber = (UniFiber*)(*itr);
		int LPCap = pUniFiber->m_pChannel->m_nCapacity;
		//Lightpath::LPProtectionType pType;
		
		// construct the circuit
		//-B: create the new Lightpath with Id = 0 and ProtectionType = LPT_PAL_Unprotected
		Lightpath_Seg *pLPSeg1 = new Lightpath_Seg(0, Lightpath::LPProtectionType::LPT_PAL_Unprotected, LPCap);
	*/
	
	itrLP = cir->m_hRoute.begin();

	//-B: PERCHE' A QUESTA FUNZIONE VIENE PASSATO IL VALORE 5, CHE VA AD IMPOSTARSI COME LiknId???
	//	IN TUTTI GLI ALTRI CASI IN CUI VIENE USATO IL COSTRUTTORE DI Lightpath_Seg VIENE PASSATO 0 -> MOD
	//-B: se non gli mettessi in input al metodo costruttore il secondo e il terzo param
	//	prenderebbe i valori di default presenti nella dichiarazione del costruttore in Lightpath_Seg.h
	Lightpath_Seg* nuovoLP = new Lightpath_Seg(0, Lightpath_Seg::LPProtectionType::LPT_PAL_Unprotected, (*itrLP)->m_nCapacity);
	
	//se non lo creo con new, mi da errore all'uscita della funzione	
  for( itrLP = cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end(); itrLP++)
  {
	//Lightpath_Seg* nuovoLP = new Lightpath_Seg((*itrLP)->m_nLinkId);
	// nuovoLP((*itrLP)->m_nLinkId);
	for(itr = (*itrLP)->m_hPRoute.begin() ; itr != (*itrLP)->m_hPRoute.end() ; itr++)
	{	
		fib = (UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());
		(*itr)->m_used = 1;  //bit utilizzo
		nuovoLP->m_hPRoute.push_back(fib);	
	}
	nuovoLP->m_nLinkId = (*itrLP)->m_nLinkId;
	nuovoLP->wlAssigned = (*itrLP)->wlAssigned;
	nuovoLP->backupLength = (*itrLP)->backupLength;
	nuovoLP->primaryLength = (*itrLP)->primaryLength;
	nuovoLP->m_hCost = (*itrLP)->m_hCost;
  }
  cir->m_hRouteA.push_back(nuovoLP);
}

//-B: Something related to conflicts. I don't need it
NetMan::ConflictType NetMan::wpverifyCircuit(Circuit* cir){
	
	list<UniFiber*>::iterator itr;
	list<Lightpath*>::iterator itrLP;
	Lightpath_Seg *LPSeg;
	list<AbsPath*>::iterator itrAbsPath;
	list<AbstractLink*>::iterator itrBackupLink;
	UniFiber*	uFiber;
	UniFiber* fib;
	bool pConf=false;
	bool bConf=false;
	if(isLinkDisjointActive)
		{
			for( itrLP=cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end();itrLP++)
	{
		
		for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
		{
		fib=(UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());//ricavo la corrispondente fibra della rete originale
			if (fib->wlOccupation[(*itrLP)->wlAssigned]==UNREACHABLE)//fibra occupata
				pConf=true;
			
		}

		
		//CONTROLLO CONFLITTI BACKUP

			LPSeg=static_cast<Lightpath_Seg*> (*itrLP);//Verificare static cast...
		
		//Itero sui segmenti di backup
		for(itrAbsPath=(LPSeg->m_hBackupSegs.begin());itrAbsPath != (LPSeg->m_hBackupSegs.end());itrAbsPath++)
		{
				//itero sui link di un segmento di backup (rete PAST)
			for(itrBackupLink=(*itrAbsPath)->m_hLinkList.begin();itrBackupLink!=(*itrAbsPath)->m_hLinkList.end();itrBackupLink++)
			{
				uFiber=static_cast<UniFiber*>(m_hWDMNet.lookUpLinkById((*itrBackupLink)->getId()));
				//uFiber: fibra di backup della rete ORIGINALE
				if(uFiber->m_pChannel[LPSeg->wlAssignedBCK].m_bFree==false)
				{
					if(uFiber->m_pChannel[LPSeg->wlAssignedBCK].m_backup==false)
						bConf=true;
					else //c'è un protection; devo verificare se è posibile la condivisione
					{
					//	for (int i=0; i<LPSeg->primaryLinkID.size();i++)
							for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
						{
							//verifico se sto gia proteggendo
						if(uFiber->m_pChannel[LPSeg->wlAssignedBCK].linkProtected[(*itr)->getId()]==true)
							bConf=true;
						}
					}

				}
			}
		}
		}
			}
	else
		{
		//CONTROLLO CONFLITTI PRIMARIO
		for( itrLP=cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end();itrLP++)
	{
		
		for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
		{
		fib=(UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());//ricavo la corrispondente fibra della rete originale
			if (fib->wlOccupation[(*itrLP)->wlAssigned]==UNREACHABLE)//fibra occupata
				pConf=true;
			
		}


		//CONTROLLO CONFLITTI BACKUP

			LPSeg=static_cast<Lightpath_Seg*> (*itrLP);//Verificare static cast...
		
		//Itero sui segmenti di backup
		for(itrAbsPath=(LPSeg->m_hBackupSegs.begin());itrAbsPath != (LPSeg->m_hBackupSegs.end());itrAbsPath++)
		{
				//itero sui link di un segmento di backup (rete PAST)
			for(itrBackupLink=(*itrAbsPath)->m_hLinkList.begin();itrBackupLink!=(*itrAbsPath)->m_hLinkList.end();itrBackupLink++)
			{
				uFiber=static_cast<UniFiber*>(m_hWDMNet.lookUpLinkById((*itrBackupLink)->getId()));
				//uFiber: fibra di backup della rete ORIGINALE
				if(uFiber->m_pChannel[LPSeg->wlAssignedBCK].m_bFree==false)
				{
					if(uFiber->m_pChannel[LPSeg->wlAssignedBCK].m_backup==false)
						bConf=true;
					else //c'è un protection; devo verificare se è posibile la condivisione
					{
							if((*itrLP)->m_hPRoute.size() == 1) //attraversa un hop
						{
								//for (int i=0; i<LPSeg->primaryNodesID.size();i++)//LO TOLGO PERCHE PRIMARYNODES NON E ANCORA ASSSEGNATO!
						for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
						{
						if(uFiber->m_pChannel[LPSeg->wlAssignedBCK].nodesProtected[(*itr)->getSrc()->getId()]==true)
							bConf=true;
						}
							}
							else //attraversa piu hop
							
							{
								list<UniFiber*>::const_iterator itrP;
								list<UniFiber*>::const_iterator itrPIndex = (*itrLP)->m_hPRoute.begin();
								itrPIndex++;
									for(itrP=itrPIndex;itrP !=(*itrLP)->m_hPRoute.end();itrP++)
						{
						if(uFiber->m_pChannel[LPSeg->wlAssignedBCK].nodesProtected[(*itrP)->getSrc()->getId()]==true)
							bConf=true;
						}
								}//ELSE piu hop
					
							
							}//END else protection

				}
			}
		}//END itrLP?
		}//END NODE DISJOINT

		}
		if(pConf&&bConf)
			return BOTH_Conflict;
		else if(pConf && (!bConf))
 return PRIMARY_Conflict;
else if(bConf && (!pConf))
return BACKUP_Conflict;
else
return NO_Conflict;//CONTROLLO OK :-)

}

NetMan::ConflictType NetMan::wpverifyCircuitDED(Circuit* cir){
	
	list<UniFiber*>::iterator itr;
	list<Lightpath*>::iterator itrLP;
	Lightpath_Seg *LPSeg;
	list<AbsPath*>::iterator itrAbsPath;
	list<AbstractLink*>::iterator itrBackupLink;
	UniFiber*	uFiber;
	UniFiber* fib;

	
			for( itrLP=cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end();itrLP++)
	{
		
		for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
		{
		fib=(UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());//ricavo la corrispondente fibra della rete originale
			if (fib->wlOccupation[(*itrLP)->wlAssigned]==UNREACHABLE)//fibra occupata
				return PRIMARY_Conflict;
			
		}

		
		//CONTROLLO CONFLITTI BACKUP

			LPSeg=static_cast<Lightpath_Seg*> (*itrLP);//Verificare static cast...
		
		//Itero sui segmenti di backup
		for(itrAbsPath=(LPSeg->m_hBackupSegs.begin());itrAbsPath != (LPSeg->m_hBackupSegs.end());itrAbsPath++)
		{
				//itero sui link di un segmento di backup (rete PAST)
			for(itrBackupLink=(*itrAbsPath)->m_hLinkList.begin();itrBackupLink!=(*itrAbsPath)->m_hLinkList.end();itrBackupLink++)
			{
				uFiber=static_cast<UniFiber*>(m_hWDMNet.lookUpLinkById((*itrBackupLink)->getId()));
				//uFiber: fibra di backup della rete ORIGINALE
				if(uFiber->m_pChannel[LPSeg->wlAssignedBCK].m_bFree==false)
				{
					return PRIMARY_Conflict;

				}
			}
		}
		}
			
	
	return NO_Conflict;
}

NetMan::ConflictType NetMan::wpverifyCircuitUNPR(Circuit* cir){
	
	list<UniFiber*>::iterator itr;
	list<Lightpath*>::iterator itrLP;
	
	list<AbsPath*>::iterator itrAbsPath;
	list<AbstractLink*>::iterator itrBackupLink;
	
	UniFiber* fib;
	//CONTROLLO CONFLITTI PRIMARIO
	for( itrLP = cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end(); itrLP++)
	{
		for(itr = (*itrLP)->m_hPRoute.begin(); itr != (*itrLP)->m_hPRoute.end(); itr++)
		{
			fib = (UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId()); //ricavo la corrispondente fibra della rete originale
			if (fib->wlOccupation[(*itrLP)->wlAssigned] == UNREACHABLE) //fibra occupata
			{		
				return PRIMARY_Conflict;
			}	
		}
	}
	return NO_Conflict;
}

//-B: Something related to conflicts. I don't need it
NetMan::ConflictType NetMan::verifyCircuit(Circuit* cir){
//	NOTA : A QUESTO PUNTO HO GIA RISISTEMATO LE CONNESSIONI TIRATE GIU TEMPORANEAMENTE...					

	
	list<UniFiber*>::iterator itr;
	list<Lightpath*>::iterator itrLP;
	Lightpath_Seg *LPSeg;
	list<AbsPath*>::iterator itrAbsPath;
	list<AbstractLink*>::iterator itrBackupLink;
	UniFiber*	uFiber;
	UniFiber* fib;
	bool pConf=false;
	bool bConf=false;
	//PASSO1 :Verifica conflitti primario-primario
	//STRATEGIA: controllo se su tutti i link del primario da verificare ho almeno un canale libero
	if(isLinkDisjointActive)
		{	for( itrLP=cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end();itrLP++)
	{
		
		for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
		{
		fib=(UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());//ricavo la corrispondente fibra della rete originale
			if (fib->countFreeChannels() ==0)
				//return PRIMARY_Conflict;
				{
				pConf=true;
				break;
			}
		}
	
	//CONTROLLO PRIMARIO-BACKUP : DOVREBBE GIA ESSERE INCLUSO IN CASO SOPRA

	//CONTROLLO BACKUP


					//List <lightpath*>
/*	for( itrLP= (cir->m_hRoute.begin());(itrLP != cir->m_hRoute.end());itrLP++)
	
	{*/
		LPSeg=static_cast<Lightpath_Seg*> (*itrLP);//Verificare static cast...
		
		//Itero sui segmenti di backup
		for(itrAbsPath=(LPSeg->m_hBackupSegs.begin());itrAbsPath != (LPSeg->m_hBackupSegs.end());itrAbsPath++)
		{
				//itero sui link di un segmento di backup
			for(itrBackupLink=(*itrAbsPath)->m_hLinkList.begin();itrBackupLink!=(*itrAbsPath)->m_hLinkList.end();itrBackupLink++)
			{
				uFiber=static_cast<UniFiber*>(m_hWDMNet.lookUpLinkById((*itrBackupLink)->getId()));
				;
				//Step1:verifico se esiste almeno un canale di protezione
		if(uFiber->m_nBChannels>0)
		{	
					//Esiste ALMENO un canale di protezione..ora verifico se il link è shareable
					//o se ci sarà bisogno di allocare nuovi canali

			
				list<UniFiber*>::const_iterator itrP;
					for (itrP=(*itrLP)->m_hPRoute.begin(); itrP!=(*itrLP)->m_hPRoute.end() ; itrP++)
						{//scorro i link del working
							UINT nPLinkId = (*itrP)->getId();//ne prendo il nodo src

							if(uFiber->m_pCSet[nPLinkId] ==uFiber->m_nBChannels)
								{//è necessario allocare nuovo canale di protezione; è possibile?
									if (uFiber->countFreeChannels() ==0)
									//return BACKUP_Conflict;
										{
				bConf=true;
				break;
			}
								}
			

						}//fine-FOR; iterazioni sui link del primario

				
		}
				else // NON c'è canale di protezione..verifico se avrò lo spazio per allocarne
				{	// uno nuovo, se no ho conflitto
				if (uFiber->countFreeChannels() ==0)
				//return BACKUP_Conflict; //
					{
				bConf=true;
				break;
			}
				}

			}
		}
	}
		}
		else{
	for( itrLP=cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end();itrLP++)
	{
		
		for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
		{
		fib=(UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());//ricavo la corrispondente fibra della rete originale
			if (fib->countFreeChannels() ==0)
				//return PRIMARY_Conflict;
					{
				pConf=true;
				break;
			}
			
		}
	
	//CONTROLLO PRIMARIO-BACKUP : DOVREBBE GIA ESSERE INCLUSO IN CASO SOPRA

	//CONTROLLO BACKUP


					//List <lightpath*>
/*	for( itrLP= (cir->m_hRoute.begin());(itrLP != cir->m_hRoute.end());itrLP++)
	
	{*/
		LPSeg=static_cast<Lightpath_Seg*> (*itrLP);//Verificare static cast...
		
		//Itero sui segmenti di backup
		for(itrAbsPath=(LPSeg->m_hBackupSegs.begin());itrAbsPath != (LPSeg->m_hBackupSegs.end());itrAbsPath++)
		{
				//itero sui link di un segmento di backup
			for(itrBackupLink=(*itrAbsPath)->m_hLinkList.begin();itrBackupLink!=(*itrAbsPath)->m_hLinkList.end();itrBackupLink++)
			{
				uFiber=static_cast<UniFiber*>(m_hWDMNet.lookUpLinkById((*itrBackupLink)->getId()));
				;
				//Step1:verifico se esiste almeno un canale di protezione
		if(uFiber->m_nBChannels>0)
		{	
					//Esiste ALMENO un canale di protezione..ora verifico se il link è shareable
					//o se ci sarà bisogno di allocare nuovi canali

				
	     	if((*itrLP)->m_hPRoute.size() == 1) //attraversa un hop
			{
				list<UniFiber*>::const_iterator itrP;
				list<UniFiber*>::const_iterator itrPIndex = (*itrLP)->m_hPRoute.begin();
			for (itrP=itrPIndex; itrP!=(*itrLP)->m_hPRoute.end() ; itrP++)
				{//scorro i link del working
					UINT nPNodeId = (*itrP)->getSrc()->getId();//ne prendo il nodo src

					if(uFiber->m_pCSet[nPNodeId] ==uFiber->m_nBChannels)
						{//è necessario allocare nuovo canale di protezione; è possibile?
						if (uFiber->countFreeChannels() ==0)
						//	return BACKUP_Conflict;
							{
				bConf=true;
				break;
			}
						
						}
				
				}//fine-FOR; iterazioni sui link del primario
			}
				else
				{//primario attraversa piu hop
					list<UniFiber*>::const_iterator itrP;
					list<UniFiber*>::const_iterator itrPIndex = (*itrLP)->m_hPRoute.begin();
					itrPIndex++;	// salto il primo link..
					for (itrP=itrPIndex; itrP!=(*itrLP)->m_hPRoute.end() ; itrP++)
						{//scorro i link del working
							UINT nPNodeId = (*itrP)->getSrc()->getId();//ne prendo il nodo src

							if(uFiber->m_pCSet[nPNodeId] ==uFiber->m_nBChannels)
								{//è necessario allocare nuovo canale di protezione; è possibile?
									if (uFiber->countFreeChannels() ==0)
								//	return BACKUP_Conflict;
									{
				pConf=true;
				break;
			}
						
								}
			

						}//fine-FOR; iterazioni sui link del primario

				}//else piu hop
		}
				else // NON c'è canale di protezione..verifico se avrò lo spazio per allocarne
				{	// uno nuovo, se no ho conflitto
				if (uFiber->countFreeChannels() ==0)
				//return BACKUP_Conflict; //
				{
				bConf=true;
				break;
			}
				}

			}
		}
	}

}//END NODE DIS
		if(pConf&&bConf)
			return BOTH_Conflict;
		else if(pConf && (!bConf))
 return PRIMARY_Conflict;
else if(bConf && (!pConf))
return BACKUP_Conflict;
else
return NO_Conflict;//CONTROLLO OK :-)
}


NetMan::ConflictType NetMan::verifyCircuitUNPR(Circuit* cir){
//	NOTA : A QUESTO PUNTO HO GIA RISISTEMATO LE CONNESSIONI TIRATE GIU TEMPORANEAMENTE...					

	
	list<UniFiber*>::iterator itr;
	list<Lightpath*>::iterator itrLP;
	
	list<AbsPath*>::iterator itrAbsPath;
	list<AbstractLink*>::iterator itrBackupLink;

	UniFiber* fib;
	
	//PASSO1 :Verifica conflitti primario-primario
//STRATEGIA: controllo se su tutti i link del primario da verificare ho almeno un canale libero
	for( itrLP=cir->m_hRoute.begin(); itrLP != cir->m_hRoute.end();itrLP++)
	{
		
		if((*itrLP)->wlAssigned == -1)
		{//-B: se non è wavel continuous
			for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
			{
				//ricavo la corrispondente fibra della rete originale
				fib = (UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());
				if (fib->countFreeChannels() == 0)
					return PRIMARY_Conflict;
			}
		}//end if
		else
		{ //-B: se è wavel continuous
			for(itr=(*itrLP)->m_hPRoute.begin();itr !=(*itrLP)->m_hPRoute.end();itr++)
			{
				//ricavo la corrispondente fibra della rete originale
				fib = (UniFiber*)m_hWDMNet.lookUpLinkById((*itr)->getId());
				if ((fib->wlOccupation[(*itrLP)->wlAssigned]) == UNREACHABLE)
					return PRIMARY_Conflict;
				else
					assert(fib->m_pChannel[(*itrLP)->wlAssigned].m_bFree == true);
			}
		}//end else
	//CONTROLLO SOLO X IL PRIMARIO IN QUESTO CASO dato che e' UNPROTECTED
	}//end for


return NO_Conflict; //CONTROLLO OK :-)
}

//**************************** SPPCh ****************************
inline bool NetMan::SPPCh_Provision(Connection *pCon)
{
    assert(PT_SPPCh == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);//LEAK!




    LINK_COST hCost = 
     //   m_hWDMNet.SEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
		m_hWDMNetPast.SPPCh_ComputeRoute(*pCircuit, this, pSrc, pDst);
    if (UNREACHABLE != hCost) {

		pCircuit->m_hRouteB=pCircuit->m_hRoute;
			
		ConflictType verifyCode=wpverifyCircuit(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito
		//in realta si sovrappone ai circuiti gia esistenti

		if(verifyCode==NO_Conflict)
		{
			RouteConverter(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
/*		m_hWDMNet.dump(cout);
		cout<<"======================================";
		m_hWDMNetPast.dump(cout);*/
		
		pCircuit->m_eState=Circuit::CT_Setup;
        pCircuit->WPsetUp(this);
 // Associate the circuit w/ the connection
		
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
/*
		if( m_hWDMNet.ConIdReq==true){
		m_hWDMNet.dbggap.push_back(pCon->m_nSequenceNo);//insersco la prima connessione riuscita dopo i fallimenti
		int next=(m_hLightpathDB.peekNextId());
			next--;
		m_hWDMNet.truecon.push_back	(next);
		m_hWDMNet.ConIdReq=false;
		}*/
		}
		else
		{
		
			m_hWDMNet.ConIdReq=true;
			
			if (verifyCode==PRIMARY_Conflict)
			{
	//			cout<<"\nConflitto sul primario per connessione "<< pCon->m_nSequenceNo;
			pc++;//contatore conflitti su primario
			}
			else if (verifyCode==BACKUP_Conflict)
			{
	//			cout<<"\nConflitto sul backup per connessione "<< pCon->m_nSequenceNo;
				bc++;//contatore conflitti backup
			}
			else bothc++;

			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
				delete (*itLP);
			 delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
		hCost=UNREACHABLE;//da fare:porre stato dropped o cose simili...
		}
       


    } else {
        // see if it can be provisioned using segment protection
       
       
		pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
     pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
	
 LINK_COST hSegCost =
          //  m_hWDMNet.SEG_SP_NO_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst);FABIO: elimino momentaneamente..
			  m_hWDMNet.SPPCh_ComputeRoute(*pCircuit, this, pSrc, pDst);//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata


        if (UNREACHABLE != hSegCost) {
          //  m_hLog.m_nBlockedConDueToPath++;
           // CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			//FABIO 13 nov: 
			fc++;//FAKE CONFLICT
        }
			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
			delete (*itLP);
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
	
		m_hWDMNet.ConIdReq=true;//cosi non inserisco 2 volte consecutive in dbggap 
		
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
	 m_hWDMNetPast.restoreLinkCost();
    m_hWDMNetPast.validateAllLinks();
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}

inline bool NetMan::SPPBw_Provision(Connection *pCon)
{
    assert(PT_SPPBw == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);//LEAK!




    LINK_COST hCost = 
     //   m_hWDMNet.SEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
		m_hWDMNetPast.SPPBw_ComputeRoute(*pCircuit, this, pSrc, pDst);
    if (UNREACHABLE != hCost) {

		pCircuit->m_hRouteB=pCircuit->m_hRoute;
			
		ConflictType verifyCode=wpverifyCircuit(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito
		//in realta si sovrappone ai circuiti gia esistenti

		if(verifyCode==NO_Conflict)
		{
			RouteConverter(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
/*		m_hWDMNet.dump(cout);
		cout<<"======================================";
		m_hWDMNetPast.dump(cout);*/
		
		pCircuit->m_eState=Circuit::CT_Setup;
        pCircuit->WPsetUp(this);
 // Associate the circuit w/ the connection
		
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
/*
		if( m_hWDMNet.ConIdReq==true){
		m_hWDMNet.dbggap.push_back(pCon->m_nSequenceNo);//insersco la prima connessione riuscita dopo i fallimenti
		int next=(m_hLightpathDB.peekNextId());
			next--;
		m_hWDMNet.truecon.push_back	(next);
		m_hWDMNet.ConIdReq=false;
		}*/
		}
		else
		{
		
			m_hWDMNet.ConIdReq=true;
			
			if (verifyCode==PRIMARY_Conflict)
			{
	//			cout<<"\nConflitto sul primario per connessione "<< pCon->m_nSequenceNo;
			pc++;//contatore conflitti su primario
			}
			else if (verifyCode==BACKUP_Conflict)
			{
	//			cout<<"\nConflitto sul backup per connessione "<< pCon->m_nSequenceNo;
				bc++;//contatore conflitti backup
			}
			else bothc++;

			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
				delete (*itLP);
			 delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
		hCost=UNREACHABLE;//da fare:porre stato dropped o cose simili...
		}
       


    } else {
        // see if it can be provisioned using segment protection
       
       
		pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
     pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
	
 LINK_COST hSegCost =
          //  m_hWDMNet.SEG_SP_NO_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst);FABIO: elimino momentaneamente..
			  m_hWDMNet.SPPBw_ComputeRoute(*pCircuit, this, pSrc, pDst);//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata


        if (UNREACHABLE != hSegCost) {
          //  m_hLog.m_nBlockedConDueToPath++;
           // CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			//FABIO 13 nov: 
			fc++;//FAKE CONFLICT
        }
			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
			delete (*itLP);
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
	
		m_hWDMNet.ConIdReq=true;//cosi non inserisco 2 volte consecutive in dbggap 
		
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
	 m_hWDMNetPast.restoreLinkCost();
    m_hWDMNetPast.validateAllLinks();
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}

inline bool NetMan::SEG_SPP_Provision(Connection *pCon)
{
    assert(PT_SEG_SPP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);//LEAK!




    LINK_COST hCost = 
     //   m_hWDMNet.SEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
		m_hWDMNetPast.SEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
    if (UNREACHABLE != hCost) {

		pCircuit->m_hRouteB=pCircuit->m_hRoute;
			
		ConflictType verifyCode=verifyCircuit(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito
		//in realta si sovrappone ai circuiti gia esistenti

		if(verifyCode==NO_Conflict)
		{
			RouteConverter(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
/*		m_hWDMNet.dump(cout);
		cout<<"======================================";
		m_hWDMNetPast.dump(cout);*/
		
		pCircuit->m_eState=Circuit::CT_Setup;
        pCircuit->setUp(this);
 // Associate the circuit w/ the connection
		
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
/*
		if( m_hWDMNet.ConIdReq==true){
		m_hWDMNet.dbggap.push_back(pCon->m_nSequenceNo);//insersco la prima connessione riuscita dopo i fallimenti
		int next=(m_hLightpathDB.peekNextId());
			next--;
		m_hWDMNet.truecon.push_back	(next);
		m_hWDMNet.ConIdReq=false;
		}*/
		}
		else
		{
		
			m_hWDMNet.ConIdReq=true;
			
			if (verifyCode==PRIMARY_Conflict)
			{
	//			cout<<"\nConflitto sul primario per connessione "<< pCon->m_nSequenceNo;
			pc++;//contatore conflitti su primario
			}
			else if (verifyCode==BACKUP_Conflict)
			{
	//			cout<<"\nConflitto sul backup per connessione "<< pCon->m_nSequenceNo;
				bc++;//contatore conflitti backup
			}
			else bothc++;

			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
				delete (*itLP);
			 delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
		hCost=UNREACHABLE;//da fare:porre stato dropped o cose simili...
		}
       


    } else {
        // see if it can be provisioned using segment protection
       
       
		pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
     pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
	
 LINK_COST hSegCost =
          //  m_hWDMNet.SEG_SP_NO_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst);FABIO: elimino momentaneamente..
			  m_hWDMNet.SEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata


        if (UNREACHABLE != hSegCost) {
          //  m_hLog.m_nBlockedConDueToPath++;
           // CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			//FABIO 13 nov: 
			fc++;//FAKE CONFLICT
        }
			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
			delete (*itLP);
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
	
		m_hWDMNet.ConIdReq=true;//cosi non inserisco 2 volte consecutive in dbggap 
		
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
	 m_hWDMNetPast.restoreLinkCost();
    m_hWDMNetPast.validateAllLinks();
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}

inline bool NetMan::wpSEG_SPP_Provision(Connection *pCon)
{
    assert(PT_wpSEG_SPP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);//LEAK!



    LINK_COST hCost = 
     //   m_hWDMNet.SEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);

		m_hWDMNetPast.wpSEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
    if (UNREACHABLE != hCost) {

		pCircuit->m_hRouteB=pCircuit->m_hRoute;
			
		ConflictType verifyCode=wpverifyCircuit(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito
		//in realta si sovrappone ai circuiti gia esistenti

		if(verifyCode==NO_Conflict)
		{
			RouteConverter(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
/*
		m_hWDMNet.dump(cout);
		cout<<"======================================";
		m_hWDMNetPast.dump(cout);
		*/
		pCircuit->m_eState=Circuit::CT_Setup;
        pCircuit->WPsetUp(this);
 // Associate the circuit w/ the connection
	/*	
		m_hWDMNet.dump(cout);
		cout<<"======================================";
		m_hWDMNetPast.dump(cout);
*/
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;

		if( m_hWDMNet.ConIdReq==true){
		m_hWDMNet.dbggap.push_back(pCon->m_nSequenceNo);//insersco la prima connessione riuscita dopo i fallimenti
		int next=(m_hLightpathDB.peekNextId());
			next--;
		m_hWDMNet.truecon.push_back	(next);
		m_hWDMNet.ConIdReq=false;
		}
		}
		else
		{
		
			m_hWDMNet.ConIdReq=true;
			
			if (verifyCode==PRIMARY_Conflict)
			{
	//			cout<<"\nConflitto sul primario per connessione "<< pCon->m_nSequenceNo;
			pc++;//contatore conflitti su primario
			}
			else if(verifyCode==BACKUP_Conflict)
			{
	//			cout<<"\nConflitto sul backup per connessione "<< pCon->m_nSequenceNo;
				bc++;//contatore conflitti backup
	//			m_hWDMNet.dump(cout);
	//			ConflictType verifyCode=verifyCircuit(pCircuit);
			}
			else
				bothc++;

			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
				delete (*itLP);
			 delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
		hCost=UNREACHABLE;//da fare:porre stato dropped o cose simili...
		}
       


    } else {
        // see if it can be provisioned using segment protection
       
       
		pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
     pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
	
 LINK_COST hSegCost =
          //  m_hWDMNet.SEG_SP_NO_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst);FABIO: elimino momentaneamente..
			  m_hWDMNet.wpSEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata


        if (UNREACHABLE != hSegCost) {
          //  m_hLog.m_nBlockedConDueToPath++;
           // CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			//FABIO 13 nov: 
			fc++;//FAKE CONFLICT
        }
			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
			delete (*itLP);
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
	
		m_hWDMNet.ConIdReq=true;//cosi non inserisco 2 volte consecutive in dbggap 
		
    }

    // Postprocess: validate those links that have been invalidated temp
 //   m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
//	 m_hWDMNetPast.restoreLinkCost();
    m_hWDMNetPast.validateAllLinks();
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}

//**************************** UNPROTECTED ****************************
//	ATTENTION: this method uses a WDMNetwork object created specifically in NetMan class
//Fabio 9 dic
inline bool NetMan::UNPROTECTED_Provision(Connection *pCon)
{
    assert(PT_UNPROTECTED == m_eProvisionType); // verifica che sia stato scelto il tipo di protezione UNPROTECTED
    OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc); //salva nodo sorgente della connessione pCon (passata) in pSrc
    OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst); //salva nodo destinazione della connessione pCon (passata) in pDst
    assert(pSrc && pDst);
	Circuit *pCircuit = new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL); //LEAK!
	LINK_COST hCost = m_hWDMNetPast.UNPROTECTED_ComputeRoute(*pCircuit, this, pSrc, pDst);
	if (UNREACHABLE != hCost){
		transport_cost = hCost;
	} else {
		transport_cost=0;
	}
	if (UNREACHABLE != hCost) {
		pCircuit->m_hRouteB = pCircuit->m_hRoute;
		ConflictType verifyCode = verifyCircuitUNPR(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito in realtà si sovrappone ai circuiti gia esistenti
		if(verifyCode == NO_Conflict) 
		{
			RouteConverterUNPR(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
			/*m_hWDMNet.dump(cout);
			cout<<"======================================";
			m_hWDMNetPast.dump(cout);*/
			pCircuit->m_eState=Circuit::CT_Setup;
			pCircuit->setUp(this);
			/*m_hWDMNet.dump(cout);
			cout<<"======================================";
			m_hWDMNetPast.dump(cout);*/
			// Associate the circuit w/ the connection
	        pCon->m_pPCircuit = pCircuit;
	        // Update status
		    pCon->m_eStatus = Connection::SETUP;
		} else{
			if (verifyCode == PRIMARY_Conflict)
			{
				pc++;//contatore conflitti su primario
				//cout<<"PC:"<<pCon->m_nSequenceNo;
			}
			list<Lightpath*>::iterator itLP;
			//for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
			//MODIFICA OGGI			delete (*itLP); 
			delete pCircuit;
			pCon->m_eStatus = Connection::DROPPED;
			hCost = UNREACHABLE; //da fare:porre stato dropped o cose simili...
		}
	} else { //-B: if hCost == UNREACHABLE
		// see if it can be provisioned using segment protection
		pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
		pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
		LINK_COST hSegCost = m_hWDMNet.UNPROTECTED_ComputeRoute(*pCircuit, this, pSrc, pDst); //fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata
        if (UNREACHABLE != hSegCost) {
			//  m_hLog.m_nBlockedConDueToPath++;
			// CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			fc++; //FAKE CONFLICT //FABIO 13 nov
			//cout<<"FS:"<<pCon->m_nSequenceNo;
        }
		//	else cout<<"CapCON:"<<pCon->m_nSequenceNo;
		list<Lightpath*>::iterator itLP;
		//for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
		//MODIFICA OGGI		delete (*itLP);
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
		m_hWDMNet.ConIdReq=true;//cosi non inserisco 2 volte consecutive in dbggap 
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();		//-B: for each link of m_hLinkList set the m_hCost var equal to the mHSavedCost
	m_hWDMNet.validateAllLinks();		//-B: for each link of m_hLinkList set the m_bValid var equal to true
	m_hWDMNet.validateAllNodes();		//-B: for each node of m_hNodeList set the m_bValid var equal to true
	m_hWDMNetPast.restoreLinkCost();
    m_hWDMNetPast.validateAllLinks();	// } same things as above, but for WDMNetPast object
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}

 //**************************** wpUNPROTECTED ****************************
//-B: return true if the method can provide a route with an admissible cost -> computed by ComputeRoute method
inline bool NetMan::wpUNPROTECTED_Provision(Connection *pCon)
{
	assert(PT_wpUNPROTECTED == m_eProvisionType);
	OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc);
	OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst);
	assert(pSrc && pDst);
	Circuit *pCircuit = new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL); //LEAK!
	LINK_COST hCost = m_hWDMNetPast.wpUNPROTECTED_ComputeRoute(*pCircuit, this, pSrc, pDst);

#ifdef DEBUGB
	cout << "Ho appena eseguito ComputeRoute -> hCost: " << hCost << endl;
#endif // DEBUGB

	if (UNREACHABLE != hCost)
	{
		//pathTotCost+=hCost;
		//cout<<endl<<"Cost:"<<pathTotCost<<endl<<endl;
		pCircuit->m_hRouteB = pCircuit->m_hRoute;
		ConflictType verifyCode = wpverifyCircuitUNPR(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito
		//in realta si sovrappone ai circuiti gia esistenti
		if (verifyCode == NO_Conflict)
		{
			RouteConverterUNPR(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
			/*m_hWDMNet.dump(cout);
			cout<<"======================================";
			m_hWDMNetPast.dump(cout);*/
			pCircuit->m_eState = Circuit::CT_Setup;
			pCircuit->WPsetUp(this);
			/*m_hWDMNet.dump(cout);
			cout<<"======================================";
			m_hWDMNetPast.dump(cout);*/
			// Associate the circuit w/ the connection
			pCon->m_pPCircuit = pCircuit;
			// Update status
			pCon->m_eStatus = Connection::SETUP;
		}
		else {
			if (verifyCode == PRIMARY_Conflict)
			{
				pc++;//contatore conflitti su primario
			}
			list<Lightpath*>::iterator itLP;
			//for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
			//MODIFICA OGGI			delete (*itLP); 
			delete pCircuit;
			pCon->m_eStatus = Connection::DROPPED;
			hCost = UNREACHABLE;//da fare:porre stato dropped o cose simili...
		}
	}else { //-B: if hCost == UNREACHABLE
		// see if it can be provisioned using segment protection
		/* pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
		pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
		cout<<"Connessione fallita tra "<< pCon->m_nSrc<<" " <<pCon->m_nDst<<endl;
		m_hWDMNet.dump(cout);
		cout<<"Segue rete passata"<<endl;
		m_hWDMNet.dump(cout);*/
		LINK_COST hSegCost = m_hWDMNet.wpUNPROTECTED_ComputeRoute(*pCircuit, this, pSrc, pDst);//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata
		//dump(cout);
		if (UNREACHABLE != hSegCost) {
			//  m_hLog.m_nBlockedConDueToPath++;
			 // CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			  //FABIO 13 nov: 
			fc++;//FAKE CONFLICT
		}
		list<Lightpath*>::iterator itLP;
		//for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
		//MODIFICA OGGI		delete (*itLP);
		delete pCircuit;
		pCon->m_eStatus = Connection::DROPPED;
		m_hWDMNet.ConIdReq = true;//cosi non inserisco 2 volte consecutive in dbggap 
	}
	// Postprocess: validate those links that have been invalidated temp
	m_hWDMNet.restoreLinkCost(); //-B: for each link of m_hLinkList set the m_hCost var equal to the mHSavedCost
	m_hWDMNet.validateAllLinks(); //-B: for each link of m_hLinkList set the m_bValid var equal to true
	m_hWDMNet.validateAllNodes(); //-B: for each node of m_hNodeList set the m_bValid var equal to true
	m_hWDMNetPast.restoreLinkCost();
	m_hWDMNetPast.validateAllLinks();			// }same things as above, but for hWDMNetPast object
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}

//**************************** PAL_DPP ****************************
//8 dic:..molto simile a caso unprotected...
inline bool NetMan::PAL_DPP_Provision(Connection *pCon)
{
    assert(PT_PAL_DPP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);//LEAK!
												//da modificare



    LINK_COST hCost = 
     //   m_hWDMNet.SEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
		m_hWDMNetPast.PAL_DPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
    if (UNREACHABLE != hCost) {

		pCircuit->m_hRouteB=pCircuit->m_hRoute;
			
		ConflictType verifyCode=verifyCircuitUNPR(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito
		//in realta si sovrappone ai circuiti gia esistenti

		if(verifyCode==NO_Conflict)
		{
			RouteConverterUNPR(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
/*		m_hWDMNet.dump(cout);
		cout<<"======================================";
		m_hWDMNetPast.dump(cout);*/
		
		pCircuit->m_eState=Circuit::CT_Setup;
        pCircuit->setUp(this);
 // Associate the circuit w/ the connection
		
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;

		}
		else{
		
					
			if (verifyCode==PRIMARY_Conflict)
			{
				pc++;//contatore conflitti su primario
			}
			
		
			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
				delete (*itLP);
			 delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
		hCost=UNREACHABLE;//da fare:porre stato dropped o cose simili...
		}
       


    } else {
        // see if it can be provisioned using segment protection
       
 
		pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
     pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
	
 LINK_COST hSegCost =
          //  m_hWDMNet.SEG_SP_NO_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst);FABIO: elimino momentaneamente..
			  m_hWDMNet.PAL_DPP_ComputeRoute(*pCircuit, this, pSrc, pDst);//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata


        if (UNREACHABLE != hSegCost) {
          //  m_hLog.m_nBlockedConDueToPath++;
           // CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			//FABIO 13 nov: 
			fc++;//FAKE CONFLICT
        }
			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
			delete (*itLP);
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
	
		m_hWDMNet.ConIdReq=true;//cosi non inserisco 2 volte consecutive in dbggap 
		
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
	 m_hWDMNetPast.restoreLinkCost();
    m_hWDMNetPast.validateAllLinks();
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}//////////////////////

inline bool NetMan::wpPAL_DPP_Provision(Connection *pCon)
{
    assert(PT_wpPAL_DPP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

	Circuit *pCircuit =
		new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL); //LEAK!
												//da modificare



    LINK_COST hCost = 
     //   m_hWDMNet.SEG_SPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
		m_hWDMNetPast.wpPAL_DPP_ComputeRoute(*pCircuit, this, pSrc, pDst);
    if (UNREACHABLE != hCost) {

		pCircuit->m_hRouteB=pCircuit->m_hRoute;
			
		ConflictType verifyCode = wpverifyCircuitDED(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito
		//in realta si sovrappone ai circuiti gia esistenti

		if(verifyCode==NO_Conflict)
		{
			RouteConverter(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
	/*	m_hWDMNet.dump(cout);
		cout<<"======================================";
		m_hWDMNetPast.dump(cout);*/
		
		pCircuit->m_eState=Circuit::CT_Setup;
        pCircuit->WPsetUp(this);
 // Associate the circuit w/ the connection
		
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;

		}
		else{
		
					
			if (verifyCode==PRIMARY_Conflict)
			{
				pc++;//contatore conflitti su primario
			}
			
		
			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
				delete (*itLP);
			 delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
		hCost=UNREACHABLE;//da fare:porre stato dropped o cose simili...
		}
       


    } else {
        // see if it can be provisioned using segment protection
       
 
		pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
     pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
	
 LINK_COST hSegCost =
          //  m_hWDMNet.SEG_SP_NO_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst);FABIO: elimino momentaneamente..
			  m_hWDMNet.wpPAL_DPP_ComputeRoute(*pCircuit, this, pSrc, pDst);//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata


        if (UNREACHABLE != hSegCost) {
          //  m_hLog.m_nBlockedConDueToPath++;
           // CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			//FABIO 13 nov: 
			fc++;//FAKE CONFLICT
        }
			list<Lightpath*>::iterator itLP;
			for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
			delete (*itLP);
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
	
		m_hWDMNet.ConIdReq=true;//cosi non inserisco 2 volte consecutive in dbggap 
		
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
	 m_hWDMNetPast.restoreLinkCost();
    m_hWDMNetPast.validateAllLinks();
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}//////////////////////

 //**************************** SEG_SPP_B_HOP ****************************
inline void NetMan::SEG_SPP_B_HOP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

inline bool NetMan::SEG_SPP_B_HOP_Provision(Connection *pCon)
{
    assert(PT_SEG_SPP_B_HOP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);

    LINK_COST hCost = 
        m_hWDMNet.SEG_SPP_B_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst, pCon);
    
    if (UNREACHABLE != hCost) {
        pCircuit->setUp(this);
        // Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        // see if it can be provisioned using segment protection
        m_hWDMNet.restoreLinkCost();
        m_hWDMNet.validateAllLinks();
        m_hWDMNet.validateAllNodes();
        LINK_COST hSegCost =
            m_hWDMNet.SEG_SP_B_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst, pCon);
        if (UNREACHABLE != hSegCost) {
            m_hLog.m_nBlockedConDueToPath++;
            CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
        }

        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
    return (UNREACHABLE != hCost);
}

inline bool NetMan::SEG_SP_B_HOP_Provision(Connection *pCon)
{
    assert(PT_SEG_SP_B_HOP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);

    LINK_COST hCost = 
        m_hWDMNet.SEG_SP_B_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst, pCon);
    
    if (UNREACHABLE != hCost) {
        pCircuit->setUp(this);
#ifdef _OCHDEBUGA
//      pCircuit->dump(cout);
#endif
        // Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    
    return (UNREACHABLE != hCost);
}

inline void NetMan::SEG_SP_B_HOP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

inline bool NetMan::SEG_SP_PB_HOP_Provision(Connection *pCon)
{
    assert(PT_SEG_SP_PB_HOP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);

    LINK_COST hCost = 
        m_hWDMNet.SEG_SP_PB_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst, pCon);
    
    if (UNREACHABLE != hCost) {
        pCircuit->setUp(this);
        // Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    
    return (UNREACHABLE != hCost);
}

inline void NetMan::SEG_SP_PB_HOP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

inline void NetMan::SEG_SPP_PB_HOP_Deprovision(Connection *pCon)
{
    assert(pCon && pCon->m_pPCircuit);
    pCon->m_pPCircuit->tearDown(this);
}

inline bool NetMan::SEG_SPP_PB_HOP_Provision(Connection *pCon)
{
    assert(PT_SEG_SPP_PB_HOP == m_eProvisionType);
    OXCNode *pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
    OXCNode *pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
    assert(pSrc && pDst);

    Circuit *pCircuit = 
        new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL);

    LINK_COST hCost = 
        m_hWDMNet.SEG_SPP_PB_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst, pCon);
    
    if (UNREACHABLE != hCost) {
        pCircuit->setUp(this);
        // Associate the circuit w/ the connection
        pCon->m_pPCircuit = pCircuit;

        // Update status
        pCon->m_eStatus = Connection::SETUP;
    } else {
        // see if it can be provisioned using segment protection
        m_hWDMNet.restoreLinkCost();
        m_hWDMNet.validateAllLinks();
        m_hWDMNet.validateAllNodes();
        LINK_COST hSegCost =
            m_hWDMNet.SEG_SP_PB_HOP_ComputeRoute(*pCircuit, this, pSrc, pDst, pCon);
        if (UNREACHABLE != hSegCost) {
            m_hLog.m_nBlockedConDueToPath++;
            CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
        }

        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
    }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
    return (UNREACHABLE != hCost);
}


///////////////////////////////////////////////////////////////////////
// Mie Funzioni /////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


inline bool NetMan::UNPROTECTED_ProvisionGreen(Connection *pCon)
{
    assert(PT_UNPROTECTED_GREEN == m_eProvisionType); // verifica che sia stato scelto il tipo di protezione UNPROTECTED
    OXCNode *pSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc); //salva nodo sorgente della connesione pCon in pSrc
    OXCNode *pDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst); //salva nodo destinazione in pDst
    assert(pSrc && pDst);

	Circuit *pCircuit = new Circuit(pCon->m_eBandwidth, Circuit::CT_SEG, NULL); //LEAK!

    LINK_COST hCost = m_hWDMNetPast.UNPROTECTED_ComputeRouteGreen(*pCircuit, this, pSrc, pDst);

	if (UNREACHABLE != hCost)  {

		pCircuit->m_hRouteB = pCircuit->m_hRoute;
		ConflictType verifyCode = verifyCircuitUNPR(pCircuit);
		//FABIO 20 sett: funzione che determina se il nuovo circuito
		//in realta si sovrappone ai circuiti gia esistenti

	if(verifyCode==NO_Conflict)
	{
		   	RouteConverterUNPR(pCircuit);//assegno m_hRouteA a partire da m_hRouteB
	
    /*		m_hWDMNet.dump(cout);
	     	cout<<"======================================";
		    m_hWDMNetPast.dump(cout);*/
		
		    pCircuit->m_eState=Circuit::CT_Setup;
            pCircuit->setUp(this);
/*			m_hWDMNet.dump(cout);
		    cout<<"======================================";
		    m_hWDMNetPast.dump(cout);*/
 // Associate the circuit w/ the connection
		
            pCon->m_pPCircuit = pCircuit;
            list<Lightpath*>::iterator itLP;
			//for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++) {

			// Update status
            pCon->m_eStatus = Connection::SETUP;
			transport_cost=hCost;
			PProc_parz=this->m_hWDMNet.NodeProcessingCost;
			PEDFA_parz=this->m_hWDMNet.EDFACost;
		// se la connessione viene stabilita (m_eStatus==SETUP) incrementa il contatore nei DC
			  //  int y;	
	    //    	for (y=0;y!=this->m_hWDMNet.nDC;y++) {
		   // 	if (this->m_hWDMNet.DCvettALL[y]==this->m_hWDMNet.DCused){
			  //	this->m_hWDMNet.nconn[y]=this->m_hWDMNet.nconn[y]+1;
			 	//this->m_hWDMNet.DCCount[y]=this->m_hWDMNet.nconn[y]; } }

			//cout<<"\nht"<<pCon->m_dHoldingTime<<"S: "<<pCon->m_nSrc<<"D: "<<pCon->m_nDst;  //stampa HoldingTime per capire perchè viene diverso dopo
			//cin.get();

				int y;	
	        	for (y=0;y!=this->m_hWDMNet.nDC;y++) {
		    		if (this->m_hWDMNet.DCvettALL[y]==this->m_hWDMNet.DCused){
			  			this->m_hWDMNet.nconn[y]=this->m_hWDMNet.nconn[y]+1;
			 			this->m_hWDMNet.DCCount[y]=this->m_hWDMNet.nconn[y];
						this->m_hWDMNet.DCprocessing[y]=this->m_hWDMNet.DCprocessing[y]+ComputingCost*pCon->m_dHoldingTime;
				
				//cout<<"costo: "<<ComputingCost*pCon->m_dHoldingTime;
				//cin.get();
					}
				}
	  } 
    else{
		      if (verifyCode==PRIMARY_Conflict)
			     {
			       pc++;//contatore conflitti su primario
	               //cout<<"PC:"<<pCon->m_nSequenceNo;
			     }
			   list<Lightpath*>::iterator itLP;
			   //for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++) { //gatto
			   //(*itLP)->m_used=1;	   }   //link usato --> m_used va a 1
				   //MODIFICA OGGI			delete (*itLP); 
			   delete pCircuit;
               pCon->m_eStatus = Connection::DROPPED;
		       hCost=UNREACHABLE;//da fare:porre stato dropped o cose simili...
			   transport_cost=0;
			   PProc_parz=0;
			   PEDFA_parz=0;
	}
} // chiusura if UNREACHEABLE != hCost
	
 else { //else if hCost == UNREACHABLE
          // see if it can be provisioned using segment protection
        	pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
            pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
         LINK_COST hSegCost =
          
		      m_hWDMNet.UNPROTECTED_ComputeRouteGreen(*pCircuit, this, pSrc, pDst);//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata
			
//if (UNREACHABLE != hSegCost){
//   transport_cost=hSegCost;
//  }
//	else {
// transport_cost=0;
// }
         if (UNREACHABLE != hSegCost) {
            // m_hLog.m_nBlockedConDueToPath++;
            // CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			//FABIO 13 nov: 
			fc++;//FAKE CONFLICT
			//cout<<"FS:"<<pCon->m_nSequenceNo;

        }
		//else cout<<"CapCON:"<<pCon->m_nSequenceNo;
		list<Lightpath*>::iterator itLP;
		//for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
		//MODIFICA OGGI		delete (*itLP);
        delete pCircuit;
        pCon->m_eStatus = Connection::DROPPED;
		m_hWDMNet.ConIdReq=true;//cosi non inserisco 2 volte consecutive in dbggap 
   }

    // Postprocess: validate those links that have been invalidated temp
    m_hWDMNet.restoreLinkCost();
    m_hWDMNet.validateAllLinks();
    m_hWDMNet.validateAllNodes();
    
	 m_hWDMNetPast.restoreLinkCost();
    m_hWDMNetPast.validateAllLinks();
    m_hWDMNetPast.validateAllNodes();
    return (UNREACHABLE != hCost);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-B: AGGIUNTA DA ME
//-B: print input command line arguments
void NetMan::printInputData(int numArg, char *arg[])
{
	int i;
	cout << numArg << " INPUT DATA: (arg[])" << endl;
	for (i = 1; i < numArg; i++)
		cout << arg[i] << "\t";
	cout << endl;
}

//-B: originally taken from UNPROTECTED_Provision
inline bool NetMan::WP_BBU_Provision(Connection *pCon)
{

	assert(PT_BBU == m_eProvisionType); // verifica che sia stato scelto il tipo di protezione PT_BBU
	OXCNode *pOXCSrc = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nSrc); //salva nodo sorgente della connesione pCon in pSrc
	OXCNode *pOXCDst = (OXCNode*)m_hWDMNetPast.lookUpNodeById(pCon->m_nDst); //salva nodo destinazione in pDst
	assert(pOXCSrc && pOXCDst);
	//Vertex *pSrc = m_hGraph.lookUpVertex(pCon->m_nSrc, Vertex::VT_Access_Out, -1);	//-B: ATTENZONE!!! VT_Access_Out!!!!!!!!!!
	//Vertex *pDst = m_hGraph.lookUpVertex(pCon->m_nDst, Vertex::VT_Access_In, -1);	//-B: ATTENZONE!!! VT_Access_In!!!!!!!!!!!

	bool bSuccess;
	BandwidthGranularity bwd;
	//-B: STEP 0: assign bandwith to circuit
	if (pCon->m_eConnType == Connection::FIXED_BACKHAUL
		|| pCon->m_eConnType == Connection::MOBILE_BACKHAUL				//-B: if fixed/mobile/fixed-mobile backhaul
		|| pCon->m_eConnType == Connection::FIXEDMOBILE_BACKHAUL)
	{
		bwd = pCon->m_eBandwidth;
	}
	else   //-B: if connType == MOBILE_FRONTHAUL || FIXEDMOBILE_FRONTHAUL
	{
		bwd = pCon->m_eCPRIBandwidth;
	}

	//-B: create a circuit for the connection
	Circuit *pCircuit = new Circuit(bwd, Circuit::CT_Unprotected, NULL); //LEAK!

	bSuccess = WP_BBU_ProvisionHelper_Unprotected(pCon, pCircuit, pOXCSrc, pOXCDst);

	if (bSuccess)
	{
		// STEP 5: Associate the circuit w/ the connection
		pCon->m_pPCircuit = pCircuit;
		pCon->m_pBCircuit = NULL;

		// STEP 6: Update status
		pCon->m_eStatus = Connection::SETUP;
	}
	else
	{
		delete pCircuit;
		pCon->m_eStatus = Connection::DROPPED;
	}

	// Postprocess: validate those links that have been invalidated temporarily
	m_hWDMNet.restoreLinkCost();
	m_hWDMNet.validateAllLinks();
	m_hWDMNet.validateAllNodes();

	m_hWDMNetPast.restoreLinkCost();
	m_hWDMNetPast.validateAllLinks();
	m_hWDMNetPast.validateAllNodes();

	return bSuccess;
}

//-B: originally taken from UNPROTECTED_Deprovision
inline void NetMan::BBU_Deprovision(Connection *pCon)
{
#ifdef DEBUGC
	cout << "-> BBU_Deprovision" << endl;
#endif // DEBUGB

	assert(pCon); //&& pCon->m_pPCircuit); //-B: commented because there could be some connections (FIXED_BACKHAUL)
										 //	could have m_pPCircuit == NULL, but a valid pCircuits list
	
	if (pCon->m_pPCircuit)
	{
		pCon->m_pPCircuit->tearDownCircuit(this, pCon);
		pCon->m_pPCircuit->m_eState = Circuit::CT_Torndown;
	}
	//-B: in case of FIXED_BACKHAUL connections requiring bandw > OCLightpath
	else if (pCon->m_pCircuits.size() > 0)
	{
		list<Circuit*>::iterator itrCircuit;
		Circuit*pCircuit;
		for (itrCircuit = pCon->m_pCircuits.begin(); itrCircuit != pCon->m_pCircuits.end(); itrCircuit++)
		{
			pCircuit = (Circuit*)(*itrCircuit);
			pCircuit->tearDownCircuit(this, pCon);
			pCircuit->m_eState = Circuit::CT_Torndown;
		}
	}
	else
	{
		assert(false);
	}
}

//-B: return true if the initialization of the NetMan object has been done correctly (with all the input parameters)
//	this method is called in ConProvisionMain.cpp and it is called before the initialization of Simulator object
//	this method calls 2 main methods: readTopo (-> readTopoHelper) and genInitialStateGraph
bool NetMan::BBUinitialize(const char* pTopoFile,
	const char* pProtectionType,
	int nNumberOfPaths,
	double dEpsilon,
	int nTimePolicy,
	const char* UnAv) // M//ANDREA
{
	cout << "\n-> hNetman BBUinitialize" << endl;
	
	assert(pTopoFile && pProtectionType);
	TopoReader hTopoReader;
	if (!hTopoReader.BBUReadTopo(m_hWDMNet, pTopoFile))
	{
		cerr << "- Error reading topo file " << pTopoFile << endl;
		return false;
	}

	// if (!hTopoReader.readTopo(m_hServWDMNet, pTopoFile)) {
	//    cerr
	//}
	// Parse protection type<<"- Error reading topo Serv file "<<pTopoFile<<endl;
	//    return false;

	//-B: set the provisioning type
	if (0 == strcmp(pProtectionType, "SEG_SP_NO_HOP")) {
		m_eProvisionType = PT_SEG_SP_NO_HOP;
	}
	else if (0 == strcmp(pProtectionType, "wpSEG_SPP")) {
		m_eProvisionType = PT_wpSEG_SPP;
	}
	else if (0 == strcmp(pProtectionType, "wpUNPROTECTED")) {
		m_eProvisionType = PT_wpUNPROTECTED;
	}
	else if (0 == strcmp(pProtectionType, "wpDEDICATED")) {
		m_eProvisionType = PT_wpPAL_DPP;
	}
	else if (0 == strcmp(pProtectionType, "SPPBw")) {
		m_eProvisionType = PT_SPPBw;
	}
	else if (0 == strcmp(pProtectionType, "SPPCh")) {
		m_eProvisionType = PT_SPPCh;
	}
	else if (0 == strcmp(pProtectionType, "UNPROTECTED")) {
		m_eProvisionType = PT_UNPROTECTED;
	}
	else if (0 == strcmp(pProtectionType, "UNPROTECTED_GREEN")) {
		m_eProvisionType = PT_UNPROTECTED_GREEN;
	}
	else if (0 == strcmp(pProtectionType, "DEDICATED")) {
		m_eProvisionType = PT_PAL_DPP;
	}
	else if (0 == strcmp(pProtectionType, "SEG_SP_B_HOP")) {
		m_eProvisionType = PT_SEG_SP_B_HOP;
	}
	else if (0 == strcmp(pProtectionType, "SEG_SP_L_NO_HOP")) {
		m_eProvisionType = PT_SEG_SP_L_NO_HOP;
	}
	else if (0 == strcmp(pProtectionType, "SEG_SP_PB_HOP")) {
		m_eProvisionType = PT_SEG_SP_PB_HOP;
	}
	else if (0 == strcmp(pProtectionType, "SEG_SPP")) {
		m_eProvisionType = PT_SEG_SPP;
	}
	else if (0 == strcmp(pProtectionType, "SEG_SPP_B_HOP")) {
		m_eProvisionType = PT_SEG_SPP_B_HOP;
	}
	else if (0 == strcmp(pProtectionType, "SEG_SPP_PB_HOP")) {
		m_eProvisionType = PT_SEG_SPP_PB_HOP;
	}
	else if (0 == strcmp(pProtectionType, "SEG_SP_AGBSP")) {
		m_eProvisionType = PT_SEG_SP_AGBSP;
	}
	else if (0 == strcmp(pProtectionType, "BBU_HOTEL")) {
		m_eProvisionType = PT_BBU;
	}
	else {
		TRACE1("- ERROR: invalid protection type: %s\n", pProtectionType);
		return false;
	}

	if (nNumberOfPaths <= 0) {
		TRACE1("- ERROR: invalid #OfPaths: %d\n", nNumberOfPaths);
		return false;
	}
	m_nNumberOfPaths = nNumberOfPaths;
	m_hWDMNet.m_dEpsilon = dEpsilon;
	//m_hServWDMNet.m_dEpsilon = dEpsilon; //-M
	m_nTimePolicy = nTimePolicy; // M

								 // if (m_nTimePolicy==1){
								 //  cerr <<"The value of TimePolicy is" << m_nTimePolicy  <<endl;
								 // }

	m_UnAv = false;
	if (0 == strcmp(UnAv, "A"))
		m_UnAv = true;
	// Added ANDREA
	m_eStatActive = PBLOCK;
	// -B: added by me: create a 3-layer network.
	m_hWDMNet.genStateGraph(m_hGraph);
	// generate initial state graph: just create conflict set (don't know how it can be used for)
	genInitialStateGraph();

	m_hWDMNetPast = m_hWDMNet;

	//-B: for each macro cell, it fills its cluster of small cells directly connected to it
	if (SMALLCELLS_PER_MC <= 0)
	{
		genSmallCellCluster();

	}

	//-B: other parameters, not set so far
	if (m_eProvisionType == PT_BBU)
	{
		m_nTxReqPerLightpath = 1;
		//m_eGroomingPolicy = ;
	}
	
	return true;
}

LightpathDB& NS_OCH::NetMan::getLightpathDB()
{
	return m_hLightpathDB;
}

ConnectionDB& NS_OCH::NetMan::getConnectionDB()
{
	return m_hConnectionDB;
}

//-B: originally taken from PAC_DPP_Provision
inline bool NetMan::BBU_ProvisionNew(Connection *pCon)
{	
#ifdef DEBUGB
	cout << "-> BBU_ProvisionNew" << endl;
#endif // DEBUGB

	bool bSuccess;
	BandwidthGranularity bwd; // -L: how much bwd we are allocating

	//STEP 00: if the (fronthaul) connection has dst == NULL,
	//	it means that no valid (reachable) BBU hotel node was found
	// -L: fronthaul that hadn't find a destination in the previous step
	//     it means that DU placement failed
	if (pCon->m_nDst == NULL)
	{
#ifdef DEBUG
		cout << " -> Blocked connection due to unreachability of BBU" << endl;
#endif // DEBUGB
		//cin.get();
		//-B: check if it is NULL because it wasn't possible to find a hotel node respecting the latency budget or not
		//	(m_bHotelNotFoundBeacauseOfLatency is set/modified in placeBBUHigh method)
		if (m_bHotelNotFoundBecauseOfLatency) {
#ifdef DEBUG
			cin.get();
#endif
			pCon->m_bBlockedDueToLatency = true;
		}
		else {
			pCon->m_bBlockedDueToUnreach = true;
#ifdef DEBUG
			cout << "Connection blocked due to unreach" << endl;
			cin.get();
#endif
		}
		bSuccess = false;
		pCon->m_eStatus = Connection::DROPPED;
		return bSuccess;
	}

	//-B: STEP 0: assign bandwith to circuit
	if (pCon->m_eConnType == Connection::FIXED_BACKHAUL
		|| pCon->m_eConnType == Connection::MOBILE_BACKHAUL				//-B: if fixed/mobile/fixed-mobile backhaul
		|| pCon->m_eConnType == Connection::FIXEDMOBILE_BACKHAUL)	
	{
		bwd = pCon->m_eBandwidth;
	}
	//-L: add the midhaul case
	if (pCon->m_eConnType == Connection::FIXED_MIDHAUL)
	{
		bwd = pCon->m_eBandwidth;
	}
	//-B: if connType == MOBILE_FRONTHAUL || FIXEDMOBILE_FRONTHAUL
	else if(pCon->m_eConnType == Connection::MOBILE_FRONTHAUL ||
			pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
	{
		bwd = pCon->m_eCPRIBandwidth;
	}

	Circuit *pPCircuit;

	//-B: in case of FIXED BACKHAUL connections requiring more than the capacity of an entire channel/lightpath
	if (bwd > OCLightpath)
	{
		cout << "Bwd: " << bwd << endl;
		bSuccess = BBU_ProvisionHelper_Unprotected_BIG(pCon, bwd);
		return bSuccess;
	}
		
	//-B: STEP 1 - --------------- CIRCUIT CREATION (Unprotected) ----------------
	pPCircuit = new Circuit(bwd, Circuit::CT_Unprotected, NULL);

	Vertex *pSrc = m_hGraph.lookUpVertex(pCon->m_nSrc, Vertex::VT_Access_Out, -1);	//-B: ATTENZIONE!!! VT_Access_Out
	Vertex *pDst = m_hGraph.lookUpVertex(pCon->m_nDst, Vertex::VT_Access_In, -1);	//-B: ATTENZIONE!!! VT_Access_In
	OXCNode*pOXCSrc = (OXCNode*)this->m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
	assert(pSrc && pDst);

	//-B: it would be useless in case the path was already computed during findBestBBUHotel method, while creating the connection
	if (precomputedPath.size() == 0)
	{
	//-B: STEP 2 - !!!!!!!!!!!!!!!!!!------------------ UPDATE NETWORK GRAPH -------------------!!!!!!!!!!!!!!!!!!!

		if (pCon->m_eConnType == Connection::FIXED_BACKHAUL
			|| pCon->m_eConnType == Connection::MOBILE_BACKHAUL
			|| pCon->m_eConnType == Connection::FIXEDMOBILE_BACKHAUL)
		{
			// -L: VA MODIFICATO ????
			// *************** PRE-PROCESSING ******************
			//-B: it is used to prefer wavelength continuity over w. conversion in a condition of equality
			//	between the wavelength that would allow a wavelength path and any other best-fit wavelegth
			//	(l'ho messo anche per il backhaul dato che il costo della rete
			//	è dato dal numero di nodi attivi + numero di lightpath attivi)
			m_hGraph.preferWavelPath(); //assign a cost equal to 0.6 to simplex links of type LT_Converter and LT_Grooming

			//m_hGraph.invalidateSimplexLinkDueToCap(bwd);
			//invalidateSimplexLinkDueToFreeStatus();

			//-B: following function should include both the previous ones
			invalidateSimplexLinkDueToCapOrStatus(bwd, (UINT)(pCon->m_eConnType));

			//-B: invalidate simplex link LT_Grooming and LT_Converter for nodes that don't have active BBUs
			if (ONLY_ACTIVE_GROOMING)
			{
				this->invalidateSimplexLinkGrooming();
			}
			//updateCostsForBestFit(); //-B: TO BE USED ONLY IF BBU_NewCircuit ASSIGNS A COST = 0 TO NEW LIGHTPATHS
			//updateGraphValidityAndCosts(pOXCSrc, bwd);
		}
		else {
			;
		}
	}//end IF precomputed path == 0

	//-B: STEP 3,4,5,6 - ------------- VWP AND WP CASES --------------
	bSuccess = BBU_ProvisionHelper_Unprotected(pCon, *pPCircuit, pSrc, pDst);

	if (bSuccess)
	{
		// STEP 7 - Associate the circuit w/ the connection
		pCon->m_pPCircuit = pPCircuit;
		pCon->m_pBCircuit = NULL;

		// STEP 8 - Assign connection routing time
		pCon->m_dRoutingTime = pPCircuit->latency;

		list<Lightpath*> lightpathsOnCircuit = pPCircuit->m_hRoute;
		Lightpath* lightpathCircuit;
		list<Lightpath*>::const_iterator lightpathIter;
		list<Connection*> elementsToBeDeleted;

		// check if there are connections with which I'm doing grooming
		for (lightpathIter = lightpathsOnCircuit.begin(); lightpathIter != lightpathsOnCircuit.end(); lightpathIter++) {
			lightpathCircuit = (*lightpathIter);
			list <Connection*>::const_iterator connIter;
			Connection* pConOnLightpath;

			for (connIter = lightpathCircuit->m_hConnectionsUsingLightpath.begin(); connIter != lightpathCircuit->m_hConnectionsUsingLightpath.end(); connIter++) {
				pConOnLightpath = (Connection*)(*connIter);

				list <Connection*>::iterator itrG;
				Connection* pConG;
				for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {

					pConG = (Connection*)(*itrG);

					if (pConOnLightpath == pConG) {

						// there is a connection with which I'm grooming: 
						// - update the routing time of the connection by adding the switching latency
						// - set the variable m_groomingAlreadydone related to the lightpath to true

						
						pConOnLightpath->m_dRoutingTime += ELSWITCHLATENCY;
						
						lightpathCircuit->m_groomingAlreadydone = true;

#ifdef DEBUGX
						cout << "\tLigthpath from " << lightpathCircuit->getSrcOXC()->getId() << " to " <<
							lightpathCircuit->getDstOXC()->getId() << " on wavelength " << lightpathCircuit->wlAssigned <<
							" is doing grooming" << endl;

						cout << "Updated routing time of connection from " << pConOnLightpath->m_nSrc
							<< " to " << pConOnLightpath->m_nDst << endl;
#endif
						elementsToBeDeleted.push_back(pConG);
						//cin.get();
						break;

					}

				}
				
				list<Connection*>::const_iterator itrDelete;
				Connection* conToDelete;
				for (itrDelete = elementsToBeDeleted.begin(); itrDelete != elementsToBeDeleted.end(); itrDelete++) {
					conToDelete = (Connection*)(*itrDelete);
					groomingConnections.erase(conToDelete->m_nSequenceNo);

				}
			

			}
			
		}
	
		if (groomingConnections.size() > 0) {

			cout << "Grooming connections size is " << groomingConnections.size() << endl;
			list <Connection*>::iterator itrG;
			Connection* pConG;
			cout << "Connection to be provisioned is from " << pCon->m_nSrc << " to " << pCon->m_nDst << endl;
			for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {
				pConG = (Connection*)(*itrG);
				cout << "Connection present groomingConnections is from " << pConG->m_nSrc << " to " << pConG->m_nDst << endl;
			}
			groomingConnections.clear();
			//cin.get();
		}


		//-B: set connection's routing time = 0 in a particular case
		if (pCon->m_eConnType == Connection::MOBILE_FRONTHAUL || pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
		{
			if (pCon->m_eCPRIBandwidth == OC0)
			{
				assert(!ONE_CONN_PER_NODE && !MIDHAUL && BBUSTACKING);
				pCon->m_dRoutingTime = 0;
			}
		}

		// STEP 9 - Update connection status
		pCon->m_eStatus = Connection::SETUP;
	} //-L: END if(bsuccess)
	else
	{
		groomingConnections.clear();
		delete pPCircuit;
		pCon->m_eStatus = Connection::DROPPED;
	}

	//-B: STEP 10 - POST-PROCESS: validate those links that have been invalidated temporarily
	//-B: this function should aggregate following 2 functions
	m_hGraph.resetLinks();
	//m_hGraph.validateAllLinks();
	//-B: reset original links' costs (modified into updateGraphValidity)
	//m_hGraph.restoreLinkCost();

	return bSuccess;
}

//-B: originally taken from BBU_ProvisionHelper
inline bool NetMan::BBU_ProvisionHelper_Unprotected(Connection *pCon, Circuit& hPCircuit, Vertex *pSrc, Vertex *pDst)
{
#ifdef DEBUG
	cout << "-> BBU_ProvisionHelper_Unprotected" << endl;
#endif // DEBUGB

	list<AbstractLink*> hPrimaryPath;
	list<AbsPath*>hPPaths;
	LINK_COST hPrimaryCost = UNREACHABLE;
	UINT channel;
	channel = -1;

	//-B: since not initialized
	m_hGraph.numberOfChannels = m_hWDMNet.numberOfChannels;
	m_hGraph.numberOfNodes = m_hWDMNet.numberOfNodes;
	m_hGraph.numberOfLinks = m_hWDMNet.numberOfLinks;
	//OXCNode*pOXCSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
	//OXCNode*pOXCDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);


	//-B: STEP 3 - *********** CALCULATE NEW PATH AND RELATED COST ***********
	
	// It means it's a connection changing bbu -> i'm going to use the path already computed
	if (!(pCon->m_pPathToUse.empty())) {
		
		hPrimaryPath = pCon->m_pPathToUse;
		hPrimaryCost = pCon->m_hCostToUse;
#ifdef DEBUG
		cout << "\tPath already computed" << endl;
#endif
		list<AbstractLink*>::const_iterator itrLink;
		SimplexLink* linkPath;

#ifdef DEBUG
		for (itrLink = hPrimaryPath.begin(); itrLink != hPrimaryPath.end(); itrLink++) {
			linkPath = (SimplexLink*)(*itrLink);
			if (linkPath->getSimplexLinkType() == SimplexLink::LT_Lightpath) {
				cout << "+ Lightpath from " << linkPath->m_pLightpath->getSrc()->getId()
					<< " to " << linkPath->m_pLightpath->getDst()->getId()
					<< " on wavelength " << linkPath->m_pLightpath->wlAssigned << endl;
			} else if (linkPath->getSimplexLinkType() == SimplexLink::LT_Channel) {
			
				cout << "\t+ Fiber's length = " << linkPath->m_pUniFiber->getLength() << endl;
				cout << "\t++ Cost of the link = " << linkPath->getCost() << endl;
				cout << "\t+ From " << linkPath->m_pUniFiber->getSrc()->getId() << " to " << linkPath->m_pUniFiber->getDst()->getId() << endl;

			}
			else if (linkPath->getSimplexLinkType() == SimplexLink::LT_Converter) {
			
				cout << "\t+Electronic switch" <<endl;
			
			}

		}
#endif

	} else if (precomputedPath.size() == 0)
	{
		if (pCon->m_eConnType == Connection::MOBILE_FRONTHAUL || pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
		{
			hPrimaryCost = m_hGraph.DijkstraLatency(hPrimaryPath, pSrc, pDst, AbstractGraph::LCF_ByOriginalLinkCost);
		}
		else //-L: BACKHAUL and MIDHAUL --> no strict latency requirements
		{
			hPrimaryCost = m_hGraph.Dijkstra(hPrimaryPath, pSrc, pDst, AbstractGraph::LCF_ByOriginalLinkCost);
		}
	} else if (precomputedPath.size() > 0)
	{
		cout << "Path computed in findbestbbuhotel" << endl;
		hPrimaryPath = precomputedPath;
		hPrimaryCost = precomputedCost;
	}

#ifdef DEBUGC
	cout << "\tCosto shortest path: PrimaryCost = " << hPrimaryCost << endl;
#endif // DEBUGB

	//-B: EVALUATE PATH'S COST
	if (hPrimaryCost == UNREACHABLE)
	{
#ifdef DEBUGF
		cout << " -> Blocked connection due to unreachability" << endl;
		cin.get();
#endif // DEBUGB
		//pCon->m_eStatus = Connection::DROPPED; //-B: fatto nel chiamante BBU_ProvisionNew
		pCon->m_bBlockedDueToUnreach = true;
		return false;
	}
	//ELSE: as for cost, the path is admissible. Let's see for latency

	//-B: STEP 5 - ----------- COMPUTE && VERIFY PATH'S LATENCY -------------
	if (!(pCon->m_pPathToUse.empty())) {
	
		hPCircuit.latency = pCon->m_dLatency;

	}
	else {
		float pathLatency = computeLatencyP3(hPrimaryPath, pSrc, (UINT)(pCon->m_eConnType));
		hPCircuit.latency = pathLatency;
	}

#ifdef DEBUGB
	cout << "\t= " << hPCircuit.latency << " " << endl;
#endif // DEBUGB

	if (pCon->m_eConnType == Connection::MOBILE_FRONTHAUL || pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
	{
		if (hPCircuit.latency > LATENCYBUDGET)
		{
			//pCon->m_eStatus = Connection::DROPPED; //-B: fatto nel chiamante BBU_ProvisionNew
			pCon->m_bBlockedDueToLatency = true;
#ifdef DEBUGF
			cout << "-> Blocked connection due to latency (latency budget = " << LATENCYBUDGET << ")" << endl;
			cin.get();
#endif // DEBUGB
		}
		
		bool conditionSatisfied = true;

		list <Connection*>::const_iterator itrG;
		Connection* pConG;

		for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {

			pConG = (Connection*)(*itrG);

			if (pConG->m_eConnType == Connection::MOBILE_FRONTHAUL || pConG->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
			{
				
				if ((pConG->m_dRoutingTime + ELSWITCHLATENCY) > LATENCYBUDGET) {
#ifdef DEBUG
					cout << "Connection on grooming path fron source " << pConG->m_nSrc <<
						" exceeds the latency budget" << endl;
					cout << "\tRouting + grooming time = " << pConG->m_dRoutingTime + ELSWITCHLATENCY << endl;
#endif
					conditionSatisfied = false;
					//cin.get();
					break;
				}


			}
		}
		if (!conditionSatisfied || pCon->m_bBlockedDueToLatency == true) {

			cout << "Grooming time reset" << endl;

			groomingConnections.clear();

			pCon->m_bBlockedDueToLatency = true;

			return false;

		} else	{
	#ifdef DEBUGB
				cout << "< " << LATENCYBUDGET << " (fronthaul latency budget) ==> ok!" << endl << endl; //nothing to do: go on!
	#endif // DEBUGB
		}
	}
	else //-L: backhaul and midhaul connections
	{

		list <Connection*>::const_iterator itrG;
		Connection* pConG;
		list<Lightpath*> savedCircuit;

		// check if there is a connection on the path chosen which exceeds the latency budget
		if (hPCircuit.latency == MAXVALUE_LATENCY) {

			do {

				pCon->m_bBlockedDueToLatency = false;

				SimplexLink* linkToInvalidate = NULL;

				for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {

					pConG = (Connection*)(*itrG);

					if (pConG->m_eConnType == Connection::MOBILE_FRONTHAUL || pConG->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
					{

						if ((pConG->m_dRoutingTime + ELSWITCHLATENCY) > LATENCYBUDGET) {
#ifdef DEBUG
							cout << "Connection on grooming path from source " << pConG->m_nSrc <<
								" exceeds the latency budget" << endl;
							cout << "\tRouting + grooming time = " << pConG->m_dRoutingTime + ELSWITCHLATENCY << endl;
#endif

							savedCircuit = pConG->m_pPCircuit->m_hRoute;

							pCon->m_bBlockedDueToLatency = true;

							break;
						}
						else {
							pCon->m_bBlockedDueToLatency = false;
						}
					}
				}
				if (pCon->m_bBlockedDueToLatency) {

					list<AbstractLink*>::const_iterator itrLink;
					SimplexLink* linkPath;

					// Look for the lightpath that can't be used to route the backhaul connection -> invalidate the lightpath
					for (itrLink = hPrimaryPath.begin(); itrLink != hPrimaryPath.end(); itrLink++) {
						linkPath = (SimplexLink*)(*itrLink);
						if (linkPath->getSimplexLinkType() == SimplexLink::LT_Lightpath) {
#ifdef DEBUG
							cout << "Lightpath from " << linkPath->m_pLightpath->getSrc()->getId()
								<< " to " << linkPath->m_pLightpath->getDst()->getId()
								<< " on wavelength " << linkPath->m_pLightpath->wlAssigned << endl;
#endif

						list<Lightpath*>::const_iterator itrGroom;
						Lightpath* linkGroom;
						for (itrGroom = savedCircuit.begin(); itrGroom != savedCircuit.end(); itrGroom++) {

							linkGroom = (Lightpath*)(*itrGroom);
							if (linkGroom == (linkPath)->m_pLightpath) {
#ifdef DEBUG

								cout << "Found the lightpath on which I can't groome" << endl;
#endif

								linkToInvalidate = linkPath;
								break;
							}


						}
						}

						if (linkToInvalidate != NULL) {
							break;
						}

					}

					if (linkToInvalidate != NULL) {

						groomingConnections.clear();
						hPrimaryPath.clear();

						linkToInvalidate->invalidate();

						hPrimaryCost = m_hGraph.Dijkstra(hPrimaryPath, pSrc, pDst, AbstractGraph::LCF_ByOriginalLinkCost);
						hPCircuit.latency = computeLatencyP3(hPrimaryPath, pSrc, (UINT)pCon->m_eConnType);

						// No need to validate the link because it will be done
						// by resetLinks() in BBU_ProvisionNew
						//linkToInvalidate->validate();

					}
				}
			} while (pCon->m_bBlockedDueToLatency == true);

			if (pCon->m_bBlockedDueToLatency) {
				groomingConnections.clear();
				return false;
			}

		}
	}
#ifdef DEBUGB
	//cin.get();
#endif // DEBUGB

	//-B: as for feasibility, the connection's provisioning has been "approved". Let's build the circuit and consume bandwidth
	//-B: STEP 4 - -------------- BUILD THE CIRCUIT -----------------
	list<AbstractLink*> path = BBU_NewCircuit(hPCircuit, hPrimaryPath, channel, pCon);

#ifdef DEBUG
	list<AbstractLink*>::const_iterator itrPath = path.begin();
	SimplexLink* linkfrompath;
	cout << "After BBU_NewCircuitt" << endl;
	while (itrPath != path.end()) {
	
		linkfrompath = (SimplexLink*)(*itrPath);
		if (linkfrompath->getSimplexLinkType() == SimplexLink::LT_Lightpath) {
			cout << "Lightpath from " << linkfrompath->m_pLightpath->getSrc()->getId()
				<< " to " << linkfrompath->m_pLightpath->getDst()->getId()
				<< " on wavelength " << linkfrompath->m_pLightpath->wlAssigned << endl;
		}
		else if (linkfrompath->getSimplexLinkType() == SimplexLink::LT_Channel) {
		
			cout << "It should not be a channel" << endl;
			cin.get();
		}
		else if (linkfrompath->getSimplexLinkType() == SimplexLink::LT_UniFiber) {
		
			cout << "It should not be a channel" << endl;
			cin.get();
		}
		itrPath++;
	}
#endif

	// if the source node just changed bbu
	if (this->connectionsChangingBBU.size() > 0) {

		Connection* pConChangingBBU;
		list<Connection*>::const_iterator itr;

		// each connection which is routed to the new bbu updates the path to the new bbu: every connection will use the same path
		for (itr = this->connectionsChangingBBU.begin(); itr != this->connectionsChangingBBU.end(); itr++)
		{
			pConChangingBBU = (Connection*)(*itr);
			pConChangingBBU->m_pPathToUse = path;
			pConChangingBBU->m_hCostToUse = precomputedCost;
			pConChangingBBU->m_dLatency = hPCircuit.latency;
		}
	}
	
	connectionsChangingBBU.clear();

	//-B: STEP 6 - ----------- SETUP THE CIRCUIT -----------
	hPCircuit.Unprotected_setUpCircuit(this);

	return true;
}

//-B: it works correctly only for backhaul connections. For fronthaul connections, it should check the latency
bool NetMan::BBU_ProvisionHelper_Unprotected_BIG(Connection*pCon, BandwidthGranularity totBWD)
{
#ifdef DEBUGB
	cout << "-> BBU_ProvisionHelper_Unprotected_BIG" << endl;
#endif // DEBUGB
	
	cin.get();

	//only fixed connections can reach this instruction
	assert(pCon->m_eConnType == Connection::ConnectionType::FIXED_BACKHAUL);

	BandwidthGranularity residualBWD = totBWD;
	bool bSuccess = true;
	//UINT b = OCLightpath;
	list<AbstractLink*> hPrimaryPath;
	LINK_COST hPrimaryCost = UNREACHABLE;
	SimulationTime maxLatency = 0;
	UINT channel;
	channel = -1;
	UINT whileCounter = 1;

	//-B: STEP 1 - !!!!!!!!!!!!!!!!!!------------------ UPDATE NETWORK GRAPH -------------------!!!!!!!!!!!!!!!!!!!
	Vertex *pSrc = m_hGraph.lookUpVertex(pCon->m_nSrc, Vertex::VT_Access_Out, -1);	//-B: ATTENZIONE!!! VT_Access_Out
	Vertex *pDst = m_hGraph.lookUpVertex(pCon->m_nDst, Vertex::VT_Access_In, -1);	//-B: ATTENZIONE!!! VT_Access_In
	OXCNode*pOXCSrc = (OXCNode*)this->m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
	assert(pSrc && pDst);
	// *************** PRE-PROCESSING ******************
	//-B: it is used to prefer wavelength continuity over w. conversion in a condition of equality
	//	between the wavelength that would allow a wavelength path and any other best-fit wavelegth
	//	(l'ho messo anche per il backhaul dato che il costo della rete
	//	è dato dal numero di nodi attivi + numero di lightpath attivi)
	m_hGraph.preferWavelPath(); //assign a cost equal to 0.5 to simplex links of type LT_Converter and LT_Grooming
	m_hGraph.invalidateSimplexLinkDueToCap(OCLightpath);
	invalidateSimplexLinkDueToFreeStatus();
	//-B: invalidate simplex link LT_Grooming and LT_Converter for nodes that don't have active BBUs
	if (ONLY_ACTIVE_GROOMING)
	{
		this->invalidateSimplexLinkGrooming();
	}
	//updateCostsForBestFit(); //-B: IT SHOULD BE USELESS FOR THIS CASE
	

	//-B: +++++++++++++++++++++++++++++++++++++++ OCLIGHTPATH CONNECTION PROVISIONING ++++++++++++++++++++++++++++++++++++++++++++
	while (residualBWD > OCLightpath)
	{
#ifdef DEBUGB
		cout << "\tCREO CIRCUITO #" << whileCounter << " (residual bandwidth = " << residualBWD << ")" << endl;
#endif // DEBUGB
		//-B: STEP 2 - --------------- CIRCUIT CREATION (Unprotected) ----------------
		BandwidthGranularity BWDToBeAssigned = OCLightpath;
		Circuit*pPCircuit = new Circuit(BWDToBeAssigned, Circuit::CT_Unprotected, NULL);

		//----------------------------------------------
		////////////////// PROVISIONING /////////////////
		//----------------------------------------------
		//-B: since not initialized
		m_hGraph.numberOfChannels = m_hWDMNet.numberOfChannels;
		m_hGraph.numberOfNodes = m_hWDMNet.numberOfNodes;
		m_hGraph.numberOfLinks = m_hWDMNet.numberOfLinks;

#ifdef DEBUGB
		UINT numGroomNodes = 0;
		//numGroomNodes = this->countGroomingNodes();
		if (numGroomNodes > 1)
		{
			cout << "\tCI SONO " << numGroomNodes << " POTENZIALI GROOMING NODES" << endl;
			//cin.get();
		}
#endif // DEBUGB

		//-B: STEP 3 - *********** CALCULATE NEW PATH AND RELATED COST ***********
		hPrimaryCost = m_hGraph.Dijkstra(hPrimaryPath, pSrc, pDst, AbstractGraph::LCF_ByOriginalLinkCost);

#ifdef DEBUGB
		cout << "\tCosto shortest path: PrimaryCost = " << hPrimaryCost << endl;
#endif // DEBUGB

		//-B: EVALUATE PATH'S COST
		if (hPrimaryCost == UNREACHABLE)
		{
#ifdef DEBUGB
			cout << " -> Blocked connection due to unreachability" << endl;
			cin.get();
#endif
			pCon->m_bBlockedDueToUnreach = true;
			bSuccess = false;
			break;
		}
		//ELSE: as for cost, the path is admissible

		//-B: STEP 5 - ----------- COMPUTE PATH'S LATENCY -------------
		float pathLatency = computeLatencyP3(hPrimaryPath, pSrc, (UINT)(pCon->m_eConnType));
		pPCircuit->latency = pathLatency;
		//-B: SAVE MAX LATENCY AMONG ALL THE PARALLEL CIRCUITS
		if (pathLatency > maxLatency)
		{
			maxLatency = pathLatency;
		}
#ifdef DEBUGB
		cout << "\t= " << pPCircuit->latency << " ";
		cout << "-> (e' un backhaul) ok" << endl << endl; //no latency constraints: go on!
#endif // DEBUGB

		//-B: as for feasibility, the connection's provisioning hass been "approved". Let's build the circuit and consume bandwidth
		//-B: STEP 4 - -------------- BUILD THE CIRCUIT -----------------
		BBU_NewCircuit(*pPCircuit, hPrimaryPath, channel, pCon);

		//-B: add circuit to circuit list of current connection
		pCon->m_pCircuits.push_back(pPCircuit);

		//-B: for next iteration, invalidate those links belonging to the path computed in this iteration
		m_hGraph.invalidateSimplexLinkDueToPath(hPrimaryPath);
		m_hGraph.invalidateSimplexLinkLightpath();

		//-B: last things to do
		UINT a = residualBWD;
		a = a - OCLightpath;
		residualBWD = (BandwidthGranularity)a;
		whileCounter++;
	} //end WHILE
	//---------------------------------------------------------------------------------------------------------------------


	//-B: +++++++++++++++++++++++++++++++++++++++ RESIDUAL BANDWIDTH PROVISIONING ++++++++++++++++++++++++++++++++++++++++++++
	//-B: if at this point bSuccess is still == true, it means that we still have to provide connection to an amount of bwd < OCLightpath
	if (bSuccess)
	{
#ifdef DEBUGB
		cout << "\n\tCREO ULTIMO CIRCUITO #" << whileCounter << " (residual bandwidth = " << residualBWD << ")" << endl;
#endif // DEBUGB

		assert(residualBWD <= OCLightpath);
		//-B: !!!!!!!!!!!!!!!!!!!!!!!!!!! UPDATE NETWORK GRAPH !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//-B: !!! some links invalidated in the previous part could be valid for a connection requiring a residualBWD <= OCLightpath !!!
		m_hGraph.validateSimplexLinkDueToCap(residualBWD);

		Circuit*pPCircuit = new Circuit(residualBWD, Circuit::CT_Unprotected, NULL);

		//--------------------------------------------------
		//////////////////// PROVISIONING //////////////////
		//--------------------------------------------------
		//-B: STEP 3 - *********** CALCULATE NEW PATH AND RELATED COST ***********
		hPrimaryCost = m_hGraph.Dijkstra(hPrimaryPath, pSrc, pDst, AbstractGraph::LCF_ByOriginalLinkCost);

#ifdef DEBUGB
		cout << "\tCosto shortest path: PrimaryCost = " << hPrimaryCost << endl;
#endif // DEBUGB

		//-B: EVALUATE PATH'S COST
		if (hPrimaryCost == UNREACHABLE)
		{
#ifdef DEBUGB
			cout << " -> Blocked connection due to unreachability" << endl;
			cin.get();
#endif
			pCon->m_bBlockedDueToUnreach = true;
			bSuccess = false;
		}
		//ELSE: as for cost, the path is admissible

		//-B: since I could not put a break instruction, I have to check the boolean var
		if (bSuccess)
		{
			//-B: STEP 5 - ----------- COMPUTE PATH'S LATENCY -------------
			float pathLatency = computeLatencyP3(hPrimaryPath, pSrc, (UINT)(pCon->m_eConnType));
			pPCircuit->latency = pathLatency;
			if (pathLatency > maxLatency)
			{
				maxLatency = pathLatency;
			}
#ifdef DEBUGB
			cout << "\t= " << pPCircuit->latency << " ";
			cout << "-> (e' un backhaul) ok" << endl << endl; //no latency constraints: go on!
#endif // DEBUGB

			//-B: as for feasibility, the connection's provisioning hass been "approved". Let's build the circuit and consume bandwidth
			//-B: STEP 4 - -------------- BUILD THE CIRCUIT -----------------
			BBU_NewCircuit(*pPCircuit, hPrimaryPath, channel, pCon);

			//-B: add circuit to circuit list of current connection
			pCon->m_pCircuits.push_back(pPCircuit);

			//-B: just to check. It should be correct
			assert(pCon->m_pCircuits.size() == whileCounter);
		}	
	} //end IF bSuccess to provide connection to the last part of the connection request
	//------------------------------------------------------------------------------------------------------------

	pCon->m_pPCircuit = NULL;
	pCon->m_pBCircuit = NULL;

	
	//-B: +++++++++++++++++++++++++++++++++++ CONSUME BANDWIDTH ON PARALLEL PATHS ++++++++++++++++++++++++++++++++++
	//-B: if at this point bSuccess is still == true, then the connection for the big fixed request can be provided
	if (bSuccess)
	{
		list<Circuit*>::iterator itrCircuit;
		Circuit*pCircuit;
		int i = 1;
		for (itrCircuit = pCon->m_pCircuits.begin(); itrCircuit != pCon->m_pCircuits.end(); i++, itrCircuit++)
		{
#ifdef DEBUGB
			cout << "\tCONSUMO CIRCUITO #" << i << endl;
#endif // DEBUGB
			pCircuit = (Circuit*)(*itrCircuit);
			pCircuit->Unprotected_setUpCircuit(this);
		}
		
		// STEP 8 - Assign connection routing time
		pCon->m_dRoutingTime = maxLatency;

		// STEP 9 - Update connection status
		pCon->m_eStatus = Connection::SETUP;
	}
	else //IF !bSuccess
	{
#ifdef DEBUGB
		cout << "BIG CONNECTION BLOCKED! RIMUOVO DAL GRAPH I SIMPLEX LINKS LIGHTPATH AGGIUNTI. premi invio e stampo netman" << endl;
		//cin.get();
		//this->dump(cout);
#endif // DEBUGB
		m_hGraph.removeSimplexLinkLightpathNotUsed(); //-B: explanation above function's definition
		pCon->m_eStatus = Connection::DROPPED;
	}
	
	//-B: STEP 10 - POST-PROCESS: validate those links that have been invalidated temporarily
	m_hGraph.validateAllLinks();
	//-B: reset original links' costs (modified into updateGraphValidity)
	m_hGraph.restoreLinkCost();
		
	return bSuccess;
}


void NetMan::updateGraphValidityAndCosts(OXCNode*pOXCSrc, BandwidthGranularity bwd)
{
#ifdef DEBUGB
	cout << endl << "-> updateGraphValidityAndCosts" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itrL, itrL2;
	UniFiber*itrUniFiber;
	SimplexLink*pSLink;
	BinaryHeap<SimplexLink*, vector<SimplexLink*>, PSimplexLinkComp> simplexLinkSorted;
	list<AbstractNode*>::const_iterator itrN;
	OXCNode*itrOXC;

	//-B: consider all the other nodes of the network
	for (itrN = this->m_hWDMNet.m_hNodeList.begin(); itrN != this->m_hWDMNet.m_hNodeList.end(); itrN++)
	{
		//select a node
		itrOXC = (OXCNode*)(*itrN);

#ifdef DEBUGB
		/*
		cout << "\tNode ID: ";
		cout.width(2); cout << itrOXC->getId();
		if (itrOXC->getGrooming() == OXCNode::NoGrooming && itrOXC->getConversion() == OXCNode::WavelengthContinuous)
		{
			cout << " - NO GROOMING" << endl;
		}
		else
		{
			cout << " - GROOMING" << endl;
		}
		*/
#endif // DEBUGB		//if the selected node is not the source node (already treated)
		
		//scan all outgoing fibers from the selected node
		for (itrL = itrOXC->m_hOLinkList.begin(); itrL != itrOXC->m_hOLinkList.end(); itrL++)
		{
		  //select an outgoing fiber
		  itrUniFiber = (UniFiber*)(*itrL);
		  //if the selected fiber currently hosts at least a connection over one of its channels
		  if (itrUniFiber->getUsedStatus() == 1)
		  {
#ifdef DEBUGB
			  cout << "\tFIBER: " << itrUniFiber->getId() << " (" << itrUniFiber->getSrc()->getId()
				  << "->" << itrUniFiber->getDst()->getId() << ")" << endl;
#endif // DEBUGB
			//scan all simplex links of the graph and...
			for (itrL2 = this->m_hGraph.m_hLinkList.begin(); itrL2 != this->m_hGraph.m_hLinkList.end(); itrL2++)
			{
				pSLink = (SimplexLink*)(*itrL2);
				//...if the selected simplex link is a channel (with a valid UniFiber pointer and a valid freeCap value) and...
				if (pSLink->getSimplexLinkType() == SimplexLink::LT_Channel)
				{
					//...if the selected simplex link belongs to the selected outgoing fiber
					if (pSLink->m_pUniFiber->getId() == itrUniFiber->getId())
					{
						//IF THE SELECTED NODE IS A (!) NO (!) GROOMING/(!) NO (!) WAVEL CONVERSION NODE
						if (itrOXC->getGrooming() == OXCNode::NoGrooming && itrOXC->getConversion() == OXCNode::WavelengthContinuous)
						{
							//IF THE SELECTED NODE IS THE SOURCE NODE
							if (itrOXC->getId() == pOXCSrc->getId())
							{
								//-B: STEP 1 - invalidate all outgoing used channels from each outgoing fiber from the source node	
								//if it already hosts a connection
								if (pSLink->m_hFreeCap < OCLightpath)
								{
									//invalidate the selected simplex link
									pSLink->invalidate();
#ifdef DEBUGB
									cout << "\t\tInvalido ch: " << pSLink->m_nChannel << " - free cap: " << pSLink->m_hFreeCap << " (no free)" << endl;
#endif // DEBUGB
								}
							}
							//IF THE SELECTED NODE IS (!) NOT (!) THE SOURCE NODE
							else
							{
								//if it is correct to put < OCLightpath, there is no need to distinguish case source node from other nodes
								if (pSLink->m_hFreeCap < OCLightpath) //if (pSLink->m_hFreeCap < bwd)
								{
									//invalidate the selected simplex link because of capacity
									pSLink->invalidate();
#ifdef DEBUGB
									cout << "\t\tInvalido ch: " << pSLink->m_nChannel << " - free cap: " << pSLink->m_hFreeCap << " (no free)" << endl;
#endif // DEBUGB
								}
							}
						}
						//-B: IF THE SELECTED NODE IS A GROOMING/WAVEL CONVERSION NODE (EITHER SOURCE NODE OR ANY OTHER NODE)
						else if (itrOXC->getGrooming() == OXCNode::FullGrooming && itrOXC->getConversion() == OXCNode::FullWavelengthConversion)
						{
							//-B: STEP 2 - invalidate all outgoing channels, from each outgoing fiber (with used_satus == true) from this source node,
							//	that (-> channels) haven't enough capacity to host the connection
							if (pSLink->m_hFreeCap < bwd)
							{
								//invalidate the selected simplex link because of capacity
								pSLink->invalidate();
#ifdef DEBUGB
								cout << "\t\tInvalido ch: " << pSLink->m_nChannel << " - free cap: " << pSLink->m_hFreeCap << " (no cap)" << endl;
#endif // DEBUGB
							}
							else //if the selected simplex link channel has enough free capacity
							{
								//-B: For each outgoing fiber, put all the channels (those that have enough capacity) in a BynaryHeap
								//	sorted by their free capacity in an increasing order
								simplexLinkSorted.insert(pSLink);
#ifdef DEBUGB
								//cout << "\t\tInserisco nella sorted list il ch: " << pSLink->m_nChannel << endl;
#endif // DEBUGB
							}
						}//end IF grooming node
					}
					//if the selected simplex link doesn't belong to the selected outgoing fiber
					else 
					{
						//go to the next simplex link
					}
				}
				//if the selected simplex link is not of type Channel
				else
				{
					NULL;//go to the next simplex link
				}
			}//end FOR simplex links of the graph

			 //-B: at this time we have inserted all the valid simplex links channel belonging to the same fiber in the BynaryHeap simplexLinkSorted
#ifdef DEBUGB
			//the list's size should be = 0 for NO grooming nodes and = x for grooming nodes
			if (simplexLinkSorted.m_hContainer.size() > 0)
				cout << "\tSorted link list size: " << simplexLinkSorted.m_hContainer.size() << endl;
#endif // DEBUGB

			//-------------------------------- BEST FIT ALGORITHM 4 GROOMING -------------------------------------
			LINK_COST costAssigned = 0;
			LINK_CAPACITY cap = 0;
			//-B: STEP 2 - assign increasing costs to simplex links in the sorted list, according to their free capacity
			while (!simplexLinkSorted.empty())
			{
				//select the simplex link with the minimum cost
				pSLink = simplexLinkSorted.peekMin();
				//erase it from the sorted list
				simplexLinkSorted.popMin();
				//if its free capacity is effectively greater than previous link's free capacity
				if (pSLink->m_hFreeCap > cap)
				{
					//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					//increase link's cost by 2
					//	(because for fronthaul connections we assign a cost = 1 to simplex links Converter and 0 to Channel_Bypass,
					//	so, with this step/increase, we guarantee that best-fit algorithm is applied however
					//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					costAssigned++;
				}//else: they will have the same cost

				//-------- ASSIGN AN INCREASING COST --------
				pSLink->modifyCost(costAssigned); //-B: !!!Ricorda che qui viene modificata anche la var bool m_bCostSaved, nel caso la utilizzassi!!!

#ifdef DEBUGB
				//print only "interesting" channels, 
				if (pSLink->m_hFreeCap < OCLightpath)
				{
					cout << "\t\tSimplexLink ch " << pSLink->m_nChannel	<< " - freecap: "
						<< pSLink->m_hFreeCap << " - cost = " << pSLink->getCost() << endl;
				}
#endif // DEBUGB

				//simplexLinkSorted.buildHeap(); //commented because it should be useless, because I don't modify anything in this cycle
				//save free capacity of the selected simplex link channel
				cap = pSLink->m_hFreeCap;
			}//end WHILE simplexLinkSorted
		  }
		}//end FOR outgoing fibers
	}//end FOR node list

#ifdef DEBUGB
	cout << endl;
#endif // DEBUGB
}


// -B: originally taken from PAC_DPP_NewCircuit
// Construct a circuit
list<AbstractLink*> NetMan::BBU_NewCircuit(Circuit& hCircuit, const list<AbstractLink*>& hLinkList, UINT channel, Connection*pCon)
{
#ifdef DEBUG
	cout << "\n-> BBU_NewCircuit" << endl;
#endif // DEBUGB

	//-B: in case it has been established that it is better to build a new lightpath
	if (hCircuit.m_hRoute.size() > 0)
	{
		//remove all lightpaths that were added in lightpaths' list of the circuit
		hCircuit.m_hRoute.clear();
	}
	
	list<AbstractLink*>::const_iterator itr = hLinkList.begin();
	list<AbstractLink*> pathToReturn; 

#ifdef DEBUG
	cout << "\thPrimaryPath (size: " << hLinkList.size() << "): ";
	while (itr != hLinkList.end())
	{
		SimplexLink *pLink = (SimplexLink*)(*itr);
		cout << "ch " << pLink->m_nChannel << "(type:" << pLink->getSimplexLinkType() << ") + ";		
		itr++;
	}
	cout << endl;
	itr = hLinkList.begin();
#endif // DEBUGB
	
	while (itr != hLinkList.end())
	{
		SimplexLink *pLink = (SimplexLink*)(*itr);
		assert(pLink);
		//-B: save source vertex to add a simplex link Lightpath
		Vertex*pVSrc = (Vertex*)pLink->getSrc();
		Vertex*pAVSrc = m_hGraph.lookUpVertex(pVSrc->m_pOXCNode->getId(), Vertex::VertexType::VT_Access_Out, -1);

#ifdef DEBUGC
		cout << "\tVertex del nodo src " << pAVSrc->m_pOXCNode->getId() << endl;
#endif // DEBUGB

		//-B: lightpath's cost to be computed
		LINK_COST cost = 0;
		//-B: lightpath's length to be computed
		float length = 0;

		switch (pLink->m_eSimplexLinkType)
		{
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_UniFiber:
			{

				//-B: terzo param del costruttore: capacità; se non viene esplicitato lo prende dalla dichiarazione del costruttore => OCLightpath
				// 0 indicates it's a new lightpath to be set up

				Lightpath *pLightpath = new Lightpath(0, Lightpath::LPT_PAC_Unprotected);
				pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;

				pLightpath->wlAssigned = pLink->m_nChannel; //channel

				pLightpath->m_hConnectionsUsingLightpath.push_front(pCon->m_nSequenceNo, pCon);
#ifdef DEBUGC
				cout << "Connection #" << pCon->m_nSequenceNo << " added on the lightpath" << endl;
#endif
				while (itr != hLinkList.end())
				{
					pLink = (SimplexLink*)(*itr);
					assert(pLink);

#ifdef DEBUGC
					cout << "\tWHILE: Vertex del nodo src " << pAVSrc->m_pOXCNode->getId() << endl;
#endif // DEBUGB

					SimplexLink::SimplexLinkType eSLType = pLink->m_eSimplexLinkType;
					if ((SimplexLink::LT_Channel == eSLType) || (SimplexLink::LT_UniFiber == eSLType))
					{
			

						if (pLink->m_nChannel == pLightpath->wlAssigned)
						{
							// consume bandwidth along this physical link --> -B: removed since I'm gonna consume bandwidth in updateLinkCapacity method 
							//	almost at the end of the provisioning, after choosing wavelength to be used
							cost += 0.5 * pLink->getCost(); //0.5 * #hops (since each 1-hop link has cost = 1)
							length += pLink->getLength();

							//-B: assign a channel for each link (unifiber) crossed
							pLightpath->wlsAssigned.push_back(pLink->m_nChannel); //channels vector //-B: --> SHOULD BE UNUSED

							// construct lightpath route
							assert(pLink->m_pUniFiber);
							pLightpath->m_hPRoute.push_back(pLink->m_pUniFiber);

							//-B: add the pointer to the lightpath to the simplex link LT_Channel
							pLink->m_pLightpath = pLightpath;

#ifdef DEBUGC
							cout << "\tAggiungo la fibra " << pLink->m_pUniFiber->getId() << " al lightpath: "
								<< pLink->m_pUniFiber->getSrc()->getId() << "->" << pLink->m_pUniFiber->getDst()->getId();
							cout << endl;
#endif // DEBUGB

							//-B: update Lightpath.m_hPPhysicalPAth->m_hLinkList -> NOT USED
							//	only if it's a new lightpath! If it's not, it will use the same links of the lightpath previously created
							if (pLightpath->getId() == 0)
							{
								pLightpath->updatePhysicalPath(pLink->m_pUniFiber); //-B: added by me
							}
						}
						else
						{
							//change of wavelength -> need of a new lightpath
							itr--;
							pLink = (SimplexLink*)(*itr); //-B: FONDAMENTALE CAPRA!!!!!!!
							break; //exit from while cycle
						}
					}
					else if (SimplexLink::LT_Lightpath == eSLType)
					{
#ifdef DEBUGC
						cout << "\tE' un simplex link LT_Lightpath: " << pLink->getId()
							<< " (" << ((Vertex*)pLink->getSrc())->m_pOXCNode->getId()
							<< "->" << ((Vertex*)pLink->getDst())->m_pOXCNode->getId() << ") - torno al simplex link precedente" << endl;
#endif // DEBUGB
						itr--; //-B: because the increment is done at the end of the cycle 
						pLink = (SimplexLink*)(*itr); //-B: FONDAMENTALE CAPRA!!!!!!!
#ifdef DEBUGC
						cout << "\tDovrebbe essere un simplex link di tipo LT_Grooming: " << pLink->getId()
							<< " (" << ((Vertex*)pLink->getSrc())->m_pOXCNode->getId()
							<< "->" << ((Vertex*)pLink->getDst())->m_pOXCNode->getId() << ")";
#endif // DEBUGB
						break; //exit from while cycle
					}
					else if (SimplexLink::LT_Grooming == eSLType)
					{
						itr--; //-B: because the increment is done at the end of the cycle 
						pLink = (SimplexLink*)(*itr);
						break; //exit from while cycle
					}
					itr++;
				}//end WHILE

				//-B: save dst vertex to add a simplex link lightpath
				Vertex*pVDst = (Vertex*)pLink->getDst();
				Vertex*pAVDst = m_hGraph.lookUpVertex(pVDst->m_pOXCNode->getId(), Vertex::VertexType::VT_Access_In, -1);

#ifdef DEBUGC
				cout << "\tVertex del nodo dst " << pAVDst->m_pOXCNode->getId() << endl;
#endif // DEBUGB

#ifdef DEBUGC
				checkSrcDstLightpath(pLightpath);
#endif // DEBUGB
				///////////////////////
				// appendLightpath will add to the state graph a link corresponding to this lightpath
				hCircuit.addVirtualLink(pLightpath);

				//-B: set cost for new simplex link LT_Lightpath
				if (BBUPOLICY == 0 || BBUPOLICY == 2 || BBUPOLICY == 3) //minHotels
				{
					cost = cost;
					cout << "\tCosto del nuovo lightpath: "<<cost << endl;
				}
				else if (BBUPOLICY == 1) //minLightpaths or minFibers
				{
					cost = SMALL_COST;
				}
				else
				{
					assert(false);
				}

				//-B: ADD NEW SIMPLEX LINK LT_Lightpath TO THE GRAPH
				int sLinkId = this->m_hGraph.getNextLinkId();
				SimplexLink *pNewLink = m_hGraph.addSimplexLink(sLinkId, pAVSrc, pAVDst, cost, //oppure 1, oppure cost
					length, NULL, SimplexLink::LT_Lightpath, pLightpath->wlAssigned, pLightpath->m_nCapacity);
				//(pLightpath->m_nCapacity - hCircuit.m_eBW)); //DECREASING MOVED TO Unprotected_setUpLightpath
				assert(pNewLink && pNewLink->getSrc() && pNewLink->getDst());

				//-B: link also the simplex link LT_Lightpath to the lightpath using it
				pNewLink->m_pLightpath = pLightpath;
				///////////////////////

				pathToReturn.push_back(pNewLink);

				//update the number of connections which uses the link
				list<UniFiber*>::const_iterator itrFiber;
#ifdef DEBUGC
				cout << "\t Update number of connections on every link" << endl;
#endif

#ifdef DEBUGB
				checkSrcDstSimplexLinkLp(pNewLink, pLightpath);
#endif // DEBUGB
				
#ifdef DEBUGC
				cout << "\tHo aggiunto al grafo il simplex link LT_Lightpath " << pNewLink->getId()
					<< " che va dal vertex di tipo " << pAVSrc->m_eVType << " del nodo " << pAVSrc->m_pOXCNode->getId()
					<< " al vertex di tipo " << pAVDst->m_eVType << " del nodo " << pAVDst->m_pOXCNode->getId()
					<< " sul canale " << pNewLink->m_nChannel << endl;
				cout << "\tAggiungo lightpath al circuito! ";
				cout << pLightpath->getSrc()->getId() << "->" << pLightpath->getDst()->getId() << endl;
#endif // DEBUGB

				break;
			} //end case
			case SimplexLink::LT_Lightpath:
			{


				assert(pLink->m_pLightpath);
				pLink->m_pLightpath->m_nBWToBeAllocated = hCircuit.m_eBW;
				//-B: prova (dato che nBWToBeAllocated dovrebbe servire per consumare banda sui simplex link di tipo channel)
				//pLink->m_hFreeCap = pLink->m_hFreeCap - hCircuit.m_eBW; //DECREMENTO SPOSTATO IN Unprotected_setUpCircuit

				//-B: check because it could happen (for choice or for error) that some simplex link LT_Lightpath
				//	are completely free but still in the graph. In that case we cannot talk about grooming
				//	(if freecap < OCLightpath it means that there are already routed connection in that link -> so we are aggregating)
				if (pLink->m_hFreeCap < OCLightpath)
				{
					//pCon->grooming++; //-B: COMMENTED SINCE A CONNECTION CAN BE ROUTED OVER A SIMPLEX LINK LIGHTPATH ALREADY USED
										//	BUT WITHOUT PERFORMING GROOMING. THIS HAPPENS WHEN THE LIGHTPATH IS ALREADY USED BY A CONNECTION
										//	WITH THE SAME SOURCE NODE!!! -> multiplexing! -> NOT SURE IT CANNOT BE CONSIDERED AS GROOMING
				}
#ifdef DEBUGC
				cout << "\tLIGHTPATH " << pLink->m_pLightpath->getId() << "(" << pLink->m_pLightpath->getSrc()->getId()
					<< "->" << pLink->m_pLightpath->getDst()->getId() << ") - freecap "
					<< pLink->m_pLightpath->getFreeCapacity() << " (non ancora consumata)" << endl;
				cout << "\tAggiungo lightpath " << pLink->m_pLightpath->getId() << " (puntato dal simplex link LT_Lightpath) al circuito! ";
				cout << pLink->m_pLightpath->getSrc()->getId() << "->" << pLink->m_pLightpath->getDst()->getId() << endl;
#endif // DEBUGB
				pLink->m_pLightpath->m_hConnectionsUsingLightpath.push_front(pCon->m_nSequenceNo, pCon);
#ifdef DEBUGC
				cout << "Connection #" << pCon->m_nSequenceNo << " added on the lightpath" << endl;
#endif

				pathToReturn.push_back(pLink);

				hCircuit.addVirtualLink(pLink->m_pLightpath);
				break;
			}
			case SimplexLink::LT_Grooming:
			{
				//-B: IF A CONNECTION IS ROUTED THROUGH 2 CONSECUTIVE SIMPLEX LINKS LT_Lightpath IT WILL PASS THROUGH A LT_Grooming -> IT IS GROOMING!
				//-B: IF A CONNECTION HAS A SIMPLEX LINK LT_Lightpath AS PART OF ITS PATH (!!!BUT NOT AS FIRST LINK OF ITS PATH!!!) -> IT IS GROOMING!
				//pCon->grooming++;
				//-B: ATTENTION! IT DOESN'T WORK CORRECTLY! FOR INSTANCE, IF WE HAVE A LIGHTPATH FOR THE FIRST PART OF THE PATH
				//	AND THEN WE HAVE A SIMPLEX LINK LT_Channel, THE PATH WILL PASS THROUGH A SIMPLEX LINK LT_Grooming
				//	BUT THIS IS NOT GROOMING!!!!!!!!
				//-B: UPDATE 
				//ACCORDING TO FRANCESCO IT COULD BE CONSIDERED GROOMING......!!!
				break;
			}
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Mux:
			case SimplexLink::LT_Demux:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Converter:
				NULL;   // No need to do anything for these casesb
				break;
			default:
				DEFAULT_SWITCH;
		} // end switch
		if (itr != hLinkList.end())
			itr++;
#ifdef DEBUGC
		cout << "\t---------------" << endl;
#endif // DEBUGB

	} // end while

#ifdef DEBUGC
	if (hCircuit.m_hRoute.size() > 1)
	{	
		cout << "\tALMENO 2 LIGHTPATH PER FORMARE IL CIRCUITO!!!" << endl;
	}
	if (pCon->grooming > 0)
	{
		cout << "\tGROOMING HAPPENED!" << endl;
		cin.get();
	}
#endif // DEBUGB

	return pathToReturn;
}


//-B: added by me
//	update the channels' capacity comparing states between Unifiber (its channels) and graph's simplex Link (type LT_Channel)
void NS_OCH::NetMan::updateLinkCapacity(Lightpath*pLightpath, int wlSelected, UniFiber* fiber)
{
#ifdef DEBUGC
	cout << "-> updateLinkCapacity" << endl;
#endif // DEBUGB

	SimplexLink* sLink;
	int w = 0;
	Channel*ch; //-B: pointer declaration

#ifdef DEBUGC
		cout << "\t---> FIBER " << fiber->getId() << " (" << fiber->getSrc()->getId()
			 << "->" << fiber->getDst()->getId() << ")" << endl;
#endif // DEBUG

	//-B: scorro tutti i canali della fibra
	while (w < fiber->m_nW)
	{
		ch = &(fiber->m_pChannel[w]); //-B: pointer declaration to another pointer
		if (w == wlSelected)
		{
			sLink = m_hGraph.lookUpSimplexLink(fiber->getSrc()->getId(), Vertex::VT_Channel_Out, w, fiber->getDst()->getId(), Vertex::VT_Channel_In, w);
#ifdef DEBUGC
			cout << "\t---> CANALE " << ch->m_nW << endl;
			cout << "\tBanda da allocare: " << pLightpath->m_nBWToBeAllocated << " - free cap simplex link before updating: " << sLink->m_hFreeCap << endl;
			//assert(sLink->getValidity() == true); //-B: it can be !valid when the sLink belongs to a lightpath already active
#endif // DEBUG

			//-B: -------------------- CONSUME SIMPLEX LINK BANDWIDTH------------------------------------
			sLink->m_nBWToBeAllocated = pLightpath->m_nBWToBeAllocated; //pLightpath->m_nBWToBeAllocated was set in BBU_NewCircuit or in wpBBU_ComputeRoute
			sLink->consumeBandwidth(sLink->m_nBWToBeAllocated);
			sLink->m_nBWToBeAllocated = 0; //-B: reset value

#ifdef DEBUGC
			cout << "\tBanda da consumare: " << pLightpath->m_nBWToBeAllocated 
				<< " - updated free cap simplex link = " << sLink->m_hFreeCap << endl;
#endif // DEBUG

				// ----------------- UPDATE FREE STATUS AND WL'S COSTS ---------------
				ch->m_bFree = false;
				fiber->wlOccupation[w] = UNREACHABLE;
			//}
			break; //-B: FONDAMENTALE! Without it (the selected channel) w != fiber->m_pChannel[w].m_nW --> DON'T KNOW WHY!
				   //	senza di esso, uscirei dal ciclo while con w = 40 e con ch = fiber->m_pChannel[39], quindi ch.m_nW = 39
				   //	ma non capisco perchè poi questo va ad influenzare altro --> QUINDI LASCIARE L'ISTRUZIONE break!
		}
		w++;
	}
}

bool NS_OCH::NetMan::checkFreedomSimplexLink(UINT srcId, UINT dstId, int w)
{
	SimplexLink*sLink;
	sLink = m_hGraph.lookUpSimplexLink(srcId, Vertex::VT_Channel_Out, w, dstId, Vertex::VT_Channel_In, w);
	if (sLink->m_hFreeCap == OCLightpath)
		return true;
	return false;
}

bool NS_OCH::NetMan::checkAvailabilitySimplexLink(UINT srcId, UINT dstId, int w, UINT nBW)
{
	SimplexLink*sLink;
	sLink = m_hGraph.lookUpSimplexLink(srcId, Vertex::VT_Channel_Out, w, dstId, Vertex::VT_Channel_In, w);
	if (sLink->m_hFreeCap >= nBW)
		return true;
	return false;
}

inline bool NetMan:: BBU_Provision_OCLightpath(Connection *pCon, Circuit & hPCircuit, Vertex * pSrc, Vertex * pDst)
{
#ifdef DEBUGB
	cout << "-> BBU_Provision_OCLightpath" << endl;
	cout << "\tShortest path (vertex): " << pSrc->getId() << "->" << pDst->getId() << endl;
#endif // DEBUGB
	
	assert(OCLightpath == pCon->m_eBandwidth);
	//-B: since not initialized
	m_hGraph.numberOfChannels = m_hWDMNet.numberOfChannels;
	//OXCNode *pOXCSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
	//OXCNode *pOXCDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);
	AbstractNode*pOXCSrc = m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
	AbstractNode*pOXCDst = m_hWDMNet.lookUpNodeById(pCon->m_nDst);
	//assert(pOXCSrc && pOXCDst);

	cout << "\tShortest path (con/oxc/ver): " << pCon->m_nSrc << "/" << pOXCSrc->getId() << "/" << pSrc->getId() 
		<< "->" << pCon->m_nDst << "/" << pOXCDst->getId() << "/" << pDst->getId() << endl;
	
	// apply Dijkstra's algorithm to the reachability graph ang get the shortest path
	//m_hWDMNet.genReachabilityGraphOCLightpath(hWLGraph, this);
	m_hGraph.invalidateSimplexLinkDueToCap(pCon->m_eBandwidth);
	if (evList->wpFlag)
	{
		m_hGraph.invalidateSimplexLinkDueToWContinuity(pCon->m_nSrc, pCon->m_eBandwidth);
	}
	list<AbstractLink*> hPath1; //-B: needed for case VWP
	//AbsPath*pBestP = new AbsPath(); //-B: needed for case WP
	UINT channel = -1;
	LINK_COST hCost;
	//-B: ****************************************** OPTION 1 **********************************
	hCost = m_hGraph.Dijkstra(hPath1, pSrc, pDst, AbstractGraph::LCF_ByOriginalLinkCost);


	//-B: ****************************************** OPTION 2 **********************************
	//AbstractPath hPPath;
	//LINK_COST hCost = hWLGraph.Dijkstra(hPPath, pCon->m_nSrc, pCon->m_nDst, AbstractGraph::LCF_ByOriginalLinkCost);
	// convert to paths in the state graph to feed to PAC_DPP_NewCircuit
	//PAC_DPP_Provision_OCLightpath_Helper(hPath1, hPPath);

#ifdef DEBUGB
	cout << "\tCosto shortest path (con/oxc/ver): " << pCon->m_nSrc << "/" << pOXCSrc->getId() << "/" << pSrc->getId()
		<< "->" << pCon->m_nDst << "/" << pOXCDst->getId() << "/" << pDst->getId() << " = " << hCost << endl;
#endif // DEBUGB

	if (UNREACHABLE == hCost) {
		pCon->m_eStatus = Connection::DROPPED;
		pCon->m_bBlockedDueToUnreach = true;
		return false;
	}
	assert(hPath1.size() > 0); //-B

	//-B: case WP -> select random 
	if (evList->wpFlag)
	{
		channel = m_hGraph.WPrandomChannelSelection(pCon->m_nSrc, pCon->m_nDst, pCon->m_eBandwidth);
		cout << "\n\tCANALE SELEZIONATO CASUALMENTE: " << channel << endl << endl;
	}

	// ---------------------- BUILD THE CIRCUIT -----------------
	BBU_NewCircuit(hPCircuit, hPath1, channel, pCon);

	// -B: -----------SETUP THE CIRCUIT-----------
	hPCircuit.Unprotected_setUpCircuit(this);
	
	// STEP 5: Associate the circuit w/ the connection
	pCon->m_pPCircuit = &hPCircuit;

	// STEP 6: Update status
	pCon->m_eStatus = Connection::SETUP;
	m_hGraph.validateAllLinks();
	
	return true;
}

//-B: print some channels' data of all fibers
void NS_OCH::NetMan::printChannelReference()
{
#ifdef DEBUGC
	cout << "-> PrintChannelReference" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itrLink;
	SimplexLink*sLink;
	for (itrLink = m_hWDMNet.m_hLinkList.begin(); itrLink != m_hWDMNet.m_hLinkList.end(); itrLink++)
	{
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		assert(pUniFiber);
		//-B: print data only if the fiber is used, i.e. at least one channel on the fiber is used
		if (pUniFiber->getUsedStatus() == 1)
		{
#ifdef DEBUGC
			cout << "\tFIBER: " << pUniFiber->getId() << " (" << pUniFiber->getSrc()->getId()
				<< "->" << pUniFiber->getDst()->getId() <<")" << endl;
			cout << "\tCanale\tFreeStatus\tFreeCap" << endl;
#endif
			int w = 0;
			for (w = 0; w < pUniFiber->m_nW; w++)
			{
				sLink = this->m_hGraph.lookUpSimplexLink(pUniFiber->getSrc()->getId(), Vertex::VT_Channel_Out, w, pUniFiber->getDst()->getId(), Vertex::VT_Channel_In, w);
				//-B: print data only belonging to used channels
				if (sLink->m_hFreeCap < OCLightpath) {
#ifdef  DEBUGC
					cout << "\t" << w << "\t" << pUniFiber->m_pChannel[w].m_bFree << "\t\t" << sLink->m_hFreeCap << endl;
#endif //  DEBUG		
				}
			}
		}
	}
}


//-B: print some channels' data of all fibers
void NS_OCH::NetMan::printChannelReferencePast()
{
#ifdef DEBUGB
	cout << "-> PrintChannelReferencePast" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itrLink;
	SimplexLink*sLink;
	for (itrLink = m_hWDMNetPast.m_hLinkList.begin(); itrLink != m_hWDMNetPast.m_hLinkList.end(); itrLink++)
	{
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		assert(pUniFiber);
		//-B: print data only if the fiber is used, i.e. at least one channel on the fiber is used
		if (pUniFiber->getUsedStatus() == 1)
		{
			cout << "\tFiber: " << pUniFiber->getId();
			int w;
			cout << "\n\tCanale\tFreeStatus\tFreeCap" << endl;
			for (w = 0; w < pUniFiber->m_nW; w++)
			{
				sLink = this->m_hGraph.lookUpSimplexLink(pUniFiber->getSrc()->getId(), Vertex::VT_Channel_Out, w, pUniFiber->getDst()->getId(), Vertex::VT_Channel_In, w);
				//-B: print data only belonging to used channels
				if (sLink->m_hFreeCap < OCLightpath)
					cout << "\t" << w << "\t" << pUniFiber->m_pChannel[w].m_bFree << "\t\t" << sLink->m_hFreeCap << endl;
			}
		}
	}
}


//-B: print some channels' data of a single fiber
void NS_OCH::NetMan::printChannelReference(UniFiber*pUniFiber)
{
#ifdef DEBUGC
	cout << "\n-> PrintChannelReference" << endl;
 // DEBUGB

	SimplexLink*sLink;
 
	cout << "\tFIBER: " << pUniFiber->getId() << " (" << pUniFiber->getSrc()->getId()
		<< "->" << pUniFiber->getDst()->getId() << ")";

	if (pUniFiber->getUsedStatus() == 1)
	{
		Channel *pChannel = pUniFiber->m_pChannel;
		assert(pChannel);
		int w;

		cout << "\n\tCanale\tFree status\tFreeCap" << endl;
		for (w = 0; w < pUniFiber->m_nW; w++)
		{
			sLink = this->m_hGraph.lookUpSimplexLink(pUniFiber->getSrc()->getId(), Vertex::VT_Channel_Out, w, pUniFiber->getDst()->getId(), Vertex::VT_Channel_In, w);
			if (sLink->m_hFreeCap < OCLightpath)
				cout << "\t" << w << "\t" << pUniFiber->m_pChannel[w].m_bFree << "\t\t" << sLink->m_hFreeCap << endl;
		}

	}
	else
	{
		cout << "\tNOT USED!" << endl;
	}
#endif
}

//-B: print lightpaths referenced by channels, that (channels) are referenced by fibers
void NS_OCH::NetMan::printChannelLightpath()
{
#ifdef DEBUGB
	cout << endl << "-> PrintChannelLightpath" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itrLink;
	int i, w;
	for (itrLink = m_hWDMNet.m_hLinkList.begin(); itrLink != m_hWDMNet.m_hLinkList.end(); itrLink++)
	{
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		assert(pUniFiber);
		//-B: print data only if the fiber is used, i.e. at least one channel on the fiber is used
		if (pUniFiber->getUsedStatus() == 1)
		{
			cout << "Fiber: " << pUniFiber->getId() << endl;
			for (w = 0; w < pUniFiber->m_nW; w++)
			{
				if (pUniFiber->m_pChannel[w].LightpathsIDs.size() > 0)
				{
					cout << "\tCANALE " << w << " - LIGHTPATHS: [";
					for (i = 0; i < pUniFiber->m_pChannel[w].LightpathsIDs.size(); i++)
					{
						cout << pUniFiber->m_pChannel[w].LightpathsIDs[i] << ",";
					}
					cout << "]" << endl;
				}
			}
		}//-B: else, nothing to print
	}
}

//-B: given a certain lightpath, it releases connection's bandwidth over its simplex links
void NetMan::releaseLinkBandwidth(Lightpath*pLightpath, UINT BWToRealease)
{
	list<UniFiber*>::const_iterator itrFiber;
#ifdef DEBUGB
	cout << endl << "-> releaseLinkBandwidth" << endl;
	//cout << "\t------ Il lightpath è composto dalle fibre: ";
	//for (itrFiber = pLightpath->m_hPRoute.begin(); pLightpath->m_hPRoute.end(); itrFiber++)
#endif // DEBUGB

	int w;
	//int found = 0;
	SimplexLink* sLink;
	//-B: scorro tutte le fibre del lightpath
	for (itrFiber = pLightpath->m_hPRoute.begin(); itrFiber != pLightpath->m_hPRoute.end(); itrFiber++)
	{
		//-B: ammetto la possibilità che l'id di un lightpath possa essere presente su più di un canale della stessa fibra
		//-B: scorro tutti i canali di ciascuna fibra
		for (w = 0; w < (*itrFiber)->m_nW; w++)
		{
			//found = 0;
			//-B: scorro tutti i lightpaths' ids salvati per ogni canale
			//for (found = 0, i = 0; i < (*itrFiber)->m_pChannel[w].LightpathsIDs.size() ; i++)
			//{
			//-B: if the lightpath's pointer is valid
			if ((*itrFiber)->m_pChannel[w].m_pLightpath)
			{
				//-B: se l'id considerato è quello del lightpath di cui fare il deprovisioning
				if ((*itrFiber)->m_pChannel[w].m_pLightpath->getId() == pLightpath->getId())
				{
					//-B: trovato il canale su cui passava il lightpath
					//found = 1;
					//prima di andare ad esaminare la fibra successiva rilascio la banda sul simplex link corrispondente
					sLink = m_hGraph.lookUpSimplexLink((*itrFiber)->getSrc()->getId(), Vertex::VertexType::VT_Channel_Out, w,
						(*itrFiber)->getDst()->getId(), Vertex::VertexType::VT_Channel_In, w);
					sLink->m_hFreeCap += BWToRealease;
					int linkID = (*itrFiber)->getId();
				}
			}
		} // end w
	} // end itrFiber
}

//-B: originally taken from PAC_DPP_Provision_OCLightpath_Helper
inline UINT NetMan::BBU_ProvisionHelper_ConvertPath(list<AbstractLink*>& hPath, const AbsPath&hAbsPath)
{
#ifdef DEBUGB
	cout << "-> BBU_ProvisionHelper_ConvertPath" << endl;
#endif // DEBUGB

	SimplexLink *pLink;
	list<AbstractLink*>::const_iterator itr = hAbsPath.m_hLinkList.begin();
	while (itr != hAbsPath.m_hLinkList.end())
	{
		pLink = m_hGraph.lookUpSimplexLink((*itr)->getSrc()->getId(), Vertex::VT_Channel_Out, -1, (*itr)->getDst()->getId(), Vertex::VT_Channel_In, -1);
		assert(pLink);
		hPath.push_back(pLink);
		itr++;
	}
	return hAbsPath.wlAssigned;
}


void NS_OCH::NetMan::linkCapacityDump()
{
cout << "\n-> linkCapacityDump" << endl;

	list<AbstractLink*>::const_iterator itr;
	SimplexLink*sLink;
	int w;
	UniFiber*pFiber;

	for (itr = m_hWDMNet.m_hLinkList.begin(); itr != m_hWDMNet.m_hLinkList.end(); itr++)
	{
		cout << "FIBER ";
		cout.width(3);
		cout << (*itr)->getId();
		cout << " ("; cout.width(3); cout << (*itr)->getSrc()->getId();
		cout << "->"; cout.width(3); cout << (*itr)->getDst()->getId(); 
		cout << ")   FreeCap: ";
		pFiber = (UniFiber*)(*itr);
		for (w = 0; w < pFiber->m_nW; w++)
		{
			sLink = m_hGraph.lookUpSimplexLink((*itr)->getSrc()->getId(), Vertex::VT_Channel_Out, w, (*itr)->getDst()->getId(), Vertex::VT_Channel_In, w);
			if (sLink->m_hFreeCap < pFiber->m_pChannel->m_nCapacity)
			{
				cout.width(3);
				cout << sLink->m_hFreeCap;
			}
			else if (sLink->m_hFreeCap == pFiber->m_pChannel->m_nCapacity)
				cout << "  -";
			else
				cout << "ERR";
			cout << " ";
		}
		cout << " [capacity = " << pFiber->m_pChannel->m_nCapacity << "]" << endl;
	}
}

void NS_OCH::NetMan::simplexLinkDump()
{
#ifdef DEBUGB
	cout << "-> simplexLinkDump" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr;
	UniFiber* fibra;

	//-B: for each link, print its id, num of Primary Working (PW) channels and num of Backup (B) channels
	for (itr = m_hWDMNet.m_hLinkList.begin(); itr != m_hWDMNet.m_hLinkList.end(); itr++)
	{
		//-B: count primary working channels
		int work = 0, j;
		fibra = (UniFiber*)(*itr);
		for (int i = 0; i < fibra->m_nW; i++)
		{
			//occupato e non di backup
			if (fibra->m_pChannel[i].m_bFree == false)
				work++;
			//out<<endl;
		}

		//-B: if at least one channel belonging to this fiber is currently used
		if (getUniFiberFreeCap(fibra) < getUniFiberCap(fibra))
		{
			cout << "Fiber ";
			cout.width(3);
			cout << (fibra)->m_nLinkId;
			cout << " ("; cout.width(3); cout << fibra->getSrc()->getId();
			cout << "->"; cout.width(3); cout << fibra->getDst()->getId();
			cout << ")   "; cout.width(2); cout << work;
			cout << " W\t"; //-B: FW = Full Working = complete

			//-B: ciclo per scorrere tutti i canali
			for (int i = 0; i < fibra->m_nW; i++)
			{
				//-B: if a lightpath uses this channel
				if (fibra->m_pChannel[i].LightpathsIDs.size() > 0)
				{
					cout << " [";
					for (j = 0; j < fibra->m_pChannel[i].LightpathsIDs.size(); j++)
					{
						if ((j + 1) == fibra->m_pChannel[i].LightpathsIDs.size())
							cout << fibra->m_pChannel[i].LightpathsIDs[j];
						else
							cout << fibra->m_pChannel[i].LightpathsIDs[j] << ",";
					}
					cout << "] ";
				} //IF
				else
				{
					cout << " - ";
				}
			}
			cout << "\n";
		}

	}
}

void NS_OCH::NetMan::checkSrcDstSimplexLinkLp(SimplexLink *pLink, Lightpath *pLightpath)
{
	OXCNode*pNodeSrcLp = (OXCNode*)pLightpath->getSrc();
	OXCNode*pNodeDstLp = (OXCNode*)pLightpath->getDst();
	Vertex*pNodeSrcLink = (Vertex*)pLink->getSrc(); //Access_Out
	Vertex*pNodeDstLink = (Vertex*)pLink->getDst(); //Access_In
	assert(pNodeSrcLp->getId() == pNodeSrcLink->m_pOXCNode->getId());
	assert(pNodeDstLp->getId() == pNodeDstLink->m_pOXCNode->getId());
}

LINK_CAPACITY NS_OCH::NetMan::getUniFiberFreeCap(UniFiber*fiber)
{
#ifdef DEBUGB
	//cout << "\t-> getUniFiberFreeCap()" << endl;
#endif // DEBUGB

	LINK_CAPACITY cap = 0;
	SimplexLink* sLink;
	Channel ch;
	for (int w = 0; w < fiber->m_nW; w++)
	{
		ch = (fiber->m_pChannel[w]);
		sLink = m_hGraph.lookUpSimplexLink(fiber->getSrc()->getId(), Vertex::VT_Channel_Out, w, fiber->getDst()->getId(), Vertex::VT_Channel_In, w);
		//cout << "\t\tAggiungo " << sLink->m_hFreeCap << " - ";
		cap += sLink->m_hFreeCap;
	}
	//cout << "\t\tFreecap = " << cap;
	return cap;
}

LINK_CAPACITY NS_OCH::NetMan::getUniFiberCap(UniFiber*fiber)
{
#ifdef DEBUGB
	//cout << "\t-> getUniFiberCap()" << endl;
#endif // DEBUGB

	LINK_CAPACITY cap = 0;
	for (int w = 0; w < fiber->m_nW; w++)
	{
		cap += fiber->m_pChannel[w].m_nCapacity;
	}
	//cout << "\t\tCap = " << cap << endl;
	return cap;
}

//-B: given the source node id, return the less "expensive" BBU_hotel (according to the policy used)
UINT NetMan::findBestBBUHotel(UINT src, BandwidthGranularity&bwd, SimulationTime hTime)
{
#ifdef DEBUG
	cout << "-> findBestBBUHotel" << endl;
#endif // DEBUGB

	OXCNode*pOXCsrc = (OXCNode*)m_hWDMNet.lookUpNodeById(src);
	OXCNode*pOXCdst = NULL;
	assert(pOXCsrc);
	UINT dst = 0, bestBBU = 0;

	if (pOXCsrc->m_nBBUNodeIdsAssignedLast > 0 || pOXCsrc->m_nBBUNodeIdsAssigned > 0) {

		//-B: in case any node can hold at most 1 connection at a time, this STEP should not be possible
		//	(the simulator should never reach this part of this method in case of ONE_CONN_PER_NODE)
		assert(ONE_CONN_PER_NODE == false);

		//-B: in case of FRONTHAUL, we don't have to allocate any bandwidth for connections following the first one (--> they have same source node and dst node as well)
		if (!MIDHAUL) // == if (FRONTHAUL)
		{
			//-B: modify variable's value passed by reference so that in the calling function its value will be OC0
			bwd = OC0;
		}
		// I can't return here: need to perform pre proccesing
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//							*************** PRE-PROCESSING ******************
	//////////// (done at the beginning of the function because it avoids to do it in BBU_ProvisionHelper_Unprotected ////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//-B: it is used to prefer wavelength continuity over w. conversion in a condition of equality
	//	between the wavelength that would allow a wavelength path and any other best-fit wavelegth
	//	(assign a cost equal to 0.5 to simplex links of type LT_Converter and LT_Grooming)
	
	m_hGraph.preferWavelPath();

	//-B: invalidate those simplex links (both LT_Channel and LT_Lightpath) whose free capacity < bwd
	//m_hGraph.invalidateSimplexLinkDueToCap(bwd);
	//-B: invalidate those simplex links (!) LT_Channel (!) whose free capacity < OCLightpath (whose freeStatus = false)
	//invalidateSimplexLinkDueToFreeStatus(); 

	//-B: following function should include both the previous ones -> to reduce computational time
	invalidateSimplexLinkDueToCapOrStatus(bwd, 1);

	//-B: we can't aggregate previous and following functions because we could decide 
	//	to not use the second one (it's up to our assumptions), while the first one is essential
	//-B: invalidate simplex link LT_Grooming and LT_Converter for nodes that don't have active BBUs
	if (ONLY_ACTIVE_GROOMING)
	{
		this->invalidateSimplexLinkGrooming();
	}

	//-B: modify simplex links' costs to perform best fit algorithm in choosing a simplex link LT_Lightpath
	//updateCostsForBestFit(); //-B: TO BE USED ONLY IF BBU_NewCircuit ASSIGNS A COST = 0 TO NEW LIGHTPATHS
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//-B: STEP 0a: check if this source node has an already assigned BBU in one of the hotel nodes

	
	if (pOXCsrc->m_nBBUNodeIdsAssigned == 1 && BBUPOLICY != 3)
	{
		#ifdef DEBUGB
		cout << "\tIl nodo sorgente " << src << " ha gia' la sua BBU collocata nel nodo " << pOXCsrc->m_nBBUNodeIdsAssigned << endl;
		#endif // DEBUGB


		/*-B: PROBLEM: the semplification of the small cells, seen not as real nodes, but as sub-cells belonging to a macro-cell.
		This makes impossible to take into account of extra latency between small cells and a macro cell when the macro-cell
		has already its BBUNodeIdAssigned. In fact, when a connection has a small cell as source, the simulator can only see
		that the source is the macro cell which the small cell belongs to.

		--> SOLVED!!! Added an extraLatency in DijkstraHelperLatency
		*/

		//dst node of this connection will be the BBU hotel node already assigned to this source node
		//	(independently of how many BBUs there are into it)
		return pOXCsrc->m_nBBUNodeIdsAssigned; //-----------------------------------------------------------------------------------------
		}
	

	/*if (BBUPOLICY == 3) {

		// The src has already a BBU assigned but the last link load has been overcome
		// Delete the BBU from the list of hotels
		if (pOXCsrc->m_nBBUNodeIdsAssigned > 0) {

			pOXCdst = (OXCNode*)this->m_hWDMNet.lookUpNodeById(pOXCsrc->m_nBBUNodeIdsAssigned);
			
			if (!this->isLastLinkFull(pOXCdst->pPath)) {
				for (int i = 0; i < this->m_hWDMNet.hotelsList.size(); i++) {
					if (this->m_hWDMNet.hotelsList[i] == pOXCdst) {
						cout << "\tLink's load overcame the threashold: remove BBU from the hotelsList" << endl;
						cin.get();
						this->m_hWDMNet.hotelsList.erase(this->m_hWDMNet.hotelsList.begin() + i);
						break;

					}
				}
			}

		}


	}*/

	//-B: For policies different from policy 3: check if this source node has an already assigned BBU in one of the hotel nodes	
	
	/*
	if (BBUPOLICY == 1 || pOXCsrc->m_bBBUAlreadyChanged == true) {

		if (pOXCsrc->m_nBBUNodeIdsAssigned > 0)
		{	
			
			//Allow the source to change BBU for another time, if the time passed is enough
			if ((hTime - (pOXCsrc->m_hChangeBBUTime)) < BBU_CHANGE_INTERVAL) {
#ifdef DEBUG
				cout << "Last BBU change was done at time " << pOXCsrc->m_hChangeBBUTime << endl;
				cout << "\tIl nodo sorgente " << src << " ha gia' la sua BBU collocata nel nodo "
					<< pOXCsrc->m_nBBUNodeIdsAssigned << " e il tempo passato è di "
					<< hTime - pOXCsrc->m_hChangeBBUTime << endl;
				//cin.get();
#endif // DEBUGB


				
				-B: PROBLEM: the semplification of the small cells, seen not as real nodes, but as sub-cells belonging to a macro-cell.
				This makes impossible to take into account of extra latency between small cells and a macro cell when the macro-cell
				has already its BBUNodeIdAssigned. In fact, when a connection has a small cell as source, the simulator can only see
				that the source is the macro cell which the small cell belongs to.

				--> SOLVED!!! Added an extraLatency in DijkstraHelperLatency
				

				//dst node of this connection will be the BBU hotel node already assigned to this source node
				//(independently of how many BBUs there are into it)
				return pOXCsrc->m_nBBUNodeIdsAssigned; //-----------------------------------------------------------------------------------------
			}
			else {
#ifdef DEBUG
				cout << "Last BBU change was done at time " << pOXCsrc->m_hChangeBBUTime << endl;
				cout << "Time passed from the last change of BBU is " << hTime - pOXCsrc->m_hChangeBBUTime << endl;
				//cin.get();
#endif
			}
		}
	}
	
	*/

	//  If source node is a bbu hotel node...
	//	(but, clearly, it's the first time this node has to route a fronthaul connection.
	//	Otherwise, it should have stopped at the previous step)
	if (pOXCsrc->getBBUHotel())
	{
		//if the source node has already its bbu there and the source has other bbu than itself
		if (pOXCsrc->m_nBBUNodeIdsAssigned == src && pOXCsrc->m_nBBUs > 1) {
			cout << "\tNodo sorgente (" << src << ") è un hotel node quindi lascio qui la sua bbu" << endl;
			return src;
		}
		
		//if the source node has only itself as bbu, i can change bbu and switch this off
		if ((pOXCsrc->m_nBBUNodeIdsAssigned == src && pOXCsrc->m_nBBUs == 1)) {
		
			cout << "\tNodo sorgente (" << src << ") ha solo se stessa come BBU"<< endl;
		}
		else {

			//if the source does not have an already assigned bbu
			// but it is already active, there is no need to look for best bbu hotel node
			if (isAlreadyActive(pOXCsrc))
			{
#ifdef DEBUG
				cout << "\tNodo sorgente (" << src << ") e' gia' un hotel node";
#endif // DEBUGB
				//-B: check availability in terms of max num of active BBUs
				if (checkAvailabilityHotelNode(pOXCsrc))
				{
#ifdef DEBUG
					cout << " -> puo' attivare ancora BBUs" << endl;
#endif // DEBUGB
					pOXCsrc->m_nBBUs++; //-L: increase num of active BBU in this node

					//increase num of nodes whose BBU is in this node -> num of BBUs active in this node

					//if the source was asssigned another bbu but now it can place it at itself
					if (pOXCsrc->m_nBBUNodeIdsAssigned > 0 && pOXCsrc->m_nBBUNodeIdsAssigned != src) {
						pOXCsrc->m_nBBUNodeIdsAssignedLast = src;
						pOXCsrc->m_bBBUAlreadyChanged = true;
						pOXCsrc->m_hChangeBBUTime = hTime;
						pOXCsrc->m_nCountBBUsChanged++;
						return src;
					}
					else {
						pOXCsrc->m_nBBUNodeIdsAssigned = src;
						return src; //-----------------------------------------------------------------------------------------------------------
					}
				}
				else
				{
					;
#ifdef DEBUG
					cout << " -> NON HA PIU' SPAZIO PER ALTRE BBU!" << endl;
#endif // DEBUGB
				}
			}
		}
	}

	//ELSE IF this candidate hotel node is still inactive (does not have any BBU in it)
	//	or is already full of BBUs (or, simply, the source node is not a candidate BBU hotel node)
	// or I want to allocate a second new BBU:
	//	------> check other candidate hotel nodes using a sorted hotels list

	//-B: reset BBUsReachCost = UNREACHABLE for all nodes (potentially modified in the previous iteration)
	//m_hWDMNet.resetBBUsReachabilityCost(); //-B: --> ALREADY DONE BY resetPreProcessing method (inside BBU_newConnection)

	pOXCdst = NULL;
	OXCNode*pOXCdst2 = NULL;
	bool enough = true;
	//LINK_COST costBestActiveHotel = UNREACHABLE;

	//-B: build a list with all bbu hotel node already active (BBUs > 0) with enough "space" to host a new BBU (BBUs < MAXNUMBBU)
	vector<OXCNode*>auxBBUsList;
	genAuxBBUsList(auxBBUsList);

	if (auxBBUsList.size() > 0)
	{
#ifdef DEBUG
		cout << "\tCerco tra i " << auxBBUsList.size() << " hotels già attivi" << endl;
#endif // DEBUGB

		//SEARCH FOR "BEST" BBU AMONG (!) ALREADY ACTIVATED (!) BBU HOTEL NODES
		switch (BBUPOLICY)
		{
			//-B: GENERAL ADVICE: BE CAREFUL USING GLOBAL VARIABLE precomputedCost AND precomputedPath INSIDE placeBBU METHODS
			//	(since they will be used in BBU_ProvisionHelper_Unprotected)
			case 0:
				//(in this function the core CO is preferred over the others as BBU hotel node)
				bestBBU = placeBBUHigh(src, auxBBUsList);
				break;
			case 1:
				bestBBU = placeBBUClose(src, auxBBUsList);
				enough = false;
				break;
			case 2:
				bestBBU = placeBBU_Metric(src, auxBBUsList);
				enough = false;
				break;
			case 3:
				bestBBU = placeBBUHigh(src, auxBBUsList);
				break;
			default:
				DEFAULT_SWITCH;
		}

		//if a valid bbu hotel node has been selected
		if (bestBBU > 0)
			pOXCdst = (OXCNode*)this->m_hWDMNet.lookUpNodeById(bestBBU);
	}


	if (bestBBU == 0)
	{
		//-B: it means no valid dst hotel node was found in the algorithm placeBBUX
		enough = false;
	}

#ifdef DEBUG
		//pOXCDst could be NULL
		if (pOXCdst)
			cout << "\tPath's cost towards best BBU hotel node, already active: " << pOXCdst->m_nBBUReachCost << endl << endl;
#endif // DEBUGB


	//-B: STEP 3 - check still inactive candidate BBU hotel nodes
	if (!enough)
	{
		//-B: build inactive candidate BBU hotel nodes list
		//(for policy #2, the hotels list was reduced before starting the simulation, in buildBestHotelsList() method)
		vector<OXCNode*> inactiveBBUs;
		buildNotActiveBBUsList(inactiveBBUs);
		
		//if there is at least an INactive BBU hotel node
		if (inactiveBBUs.size() > 0)
		{
#ifdef DEBUG
			cout << "\tCerco tra i " << inactiveBBUs.size() << " hotels ancora inattivi" << endl << endl;
#endif // DEBUGB
	
			//SEARCH FOR "BEST" BBU AMONG (!) NOT YET ACTIVATED (!) BBU HOTEL NODES
			switch (BBUPOLICY)
			{
				case 0: //1st algorithm
					//	(in this function the core CO is preferred over the others as BBU hotel node)
					dst = placeBBUHigh(src, inactiveBBUs);
					break;
				case 1: //2nd algorithm
					dst = placeBBUClose(src, inactiveBBUs);
					break;
				case 2: //3rd algorithm
					dst = placeBBU_Metric (src, inactiveBBUs);
					break;
				case 3:
					//	(in this function the core CO is preferred over the others as BBU hotel node)
					dst = placeBBUHigh(src, inactiveBBUs);
					break;
				default:
					DEFAULT_SWITCH;
			}

			//if a valid bbu hotel node has been selected
			if (dst > 0)
				pOXCdst2 = (OXCNode*)this->m_hWDMNet.lookUpNodeById(dst);

#ifdef DEBUG
			//pOXCDst2 could be NULL
			if (pOXCdst2) {
				cout << "\tBest candidate BBU hotel node, still inactive: " << pOXCdst2->getId() << endl << endl;
				//cin.get();
			}
#endif // DEBUGB
		}
	}

	OXCNode *bbuNode = NULL;

	//CASE 1 - use an already active BBU
	if (pOXCdst && pOXCdst2 == NULL)
	{

		//BEST BBU NODE: an already active bbu
		//-B: save best precomputed cost
		precomputedCost = pOXCdst->m_nBBUReachCost;
		//-B: save best precomputed path
		precomputedPath = pOXCdst->pPath;

		if (bestBBU == pOXCsrc->m_nBBUNodeIdsAssigned) {
#ifdef DEBUG
			cout << "\tThe BBU found for node " << src << " is the same it already has" << endl;
#endif
			return bestBBU;
		}

		// Assign a second BBU (which was already active) for the node
		if (pOXCsrc->m_nBBUNodeIdsAssigned >0) {

#ifdef DEBUG		
			cout << "\tChange BBU (with an already active one); BBU: " << bestBBU << endl;
			cout << "\tPrevious BBU is " << pOXCsrc->m_nBBUNodeIdsAssigned << endl;
#endif
			
			//BEST BBU NODE: an already active bbu
			//-B: save best precomputed cost
			precomputedCost = pOXCdst->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst->pPath;

			nodeActivatingNewBBU[pOXCsrc->getId()]++;
			//-B: assign, to the source node of the connection, the hotel node hosting its BBU
			pOXCsrc->m_nBBUNodeIdsAssignedLast = bestBBU;
			pOXCsrc->m_bBBUAlreadyChanged = true;
			pOXCsrc->m_hChangeBBUTime = hTime;

			//-B: increase the number of active BBUs in the thotel node (the value will be more than 1 since it was already activated)
			pOXCdst->m_nBBUs++;
			pOXCsrc->m_nCountBBUsChanged++;

#ifdef DEBUGC
			cout << "Updated the number of bbus for node: " << pOXCdst->getId() << endl;

			cout << "Updated number of BBUs changed to " << pOXCsrc->m_nCountBBUsChanged << endl;
#endif
		
			return bestBBU; //------------------------------------------------------------------------------------------------------
				
		}


		//-B: increase the number of active BBUs in the thotel node (the value will be more than 1 since it was already activated)
		pOXCdst->m_nBBUs++;

		//-B: assign, to the source node of the connection, the hotel node hosting its BBU
		pOXCsrc->m_nBBUNodeIdsAssigned = bestBBU;
		return bestBBU; //------------------------------------------------------------------------------------------------------

	}

	//CASE 2 - activate a new BBU
	if (pOXCdst == NULL && pOXCdst2)
	{
		//BEST BBU NODE: a still inactive bbu
		//-B: save best precomputed cost
		precomputedCost = pOXCdst2->m_nBBUReachCost;
		//-B: save best precomputed path
		precomputedPath = pOXCdst2->pPath;

		//Assign a second BBU (which was inactive) for the node
		if (pOXCsrc->m_nBBUNodeIdsAssigned > 0) {

			if (pOXCsrc->m_nBBUNodeIdsAssigned == pOXCdst2->getId()) {

				cout << "\tThe node has the BBU in itself" << endl;
				return pOXCsrc->m_nBBUNodeIdsAssigned;
			}

#ifdef DEBUG
			cout << "\tChange BBU ( with an inactive one): BBU: " << pOXCdst2->getId() << endl;
			cout << "\tPrevious BBU is " << pOXCsrc->m_nBBUNodeIdsAssigned << endl;
#endif

			
			//BEST BBU NODE: a still inactive bbu
			//-B: save best precomputed cost
			precomputedCost = pOXCdst2->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst2->pPath;
			//-B: add the new hotel node to the list of active hotel node
			this->m_hWDMNet.BBUs.push_back(pOXCdst2);

			//-B: assign, to the source node of the connection, the hotel node hosting its BBU
			pOXCsrc->m_nBBUNodeIdsAssignedLast = dst;
			pOXCsrc->m_bBBUAlreadyChanged = true;
			pOXCsrc->m_hChangeBBUTime = hTime;

			//-B: increase the number of active BBUs in the thotel node (the value will be more than 1 since it was already activated)
			pOXCdst2->m_nBBUs++;

			pOXCsrc->m_nCountBBUsChanged++;
#ifdef DEBUGC
			cout << "Updated the number of bbus for node: " << pOXCdst2->getId() << endl;
			cout << "Updated number of BBUs changed to " << pOXCsrc->m_nCountBBUsChanged << endl;
#endif
			return dst; //----------------------------------------------------------------
		
		}

		this->m_hWDMNet.BBUs.push_back(pOXCdst2);
		pOXCdst2->m_nBBUs++;

		//-B: assign, to the source node of the connection, the hotel node hosting its BBU
		pOXCsrc->m_nBBUNodeIdsAssigned = dst;
		return dst; //---------------------------------------------------------------------------------------------------------
	}

	//-B: STEP 4 - evaluation best Hotel
	//CASE 3 - both are valid --> not possible for policy #0
	if (pOXCdst && pOXCdst2)
	{
		bool alreadyActive = false;
		switch (BBUPOLICY)
		{
		case 1:
		case 2:
		{

			if (pOXCdst->m_nBBUReachCost <= pOXCdst2->m_nBBUReachCost)
			{
				alreadyActive = true;
#ifdef DEBUGB
				cout << "\tReach Cost a favore dell'hotel gia' attivo: " << pOXCdst->m_nBBUReachCost
					<< " vs " << pOXCdst2->m_nBBUReachCost << " -> node: " << bestBBU << endl << endl;
#endif // DEBUGB
			}
			else
			{
				alreadyActive = false;
#ifdef DEBUGB
				cout << "\tReach Cost a favore dell'hotel ancora inattivo: " << pOXCdst->m_nBBUReachCost
					<< " vs " << pOXCdst2->m_nBBUReachCost << " -> node: " << dst << endl << endl;
				//cin.get();
#endif // DEBUGB
			}
			break;

		}
		case 3:
		default:
			DEFAULT_SWITCH;
		}



		//choose better dst node: cost difference
		if (alreadyActive)
		{

			//BEST BBU NODE: an already active bbu
			//-B: save best precomputed cost
			precomputedCost = pOXCdst->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst->pPath;

			// The BBU already assigned to the node is still the optimal one  
			if (bestBBU == pOXCsrc->m_nBBUNodeIdsAssigned) {

				cout << "\tThe BBU already assigned to the node is still the optimal one" << endl;

				// I don't have to increase the number of bbu active at dst
				return bestBBU;
			}

			// The BBU already assigned to the node is no more the optimal one: allocate a new BBU
			if (pOXCsrc->m_nBBUNodeIdsAssigned >0) {

				cout << "\tThe BBU already assigned to the node is no more the optimal one OR the last link was full" << endl;
				cout << "\tSecond bbu: Press enter to continue...." << endl;
				//cin.get();
				nodeActivatingNewBBU[pOXCsrc->getId()]++;
				
				//-B: assign, to the source node of the connection, the hotel node hosting its BBU
				pOXCsrc->m_nBBUNodeIdsAssignedLast = bestBBU;
				pOXCsrc->m_bBBUAlreadyChanged = true;
				pOXCsrc->m_hChangeBBUTime = hTime;

				//-B: increase the number of active BBUs in the thotel node (the value will be more than 1 since it was already activated)
				pOXCdst->m_nBBUs++;

				return bestBBU;
			}

			cout << "\t First BBU assigned to the node" << endl;

			//-B: increase the number of active BBUs in the thotel node (the value will be more than 1 since it was already activated)
			pOXCdst->m_nBBUs++;
			//-B: assign, to the source node of the connection, the hotel node hosting its BBU
			pOXCsrc->m_nBBUNodeIdsAssigned = bestBBU;
			return bestBBU;

		}
		else
		{
			//-B: save best precomputed cost
			precomputedCost = pOXCdst2->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst2->pPath;
			//-B: add the new hotel node to the list of active hotel node
			this->m_hWDMNet.BBUs.push_back(pOXCdst2);

			// Assign a new BBU to the node
			if (pOXCsrc->m_nBBUNodeIdsAssigned >0) {

				cout << "\tThe BBU already assigned to the node is no more the optimal one OR the number of connections has been reached: allocate a new BBU" << endl;
				cout << "\tSecond bbu: Press enter to continue...." << endl;
				//cin.get();
				
				pOXCsrc->m_nBBUNodeIdsAssignedLast = dst;
				pOXCsrc->m_bBBUAlreadyChanged = true;
				pOXCsrc->m_hChangeBBUTime = hTime;

				//-B: increase the number of active BBUs in the thotel node (the value will be 1 since it was not activated yet)
				pOXCdst2->m_nBBUs++;

				return dst;

			}
			else {

				//-B: increase the number of active BBUs in the thotel node (the value will be 1 since it was not activated yet)
				pOXCdst2->m_nBBUs++;

				//-B: assign, to the source node of the connection, the hotel node hosting its BBU
				pOXCsrc->m_nBBUNodeIdsAssigned = dst;
				return dst;
			}
		}
	}
	//-B: POLICY 2 - LAST OPTION BEFORE CO-LOCATING BBU AND RRH IN THE SAME SITE
	//-B: both for fronthaul and midhaul
	if (pOXCdst == NULL && pOXCdst2 == NULL && BBUPOLICY == 2)
	{
#ifdef DEBUGB
		cout << "\tHERE WE ARE!" << endl;
		cin.get();
#endif // DEBUGB

		dst = 0;
		vector<OXCNode*> otherHotels;
		buildHotelsList(otherHotels);
		//if there is at least a hotel
		if (otherHotels.size() > 0)
		{
#ifdef DEBUGB
			cout << "\tOther hotels list size: " << otherHotels.size() << endl;

			vector<OXCNode*>::const_iterator itr;
			OXCNode*pNode;
			for (itr = otherHotels.begin(); itr != otherHotels.end(); itr++)
			{
				pNode = (OXCNode*)(*itr);
				cout << "\t+Hotel: "<<pNode->getId() << endl;
			}
			cin.get();
#endif // DEBUGB

			dst = placeBBU_Metric(src, otherHotels);
		}
		//if a valid bbu hotel node has been selected
		if (dst > 0)
		{
			pOXCdst2 = (OXCNode*)this->m_hWDMNet.lookUpNodeById(dst);
			//BEST BBU NODE: a still inactive bbu
			//-B: save best precomputed cost
			precomputedCost = pOXCdst2->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst2->pPath;
			//-B: add the new hotel node to the list of active hotel node
			this->m_hWDMNet.BBUs.push_back(pOXCdst2);
			this->m_hWDMNet.hotelsList.push_back(pOXCdst2);
			//-B: increase the number of active BBUs in the hotel node (the value will be 1 since it was not activated yet)
			pOXCdst2->m_nBBUs++;
			cout << "\tNumber of BBUs in the hotel chosen(it should be 1): " << pOXCdst2->m_nBBUs << endl;
			//-B: assign, to the source node of the connection, the hotel node hosting its BBU
			pOXCsrc->m_nBBUNodeIdsAssigned = dst;
#ifdef DEBUGB
			cout << "\tScelto come destinazione l'hotel: " << dst << endl;
			cin.get();
#endif // DEBUGB

			return dst;
		} //END if dst
#ifdef DEBUG
		cout << "\tNo hotel chosen as destination!!! I'm gonna choose the CS itself: " << src << endl;
		cin.get();
#endif // DEBUGB
	} //END if

	//-B: STEP 5 - POST-PROCESS: validate those links that have been invalidated temporarily
	//m_hGraph.validateAllLinks();
	//COMMENTED SO THAT WE DON'T HAVE TO DO IT AGAIN IN BBU_ProvisionHelper_Unprotected
	
#ifdef DEBUG
	cout << "\tTUTTI I BBU HOTEL NODES (!) RAGGIUNGIBILI (!) DAL NODO " << src << " SONO PIENI?!" << endl;
	cout << "\tALLORA METTO LA BBU NEL CELL SITE STESSO DELLA SORGENTE " << src << endl;
	cin.get();
	//cin.get();
#endif // DEBUGB


	if (pOXCsrc->m_nBBUNodeIdsAssigned == src) {

		cout << "\tThe node has the BBU in itself" << endl;
		//cin.get();
		return src;
	}

	if (pOXCsrc->m_nBBUNodeIdsAssigned > 0 && pOXCsrc->m_nBBUNodeIdsAssigned != src) {

		nodeActivatingNewBBU[pOXCsrc->getId()]++;

		//-B: assign, to the source node of the connection, the hotel node hosting its BBU
		pOXCsrc->m_nBBUNodeIdsAssignedLast = src;
		pOXCsrc->m_bBBUAlreadyChanged = true;
		pOXCsrc->m_hChangeBBUTime = hTime;
		pOXCsrc->m_nBBUs++;

		assert(pOXCsrc->m_nBBUs == 1);

		return src;

	}

	//NON INSERISCO IL NODO NELLA LISTA this->m_hWDMNet.BBUs!!!
	pOXCsrc->m_nBBUNodeIdsAssigned = src;
	pOXCsrc->m_nBBUs++;
	//-B: THE PATH WILL BE COMPUTED DURING CONNECTION'S PROVISIONING

	//-B: if it has reached this instruction, the algorithm could not find
	//	any valid (REACHABLE) BBU hotel node for source src
	return src;
}

//-B: given the source node id, return the less "expensive" BBU_pool (according to the policy used)
//-B: DIFFERENCE WITH findBestBBUPool_Soft: here we look for the best BBU pooling node
//	each time we have to provide connection for the new arrival
UINT NetMan::findBestBBUPool_Evolved(UINT src, BandwidthGranularity bwd, BandwidthGranularity backhaulBwd)
{
#ifdef DEBUGB
	cout << "-> findBestBBUPool" << endl;
#endif // DEBUGB

	OXCNode*pOXCsrc = (OXCNode*)m_hWDMNet.lookUpNodeById(src);
	assert(pOXCsrc);
	UINT dst = 0, bestBBU = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//							*************** PRE-PROCESSING ******************
	//////////// (done at the beginning of the function because it avoids to do it in BBU_ProvisionHelper_Unprotected ////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//-B: it is used to prefer wavelength continuity over w. conversion in a condition of equality
	//	between the wavelength that would allow a wavelength path and any other best-fit wavelegth
	//	(assign a cost equal to 0.5 to simplex links of type LT_Converter and LT_Grooming)
	m_hGraph.preferWavelPath();
	//-B: invalidate those simplex links (both LT_Channel and LT_Lightpath) whose free capacity < bwd
	m_hGraph.invalidateSimplexLinkDueToCap(bwd);
	//-B: invalidate those simplex links (!) LT_Channel (!) whose free capacity < OCLightpath (whose freeStatus = false)
	invalidateSimplexLinkDueToFreeStatus();
	//-B: invalidate simplex link LT_Grooming and LT_Converter for nodes that don't have active BBUs
	if (ONLY_ACTIVE_GROOMING)
	{
		this->invalidateSimplexLinkGrooming();
	}
	//-B: modify simplex links' costs to perform best fit algorithm in choosing a simplex link LT_Lightpath
	//updateCostsForBestFit(); //-B: TO BE USED ONLY IF BBU_NewCircuit ASSIGNS A COST = 0 TO NEW LIGHTPATHS
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//-B: STEP 0a: it doesn't matter if this source node has an already assigned BBU in one of the hotel nodes

	//-B: STEP 0b - if source node is a bbu hotel node...
	if (pOXCsrc->getBBUHotel())
	{
		this->m_hWDMNet.computeTrafficProcessed_BBUPooling(pOXCsrc, this->getConnectionDB().m_hConList);
		//...and it is already active, there is no need to look for best bbu hotel node
		if (pOXCsrc->m_dTrafficProcessed > 0)
		{
#ifdef DEBUGB
			cout << "\tNodo sorgente (" << src << ") e' gia' un hotel node";
#endif // DEBUGB
			//-B: if this BBU pool node has still some available processing resources
			if (checkResourcesPoolNode(pOXCsrc, backhaulBwd))
			{
#ifdef DEBUGB
				cout << " -> puo' accogliere ancora traffico" << endl;
#endif // DEBUGB
				//increase num of nodes whose BBU is in this node -> num of BBUs active in this node
				pOXCsrc->m_nBBUs++;
				
				//-B: with this kind of 1:N RRH-BBU assignment it seems unnecessary
				pOXCsrc->m_nBBUNodeIdsAssigned = src;
				
				//-B: RETURN WITHOUT CHECKING THE REACHABILITY BECAUSE SRC AND DST COINCIDES
				return src; //-----------------------------------------------------------------------------------------------------------
			}
			else
			{
				;
#ifdef DEBUGB
				cout << " -> RISORSE DI QUESTO BBU POOL ESAURITE!" << endl;
#endif // DEBUGB
			}
		}
	}
	//ELSE IF this candidate pool node is still inactive
	//	or is already full (or, simply, the source node is not a candidate BBU hotel node)
	//	------> check other candidate pool nodes using a sorted nodes list

	OXCNode*pOXCdst = NULL, *pOXCdst2 = NULL;
	bool enough = true;
	//LINK_COST costBestActiveHotel = UNREACHABLE;

	//-B: build a list with all bbu pooling node already active with enough resources to host a new BBU
	vector<OXCNode*>auxBBUsList;
	genAuxBBUsList_BBUPooling(auxBBUsList, backhaulBwd);

	//-B: if there is at least an already active pooling node with free resources
	if (auxBBUsList.size() > 0)
	{
#ifdef DEBUGB
		cout << "\tCerco tra i " << auxBBUsList.size() << " pooling nodes già attivi" << endl;
#endif // DEBUGB

		//SEARCH FOR "BEST" BBU AMONG (!) ALREADY ACTIVATED (!) BBU POOLING NODES
		switch (BBUPOLICY)
		{
			//-B: GENERAL ADVICE: BE CAREFUL USING GLOBAL VARIABLE precomputedCost AND precomputedPath INSIDE placeBBUx METHODS
			//	(since they will be potentially used in BBU_ProvisionHelper_Unprotected)
			case 0:
				//(in this function the core CO is preferred over the others as BBU hotel node)
				bestBBU = placeBBUHigh(src, auxBBUsList);
				break;
			case 1:
				bestBBU = placeBBUClose(src, auxBBUsList);
				enough = false;
				break;
			case 2:
				bestBBU = placeBBU_Metric(src, auxBBUsList);
				enough = false;
				break;
			default:
				DEFAULT_SWITCH;
		}

		//if a valid bbu hotel node has been selected
		if (bestBBU > 0)
			pOXCdst = (OXCNode*)this->m_hWDMNet.lookUpNodeById(bestBBU);
	}

	if (bestBBU == 0)
	{
		enough = false;
	}

#ifdef DEBUGB
	//pOXCDst could be NULL
	if (pOXCdst)
		cout << "\tPath's cost towards best BBU pool node, already active: " << pOXCdst->m_nBBUReachCost << endl << endl;
#endif // DEBUGB


	//-B: STEP 3 - check still inactive candidate BBU pool nodes
	if (!enough)
	{
		//-B: build inactive candidate BBU hotel nodes list
		//(for policy #2, the hotels list was reduced before starting the simulation, in buildBestHotelsList() method)
		vector<OXCNode*> inactiveBBUs;
		buildNotActivePoolsList(inactiveBBUs);

		//if there is at least an INactive BBU pooling node
		if (inactiveBBUs.size() > 0)
		{
#ifdef DEBUGB
			cout << "\tCerco tra i " << inactiveBBUs.size() << " pooling nodes ancora inattivi" << endl << endl;
#endif // DEBUGB

			//SEARCH FOR "BEST" BBU AMONG (!) NOT YET ACTIVATED (!) BBU POOLING NODES
			switch (BBUPOLICY)
			{
				case 0:
					dst = placeBBUHigh(src, inactiveBBUs);
					break;
				case 1:
					dst = placeBBUClose(src, inactiveBBUs);
					break;
				case 2:
					dst = placeBBU_Metric(src, inactiveBBUs);
					break;
				default:
					DEFAULT_SWITCH;
			}

			//if a valid bbu hotel node has been selected
			if (dst > 0)
				pOXCdst2 = (OXCNode*)this->m_hWDMNet.lookUpNodeById(dst);

#ifdef DEBUGB
			//pOXCDst2 could be NULL
			if (pOXCdst2)
				cout << "\tPath's cost towards best candidate BBU pooling node, still inactive: " << pOXCdst2->m_nBBUReachCost << endl << endl;
#endif // DEBUGB
		}
	}


	//CASE 1 - use an already active BBU
	if (pOXCdst && pOXCdst2 == NULL)
	{
		//BEST BBU NODE: an already active bbu
		//-B: save best precomputed cost
		precomputedCost = pOXCdst->m_nBBUReachCost;
		//-B: save best precomputed path
		precomputedPath = pOXCdst->pPath;
		//-B: increase the number of active BBUs in the thotel node (the value will be more than 1 since it was already activated)
		pOXCdst->m_nBBUs++;
		//-B: assign, to the source node of the connection, the pooling node hosting "its" resources
		pOXCsrc->m_nBBUNodeIdsAssigned = bestBBU;
		return bestBBU; //------------------------------------------------------------------------------------------------------
	}

	//CASE 2 - activate a new BBU
	if (pOXCdst == NULL && pOXCdst2)
	{
		//BEST BBU NODE: a still inactive bbu
		//-B: save best precomputed cost
		precomputedCost = pOXCdst2->m_nBBUReachCost;
		//-B: save best precomputed path
		precomputedPath = pOXCdst2->pPath;
		//-B: add the new hotel node to the list of active pooling node
		this->m_hWDMNet.BBUs.push_back(pOXCdst2);
		//-B: increase the number of active BBUs in the pooling node (the value will be 1 since it was not activated yet)
		pOXCdst2->m_nBBUs++;
		//-B: assign, to the source node of the connection, the pooling node hosting "its" resources
		pOXCsrc->m_nBBUNodeIdsAssigned = dst;
		return dst; //---------------------------------------------------------------------------------------------------------
	}

	//-B: STEP 4 - evaluation best Pool
	//CASE 3 - both are valid --> not possible for policy #0
	if (pOXCdst && pOXCdst2)
	{
		bool alreadyActive = false;
		switch (BBUPOLICY)
		{
		case 1:
		{
			if (pOXCdst->m_nBBUReachCost <= pOXCdst2->m_nBBUReachCost)
			{
				alreadyActive = true;
#ifdef DEBUGB
				cout << "\tReach Cost a favore del pooling node gia' attivo: " << pOXCdst->m_nBBUReachCost
					<< " vs " << pOXCdst2->m_nBBUReachCost << " -> node: " << bestBBU << endl << endl;
#endif // DEBUGB
			}
			else
			{
				alreadyActive = false;
#ifdef DEBUGB
				cout << "\tReach Cost a favore del pooling node ancora inattivo: " << pOXCdst->m_nBBUReachCost
					<< " vs " << pOXCdst2->m_nBBUReachCost << " -> node: " << dst << endl << endl;
#endif // DEBUGB
			}
			break;
		}
		case 2:
		{
			if (pOXCdst->m_dCostMetric <= pOXCdst2->m_dCostMetric)
			{

				alreadyActive = true;
#ifdef DEBUGB
				cout << "\tCost metric a favore del pooling node gia' attivo: " << pOXCdst->m_dCostMetric
					<< " vs " << pOXCdst2->m_dCostMetric << " -> node: " << bestBBU << endl << endl;
#endif // DEBUGB
			}
			else
			{
				alreadyActive = false;
#ifdef DEBUGB
				cout << "\tCost metric a favore del pooling node ancora inattivo: " << pOXCdst->m_dCostMetric
					<< " vs " << pOXCdst2->m_dCostMetric << " -> node: " << dst << endl << endl;
#endif // DEBUGB
			}
			break;
		}
		default:
			DEFAULT_SWITCH;
		}
		//choose better dst node: cost difference
		if (alreadyActive)
		{
			//BEST BBU NODE: an already active bbu
			//-B: save best precomputed cost
			precomputedCost = pOXCdst->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst->pPath;
			//-B: increase the number of active BBUs in the pooling node (the value will be more than 1 since it was already activated)
			pOXCdst->m_nBBUs++;
			//-B: assign, to the source node of the connection, the pooling node hosting "its" resources
			pOXCsrc->m_nBBUNodeIdsAssigned = bestBBU;
			return bestBBU;
		}
		else
		{
			//BEST BBU NODE: a still inactive bbu
			//-B: save best precomputed cost
			precomputedCost = pOXCdst2->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst2->pPath;
			//-B: add the new hotel node to the list of active hotel node
			this->m_hWDMNet.BBUs.push_back(pOXCdst2);
			//-B: increase the number of active BBUs in the pooling node (the value will be 1 since it was not activated yet)
			pOXCdst2->m_nBBUs++;
			//-B: assign, to the source node of the connection, the pooling node hosting "its" resources
			pOXCsrc->m_nBBUNodeIdsAssigned = dst;
			return dst;
		}
	}

	//-B: STEP 5 - POST-PROCESS: validate those links that have been invalidated temporarily
	//m_hGraph.validateAllLinks();
	//COMMENTED SO THAT WE DON'T HAVE TO DO IT AGAIN IN BBU_ProvisionHelper_Unprotected

#ifdef DEBUGB
	cout << "\tTUTTI I BBU POOLING NODES (!) RAGGIUNGIBILI (!) DAL NODO " << src << " SONO PIENI?!" << endl;
	cout << "\tALLORA METTO LA BBU NEL CELL SITE STESSO DELLA SORGENTE " << src << endl;
	//cin.get();
#endif // DEBUGB

	//NON INSERISCO IL NODO NELLA LISTA this->m_hWDMNet.BBUs!!!
	pOXCsrc->m_nBBUNodeIdsAssigned = src;
	pOXCsrc->m_nBBUs++;
	//-B: THE PATH WILL BE COMPUTED DURING CONNECTION'S PROVISIONING

	//-B: if it has reached this instruction, the algorithm could not find
	//	any valid (REACHABLE) BBU hotel node for source src
	return src;
}


//-B: given the source node id, return the less "expensive" BBU_pool (according to the policy used)
//-B: SO FAR IT IS SIMPLY A COPY-PASTE OF findBestBBUHotel
UINT NetMan::findBestBBUPool_Soft(UINT src, BandwidthGranularity bwd)
{
#ifdef DEBUGB
	cout << "-> findBestBBUPool" << endl;
#endif // DEBUGB

	OXCNode*pOXCsrc = (OXCNode*)m_hWDMNet.lookUpNodeById(src);
	assert(pOXCsrc);
	UINT dst = 0, bestBBU = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//							*************** PRE-PROCESSING ******************
	//////////// (done at the beginning of the function because it avoids to do it in BBU_ProvisionHelper_Unprotected ////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//-B: it is used to prefer wavelength continuity over w. conversion in a condition of equality
	//	between the wavelength that would allow a wavelength path and any other best-fit wavelegth
	//	(assign a cost equal to 0.5 to simplex links of type LT_Converter and LT_Grooming)
	m_hGraph.preferWavelPath();
	//-B: invalidate those simplex links (both LT_Channel and LT_Lightpath) whose free capacity < bwd
	m_hGraph.invalidateSimplexLinkDueToCap(bwd);
	//-B: invalidate those simplex links (!) LT_Channel (!) whose free capacity < OCLightpath (whose freeStatus = false)
	invalidateSimplexLinkDueToFreeStatus();
	//-B: invalidate simplex link LT_Grooming and LT_Converter for nodes that don't have active BBUs
	if (ONLY_ACTIVE_GROOMING)
	{
		this->invalidateSimplexLinkGrooming();
	}
	//-B: modify simplex links' costs to perform best fit algorithm in choosing a simplex link LT_Lightpath
	//updateCostsForBestFit(); //-B: TO BE USED ONLY IF BBU_NewCircuit ASSIGNS A COST = 0 TO NEW LIGHTPATHS
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//-B: STEP 0a: check if this source node has an already assigned BBU in one of the hotel nodes
	if (pOXCsrc->m_nBBUNodeIdsAssigned > 0)
	{
#ifdef DEBUGB
		cout << "\tIl nodo sorgente " << src << " ha gia' la sua BBU collocata nel nodo " << pOXCsrc->m_nBBUNodeIdsAssigned << endl;
		cout << "\tControllo che sia raggiungibile" << endl;
#endif // DEBUGB

		//dst node of this connecton will be the BBU hotel node already assigned to this source node
		//	(independently of how many BBUs there are into it)
		return pOXCsrc->m_nBBUNodeIdsAssigned; //-----------------------------------------------------------------------------------------
	}

	//-B: STEP 0b - if source node is a bbu hotel node...
	//	(but, clearly, it's the first time this node has to route a fronthaul connection.
	//	Otherwise, it should have stopped at the previous step (step 0a))
	if (pOXCsrc->getBBUHotel())
	{
		//...and it is already active, there is no need to look for best bbu hotel node
		if (isAlreadyActive(pOXCsrc))
		{
#ifdef DEBUGB
			cout << "\tNodo sorgente (" << src << ") e' gia' un hotel node";
#endif // DEBUGB
			if (checkAvailabilityHotelNode(pOXCsrc))
			{
#ifdef DEBUGB
				cout << " -> puo' accogliere ancora BBUs" << endl;
#endif // DEBUGB
				//increase num of nodes whose BBU is in this node -> num of BBUs active in this node
				pOXCsrc->m_nBBUs++;
				pOXCsrc->m_nBBUNodeIdsAssigned = src;
				return src; //-----------------------------------------------------------------------------------------------------------
			}
			else
			{
				;
#ifdef DEBUGB
				cout << " -> NON HA PIU' SPAZIO PER ALTRE BBU!" << endl;
#endif // DEBUGB
			}
		}
	}
	//ELSE IF this candidate hotel node is still inactive (does not have any BBU in it)
	//	or is already full of BBUs (or, simply, the source node is not a candidate BBU hotel node)
	//	------> check other candidate hotel nodes using a sorted hotels list

	//-B: reset BBUsReachCost = UNREACHABLE for all nodes (potentially modified in the previous iteration)
	//m_hWDMNet.resetBBUsReachabilityCost(); //-B: ALREADY DONE BY resetPreProcessing method (inside BBU_newConnection)

	OXCNode*pOXCdst = NULL, *pOXCdst2 = NULL;
	bool enough = true;
	//LINK_COST costBestActiveHotel = UNREACHABLE;

	//-B: build a list with all bbu hotel node already active with enough space to host a new BBU
	//(for policy #2, the hotels list was reduced before starting the simulation, in +List() method)
	vector<OXCNode*>auxBBUsList;
	genAuxBBUsList(auxBBUsList);

	if (auxBBUsList.size() > 0 || BBUPOLICY == 2)
	{
#ifdef DEBUGB
		cout << "\tCerco tra i " << auxBBUsList.size() << " hotels già attivi" << endl;
#endif // DEBUGB

		//SEARCH FOR "BEST" BBU AMONG (!) ALREADY ACTIVATED (!) BBU HOTEL NODES
		switch (BBUPOLICY)
		{
			//-B: GENERAL ADVICE: BE CAREFUL USING GLOBAL VARIABLE precomputedCost AND precomputedPath INSIDE placeBBU METHODS
			//	(since they will be used in BBU_ProvisionHelper_Unprotected)
		case 0:
			//(in this function the core CO is preferred over the others as BBU hotel node)
			bestBBU = placeBBUHigh(src, auxBBUsList);
			break;
		case 1:
			bestBBU = placeBBUClose(src, auxBBUsList);
			enough = false;
			break;
		case 2:
			bestBBU = placeBBU_Metric(src, auxBBUsList);
			enough = false;
			break;
		default:
			DEFAULT_SWITCH;
		}

		//if a valid bbu hotel node has been selected
		if (bestBBU > 0)
			pOXCdst = (OXCNode*)this->m_hWDMNet.lookUpNodeById(bestBBU);
	}

	if (bestBBU == 0)
	{
		enough = false;
	}

#ifdef DEBUGB
	//pOXCDst could be NULL
	if (pOXCdst)
		cout << "\tPath's cost towards best BBU hotel node, already active: " << pOXCdst->m_nBBUReachCost << endl << endl;
#endif // DEBUGB

	/*
	//-B: STEP 2b - evaluate inactive BBUs hotel nodes only if path cost to best BBU hotel node already active (if found) is > 3
	if (pOXCdst)
	{
	if (pOXCdst->m_nBBUReachCost > 3)
	{
	//it could be more convenient to activate one for the candidate hotel nodes still inactive
	enough = false;
	}
	else
	{
	; //the best BBU hotel node already active has to be preferred
	#ifdef DEBUGB
	cout << "\tNon considero i BBU hotel node inattivi" << endl << endl;
	#endif // DEBUGB
	}
	}
	else //best active BBU not found
	{
	//we are going to look for which candidate hotel node should be activated in the following step
	enough = false;
	}*/

	//-B: STEP 3 - check still inactive candidate BBU hotel nodes
	if (!enough)
	{
		//-B: build inactive candidate BBU hotel nodes list
		//(for policy #2, the hotels list was reduced before starting the simulation, in buildBestHotelsList() method)
		vector<OXCNode*> inactiveBBUs;
		buildNotActiveBBUsList(inactiveBBUs);

		//if there is at least an INactive BBU hotel node
		if (inactiveBBUs.size() > 0)
		{
#ifdef DEBUGB
			cout << "\tCerco tra i " << inactiveBBUs.size() << " hotels ancora inattivi" << endl << endl;
#endif // DEBUGB

			//SEARCH FOR "BEST" BBU AMONG (!) NOT YET ACTIVATED (!) BBU HOTEL NODES
			switch (BBUPOLICY)
			{
			case 0:
				//	(in this function the core CO is preferred over the others as BBU hotel node)
				dst = placeBBUHigh(src, inactiveBBUs);
				break;
			case 1:
				dst = placeBBUClose(src, inactiveBBUs);
				break;
			case 2:
				dst = placeBBU_Metric(src, inactiveBBUs);
				break;
			default:
				DEFAULT_SWITCH;
			}

			//if a valid bbu hotel node has been selected
			if (dst > 0)
				pOXCdst2 = (OXCNode*)this->m_hWDMNet.lookUpNodeById(dst);

#ifdef DEBUGB
			//pOXCDst2 could be NULL
			if (pOXCdst2)
				cout << "\tPath's cost towards best candidate BBU hotel node, still inactive: " << pOXCdst2->m_nBBUReachCost << endl << endl;
#endif // DEBUGB
		}
	}


	//CASE 1 - use an already active BBU
	if (pOXCdst && pOXCdst2 == NULL)
	{
		//BEST BBU NODE: an already active bbu
		//-B: save best precomputed cost
		precomputedCost = pOXCdst->m_nBBUReachCost;
		//-B: save best precomputed path
		precomputedPath = pOXCdst->pPath;
		//-B: increase the number of active BBUs in the thotel node (the value will be more than 1 since it was already activated)
		pOXCdst->m_nBBUs++;
		//-B: assign, to the source node of the connection, the hotel node hosting its BBU
		pOXCsrc->m_nBBUNodeIdsAssigned = bestBBU;
		return bestBBU; //------------------------------------------------------------------------------------------------------
	}

	//CASE 2 - activate a new BBU
	if (pOXCdst == NULL && pOXCdst2)
	{
		//BEST BBU NODE: a still inactive bbu
		//-B: save best precomputed cost
		precomputedCost = pOXCdst2->m_nBBUReachCost;
		//-B: save best precomputed path
		precomputedPath = pOXCdst2->pPath;
		//-B: add the new hotel node to the list of active hotel node
		this->m_hWDMNet.BBUs.push_back(pOXCdst2);
		//-B: increase the number of active BBUs in the thotel node (the value will be 1 since it was not activated yet)
		pOXCdst2->m_nBBUs++;
		//-B: assign, to the source node of the connection, the hotel node hosting its BBU
		pOXCsrc->m_nBBUNodeIdsAssigned = dst;
		return dst; //---------------------------------------------------------------------------------------------------------
	}

	//-B: STEP 4 - evaluation best Hotel
	//CASE 3 - both are valid --> not possible for policy #0
	if (pOXCdst && pOXCdst2)
	{
		bool alreadyActive = false;
		switch (BBUPOLICY)
		{
		case 1:
		{
			if (pOXCdst->m_nBBUReachCost <= pOXCdst2->m_nBBUReachCost)
			{
				alreadyActive = true;
#ifdef DEBUGB
				cout << "\tReach Cost a favore della BBU gia' attiva: " << pOXCdst->m_nBBUReachCost
					<< " vs " << pOXCdst2->m_nBBUReachCost << " -> node: " << bestBBU << endl << endl;
#endif // DEBUGB
			}
			else
			{
				alreadyActive = false;
#ifdef DEBUGB
				cout << "\tReach Cost a favore della BBU ancora inattiva: " << pOXCdst->m_nBBUReachCost
					<< " vs " << pOXCdst2->m_nBBUReachCost << " -> node: " << dst << endl << endl;
#endif // DEBUGB
			}
			break;
		}
		case 2:
		{
			if (pOXCdst->m_dCostMetric <= pOXCdst2->m_dCostMetric)
			{

				alreadyActive = true;
#ifdef DEBUGB
				cout << "\tCost metric a favore della BBU gia' attiva: " << pOXCdst->m_dCostMetric
					<< " vs " << pOXCdst2->m_dCostMetric << " -> node: " << bestBBU << endl << endl;
#endif // DEBUGB
			}
			else
			{
				alreadyActive = false;
#ifdef DEBUGB
				cout << "\tCost metric a favore della BBU ancora inattiva: " << pOXCdst->m_dCostMetric
					<< " vs " << pOXCdst2->m_dCostMetric << " -> node: " << dst << endl << endl;
#endif // DEBUGB
			}
			break;
		}
		default:
			DEFAULT_SWITCH;
		}
		//choose better dst node: cost difference
		if (alreadyActive)
		{
			//BEST BBU NODE: an already active bbu
			//-B: save best precomputed cost
			precomputedCost = pOXCdst->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst->pPath;
			//-B: increase the number of active BBUs in the thotel node (the value will be more than 1 since it was already activated)
			pOXCdst->m_nBBUs++;
			//-B: assign, to the source node of the connection, the hotel node hosting its BBU
			pOXCsrc->m_nBBUNodeIdsAssigned = bestBBU;
			return bestBBU;
		}
		else
		{
			//BEST BBU NODE: a still inactive bbu
			//-B: save best precomputed cost
			precomputedCost = pOXCdst2->m_nBBUReachCost;
			//-B: save best precomputed path
			precomputedPath = pOXCdst2->pPath;
			//-B: add the new hotel node to the list of active hotel node
			this->m_hWDMNet.BBUs.push_back(pOXCdst2);
			//-B: increase the number of active BBUs in the thotel node (the value will be 1 since it was not activated yet)
			pOXCdst2->m_nBBUs++;
			//-B: assign, to the source node of the connection, the hotel node hosting its BBU
			pOXCsrc->m_nBBUNodeIdsAssigned = dst;
			return dst;
		}
	}

	//-B: STEP 5 - POST-PROCESS: validate those links that have been invalidated temporarily
	//m_hGraph.validateAllLinks();
	//COMMENTED SO THAT WE DON'T HAVE TO DO IT AGAIN IN BBU_ProvisionHelper_Unprotected

#ifdef DEBUGB
	cout << "\tTUTTI I BBU HOTEL NODES (!) RAGGIUNGIBILI (!) DAL NODO " << src << " SONO PIENI?!" << endl;
	cout << "\tALLORA METTO LA BBU NEL CELL SITE STESSO DELLA SORGENTE " << src << endl;
	//cin.get();
	//cin.get();
#endif // DEBUGB

	//NON INSERISCO IL NODO NELLA LISTA this->m_hWDMNet.BBUs!!!
	pOXCsrc->m_nBBUNodeIdsAssigned = src;
	pOXCsrc->m_nBBUs++;
	//-B: THE PATH WILL BE COMPUTED DURING CONNECTION'S PROVISIONING

	//-B: if it has reached this instruction, the algorithm could not find
	//	any valid (REACHABLE) BBU hotel node for source src
	return src;
}


UINT NetMan::placeBBUHigh(UINT src, vector<OXCNode*>&BBUsList)
{
#ifdef DEBUG
	cout << "-> placeBBUHigh" << endl;
#endif // DEBUGB

	LINK_COST bestCost = UNREACHABLE;
	//LINK_COST cost;
	UINT bestBBU = 0;
	//list<AbstractLink*>path;
	OXCNode*pOXCdst;
	OXCNode*pOXCsrc;
	UINT id;
	LINK_COST pathCost = UNREACHABLE;

	//-B: each time the cycle run calls BBU_newConnection method precomputedPath is cleared (from previous cycle calculation)

	//lookup source vertex
	Vertex *pSrc = m_hGraph.lookUpVertex(src, Vertex::VT_Access_Out, -1);	//-B: ATTENTION!!! VT_Access_Out
	list <AbstractLink*> savedPath;
	bool pathAlreadyFound = false;
	UINT bestBBUFound = 0;

	for (int j = 0; j < BBUsList.size(); j++)
	{
		//-B: we must declare it here (and not before the for cycle)!!!
		//list<AbstractLink*> pPath; --> not NEEDED for this policy

		//-B: reset boolean var because we are considering another node
		m_bHotelNotFoundBecauseOfLatency = false;
	

		id = BBUsList[j]->getId();

		pOXCdst = (OXCNode*)m_hWDMNet.lookUpNodeById(id);
		pOXCsrc = (OXCNode*)m_hWDMNet.lookUpNodeById(src);
		Vertex* pDst = m_hGraph.lookUpVertex(id, Vertex::VT_Access_In, -1);	//-B: ATTENTION!!! VT_Access_In
		Vertex* pSrc = m_hGraph.lookUpVertex(src, Vertex::VT_Access_Out, -1);
				
		//-B: STEP 2 - *********** CALCULATE PATH AND RELATED COST ***********
		//-B: calcolo il costo dello shortest path che va dalla source data in input alla funzione
		//	fino al nodo destinazione che cambia ad ogni ciclo
	
		list<AbsPath*> hPathList;
		m_hGraph.Yen(hPathList, pSrc, pOXCsrc, pDst, 10, this, AbstractGraph::LinkCostFunction::LCF_ByOriginalLinkCost);
		pOXCdst->updateHotelCostMetricForP0(this->m_hWDMNet.getNumberOfNodes());
		pOXCdst->m_dCostMetric += pOXCdst->m_nBBUReachCost;
#ifdef DEBUG
		cout << "List of paths found: " << hPathList.size() << endl;
#endif

		if (hPathList.size() == 0) {
			m_bHotelNotFoundBecauseOfLatency = true;
			groomingConnections.clear();
		}
		else if(hPathList.size() == 1){

				list<AbsPath*>::const_iterator itrPath;

				itrPath = hPathList.begin();
				bool pathAlreadyFound = false;

					
				list <AbstractLink*> pathComputedByYen = (*itrPath)->m_hLinkList;

				printPath(pathComputedByYen);

				//-B: STEP 2 - *********** CALCULATE PATH AND RELATED COST ***********
				//-B: calcolo il costo dello shortest path che va dalla source data in input alla funzione
				//	fino al nodo destinazione che cambia ad ogni ciclo
				pathCost = (*itrPath)->calculateCost();
				pOXCdst->m_nBBUReachCost = pathCost;

				pOXCdst->updateHotelCostMetricForP0(this->m_hWDMNet.getNumberOfNodes());
				pOXCdst->m_dCostMetric += pOXCdst->m_nBBUReachCost;

				//-B: *********** UPDATE COST METRIC ************ (cost metric is reset in resetPreProcessing method, inside BBU_newConnection)

				///////////////////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////////////    POLICY 0    ///////////////////////////////////////
				////////////////// (CHOOSE CORE CO OR THE "HIGHEST" BBU HOTEL NODE) //////////////////// ("highest" = closest to the core co)
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				if (pathCost < UNREACHABLE)
				{


					if ((pOXCdst->m_dCostMetric < bestCost)) {

						bestCost = pOXCdst->m_dCostMetric;
#ifdef DEBUG

							cout << "New best cost BBU (satisfying constraints) is " << pOXCdst->getId() << endl;
#endif
							pathAlreadyFound = true;
							pOXCdst->pPath = pathComputedByYen;

							//precomputedPath = pOXCdst->pPath; //overwritten at the end of findBestBBUHotel --> SO DON'T DO IT HERE!

							//Reset grooming time related to the old best bbu
							//otherwise, I will update the grooming time to connections not on the chosen path
							list <Connection*>::const_iterator itrG;
							Connection* pConG;
							list<Connection*> connectinGroomingReset;

							for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {

								pConG = (Connection*)(*itrG);
#ifdef DEBUGC
								cout << "Connection from " << pConG->m_nSrc << " to "
									<< pConG->m_nDst << " has grooming time > 0" << endl;
#endif						
								Connection* connFound = checkLinks(pConG, savedPath, pOXCdst->pPath);
								if (connFound != NULL) {

									connectinGroomingReset.push_back(connFound);

								}

							}

							list < Connection*>::const_iterator itrCheckLinks;
							Connection* connectionToDelete;

							for (itrCheckLinks = connectinGroomingReset.begin(); itrCheckLinks != connectinGroomingReset.end(); itrCheckLinks++) {
								connectionToDelete = (Connection*)(*itrCheckLinks);
								groomingConnections.erase(connectionToDelete->m_nSequenceNo);

							}


							savedPath = pOXCdst->pPath;

							bestBBU = pOXCdst->getId();
							if (bestBBU == 46) {
								return bestBBU;
							}

						} else {
							groomingConnections.clear();
						}

					} //end IF unreachable

#ifdef DEBUG
					cout << "\tCosto " << src;
					list<AbstractLink*>::const_iterator itr = pathComputedByYen.begin();
					SimplexLink*sLink;
					while (itr != pathComputedByYen.end())
					{
						sLink = (SimplexLink*)(*itr);
						if (sLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Channel)
							cout << "->" << sLink->m_pUniFiber->getDst()->getId();
						if (sLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
							cout << "->" << sLink->m_pLightpath->getDst()->getId();
						itr++;
					}
					cout << " = " << pOXCdst->m_nBBUReachCost << endl;
#endif // DEBUGB
				
			if (!pathAlreadyFound) {
				m_bHotelNotFoundBecauseOfLatency = true;
			}
		}
		else {
			cout << "Why do i arrive here ?" << endl;
			cin.get();
		}
		
	} //end FOR bbu hotel nodes

	return bestBBU;
}


UINT NetMan::placeBBUClose(UINT src, vector<OXCNode*>&BBUsList)
{
#ifdef DEBUGB
	cout << "-> placeBBUClose" << endl;
#endif // DEBUGB

	LINK_COST minCost = UNREACHABLE;
	//LINK_COST cost;
	UINT bestBBU = 0;
	//list<AbstractLink*>path;
	OXCNode*pOXCdst;
	int id;
	LINK_COST pathCost;

	//-B: each time the cycle run calls BBU_newConnection method precomputedPath is cleared (from previous cycle calculation)

	//lookup source vertex
	Vertex *pSrc = m_hGraph.lookUpVertex(src, Vertex::VT_Access_Out, -1);	//-B: ATTENTION!!! VT_Access_Out

	for (int j = 0; j < BBUsList.size(); j++)
	{
		//-B: reset boolean var because we are considering another node
		m_bHotelNotFoundBecauseOfLatency = false;

		id = BBUsList[j]->getId();
		pOXCdst = (OXCNode*)m_hWDMNet.lookUpNodeById(id);
		Vertex *pDst = m_hGraph.lookUpVertex(id, Vertex::VT_Access_In, -1);	//-B: ATTENTION!!! VT_Access_In

#ifdef DEBUGB
		UINT numGroomNodes = 0;
		//numGroomNodes = this->countGroomingNodes();
		if (numGroomNodes > 1)
		{
			cout << "\tCI SONO " << numGroomNodes << " POTENZIALI GROOMING NODES" << endl;
			//cin.get();
		}
#endif // DEBUGB

		//-B: STEP 2 - *********** CALCULATE PATH AND RELATED COST ***********
		//-B: calcolo il costo dello shortest path che va dalla source data in input alla funzione
		//	fino al nodo destinazione che cambia ad ogni ciclo
		pathCost = m_hGraph.DijkstraLatency(pOXCdst->pPath, pSrc, pDst, AbstractGraph::LinkCostFunction::LCF_ByOriginalLinkCost);
		pOXCdst->m_nBBUReachCost = pathCost;

		///////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////      POLICY 1    //////////////////////////////////////
		///////////////////////// (CHOOSE THE NEAREST BBU HOTEL NODE) //////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////
		if (pathCost < UNREACHABLE)
		{
			Connection::ConnectionType connType = Connection::MOBILE_FRONTHAUL;

			float lat = computeLatencyP3(pOXCdst->pPath, pSrc, (UINT)connType);
			if (lat <= LATENCYBUDGET) //-B: -> should be unnecessary using DijkstraLatency -> IT IS ABSOLUTELY NECESSARY! (read at the end of DijkstraHelperLatency)
			{		
				//PoP node as BBU hotel node should be preferred
				//	(if not, we could have strange situation in which the connection
				//	is routed towards its BBU node passing through the PoP node
				//	while having enough space to host its BBU)
				//if (id == m_hWDMNet.DummyNode)
				//{
					//precomputedCost = 0;
					//minCost = precomputedCost;
					//precomputedPath = pOXCdst->pPath;
					//bestBBU = id;
					//break; //-B exit from for cycle
				//}

				//if current selected BBU is better than previous best BBU
				if (pathCost < minCost)
				{
					minCost = pathCost;
					//precomputedPath = pOXCdst->pPath; //then overwritten at the end of findBestBBUHotel --> SO DON'T DO IT!
					bestBBU = id;
				}
			} //end IF latency
			else
			{
#ifdef DEBUGB
				cout << "\tHotel node " << id << " scartato per superamento di latenza max -> " << lat << endl;
				//cin.get();
#endif // DEBUGB
				m_bHotelNotFoundBecauseOfLatency = true;
			}
		} //end IF cost < UNREACHABLE
		//////////////////////////////////////////////////////////////////////////////////////////////////////7

#ifdef DEBUGB
		cout << "\tCosto " << src;
		list<AbstractLink*>::const_iterator itr = pOXCdst->pPath.begin();
		SimplexLink*sLink;
		while (itr != pOXCdst->pPath.end())
		{
			sLink = (SimplexLink*)(*itr);
			if (sLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Channel)
				cout << "->" << sLink->m_pUniFiber->getDst()->getId();
			if (sLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
				cout << "->" << sLink->m_pLightpath->getDst()->getId();
			itr++;
		}
		cout << " = " << pOXCdst->m_nBBUReachCost << endl;
#endif // DEBUGB

	} //end FOR BBU hotel nodes

	return bestBBU;
}


UINT NetMan::placeBBU_Metric(UINT src, vector<OXCNode*>&BBUsList)
{
#ifdef DEBUGB
	cout << "-> placeBBU_Metric" << endl;
#endif // DEBUGB

	vector<OXCNode*>::const_iterator itr;
	OXCNode*pNode;
	LINK_COST bestCost = UNREACHABLE, pathCost = UNREACHABLE;
	Vertex*pVDst;
	Vertex*pVSrc = this->m_hGraph.lookUpVertex(src, Vertex::VT_Access_Out, -1);
	UINT bestBBU = 0;

	for (itr = BBUsList.begin(); itr != BBUsList.end(); itr++)
	{
		//-B: reset boolean var because we are considering another node
		m_bHotelNotFoundBecauseOfLatency = false;

		pNode = (OXCNode*)(*itr);
		pVDst = this->m_hGraph.lookUpVertex(pNode->getId(), Vertex::VT_Access_In, -1);

		pathCost = this->m_hGraph.DijkstraLatency(pNode->pPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);
		pNode->m_nBBUReachCost = pathCost;

		bool valid = this->noFullLinks(pNode->pPath);
		if (!valid)
		{
			pathCost = UNREACHABLE;
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////      POLICY 2    //////////////////////////////////////
		///////////////////////// (CHOOSE THE NEAREST BBU HOTEL NODE) //////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////
		if (pathCost < UNREACHABLE)
		{
			Connection::ConnectionType connType = Connection::MOBILE_FRONTHAUL;

			float lat = computeLatencyP3(pNode->pPath, pVSrc, connType);
			if (lat <= LATENCYBUDGET) //-B: -> should be unnecessary using DijkstraLatency -> IT IS ABSOLUTELY NECESSARY! (read at the end of DijkstraHelperLatency)
			{
				//PoP node as BBU hotel node should be preferred
				//	(if not, we could have strange situation in which the connection
				//	is routed towards its BBU node passing through the PoP node
				//	while having enough space to host its BBU)
				//if (id == m_hWDMNet.DummyNode)
				//{
				//precomputedCost = 0;
				//minCost = precomputedCost;
				//precomputedPath = pOXCdst->pPath;
				//bestBBU = id;
				//break; //-B exit from for cycle
				//}

				//MONICA: NOT CORRECT
			/*
					//Check for every connection in the DB that by adding the grooming time (switching
					// time) the latency budget is not exceeded. 
					list<Connection*>::const_iterator itr2;
					Connection* pConDB;
					bool conditionSatisfied = true;
					for (itr2 = this->m_hConnectionDB.m_hConList.begin(); itr2 != this->m_hConnectionDB.m_hConList.end(); itr2++) {
						pConDB = (Connection*)(*itr2);

						if ((pConDB->m_dGroomingTime > 0) && pConDB->m_eConnType != Connection::MOBILE_FRONTHAUL && pConDB->m_eConnType != Connection::FIXEDMOBILE_FRONTHAUL) {
							pConDB->m_dGroomingTime = 0;
#ifdef DEBUG
							cout << "\tConnection with which I groome it's a backhaul" << endl;
							cout << "\tConnection's source: " << pConDB->m_nSrc << endl;
							cout << "\tConnection's type: " << pConDB->m_eConnType << endl;
							//cin.get();
#endif
						}
						else if (pConDB->m_dGroomingTime > 0 && (pConDB->m_dRoutingTime + pConDB->m_dGroomingTime > LATENCYBUDGET)) {
							conditionSatisfied = false;
							cout << "\tGrooming: connection of source " << pConDB->m_nSrc <<
								" exceeds the latency budget" << endl;
							cout << "\tRouting + grooming time = " << pConDB->m_dRoutingTime + pConDB->m_dGroomingTime << endl;
							break;
						}

					}

					if (conditionSatisfied && (pathCost < bestCost)) {

						bestCost = pathCost;
						//precomputedPath = pOXCdst->pPath; //overwritten at the end of findBestBBUHotel --> SO DON'T DO IT HERE!
						bestBBU = pNode->getId();

					}
					else {
						list<Connection*>::const_iterator itr2;
						Connection* pConDB;
						for (itr2 = this->m_hConnectionDB.m_hConList.begin(); itr2 != this->m_hConnectionDB.m_hConList.end(); itr2++) {
							pConDB = (Connection*)(*itr2);
							pConDB->m_dGroomingTime = 0;
						}

					}
*/
			} //end IF latency
			else
			{
#ifdef DEBUGB
				cout << "\tHotel node " << pNode->getId() << " scartato per superamento di latenza max -> " << lat 
					<< " - reach cost = " << pathCost << endl;
				//cin.get();
#endif // DEBUGB
				/*
				list<Connection*>::const_iterator itr2;
				Connection* pConDB;
				for (itr2 = this->m_hConnectionDB.m_hConList.begin(); itr2 != this->m_hConnectionDB.m_hConList.end(); itr2++) {
					pConDB = (Connection*)(*itr2);
					pConDB->m_dGroomingTime = 0;
				}*/
				m_bHotelNotFoundBecauseOfLatency = true;
			}
		} //end IF cost < UNREACHABLE
		  //////////////////////////////////////////////////////////////////////////////////////////////////////7

#ifdef DEBUGB
		cout << "\tCosto " << src;
		list<AbstractLink*>::const_iterator itr = pNode->pPath.begin();
		SimplexLink*sLink;
		while (itr != pNode->pPath.end())
		{
			sLink = (SimplexLink*)(*itr);
			if (sLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Channel)
				cout << "->" << sLink->m_pUniFiber->getDst()->getId();
			if (sLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
				cout << "->" << sLink->m_pLightpath->getDst()->getId();
			itr++;
		}
		cout << " = " << pNode->m_nBBUReachCost << endl;
#endif // DEBUGB

		/* //-B: OBSOLETE PROCEDURE
				//-B: *********** UPDATE COST METRIC ************ (cost metric is reset in resetPreProcessing method, inside BBU_newConnection)
		bool valid = this->isLastLinkFull(pNode->pPath);
		if (valid)
		{
			pNode->updateHotelCostMetricForP2(this->m_hWDMNet.getNumberOfNodes(), this->m_hWDMNet.DummyNode);
			pNode->m_dCostMetric += pNode->m_nBBUReachCost;
		}
		else
		{
			pNode->m_dCostMetric = UNREACHABLE;
		}

#ifdef DEBUGB
		cout << "\tHotel node #" << pNode->getId() << " - cost metric = " << pNode->m_dCostMetric << endl;
		//cin.get();
#endif // DEBUGB
		if (pathCost < UNREACHABLE)
		{
			if (computeLatency(pNode->pPath) <= LATENCYBUDGET) //-B: should be unnecessary using DijkstraLatency
			{
				//lower cost metric is better
				if (pNode->m_dCostMetric < bestCost)
				{
					bestCost = pNode->m_dCostMetric;
					//precomputedPath = pNode->pPath; //overwritten at the end of findBestBBUHotel --> SO, DON'T DO IT HERE!
					bestBBU = pNode->getId();
				}
			}
			else
			{
				m_bHotelNotFoundBecauseOfLatency = true;
#ifdef DEBUGB
				cout << "\tLATENZA SUPERATA!!!!!!! Partenza dal nodo src "
					<< pVSrc->m_pOXCNode->getId() << endl;
				//cin.get();
#endif // DEBUGB
				//continue; //not needed
			}
		}
		*/

	} // end FOR nodes
	return bestBBU;
}

//-B: set simplex link's (LT_Lightpath) costs for best fit algorithm when grooming
void NetMan::updateCostsForBestFit()
{
#ifdef DEBUGB
	cout << "-> updateCostsForBestFit" << endl;
#endif // DEBUGB

	list<AbstractNode*>::const_iterator itrNode;
	list<AbstractNode*>::const_iterator itrV;
	list<AbstractLink*>::const_iterator itrLink1, itrLink2;
	OXCNode*pNode;
	Vertex*pVertex;
	SimplexLink*pLink1, *pLink2;
	BinaryHeap<SimplexLink*, vector<SimplexLink*>, PSimplexLinkComp> simplexLinkSorted;

	//-B: scan all WDM network's nodes
	for (itrNode = this->m_hWDMNet.m_hNodeList.begin(); itrNode != this->m_hWDMNet.m_hNodeList.end(); itrNode++)
	{
		pNode = (OXCNode*)(*itrNode);
		//scan all graph's vertexes
		for (itrV = this->m_hGraph.m_hNodeList.begin(); itrV != this->m_hGraph.m_hNodeList.end(); itrV++)
		{
			pVertex = (Vertex*)(*itrV);
			//if the selected vertex has type VT_Access_Out (so, it can have outgoing simplex links LT_Lightpath)
			if (pVertex->m_eVType == Vertex::VertexType::VT_Access_Out)
			{
				//consider only vertexes belonging to the selected network's nodes
				if (pVertex->m_pOXCNode->getId() == pNode->getId())
				{
					//scan all the simplex link outgoing from the selected VT_Access_Out vertex
					for (itrLink1 = pVertex->m_hOLinkList.begin(); itrLink1 != pVertex->m_hOLinkList.end(); itrLink1++)
					{
						pLink1 = (SimplexLink*)(*itrLink1);
						//if the selected simplex link has type LT_Lightpath
						if (pLink1->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
						{
							//-B: consider only simplex links that are valid (freeCap > reqBwd) and whose cost has not been modified yet
							//	(if cost > 0, it means that it has already been considered during a previous cycle)
							//	(THIS CONSIDERATION IS VALID ONLY IF IN BBU_NewCircuit THE COST ASSIGNED TO THE NEW LIGHTPATH IS 0!!!)
							if (pLink1->getValidity() && pLink1->getCost() == 0)
							{
								//insert the selected simplex link LT_Lightpath in the sorted list (sorted by their freeCap)
								simplexLinkSorted.insert(pLink1);
								//scan all the outgoing simplex links from the same selected vertex VT_Access_Out
								for (itrLink2 = pVertex->m_hOLinkList.begin(); itrLink2 != pVertex->m_hOLinkList.end(); itrLink2++)
								{
									pLink2 = (SimplexLink*)(*itrLink2);
									//if the selected simplex link has type LT_Lightpath
									if (pLink2->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
									{
										//if it's not the previous selected simplex link LT_Lightpath
										if (pLink2->getId() != pLink1->getId())
										{
											//if also this selected simplex link is valid (freeCap > reqBwd)
											//	and it corresponds to the same virtual link of the previous selected simplex link
											//	and ecc. (as for cost)
											if (pLink2->getValidity() && pLink2->getDst()->getId() == pLink1->getDst()->getId() && pLink2->getCost() == 0)
											{
												//insert in the sorted list
												simplexLinkSorted.insert(pLink2);
											}
										}
									}
								}
								//-B: at this time we have inserted all the valid simplex links LT_Lightpath,
								//	corresponding to the same virtual link, in the BynaryHeap simplexLinkSorted

								if (simplexLinkSorted.m_hContainer.size() > 1)
								{
#ifdef DEBUGB
									cout << "\tPer il nodo " << pNode->getId() << " - Sorted link list size: "
										<< simplexLinkSorted.m_hContainer.size() << endl;
									cout << "\tSimplex links LT_Lightpath che vanno da " << ((Vertex*)pLink1->getSrc())->m_pOXCNode->getId()
										<< " fino al nodo " << ((Vertex*)pLink1->getDst())->m_pOXCNode->getId() << endl;
									//cin.get();
									//cin.get();
#endif // DEBUGB
									//-------------------------------- COSTS ASSIGNMENT -------------------------------------
									LINK_COST costAssigned = pLink1->getCost(); //NOT 0!!!;
									LINK_CAPACITY minCap = 0;
									//-B: assign increasing costs to simplex links in the sorted list, according to their free capacity
									while (!simplexLinkSorted.empty())
									{
										//select the simplex link with the minimum cost
										pLink2 = simplexLinkSorted.peekMin();
										//erase it from the sorted list
										simplexLinkSorted.popMin();
										//if its free capacity is effectively greater than previous link's free capacity
										if (pLink2->m_hFreeCap > minCap)
										{
											costAssigned += 0.1;
										}//else: they will have the same cost

										//-------- ASSIGN AN INCREASING COST --------
										pLink2->modifyCost(costAssigned);
										//-B: !!!Ricorda che qui viene modificata anche la var bool m_bCostSaved, nel caso la utilizzassi!!!
	#ifdef DEBUGB
											cout << "\t\tSimplex Link LT_Lightpath ch " << pLink2->m_nChannel << " (" 
												<< ((Vertex*)pLink2->getSrc())->m_pOXCNode->getId()	<< "->"
												<< ((Vertex*)pLink2->getDst())->m_pOXCNode->getId() << ") - freecap: "
												<< pLink2->m_hFreeCap << " - cost = " << pLink2->getCost() << endl;
	#endif // DEBUGB
										//simplexLinkSorted.buildHeap(); //commented because it should be useless, since I don't modify anything in this cycle
										//save free capacity of the selected simplex link channel
										minCap = pLink2->m_hFreeCap;
									}//end WHILE simplexLinkSorted
								}
								//empty the sorted list
								while (!simplexLinkSorted.empty())
								{
									simplexLinkSorted.popMin();
								}
							}
						}
					}
				}
			}
		}
	}
}

void NetMan::buildHotelsList(vector<OXCNode*>&otherHotels)
{
#ifdef DEBUGB
	cout << "-> buildHotelsList" << endl;
#endif // DEBUGB

	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode, *pHotel;
	vector<OXCNode*>::const_iterator itrH;
	bool found = false;
	//-B: scan all network nodes, looking for hotel nodes
	for (itr = this->m_hWDMNet.m_hNodeList.begin(); itr != this->m_hWDMNet.m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		//-B: check only BBU hotel nodes
		if (pNode->getBBUHotel() == true) {

			cout << "\tNode " << pNode->getId() <<" is a candidate hotel" << endl;

			//if it is a hotel
			found = false;

			//-B: it must not be a "favourite" hotel for policy 2
			for (itrH = this->m_hWDMNet.hotelsList.begin(); itrH != m_hWDMNet.hotelsList.end(); itrH++)
			{
				pHotel = (OXCNode*)(*itrH);
				if (pNode->getId() == pHotel->getId())
				{
					cout << "\tNode found in hotel list: " << pNode->getId() << endl;
					found = true;
					break;
				}
			}
			if (!found)
			{
				cout << "I'm going to insert the hotel " << pNode->getId() << endl;
				otherHotels.push_back(pNode);
			}
			//else continue
		}
	}
}

void NetMan::buildNotActiveBBUsList(vector<OXCNode*>&inactiveBBUs)
{
#ifdef DEBUGB
	cout << "-> buildNotActiveBBUsList" << endl;
#endif // DEBUGB

	int i;
	bool found = false;
	inactiveBBUs.clear();

	//scan all candidate bbu hotel nodes, sorted by their reachability cost (towards core co)
	for (i = 0; i < m_hWDMNet.hotelsList.size(); i++)
	{
		//if it is still inactive
		if (m_hWDMNet.hotelsList[i]->m_nBBUs == 0)
		{
			//insert in corresponding list
			inactiveBBUs.push_back(m_hWDMNet.hotelsList[i]);
		}
	}

#ifdef DEBUGB
	cout << "\tinactiveBBUs list:";
	for (i = 0; i < inactiveBBUs.size(); i++)
	{
		cout << " " << inactiveBBUs[i]->getId();
	}
	cout << endl;
#endif // DEBUGB

}


void NetMan::buildNotActivePoolsList(vector<OXCNode*>&inactiveBBUs)
{
#ifdef DEBUGB
	cout << "-> buildNotActiveBBUsList" << endl;
#endif // DEBUGB

	int i;
	OXCNode*pNode;
	bool found = false;
	inactiveBBUs.clear();

	//scan all candidate bbu pool nodes, sorted by their reachability cost (towards core co)
	for (i = 0; i < m_hWDMNet.hotelsList.size(); i++)
	{
		pNode = m_hWDMNet.hotelsList[i];
		this->m_hWDMNet.computeTrafficProcessed_BBUPooling(pNode, this->getConnectionDB().m_hConList);
		//if it is still inactive
		if (pNode->m_dTrafficProcessed == 0)
		{
			//insert in corresponding list
			inactiveBBUs.push_back(pNode);
		}
	}

#ifdef DEBUGB
	cout << "\tinactivePools list:";
	for (i = 0; i < inactiveBBUs.size(); i++)
	{
		cout << " " << inactiveBBUs[i]->getId();
	}
	cout << endl;
#endif // DEBUGB

}


//-B
void NS_OCH::NetMan::connDBDump()
{
	m_hConnectionDB.dump(cout);
	cout << endl;
	return;
}


//-B: my cost function based on lightpath given by dijkstra's algorithm
LINK_COST NS_OCH::NetMan::calculateCost(list<AbstractLink*>hPrimaryPath)
{
#ifdef DEBUGB
	cout << "-> calculateCost" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr = hPrimaryPath.begin();
	LINK_COST pathCost = (*itr)->getSrc()->getCost();

	while (itr != hPrimaryPath.end())
	{
		pathCost += (*itr)->getDst()->getCost();
		pathCost += (*itr)->getCost();
		itr++;
	}
	return pathCost;
}


bool NetMan::WP_BBU_ProvisionHelper_Unprotected(Connection*pCon, Circuit*pCircuit, OXCNode*pSrc, OXCNode*pDst)
{
#ifdef DEBUGB
	cout << "-> WP_BBU_ProvisionHelper_Unprotected" << endl;
#endif // DEBUG

	//-B: **************** CHOOSE ROUTING ALGORITHM (for BBU_ComputeRoute) ******************
	int algorithm_type = SHORTEST_PATH; //-B: shortest path = 0

	/////////////////////////////////////////////////////////////////////////////////////////////
	//******************************************************************************************
	/*
	AbsPath*pBestP = new AbsPath();

	hCost = m_hWDMNetPast.myAlg(*pBestP, pSrc, pDst, m_hWDMNetPast.numberOfChannels);

#ifdef DEBUGB
	cout << "\tFINITO MYALG. VAI CON DIJKSTRA" << endl;
#endif // DEBUGB

	list<AbstractLink*>pBestPath;

	//-B: PRE-PROCESSING PHASE to invalidate links???
	//.....................................
	Vertex *pVSrc = m_hGraph.lookUpVertex(pCon->m_nSrc, Vertex::VT_Channel_Out, pBestP->wlAssigned);	//-B: ATTENZONE: Originally VT_Access_Out!!!!!!!!!!
	Vertex *pVDst = m_hGraph.lookUpVertex(pCon->m_nDst, Vertex::VT_Channel_In, pBestP->wlAssigned);	//-B: ATTENZONE: Originally VT_Access_In!!!!!!!!!!!
	assert(pVSrc, pVDst);
	hCost = m_hGraph.Dijkstra(pBestPath, pVSrc, pVDst, AbstractGraph::LCF_ByOriginalLinkCost);

#ifdef DEBUGB
	cout << "\t
	shortest path: PrimaryCost = " << hCost << endl;
#endif // DEBUGB

	if (UNREACHABLE == hCost)
	{
		//pCon->m_eStatus = Connection::DROPPED; //-B: fatto nel chiamante BBU_ProvisionNew
		pCon->m_bBlockedDueToUnreach = true;
		return false;
	}

	BBU_NewCircuit(*pCircuit, pBestPath, pBestP->wlAssigned);

	pCircuit->Unprotected_setUpCircuit(this);
	
	return true;
	*/
	//*******************************************************************************************
	//////////////////////////////////////////////////////////////////////////////////////////////




	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//___________________________________________________________________________________________________________________________
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//---------------------------------------------------------------------------------------------------------------------------
	/*
	// ---------------------- UPDATE NETWORK GRAPH -------------------
	// Preprocess: Invalidate those links that don't have enough capacity
	m_hGraph.invalidateSimplexLinkDueToCap(pCircuit->m_eBW);
	//-B: LEGGERE! It could also work but something change wlOccupation values before choosing the random channel
	//	in updateOutLinks (inside myAlg). If not, it could be correct
	m_hWDMNetPast.invalidateWlOccupationLinks(pCircuit, m_hGraph);

	//-B: compute cost and insert the computed lightpath in the m_hRoute list (belonging to Circuit object)
	//-B: hCost viene calcolato col metodo calculateCost alla fine del metodo wpBBU_ComputeRoute

	hCost = m_hWDMNetPast.wpBBU_ComputeRoute(pCircuit, this, pSrc, pDst);

	//-B: m_hRouteA and m_hRouteB are still empty after ComputeRoute

#ifdef DEBUGB
	if (hCost == UNREACHABLE)
		cout << "Costo della route: UNREACHABLE";
	else
	{
		cout << "Costo della route: " << hCost;
	}
	cout << "\tBandw Circuit: " << pCircuit->m_eBW << endl;
#endif // DEBUGB

	if (UNREACHABLE != hCost) {
		//pCircuit->m_hRouteB = pCircuit->m_hRoute;
		//FABIO 20 sett: funzione che determina se il nuovo circuito in realtà si sovrappone ai circuiti gia esistenti
		//-B: andando a vedere se nei link su cui passano i lightpath
		//	appartenenti al circuito considerato c'è almeno un canale libero
		//	(!!!da vedere meglio nel caso una connessione non occupi un solo canale!!!)
		//	viene trattato sia il caso wavel continuity che wavel conversion
		ConflictType verifyCode;
		//if (evList->wpFlag)
		verifyCode = wpverifyCircuitUNPR(pCircuit);
		//else
		//verifyCode = verifyCircuitUNPR(pCircuit);

		if (verifyCode == NO_Conflict)
		{

#ifdef DEBUGB
			cout << "\nNO CONFLICT" << endl;
#endif // DEBUGB

			RouteConverterUNPR(pCircuit); //-B: imposto m_hRouteA uguale a m_hRouteB, che era stato preso da m_hRoute (vedi qualche riga più sopra)

										  //-B: update circuit status and setup
			pCircuit->m_eState = Circuit::CT_Setup;
			//Lightpath*Lp = (Lightpath*)(*pCircuit->m_hRoute.begin());
			//if (evList->wpFlag)
			pCircuit->Unprotected_setUpCircuit(this); //-B: WPsetUp(this);
												//else
			//pCircuit->BBU_setUpCir(this); //-B: setUp(this);
										  //Associate the circuit w/ the connection
			pCon->m_pPCircuit = pCircuit;

			//-B: Update connection status
			//pCon->m_eStatus = Connection::SETUP; //already done in calling method WP_BBU_Provision
			transport_cost = hCost;
		}
		else
		{
			cout << "CONFLICT! VerifyCode = " << verifyCode << endl;
			if (verifyCode == PRIMARY_Conflict)
			{
				pc++;//contatore conflitti su primario
					 //cout<<"PC:"<<pCon->m_nSequenceNo;
			}
			list<Lightpath*>::iterator itLP;
			//for(itLP = pCircuit->m_hRoute.begin(); itLP != pCircuit->m_hRoute.end(); itLP++) { //gatto
			//(*itLP)->m_used=1;	   }   //link usato --> m_used va a 1
			//MODIFICA OGGI			delete (*itLP); 
			delete pCircuit;
			pCon->m_eStatus = Connection::DROPPED;
			hCost = UNREACHABLE; //da fare: porre stato dropped o cose simili...
			transport_cost = 0;
		}
	}// chiusura if UNREACHEABLE != hCost
	else
	{ //else if hCost == UNREACHABLE

#ifdef DEBUGB
		cout << "PROVA A CALCOLARE LA ROUTE CON SEGMENT PROTECTION" << endl;
		cin.get();
		cin.get();
		cin.get();
		cin.get();
		cin.get();
#endif // DEBUGB

		// see if it can be provisioned using segment protection
		pSrc = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nSrc);
		pDst = (OXCNode*)m_hWDMNet.lookUpNodeById(pCon->m_nDst);

		//fabio 13 nov: mi interessa vedere se connessione sarebbe stata cmq rifiutata
		LINK_COST hSegCost;
		//if (evList->wpFlag)
		//{
		// -B: hCost viene calcolato col metodo calculateCost alla fine del metodo wpBBU_ComputeRoute
		hSegCost = m_hWDMNet.wpBBU_ComputeRoute(pCircuit, this, pSrc, pDst);
		//}
		//else
		//{
		//-B: hCost viene prima calcolato col metodo calculateCost usato nel metodo YenHelper;
		//	in seguito viene sovrascritto dal calcolo fatto direttamente in BBU_ComputeRoute (successivo alla chiamata del metodo Yen)
		hSegCost = m_hWDMNet.BBU_ComputeRoute(pCircuit, this, pSrc, pDst, algorithm_type);
		//}

		//if (UNREACHABLE != hSegCost){
		//   transport_cost=hSegCost;
		//  }
		//	else {
		// transport_cost=0;
		// }

		if (UNREACHABLE != hSegCost) {
			// m_hLog.m_nBlockedConDueToPath++;
			// CHECK_THEN_DELETE(pCircuit->m_hRoute.front());
			//FABIO 13 nov: 
			fc++;//FAKE CONFLICT
				 //cout<<"FS:"<<pCon->m_nSequenceNo;

		}
		//else cout<<"CapCON:"<<pCon->m_nSequenceNo;
		//for(itLP=pCircuit->m_hRoute.begin();itLP!=pCircuit->m_hRoute.end();itLP++)
		//MODIFICA OGGI		delete (*itLP);
		delete pCircuit;
		pCon->m_eStatus = Connection::DROPPED;
		m_hWDMNet.ConIdReq = true; //cosi non inserisco 2 volte consecutive in dbggap 
	}

#ifdef DEBUGB
	cout << "END BBU_Provision -> costo finale: " << hCost << endl;
#endif // DEBUGB

	return (UNREACHABLE != hCost);
	*/
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//___________________________________________________________________________________________________________________________
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//---------------------------------------------------------------------------------------------------------------------------
	return true;
}

void NS_OCH::NetMan::addConnection(Connection*pCon)
{
	m_hConnectionDB.addConnection(pCon);
}

//-B: print lightpaths referenced by channels, that (channels) are referenced by fibers
void NS_OCH::NetMan::printChannelLightpathNetPast()
{
#ifdef DEBUGB
	cout << endl << "-> PrintChannelLightpathNetPast" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itrLink;
	int i, w;
	for (itrLink = m_hWDMNetPast.m_hLinkList.begin(); itrLink != m_hWDMNetPast.m_hLinkList.end(); itrLink++)
	{
		UniFiber *pUniFiber = (UniFiber*)(*itrLink);
		assert(pUniFiber);
		//-B: print data only if the fiber is used, i.e. at least one channel on the fiber is used
		if (pUniFiber->getUsedStatus() == 1)
		{
			cout << "Fiber: " << pUniFiber->getId() << endl;
			for (w = 0; w < pUniFiber->m_nW; w++)
			{
				if (pUniFiber->m_pChannel[w].LightpathsIDs.size() > 0)
				{
					cout << "\tCANALE " << w << " - LIGHTPATHS: [";
					for (i = 0; i < pUniFiber->m_pChannel[w].LightpathsIDs.size(); i++)
					{
						cout << pUniFiber->m_pChannel[w].LightpathsIDs[i] << ",";
					}
					cout << "]" << endl;
				}
			}
		}//-B: else, nothing to print
	}
}

//-B: I should have used checkFreedomSimplexLink method to get some help
void NS_OCH::NetMan::invalidateSimplexLinkDueToFreeStatus()
{
#ifdef DEBUGB
	cout << "-> invalidateSimplexLinkDueToFreeStatus" << endl;
	//cout << "\tInvalido i ch:";
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr, itrL;
	SimplexLink*pLink;
	UniFiber*pUniFiber;
	int w;

	//-B: scorro i simplex link channels
	for (itr = m_hGraph.m_hLinkList.begin(); itr != m_hGraph.m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		//-B: se il simplex link considerato ha un pointer valido alla fibra => if (pLink->m_pUniFiber) 
		if (pLink->getSimplexLinkType() == SimplexLink::LT_Channel)
		{
			if (pLink->getValidity())
			{
				//-B: scorro le fibre della rete
				for (itrL = m_hWDMNet.m_hLinkList.begin(); itrL != m_hWDMNet.m_hLinkList.end(); itrL++)
				{
					pUniFiber = (UniFiber*)(*itrL);
					//cout << "\tfibers:";
					//cout << "\t" << pUniFiber->getId();
					//cout << "\t" << pLink->m_pUniFiber->getId() << " - simplex link type: " << pLink->getSimplexLinkType() << endl;
					//-B: se è la stessa fibra a cui appartiene/punta il simplex link considerato
					if (pLink->m_pUniFiber->getId() == pUniFiber->getId())
					{
						//-B: scorro tutti i canali della fibra considerata
						for (w = (pUniFiber->m_nW - 1); w >= 0; w--)
						{
							//cout << "\t\tchannels:";
							//cout << "\t" << w;
							//cout << "\t" << pLink->m_nChannel << endl;
							//-B: se è lo stesso canale del simplex link considerato
							if (pLink->m_nChannel == w)
							{
								//cout << "\t\t\tstatus: " << pUniFiber->m_pChannel[w].m_bFree << endl;
								//-B: se il canale non è completamente libero
								if (!(pUniFiber->m_pChannel[w].m_bFree))
								{
									//-B: invalida il simplex link corrispondente al canale considerato
									pLink->invalidate();
									//cout << " " << pLink->m_nChannel;
								}
								break;
							}
						}
						break;
					}
				} //end FOR netwrok links list
			}// end IF valid
		} //end IF LT_Channel
	} // end FOR graph links list
#ifdef DEBUGB
	cout << endl;
#endif // DEBUGB
}

void NS_OCH::NetMan::invalidateSimplexLinkDueToCapOrStatus(UINT bwd, UINT connType)
{
	list<AbstractLink*>::const_iterator itr, itrL;
	SimplexLink*pLink;
	UniFiber*pUniFiber;
	int w;

	//-B: scan all simplex links
	for (itr = m_hGraph.m_hLinkList.begin(); itr != m_hGraph.m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);

		//-B: *********** INVALIDATE SIMPLEX LINK DUE TO CAPACITY *************
		//-B: whatever type of simplex link (LT_X)
		//	if it is valid
		if (pLink->getValidity())
		{
			if (pLink->m_hFreeCap < bwd)
			{
				pLink->invalidate();
				//cout << " " << pSimplexLink->m_nChannel << "(type " << pSimplexLink->getSimplexLinkType() << ")";
				continue;
			}
		}

		//-B: *********** INVALIDATE SIMPLEX LINK DUE TO FREE STATUS *************
		//-B: don't know if the following part is really useful
		//-B: se il simplex link LT_Channel considerato ha un pointer valido alla fibra => if (pLink->m_pUniFiber) 
		if (pLink->getSimplexLinkType() == SimplexLink::LT_Channel)
		{
			//-B: if it is still valid
			if (pLink->getValidity())
			{
				//-B: scorro le fibre della rete
				for (itrL = m_hWDMNet.m_hLinkList.begin(); itrL != m_hWDMNet.m_hLinkList.end(); itrL++)
				{
					pUniFiber = (UniFiber*)(*itrL);
					//cout << "\tfibers:";
					//cout << "\t" << pUniFiber->getId();
					//cout << "\t" << pLink->m_pUniFiber->getId() << " - simplex link type: " << pLink->getSimplexLinkType() << endl;
					//-B: se è la stessa fibra a cui appartiene/punta il simplex link considerato
					if (pLink->m_pUniFiber->getId() == pUniFiber->getId())
					{
						//-B: scorro tutti i canali della fibra considerata
						for (w = (pUniFiber->m_nW - 1); w >= 0; w--)
						{
							//cout << "\t\tchannels:";
							//cout << "\t" << w;
							//cout << "\t" << pLink->m_nChannel << endl;
							//-B: se è lo stesso canale del simplex link considerato
							if (pLink->m_nChannel == w)
							{
								//cout << "\t\t\tstatus: " << pUniFiber->m_pChannel[w].m_bFree << endl;
								//-B: se il canale non è completamente libero
								if (!(pUniFiber->m_pChannel[w].m_bFree))
								{
									//-B: invalida il simplex link corrispondente al canale considerato
									pLink->invalidate();
									//cout << " " << pLink->m_nChannel;
								}
								break;
							}
						}
						break;
					}
				} //end FOR netwrok links list
			}// end IF valid
		} //end IF LT_Channel
		else if ((pLink->getSimplexLinkType() == SimplexLink::LT_Lightpath) && (connType!= 3 && connType != 4)) {

		  if (pLink->m_latency > LATENCYBUDGET) {

		  #ifdef DEBUGF

		  cout << "I'm going to invalidate the lightpath from " << pLink->m_pLightpath->getSrc()->getId()
		  << " to " << pLink->m_pLightpath->getDst()->getId() << "; its latency is " << pLink->m_latency << endl;
		  pLink->invalidate();
		  //cin.get();

		  #endif

		  }


		 }
	} // end FOR graph links list
}

void NS_OCH::NetMan::invalidateSimplexLinkDueToFreeStatus(BandwidthGranularity nBW)
{
	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;
	for (itr = m_hGraph.m_hLinkList.begin(); itr != m_hGraph.m_hLinkList.end(); itr++) {
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);

		if (pSimplexLink->m_hFreeCap < nBW)
		{
			pSimplexLink->invalidate();
			cout << " " << pSimplexLink->m_nChannel;
		}
	}
	cout << endl << endl;
}

void NS_OCH::NetMan::invalidateSimplexLinkDueToOccupancy()
{
#ifdef DEBUGB
	cout << "-> invalidateSimplexLinkDueToOccupancy" << endl;
	cout << "\tInvalido i ch:";
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr, itrL;
	SimplexLink*pLink;
	UniFiber*pUniFiber;
	int w;

	//-B: scorro i simplex link channels
	for (itr = m_hGraph.m_hLinkList.begin(); itr != m_hGraph.m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		//-B: se il simplex link considerato ha un pointer valido alla fibra == if (pLink->getSimplexLinkType() == LT_Channel)
		if (pLink->m_pUniFiber)
		{
			//-B: se c'è almeno una connessione già instradata in esso, lo devo invalidare
			if (pLink->m_hFreeCap < OCLightpath)
			{
				//-B: scorro le fibre della rete
				for (itrL = m_hWDMNet.m_hLinkList.begin(); itrL != m_hWDMNet.m_hLinkList.end(); itrL++)
				{
					pUniFiber = (UniFiber*)(*itrL);
					//-B: se è la stessa fibra a cui appartiene/punta il simplex link considerato
					//cout << "\tfibers:";
					//cout << "\t" << pUniFiber->getId();
					//cout << "\t" << pLink->m_pUniFiber->getId() << " - simplex link type: " << pLink->getSimplexLinkType() << endl;
					if (pLink->m_pUniFiber->getId() == pUniFiber->getId())
					{
						//-B: scorro tutti i canali della fibra considerata
						for (w = (pUniFiber->m_nW - 1); w >= 0; w--)
						{
							//cout << "\t\tchannels:";
							//cout << "\t" << w;
							//cout << "\t" << pLink->m_nChannel << endl;
							//-B: se è lo stesso canale del simplex link considerato
							if (pLink->m_nChannel == w)
							{
								//-B: invalida il simplex link corrispondente al canale considerato
								pLink->invalidate();
								cout << " " << pLink->m_nChannel;
								break;
							}
						}
						break;
					}
				} //end FOR
			} //end IF simplex link's free cap < channel's capacity
		} //end IF pointer to UniFiber valid
	} // end FOR
	cout << endl << endl;
}


//not used anymore. Circuit.computeLatency used instead
float NetMan::calculateLatency(list<AbstractLink*>&hLinkList)
{
#ifdef DEBUGB
	cout << "\n-> calculateLatency" << endl;
#endif // DEBUGB
	float lat = 0;

	list<AbstractLink*>::const_iterator itr = hLinkList.begin();
	while (itr != hLinkList.end())
	{
		SimplexLink *pLink = (SimplexLink*)(*itr);
		if (pLink->getSimplexLinkType() == SimplexLink::LT_Channel)
		{
			cout << "\tKm fibra: " << pLink->m_pUniFiber->m_nLength << " -> aggiungo ";
			cout.precision(8); cout << pLink->m_pUniFiber->m_latency;
			cout << " alla latenza" << endl;
			lat += pLink->m_pUniFiber->m_latency;
		}
		else if (pLink->getSimplexLinkType() == SimplexLink::LT_Converter)
		{
			cout << "\tSWITCH ELETTRONICO ATTRAVERSATO! Aggiungo ";
			cout.precision(8); cout << ELSWITCHLATENCY;
			cout << " alla latenza" << endl;
			lat = lat + (float)ELSWITCHLATENCY;
		}
		else
		{
			; //to do
		}
		itr++;
	}

#ifdef DEBUGB
	cout << "\tLatenza tot = ";
	cout.precision(8); cout << lat;
#endif // DEBUGB

	return lat;
}

//-B: costruisce la lista delle BBU già attive non piene, ordinata secondo il costo di reachability rispetto al core co
void NetMan:: genAuxBBUsList(vector<OXCNode*>&auxBBUsList)
{
	//per scrupolo (per fare un assert alla fine)
	auxBBUsList.clear();
	int i;

	//scorro la lista ordinata di tutti i candidate bbu hotel nodes (sorted in base al costo di reachability del core co)
	for (i = 0; i < m_hWDMNet.hotelsList.size(); i++)
	{
		if (BBUPOLICY == 2)
		{
			if (m_hWDMNet.hotelsList[i]->m_nBBUs > 0 /*&& m_hWDMNet.hotelsList[i]->m_nBBUs < m_hWDMNet.hotelsList[i]->m_uNumOfBBUSupported*/)
			{
				//lo inserisco nella lista dei nodi attivi non pieni
				auxBBUsList.push_back(m_hWDMNet.hotelsList[i]);
			}
		}
		else
		{
			if (m_hWDMNet.hotelsList[i]->m_nBBUs > 0 && m_hWDMNet.hotelsList[i]->m_nBBUs < MAXNUMBBU)
			{
				//lo inserisco nella lista dei nodi attivi non pieni
				auxBBUsList.push_back(m_hWDMNet.hotelsList[i]);
			}
		}
	}

#ifdef DEBUG
	cout << "\tauxBBUsList:";
	for (i = 0; i < auxBBUsList.size(); i++)
	{
		cout << " " << auxBBUsList[i]->getId();
	}
	cout << endl;
	if (auxBBUsList.size() > 1)
		NULL;
	//cin.get();
#endif // DEBUGB

}

//-B: costruisce la lista delle BBU già attive non piene, ordinata secondo il costo di reachability rispetto al core co
void NetMan::genAuxBBUsList_BBUPooling(vector<OXCNode*>&auxBBUsList, BandwidthGranularity backBwd)
{
	//per scrupolo (per fare un assert alla fine)
	auxBBUsList.clear();
	OXCNode*pNode;
	int i;

	//scorro la lista ordinata di tutti i candidate bbu hotel nodes (sorted in base al costo di reachability del core co)
	for (i = 0; i < m_hWDMNet.hotelsList.size(); i++)
	{
		pNode = m_hWDMNet.hotelsList[i];
		this->m_hWDMNet.computeTrafficProcessed_BBUPooling(pNode, this->getConnectionDB().m_hConList);
		//-B: if this pool node is active and still has enough resources to serve the new connection
		if (pNode->m_dTrafficProcessed > 0 && pNode->m_dTrafficProcessed + backBwd <= MAXTRAFFIC_FORPOOL)
		{
			//lo inserisco nella lista dei nodi attivi non pieni
			auxBBUsList.push_back(pNode);
		}
	}

#ifdef DEBUG
	cout << "\tauxBBUsList:";
	for (i = 0; i < auxBBUsList.size(); i++)
	{
		cout << " " << auxBBUsList[i]->getId();
	}
	cout << endl;
	if (auxBBUsList.size() > 1)
		NULL;
	//cin.get();
#endif // DEBUGB

}

//-B: return true if the source node (an hotel node) has 
bool NetMan::isAlreadyActive(OXCNode*pOXCsrc)
{
	for (int i = 0; i < this->m_hWDMNet.BBUs.size(); i++)
	{
		if (pOXCsrc->getId() == this->m_hWDMNet.BBUs[i]->getId())
		{
			return true;
		}
	}
	return false;
}

bool NetMan::checkAvailabilityHotelNode(OXCNode*pOXCsrc)
{
	if (BBUPOLICY == 2)
	{
		if (pOXCsrc->m_nBBUs < MAXNUMBBU)
			return true;
		return false;
	}
	else
	{
		if (pOXCsrc->m_nBBUs < MAXNUMBBU)
			return true;
		return false;
	}
}

bool NetMan::checkResourcesPoolNode(OXCNode*pOXCsrc, BandwidthGranularity backBwd)
{
	this->m_hWDMNet.computeTrafficProcessed_BBUPooling(pOXCsrc, this->getConnectionDB().m_hConList);
	if (pOXCsrc->getTrafficProcessed() + backBwd <= MAXTRAFFIC_FORPOOL)
		return true;
	//else
	return false;
}


AbstractLink* NetMan::lookUpSimplexLink(AbstractNode*pSrc, AbstractNode*pDst, int channel)
{
	return this->m_hGraph.lookUpSimplexLink(pSrc->getId(), Vertex::VT_Channel_Out, channel,
				pDst->getId(), Vertex::VT_Channel_In, channel);
}

int NetMan::getSimplexLinkGraphSize()
{
	return this->m_hGraph.m_hLinkList.size();
}

void NetMan::removeSimplexLinkFromGraph(Lightpath*pLightpath)
{
	m_hGraph.removeSimplexLinkLightpath(pLightpath);
}

void NetMan::removeEmptyLinkFromGraph()
{
	m_hGraph.removeSimplexLinkLightpath();
}


void NetMan::releaseVirtualLinkBandwidth(UINT nBWToRelease, Lightpath*pLightpath)
{
	m_hGraph.releaseSimplexLinkLightpathBwd(nBWToRelease, pLightpath);
}


void NetMan::checkSrcDstLightpath(Lightpath*pLightpath)
{
	UniFiber* itr;
	
	//access first element
	itr = pLightpath->m_hPRoute.front();
	//verify src
	assert(pLightpath->getSrc()->getId() == itr->getSrc()->getId());

	//go to the last element
	itr = pLightpath->m_hPRoute.back();
	//verify dst
	assert (pLightpath->getDst()->getId() == itr->getDst()->getId());
}


void NetMan::checkSimplexLinkRemoval(Lightpath*pLightpath)
{
	assert(m_hGraph.isSimplexLinkRemoved(pLightpath) == true);
}

void NetMan::buildBestHotelsList()
{
	cout << "-> buildBestHotelsList" << endl << "..." << endl;
	vector<OXCNode*>::const_iterator itr;

	this->m_hWDMNet.fillHotelsList();

	switch (BBUPOLICY)
	{
		case 0:
			this->m_hWDMNet.sortHotelsList(m_hGraph); //sort by stage + reachability + distance(km)
			//this->m_hWDMNet.sortHotelsList_Evolved(m_hGraph); //sort by a "close" reachability/proximity
			break;
		case 1:
			NULL; //no need to sort hotel nodes for this policy
			break;
		case 2:
			//this->m_hWDMNet.computeShortestPaths(m_hGraph); //USELESS because it cannot be exploited
			this->m_hWDMNet.setNodesStage(m_hGraph);
			this->m_hWDMNet.setBetweennessCentrality(m_hGraph);
			this->m_hWDMNet.setReachabilityDegree(m_hGraph);
			this->m_hWDMNet.sortHotelsListByMetric();
			//this->m_hWDMNet.setProximityDegree(m_hGraph); -> OBSOLETE: we chose to use only the betweenness centrality
#ifdef DEBUGB
			cout << "\tNum di candidate hotel nodes nella rete: " << m_hWDMNet.hotelsList.size() << endl;
			for (itr = m_hWDMNet.hotelsList.begin(); itr != m_hWDMNet.hotelsList.end(); itr++)
			{
				cout << "\tNodo #" << (*itr)->getId() << " - stage: " << (*itr)->m_nNetStage << " - reach degree: " << (*itr)->m_nReachabilityDegree
					<< " - betw centr: " << (*itr)->m_nBetweennessCentrality << endl;
			}
			//cin.get();
#endif // DEBUGB
			break;
		case 3:
			this->m_hWDMNet.sortHotelsList(m_hGraph);
			break;
		default: 
			DEFAULT_SWITCH;
	}

	//-B: build reachable/best hotels list for each node, so that a source node doesn't have to check
	//	the reachability of every hotel node while choosing the best one to host its BBU
	//buildHotelsListForNode(); //-B: not implemented yet
}

/*
Monica
This function is used to compute the latency of a path (used instead of computeLatency where grooming is not done correctly)
*/

float NetMan::computeLatencyP3(list<AbstractLink*>&path, Vertex* pSrc, UINT connType)
{
#ifdef DEBUG
	cout << "-> computeLatencyP3" << endl;
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
#ifdef DEBUG
				cout << "\t+ Fiber's length = " << pLink->m_pUniFiber->getLength() << endl;
				cout << "\t++ Cost of the link = " << pLink->getCost() << endl;
				cout << "\t+ From " << pLink->m_pUniFiber->getSrc()->getId() << " to " << pLink->m_pUniFiber->getDst()->getId() << endl;
#endif // DEBUG

				latency += pLink->m_pUniFiber->m_latency;
				break;
			}
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Lightpath:
			{

				UINT lightpathSrc = pLink->m_pLightpath->getSrc()->getId();
				UINT lightpathDst = pLink->m_pLightpath->getDst()->getId();
				UINT src = pSrc->m_pOXCNode->getId();

#ifdef DEBUG
				cout << "\t+ Lightpath's length = " << pLink->getLength() << endl;
				cout << "\t Link from " << lightpathSrc << " -> " << pLink->m_pLightpath->getDst()->getId() 
					<< " on wavelength " << pLink->m_pLightpath->wlAssigned << endl;
				cout << "\t++ Cost of the link = " << pLink->getCost() << endl;

#endif // DEBUGB


				//If the lightpath starts from the source, I don't have to consider grooming
				if (src != lightpathSrc || (connType != 1 && connType != 2)) {
					if (!pLink->m_pLightpath->m_groomingAlreadydone) {
						list<Connection*>::const_iterator itr;
						Connection* pCon;
						for (itr = pLink->m_pLightpath->m_hConnectionsUsingLightpath.begin(); itr != pLink->m_pLightpath->m_hConnectionsUsingLightpath.end(); itr++) {
							pCon = (Connection*)(*itr);

							//it means it'a backhaul 
							if ((connType != 1 && connType != 2) && (pCon->m_nSrc != pCon->m_nDst)) {

								//grooming with a midhaul connection
								if ((pCon->m_nSrc!=src) && (pCon->m_eConnType == Connection::MOBILE_FRONTHAUL || pCon->m_eConnType == Connection::MOBILE_FRONTHAUL)) {
#ifdef DEBUG
									cout << "Here i update grooming for backhaul" << endl;
									cout << "connection from " << pCon->m_nSrc << " to " << pCon->m_nDst
										<< " of type fronthaul" << endl;
#endif
									bool connPresent = checkifConnectionpresent(pCon->m_nSequenceNo);
									if (!connPresent) {
										groomingConnections.push_front(pCon->m_nSequenceNo, pCon);
									}
									if ((pCon->m_dRoutingTime + ELSWITCHLATENCY) > LATENCYBUDGET) {
									
										latency = MAXVALUE_LATENCY;
										return latency;
									}
								}

							}
							//it means it's a midhaul connection involved in the grooming
							if (src != lightpathSrc && pCon->m_nSrc != pCon->m_nDst && (pCon->m_nSrc != src)) {

								bool connPresent = checkifConnectionpresent(pCon->m_nSequenceNo);
								if (!connPresent) {
									groomingConnections.push_front(pCon->m_nSequenceNo, pCon);
								}
#ifdef DEBUGX
								cout << "\t Connection from " << pCon->m_nSrc << " to " << pCon->m_nDst
									<< "  with seq number " << pCon->m_nSequenceNo << " is on this lightpath" << endl;
#endif
							}

							/*
							// Don't consider connections starting from the source
							// Consider connections having as source the lightpath source
							// Don't consider connections starting and ending at the same node
							
							*/
						}



					}
					else if (pLink->m_pLightpath->m_groomingAlreadydone) { //grooming is already done on this lightpath -> dont add latency another time
#ifdef DEBUG
						cout << "\t++ Cost of the link = " << pLink->getCost() << endl;
						cout << "\t Grooming already done by a previous connection" << endl;
#endif
					}
				}


				latency += pLink->m_latency;
				break;
			}
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Converter:
			{
#ifdef DEBUG
				cout << "\t++ Cost of the link = " << pLink->getCost() << endl;
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
	return latency;
}

void NetMan::clearPrecomputedPath()
{
	//-B: precomputedPath cleared, from previous cycle calculation, each time the cycle calls BBU_newConnection method
	precomputedPath.clear();
}

void NetMan::consumeBandwOnSimplexLink(Lightpath*pLightpath, UINT BWToBeAllocated)
{
	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink;

	//-B: it is not possible to use Graph.lookUpSimplexLink instead of scan all the link list

	for (itr = this->m_hGraph.m_hLinkList.begin(); itr != this->m_hGraph.m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		if (pLink->m_pLightpath == pLightpath)
		{
			pLink->m_hFreeCap -= BWToBeAllocated;
			assert(pLightpath->getFreeCapacity() == pLink->m_hFreeCap);
			break;
		}
	}
}


UINT NetMan::checkEmptyLightpaths()
{
	UINT count = 0;
	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink;

	for (itr = this->m_hGraph.m_hLinkList.begin(); itr != this->m_hGraph.m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		switch (pLink->getSimplexLinkType())
		{
			case SimplexLink::SimplexLinkType::LT_Channel:
			case SimplexLink::SimplexLinkType::LT_Channel_Bypass:
			case SimplexLink::SimplexLinkType::LT_Converter:
			case SimplexLink::SimplexLinkType::LT_Demux:
			case SimplexLink::SimplexLinkType::LT_Grooming:
			case SimplexLink::SimplexLinkType::LT_Mux:
			case SimplexLink::SimplexLinkType::LT_Rx:
			case SimplexLink::SimplexLinkType::LT_Tx:
			case SimplexLink::SimplexLinkType::LT_UniFiber:
				NULL;
				break;
			case SimplexLink::SimplexLinkType::LT_Lightpath:
			{
				if (pLink->m_hFreeCap == OCLightpath)
					count++;
			}
		}
	}

	return count;
}

void NetMan::invalidateGrooming()
{
	m_hGraph.invalidateGrooming();
}


//-B: invalidate all simplex links whose type is LT_Converter or LT_Grooming
//	so that only wavel continuity is possible
// USED FOR: invalidate, if possible, LT_Converter and LT_Grooming links in candidate hotel nodes without any BBUs
void NetMan::invalidateSimplexLinkGrooming()
{
#ifdef DEBUGB
	cout << "-> invalidateSimplexLinkGrooming" << endl;
#endif // DEBUGB

	//build grooming nodes list
	this->getGroomNodesList();

	OXCNode*pNode;
	SimplexLink *pSimplexLink;
	list<AbstractLink*>::iterator itr;
	Vertex*pVertex;
	
	for (itr = this->m_hGraph.m_hLinkList.begin(); itr != this->m_hGraph.m_hLinkList.end(); itr++)
	{
		pSimplexLink = (SimplexLink*)(*itr);
		assert(pSimplexLink);
		switch (pSimplexLink->m_eSimplexLinkType)
		{
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_Converter:
			{
				if (pSimplexLink->getValidity())
				{
					pVertex = (Vertex*)(pSimplexLink->getSrc());
					pNode = pVertex->m_pOXCNode;
					
					//-B: we have to remove the grooming capability only if there are no bbus hosted in this node and...
					if (pNode->m_nBBUs <= 0)
					{
						bool groom = false;
						// ...there is no grooming currently performed in this node (otherwise we cannot deactivate it)
						vector<UINT>::const_iterator itr;
						UINT id;
						//-B: scan all grooming nodes ids in the corresponging list
						for (itr = this->m_hWDMNet.groomingNodesIds.begin(); itr != this->m_hWDMNet.groomingNodesIds.end(); itr++)
						{
							id = (UINT)(*itr);
							//-B: if the considered node is present in the list
							if (id == pNode->getId())
							{
								groom = true;
#ifdef DEBUGB
								cout << "\tC'E' GROOMING NEL NODO " << pNode->getId();
								//cout << "\tGrooming nodes list:";
								//for (itr = this->m_hWDMNet.groomingNodesIds.begin(); itr != this->m_hWDMNet.groomingNodesIds.end(); itr++)
								//{
									//cout << " " << ((UINT)(*itr));
								//}
								//cin.get();
								//this->dump(cout);
								//cin.get();
#endif // DEBUGB
								break;
							}
						}//end FOR
						if (!groom)
						{
							//-B: I'm invalidating either a LT_Gromming simplex link or a LT_Converter simplex link
							//	belonging to a node in which there are no active BBUs and grooming is not performed
							pSimplexLink->invalidate();
						}
					}// end IF bbus = 0
				}//end IF validity
				break;
			}//end case
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
		}//end SWITCH
	}//end FOR simplex links
}

void NetMan::validateGroomingHotelNode()
{
	this->m_hGraph.validateGroomingHotelNode();
}


UINT NetMan::countGroomingNodes()
{
	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink;
	UINT groomNodes = 0;

	for (itr = this->m_hGraph.m_hLinkList.begin(); itr != this->m_hGraph.m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);
		//-B: there is only 1 simplex link LT_Grooming for each node
		if (pLink->getSimplexLinkType() == SimplexLink::LT_Grooming)
		{
			if (pLink->getValidity())
			{
				groomNodes++;
#ifdef DEBUGB
				OXCNode*pNode = ((Vertex*)(pLink->getSrc()))->m_pOXCNode;
				//-B: NO SENSE IN CASE OF INTER_BBUPOOLING
				cout << "\tNodo " << pNode->getId() << " risulta avere " << pNode->m_nBBUs << " BBUs attive" << endl;
#endif // DEBUGB
			}
		}
	}
	return groomNodes;
}


UINT NetMan::countLinksInGraph()
{
	return this->m_hGraph.m_hLinkList.size();
}

//-B: add to a list all nodes in which grooming is performed
void NetMan::getGroomNodesList()
{
	list<Connection*>::const_iterator itr;
	list<AbstractNode*>::const_iterator itrNode;
	list<Circuit*>::iterator itrCir;
	Circuit*pCir;
	Connection*pCon;

	//-B: reset grooming nodes ids list
	this->m_hWDMNet.groomingNodesIds.clear();

	//-B: scan all active connections
	for (itr = this->m_hConnectionDB.m_hConList.begin(); itr != this->m_hConnectionDB.m_hConList.end(); itr++)
	{
		pCon = (Connection*)(*itr);
		//-B: case of connection with bw <= OCLightpath
		if (pCon->m_pPCircuit)
		{
			assert(pCon->m_pCircuits.size() == 0);
			buildGroomNodeList(pCon->m_pPCircuit);
		}
		//-B: case of connection with bw > OCLightpath (fixed backhaul)
		else if (pCon->m_pCircuits.size() > 0)
		{
			for (itrCir = pCon->m_pCircuits.begin(); itrCir != pCon->m_pCircuits.end(); itrCir++)
			{
				pCir = (Circuit*)(*itrCir);
				buildGroomNodeList(pCir);
			}
		}
	} //end FOR
}

void NetMan::buildGroomNodeList(Circuit*pCircuit)
{
	list<Lightpath*>::const_iterator itr;
	Lightpath*pLp;
	OXCNode*pNode;

	itr = pCircuit->m_hRoute.begin();
	//-B: do not take into account the first lp, since it could be a case of multiplexing, but not of grooming
	//	grooming can happen only after the first lightpath of the path over which the connection is routed
	itr++;
	while (itr != pCircuit->m_hRoute.end())
	{
		pLp = (Lightpath*)(*itr);
		//-B: if in this lightpath is routed another circuit beside the current one
		if (pLp->m_hCircuitList.size() > 1)
		{
			pNode = pLp->getSrcOXC();
			this->m_hWDMNet.groomingNodesIds.push_back(pNode->getId());
		}
		itr++;
	}
}


void NetMan::logNetCostPeriodical(SimulationTime hTimeSpan)
{
	//-B: log peak cost
	UINT nCost = computeNetworkCost();
	if (nCost > this->m_hLog.peakNetCost)
	{
		this->m_hLog.peakNetCost = nCost;
	}

	//-B: log average cost
	this->m_hLog.networkCost += (nCost*hTimeSpan);
	this->m_runLog.networkCost += (nCost*hTimeSpan);

#ifdef DEBUG
	if (nCost < 100)
	{
		cout << "\tCosto rete = " << nCost << endl;
		this->m_hWDMNet.printBBUs();

		this->m_hWDMNet.printHotelNodes();
		//cin.get();
	}
#endif // DEBUGB
}

void NetMan::logActiveLpPeriodical(SimulationTime hTimeSpan)
{
	UINT numLightpaths = this->getLightpathDB().m_hLightpathList.size();
	//-B: log peak
	if (numLightpaths > this->m_hLog.peakNumLightpaths)
	{
		this->m_hLog.peakNumLightpaths = numLightpaths;
	}

	//-B: log average
	this->m_hLog.avgActiveLightpaths += (numLightpaths*hTimeSpan);
	this->m_runLog.avgActiveLightpaths += (numLightpaths*hTimeSpan);
}

void NetMan::logActiveHotelsPeriodical(SimulationTime hTimeSpan)
{
	//-B: log peak active nodes
	UINT numActiveNodes = this->m_hWDMNet.countActiveNodes();
	if (numActiveNodes > this->m_hLog.peakNumActiveNodes)
	{
		this->m_hLog.peakNumActiveNodes = numActiveNodes;
	}

	//-B: log average
	this->m_hLog.avgActiveNodes += (numActiveNodes*hTimeSpan);
	this->m_runLog.avgActiveNodes += (numActiveNodes*hTimeSpan);
}

void NetMan::logActiveBBUs(SimulationTime hTimeSpan)
{
	//-B: log peak active BBUs
	UINT numActiveBBUs = this->m_hWDMNet.countBBUs();
	if (numActiveBBUs > this->m_hLog.peakNumActiveBBUs)
	{
		this->m_hLog.peakNumActiveBBUs = numActiveBBUs;
	}

	//-B: log average
	this->m_hLog.avgActiveBBUs += (numActiveBBUs*hTimeSpan);
	this->m_runLog.avgActiveBBUs += (numActiveBBUs*hTimeSpan);
}

void NetMan::logActiveSmallCells(SimulationTime hTimeSpan)
{
	//-B: log peak active BBUs
	UINT numActiveSC = this->m_hWDMNet.countActiveSmallCells();
	if (numActiveSC > this->m_hLog.peakActiveSC)
	{
		this->m_hLog.peakActiveSC = numActiveSC;
	}

	//-B: log average
	this->m_hLog.avgActiveSC += (numActiveSC*hTimeSpan);
	this->m_runLog.avgActiveSC += (numActiveSC*hTimeSpan);
}

//-B: reduce the number of candidate hotel nodes according to the consolidation factor
//	NOT USED ANYMORE
void NetMan::reduceHotelsForConsolidation(UINT numOfHotels)
{
#ifdef DEBUGB
	cout << "->reduceHotelsForConsolidation" << endl;
#endif // DEBUGB

	cout << "Hotels list (" << numOfHotels << " hotels):";
	vector<OXCNode*>::iterator itr;

	if (numOfHotels < m_hWDMNet.hotelsList.size())
	{
		itr = this->m_hWDMNet.hotelsList.begin();
		//-B: till the end of the list isn't reached...
		while (this->m_hWDMNet.hotelsList.size() > numOfHotels)
		{
			// ...erase the element of the list
			this->m_hWDMNet.hotelsList.erase(itr + numOfHotels);
		}
	}

	//to check during simulation
	for (itr = this->m_hWDMNet.hotelsList.begin(); itr != this->m_hWDMNet.hotelsList.end(); itr++)
	{
		cout << " " << (*itr)->getId();
	}
#ifdef DEBUGB
	cin.get();
#endif // DEBUGB

}

//-B: count all the times the grooming is performed
//	the only problem is that it counts just once for each circuit of each connection,
//	even if potentially, within a circuit, there could be 2 grooming times,
//	one for a lightpath and the second one for another lightpath (of the same circuit)
void NetMan::checkGrooming(Connection*pCon)
{
	list<Circuit*>::iterator itrCir;
	Circuit*pCir;
	bool groomEvent;

	//-B: case of connection with bw <= OCLightpath
	if (pCon->m_pPCircuit)
	{
		assert(pCon->m_pCircuits.size() == 0);
		groomEvent = false;
		groomEvent = pCon->m_pPCircuit->checkGrooming();
		if (groomEvent)
		{
			pCon->grooming++;
#ifdef DEBUGB
			cout << "\tGROOMING!" << endl;
#endif // DEBUGB
		}
	} //end IF
	//-B: case of connection with bwd > OCLightpath (fixed backhaul)
	else if (pCon->m_pCircuits.size() > 0)
	{
		for (itrCir = pCon->m_pCircuits.begin(); itrCir != pCon->m_pCircuits.end(); itrCir++)
		{
			pCir = (Circuit*)(*itrCir);
			groomEvent = false;
			groomEvent = pCir->checkGrooming();
			if (groomEvent)
			{
				pCon->grooming++;
#ifdef DEBUGB
				cout << "\tGROOMING!" << endl;
#endif // DEBUGB
			}
		} //end FOR circuits
	} //end ELSE IF

}

void NetMan::computeAvgLatency(Event*pEvent)
{
	assert(pEvent->m_pConnection->m_eConnType == Connection::MOBILE_FRONTHAUL
		|| pEvent->m_pConnection->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL);

	this->m_hLog.avgLatency += pEvent->m_pConnection->m_dRoutingTime;
	this->m_runLog.avgLatency += pEvent->m_pConnection->m_dRoutingTime;

	//-B: it will be needed at the end of the simulation to print the correct avg latency value
	this->m_hLog.countConnForLatency++;
	this->m_runLog.countConnForLatency++;
}
//Check for every link on the path if it exceeds the occupancy threshold: it is enough
//that one link exceeds the threshold to return false
bool NetMan::noFullLinks(list<AbstractLink*>&pPath) 
{
#ifdef DEBUG
	cout << "-> areThereFullLinks" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink = NULL;

	for (itr = pPath.begin(); itr != pPath.end(); itr++)
	{
		if (((SimplexLink*)(*itr))->getSimplexLinkType() == SimplexLink::LT_Channel
			|| ((SimplexLink*)(*itr))->getSimplexLinkType() == SimplexLink::LT_Lightpath)
		{
			pLink = (SimplexLink*)(*itr);
			if (pLink == NULL) {
				//it could mean that src and dst node are the same node
				return true;

			}
			if (((SimplexLink*)(*itr))->getSimplexLinkType() == SimplexLink::LT_Channel) {

#ifdef DEBUGC
				cout << "\tLINK (" << ((Vertex*)(pLink->getSrc()))->m_pOXCNode->getId() << "->"
					<< ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId() << ") DI TIPO CHANNEL";

				cout << " - free cap = " << pLink->m_hFreeCap << endl;
#endif
				UINT nFreeChannels = pLink->m_pUniFiber->countFreeChannels();
				double MinimumCapacityFree = (pLink->m_pUniFiber->numOfChannels()) - LINKOCCUPANCY_PERC*(pLink->m_pUniFiber->numOfChannels());

#ifdef DEBUGC
				list<Connection*>::const_iterator itr2;
				Connection* pConDB;
				Lightpath* lightpathDB;
				for (itr2 = this->m_hConnectionDB.m_hConList.begin(); itr2 != this->m_hConnectionDB.m_hConList.end(); itr2++) {
					pConDB = (Connection*)(*itr2);
					list<Lightpath*>::iterator itLP;
					if (pConDB->m_pPCircuit != NULL) {
						for (itLP = pConDB->m_pPCircuit->m_hRoute.begin(); itLP != pConDB->m_pPCircuit->m_hRoute.end(); itLP++) {
							lightpathDB = (Lightpath*)(*itLP);
							UINT linkSrc = ((Vertex*)(pLink->getSrc()))->m_pOXCNode->getId();
							UINT linkDst = ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId();
							if (lightpathDB->getSrcOXC()->getId() == linkSrc && lightpathDB->getDstOXC()->getId() == linkDst) {
								//if (lightpathDB->m_nFreeCapacity <= 29) {

									cout << "Lightpath #" << lightpathDB->wlAssigned << " with capacity " << lightpathDB->m_nFreeCapacity << " from " <<
										lightpathDB->getSrcOXC()->getId() << " to " << lightpathDB->getDstOXC()->getId() <<
										" from source " << pConDB->m_nSrc << endl;
								//}
							}
						}
					}
				}
#endif

#ifdef DEBUGC
				cout << "\tI canali liberi su questa fibra sono: " << nFreeChannels << endl;
				cout << "\tSpazio libero richiesto: " << MinimumCapacityFree << endl;
#endif // DEBUG

				//-B: if the number of the free channels on this fiber is 1
				//	it means that this free channel is the channel that this connection is going to occupy
				if (nFreeChannels <= MinimumCapacityFree) {
					cout << "\t==> reject the hotel" << endl;
					return false;
				}
			}
			else if (pLink->getSimplexLinkType() == SimplexLink::LT_Lightpath)
			{
#ifdef DEBUGC
				cout << "\tLINK (" << ((Vertex*)(pLink->getSrc()))->m_pOXCNode->getId() << "->"
					<< ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId() << ") DI TIPO LIGHPATH" << endl;
#endif			
			}
		}
	}

	if (pLink == NULL) {
		return true;
	}
#ifdef DEBUG
	cout <<"\t==> accept the hotel " << ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId() << endl;
#endif
	return true;
}


//-B: if last link's load overcome a certain threshold, it will return false
//	this way, the candidate hotel node won't be selected as a potential hotel
bool NetMan::isLastLinkFull(list<AbstractLink*>&pPath)
{
#ifdef DEBUG
	cout << "-> isLastLinkFull" << endl;
#endif // DEBUGB
	
	list<AbstractLink*>::const_iterator itr;
	SimplexLink*pLink = NULL;

	for (itr = pPath.begin(); itr != pPath.end(); itr++)
	{
		if (((SimplexLink*)(*itr))->getSimplexLinkType() == SimplexLink::LT_Channel
			|| ((SimplexLink*)(*itr))->getSimplexLinkType() == SimplexLink::LT_Lightpath)
		{
			pLink = (SimplexLink*)(*itr);

		}
	}
	//-B: it will go out from this cycle having the last true link saved in pLink (of type either LT_Channel or LT_Lightpath)

	if (pLink == NULL)
	{
		//it could mean that src and dst node are the same node
		return true;
	}

#ifdef DEBUG
	cout << "\tULTIMO LINK (" << ((Vertex*)(pLink->getSrc()))->m_pOXCNode->getId() << "->"
		<< ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId() << ") DI TIPO: ";
	if (pLink->getSimplexLinkType() == SimplexLink::LT_Channel)
		cout << "LT_Channel";
	else if (pLink->getSimplexLinkType() == SimplexLink::LT_Lightpath)
		cout << "LT_Lightpath";
	else
		assert(false);
	cout << " - free cap = " << pLink->m_hFreeCap << endl;
#endif // DEBUG

	//-B: if this last link is a LT_Channel simplex link -> it means that is a free channel (otherwise, it would have been a LT_Lightpath)
	if (pLink->getSimplexLinkType() == SimplexLink::LT_Channel) {

		UINT nFreeChannels = pLink->m_pUniFiber->countFreeChannels();
		double MinimumCapacityFree = (pLink->m_pUniFiber->numOfChannels()) - LINKOCCUPANCY_PERC*(pLink->m_pUniFiber->numOfChannels());

#ifdef DEBUGC
		cout << "\tI canali liberi su questa fibra sono: " << nFreeChannels << endl;
		cout << "\tSpazio libero richiesto: " << MinimumCapacityFree << endl;
#endif // DEBUG

		//-B: if the number of the free channels on this fiber is 1
		//	it means that this free channel is the channel that this connection is going to occupy
		if (nFreeChannels <= MinimumCapacityFree) {
#ifdef DEBUG
			cout << "\t==> reject the hotel " << ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId() << endl;
			//cin.get();
#endif // DEBUGB
			//-B: it means there is congestion -> reject this hotel
			return false;
		}
		else //if, for example, LINKOCCUPANCY_THR == 1, the num of free channels should be at least equal to 2
		{
#ifdef DEBUG
			cout << "\t==> accept the hotel " << ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId() << endl;
#endif // DEBUGB
			//hotel ok
			return true;
		}
	}
	//-B: if the path uses a simplex link LT_Lightpath as last link
	else if (pLink->getSimplexLinkType() == SimplexLink::LT_Lightpath)
	{
#ifdef DEBUG
		cout << "\t==> accept the hotel " << ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId() << endl;
		//cin.get();
#endif // DEBUGB
		//at least for the moment, accept the hotel
		return true;
	}
	else
	{
		assert(false);
	}
	//assert(false);
	//return true;
}


void NetMan::genSmallCellCluster()
{
	list<AbstractNode*>::const_iterator itr;
	list<AbstractLink*>::const_iterator itr2;
	OXCNode*pNode;
	UniFiber*pFiber;

	for (itr = this->m_hWDMNet.m_hNodeList.begin(); itr != this->m_hWDMNet.m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		if (pNode->isMacroCell())
		{
			for (itr2 = this->m_hWDMNet.m_hLinkList.begin(); itr2 != this->m_hWDMNet.m_hLinkList.end(); itr2++)
			{
				pFiber = (UniFiber*)(*itr2);
				if (pFiber->getDst()->getId() == pNode->getId())
				{
					if (((OXCNode*)(pFiber->getSrc()))->isSmallCell())
					{
						pNode->m_vSmallCellCluster.push_back((OXCNode*)(pFiber->getSrc()));
					}
				}
			} //end FOR fibers
#ifdef DEBUGB
			cout << "MACRO cell #" << pNode->getId() << " ha " << pNode->m_vSmallCellCluster.size() << " small cells" << endl;
#endif // DEBUGB
		}
	} //end FOR nodes
}

//-B: VERY BIG CHECK: CONTROL FREE CAPACITY OF LIGHTPATHS, SIMPLEX LINKS LT_Lightpath AND LT_Channel
//	if you have doubts about correct provisioning and deprovisioning of connections, let it uncommented at the end of while cycle in Simulator.run
void NetMan::checkLpToSLinkEquality()
{
#ifdef DEBUG
	cout << "\n\n-> checkLpToSLinkEquality" << endl;
	cout << "INZIO IL GRAN CHECK! Press Enter" << endl;
#endif // DEBUGB

	list<AbstractLink*>::const_iterator itr, itr2;
	SimplexLink*pLink, *pLink2;
	list<Lightpath*>::const_iterator itrLp;
	Lightpath*pLp;
	list<UniFiber*>::const_iterator itrFiber;
	UniFiber*pFiber;

///////////////////////////////////////////////////////////////
#ifdef DEBUGX
	cout << "\n\tFIRST CHECK! Controllo che lightpath e simplex link LT_Lightpath abbiano la stessa free capacity" << endl;
#endif // DEBUGB

	for (itr = this->m_hGraph.m_hLinkList.begin(); itr != this->m_hGraph.m_hLinkList.end(); itr++)
	{
		pLink = (SimplexLink*)(*itr);

		switch (pLink->getSimplexLinkType())
		{
			case SimplexLink::LT_Lightpath:
			{
				for (itrLp = this->getLightpathDB().m_hLightpathList.begin(); itrLp != this->getLightpathDB().m_hLightpathList.end(); itrLp++)
				{
					pLp = (Lightpath*)(*itrLp);
					if (pLink->m_pLightpath == pLp)
					{
#ifdef DEBUGX
						cout << "\tTrovato lightpath " << pLp->getId() << " (" << ((OXCNode*)pLp->getSrc())->getId() << "->" << ((OXCNode*)pLp->getDst())->getId() << ") "
							<< " corrispondente al simplex link LT_Lightpath " << pLink->getId() << " (" << ((Vertex*)pLink->getSrc())->m_pOXCNode->getId()
							<< "->" << ((Vertex*)pLink->getDst())->m_pOXCNode->getId() << ")" << endl << endl;
#endif // DEBUG		
						if (pLp->m_nFreeCapacity != pLink->m_hFreeCap)
						{
#ifdef DEBUG
							cout << "\tATTENTION! Free cap of lightpath: " << pLp->m_nFreeCapacity << endl;
							cout << "\t\tFree cap of simplex link LT_Lightpath: " << pLink->m_hFreeCap << endl << endl;
							cin.get();
#endif // DEBUGB
						}
						else
						{
#ifdef DEBUGX
							cout << "\tCheck sulla free capacity di lp e slink LT_Lightpath superato!" << endl << endl;
#endif // DEBUGB
						}
					} //end
				}
				break;
			}
			case SimplexLink::LT_Channel:
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Converter:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Tx:
				break; //nothing to do
		} //end SWITCH
	}
///////////////////////////////////////////////////////////////
#ifdef DEBUG
	cout << "\n\tSECOND CHECK! Controllo che i canali utilizzati dal lightpath siano occupati" << endl;
#endif // DEBUG

	for (itrLp = this->getLightpathDB().m_hLightpathList.begin(); itrLp != this->getLightpathDB().m_hLightpathList.end(); itrLp++)
	{
		pLp = (Lightpath*)(*itrLp);
		for (itrFiber = pLp->m_hPRoute.begin(); itrFiber != pLp->m_hPRoute.end(); itrFiber++)
		{
			pFiber = (UniFiber*)(*itrFiber);
			for (int w = 0; w < pFiber->m_nW; w++)
			{
				if (w == pLp->wlAssigned)
				{
					if (pFiber->m_pChannel[w].m_bFree == true)
					{
#ifdef DEBUGB
						/*cout << "\tATTENTION! Channel " << w << " used by this lightpath on fiber " << pFiber->getId()
							<< " (" << pFiber->getSrc() << "->" << pFiber->getDst() << ") seems to be free!" << endl << endl;
						*/
						//cin.get();
#endif // DEBUGB

					}
					else
					{
#ifdef DEBUGB
						//cout << "\tChannel " << w << " on fiber " << pFiber->getId() << " is NOT free. Good!" << endl << endl;
#endif // DEBUGB
					}
					break;
				}
			}
		}
	}
///////////////////////////////////////////////////////////////
#ifdef DEBUGX
	cout << "\n\t3RD CHECK! Controllo che il simplex link LT_Lightpath abbia la stessa free cap su tutti i simplex link LT_Channel su cui e' instradato" << endl;
#endif // DEBUG

	for (itrLp = this->getLightpathDB().m_hLightpathList.begin(); itrLp != this->getLightpathDB().m_hLightpathList.end(); itrLp++)
	{
		pLp = (Lightpath*)(*itrLp);
		for (itr2 = this->m_hGraph.m_hLinkList.begin(); itr2 != this->m_hGraph.m_hLinkList.end(); itr2++)
		{
			bool flag = false;
			pLink2 = (SimplexLink*)(*itr2);
			switch (pLink2->getSimplexLinkType())
			{
			case SimplexLink::LT_Channel:
			{
				if (pLink2->m_pLightpath == pLp)
				{
					if (pLink2->m_nChannel == pLp->wlAssigned)
					{
						if (pLink2->m_hFreeCap != pLp->m_nFreeCapacity)
						{
#ifdef DEBUGB
							cout << "\tATTENTION! Free cap of lightpath " << pLp->getId() << " is " << pLp->getFreeCapacity() << endl;
							cout << "\t\tFree cap of simplex link LT_Channel: " << pLink2->m_hFreeCap << endl << endl;
							
							cin.get();
#endif // DEBUGB
						}
						else
						{
							;
#ifdef DEBUGX
							//cout << "\tCheck sulla free capacity di lp e slink LT_Channel superato!" << endl << endl;
#endif // DEBUGB
						}
					}
				}
				break;
			}
			case SimplexLink::LT_Channel_Bypass:
			case SimplexLink::LT_Converter:
			case SimplexLink::LT_Grooming:
			case SimplexLink::LT_UniFiber:
			case SimplexLink::LT_Rx:
			case SimplexLink::LT_Tx:
			case SimplexLink::LT_Lightpath:
				break; //nothing to do
			}
		}
	}
#ifdef DEBUGX
	cout << "\nFINITO IL GRAN CHECK! Press Enter to continue" << endl;
	//cin.get();
#endif // DEBUG

}

void NetMan::buildHotelsListForNode()
{
	list<AbstractNode*>::const_iterator itr;
	vector<OXCNode*>::const_iterator itrOXC;
	OXCNode*pNode, *pOXC, *pBestNode;
	bool coreCOAlreadyFound;

	//-B: scan all network nodes
	for (itr = this->m_hWDMNet.m_hNodeList.begin(); itr != this->m_hWDMNet.m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		coreCOAlreadyFound = false;
		LINK_COST cost = UNREACHABLE, bestCost = UNREACHABLE;
		//-B: scan all hotel nodes (already sorted)
		for (itrOXC = this->m_hWDMNet.hotelsList.begin(); itrOXC != this->m_hWDMNet.hotelsList.end(); itrOXC++)
		{
			pOXC = (OXCNode*)(*itrOXC);
			Vertex*pVSrc = (Vertex*)this->m_hGraph.lookUpVertex(pNode->getId(), Vertex::VertexType::VT_Access_Out, -1);
			Vertex*pVDst = (Vertex*)this->m_hGraph.lookUpVertex(pOXC->getId(), Vertex::VertexType::VT_Access_In, -1);

			//-B: if this hotel is reachable in terms of latency (and capacity, even if it has no sense right now)
			pNode->pPath.clear();
			cost = this->m_hGraph.Dijkstra(pNode->pPath, pVSrc, pVDst, AbstractGraph::LinkCostFunction::LCF_ByOriginalLinkCost);
			
			if (cost < UNREACHABLE)
			{
				if (pNode->m_vReachableHotels.size() < 3)
				{
					//-B: add it to the reachable hotels' list
					pNode->m_vReachableHotels.push_back(pOXC);
				}
				else // >= 3
				{
					if (pOXC->getId() == this->m_hWDMNet.DummyNode)
					{
						//-B: add it to the reachable hotels' list
						pNode->m_vReachableHotels.push_back(pOXC);
					}
					if (cost < bestCost)
					{
						bestCost = cost;
						pBestNode = pOXC;
					}
				}
			}
		}
	}
}


void NetMan::reinsertBBU(OXCNode* nodeToInsert) {
	int found = false;

	for (int i = 0; i < this->m_hWDMNet.hotelsList.size(); i++) {
		if (nodeToInsert == this->m_hWDMNet.hotelsList[i]) {
			found = true;
		}

	}

	if (!found) {
		this->m_hWDMNet.hotelsList.push_back(nodeToInsert);
	}
}

void NetMan::printSecondBBUs() {

	for (int i = 0; i < 500; i++) {
		if (nodeActivatingNewBBU[i] > 0) {
#ifdef DEBUG
			cout << "Number of changes of BBU in node " << i << " is " << nodeActivatingNewBBU[i] << endl;
#endif		
		}
	}


}

//Verify that by adding all the connections (which are originated in pOXCSrc),
//on the path to the new candidate BBU, the latency constraint is not violated.
//If it is, I cannot use the new BBU, I will continue using the old one.
bool NetMan::verifyCapacity(OXCNode* pOXCSrc, UINT bestBBU, list <AbstractLink*> pathToCheck) {

#ifdef DEBUG
	cout << "\n\n-> verifyCapacity" << endl;
#endif

	list<Connection*>::const_iterator itr;
	Connection* pConDB;
	UINT bwd = 29; //already considering the corrent connection
	LINK_COST hPrimaryCost = UNREACHABLE;


	for (itr = this->m_hConnectionDB.m_hConList.begin(); itr != this->m_hConnectionDB.m_hConList.end(); itr++) {
		pConDB = (Connection*)(*itr);
		if (pConDB->m_nSrc == pOXCSrc->getId()) {
			if (pConDB->m_eConnType == Connection::MOBILE_FRONTHAUL || pConDB->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL) {
				bwd += (UINT)pConDB->m_eCPRIBandwidth;
			}
		}
	}

#ifdef DEBUG
	cout << "\tTotal bandwidth needed: " << bwd << endl;
	//cin.get();
#endif

	Circuit* pCircuit = new Circuit((BandwidthGranularity)bwd, Circuit::CT_Unprotected, NULL);

	Vertex *pSrc = m_hGraph.lookUpVertex(pOXCSrc->getId(), Vertex::VT_Access_Out, -1);
	Vertex *pDst = m_hGraph.lookUpVertex(bestBBU, Vertex::VT_Access_In, -1);


	Connection::ConnectionType connType = Connection::MOBILE_FRONTHAUL;

	list<AbstractLink*>::const_iterator itrLink = pathToCheck.begin();

#ifdef DEBUG
	cout << "\thPrimaryPath (size: " << pathToCheck.size() << "): " << endl;
#endif
	while (itrLink != pathToCheck.end())
	{
		SimplexLink *pLink = (SimplexLink*)(*itrLink);

		if (pLink->m_eSimplexLinkType == SimplexLink::LT_Lightpath) {
#ifdef DEBUGC
			cout << "\t\tch " << pLink->m_nChannel << "(type:" << pLink->getSimplexLinkType() << ") + "
				<< " free cap: " << pLink->m_hFreeCap << endl;;
#endif
			if (pLink->m_hFreeCap < bwd) {
#ifdef DEBUGC
				cout << "Free capacity on the link is " << pLink->m_hFreeCap <<
					" but we need a capacity of " << bwd << endl;
#endif
				return false;
			}

		}
		else if (pLink->m_eSimplexLinkType == SimplexLink::LT_UniFiber) {

			// here the condition is satisfied only if i need a bwd > channel capacity
			if (pLink->m_hFreeCap < bwd) {

#ifdef DEBUG
				cout << "Free capacity on the link is " << pLink->m_hFreeCap <<
					" but we need a capacity of " << bwd << endl;
#endif
				return false;
			}

		}
		else if (pLink->m_eSimplexLinkType == SimplexLink::LT_Channel) {

			// here the condition is satisfied only if i need a bwd > channel capacity
			if (pLink->m_hFreeCap < bwd) {

#ifdef DEBUG
				cout << "Free capacity on the link (channel) is " << pLink->m_hFreeCap <<
					" but we need a capacity of " << bwd << endl;
#endif
				return false;
			}

		}

		itrLink++;

	}
	return true;
}

Connection* NetMan::checkLinks(Connection* pConFromDB, list<AbstractLink*>& savedPath, list<AbstractLink*>& path) {
#ifdef DEBUGC
	cout << "\n-> checkLinks" << endl;
#endif
	list <AbstractLink*>::const_iterator itrLink;
	bool found = false;

	for (itrLink = savedPath.begin(); itrLink != savedPath.end(); itrLink++) {
		SimplexLink* pLink = (SimplexLink*)(*itrLink);
		if (pLink->m_eSimplexLinkType == SimplexLink::LT_Lightpath) {
			list <Lightpath*>::const_iterator itrRoute;
			if (pConFromDB->m_pPCircuit != NULL) {

				for (itrRoute = pConFromDB->m_pPCircuit->m_hRoute.begin(); itrRoute != pConFromDB->m_pPCircuit->m_hRoute.end(); itrRoute++) {

					if ((*itrRoute) == pLink->m_pLightpath) {
#ifdef DEBUGC
						cout << "\tLink from " << ((Vertex*)pLink->getSrc())->m_pOXCNode->getId() << " to "
							<< ((Vertex*)pLink->getDst())->m_pOXCNode->getId() << endl;
#endif

						list <AbstractLink*>::const_iterator itrLinkToDst;
						for (itrLinkToDst = path.begin(); itrLinkToDst != path.end(); itrLinkToDst++) {
							SimplexLink* pLinkToDst = (SimplexLink*)(*itrLinkToDst);
							if (((Vertex*)pLinkToDst->getSrc())->m_pOXCNode->getId() == ((Vertex*)pLink->getSrc())->m_pOXCNode->getId()
								&& ((Vertex*)pLinkToDst->getDst())->m_pOXCNode->getId() == ((Vertex*)pLink->getDst())->m_pOXCNode->getId()) {
#ifdef DEBUGC
								cout << "\t\tIt is also on the new preferred path: do not reset grooming" << endl;
#endif
								found = true;
								break;
							}
						}

						if (!found) {
#ifdef DEBUGC
							cout << "+ Reset grooming for connection from " << pConFromDB->m_nSrc
								<< " to " << pConFromDB->m_nDst << endl;
#endif
							return pConFromDB;

						}
					}

				}
			}
		}
	}
	return NULL;
}

bool NetMan::checkifConnectionpresent(UINT seqNumber) {
	list <Connection*>::const_iterator itrG;
	Connection* pConG;

	for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {
		pConG = (Connection*)(*itrG);
		if (pConG->m_nSequenceNo == seqNumber) {
			return true;
		}

	}
	return false;

}

list<AbstractLink*> NetMan::removeGroomingLink(list<AbstractLink*> path) {

	list<AbstractLink*>::const_iterator itrP;
	SimplexLink* pLink;
	list<AbstractLink*> linkToRemove;
	list<AbstractLink*> newPath;
	int countTx = 0;
	for (itrP = path.begin(); itrP != path.end(); itrP++) {

		pLink = (SimplexLink*)(*itrP);
		if (pLink->getSimplexLinkType() == SimplexLink::LT_Channel) {
			list<AbstractLink*>::const_iterator itrP2;

			cout << "Link from " << ((Vertex*)(pLink->getSrc()))->m_pOXCNode->getId()
				<< " to " << ((Vertex*)(pLink->getDst()))->m_pOXCNode->getId()
				<< " of type " << pLink->getSimplexLinkType() << endl;

			if (next(itrP, 1 ) != path.end()) {
				itrP2 = next(itrP, 1);
				SimplexLink* pLink2;
				pLink2 = (SimplexLink*)(*itrP2);
				cout << "Link is from " << ((Vertex*)(pLink2->getSrc()))->m_pOXCNode->getId()
					<< " to " << ((Vertex*)(pLink2->getDst()))->m_pOXCNode->getId()
					<< " of type " << pLink2->getSimplexLinkType() << endl;
				//cin.get();

				if (pLink2->getSimplexLinkType() == SimplexLink::LT_Grooming || pLink2->getSimplexLinkType() == SimplexLink::LT_Converter) {

					cout << "Link is from " << ((Vertex*)(pLink2->getSrc()))->m_pOXCNode->getId()
						<< " to " << ((Vertex*)(pLink2->getDst()))->m_pOXCNode->getId()
						<< " of type " << pLink2->getSimplexLinkType() << endl;
					cin.get();

					list<AbstractLink*>::const_iterator itrP3;
					itrP3 = next(itrP2, 1);

					if (((SimplexLink*)(*itrP3))->getSimplexLinkType() == SimplexLink::LT_Channel) {

						//remove grooming link
						cout << "Link to remove is from " << ((Vertex*)(pLink2->getSrc()))->m_pOXCNode->getId()
							<< " to " << ((Vertex*)(pLink2->getDst()))->m_pOXCNode->getId()
							<< " of type " << pLink2->getSimplexLinkType() << endl;
						cin.get();

						linkToRemove.push_back((*itrP2));

					}

				}
			}
		}

	}

	for (itrP = path.begin(); itrP != path.end(); itrP++) {
		list<AbstractLink*>::const_iterator itrP2;
		bool linkToAdd = true;

		for (itrP2 = linkToRemove.begin(); itrP2 != linkToRemove.end(); itrP2++) {
		
			if ((*itrP) == (*itrP2)) {		
				linkToAdd = false;
				break;
			}		
		}
		if (linkToAdd) {
		
			newPath.push_back((*itrP));

		}
	}

	return newPath;
}

void NetMan::printPath(list<AbstractLink*> pathComputedByYen) {


	list<AbstractLink*>::const_iterator itrPathYen;
	SimplexLink* pLinkYen;
	for (itrPathYen = pathComputedByYen.begin(); itrPathYen != pathComputedByYen.end(); itrPathYen++) {

		pLinkYen = (SimplexLink*)(*itrPathYen);

		switch (pLinkYen->m_eSimplexLinkType)
		{
		case SimplexLink::LT_Channel:
		{
#ifdef DEBUG
			cout << "\t+ Fiber from " << pLinkYen->m_pUniFiber->getSrc()->getId() << " to " << pLinkYen->m_pUniFiber->getDst()->getId() <<
				" on channel " << pLinkYen->m_nChannel << endl;
#endif // DEBUG
			break;
		}
		case SimplexLink::LT_Lightpath:
		{

#ifdef DEBUG
			cout << "\t Lightpath from " << pLinkYen->m_pLightpath->getSrc()->getId() << " -> " << pLinkYen->m_pLightpath->getDst()->getId()
				<< " on wavelength " << pLinkYen->m_pLightpath->wlAssigned << endl;

#endif // DEBUGB
			break;
		}
		}
	}


}

void NetMan::resetGrooming() {

	groomingConnections.clear();

}

bool NetMan::isLatencyExceeded() {
	list<Connection*>::const_iterator itrG;
	Connection* pConG;
	for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {

		pConG = (Connection*)(*itrG);

		if (pConG->m_eConnType == Connection::MOBILE_FRONTHAUL || pConG->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
		{

			if ((pConG->m_dRoutingTime + ELSWITCHLATENCY) > LATENCYBUDGET) {
#ifdef DEBUG
				cout << "Connection on grooming path from source " << pConG->m_nSrc <<
					" exceeds the latency budget" << endl;
				cout << "\tRouting + grooming time = " << pConG->m_dRoutingTime + ELSWITCHLATENCY << endl;
#endif
				return true;
			}


		}
	}
	return false;
}
/*
void NetMan::checkPath(UINT src, UINT id, LINK_COST pathCost) {

	OXCNode* pOXCdst = (OXCNode*)this->m_hWDMNet.lookUpNodeById(id);
	Vertex *pSrc = m_hGraph.lookUpVertex(src, Vertex::VT_Access_Out, -1);	//-B: ATTENTION!!! VT_Access_Out

	if (pathCost < UNREACHABLE)
	{
		Connection::ConnectionType connType = Connection::MOBILE_FRONTHAUL;

		float lat = computeLatencyP3(pOXCdst->pPath, pSrc, (UINT)(connType));
		if (lat <= LATENCYBUDGET)  //-B: -> should be unnecessary using DijkstraLatency -> IT IS ABSOLUTELY NECESSARY! (read at the end of DijkstraHelperLatency)
		{

			bool conditionSatisfied = true;

			OXCNode* pOXCsrc = (OXCNode*)this->m_hWDMNet.lookUpNodeById(src);
			if ((pOXCsrc->m_nBBUNodeIdsAssigned > 0) && pOXCdst->getId() != pOXCsrc->m_nBBUNodeIdsAssigned && (pOXCdst->m_dCostMetric < bestCost)) {

				bool capacityIsEnough = verifyCapacity(pOXCsrc, pOXCdst->getId(), pOXCdst->pPath);
				if (capacityIsEnough) {
					conditionSatisfied = true;
#ifdef DEBUGC
					cout << "\tCapacity is enough for choosing BBU " << pOXCdst->getId() << endl;
#endif
				}
				else {
					conditionSatisfied = false;
#ifdef DEBUGC
					cout << "\tCapacity is NOT enough for BBU " << pOXCdst->getId() << endl;
#endif
				}

			}
			else if (pOXCdst->m_dCostMetric > bestCost) {
				conditionSatisfied = false;
#ifdef DEBUGC
				cout << "\tDo not check capacity for BBU " << pOXCdst->getId() <<
					"; the cost is higher than the best one" << endl;
				BBUToCheck = false;
#endif
			}

			if (!conditionSatisfied) {
				groomingConnections.clear();
			}
			else {

				list <Connection*>::const_iterator itrG;
				Connection* pConG;

				for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {

					pConG = (Connection*)(*itrG);

					if ((pConG->m_eConnType == Connection::MOBILE_FRONTHAUL) || (pConG->m_eConnType != Connection::FIXEDMOBILE_FRONTHAUL)) {

						if ((pConG->m_dRoutingTime + ELSWITCHLATENCY) > LATENCYBUDGET) {
							conditionSatisfied = false;
							callYen = true;
#ifdef DEBUGC
							cout << "\tGrooming: connection from source " << pConG->m_nSrc <<
								" exceeds the latency budget" << endl;
							cout << "\tRouting + grooming time = " << pConG->m_dRoutingTime + ELSWITCHLATENCY << endl;
#endif
							break;
						}

					}

				}
			}

			if (conditionSatisfied && (pOXCdst->m_dCostMetric < bestCost)) {

				bestCost = pOXCdst->m_dCostMetric;
#ifdef DEBUGC

				cout << "New best cost BBU (satisfying constraints) is " << pOXCdst->getId() << endl;
#endif
				pathAlreadyFound = true;

				//precomputedPath = pOXCdst->pPath; //overwritten at the end of findBestBBUHotel --> SO DON'T DO IT HERE!

				//Reset grooming time related to the old best bbu
				//otherwise, I will update the grooming time to connections not on the chosen path
				list <Connection*>::const_iterator itrG;
				Connection* pConG;
				list<Connection*> connectinGroomingReset;

				for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {

					pConG = (Connection*)(*itrG);
#ifdef DEBUGC
					cout << "Connection from " << pConG->m_nSrc << " to "
						<< pConG->m_nDst << " has grooming time > 0" << endl;
#endif						
					Connection* connFound = checkLinks(pConG, savedPath, pOXCdst->pPath);
					if (connFound != NULL) {

						connectinGroomingReset.push_back(connFound);

					}

				}

				list < Connection*>::const_iterator itrCheckLinks;
				Connection* connectionToDelete;

				for (itrCheckLinks = connectinGroomingReset.begin(); itrCheckLinks != connectinGroomingReset.end(); itrCheckLinks++) {
					connectionToDelete = (Connection*)(*itrCheckLinks);
					groomingConnections.erase(connectionToDelete->m_nSequenceNo);

				}


				savedPath = pOXCdst->pPath;

				bestBBU = pOXCdst->getId();

			}
			else {
				groomingConnections.clear();

			}

		}
		else
		{
#ifdef DEBUGC
			cout << "\tPath to hotel node " << id << " scartato per superamento di latenza max -> " << lat << endl;
			cin.get();
#endif // DEBUGB
			groomingConnections.clear();
			callYen = true;
			m_bHotelNotFoundBecauseOfLatency = true;
		}
	}

#ifdef DEBUGF

}*/



