#pragma warning(disable: 4786)
#include <assert.h>
#include <string.h>
#include <iostream>
#include <random>

#include<fstream>
#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "AbstractPath.h"
#include "Connection.h"
#include "Event.h"
#include "EventList.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "OXCNode.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "ConnectionDB.h"
#include "LightpathDB.h"
#include "Log.h"
#include "NetMan.h"
#include "Circuit.h"
#include "Simulator.h"
#include "Lightpath.h"
#include "Lightpath_Seg.h"
#include "OchMemDebug.h"
#include "ConstDef.h"

//#define arr_dep_   //---t

//#define DEBUGMASSIMO
//#define TESTNET
//#define DEBUG_FABIO

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;
static int z=0;
static int v=0;
static int a1=0;
static int a2=0;
bool dbgactive=false;
Simulator::Simulator(): m_dArrivalRate(0), m_dMeanHoldingTime(0),
    m_pNetMan(NULL), m_ePClass(PC_DedicatedPath)
{
}


Simulator::~Simulator()
{
}

void Simulator::dump(ostream &out) const
{
	out<<"\n-> Simulator dump" << endl;

	out << "\tNum running conn = " << m_nRunConnections
		<< " | Transitory conn = " << m_nTransitoryCon
		<< " | Total conn = " << m_nTotalConnections;
	out << " | ArrRate: " << m_dArrivalRate
		<< " | ArrRate var: " << m_dArrivalRate_var
		<< " | Mean hold time: " << m_dMeanHoldingTime;
	out << " | Num OXC nodes: " << m_nNumberOfOXCNodes
		<< " | Num of node pairs: " << m_nNumberOfNodePairs;
	out << endl;
}

bool Simulator::initialize(UINT nCon, double dArrival,
                           double dHoldingTime, 
                           const char* pBWDistribution,
                           const char* pProvisionType,
                           int nHopCount)
{
	cout << "\n-> hSimulator initialize";
	//cin.get();

    assert(pBWDistribution);
    m_nTotalConnections = nCon; // stopping condition
	m_nRunConnections = N_RUN_CONNECTIONS;		// Add by ANDREA
    m_nTransitoryCon = N_TRANS_CONNECTIONS;		// Add by ANDREA-FABIO
    m_dArrivalRate = dArrival;
	cout << "Arrival rate è: " << m_dArrivalRate << endl;
	m_dArrivalRate_var = dArrival;

   // m_dMeanHoldingTime = dHoldingTime; //..in realtà questo è il mu ????
	 m_dMeanHoldingTime = 1.0/dHoldingTime; //..cosi in ingresso dovrebbe accettare l'holding time
	 cout << "Holding time è: "<< m_dMeanHoldingTime << endl;

    assert((nCon > 0) && (dArrival > 0) && (dHoldingTime > 0));

    // Parse bandwidth distribution
    if (0 == strcmp(pBWDistribution, "Zipf"))
        m_eBWDistribution = BD_Zipf;
    else if (0 == strcmp(pBWDistribution, "SBC"))
        m_eBWDistribution = BD_SBC;
    else if (0 == strcmp(pBWDistribution, "Sprint"))
        m_eBWDistribution = BD_Sprint;
    else if (0 == strcmp(pBWDistribution, "DBG_Uniform"))
        m_eBWDistribution = BD_Uniform;
    else if (0 == strcmp(pBWDistribution, "DBG_Uniform48"))
        m_eBWDistribution = BD_Debug48;
    else if (0 == strcmp(pBWDistribution, "DBG_Uniform192"))
        m_eBWDistribution = BD_Debug192;
    else {
        TRACE1("- ERROR: invalid bandwidth distribution: %s\n", pBWDistribution);
        return false;
    }

    if (NULL != strstr(pProvisionType, "DPP"))
        m_ePClass = PC_DedicatedPath;
    else if (NULL != strstr(pProvisionType, "SPP"))
        m_ePClass = PC_SharedPath;
    else if (NULL != strstr(pProvisionType, "SEG"))
        m_ePClass = PC_SharedSegment;
    else {
      //  assert(false);  // to do
        m_ePClass = PC_NoProtection;
    }
	if(m_pNetMan->m_eProvisionType==NetMan::PT_wpSEG_SPP || m_pNetMan->m_eProvisionType==NetMan::PT_wpUNPROTECTED ||
		m_pNetMan->m_eProvisionType== NetMan::PT_wpPAL_DPP|| m_pNetMan->m_eProvisionType== NetMan::PT_SPPBw || 
		m_pNetMan->m_eProvisionType== NetMan::PT_SPPCh)
		m_hEventList.wpFlag = true;
	else
		m_hEventList.wpFlag = false;
		
	if (m_pNetMan->m_eProvisionType == NetMan::PT_BBU)
		m_hEventList.wpFlag = WPFLAG;

    m_nHopCount = (UINT)nHopCount;
    // First arrival for each node
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pOXCNode;
	for (itr = this->m_pNetMan->m_hWDMNet.m_hNodeList.begin(); itr != this->m_pNetMan->m_hWDMNet.m_hNodeList.end(); itr++)
	{
		pOXCNode = (OXCNode*)(*itr);
		this->genNextPoissonEvent(pOXCNode, 0);
	}

	m_pNetMan->evList = &m_hEventList; //Fabio 20 sett

	//-B: ------------------- fronthaul events' list initialization -----------------------
	//m_hFrontEventList.insertEvent(new Event(0, Event::EVT_ARRIVAL, NULL));
									   
#ifdef DEBUGB
	char*provType;
	char*protClass;
	if (m_pNetMan->m_eProvisionType == 21)
		provType = "PT_BBU";
	if (m_ePClass == 0)
		protClass = "PC_NoProtection";

	cout << "\n\tAppena creato i primi eventi di Poisson (1 per ogni nodo) con genNextPoissonEvent: ARRIVAL" << endl;
	//cout << "\n\tNetMan provisioning type = " << provType << "\tpBandw distribution = " << pBWDistribution
		//<< "\tProtection class = " << protClass << "\twpFlag = " << m_hEventList.wpFlag << endl;
#endif // DEBUGB

	/*
		cout << "\nPROVA TRAFFICO DI BERNOULLI." << endl;
	int S = 10;
	double arrRate;
	double mean;
	for (int i = 0; i <= S; i++)
	{
		mean = 0;
		arrRate = 0.5 * (S - i);
		cout << "i = " << i;
		//cout << " --> TEMPO DI INTERARRIVO PER PROSSIMO EVENTO: ";
		int a;
		for (a = 0; a < 1000; a++)
		{
			if (arrRate != 0)
			{
				double nextArr = this->m_hEventList.nextBernoulliArrival(arrRate);
				mean += nextArr;
				//cout << nextArr << " ";
			}
			else
			{
				//cout << "UNREACHABLE ";
			}
		}
		cout << " --> mean = " << mean/a << endl;
	}
	cin.get();
	*/

	//-B
	this->dump(cout);
    
	return true;
}

void Simulator::run(SimulationTime delay)
{
    assert(m_pNetMan);

    bool statistic = true;		// Add by ANDREA
    bool stat_achieved = false;	// Add by ANDREA
    UINT numcon = 0;			// Add by ANDREA
    SimulationTime span = 0;		// Add by ANDREA
	int y = 0;
	SimulationTime hPrevLogTime = 0;    // assume starting from time 0
    bool bDone = false;
    Event *pEvent;
	bool transitory = false;
	//double diff=1000;
	//bool first=true;
    
    //mettere a 0 m_used,
	//UniFiber *pUniFiber;
	//list<AbstractLink*>::const_iterator itr;
	//for (itr=m_pNetMan->m_hWDMNet.m_hLinkList.begin(); itr!=m_pNetMan->m_hWDMNet.m_hLinkList.end(); itr++) {
	//	pUniFiber = (UniFiber*)(*itr);
	//	(*itr)->m_used=0;
	//	cout<<"init"<<(*itr)->getId()<<"  "<<(*itr)->m_used;	
	//}
	
	/* //-B: NOT NEEDED FOR MY WORK
	// inizializzazione a 0 dei vettori che contano le connessioni nei DC
	int kk=0;
	for (kk = 0; kk != m_pNetMan->m_hWDMNet.nDC; kk++)
	{
		m_pNetMan->m_hWDMNet.DCCount[kk]=0; 
		m_pNetMan->m_hWDMNet.nconn[kk]=0;
	}

	double Total_Transport_Cost=0;
    double Total_EDFA_Cost=0;
	double Total_NodeProcessing_Cost=0;

	double *gp = new double[m_pNetMan->m_hWDMNet.nDC];
	double *green_energy_DC = new double[m_pNetMan->m_hWDMNet.nDC];
	double *brown_energy_DC = new double[m_pNetMan->m_hWDMNet.nDC];
	double *gp_rest = new double[m_pNetMan->m_hWDMNet.nDC];
	double *gp_incr = new double[m_pNetMan->m_hWDMNet.nDC];
	double *diff_conn = new double[m_pNetMan->m_hWDMNet.nDC];
	double *g_en = new double[m_pNetMan->m_hWDMNet.nDC];
	double *DCprocessing = new double[m_pNetMan->m_hWDMNet.nDC];
	double *diff_DCprocessing = new double[m_pNetMan->m_hWDMNet.nDC];
	double *DCprocessing_old = new double[m_pNetMan->m_hWDMNet.nDC];
	double *g_en_residual = new double[m_pNetMan->m_hWDMNet.nDC];
	double *gptot = new double[m_pNetMan->m_hWDMNet.nDC];
	double *nconn_old = new double[m_pNetMan->m_hWDMNet.nDC];

	double green_energy_temp[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double brown_energy_temp[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double transport_temp[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double CO2emissions_comp[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };			//emissioni CO2 per computing
	double CO2emissions_transport[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };		//emissioni CO2 per trasporto
	double CO2emissions_tot[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };			//emissioni CO2 totali
	double EDFA_temp[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double processing_temp[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double provi[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	//std::ofstream fout("dump.txt",ios_base::app);
	for (int k = 0; k != m_pNetMan->m_hWDMNet.nDC; k++) {
	gp_rest[k]=0;
	gp_incr[k]=0;
	brown_energy_DC[k]=0;
	green_energy_DC[k]=0;
	diff_conn[k]=0;
	nconn_old[k]=0;
	m_pNetMan->m_hWDMNet.DCprocessing[k]=0;
	DCprocessing_old[k]=0;
	diff_DCprocessing[k]=0;
	}
	*/
	
	//-B:
	double count = -1;
	UINT numSimplexLinks = m_pNetMan->countLinksInGraph();
	
	int m = 1;                                                 // contatore per output ogni ora dei dati
	int r = 1;													//-B: same as above, but for 
	int start_time = 0;                                        //ora di inizio della simulazione, es: 7-->7:00
														   
															   //----------------------------------------------------------------------------------------------------------------------
	//-B: ::::::::::::::::::::::::::::::::::::::::: PRE-PROCESSING PHASE ::::::::::::::::::::::::::::::::::::::::::
	//-B: build sorted list of BBU candidate hotel nodes
	// ATTENTION FOR THE DIFFERENT BBU PLACEMENT POLICIES
	m_pNetMan->buildBestHotelsList();


	if (BBUPOLICY == 2)
	{
		//-B: set the adequate number of active hotels in this->m_nNumOfActiveHotels
		this->computeNumOfActiveHotels();
		
		//-B: HERE ANOTHER PASSAGE SHOULD BE ADDED: CHECK IF ALL NETWORK NODES REACH AT LEAST ONE HOTEL AMONG THE FIRST this->m_nNumOfActiveHotels HOTEL NODES
		//	IF NOT, CHANGE 1 OR MORE HOTELS SELECTED, OR ADD A NEW ONE, IN ORDER TO HAVE A FULL COVERAGE OF THE NETWORK NODES
		
		m_pNetMan->reduceHotelsForConsolidation(this->m_nNumOfActiveHotels);
	}

	//reset some vars used during build hotels list
	m_pNetMan->m_hWDMNet.resetPreProcessing();
	if (ONLY_ACTIVE_GROOMING)
	{
		//-B: invalidate LT_Converter and LT_Grooming links for candidate hotel nodes
		m_pNetMan->invalidateGrooming();
	}
	//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	//----------------------------------------------------------------------------------------------------------------------

#ifdef DEBUGB
	//UINT numGroomNodes = m_pNetMan->countGroomingNodes();
	//cout << "Ci sono " << numGroomNodes << " grooming nodes" << endl;
#endif // DEBUGB

	//-B: pointer to NetMan object
	m_hEventList.m_pNetMan = m_pNetMan;

	cout << endl << "INIZIO DELLA SIMULAZIONE" << endl;
	//cin.get();
	cout << "..." << endl;
 
	//-----------------------------------------start While--------------------------------------------------------------------
while ((pEvent = m_hEventList.nextEvent()) && (!bDone) && (!stat_achieved))
{//-B: si chiude a riga 778
#ifdef DEBUGB
	//cin.get();
#endif // DEBUGB

	assert(pEvent);
	
	//-B: if pEvent is a backhaul event, I have to delete its corresponding fronthaul event in the related events' list
	if (pEvent->fronthaulEvent && pEvent->midhaulEvent) // -L: if true -> it's a backhaul event
	{
		m_hFrontEventList.updateFronthaulEventList(pEvent);
	}

#ifdef DEBUGC
	cout << "*********************************************************************\n"
		<< "INIZIO WHILE:\t" << m_pNetMan->m_hLog.m_nProvisionedCon << " prov ("
		<< m_pNetMan->m_hLog.m_nProvisionedMobileFrontConn << " M-F + "
		<< m_pNetMan->m_hLog.m_nProvisionedFixMobFrontConn << " FM-F + "
		<< m_pNetMan->m_hLog.m_nProvisionedFixedBackConn << " F-B + "
		<< m_pNetMan->m_hLog.m_nProvisionedMobileBackConn << " M-B + "
		<< m_pNetMan->m_hLog.m_nProvisionedFixMobBackConn << " FM-B"
		<< ") connections\n\t\t" << m_pNetMan->m_hLog.m_nBlockedCon << " bloc ("
		<< m_pNetMan->m_hLog.m_nBlockedMobileFrontConn << " M-F + "
		<< m_pNetMan->m_hLog.m_nBlockedFixMobFrontConn << " FM-F + "
		<< m_pNetMan->m_hLog.m_nBlockedFixedBackConn << " F-B + "
		<< m_pNetMan->m_hLog.m_nBlockedMobileBackConn << " M-B + "
		<< m_pNetMan->m_hLog.m_nBlockedFixMobBackConn << " FM-B) ("
		<< m_pNetMan->m_hLog.m_nBlockedBWDueToUnreach << " unreach + "
		<< m_pNetMan->m_hLog.m_nBlockedConDueToLatency << " lat"
		<< ") connections\n\t\t" << (m_hEventList.getNumArr() + m_hEventList.getNumDep())
		<< " (" << m_hEventList.getNumArr() << " arr + " << m_hEventList.getNumDep()
		<< " dep) " << " events" << endl
		<< "*********************************************************************" << endl;
#endif // DEBUGB

#ifdef TESTNET
//this->m_pNetMan->m_hWDMNet.dump(cout);
//cout<<endl<<"Segue rete passata"<<endl<<endl;
this->m_pNetMan->m_hWDMNetPast.dump(cout);
#endif

  if (pEvent->m_hEvent == Event::EVT_ARRIVAL)
  {
	// Add by ANDREA
	// Statistical Control: activate/disactivate it at the very beginning of the .run method
	if (statistic)
	{
		//-B: if transitory phase has been passed
		//if (numcon >= m_nTransitoryCon) -> since mobile connections consist of fronthaul + backhaul connection
		if (m_hEventList.getNumArr() >= m_nTransitoryCon) //-B: sostituzione numcon -> getNumArr()
		{
			// at the end of transitory reset Log
			//if ((numcon == m_nTransitoryCon) && (pEvent->m_hEvent == Event::EVT_ARRIVAL))
			if (/*pEvent->m_hTime >= 10 &&*/ !transitory && (pEvent->m_hEvent == Event::EVT_ARRIVAL) && (!pEvent->fronthaulEvent)) //-B: sostituzione
			{
#ifdef DEBUGB
				m_pNetMan->m_runLog.m_hSimTimeSpan = pEvent->m_hTime - span;
				m_pNetMan->m_hLog.m_hSimTimeSpan = pEvent->m_hTime;

				cout << "\nTIME: " << pEvent->m_hTime << " - running time span: " << m_pNetMan->m_runLog.m_hSimTimeSpan
					<< " - #CONNECTIONS: " << numcon << " - Premi invio per aggiornare le stats" << endl;
				//cin.get();

				/////////////////////////// PERIODICAL STATS
				UINT networkCost = m_pNetMan->computeNetworkCost();
				cout << "- NETWORK COST (medio): " << m_pNetMan->m_hLog.networkCost/ (m_pNetMan->m_hLog.m_hSimTimeSpan - m_pNetMan->m_hLog.transitoryTime) << endl;
				cout << "\tPeak: " << m_pNetMan->m_hLog.peakNetCost << " (active nodes peak = " << m_pNetMan->m_hLog.peakNumActiveNodes
					<< "; lightpaths peak = " << m_pNetMan->m_hLog.peakNumLightpaths << ")" << endl;

				//-B: PRINT RESULTS
				m_pNetMan->m_hLog.output(); //-B: dump hLog is equal to dump runLog at this point
				showLog(cout);
				m_pNetMan->printStat(cout); 
				//cin.get();
#endif // DEBUGB
				//m_pNetMan->m_hLog.output();
				cout << "------------------------------- TRANSITORY PHASE PASSED: RESETTO LOG e RUN_LOG -------------------------------" << endl;
				//-B: RESET run_Log after transitory phase: NON DOVREI RESETTARE ANCHE L'hLog?????????????????? COSI' COME LE ALTRE VAR USATE NEL PERIODICAL LOG
				//-B: RESET Log
				m_pNetMan->m_hLog.resetLog(); //-B: added by me
				//-B: RESET runLog
				m_pNetMan->m_runLog.resetLog();
				//-B: RESET Sstat OBJECT p_block (added by me)
				//m_pNetMan->p_block->reset(); //SHOULD NOT BE NEEDED
				m_pNetMan->m_hWDMNet.resetHotelStats();
				
				//LOG SOME STATS
				span = pEvent->m_hTime;
				m_pNetMan->m_hLog.transitoryTime = span;
				count = 0;
				transitory = true;
#ifdef DEBUGB
				//cin.get();
#endif // DEBUGB
			}
			//°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° RUNNING CONNECTIONS °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
			//if ((numcon % m_nRunConnections == 0) && (numcon != m_nTransitoryCon) && (pEvent->m_hEvent == Event::EVT_ARRIVAL))
			if ((m_pNetMan->m_runLog.m_nProvisionedCon + m_pNetMan->m_runLog.m_nBlockedCon) == m_nRunConnections && transitory //<-- sostituzione <-- && (m_hEventList.getNumArr() != m_nTransitoryCon)
				&& (pEvent->m_hEvent == Event::EVT_ARRIVAL) && (!pEvent->fronthaulEvent))
				//-B: && (!pEvent->fronthaulEvent)) would work with only mobile connections -> if !pEvent->fronthaulEvent it means it is a fronthaul event (here we haven't provided yet the connection to the mobile request)
				//-B: && (m_pNetMan->m_runLog.m_nProvisionedCon + m_pNetMan->m_runLog.m_nBlockedCon) == m_nRunConnections -> this condition doesn't work correctly!!!
				//(m_hEventList.getNumArr() % m_nRunConnections == 0)
			{
				m_pNetMan->m_runLog.m_hSimTimeSpan = pEvent->m_hTime - span; //-B: (current time) - (time of previous periodical update)
				m_pNetMan->m_hLog.m_hSimTimeSpan = pEvent->m_hTime;
#ifdef DEBUGC
				cout << " WE ARE HERE " << endl; 
				cout << "_________________________________________________________________________________________________________________" << endl;
				cout << "\nTIME: " << pEvent->m_hTime << " - simulation time span: " << m_pNetMan->m_hLog.m_hSimTimeSpan
					<< " - running time span: " << m_pNetMan->m_runLog.m_hSimTimeSpan
					<< " - #CONNECTIONS: " << m_hEventList.getNumArr() << " - Premi invio per aggiornare le stats" << endl; //-B: m_hEventList.getNumArr() sostituisce numcon
																															//-B: dovrebbe essere corretto			
				cout << "- AVERAGE NETWORK COST: " << m_pNetMan->m_hLog.networkCost/(m_pNetMan->m_hLog.m_hSimTimeSpan - m_pNetMan->m_hLog.transitoryTime) << endl;
				cout << "\tPeak: " << m_pNetMan->m_hLog.peakNetCost 
					<< " (active nodes peak = " << m_pNetMan->m_hLog.peakNumActiveNodes
					<< "; lightpaths peak = " << m_pNetMan->m_hLog.peakNumLightpaths
					<< "; active BBUs peak = " << m_pNetMan->m_hLog.peakNumActiveBBUs << ")" << endl;
#endif
				//-B: UPDATE STATS
				//stat_achieved = m_pNetMan->updateStat(numcon, m_nRunConnections); //-B: here it is possible to activate isConfidenceSatisfied
																				  //(conf = 0.95, stop_perc = 5.0 -> set by NetMan constructor)
				//-B: update blocking prob confidence
				stat_achieved = m_pNetMan->updateStat(m_hEventList.getNumArr(), m_nRunConnections);
				//-B: if RUNNING STATS are on
				if (RUNNING_STATS)
				{
					saveStats(m, m_pNetMan->m_hLog);
					saveStats(r, m_pNetMan->m_runLog);
				}

				//-B: PRINT RESULTS
				//m_pNetMan->m_hLog.output();
				cout << "\tRUNLOG OUTPUT" << endl;
				m_pNetMan->m_runLog.output();
				showLog(cout);
				m_pNetMan->printStat(cout);

				//-B: RESET
				m_pNetMan->m_runLog.resetLog();
				span = pEvent->m_hTime; //-B: save time of the last periodical update
#ifdef DEBUGB
				cout << "Premi invio per continuare";
				//cin.get();
#endif // DEBUGB
				cout << "..." << endl;
			}

		}//if #connections >= #transitory connections
		/*
		else //-B: still in transitory phase
		{

#ifdef DEBUGB
			cout << "\nFase transitoria " << endl;
#endif // DEBUGB

			m_pNetMan->pc = 0;
			m_pNetMan->bc = 0;
			m_pNetMan->fc = 0;
			m_pNetMan->bothc = 0;
		}*/

	} //if statistic
  } //if arrival
	
	//////////////////////////---------- PERIODICAL LOG ----------///////////////////////////
  if (count >= 0)
  {
	  m_pNetMan->logPeriodical(pEvent->m_hTime - hPrevLogTime);
  }
  count++;

	hPrevLogTime = pEvent->m_hTime;

	//if ((m_pNetMan->m_runLog.m_nProvisionedCon + m_pNetMan->m_runLog.m_nBlockedCon) == 1000 
		//&& (pEvent->m_hEvent == Event::EVT_ARRIVAL) && (!pEvent->fronthaulEvent))
	if (m_hEventList.getNumArr() % 1000 == 0 && pEvent->m_hEvent == Event::EVT_ARRIVAL && (!pEvent->fronthaulEvent))
	{
#ifdef DEBUG
		cout << "#connessioni: " << m_hEventList.getNumArr() << " - time: " << pEvent->m_hTime << endl;
		cout << "Lightpath list DB size: " << m_pNetMan->getLightpathDB().m_hLightpathList.size();
		cout << " - Connection list DB size: " << m_pNetMan->getConnectionDB().m_hConList.size() << endl;
		cout << "Graph link list size: " << m_pNetMan->countLinksInGraph() << " - Event list size: " << m_hEventList.getSize() << endl;
		cout << "Provisioned connections: " << m_pNetMan->m_hLog.m_nProvisionedCon
			<< " - blocked connections: " << m_pNetMan->m_hLog.m_nBlockedCon << endl;
		cout << "..." << endl;
#endif
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	
	switch (pEvent->m_hEvent)
	{
	case Event::EVT_ARRIVAL:
	{
		//---------------------------------------------------------------------------------------------
		//-B: +++++++++++++++***************** CREATE THE CONNECTION *******************+++++++++++++++
		//---------------------------------------------------------------------------------------------
		Connection *pCon;
		if (m_pNetMan->m_eProvisionType == NetMan::ProvisionType::PT_BBU)
		{
			if (pEvent->m_pConnection == NULL) { 
				pCon = BBU_newConnection_Bernoulli(pEvent, m);
				pEvent->m_pConnection = pCon;
				pCon->m_bTrafficToBeUpdatedForArrival = true;
				pCon->m_bTrafficToBeUpdatedForDep = true;


				if (pCon->m_eConnType == Connection::MOBILE_BACKHAUL || pCon->m_eConnType == Connection::FIXEDMOBILE_BACKHAUL) {
					backhaul_id[pEvent->fronthaulEvent->m_pConnection->m_nSequenceNo] = pCon->m_nSequenceNo;
				}
			}
			// it enters here only if the event refers to a connection changing BBUB
			else if (pCon->m_eConnType == Connection::MOBILE_FRONTHAUL) 
			{
				pCon = pEvent->m_pConnection;
				pCon->m_bTrafficToBeUpdatedForDep = true;

				//-L: saved id for the backhaul and midhaul
				pCon->m_nMidhaulSaved = (pCon->m_nSequenceNo) + 1;
				pCon->m_nBackhaulSaved = (pCon->m_nSequenceNo) + 2;

#ifdef DEBUGC
				cout << "Evento contiene connessione; è di tipo " << pCon->m_eConnType <<
					" dalla sorgente " << pCon->m_nSrc << " con seq number " << pCon->m_nSequenceNo << endl;

				cout << "Id for the backhaul will be " << pCon->m_nBackhaulSaved << endl;
#endif


#ifdef DEBUGC

				MappedLinkList<UINT, Connection*> conList = this->m_pNetMan->getConnectionDB().m_hConList;
				list<Connection*>::const_iterator itr;
				Connection* pConDB;
				UINT pDst = pEvent->m_pConnection->m_nDst;
				int countConnectionsWithMyNumber = 0;
				for (itr = conList.begin(); itr != conList.end(); itr++) {
					pConDB = (Connection*)(*itr);
					if (pConDB->m_nSequenceNo == pCon->m_nSequenceNo) {
						cout << "OPS! I found another connection with my same seq number (" <<
							pCon->m_nSequenceNo << ") which has as BBU " << pConDB->m_nDst << endl;
						countConnectionsWithMyNumber++;
						cin.get();
					}
				}
				assert(countConnectionsWithMyNumber == 0);
#endif

			}

			//-B: in case of (!ONE_CONN_PER_NODE) + (!MIDHAUL) + (BBUSTACKING) --> we have to update an already existing departure event
			if (!ONE_CONN_PER_NODE && !MIDHAUL && BBUSTACKING)
			{
				if (pCon->m_eConnType == Connection::MOBILE_FRONTHAUL || pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
				{
					if (pCon->m_eCPRIBandwidth == OC0)
					{
						//-L: devo fare lo stesso per il midhaul 
						//-B: it means it is not the first connection of this source node
						// --> it doesn't have to route its fronthaul, it just has to delay the related departure event
						updateCorrespondingDepEvent(pCon);
					}
				}
			}
		}
		else
		{
			pCon = newConnection(pEvent->m_hTime);
		}

		//-B: update val of numCon after creating a new Connection
		numcon = pCon->m_nSequenceNo;

#ifdef DEBUGB
		//-B: CONNECTION DUMP
		pCon->dump(cout);
		//cin.get();
#endif // DEBUGB

//----------------------------------------------------------------------------------------------------------------------------------------
/////////////////////////////////////////// IMPOSTARE DURATA SIMULAZIONE IN BASE AL TEMPO O CONNESSIONI //////////////////////////////////		
//----------------------------------------------------------------------------------------------------------------------------------------
		if ((m_hEventList.getNumArr() - m_pNetMan->m_hLog.m_nNumConnectionsChangingBBU) >= m_nTotalConnections)
			//if (pCon->m_nSequenceNo >= m_nTotalConnections)   // se SequenceNumber == numero di connessioni totali     
			//if (pEvent->m_hTime >= SIM_DURATION)    //limite di 86400 secondi (24 ore)
		{
#ifdef DEBUG
			cout << "ULTIMO WHILE. SONO ALLA CONNESSIONE NUMERO: " << pCon->m_nSequenceNo << " = " << numcon << endl;
#endif // DEBUG
			//-B: prima del while viene impostato a false. Se entra in questo if, e quindi esegue questa istruzione, uscirà dal while -> EXIT CONDITION
			bDone = true;
			//-B: save simultation time without subtracting transitory time
			m_pNetMan->m_hLog.m_hSimTimeSpan = pEvent->m_hTime;
			m_pNetMan->m_runLog.m_hSimTimeSpan = pEvent->m_hTime - span;		// Statistical Log Add by Andrea  *
		} //chiusura if
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		int k = 0;
		int j = 0;
		int time = floor((pEvent->m_hTime) / 3600); // arrotondo il tempo di simulazione da secondi a ore
		time = (start_time + time) % 24;
		//cout<<"tempo "<<time;
		//cin.get();

		//---------- Andamento traffico variabile, profilo di traffico preso da INTERNET project
		//tassi arrivo[time] can be use to change arrival rate according to the hour of the day (for 
		// now everything is put to 1).
		m_dArrivalRate = m_dArrivalRate_var*((tassiarrivo[time]));
		//----------------------------------------------------------------------------------------

		/* //-B: NOT NEEDED FOR MY WORK
			//inizializzazione a 0
		 for (k = 0; k != m_pNetMan->m_hWDMNet.nDC; k++) {
			gptot[k]=0;
			gp[k]=0;
		}

		 for (k = 0; k != m_pNetMan->m_hWDMNet.nDC; k++)
		 {
			if (m_pNetMan->m_hWDMNet.DCvettALL[k] != 99)
			{
				for (j = start_time; j != (time + 1); j++)
				{
				   j=j%24;
				   gp_incr[k]=m_pNetMan->m_hWDMNet.EstimateGreenPower(m_pNetMan->m_hWDMNet.DCvettALL[k],m_pNetMan->m_hWDMNet.TZvett[k],j);  //gp[k] contiente l'energia Green di partenza nel DC k-esimo all'istante j, con il proprio TimeZone
				   gptot[k]=gptot[k]+gp_incr[k];				//gptot[k] è la somma di tutta l'energia Green del k-esimo DC
				}
				gp[k]=gp_incr[k]+gp_rest[k];
				m_pNetMan->m_hWDMNet.gp[k]=gp[k];
				DCprocessing[k]=m_pNetMan->m_hWDMNet.DCprocessing[k];
				g_en_residual[k]=gptot[k]-DCprocessing[k];  //somma di tutti gli incrementi fino a quest'ora + energia avanzata (se c'è) - potenza necessaria per le connessioni di quel nodo fino a quel momento
				m_pNetMan->m_hWDMNet.g_en_residual[k]=g_en_residual[k];
				//cout<<"\nresidual: "<<g_en_residual[k]<<" DC proc: "<<DCprocessing[k];
				//cin.get();
			}
		}
		//trova il nodo con la massima disponibilità di energia
		int i,MaxNodo;
		double max=-32000;
		for (i = 0; i < m_pNetMan->m_hWDMNet.nDC; i++) {
			if (g_en_residual[i]>max) {
				max=g_en_residual[i];
				MaxNodo=m_pNetMan->m_hWDMNet.DCvettALL[i];
				m_pNetMan->m_hWDMNet.BestGreenNode = MaxNodo;
		   }
		}
	//cout<<"\nMaggiore disponibilita' di energia (" <<max<<" Watt) nel nodo "<<MaxNodo<<"\n";
	//cin.get();

	//Quando max è minore di 0 si usa energia NON rinnovabile

				// compute route, and set up if successful

			  // Make a service copy of event_List
			  // to be decremented of departure events
		*/


		list<Event*> hDepEventList;
		m_hEventList.ExtractDepEvents(hDepEventList);//FABIO: DA RIFARE
														//QUANDO CONSIDERERO' I TEMPI
														//DI CHIAMATA
		//m_hEventList.dump2(cout);


//		pCon->m_nSrc=0;
//		pCon->m_nDst=4;
//if(pCon->m_nSequenceNo>500)

		//-B: non incremento il num di arrivi solo nel caso in cui l'evento corrente sia un mobile/fixed-mobile backhaul event
		if (pCon->m_eConnType == Connection::FIXED_BACKHAUL
			|| pCon->m_eConnType == Connection::MOBILE_FRONTHAUL
			|| pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
		{
			//-B: increase the number of arrivals (fronthaul and its corresponding backhaul are considered as a single connection)
			m_hEventList.increaseArr();
		}

		bool bSuccess = false;
		if (pCon->m_nSrc != pCon->m_nDst)
		{
			//*********************************************************************************************************
			//---------------------------------- PROVISION CONNECTION --------------------------------------------------
			//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			bSuccess = m_pNetMan->provisionConnectionxTime(pCon, hDepEventList, m_hEventList);
		}
		else
		{ //-B: it does not have to setup any circuit. PAY ATTENTION TO THAT!

			m_pNetMan->connectionsChangingBBU.clear();

#ifdef DEBUGC
			cout << "\n\tNODO SORGENTE = NODO DESTINAZIONE" << endl;
#endif // DEBUG

			if (pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL || pCon->m_eConnType == Connection::MOBILE_FRONTHAUL) 
			{
				m_pNetMan->m_hGraph.resetLinks();
				if (!isBBUNode(pCon->m_nDst)) {
#ifdef DEBUG
					cout << "LINKS HAVE BEEN RESET" << endl;
					//cin.get();
#endif
				}
			}

			pCon->m_eStatus = Connection::SETUP;
			bSuccess = true; 
			//-B: it increases m_hLog.m_nProvisionedCon++ and other counters
			pCon->log(m_pNetMan->m_hLog);
			//-B: it increases m_runLog.m_nProvisionedCon++ and other counters
			pCon->log(m_pNetMan->m_runLog);
			//-B: add the current connection to the connectionDB
			m_pNetMan->addConnection(pCon);
		}

		if (!bSuccess) //-B: CONNECTION BLOCKED
		{
#ifdef DEBUGF
			cout << "\n++++++++++++++++++++ BLOCKED!" << endl << endl;
			//cin.get();
#endif // DEBUGB
		
			OXCNode*pOXCNode;
			// -L: Da modificare per includere il midhaul, o faccio un altro if o unisco i due casi (meglio un altro if)
			if (pCon->m_eConnType == Connection::MOBILE_BACKHAUL
				|| pCon->m_eConnType == Connection::FIXEDMOBILE_BACKHAUL)
			{
				//-L: create and insert a departure event, for the midhaul connection already provided, to be executed immediately!
				// Event*nEv_DepFront = new Event(pEvent->m_hTime, Event::EVT_DEPARTURE, pEvent->fronthaulEvent->m_pConnection);
				Event*nEv_DepMid = new Event(pEvent->m_hTime, Event::EVT_DEPARTURE, pEvent->midhaulEvent->m_pConnection);
				// nEv_DepFront->arrTimeAs = hPrevLogTime;
				// nEv_DepFront->backhaulBlocked = true; 
				// m_hEventList.insertEvent(nEv_DepFront);
				nEv_DepMid->arrTimeAs = hPrevLogTime;
				nEv_DepMid->backhaulBlocked = true; 
				m_hEventList.insertEvent(nEv_DepMid);

				//-L: da controllare
				//-B: get OXCNode to set it as source in next Bernoulli arrival (after if ... else ...)
				pOXCNode = (OXCNode*)(this->m_pNetMan->m_hWDMNet.lookUpNodeById(pEvent->midhaulEvent->m_pConnection->m_nSrc));

#ifdef DEBUGC
				cout << "\tE' un backhaul event -> Visto che la connessione e' stata bloccata, "
					<< "al prossimo evento faro' il deprovisioning anche della fronthaul connection corrispondente:" << endl << "\t";
				pEvent->fronthaulEvent->m_pConnection->dump(cout);
#endif // DEBUGB
			}
			else if (pCon->m_eConnType == Connection::FIXED_MIDHAUL)
			{
				//-B: create and insert a departure event, for the fronthaul connection already provided, to be executed immediately!
				Event*nEv_DepFront = new Event(pEvent->m_hTime, Event::EVT_DEPARTURE, pEvent->fronthaulEvent->m_pConnection);
				nEv_DepFront->arrTimeAs = hPrevLogTime;
				nEv_DepFront->backhaulBlocked = true; //-L: cambiare con midhaulblocked (da dichiarare)
				m_hEventList.insertEvent(nEv_DepFront);

				//-B: get OXCNode to set it as source in next Bernoulli arrival (after if ... else ...)
				pOXCNode = (OXCNode*)(this->m_pNetMan->m_hWDMNet.lookUpNodeById(pEvent->fronthaulEvent->m_pConnection->m_nSrc));

#ifdef DEBUGC
				cout << "\tE' un midhaul event -> Visto che la connessione e' stata bloccata, "
					<< "al prossimo evento faro' il deprovisioning anche della fronthaul connection corrispondente:" << endl << "\t";
				pEvent->fronthaulEvent->m_pConnection->dump(cout);
#endif // DEBUGB
			}
			else if (pCon->m_eConnType == Connection::MOBILE_FRONTHAUL || pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
			{
				//-B: dst could be NULL if no admissible hotel node is found during BBU_newConnection in findBestBBUHotel
				if (pEvent->m_pConnection->m_nDst != NULL)
				{
					if (BBUSTACKING == true && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == false)
					{
						//remove connection's dst node from list of active BBUs, if needed
						m_pNetMan->m_hWDMNet.updateBBUsUseAfterBlock(pEvent->m_pConnection, m_pNetMan->getConnectionDB());
						//m_pNetMan->invalidateSimplexLinkGrooming(); //-B: it should be performed each time we provide connection for a request -> at each iteration
																	//	(instruction moved before computing the path (both for fronthaul (bbu) and for bakhaul)
					}
					else if (BBUSTACKING == false && INTRA_BBUPOOLING == true && INTER_BBUPOOLING == false)
					{
						;
					}
					else if (BBUSTACKING == false && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == true)
					{
						m_pNetMan->m_hWDMNet.updatePoolsUse(pEvent->m_pConnection, m_pNetMan->getConnectionDB());
					}
					else
					{
						assert(false);
					}
				}
				//don't create any midhaul event

				//-B: get OXCNode to set it as source in next Bernoulli arrival
				pOXCNode = (OXCNode*)(this->m_pNetMan->m_hWDMNet.lookUpNodeById(pCon->m_nSrc)); //-L: ????????
			
			}
			
			/* -L: ???
			if the if is verified, it means it is a connection towards the new BBU and so
			since it is blocked, I have to update the number of free sources at the source node.
			In all the other cases (else) i will generate a new arrival.	
			*/
			if (!pEvent->m_pConnection->m_bTrafficToBeUpdatedForArrival) {
				//-B: be sure that is a fronthaul event
				assert(pEvent->fronthaulEvent == NULL);
				//-B: original source node is the source of this fronthaul connection
				OXCNode* pSrc = (OXCNode*)(this->m_pNetMan->m_hWDMNet.lookUpNodeById(pEvent->m_pConnection->m_nSrc));
				//-B: increase traffic generated by this source node, taking into account only the backhaul bandwith (the "payload")
				pSrc->m_dTrafficGen -= pEvent->m_pConnection->m_eBandwidth;

				//-B: after changing the way of representing small cells, we have to modify PREVIOUS check
				if (pSrc->isMacroCell() && this->m_pNetMan->m_hWDMNet.isMobileNode(pSrc->getId()))
				{
					if (pSrc->m_dTrafficGen < 0 || pSrc->m_dTrafficGen >(MAXTRAFFIC_MACROCELL + (SMALLCELLS_PER_MC * MAXTRAFFIC_SMALLCELL)))
						cout << "\tATTENTION! Traffico generato dalla MACRO cell #" << pSrc->getId() << " = " << pSrc->m_dTrafficGen << endl;
					assert(pSrc->m_dTrafficGen >= 0);
					assert(pSrc->m_dTrafficGen <= (MAXTRAFFIC_MACROCELL + (SMALLCELLS_PER_MC * MAXTRAFFIC_SMALLCELL)));
				}
			}
			else {
				//-B: Insert next BERNOULLI arrival in events' list for this node
				this->genNextPoissonEvent(pOXCNode, pEvent->m_hTime);
			}			
#ifdef DEBUGB
			//-B: remove the comment if you want to understand why this connection has been blocked
			//this->m_pNetMan->dump(cout);
			//cin.get();
#endif // DEBUGB

			delete pCon;
		}
		else //-B: if bSuccess = true -> CONNECTION PROVISIONED
		{
#ifdef DEBUG
			cout << "\n++++++++++++++++++++ PROVISIONED!" << endl << endl;
			pCon->dump(cout);
			//cin.get();
#endif // DEBUGB

			if (pCon->m_eConnType == Connection::FIXED_BACKHAUL)
			{

				//-B: STEP 1 - insert corresponding departure event (simple case)
				Event* nEvDep = new Event(pEvent->m_hTime + pCon->m_dRoutingTime + pCon->m_dHoldingTime, Event::EVT_DEPARTURE, pCon);
				nEvDep->arrTimeAs = hPrevLogTime; //-B: should be same as pEvent->m_hTime
				m_hEventList.insertEvent(nEvDep);

#ifdef DEBUGC
				cout << "\tSto inserendo il suo departure event nella event list." << endl;
#endif // DEBUGB

				OXCNode*pOXCNode = (OXCNode*)(this->m_pNetMan->m_hWDMNet.lookUpNodeById(pCon->m_nSrc));

				// if the connection is changing BBU, m_bTrafficToBeUpdatedForArrival is set to false -> I don't have to update 
				// the number of free sources
				if (pEvent->fronthaulEvent->m_pConnection->m_bTrafficToBeUpdatedForArrival) {
					//-B: STEP 2 - update amount of traffic generated by source node OF THE ORIGINAL CONNECTION -> i.e. SOURCE OF CORRESPONDING FRONTHAUL EVENT
					m_pNetMan->m_hWDMNet.updateTrafficPerNode(pEvent);

					//-B: STEP 3 - insert next BERNOULLI arrival in events' list for this node
					//double dNextArrival = m_hEventList.nextPoissonArrival(m_dArrivalRate);


					int numOfFreeSources = (pOXCNode->getNumberOfSources() - pOXCNode->getNumberOfBusySources());

					this->genNextPoissonEvent(pOXCNode, pEvent->m_hTime);

				}

			}

			else if (pCon->m_eConnType == Connection::MOBILE_FRONTHAUL
				|| pCon->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
			{
				//-B: il num di provisioned connections era stato aumentato nel metodo di provisioning
				//	ma è giusto aumentarlo solo se la connessione nella sua interezza (fronthaul+backhaul)
				//	viene instradata correttamente, quindi non possiamo ancora dirlo
				//m_pNetMan->m_hLog.m_nProvisionedCon--;
				//m_pNetMan->m_runLog.m_nProvisionedCon--;
				//-B: HO MODIFICATO L'INCREMENTO NEL METODO DI PROVISIONING: ORA VIENE FATTO SOLO PER BACKHAUL CONNECTIONS -> COMMENTO QUESTO DECREMENTO

				//-B: STEP 1 - create a new event corresponding to connection served in this iteration
				//	and insert it in the fronthaul events' list
				Event*nEvFront = new Event((pEvent->m_hTime), Event::EVT_ARRIVAL, pCon);
				m_hFrontEventList.insertEvent(nEvFront);

				//-B: STEP 2 - create corresponding midhaul event and insert it in (standard) events' list
				//-L: connection is set to NULL because it will be created if the provisioning of the midhaul succeeds. 
				Event*nEvMid = new Event((pEvent->m_hTime), Event::EVT_ARRIVAL, NULL, nEvFront); 
				nEvMid->arrTimeAs = hPrevLogTime; //-B: should be same as pEvent->m_hTime
				m_hEventList.insertEvent(nEvMid);
#ifdef DEBUGC
				cout << "\tInserisco evento connessione di backhaul: ";
				if (nEvBack->fronthaulEvent == NULL)
					cout << "NO!" << endl;
				else
					cout << (pEvent->m_hTime) << endl;
#endif // DEBUGB

			}

			else if (pCon->m_eConnType == Connection::FIXED_MIDHAUL)
			{
				//-L: STEP 1 - create corresponding backhaul event and insert it in (standard) events' list
				//-L: connection is set to NULL because it will be created if the provisioning of the backhaul succeeds. 
				//-L: The new backhaul event is initialized with the related fronthaul and midhaul events
				Event*nEvBack = new Event((pEvent->m_hTime), Event::EVT_ARRIVAL, NULL, pEvent->fronthaulEvent, pEvent);
				nEvBack->arrTimeAs = hPrevLogTime; //-B: should be same as pEvent->m_hTime
				m_hEventList.insertEvent(nEvBack);
#ifdef DEBUGC
				cout << "\tInserisco evento connessione di backhaul: ";
				if (nEvBack->fronthaulEvent == NULL)
					cout << "NO!" << endl;
				else
					cout << (pEvent->m_hTime) << endl;
#endif // DEBUGB

			}

			else if (pCon->m_eConnType == Connection::MOBILE_BACKHAUL
				|| pCon->m_eConnType == Connection::FIXEDMOBILE_BACKHAUL)
			{

				//assert(isBBUNode(pCon->m_nSrc) == true); //-B: COMMENTED if we allow a node hosts its own BBU in the cell site as last extreme option

				//-L: STEP 1 - create and insert corresponding departure events for fronthaul, midhaul and backhaul
				Event*nEv_DepFront = new Event((pEvent->m_hTime + pEvent->fronthaulEvent->m_pConnection->m_dHoldingTime
					+ pCon->m_dRoutingTime + pEvent->fronthaulEvent->m_pConnection->m_dRoutingTime),
					Event::EVT_DEPARTURE, pEvent->fronthaulEvent->m_pConnection);
				
				Event*nEv_DepMid = new Event((pEvent->m_hTime + pEvent->fronthaulEvent->m_pConnection->m_dHoldingTime
					+ pCon->m_dRoutingTime + pEvent->fronthaulEvent->m_pConnection->m_dRoutingTime),
					Event::EVT_DEPARTURE, pEvent->midhaulEvent->m_pConnection);

				Event*nEv_DepBack = new Event((pEvent->m_hTime + pEvent->fronthaulEvent->m_pConnection->m_dHoldingTime
					+ pCon->m_dRoutingTime + pEvent->fronthaulEvent->m_pConnection->m_dRoutingTime),
					Event::EVT_DEPARTURE, pCon);

				nEv_DepFront->arrTimeAs = hPrevLogTime;
				nEv_DepMid->arrTimeAs = hPrevLogTime;
				nEv_DepBack->arrTimeAs = hPrevLogTime;
				m_hEventList.insertEvent(nEv_DepFront);
				m_hEventList.insertEvent(nEv_DepMid);
				m_hEventList.insertEvent(nEv_DepBack);

#ifdef DEBUGC
				cout << "\tE' un backhaul event -> Inserisco eventi di departure sia per backhaul sia per fronthaul" << endl << endl;
#endif // DEBUGB

				OXCNode*pOXCNode = (OXCNode*)(this->m_pNetMan->m_hWDMNet.lookUpNodeById(pEvent->fronthaulEvent->m_pConnection->m_nSrc));

				// if the connection is changing BBU, m_bTrafficToBeUpdatedForArrival is set to false -> I don't have to update 
				// the number of free sources
				if (pEvent->fronthaulEvent->m_pConnection->m_bTrafficToBeUpdatedForArrival) {

					//-B: STEP 2 - update amount of traffic generated by source node OF THE ORIGINAL CONNECTION -> i.e. SOURCE OF CORRESPONDING FRONTHAUL EVENT
					m_pNetMan->m_hWDMNet.updateTrafficPerNode(pEvent);

					//-B: STEP 3 - insert next BERNOULLI arrival in events' list
					//double dNextArrival = m_hEventList.nextPoissonArrival(m_dArrivalRate);

					int numOfFreeSources = (pOXCNode->getNumberOfSources() - pOXCNode->getNumberOfBusySources());

					this->genNextPoissonEvent(pOXCNode, pEvent->m_hTime);
				}

			}

			/* //-B: NOT NEEDED FOR MY WORK
			// calcolo costo totale percorso
			Total_Transport_Cost = Total_Transport_Cost + m_pNetMan->transport_cost;
			Total_EDFA_Cost = Total_EDFA_Cost + m_pNetMan->PEDFA_parz;
			Total_NodeProcessing_Cost = Total_NodeProcessing_Cost + m_pNetMan->PProc_parz;
			//cout<<"\nTotalTransportCost(parziale): "<<Total_Transport_Cost;
			//cout<<"\nTotalEDFCost(parziale): "<<Total_EDFA_Cost;
			//cout<<"\nTotalNodeProcessingCost(parziale): "<<Total_NodeProcessing_Cost;
			//cin.get();
			*/

		} //END else if bSuccess
		break;
	} //-B: end case Event::EVT_ARRIVAL:

	//-L: Devo modificare qualcosa qui? 
	case Event::EVT_DEPARTURE:///////////////////////////////////////////////////
	{

		UINT numOfConnections = m_pNetMan->getConnectionDB().m_hConList.size();
		// -L: ???
		// Permettendo di cambiare BBU a una cell site, può avvenire che nell'eventlist ci
		// sia departure di una connessione che è già stata deprovisioned. In questo caso
		// se non trovo la connessione del database, esco dallo switch perché
		// la connessione è gia stata deprovisioned. 
		MappedLinkList<UINT, Connection*> conList = this->m_pNetMan->getConnectionDB().m_hConList;
		list<Connection*>::const_iterator itr;
		Connection* pConDB;
		bool connectionPresent = false;
		UINT pDst = pEvent->m_pConnection->m_nDst;

		for (itr = conList.begin(); itr != conList.end(); itr++) {
			pConDB = (Connection*)(*itr);

				if (pConDB == pEvent->m_pConnection) {
					connectionPresent = true;
					break;
				}
		}

		if (!connectionPresent) { 
#ifdef DEBUGC
			cout << "Connection from " << pEvent->m_pConnection->m_nSrc << " to " <<
				pEvent->m_pConnection->m_nDst << " of type " << pEvent->m_pConnection->m_eConnType <<
				" not present, already deprovisioned!!!" << endl;
			//cin.get();
#endif
			break;
		}

#ifdef DEBUGC
		cout << "Connessione in partenza: " << pEvent->m_pConnection->m_nSequenceNo << ", " << pEvent->m_pConnection->m_nSrc << "->"
			<< pEvent->m_pConnection->m_nDst;
		if (pEvent->m_pConnection->m_pPCircuit)
		{
			cout << " - ha " << pEvent->m_pConnection->m_pPCircuit->m_hRoute.size() << " lightpath associato/i: " << endl;
		}
		else if (pEvent->m_pConnection->m_pCircuits.size() > 0)
		{
			assert(pEvent->m_pConnection->m_eConnType == Connection::ConnectionType::FIXED_BACKHAUL);
			cout << " - ha " << pEvent->m_pConnection->m_pCircuits.size() << " CIRCUITI associati" << endl;
		}
		else
		{
			cout << " - non ha lightpaths associati!" << endl;
		}
#endif //DEBUGB

		assert(pEvent->m_pConnection && m_pNetMan);

#ifdef DEBUGC
		cout << "DEPARTURE EVENT:" << endl;
#endif // DEBUGB
		//-B: if it is a fixed/mobile/fixed-mobile backhaul connection's departure event
		if (pEvent->m_pConnection->m_eConnType == Connection::FIXED_BACKHAUL
			|| pEvent->m_pConnection->m_eConnType == Connection::MOBILE_BACKHAUL
			|| pEvent->m_pConnection->m_eConnType == Connection::FIXEDMOBILE_BACKHAUL)
		{
			assert(!pEvent->backhaulBlocked);
			//-B: increase the number of departures
			m_hEventList.increaseDep();
		}
		else{ //connType == MOBILE_FRONTHAUL || FIXEDMOBILE_FRONTHAUL
		
			//-B: else, se è il departure event di una mobile/fixed-mobile fronthaul connection
			//	non devo incrementare il num di departure perchè la connessione è considerata nella sua totalità
			//	fronthaul+backhaul -> il num di departure verrà incrementato solo nel momento in cui
			//	ci sarà il deprovisioning della corrispondente backhaul connection.
			//	INOLTRE, se anche fosse un departure event per una mobile/fixed-mobile fronthaul connection
			//	il cui fronthaul è stato instradato con successo e il cui backhaul invece è stato bloccato
			//	non devo incrementare il num di departure perchè non è stata instradata con successo la connessione
			//	nella sua totalità (fronthaul+backhaul) e quindi non viene conteggiata come una departure
			//	di una connessione avvenuta

			if (BBUSTACKING == true && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == false)
			{
				//remove connection's dst node from list of active BBUs, if needed
				m_pNetMan->m_hWDMNet.updateBBUsUseAfterDeparture(pEvent->m_pConnection, m_pNetMan->getConnectionDB());
			}
			else if (BBUSTACKING == false && INTRA_BBUPOOLING == true && INTER_BBUPOOLING == false)
			{
				;
			}
			else if (BBUSTACKING == false && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == true)
			{

				m_pNetMan->m_hWDMNet.updatePoolsUse(pEvent->m_pConnection, m_pNetMan->getConnectionDB());
			}
			else
			{
				assert(false);
			}

			//-B: if the fronthaul departure event is not related with a backhaul blocked connection
			if (!pEvent->backhaulBlocked)
			{
				OXCNode*pOXCNode = (OXCNode*)(this->m_pNetMan->m_hWDMNet.lookUpNodeById(pEvent->m_pConnection->m_nSrc));

				if (pEvent->m_pConnection->m_bTrafficToBeUpdatedForDep == true) {
					//-B: update amount of traffic currently generated by source node
					//	(this kind of update must be done here, with the fronthaul departure event, because the connection
					//	has its original source node in the fronthaul connection's source node)

					m_pNetMan->m_hWDMNet.updateTrafficPerNode(pEvent);

				}
			
				//-B: COMPUTE AVERAGE LATENCY
				m_pNetMan->computeAvgLatency(pEvent);
			}
		}//END ELSE

			if (pEvent->m_pConnection->m_pPCircuit || pEvent->m_pConnection->m_pCircuits.size() > 0)
			{
#ifdef DEBUGC
				cout << "\tSorgente != destinazione -> circuito da deprovide + rimuovo la connessione" << endl << endl;
#endif // DEBUGB

				/////// ------------------------ DEPROVISION CONNECTION ----------------------------
				/*-B: INSTRCTIONS MOVED INTO THE DEPROVISION METHOD
				if (pEvent->m_pConnection->m_pPCircuit)
				{
					pEvent->m_pConnection->m_pPCircuit->m_eState = Circuit::CT_Torndown; //tear down su rete principale
				}*/
				m_pNetMan->deprovisionConnection(pEvent->m_pConnection); //Libero le risorse..
			}
			else //-B: it could not to have a circuit if source and destination were in the same node
			{
#ifdef DEBUGC
				cout << "\tSorgente = destinazione -> nessun circuito da deprovide -> rimuovo la connessione" << endl << endl;
#endif // DEBUGB
				pEvent->m_pConnection->m_eStatus = Connection::TORNDOWN;
				m_pNetMan->getConnectionDB().removeConnection(pEvent->m_pConnection);
			}

			//-B: check if it has really removed the connection from connection db
			assert((numOfConnections - 1) == m_pNetMan->getConnectionDB().m_hConList.size());
			//m_pNetMan->getConnectionDB().dump(cout);

			break;
		} // chiusura case Event::EVT_DEPARTURE:

	default:
		DEFAULT_SWITCH;

	// END switch (pEvent->m_hEvent)
	}
//-----------------SALVATAGGIO PARAMETRI OGNI ORA-----------------------
	if (pEvent->m_hTime > 3600 * m - 1) //(pEvent->m_hTime > 10 * m && transitory) //con 60 (togliendo - 1?) è ogni minuto
	{   
		//-B: added by me to choose if we want stats each hour or each N_RUN_CONNECTIONS
		if (!RUNNING_STATS)
		{
			cout << endl << "TIME: "; cout.precision(10); cout << pEvent->m_hTime;
			cout << " -> SALVATAGGIO PERIODICO PARAMETRI DI hLog" << endl;
			UINT ind = (start_time + m - 1) % 24;
			cout << "ind = " << ind << endl;

			//-B: save blocking probability hour by hour
			m_pNetMan->m_hLog.Pblock_hour[ind] = ((double)m_pNetMan->m_hLog.m_nBlockedCon
												/ ((double)m_pNetMan->m_hLog.m_nBlockedCon + (double)m_pNetMan->m_hLog.m_nProvisionedCon));
			cout << "PBlock_hour = " << m_pNetMan->m_hLog.Pblock_hour[ind] << endl;
			m_pNetMan->m_hLog.PblockFront_hour[ind] = ((double)m_pNetMan->m_hLog.m_nBlockedMobileFrontConn + (double)m_pNetMan->m_hLog.m_nBlockedFixMobFrontConn)
					/ ((double)m_pNetMan->m_hLog.m_nBlockedMobileFrontConn + (double)m_pNetMan->m_hLog.m_nBlockedFixMobFrontConn
					+ (double)m_pNetMan->m_hLog.m_nProvisionedMobileFrontConn + (double)m_pNetMan->m_hLog.m_nProvisionedFixMobFrontConn);
		
			m_pNetMan->m_hLog.PblockBack_hour[ind] = ((double)m_pNetMan->m_hLog.m_nBlockedFixedBackConn + (double)m_pNetMan->m_hLog.m_nBlockedMobileBackConn + (double)m_pNetMan->m_hLog.m_nBlockedFixMobBackConn)
					/ ((double)m_pNetMan->m_hLog.m_nBlockedFixedBackConn + (double)m_pNetMan->m_hLog.m_nBlockedMobileBackConn + (double)m_pNetMan->m_hLog.m_nBlockedFixMobBackConn
					+ (double)m_pNetMan->m_hLog.m_nProvisionedFixedBackConn + (double)m_pNetMan->m_hLog.m_nProvisionedMobileBackConn + (double)m_pNetMan->m_hLog.m_nProvisionedFixMobBackConn);

			m_pNetMan->m_hLog.PblockBW_hour[ind] = (double)m_pNetMan->m_hLog.m_nBlockedBW / ((double)m_pNetMan->m_hLog.m_nBlockedBW + (double)m_pNetMan->m_hLog.m_nProvisionedBW);
		
			m_pNetMan->m_hLog.PblockFrontBW_hour[ind] = ((double)m_pNetMan->m_hLog.m_nBlockedMobileFrontBW + (double)m_pNetMan->m_hLog.m_nBlockedFixMobFrontBW)
					/ ((double)m_pNetMan->m_hLog.m_nBlockedMobileFrontBW + (double)m_pNetMan->m_hLog.m_nBlockedFixMobFrontBW
					+ (double)m_pNetMan->m_hLog.m_nProvisionedFixMobFrontBW + (double)m_pNetMan->m_hLog.m_nProvisionedMobileFrontBW);

			m_pNetMan->m_hLog.PblockBackBW_hour[ind] = ((double)m_pNetMan->m_hLog.m_nBlockedFixedBackBW + (double)m_pNetMan->m_hLog.m_nBlockedMobileBackBW + (double)m_pNetMan->m_hLog.m_nBlockedFixMobBackBW)
					/ ((double)m_pNetMan->m_hLog.m_nBlockedFixedBackBW + (double)m_pNetMan->m_hLog.m_nBlockedMobileBackBW + (double)m_pNetMan->m_hLog.m_nBlockedFixMobBackBW
					+ (double)m_pNetMan->m_hLog.m_nProvisionedFixedBackBW + (double)m_pNetMan->m_hLog.m_nProvisionedFixMobBackBW + (double)m_pNetMan->m_hLog.m_nProvisionedMobileBackBW);
		
			//m_pNetMan->m_hLog.output();

		
			//-B: save load, even if I don't know if I need it
			m_pNetMan->m_hLog.carico[ind] = m_pNetMan->carico_rete;

			//-B: for each fiber and for each hour, it save the corresponding load in terms of: (channel used*time) / (tot channels * time)
			list<AbstractLink*>::const_iterator itr;
			UniFiber*pUniFiber;
			vector<double> UniFiberLoad;
			double value;
			//m_pNetMan->dump(cout);
			for (itr = m_pNetMan->m_hWDMNet.m_hLinkList.begin(); itr != m_pNetMan->m_hWDMNet.m_hLinkList.end(); itr++)
			{
				pUniFiber = (UniFiber*)(*itr);
				value = pUniFiber->m_dLinkLoad / ((double)pUniFiber->m_nW * pEvent->m_hTime);
				UniFiberLoad.push_back(value); //-B: li avrò quindi dall'ultimo link fino al primo (decrescente diciamo)
	#ifdef DEBUG
				cout <<"#" << pUniFiber->getId() << " LOAD = " << pUniFiber->m_dLinkLoad << "/ (" << (double)pUniFiber->m_nW << "*" << pEvent->m_hTime << ") = " << value << endl;
				//cin.get();
	#endif // DEBUGB
				//-B: reset values to do periodic measurements
				pUniFiber->m_dLinkLoad = 0;
			}
			m_pNetMan->m_hLog.UniFiberLoad_hour.insert(pair<UINT, vector<double> >(ind, (UniFiberLoad)));
			//cin.get();
			//cout << "..." << endl;

			/* -B:########################### NOT NEEDED FOR MY WORK ###############################
						////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//double conn_block=(m_pNetMan->m_hLog.m_nBlockedCon);        // # connessioni bloccate (fino a questo istante)
			//double conn_prov=(m_pNetMan->m_hLog.m_nProvisionedCon);     // # connessioni instaurate
			//double tot_conn = conn_block+conn_prov;                       // # connessioni totali
			//double Pblock=conn_block/tot_conn;
			double sum_E=0;
			double sum_T=0;
			double sum_P=0;
			double green_energy1=0;
			double green_energy2=0;
			double brown_energy=0;
		
			int k = 0;
			//Pblock_hour[(start_time+m-1)%24]=Pblock;

					for (k=0;k!=m_pNetMan->m_hWDMNet.nDC;k++) 
					  {  
						 gp[k]=gp_incr[k]+gp_rest[k];								    //energia disponibile in ogni DC (compresa la rimanente da ora prec.)
					  // gp[k]=gp_incr[k];												//senza sommare quella avanzata
						 diff_DCprocessing[k]=DCprocessing[k]-DCprocessing_old[k];		// potenza usata dal DC k in ques'ora, introdotta la dipendenza dall'HT
						 diff_conn[k]=m_pNetMan->m_hWDMNet.nconn[k]-nconn_old[k];       // differenza del numero di connessioni instaurate nel DC k	
						 nconn_old[k]=m_pNetMan->m_hWDMNet.nconn[k];
						 DCprocessing_old[k]=DCprocessing[k];	
					   }

					if (m_pNetMan->m_eProvisionType==NetMan::PT_UNPROTECTED_GREEN) {
						for (k=0;k!=m_pNetMan->m_hWDMNet.nDC;k++)  { 
							  g_en[k]=gp[k]-diff_DCprocessing[k];				   // differenza tra energia Green disponibile in quest'ora e l'energia richiesta dalle connessioni arrivate in quest'ora -->energia residua
						
							if (g_en[k]<0) {										// se la differenza è negativa, utilizzo energia Green + energia Brown
								 brown_energy_DC[k]=(-1)*g_en[k];					//quella brown è data dal valore negativo di green
								 green_energy_DC[k]=gp[k];							// uso tutta quella che ho green (compresa la rimanente da ora prec.)
								 green_energy1=green_energy1+green_energy_DC[k];	// sommo su tutti i DC per ottenere l'energia green totale (1) consumata in quest'ora
								 brown_energy=brown_energy+brown_energy_DC[k];		// sommo su tutti i DC per ottenere l'energia brown totale consumata in quest'ora
								 gp_rest[k]=0;										// energia green non usata è in questo caso 0
								}
							else {													// se la differenza è positiva, utilizzo solo energia Green!!
								 green_energy_DC[k]=diff_DCprocessing[k];			// energia green è data dalla differenza tra quella totale e quella usata
								 gp_rest[k]=gp[k]-diff_DCprocessing[k];			    // energia green non usata è la differenza tra quella green disp nell'ora e quella usata nell'ora
								 green_energy2=green_energy2+green_energy_DC[k];	// sommo su tutti i DC per ottenere l'energia green totale (2) consumata in quest'ora
								}
						}
					 }
					else { 
						double conn_prov2 = m_pNetMan->m_hLog.m_nProvisionedCon;
						brown_energy_temp[ind]=ComputingCost*conn_prov2-brown_energy_temp[ind-1];	// caso in cui non ci sono green DC--> solo Brown 
						green_energy_temp[ind]=0;	
					}  
			
				if (m==24) {
				for (int jj=start_time;jj!=ind;jj++) {
						jj=jj%24;
					   sum_E=sum_E+EDFA_temp[jj];
					   sum_T=sum_T+transport_temp[jj];
					   sum_P=sum_P+processing_temp[jj];}
				}
				else {
				for (int jj=start_time;jj!=ind+1;jj++) {
					   jj=jj%24;
					   sum_E=sum_E+EDFA_temp[jj];
					   sum_T=sum_T+transport_temp[jj];
					   sum_P=sum_P+processing_temp[jj];}
				}

			brown_energy_temp[ind]=brown_energy;
			green_energy_temp[ind]=green_energy1+green_energy2;					         	//sommo i 2 contributi di green energy
			EDFA_temp[ind]=Total_EDFA_Cost-sum_E;
			processing_temp[ind]=Total_NodeProcessing_Cost-sum_P;
			transport_temp[ind]=Total_Transport_Cost-sum_T;   	    
			CO2emissions_comp[ind]=gCO2perKWh*brown_energy_temp[ind]/1000;                 // in [(grammi/kwh )* kilowatt(in un'ora)]== grammi, poi nell'output divido per 1000 -->kilogrammi 
			CO2emissions_transport[ind]=gCO2perKWh*transport_temp[ind]/1000;  
			CO2emissions_tot[ind]=CO2emissions_comp[ind]+CO2emissions_transport[ind];    
			//provi[ind]=conn_prov;															//connessioni instaurate in quest'ora
			m++;
			//    cout<<"\nTotalEDFACost(parziale): "<<Total_EDFA_Cost;	
			//    cout<<"\nTotalNodeProcessingCost(parziale): "<<Total_NodeProcessing_Cost;
			// 	  cout<<"\nTotalTransportCost(parziale): "<<Total_Transport_Cost;
			// 	  cin.get();
			*/
	
		} //end IF !RUNNING_STATS

	} // chiusura if salvataggio ora

//---------------------------------------- FINE SALVATAGGIO PARAMETRI OGNI ORA ---------------------------------
 
#ifdef DEBUGB
	if (m_hEventList.getNumArr() % m_nRunConnections == 0 && pEvent->m_hEvent != Event::EVT_DEPARTURE) //m_hEventList.getNumArr() sostituisce 
	{
		//m_pNetMan->dump(cout);
		//cin.get();
	}
#endif // DEBUGB

	//-B: for each active lightpath we should have added a simplex link LT_Lightpath in the extended graph
	assert(m_pNetMan->getLightpathDB().m_hLightpathList.size() == (m_pNetMan->countLinksInGraph() - numSimplexLinks));


	delete pEvent; //(riaggiunto)FABIO:per evitare problemi con l'evento in arrivalList
	//cout << "#" << numcon << ", Fine ciclo." << endl;
	//cin.get();
#ifdef DEBUGC

		 cout << "\n- LightpathDB, # = " << m_pNetMan->getLightpathDB().m_hLightpathList.size() << endl
			 << "- ConnectionDB, # = " << m_pNetMan->getConnectionDB().m_hConList.size() << endl
			 << "- Graph simplex links, # = " << m_pNetMan->getSimplexLinkGraphSize() << endl;
#endif
#ifdef DEBUGF
		 cout << "*********************************************************************\n"
			 << "FINE WHILE:\t" << m_pNetMan->m_hLog.m_nProvisionedCon << " prov ("
			 << m_pNetMan->m_hLog.m_nProvisionedMobileFrontConn << " M-F + "
			 << m_pNetMan->m_hLog.m_nProvisionedFixMobFrontConn << " FM-F + "
			 << m_pNetMan->m_hLog.m_nProvisionedFixedBackConn << " F-B + "
			 << m_pNetMan->m_hLog.m_nProvisionedMobileBackConn << " M-B + "
			 << m_pNetMan->m_hLog.m_nProvisionedFixMobBackConn << " FM-B"
			 << ") connections\n\t\t" << m_pNetMan->m_hLog.m_nBlockedCon << " bloc ("
			 << m_pNetMan->m_hLog.m_nBlockedMobileFrontConn << " M-F + "
			 << m_pNetMan->m_hLog.m_nBlockedFixMobFrontConn << " FM-F + "
			 << m_pNetMan->m_hLog.m_nBlockedFixedBackConn << " F-B + "
			 << m_pNetMan->m_hLog.m_nBlockedMobileBackConn << " M-B + "
			 << m_pNetMan->m_hLog.m_nBlockedFixMobBackConn << " FM-B) ("
			 << m_pNetMan->m_hLog.m_nBlockedBWDueToUnreach << " unreach + "
			 << m_pNetMan->m_hLog.m_nBlockedConDueToLatency << " lat"
			 << ") connections\n\t\t" << (m_hEventList.getNumArr() + m_hEventList.getNumDep())
			 << " (" << m_hEventList.getNumArr() - m_pNetMan->m_hLog.m_nNumConnectionsChangingBBU << " arr + " << m_hEventList.getNumDep()
			 << " dep) " << " events\n\t\t" << "BBU changes " << m_pNetMan->m_hLog.m_nNumConnectionsChangingBBU
			 << endl
			 << "*********************************************************************" << endl;
		 m_pNetMan->m_hWDMNet.printHotelNodes();
		 if (BBUSTACKING == true)
		 {
			 if (m_pNetMan->m_hWDMNet.BBUs.size() > 1 && MAXNUMBBU >= m_pNetMan->m_hWDMNet.numberOfNodes)
			 {
#ifdef DEBUG
				 m_pNetMan->m_hWDMNet.printBBUs();
#endif
				 //cout << "SE PREMI INVIO FACCIO NETMAN DUMP" << endl;
				 //cin.get();
				 //m_pNetMan->dump(cout);
				 //cin.get();

			 }
		 }

#ifdef DEBUGX
		 //print number of connections at each source
		 list<AbstractNode*>::const_iterator iter;
		 OXCNode*pOXCNode;
		 for (iter = this->m_pNetMan->m_hWDMNet.m_hNodeList.begin(); iter != this->m_pNetMan->m_hWDMNet.m_hNodeList.end(); iter++)
		 {
			 pOXCNode = (OXCNode*)(*iter);
			 MappedLinkList<UINT, Connection*> conList = this->m_pNetMan->getConnectionDB().m_hConList;

			 list<Connection*>::const_iterator itr;
			 Connection* pConDB;
			 UINT numConnection = 0 ;

			 for (itr = conList.begin(); itr != conList.end(); itr++) {
				 pConDB = (Connection*)(*itr);
				 if (pConDB->m_nSrc == pOXCNode->getId()) {
					 numConnection++;
				 }
			 }
			 if (numConnection > 0) {

				 cout << "\tNumber of connections at node " << pOXCNode->getId() << ": " << numConnection << endl;
			
			 }
		 }
		 cout << "Provisioned BWD: " << m_pNetMan->m_hLog.m_nProvisionedBW << endl;
		 cout << "Blocked BWD: " << m_pNetMan->m_hLog.m_nBlockedBW << endl;
#endif
		 //if (m_pNetMan->m_hLog.m_nBlockedCon > 0)
		 //{
		 //cin.get();
		 //}
		 //m_pNetMan->printChannelLightpath();
		 //cout << endl;
		 //m_pNetMan->printChannelReference();
		 //cout << endl;
		 //m_pNetMan->dump(cout);
		 //cin.get();
		 //m_pNetMan->connDBDump();
		 //cout << "-> EventList dump" << endl;
		 //m_hEventList.dump(cout);
		 //cout << "-> FRONThaulEventList dump" << endl;
		 //m_hFrontEventList.dump(cout);
#endif // DEBUGB

#ifdef DEBUGB
		 //-B: VERY BIG CHECK: CONTROL FREE CAPACITY OF LIGHTPATHS, SIMPLEX LINKS LT_Lightpath AND LT_Channel
		 //	if you have doubts about correct provisioning and deprovisioning of connections, let it uncommented
		 m_pNetMan->checkLpToSLinkEquality();
#endif // DEBUGB
 
} //---------------------------------------end of While------------------------------------------------------------------------------------------------
 //-----------------------------------------------------------------------------------------------------------------------------------------------------


 //---------------------------salvataggio nel file "grafici.txt"------------------------------------

	if (stat_achieved == true && bDone == false)
	{
		cout << endl << "USCITO DAL CICLO WHILE PER CONFIDENZA STATISTICA RAGGIUNTA!" << endl << endl;
	}
	else if (bDone == true && stat_achieved == false)
	{
		cout << endl << "USCITO DAL CICLO WHILE PER NUMERO DI CONNESSIONI TOT/TEMPO DA SIMULARE RAGGIUNTO!" << endl << endl;
	}
	else
	{
		cout << "bDone = " << bDone << " - stat_achieved = " << stat_achieved << endl;
	}

	//tot num of connections during the simulation (needed in ConProvisionMain)
	m_pNetMan->m_hLog.NumOfConnections = m_hEventList.getNumArr(); //m_hEventList.getNumArr() sostituisce numcon

#ifdef DEBUGB
	cout << "-----------STAMPO RISULTATI FINALI DOPO IL WHILE-----------" << endl;
#endif // DEBUGB

	/* -B: NOT NEEDED FOR MY WORK
	std::ofstream out("GraficiGreen.txt",ios_base::app);  
	out<<"\n------------------------------------------------------------------- ";   
	out<<"\nPBloc: ";
	for (int oo=0;oo<24;oo++) {
	out<<Pblock_hour[oo]<<" ";}
	out<<"\n\nGreen [kW]: ";
	for (int oo=0;oo<24;oo++) {
	out<<green_energy_temp[oo]/1000<<" ";}
	out<<"\n\nBrown [kW]: ";
	for (int oo=0;oo<24;oo++) {
	out<<brown_energy_temp[oo]/1000<<" "; }
	out<<"\n\nTransport [kW]: ";
	for (int oo=0;oo<24;oo++) {
	out<<transport_temp[oo]/1000<<" ";}
    out<<"\n\nEDFA [kW]: ";
	for (int oo=0;oo<24;oo++) {
	out<<EDFA_temp[oo]/1000<<" ";}
	out<<"\n\nProcessing [kW]: ";
	for (int oo=0;oo<24;oo++) {
	out<<processing_temp[oo]/1000<<" ";}
	out<<"\n\nC02comp [Kg]: ";
	for (int oo=0;oo<24;oo++) {
	out<<CO2emissions_comp[oo]/1000<<" ";}
	out<<"\n\nCO2trans [Kg]: ";
	for (int oo=0;oo<24;oo++) {
	out<<CO2emissions_transport[oo]/1000<<" ";}
	out<<"\n\nCO2tot [Kg]: ";
	for (int oo=0;oo<24;oo++) {
	out<<CO2emissions_tot[oo]/1000<<" ";}
	out<<"\n\ncarico: ";
	for (int oo=0;oo<24;oo++) {
	out<<carico[oo]<<" ";}
	*/
//----------------------------------------------------------------------------------

	/* -B: NOT NEEDED FOR MY WORK
		// CALCOLO COMPUTING COST TOTALE //
double green_total=0;
double brown_total=0;
for (int oo = 0; oo < 24; oo++) {
	green_total=green_total+green_energy_temp[oo];  //somma dei 24 valori presi ogni ora
    brown_total=brown_total+brown_energy_temp[oo];
}
double computing_cost=green_total+brown_total;

  cout <<"\n--------------------------";
  cout <<"\nCOMPUTING COST ";
  cout <<"\n   GREEN ENERGY: "<<green_total/1000<<" kW";
  cout <<"\n   BROWN ENERGY: "<<brown_total/1000<<" kW";
  cout <<"\nTRANSPORT+EDFA COST: "<<Total_Transport_Cost/1000000<<" MW";
  int cont=0;
  int tot=0;
  cout <<"\n\nCONNESSIONI RIUSCITE: ";
  for (cont = 0; cont != m_pNetMan->m_hWDMNet.nDC; cont++) {
	  tot = tot + m_pNetMan->m_hWDMNet.DCCount[cont];
  }
	  //cout<<m_pNetMan->m_hWDMNet.DCCount[cont]<<" ";  } 
      cout<<tot<<"\n";
cout<<"\n--------------------------\n";


m_pNetMan->PTot=Total_Transport_Cost/1000000+computing_cost/1000000;
m_pNetMan->PTransport=Total_Transport_Cost/1000000;
m_pNetMan->PProc=Total_NodeProcessing_Cost/1000000;
m_pNetMan->PEDFA=Total_EDFA_Cost/1000000;
m_pNetMan->PGreen=green_total/1000000;
m_pNetMan->PBrown=brown_total/1000000;
//m_pNetMan->PBrown=m_pNetMan->PTot-green_total/1000000; //Compresa Potenza Non Rinnov. EDFA
	*/

//cout<<"TotalTime:"<<pEvent->m_hTime<<endl;
    // The above while can exit when pEvent is NOT NULL;
    if (pEvent) delete pEvent;
    while (pEvent = m_hEventList.nextEvent()) {
        delete pEvent;
    }
	 // log anything left
    m_pNetMan->logFinal();
	extern int counter;
	extern long int linkvisited;
}


bool Simulator::setNetworkManager(NetMan *pNetMan)
{
    assert(pNetMan);
    m_pNetMan = pNetMan;
    m_nNumberOfOXCNodes = pNetMan->getNumberOfOXCNodes();
    m_nNumberOfNodePairs = m_nNumberOfOXCNodes * (m_nNumberOfOXCNodes - 1);
    if (m_nNumberOfNodePairs <= 0) {
        TRACE2("- Warning: m_nNumberOfNodePairs = %d, m_nNumberOfOXCNodes = %d\n",
            m_nNumberOfNodePairs, m_nNumberOfOXCNodes);
        return false;
    }
    return true;
}

//EventList Simulator::getEventList()
//
//return m_hEventList;
//

//-B: method modified to adapt to RAN
Connection* Simulator::newConnection(SimulationTime  tArrivalTime)
{
	static UINT g_nConSeqNo = 1;
	/* -B: commented
	static UINT g_ppBWD[][5] = {
		{192, 64, 16, 4, 1},        // Zipf
		{10000, 2000, 200, 20, 2},  // SBC
		{300, 20, 6, 4, 1},         // Sprint 2003
		{1, 1, 1, 1, 1},            // uniform
		{0, 0, 0, 1, 0},            // for debug, everything OC48
		{0, 0, 0, 0, 1},            // for debug, everything OC19k
	};
	static UINT g_pBWSummation[] = { 277, 12222, 331, 5, 1, 1 };
	*/

	// Genero Sorgenti Casuali verso il Dummy Node
	UINT nDst;
	UINT nSrc;
	// generate src/dst according to uniform distribution
	// -B: commented because it seems not to be random
	/*int nNodePair =	(int)((rand() / (RAND_MAX + 1.0)) * m_nNumberOfNodePairs) + 1;
	nNodePair = nNodePair + ((nNodePair - 1) / m_nNumberOfOXCNodes + 1);*/
	do {
		//-B: rand() seems to be not random at all
		//-B: generate random number (between 1 and numberOfOXCNodes) with Mersenne Twister Engine
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(1, m_nNumberOfOXCNodes);
		nSrc = dis(gen);
		
		// -B: generate random number (between 1 and numberOfOXCNodes) with Default Random Engine
		/*std::default_random_engine generator;
		std::uniform_int_distribution<int> distribution(1, m_nNumberOfOXCNodes);
		nSrc = distribution(generator); //sorgente casuale distribuzione uniforme
		*/

	} while (nSrc == m_pNetMan->m_hWDMNet.DummyNode);

	//nSrc = 12; //-B: 11 è l'unico nodo che ha 2 path disgiunti per arrivare al nodo destinazione 5. Viene riempito prima lo shortest e poi l'altro
	nDst = m_pNetMan->m_hWDMNet.DummyNode;    //la destinazione è sempre il Core CO

	//nDst = (nNodePair - 1) % m_nNumberOfOXCNodes;  //non mod
	//nSrc = (int) (rand() %(m_nNumberOfOXCNodes-1) +1); // sorgente casuale, se DummyNode è nodo 0
	//nSrc = (int) (rand() %(m_nNumberOfOXCNodes-1)); // sorgente casuale, se DummyNode è l'ultimo nodo della lista


//int sorgi[]={0,2,3,4,6,7,8,9,10,12,14,16,18,19,20,21,22,23};
//int desti[]={1,5,11,13,15,17};
//int indices=1+rand()%(18-1);
//int indiced=1+rand()%(6-1);
//nSrc = sorgi[indices];
//nDst = desti[indiced];

	//cout<<"\nSource:"<<nSrc<<" Dest:"<<nDst;  //stampa Sorg e Dest della connessione
	//cin.get();
	assert(0 <= nSrc && nSrc <= m_nNumberOfOXCNodes);

	// generate bandwidth distribution
	BandwidthGranularity eBandwidth;
	BandwidthGranularity CPRIBwd;

	OXCNode*source = (OXCNode*)m_pNetMan->m_hWDMNet.lookUpNodeById(nSrc);;
	switch (m_pNetMan->m_eProvisionType)
	{
		case 21: //-B: == PT_BBU
		{	
			//-B: assign bandwidth amount to traffic request
			if (m_pNetMan->m_hWDMNet.isMobileNode(nSrc))	//-B: mobile request
			{
				eBandwidth = BWDGRANULARITY;
				CPRIBwd = OCcpriMC;
				cout << "Mobile: ";
				//break;
			}
			else if (m_pNetMan->m_hWDMNet.isFixedNode(nSrc))	//-B: fixed request
			{
				eBandwidth = FIXED_TRAFFIC;
				CPRIBwd = OC0;
				cout << "Fixed: ";
				//break;
			}
			else if (m_pNetMan->m_hWDMNet.isFixMobNode(nSrc))	//-B: fixed-mobile request
			{
				eBandwidth = FIXED_TRAFFIC;
				CPRIBwd = OCcpriMC;
				cout << "Fixed-Mobile: ";
				//break;
			}
			else   //-B: simple connection
			{
				eBandwidth = BWDGRANULARITY; //OC3
				CPRIBwd = OC0;
				break;
			}
		}	// END case 21
		default:
		{
			eBandwidth = OC192;
			break;
		}
	}	

//  int nBWRange = 
//     (int)((rand() / (RAND_MAX + 1.0)) * g_pBWSummation[m_eBWDistribution]) + 1;
//  UINT *pBWD = g_ppBWD[m_eBWDistribution];
//  if (nBWRange <= pBWD[0])
//      eBandwidth = OC1;
//  else if (nBWRange <= (pBWD[0] + pBWD[1]))
//      eBandwidth = OC3;
//  else if (nBWRange <= (pBWD[0] + pBWD[1] + pBWD[2]))
//      eBandwidth = OC12;
//  else if (nBWRange <= (pBWD[0] + pBWD[1] + pBWD[2] + pBWD[3]))
//      eBandwidth = OC48;
//  else if (nBWRange <= (pBWD[0] + pBWD[1] + pBWD[2] + pBWD[3] + pBWD[4]))
//      eBandwidth = OC192;
//  else {
//      assert(false);
//  }

	//-B: PERCHE' VIENE PASSATO g_nConSeqNo++ COME NUM DI SEQUENZA DELLA CONNESSIONE??? MOD: INCREMENTO MESSO A FINE METODO
	Connection *pConnection;
	if (CPRIBwd >= 0)  // -L: ??? if ed else fanno lo stesso
	{
		pConnection = new Connection(g_nConSeqNo, nSrc, nDst, //LEAK
				tArrivalTime, EventList::expHoldingTime(m_dMeanHoldingTime),
				eBandwidth, m_ePClass);
	}
	else
	{
		pConnection = new Connection(g_nConSeqNo, nSrc, nDst, //LEAK
			tArrivalTime, EventList::expHoldingTime(m_dMeanHoldingTime),
			eBandwidth, m_ePClass);
	}


	//cout<<"holding time:"<<EventList::expHoldingTime(m_dMeanHoldingTime)<<"mean: "<<m_dMeanHoldingTime<<"S: "<<nSrc<<"D:"<<nDst;
	//cin.get();

	// generate hop count 5: 6: 7: inf = 10: 10: 20: 60;
    // static int g_pHopCount[4] = {1, 2, 4, 10};
    // generate hop count 5: 6: 7: inf = 30: 20: 10: 40;
    static int g_pHopCount[4] = {3, 5, 6, 10};
    UINT nHopCount;
	//-B: il metodo initialize della classe Simulator inizializza la var m_nHopCount = 30 che è il num max di hops
	//	(val dato da linea di comando), non il val di hop per qualsiasi connessione
	//	perciò in questo if si entra solo se da linea di comando metto 0 -> in tal modo il max num di hop se lo calcola da solo
    if (0 == m_nHopCount) {
        int nIndex = (int)((rand() / (RAND_MAX+1.0)) * 10) + 1;
        if (nIndex <= g_pHopCount[0]) {
            nHopCount = 5;
		} else if (nIndex <= g_pHopCount[1]) {
            nHopCount = 6;
		} else if (nIndex <= g_pHopCount[2]) {
            nHopCount = 7;
		} else if (nIndex <= g_pHopCount[3]) {
            nHopCount = (UINT)(UNREACHABLE - 1);    
		} else {
            assert(false);
		}
    } else {
        nHopCount = m_nHopCount;
    }
    pConnection->m_nHopCount = nHopCount;
	g_nConSeqNo++;
    return pConnection;
}

bool NS_OCH::Simulator::isBBUNode(UINT nodeId)
{
	OXCNode*node;
	node = (OXCNode*)m_pNetMan->m_hWDMNet.lookUpNodeById(nodeId);
	assert(node);
	return (node->getBBUHotel());
}

void Simulator::showLog(ostream& out) const
{//m_pNetMan->m_hLog.m_nPHopDistance+m_pNetMan->m_hLog.m_nBHopDistance

////Controllo parametri CARICO IN RETE
//cout<<"\n SLL"<<m_pNetMan->m_hLog.m_dSumLinkLoad;
//cout<<"\n AR/HT"<<m_dArrivalRate/m_dMeanHoldingTime;
//cout<<"\n Fiber"<<(m_pNetMan->getNumberOfUniFibers()-m_pNetMan->m_hWDMNet.nDC*2);
//cout<<"\n CountChannels"<<m_pNetMan->m_hWDMNet.countChannels();
//cin.get();

	cout << "-> showLog (ogni " << m_nRunConnections << " connections)" << endl;

	double carico_in_rete = (double)(m_pNetMan->m_hLog.m_dSumLinkLoad)/				//NOTA:DOVREBBE essere in realta il reciproco dell HT
		(double)(m_pNetMan->m_hLog.m_nProvisionedCon+m_pNetMan->m_hLog.m_nBlockedCon)*
		(m_dArrivalRate/m_dMeanHoldingTime)/
		((m_pNetMan->getNumberOfUniFibers()-m_pNetMan->m_hWDMNet.nDC*2)* m_pNetMan->m_hWDMNet.countChannels() );
	
	out<< "- CARICO IN RETE: " << (double)(m_pNetMan->m_hLog.m_dSumLinkLoad)/				//NOTA:DOVREBBE essere in realta il reciproco dell HT
		(double)(m_pNetMan->m_hLog.m_nProvisionedCon+m_pNetMan->m_hLog.m_nBlockedCon)*
		(m_dArrivalRate/m_dMeanHoldingTime)/
		(m_pNetMan->getNumberOfUniFibers()* m_pNetMan->m_hWDMNet.countChannels()) //-B: it gives the num of channels in all the network
		//((m_pNetMan->getNumberOfUniFibers()-m_pNetMan->m_hWDMNet.nDC*2)* m_pNetMan->m_hWDMNet.countChannels()) //-B: commented by me, because I don't have any DC
			<<'\t'; //FABIO:carico in rete

	m_pNetMan->carico_rete = carico_in_rete;
    out << "\n- ARRIVAL RATE: " << m_dArrivalRate << endl;
    m_pNetMan->showLog(out);
    out << endl;
	cout << "---------------ESCO showLog---------------" << endl;

}

//-B: originally taken from newConnection
Connection* Simulator::BBU_newConnection(Event*pEvent, int runningPhase)
{
#ifdef DEBUG
	cout << "-> BBU_NewConnection: " << endl;
#endif // DEBUGB

	static UINT g_nConSeqNo = 1;
	/* -B: commented
	static UINT g_ppBWD[][5] = {
	{192, 64, 16, 4, 1},        // Zipf
	{10000, 2000, 200, 20, 2},  // SBC
	{300, 20, 6, 4, 1},         // Sprint 2003
	{1, 1, 1, 1, 1},            // uniform
	{0, 0, 0, 1, 0},            // for debug, everything OC48
	{0, 0, 0, 0, 1},            // for debug, everything OC19k
	};
	static UINT g_pBWSummation[] = { 277, 12222, 331, 5, 1, 1 };
	*/

	//-B: ------------- SELECT SOURCE AND DESTINATION NODE && ASSIGN CONNECTION'S BANDWIDTH -----------------------
	UINT nDst;
	UINT nSrc;
	int r;
	BandwidthGranularity eBandwidth;
	BandwidthGranularity CPRIBwd;
	Connection::ConnectionType connType;
	bool fronthaulFlag = false;
	SimulationTime holdingTime = -1;
	bool office = true;

	//-B: precomputedPath cleared, from previous cycle calculation, each time the cycle calls BBU_newConnection method
	m_pNetMan->clearPrecomputedPath();

	//-B: if fronthaul has not been routed yet
	if (pEvent->fronthaulEvent == NULL)
	{	
		//-B: ############### SELECT RANDOM SOURCE ###################

		// generate src/dst according to uniform distribution
		/* // -B: commented because it seems not to be random
		int nNodePair =	(int)((rand() / (RAND_MAX + 1.0)) * m_nNumberOfNodePairs) + 1;
		nNodePair = nNodePair + ((nNodePair - 1) / m_nNumberOfOXCNodes + 1);
		*/

		/*
		//-B: generate random number (between 1 and 10) with Mersenne Twister Engine
		std::random_device rdmz;
		std::mt19937 generate(rdmz());
		std::uniform_int_distribution<> distr(1, 10);
		r = distr(generate);

		if (runningPhase >= END_OFFICE_HOURS) //--> less probable to have office nodes as source, more probable for residential ones
		{
			if (r > 6) //--> 4 possibilities -> 40% prob
			{
				//-B: EXTRACT OFFICE NODE AS SOURCE
				//nSrc = this->genSource(m_pNetMan->m_hWDMNet.OfficeNodes);
				office = true;
			}
			else //if (r <= 6) //--> 6 possibilities -> 60% prob
			{
				//-B: EXTRACT RESIDENTIAL NODE AS SOURCE
				//nSrc = this->genSource(m_pNetMan->m_hWDMNet.ResidentialNodes);
				office = false;
			}
		}
		else //if (runningPhase < END_OFFICE_HOURS) //--> more probable to have office nodes as source, less probable for residential ones
		{
			if (r <= 6) //--> 6 possibilities -> 60% prob
			{
				//-B: EXTRACT OFFICE NODE AS SOURCE
				//nSrc = this->genSource(m_pNetMan->m_hWDMNet.OfficeNodes);
				office = true;
			}
			else //if (r > 6) //--> 4 possibilities -> 40% prob
			{
				//-B: EXTRACT RESIDENTIAL NODE AS SOURCE
				//nSrc = this->genSource(m_pNetMan->m_hWDMNet.ResidentialNodes);
				office = false;
			}
		}*/

		//-B: gen backhaul traffic for the new connection
		//	(MC: with granularity of OC3; SC: with granularity of OC1)
		BandwidthGranularity bwdMC = (BandwidthGranularity)this->genMobileBwdMCell();
		UINT bwdSC = this->genMobileBwdSCell();

		vector<UINT> potentialSources;

		//********************************************************************
		//-B: ------------------------- IMPORTANT! ---------------------------
		//	the implementation assumes that network consists either of only [macro cells] or of [macro cells + small cells] (--> hetnet)
		//********************************************************************
		if (ONE_CONN_PER_NODE)
		{
			//-B: get only MACRO cells not having any connection
			m_pNetMan->m_hWDMNet.extractPotentialSourcesMC_1conn(office, potentialSources, this->m_pNetMan->getConnectionDB().m_hConList);
		}
		else
		{
			//-B: MORE THAN 1 CONNECTION at a time is possible
			//-B: get only MACRO cells not overcoming their max capacity
			m_pNetMan->m_hWDMNet.extractPotentialSourcesMC_Nconn(office, potentialSources, bwdMC, bwdSC);
		}

		if (potentialSources.size() == 0)
			assert(false);

		//do {
		//-B: generate random number (between 1 and potentialSources.size()) with Mersenne Twister Engine
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(1, potentialSources.size());
		r = dis(gen);
		nSrc = potentialSources[r - 1];
		// -B: generate random number (between 1 and numberOfOXCNodes) with Default Random Engine
		/*std::default_random_engine generator;
		std::uniform_int_distribution<int> distribution(1, m_nNumberOfOXCNodes);
		nSrc = distribution(generator); //sorgente casuale distribuzione uniforme
		*/
		//} while (nSrc == m_pNetMan->m_hWDMNet.DummyNode);

		//-B: if the macro cell chosen is already a source node -> choose one of its small cell as source
		//	(funzionamento analogo sia per fronthaul che per midhaul)
		OXCNode*pNode = (OXCNode*)m_pNetMan->m_hWDMNet.lookUpNodeById(nSrc);
		if (ONE_CONN_PER_NODE == false) //MORE CONNECTIONS PER NODE
		{
			//-B: if pNode was put into potentialSources list because there isn't enough capacity into it, but there is enough capacity into its small cells
			if (pNode->m_dTrafficGen + bwdMC > MAXTRAFFIC_MACROCELL)
			{
#ifdef DEBUGB
				cout << "ECCOLO! Macro Cell " << nSrc << " gia' source. Scelgo una delle sue small cell" << endl;
				//cin.get();
#endif // DEBUGB
				//-B: MORE THAN 1 CONNECTION at a time is possible
				//-B: get only small cells not having overcome their maximum capacity
				m_pNetMan->m_hWDMNet.extractPotentialSourcesSC_Nconn(potentialSources, bwdSC, nSrc);

#ifdef DEBUGB
				cout << "Le small cell disponibili sono " << potentialSources.size() << endl;
#endif // DEBUGB
				assert(potentialSources.size() > 0); //-B: otherwise this macro cell should not have been chosen as source

				//-B: generate random number (between 1 and potentialSources.size()) with Mersenne Twister Engine
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(1, potentialSources.size());
				r = dis(gen);
				nSrc = potentialSources[r - 1];

				//-B: one step more in case of more connections per node
				//-B: consider small cell extracted
				pNode = (OXCNode*)m_pNetMan->m_hWDMNet.lookUpNodeById(nSrc);
				//-B: if, with the new connection, the small cell would overcome its max capacity
				if ((pNode->m_dTrafficGen + bwdSC) > MAXTRAFFIC_SMALLCELL)
				{
					//-B: reduce the backhaul bandwidth to the maximum admissible value
					bwdSC = (MAXTRAFFIC_SMALLCELL - pNode->m_dTrafficGen);
				}
			}
		}
		else //only 1 conn per node
		{
			if (m_pNetMan->m_hWDMNet.isAlreadySource(nSrc, m_pNetMan->getConnectionDB().m_hConList))
			{
#ifdef DEBUGB
				cout << "ECCOLO! Macro Cell " << nSrc << " gia' source. Scelgo una delle sue small cell" << endl;
				//cin.get();
#endif // DEBUGB
				//-B: get only small cells not having any connection
				m_pNetMan->m_hWDMNet.extractPotentialSourcesSC_1conn(potentialSources,
					this->m_pNetMan->getConnectionDB().m_hConList, nSrc);

#ifdef DEBUGB
				cout << "Le small cell disponibili sono " << potentialSources.size() << endl;
#endif // DEBUGB
				assert(potentialSources.size() > 0); //-B: otherwise this macro cell should not have been chosen as source

													 //-B: generate random number (between 1 and potentialSources.size()) with Mersenne Twister Engine
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(1, potentialSources.size());
				r = dis(gen);
				nSrc = potentialSources[r - 1];
				pNode = (OXCNode*)m_pNetMan->m_hWDMNet.lookUpNodeById(nSrc);
			}
		}

		//-B: ------------ SELECT DESTINATION && ASSIGN FRONTHAUL CONNECTION BANDWIDTH -------------
		int mobile = -1;
		//-B: if it is a mobile connection -> destination is the best BBU_hotel
		if (m_pNetMan->m_hWDMNet.isMobileNode(nSrc)) //-B: mobile request
		{
			assert(m_pNetMan->m_hWDMNet.isMobileNode(nSrc));
			fronthaulFlag = true;
			OXCNode*pNode = (OXCNode*)m_pNetMan->m_hWDMNet.lookUpNodeById(nSrc);
			if (pNode->isMacroCell())
			{
				eBandwidth = bwdMC;
				if (MIDHAUL)
				{
					CPRIBwd = genMidhaulBwd(bwdMC);
				}
				else //FRONTHAUL
				{
					CPRIBwd = OCcpriMC;
				}
			}
			else if (pNode->isSmallCell())
			{
				eBandwidth = (BandwidthGranularity)bwdSC;
				if (MIDHAUL)
				{
					CPRIBwd = genMidhaulBwd((BandwidthGranularity)bwdSC);
				}
				else //FRONTHAUL
				{
					CPRIBwd = OCcpriSC;
				}
			}
			connType = Connection::MOBILE_FRONTHAUL;
#ifdef DEBUGB
			cout << "MOBILE FRONTHAUL\n\n";
#endif // DEBUGB
			
			//***********************************************************************
			//--------------------------- IMPORTANT ---------------------------------
			//	the implementation assumes that network consists either of only macro cells
			//	OR of macro cells + small cells (hetnet)
			//***********************************************************************
			if (BBUSTACKING == true && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == false)
			{
				nDst = m_pNetMan->findBestBBUHotel(nSrc, CPRIBwd, pEvent->m_hTime);
			}
			else if (BBUSTACKING == false && INTRA_BBUPOOLING == true && INTER_BBUPOOLING == false)
			{
				nDst = m_pNetMan->findBestBBUPool_Soft(nSrc, CPRIBwd); //-B: NOT IMPLEMENTED YET
			}
			else if (BBUSTACKING == false && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == true)
			{
				nDst = m_pNetMan->findBestBBUPool_Evolved(nSrc, CPRIBwd, eBandwidth);
			}
			else
			{
				assert(false);
			}
		}
		//	if source is a fixed connection -> destination is directly the PoP node
		else if (m_pNetMan->m_hWDMNet.isFixedNode(nSrc)) //-B: fixed request
		{
			nDst = m_pNetMan->m_hWDMNet.DummyNode; //la destinazione è sempre il Core CO
			eBandwidth = (BandwidthGranularity)this->genFixedBwd(); //FIXED_TRAFFIC;
			CPRIBwd = OC0;
			connType = Connection::FIXED_BACKHAUL;
#ifdef DEBUGB
			cout << "FIXED BACKHAUL\n\n";
#endif // DEBUGB
		}
		else	//-B: fixed-mobile request
		{
#ifdef DEBUGB
			cout << "FIXED-MOBILE NODE: -> ";
#endif // DEBUGB
			//mobile = rand() % 1;
			//-B: generate random number (between 0 and 1) with Mersenne Twister Engine
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(0, 1);
			mobile = dis(gen);
#ifdef DEBUGB
			cout << "mobile = " << mobile << " ";
#endif // DEBUGB

			if (mobile == 0) //-B: fixed request
			{
				nDst = m_pNetMan->m_hWDMNet.DummyNode; //la destinazione è sempre il Core CO
				eBandwidth = (BandwidthGranularity)this->genFixedBwd();  //FIXED_TRAFFIC;
				CPRIBwd = OC0;
				connType = Connection::FIXED_BACKHAUL;
#ifdef DEBUGB
				cout << "FIXED BACKHAUL\n\n";
#endif // DEBUGB
			}
			//-B: if it is a mobile connection -> destination is the best BBU_hotel
			else //-B: mobile request
			{
				assert(mobile == 1);
				fronthaulFlag = true;
				//-B: ASSUMING A FIXED-MOBILE NODE IS ALWAYS A MACRO-CELL
				eBandwidth = bwdMC;
				if (MIDHAUL)
				{
					CPRIBwd = genMidhaulBwd(bwdMC);
				}
				else //FRONTHAUL
				{
					CPRIBwd = OCcpriMC;
				}
				connType = Connection::FIXEDMOBILE_FRONTHAUL;
#ifdef DEBUGB
				cout << "FIXED-MOBILE FRONTHAUL\n\n";
#endif // DEBUGB
				//-B: DISTINCTION BETWEEN BBU STACKING OR POOLING: TO BE DONE!!!!!!!!!!
				nDst = m_pNetMan->findBestBBUHotel(nSrc, CPRIBwd, pEvent->m_hTime);
			}
		}
	} //END fronthaulEvent NULL
	else   //-B: if fronthaul is not NULL, it has been already routed -> backhaul event
	{
		//assert(isBBUNode(pEvent->fronthaulEvent->m_pConnection->m_nDst)); //-B: COMMENTED if we allow a node hosts its own BBU in the cell site as last extreme option

		//-B: SELECT SOURCE AND DESTINATION
		nSrc = pEvent->fronthaulEvent->m_pConnection->m_nDst;	// source = original connection's source node
		nDst = m_pNetMan->m_hWDMNet.DummyNode;					// destination = PoP node
	
		//-B: ASSIGN BACKHAUL CONNECTION BANDWIDTH
		eBandwidth = pEvent->fronthaulEvent->m_pConnection->m_eBandwidth;
		CPRIBwd = pEvent->fronthaulEvent->m_pConnection->m_eCPRIBandwidth;

		//-B: assign original holding time
		holdingTime = pEvent->fronthaulEvent->m_pConnection->m_dHoldingTime;

		//-B: if original surce was a mobile node
		if (m_pNetMan->m_hWDMNet.isMobileNode(pEvent->fronthaulEvent->m_pConnection->m_nSrc))
		{
			connType = Connection::MOBILE_BACKHAUL;
#ifdef DEBUGB
			cout << "MOBILE BACKHAUL\n";
#endif // DEBUGB
		}
		//-B: if it was fixed-mobile node
		else if (m_pNetMan->m_hWDMNet.isFixMobNode(pEvent->fronthaulEvent->m_pConnection->m_nSrc))
		{
			connType = Connection::FIXEDMOBILE_BACKHAUL;
#ifdef DEBUGB
			cout << "FIXED-MOBILE BACKHAUL\n";
#endif // DEBUGB
		}
		//-B: if it was a fixed node -> impossible!
		else
		{
			//-B: a fixed node cannot have a pointer to a valid event in reference pEvent->fronthaulEvent
			assert(false);
			/*
			//-B: fixed request
			eBandwidth = FIXED_TRAFFIC;
			CPRIBwd = OC0;
			cout << "Fixed backhaul: ";
			*/
		}

#ifdef DEBUGB
		//cout << "\tFronthaul già instradato nella connection: " << pEvent->fronthaulEvent->m_pConnection->m_nSequenceNo;
		//cout << " - " << pEvent->fronthaulEvent->m_pConnection->m_nSrc << "->";
		//cout << pEvent->fronthaulEvent->m_pConnection->m_nDst << endl;
#endif // DEBUGB

	}// end ELSE fronthaul != NULL
	
	assert(0 <= nSrc && nSrc <= m_nNumberOfOXCNodes);
	//-B: DO NOT ASSERT (0 <= nDst && nDst <= m_nNumberOfOXCNodes) 
	//	BECAUSE FOR FRONTHAUL CONNECTIONS, IT COULD BE nDst == NULL, WHEN NO VALID (REACHABLE) BBU HOTEL NODE WAS FOUND

	//restore node reachability cost
	//this->m_pNetMan->m_hWDMNet.restoreNodeCost();
	//this->m_pNetMan->m_hWDMNet.resetBBUsReachabilityCost();
	this->m_pNetMan->m_hWDMNet.resetPreProcessing();


	// generate bandwidth distribution
	//  int nBWRange = 
	//      (int)((rand() / (RAND_MAX + 1.0)) * g_pBWSummation[m_eBWDistribution]) + 1;
	//  UINT *pBWD = g_ppBWD[m_eBWDistribution];
	//  if (nBWRange <= pBWD[0])
	//      eBandwidth = OC1;
	//  else if (nBWRange <= (pBWD[0] + pBWD[1]))
	//      eBandwidth = OC3;
	//  else if (nBWRange <= (pBWD[0] + pBWD[1] + pBWD[2]))
	//      eBandwidth = OC12;
	//  else if (nBWRange <= (pBWD[0] + pBWD[1] + pBWD[2] + pBWD[3]))
	//      eBandwidth = OC48;
	//  else if (nBWRange <= (pBWD[0] + pBWD[1] + pBWD[2] + pBWD[3] + pBWD[4]))
	//      eBandwidth = OC192;NS_OCH::Connection
	//  else {
	//      assert(false);
	//  }

	//-B: ******************* CREATE CONNECTION **********************
	//-B: perchè viene passato g_nConSeqNo++ come num di seq della connessione???
	//	modifica: incremento messo a fine metodo -> commentato! Non serve a niente incrementarlo
	//	dato che ogni volta che viene chiamato questo metodo la var viene inizializzata a 1
	//	e utilizzata solo una volta nel costruttore Connection, prima dell'incremento
	Connection *pConnection;

	//-B: in case of backhaul event, holding time was already assigned to corresponding fronthaul event
	if (holdingTime < 0)
	{
		holdingTime = EventList::expHoldingTime(m_dMeanHoldingTime);
	}
	//else: it has been already assigned, taken from the corresponding fronthaul connection

	pConnection = new Connection(g_nConSeqNo, nSrc, nDst, //LEAK
			pEvent->m_hTime, holdingTime,
			eBandwidth, CPRIBwd, m_ePClass, connType);

	// generate hop count 5: 6: 7: inf = 10: 10: 20: 60;
	// static int g_pHopCount[4] = {1, 2, 4, 10};
	// generate hop count 5: 6: 7: inf = 30: 20: 10: 40;
	static int g_pHopCount[4] = { 3, 5, 6, 10 };
	UINT nHopCount;
	//-B: il metodo initialize della classe Simulator inizializza la var m_nHopCount = 30 che è il num max di hops
	//	(val dato da linea di comando), non il val di hop per qualsiasi connessione
	//	perciò in questo if si entra solo se da linea di comando metto 0 -> in tal modo il max num di hop se lo calcola da solo
	if (0 == m_nHopCount) {
		int nIndex = (int)((rand() / (RAND_MAX + 1.0)) * 10) + 1;
		if (nIndex <= g_pHopCount[0]) {
			nHopCount = 5;
		}
		else if (nIndex <= g_pHopCount[1]) {
			nHopCount = 6;
		}
		else if (nIndex <= g_pHopCount[2]) {
			nHopCount = 7;
		}
		else if (nIndex <= g_pHopCount[3]) {
			nHopCount = (UINT)(UNREACHABLE - 1);
		}
		else {
			assert(false);
		}
	}
	else {
		nHopCount = m_nHopCount;
	}
	pConnection->m_nHopCount = nHopCount;
	
	g_nConSeqNo++;

	return pConnection;
}

//-B: originally taken from BBU_newConnection
//-B: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//	THIS FUNCTION CAN BE USED ONLY WITH SOME CONDITIONS:
//	topology with only macro cells; mobile connection requests only; Bernoulli traffic generation model;
//-B: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Connection* Simulator::BBU_newConnection_Bernoulli(Event*pEvent, int runningPhase)
{
#ifdef DEBUG
	cout << "-> BBU_NewConnection_Bernoulli: " << endl;
#endif // DEBUGB

	static UINT g_nConSeqNo = 1;
	/* -B: commented
	static UINT g_ppBWD[][5] = {
	{192, 64, 16, 4, 1},        // Zipf
	{10000, 2000, 200, 20, 2},  // SBC
	{300, 20, 6, 4, 1},         // Sprint 2003
	{1, 1, 1, 1, 1},            // uniform
	{0, 0, 0, 1, 0},            // for debug, everything OC48
	{0, 0, 0, 0, 1},            // for debug, everything OC19k
	};
	static UINT g_pBWSummation[] = { 277, 12222, 331, 5, 1, 1 };
	*/

	//-B: ------------- SELECT SOURCE AND DESTINATION NODE && ASSIGN CONNECTION'S BANDWIDTHS -----------------------
	UINT nDst;
	UINT nSrc;
	BandwidthGranularity eBandwidth;
	BandwidthGranularity CPRIBwd;
	Connection::ConnectionType connType;
	bool fronthaulFlag = false;
	SimulationTime holdingTime = -1; // -L: ???
	bool office = true; // -L: ???

	//-B: precomputedPath cleared, from previous cycle calculation, each time the cycle calls BBU_newConnection method
	m_pNetMan->clearPrecomputedPath();
	//-L:  significa che è un fronthaulEvent
	//-B: if fronthaul has not been routed yet
	if (pEvent->fronthaulEvent == NULL)
	{

		assert(pEvent->m_pSource != NULL);

		//-B: ASSIGN SOURCE
		nSrc = pEvent->m_pSource->getId();
		int numOfFreeSources = (pEvent->m_pSource->getNumberOfSources() - pEvent->m_pSource->getNumberOfBusySources());
		assert(numOfFreeSources > 0);

#ifdef DEBUGX
		list<AbstractNode*>::const_iterator iter;
		OXCNode*pOXCNode;
		for (iter = this->m_pNetMan->m_hWDMNet.m_hNodeList.begin(); iter != this->m_pNetMan->m_hWDMNet.m_hNodeList.end(); iter++)
		{
			pOXCNode = (OXCNode*)(*iter);
			MappedLinkList<UINT, Connection*> conList = this->m_pNetMan->getConnectionDB().m_hConList;

			list<Connection*>::const_iterator itr;
			Connection* pConDB;
			UINT numConnection = 0;

			for (itr = conList.begin(); itr != conList.end(); itr++) {
				pConDB = (Connection*)(*itr);
				if (pConDB->m_nSrc == pOXCNode->getId()) {
					numConnection++;
				}
			}
			if (numConnection > 0){

				cout << "\tNumber of connections at node " << pOXCNode->getId() << ": " << numConnection << endl;
			
			}
		}
#endif

		//-B: take into account small cell extra latency due to "hidden" path between SC and MC (then used in DijkstraLatencyHelper)
		if (pEvent->m_pSource->m_dTrafficGen > MAXTRAFFIC_MACROCELL) //small cell
			pEvent->m_pSource->m_dExtraLatency = PROPAGATIONLATENCY * DISTANCE_SC_MC; //-B: another component of ELSWITCHLATENCY should be added but we have an already sctricter than normal latency
		else //macro cell
			pEvent->m_pSource->m_dExtraLatency = 0;
		
		//-B: ASSIGN BACKHAUL BANDWIDTH
		eBandwidth = BWDGRANULARITY;

		//-L: ????????
		//-B: ASSIGN X-HAUL BANDWIDTH
		if (MIDHAUL)
		{
			CPRIBwd = genMidhaulBwd(eBandwidth);
		}
		else //FRONTHAUL
		{
			CPRIBwd = OCcpriMC;
		}

		//-B: ASSIGN CONNECTION TYPE
		connType = Connection::MOBILE_FRONTHAUL;
#ifdef DEBUG
		cout << "MOBILE FRONTHAUL\n";
#endif // DEBUGB
		
		//-B: ASSIGN DESTINATION --> FIND BEST BBU HOTEL NODE
		//-B: ONLY BBUSTACKING IS ACCEPTED (RRH-BBU --> 1:1 ASSOCIATION)
		if (BBUSTACKING == true && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == false)
		{
			nDst = m_pNetMan->findBestBBUHotel(nSrc, CPRIBwd, pEvent->m_hTime);

			if (pEvent->m_pSource->m_nBBUNodeIdsAssignedLast > 0 && pEvent->m_pSource->m_nBBUNodeIdsAssigned > 0) {
				updateBBUforallConnections(nDst, nSrc, pEvent);
			}
		}
		else
		{
			assert(false);
		}

	} //END fronthaulEvent NULL
	
	// -B: if fronthaul is not NULL, it has been already routed -> backhaul or midhaul event
	// -L: if midhaul is NULL, midhaul is not been routed yet, so it's a midhaul 
	else if (pEvent->fronthaulEvent != NULL && pEvent->midhaulEvent == NULL) //-L: significa che è midhaul   
	{
		//assert(isBBUNode(pEvent->fronthaulEvent->m_pConnection->m_nDst)); //-B: COMMENTED if we allow a node hosts its own BBU in the cell site as last extreme option

		//-B: SELECT SOURCE AND DESTINATION
		nSrc = pEvent->fronthaulEvent->m_pConnection->m_nDst;	// source = original connection's source node
		nDst = m_pNetMan->m_hWDMNet.DummyNodeMid;					// destination = core CO/PoP node

		//-B: ASSIGN BACKHAUL CONNECTION BANDWIDTH
		eBandwidth = pEvent->fronthaulEvent->m_pConnection->m_eBandwidth;
		CPRIBwd = pEvent->fronthaulEvent->m_pConnection->m_eCPRIBandwidth;

		//-B: assign original holding time
		holdingTime = pEvent->fronthaulEvent->m_pConnection->m_dHoldingTime;

		//-B: original source had to be a mobile node
		connType = Connection::FIXED_MIDHAUL;
#ifdef DEBUG
		cout << "MOBILE BACKHAUL\n";
#endif // DEBUGB

#ifdef DEBUG
		//cout << "\tFronthaul già instradato nella connection: " << pEvent->fronthaulEvent->m_pConnection->m_nSequenceNo;
		//cout << " - " << pEvent->fronthaulEvent->m_pConnection->m_nSrc << "->";
		//cout << pEvent->fronthaulEvent->m_pConnection->m_nDst << endl;
#endif // DEBUGB

	}// end ELSE IF fronthaul != NULL
	
	 // -L: signfica che è backhaul
	else if (pEvent->fronthaulEvent != NULL && pEvent->midhaulEvent != NULL) {
		//-B: SELECT SOURCE AND DESTINATION
		nSrc = pEvent->midhaulEvent->m_pConnection->m_nDst;	// source = original connection's source node
		nDst = m_pNetMan->m_hWDMNet.DummyNode;					// destination = core CO/PoP node //-L: da modificare

																//-B: ASSIGN BACKHAUL CONNECTION BANDWIDTH
		eBandwidth = pEvent->midhaulEvent->m_pConnection->m_eBandwidth;
		CPRIBwd = pEvent->midhaulEvent->m_pConnection->m_eCPRIBandwidth;

		//-B: assign original holding time
		holdingTime = pEvent->midhaulEvent->m_pConnection->m_dHoldingTime;

		//-B: original source had to be a mobile node
		connType = Connection::MOBILE_BACKHAUL;
	} // -L: fine backhaul
	
	assert(0 <= nSrc && nSrc <= m_nNumberOfOXCNodes);
	//-B: DO NOT ASSERT (0 <= nDst && nDst <= m_nNumberOfOXCNodes) 
	//	BECAUSE FOR FRONTHAUL CONNECTIONS, IT COULD BE nDst == NULL, WHEN NO VALID (REACHABLE) BBU HOTEL NODE WAS FOUND

	//restore node reachability cost and other var
	this->m_pNetMan->m_hWDMNet.resetPreProcessing();


	//-B: ******************* CREATE CONNECTION **********************
	//-B: perchè viene passato g_nConSeqNo++ come num di seq della connessione???
	//	modifica: incremento messo a fine metodo -> commentato! Non serve a niente incrementarlo
	//	dato che ogni volta che viene chiamato questo metodo la var viene inizializzata a 1
	//	e utilizzata solo una volta nel costruttore Connection, prima dell'incremento
	Connection *pConnection;

	//-B: in case of backhaul event, holding time was already assigned to corresponding fronthaul event
	if (holdingTime < 0)
	{
		holdingTime = EventList::expHoldingTime(m_dMeanHoldingTime);
	}
	//else: it has been already assigned, taken from the corresponding fronthaul connection
	
	//-L: if it is backhaul
	if (pEvent->fronthaulEvent != NULL && pEvent->midhaulEvent != NULL) {  
		if (pEvent->fronthaulEvent->m_pConnection->m_nBackhaulSaved > 0) {
			UINT seqNumber = pEvent->fronthaulEvent->m_pConnection->m_nBackhaulSaved;
#ifdef DEBUGC
			cout << "Sequence number saved for the backhaul in the fronthaul is "
				<< seqNumber << endl;
#endif
			pConnection = new Connection(seqNumber, nSrc, nDst, //LEAK
				pEvent->m_hTime, holdingTime,
				eBandwidth, CPRIBwd, m_ePClass, connType);

			//-B: not useful for me
			pConnection->m_nHopCount = m_nHopCount;

			//Sequence number has not to be increased!

			return pConnection;

		}
	
	}

	//-L: if it is midhaul
	if (pEvent->fronthaulEvent != NULL && pEvent->midhaulEvent == NULL) {
		if (pEvent->fronthaulEvent->m_pConnection->m_nMidhaulSaved > 0) {
			UINT seqNumber = pEvent->fronthaulEvent->m_pConnection->m_nMidhaulSaved;
#ifdef DEBUGC
			cout << "Sequence number saved for the midhaul in the fronthaul is "
				<< seqNumber << endl;
#endif
			pConnection = new Connection(seqNumber, nSrc, nDst, //LEAK
				pEvent->m_hTime, holdingTime,
				eBandwidth, CPRIBwd, m_ePClass, connType);

			//-B: not useful for me
			pConnection->m_nHopCount = m_nHopCount;

			//Sequence number has not to be increased!

			return pConnection;

		}
	}

	//-L: if it is fronthaul
	pConnection = new Connection(g_nConSeqNo, nSrc, nDst, //LEAK
		pEvent->m_hTime, holdingTime,
		eBandwidth, CPRIBwd, m_ePClass, connType);

	//-B: not useful for me
	pConnection->m_nHopCount = m_nHopCount;

	g_nConSeqNo++;

	return pConnection;
}


UINT Simulator::genMobileBwdMCell()
{
	UINT firstStep = OC6;
	UINT lastStep = OC15;
	vector<UINT> bwd;
	UINT numToInsert;
	UINT mobileBwd = OC0;
	int index = -1;

	bwd.clear();

	for (numToInsert = firstStep; numToInsert <= lastStep; numToInsert += BWDGRANULARITY)
	{
		assert(numToInsert >= firstStep && numToInsert <= lastStep);
		bwd.push_back(numToInsert);
	}

	//-B: generate random number (between 0 and bwd.size()) with Mersenne Twister Engine
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, (bwd.size() - 1));
	index = dis(gen);

	mobileBwd = bwd[index];

	return mobileBwd;
}

UINT Simulator::genMobileBwdSCell()
{
	UINT firstStep = OC2;
	UINT lastStep = OC5;
	vector<UINT> bwd;
	UINT numToInsert;
	UINT mobileBwd = OC0;
	int index = -1;

	bwd.clear();

	for (numToInsert = firstStep; numToInsert <= lastStep; numToInsert++)
	{
		assert(numToInsert >= firstStep && numToInsert <= lastStep);
		bwd.push_back(numToInsert);
	}

	//-B: generate random number (between 0 and bwd.size()) with Mersenne Twister Engine
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, (bwd.size() - 1));
	index = dis(gen);

	mobileBwd = bwd[index];

	return mobileBwd;
}

UINT Simulator::genFixedBwd()
{
	UINT firstStep = OC192;
	UINT lastStep = OC384;
	vector<UINT> bwd;
	UINT numToInsert;
	UINT mobileBwd = OC0;
	int index = -1;

	bwd.clear();

	for (numToInsert = firstStep; numToInsert <= lastStep; numToInsert += BWDGRANULARITY)
	{
		assert(numToInsert >= firstStep && numToInsert <= lastStep);
		bwd.push_back(numToInsert);
	}

	//-B: generate random number (between 0 and bwd.size()) with Mersenne Twister Engine
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, (bwd.size() - 1));
	index = dis(gen);

	mobileBwd = bwd[index];

	return mobileBwd;
}

void Simulator::saveStats(int&n, Log&m_hLog)
{
	cout << " -> SALVATAGGIO PERIODICO PARAMETRI DI hLog" << endl;
	UINT ind = (n - 1) % 24; //-B: INSTEAD OF: ind = (start_time + m - 1) % 24; WITH start_time = 0
	cout << "index = " << ind << endl;

	//-B: save blocking probability hour by hour
	m_hLog.Pblock_hour[ind] = ((double)m_hLog.m_nBlockedCon
		/ ((double)m_hLog.m_nBlockedCon + (double)m_hLog.m_nProvisionedCon));
	cout << "PBlock_hour = " << m_hLog.Pblock_hour[ind] << endl;

	m_hLog.PblockFront_hour[ind] = ((double)m_hLog.m_nBlockedMobileFrontConn + (double)m_hLog.m_nBlockedFixMobFrontConn)
		/ ((double)m_hLog.m_nBlockedMobileFrontConn + (double)m_hLog.m_nBlockedFixMobFrontConn
			+ (double)m_hLog.m_nProvisionedMobileFrontConn + (double)m_hLog.m_nProvisionedFixMobFrontConn);

	m_hLog.PblockBack_hour[ind] = ((double)m_hLog.m_nBlockedFixedBackConn + (double)m_hLog.m_nBlockedMobileBackConn + (double)m_hLog.m_nBlockedFixMobBackConn)
		/ ((double)m_hLog.m_nBlockedFixedBackConn + (double)m_hLog.m_nBlockedMobileBackConn + (double)m_hLog.m_nBlockedFixMobBackConn
			+ (double)m_hLog.m_nProvisionedFixedBackConn + (double)m_hLog.m_nProvisionedMobileBackConn + (double)m_hLog.m_nProvisionedFixMobBackConn);

	m_hLog.PblockBW_hour[ind] = (double)m_hLog.m_nBlockedBW / ((double)m_hLog.m_nBlockedBW + (double)m_hLog.m_nProvisionedBW);

	m_hLog.PblockFrontBW_hour[ind] = ((double)m_hLog.m_nBlockedMobileFrontBW + (double)m_hLog.m_nBlockedFixMobFrontBW)
		/ ((double)m_hLog.m_nBlockedMobileFrontBW + (double)m_hLog.m_nBlockedFixMobFrontBW
			+ (double)m_hLog.m_nProvisionedFixMobFrontBW + (double)m_hLog.m_nProvisionedMobileFrontBW);

	m_hLog.PblockBackBW_hour[ind] = ((double)m_hLog.m_nBlockedFixedBackBW + (double)m_hLog.m_nBlockedMobileBackBW + (double)m_hLog.m_nBlockedFixMobBackBW)
		/ ((double)m_hLog.m_nBlockedFixedBackBW + (double)m_hLog.m_nBlockedMobileBackBW + (double)m_hLog.m_nBlockedFixMobBackBW
			+ (double)m_hLog.m_nProvisionedFixedBackBW + (double)m_hLog.m_nProvisionedFixMobBackBW + (double)m_hLog.m_nProvisionedMobileBackBW);

	//-B: UPDATE NETWORK COST
	//m_pNetMan->m_hLog.NetCost_hour[ind] = m_pNetMan->m_runLog.networkCost / m_pNetMan->m_runLog.m_hSimTimeSpan; //commented because we take hLog o runLog as input parameter
	m_hLog.Avg_NetCost_hour[ind] = m_hLog.networkCost / (m_hLog.m_hSimTimeSpan - m_hLog.transitoryTime);

	//-B: save average latency of fronthaul connections
	m_hLog.AvgLatency_hour[ind] = m_hLog.avgLatency / m_hLog.countConnForLatency;

	//-B: save load, even if I don't know if I need it
	m_hLog.carico[ind] = m_pNetMan->carico_rete;

	//-B: for each fiber and for each hour, it save the corresponding load in terms of: (channel used*time) / (tot channels * time)
	list<AbstractLink*>::const_iterator itr;
	UniFiber*pUniFiber;
	vector<double> UniFiberLoad;
	double value;
	//m_pNetMan->dump(cout);
	for (itr = m_pNetMan->m_hWDMNet.m_hLinkList.begin(); itr != m_pNetMan->m_hWDMNet.m_hLinkList.end(); itr++)
	{
		pUniFiber = (UniFiber*)(*itr);
		value = pUniFiber->m_dLinkLoad / ((double)pUniFiber->m_nW * m_hLog.m_hSimTimeSpan); //m_pNetMan->m_hLog.m_hSimTimeSpan SHOULD BE = pEvent->m_hTime
		UniFiberLoad.push_back(value); //-B: li avrò quindi dall'ultimo link fino al primo (decrescente diciamo)
#ifdef DEBUGB
		cout << "FIBER #" << pUniFiber->getId() << " LOAD = " << pUniFiber->m_dLinkLoad << "/ (" 
			<< (double)pUniFiber->m_nW << "*" << m_hLog.m_hSimTimeSpan << ") = " << value << endl;
		//cin.get();
#endif // DEBUGB
		//-B: reset values to do periodic measurements
		pUniFiber->m_dLinkLoad = 0;
	}
	m_hLog.UniFiberLoad_hour.insert(pair<UINT, vector<double> >(ind, (UniFiberLoad)));

	//cin.get();
	cout << "..." << endl;

	//INCREASE VALUE OF m FOR NEXT INDEX
	n++;
}

UINT Simulator::genSource(vector<OXCNode*>&nodesList)
{
	UINT src, n;

	do
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, (nodesList.size() - 1));
		n = dis(gen);
		src = (nodesList[n])->getId();
	} while (src == m_pNetMan->m_hWDMNet.DummyNode);

	return src;
}

BandwidthGranularity Simulator::genMidhaulBwd(BandwidthGranularity bwd)
{
	UINT midhaulBwd = (ceil(bwd * MIDHAUL_FACTOR));
	return (BandwidthGranularity)midhaulBwd;
}

//-B: we have to delay the corresponding FRONTHAUL departure event of an already existing connection
void Simulator::updateCorrespondingDepEvent(Connection*pCon)
{
	m_hEventList.delayFronthaulEvent(pCon);
}

// -L: genera un nuovo evento di tipo ARRIVAL se ho risorse libere e lo inserisce nella lista hEventList
void Simulator::genNextPoissonEvent(OXCNode*pOXCNode, SimulationTime m_hTime)
{
	double arrRate = (pOXCNode->getNumberOfSources() - pOXCNode->getNumberOfBusySources());
	if (arrRate > 0)
	{
		double dNextArrival = this->m_hEventList.nextBernoulliArrival(this->m_dArrivalRate);
		m_hEventList.insertEvent(new Event(m_hTime + dNextArrival, Event::EVT_ARRIVAL, NULL, pOXCNode));

#ifdef DEBUG
		cout << "\tInserisco next POISSON arrival - NODE #" << pOXCNode->getId() << ": " << (m_hTime + dNextArrival) << endl;
		//cin.get();
#endif // DEBUGB
	}
	else
	{
#ifdef DEBUGB
		cout << "\tATTENZIONE! NODO #" << pOXCNode->getId() << " PIENO! Traffico generato = " << pOXCNode->m_dTrafficGen << endl;
#endif // DEBUGB
		this->m_hEventList.addBernoulliArrAfterDep(pOXCNode, m_dArrivalRate);
	}
}

//-B: attempt to compute, a priori, an adequate number of hotel nodes that should be active during the simulation
//	to hold the traffic of the network, given a certain arrival rate
void Simulator::computeNumOfActiveHotels()
{
#ifdef DEBUGB
	cout << "--> computeNumOfActiveHotels" << endl;
	//cin.get()
#endif // DEBUGB

	list<AbstractNode*>::const_iterator itr;

	vector<OXCNode*>::const_iterator itrN;

	//-B: each macro cell can host a certain number of connection (given its maximum capacity,
	//	its number of small cells, their maximum capacity and the connection bwd)
	UINT maxConn_MC = ceil((double)MAXTRAFFIC_MACROCELL / (double)BWDGRANULARITY) + ((double)SMALLCELLS_PER_MC * ((double)MAXTRAFFIC_SMALLCELL / (double)BWDGRANULARITY));

	//-B: for Bernoulli traffic --> compute a = lambda'/(lambda' + mu)
	double factor = m_dArrivalRate / (m_dArrivalRate + (1 / m_dMeanHoldingTime));
	cout << "Factor = " << factor << endl;


	//-B: for Bernoulli traffic: Ao = S*a
	//average number of connections per second per node
	UINT offeredTraffic_MC = ceil((double)maxConn_MC * factor); //-B: CHANGE: floor->ceil

	if (offeredTraffic_MC > maxConn_MC)
	{
		cout << "\tImpossible! Average number of connections per second per MC > max number of connections per second per MC";
		offeredTraffic_MC = maxConn_MC;
		// cin.get();
	}

#ifdef DEBUG
	cout << "\tOgni MC puo' avere massimo " << maxConn_MC << " connessioni" << endl;
	cout << "\tOgni MC ha in media " << offeredTraffic_MC << " connessioni in arrivo al secondo" << endl;
#endif // DEBUG

	//-B: for each candidate hotel node compute the maximum number of nodes
	//	whose x-haul traffic would be supported by it (hosting their BBU into it)
	if (MIDHAUL)
	{
		m_pNetMan->m_hWDMNet.computeNumOfBBUSupported_CONSERVATIVE(offeredTraffic_MC*BWDGRANULARITY*MIDHAUL_FACTOR);
		//m_pNetMan->m_hWDMNet.computeNumOfBBUSupported(offeredTraffic_MC*BWDGRANULARITY*MIDHAUL_FACTOR);
	}
	else //FRONTHAUL
	{
		m_pNetMan->m_hWDMNet.computeNumOfBBUSupported_CONSERVATIVE(OCcpriMC);
	}

	//-B: compute the average number of BBU supported per candidate hotel node
	UINT avgNumBBU = 0;
	double additionalHotel = 0;
	////////////////////////////////////////////////////
	if (!MIDHAUL)
	{
		cout << "offered traffic MC = " << offeredTraffic_MC << endl;
		double wavel_utilization = (double)OCcpriMC / (double)(m_pNetMan->m_hWDMNet.getChannelCapacity());
		cout << "wavel utilization = " << wavel_utilization << endl;
		double residual_wavel_cap = (double)m_pNetMan->m_hWDMNet.getChannelCapacity() - ((double)m_pNetMan->m_hWDMNet.getChannelCapacity()*wavel_utilization);
		cout << "residual wavel cap = " << residual_wavel_cap << endl;
		double MCs_per_wavel_BH = floor(residual_wavel_cap / ((double)offeredTraffic_MC*BWDGRANULARITY));
		cout << "MCs per wavel (BH) = " << MCs_per_wavel_BH << endl;
		double MCs_per_fiber_BH = MCs_per_wavel_BH * m_pNetMan->m_hWDMNet.numberOfChannels;
		double additionalBBU_BH = MCs_per_fiber_BH;
		double BBUSupported = floor(additionalBBU_BH / 2);
		cout << "BBUs supported = " << BBUSupported << endl;
		if (BBUSupported > 0)
		{
			additionalHotel++;
			while (additionalBBU_BH < m_pNetMan->m_hWDMNet.numberOfNodes)
			{
				additionalBBU_BH += BBUSupported;
				additionalHotel++;
			}
		}
		cout << "AdditionalHotels for BH = " << additionalHotel << endl;
	}
	//////////////////////////////////////////////////
	this->m_nNumOfActiveHotels = 0;
	int count = 0;
	for (itrN = m_pNetMan->m_hWDMNet.hotelsList.begin(); itrN != m_pNetMan->m_hWDMNet.hotelsList.end()
		&& count < m_pNetMan->m_hWDMNet.numberOfNodes; itrN++)
	{
		count += (*itrN)->m_uNumOfBBUSupported;
#ifdef DEBUGB
		cout << "\tIl nodo " << (*itrN)->getId() << " supporta " << (*itrN)->m_uNumOfBBUSupported << " BBUs" << endl;
		cout << "count = " << count << endl;
#endif // DEBUGB
		this->m_nNumOfActiveHotels++;
	}

	cout << "Number of active hotels : "<< this->m_nNumOfActiveHotels << endl;
	//cin.get();

	//-B: add a 20% or 50% to be conservative (grooming does not happen in each node, above all in 4G case study)
	if (ONLY_ACTIVE_GROOMING == false) //5G (midhaul)
		this->m_nNumOfActiveHotels = this->m_nNumOfActiveHotels + ceil((double)this->m_nNumOfActiveHotels * 20 / 100);
	else if (ONLY_ACTIVE_GROOMING == true && MIDHAUL == true) //4G + midhaul
		this->m_nNumOfActiveHotels = this->m_nNumOfActiveHotels + ceil((double)this->m_nNumOfActiveHotels * 50 / 100);
	else if (ONLY_ACTIVE_GROOMING == true && MIDHAUL == false) //4G + fronthaul
		this->m_nNumOfActiveHotels = this->m_nNumOfActiveHotels + ceil((double)this->m_nNumOfActiveHotels * 20 / 100);

	//-B: for fronthaul case additionalHotel will be > 0
	this->m_nNumOfActiveHotels += additionalHotel;
	//this->m_nNumOfActiveHotels = ceil((double)(double)m_pNetMan->m_hWDMNet.numberOfNodes / (double)avgNumBBU);
#ifdef DEBUGB
	//cout << endl << "\tIl numero medio di BBU supportate per hotel node e' " << avgNumBBU << endl;
	cout << " \t--> Il numero di hotel node attivi necessari e': " << this->m_nNumOfActiveHotels << endl;
#endif // DEBUGB

}


void Simulator::updateBBUforallConnections(UINT newBBU, UINT m_nSrc, Event* pEvent) {

	MappedLinkList<UINT, Connection*> conList = this->m_pNetMan->getConnectionDB().m_hConList;

	list<Connection*>::const_iterator itr;
	Connection* pConDB;
	UINT nDst = newBBU;

	for (itr = conList.begin(); itr != conList.end(); itr++) {
		pConDB = (Connection*)(*itr);
		if (pConDB->m_nSrc == m_nSrc) {


			//if the connection is a fronthaul, create the departure for the fronthaul and the backhaul 
			if (pConDB->m_eConnType == Connection::MOBILE_FRONTHAUL || pConDB->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL) {

				Connection* pConNew = BBU_newConnection_Bernoulli_NewBBU(pConDB, newBBU, pEvent);
				m_pNetMan->connectionsChangingBBU.push_back(pConNew);
				

				//Traffic has not to be updated (it was already updated when this connection
				//arrived at the old bbu)
				pConNew->m_bTrafficToBeUpdatedForArrival = false;

				//departure for the fronthaul
				pConDB->m_bTrafficToBeUpdatedForDep = false;
				Event* nEvDepFr = new Event(pEvent->m_hTime, Event::EVT_DEPARTURE, pConDB);
				nEvDepFr->arrTimeAs = pEvent->m_hTime;
				m_hEventList.insertEvent(nEvDepFr);
#ifdef DEBUGC

				cout << "\t\t+Created event departure (fronthaul)" << endl;
#endif
				//find the backhaul coresponding to the fronthaul 
				UINT pConBackhaulId = backhaul_id[pConDB->m_nSequenceNo];
#ifdef DEBUGC
				cout << "\t\t+Backhaul to be departed has id: " << pConBackhaulId << endl;
#endif
				list<Connection*>::const_iterator itr2;
				Connection* pCon2DB;
				for (itr2 = conList.begin(); itr2 != conList.end(); itr2++) {
					pCon2DB = (Connection*)(*itr2);
					if (pCon2DB->m_nSequenceNo == pConBackhaulId) {

						//departure for the backhaul
						Event* nEvDepBack = new Event(pEvent->m_hTime, Event::EVT_DEPARTURE, pCon2DB);
						nEvDepBack->arrTimeAs = pEvent->m_hTime;
						m_hEventList.insertEvent(nEvDepBack);
#ifdef DEBUGC

						cout << "\t\t+Created event departure (backhaul)" << endl;
#endif
						break;
					}
				}

				m_hEventList.insertEvent(new Event(pEvent->m_hTime, Event::EVT_ARRIVAL, pConNew));
				m_pNetMan->m_hLog.m_nNumConnectionsChangingBBU++;
#ifdef DEBUG

				cout << "\t\t+Arrival created" << endl;
#endif
			}
			else {
#ifdef DEBUGC
				cout << "E' un backhaul che va da " << pConDB->m_nSrc <<
					" a " << pConDB->m_nDst << " con id " << pConDB->m_nSequenceNo <<
					" ma in teoria non dovrei fare nulla..." << endl;
				//cin.get();
#endif
			}
		}

	}
}

Connection* Simulator::BBU_newConnection_Bernoulli_NewBBU(Connection* pConTobeSwitched, int newBBU, Event* pEvent){
	//-B: ------------- SELECT SOURCE AND DESTINATION NODE && ASSIGN CONNECTION'S BANDWIDTHS -----------------------
	UINT nDst;
	UINT nSrc;
	BandwidthGranularity eBandwidth;
	BandwidthGranularity CPRIBwd;
	Connection::ConnectionType connType;
	SimulationTime remainingHoldingTime = -1;
	list <AbstractLink*> pathToSave;
	LINK_COST costToSave = UNREACHABLE;

#ifdef DEBUG
	cout << "\tI'm creating the new fronthaul connection with id: " << pConTobeSwitched->m_nSequenceNo << endl;
#endif
	//-B: ASSIGN SOURCE
	nSrc = pConTobeSwitched->m_nSrc;	//-B: ASSIGN SOURCE

	OXCNode* pOXCSrc = (OXCNode*)(this->m_pNetMan->m_hWDMNet.lookUpNodeById(pConTobeSwitched->m_nSrc));

	int numOfFreeSources = (pOXCSrc->getNumberOfSources() - pOXCSrc->getNumberOfBusySources());
	assert(numOfFreeSources > 0);

	//-B: take into account small cell extra latency due to "hidden" path between SC and MC (then used in DijkstraLatencyHelper)
	if (pOXCSrc->m_dTrafficGen > MAXTRAFFIC_MACROCELL) //small cell
		pOXCSrc->m_dExtraLatency = PROPAGATIONLATENCY * DISTANCE_SC_MC; //-B: another component of ELSWITCHLATENCY should be added but we have an already sctricter than normal latency
	else //macro cell
		pOXCSrc->m_dExtraLatency = 0;

	//-B: ASSIGN BACKHAUL BANDWIDTH
	eBandwidth = BWDGRANULARITY;

	//-B: ASSIGN X-HAUL BANDWIDTH
	if (MIDHAUL)
	{
		CPRIBwd = genMidhaulBwd(eBandwidth);
	}
	else //FRONTHAUL
	{
		CPRIBwd = OCcpriMC;
	}

	//-B: ASSIGN CONNECTION TYPE
	connType = pConTobeSwitched->m_eConnType;

	//-B: ASSIGN DESTINATION --> FIND BEST BBU HOTEL NODE
	//-B: ONLY BBUSTACKING IS ACCEPTED (RRH-BBU --> 1:1 ASSOCIATION)
	if (BBUSTACKING == true && INTRA_BBUPOOLING == false && INTER_BBUPOOLING == false)
	{
		nDst = newBBU;

	}
	else
	{
		assert(false);
	}

	remainingHoldingTime = (pConTobeSwitched->m_dHoldingTime + pConTobeSwitched->m_dRoutingTime) - (pEvent->m_hTime - pConTobeSwitched->m_dArrivalTime);

	//} //END fronthaulEvent NULL

	assert(0 <= nSrc && nSrc <= m_nNumberOfOXCNodes);
	//-B: DO NOT ASSERT (0 <= nDst && nDst <= m_nNumberOfOXCNodes) 
	//	BECAUSE FOR FRONTHAUL CONNECTIONS, IT COULD BE nDst == NULL, WHEN NO VALID (REACHABLE) BBU HOTEL NODE WAS FOUND

	//restore node reachability cost and other var
	this->m_pNetMan->m_hWDMNet.resetPreProcessing();


	//-B: ******************* CREATE CONNECTION **********************
	//-B: perchè viene passato g_nConSeqNo++ come num di seq della connessione???
	//	modifica: incremento messo a fine metodo -> commentato! Non serve a niente incrementarlo
	//	dato che ogni volta che viene chiamato questo metodo la var viene inizializzata a 1
	//	e utilizzata solo una volta nel costruttore Connection, prima dell'incremento
	Connection *pConnectionNew;


	//else: it has been already assigned, taken from the corresponding fronthaul connection

	UINT connNumber = pConTobeSwitched->m_nSequenceNo;

	pConnectionNew = new Connection(connNumber, nSrc, nDst, //LEAK
		pEvent->m_hTime, remainingHoldingTime,
		eBandwidth, CPRIBwd, m_ePClass, connType);

	pConnectionNew->m_bTrafficToBeUpdatedForDep = true;

	//-B: not useful for me
	pConnectionNew->m_nHopCount = m_nHopCount;

	return pConnectionNew;

}

