// Massimo`s modifications are highlighted by means of //M

#pragma warning(disable: 4786)
#define _OCH_JIT_DEBUG
#include <assert.h>
#include <time.h>
#include <list> //-M
#include<iostream>
#include<fstream>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "BinaryHeap.h"
#include "AbstractPath.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "OXCNode.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "TopoReader.h"
#include "Connection.h"
#include "ConnectionDB.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "Event.h"
#include "EventList.h"
#include "Log.h"
#include "Simulator.h"
#include "Lightpath.h"
#include "LightpathDB.h"
#include "NetMan.h"
#include "matrix.h"
#include "OchMemDebug.h"

//#define CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEBUG_FABIO

using namespace NS_OCH;

//void genKeyaoJSACTraffic(const char *pFile);
//void testLightpathDB();
//void testBinaryHeap();
//void writeResults();


int main(int argc, char **argv) {
//clock_t start, finish;
//start = clock();

#ifdef _DEBUG
//  _CrtSetBreakAlloc(3828);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //B: passo come param l'or bit a bit di queste 2 costanti
#endif
	
    // if (argc < 9) {
    if (argc < 11) { //M
        cout<<"Usage: prov_seg RandomSeed TopoFile ProvisionType ";
        cout<<"#ofConnections ArrivalRate #OfPaths ";
        cout<<"HopCount Epsilon TimePolicy UnAvConn"; //M//-t
        cout<<endl;
        return -1;
    }
    //srand(atoi(argv[1]));
	srand(time(NULL));

   /* 
	Matrix mat=Matrix(3,4);
	valarray<double> v2;
	for (int i=0;i<3;i++)
	{
		mat(i,1)=i;
		cout<<mat(i,1);
		mat(i,2)=i;
		mat(i,3)=i;
	}
	cout<<mat(1,1);
	cout<<mat(1,2);
v2=mat.column(1).v->cshift(1);
cout<<endl<<v2[0]<<v2[1]<<v2[2];*/

    NetMan hNetMan;

#ifdef DEBUGB
	hNetMan.printInputData(argc, argv);
#endif // DEBUGB
	for (int i = 0; i < argc; i++) {
		if (i == 2)
			cout << "Topo File: ";
		if (i == 3)
			cout << "Provision type: ";
		if (i == 4)
			cout << "nCon: ";
		if (i == 5)
			cout << "arrival rate: ";
		cout << argv[i] << endl;
	}
		
    // if (!hNetMan.initialize(argv[2], argv[3], atoi(argv[6]), atof(argv[8]))) 
	std::string type;
	type =  argv[3];
	cout << type << endl;
	if (type != "BBU_HOTEL")
	{
		if (!hNetMan.initialize(argv[2], argv[3], atoi(argv[6]), atof(argv[8]), atoi(argv[9]), argv[10])) //M//-t
		{
			return -1;
		}
	}
	else
	{
		if (!hNetMan.BBUinitialize(argv[2], argv[3], atoi(argv[6]), atof(argv[8]), atoi(argv[9]), argv[10]))
		{
			return -1;
		}
	}

    Simulator hSimulator;
    if (!hSimulator.setNetworkManager(&hNetMan))
        return -1;
	
    if (!hSimulator.initialize((UINT)atoi(argv[4]), atof(argv[5]), 
                            atof(argv[12]), "DBG_Uniform192", argv[3], atoi(argv[7])))
        return -1;

	if ((UINT)atoi(argv[13]) == 0)
		hNetMan.isLinkDisjointActive = false;
	else
		hNetMan.isLinkDisjointActive = true;

#ifdef DEBUGB
	//-B: NETMAN DUMP
	hNetMan.dump(cout);
	cin.get();
#endif // DEBUGB

    hSimulator.run(atof(argv[11]));
	//-t

    // show results 
double delay = atof(argv[11]);
double HT = atof(argv[12]); //Holding Time of each connection
double pc=hSimulator.m_pNetMan->pcPer/100L;
double bc=hSimulator.m_pNetMan->bcPer/100L;
double fc=hSimulator.m_pNetMan->fcPer/100L;
double bothc=hSimulator.m_pNetMan->bothcPer/100L;
double blocked=hSimulator.m_pNetMan->m_hLog.m_nBlockedCon;
double totalConn=hSimulator.m_pNetMan->m_hLog.m_nProvisionedCon; //-L: why total connections = provisioned connections???
double ar = atof(argv[5]); //arrival rate

//cout<<"\ntotal conn: "<<totalConn;
cout << "\nCONNESSIONI BLOCCATE: " << blocked << endl;
//cin.get();
double tot_hop=hSimulator.m_pNetMan->m_hLog.m_nPHopDistance;
double bp=hSimulator.m_pNetMan->p_block->mean(); //blocking probability
//hSimulator.showLog(cout);
double resourceOB = (hSimulator.m_pNetMan->m_hLog.m_dSumLinkLoad - hSimulator.m_pNetMan->m_hLog.m_dPrimaryRc) / hSimulator.m_pNetMan->m_hLog.m_dPrimaryRc;
double PTot=hSimulator.m_pNetMan->PTot;// Potenza Totale consumata dalla rete
double PGreen=hSimulator.m_pNetMan->PGreen;// Potenza Rinnovabile consumata dalla rete
double PBrown=hSimulator.m_pNetMan->PBrown;// Potenza Non Rinnovabile consumata dalla rete
double PTransport=hSimulator.m_pNetMan->PTransport; //Potenza Non Rinnovabile consumata x il trasporto (no computing)
double PProc=hSimulator.m_pNetMan->PProc; //Potenza Non Rinnovabile consumata x processing
double PEDFA=hSimulator.m_pNetMan->PEDFA; //Potenza Non Rinnovabile consumata x dagli EDFA

if(0 == strcmp(argv[10], "Pb")) //Se l'input dell'11esimo argomento è "Pb"
  { 
    std::ofstream out("risultati.txt",ios_base::app);  //salvataggio nel file "risultati.txt"
    //FORMATO IN USCITA: ARRrate, prBlocco
    //	out<<ar<<"\t"<<(blocked/(atof(argv[4]))*100)<<"\t"<<(pc/(atof(argv[4]))*100)<<"\t"<<(bc/(atof(argv[4]))*100)<<"\t"<<(fc/(atof(argv[4]))*100)<<endl<<endl;
	out << ar << "\t" << delay << "\t" << (bp) << "\t" << (pc) << "\t" << (bc) << "\t" << (bothc) << "\t" << (fc) << "\t" <<
		((bp == 0) ? 0 : (pc / bp)) << "\t" << ((bp == 0) ? 0 : (bc / bp)) << "\t" << ((bp == 0) ? 0 : (bothc / bp)) << "\t" << ((bp == 0) ? 0 : (fc / bp)) 
		<< "\t" << (double)(bp - pc - bc - fc - bothc);
	 out << "\t" << (double)(1 - ((bp == 0) ? 0 : (pc / bp)) - ((bp == 0) ? 0 : (bc / bp)) - ((bp == 0) ? 0 : (fc / bp)) - ((bp == 0) ? 0 : (bothc / bp))) 
		 << endl << endl;
  }


//-----------------------------------OUTPUT GREEN-----------------------------------------------------------------------------------
else if(0 == strcmp(argv[10], "power")) //Se l'input dell'11esimo argomento è "Power"
  { 
#ifdef DEBUGB
	  cout << "STAMPO NEL FILE RisultatiGreen.txt" << endl;
#endif // DEBUGB

    std::ofstream out("RisultatiGreen.txt",ios_base::app);  //salvataggio nel file 
    //FORMATO IN USCITA: ARRrate, prBlocco,   ,PotenzaTotaleConsumata,PotenzaRinnovabile, PotenzaNonRinnovabile,ConnessioniBloccate,ConnessioniInstradate
    //	out<<ar<<"\t"<<(blocked/(atof(argv[4]))*100)<<"\t"<<(pc/(atof(argv[4]))*100)<<"\t"<<(bc/(atof(argv[4]))*100)<<"\t"<<(fc/(atof(argv[4]))*100)<<endl<<endl; 
	out<<"AR  "<<"\tdelay  "<<"\tPb"<<"\tPTot"<<"\tPGreen  "<<"\tPBrown "<<"\t     PTran"<<"\t     PEDFA"<<"\t     PProc"<<"\t ConnBlock"<<"\t  ConnProv"<<"\t  Tot Hop"<<endl<<endl;
	out<<ar<<"\t"<<delay<<"\t"<<(bp)<<"\t"<<(PTot)<<"\t  "<<(PGreen)<<"\t  "<<(PBrown)<<"\t  "<<(PTransport)<<"\t  "<<(PEDFA)<<"\t  "<<(PProc)<<"\t"<<blocked<<"\t"<<totalConn<<"\t"<<tot_hop<<endl<<endl;
  }
//--------------------------------------------------------------------------------------------------------------------------------


else if(0 == strcmp(argv[10], "Ro"))//Resource overbuild output, Se l'input dell'11esimo argomento è "Ro"
  { 
    std::ofstream out("risultati.txt",ios_base::app);
     //FORMATO IN USCITA: ARRrate, prBlocco
     //	out<<ar<<"\t"<<(blocked/(atof(argv[4]))*100)<<"\t"<<(pc/(atof(argv[4]))*100)<<"\t"<<(bc/(atof(argv[4]))*100)<<"\t"<<(fc/(atof(argv[4]))*100)<<endl<<endl;
   	 out<<ar<<"\t"<<delay<<"\t"<<bp<<"\t"<<resourceOB<<endl<<endl;
  }

else if(0 == strcmp(argv[10], "Nd"))//Normalized delay (Tau/HT) output, Se l'input dell'11esimo argomento è "Nd"
  { 
     std::ofstream out("risultati.txt",ios_base::app);
    //FORMATO IN USCITA: ARRrate, prBlocco
    //	out<<ar<<"\t"<<(blocked/(atof(argv[4]))*100)<<"\t"<<(pc/(atof(argv[4]))*100)<<"\t"<<(bc/(atof(argv[4]))*100)<<"\t"<<(fc/(atof(argv[4]))*100)<<endl<<endl;
  	out<<ar<<"\t"<<delay<<"\t"<<ar/HT<<"\t"<<delay/HT<<"\t"<<bp<<"\t"<<resourceOB<<endl<<endl;
  }
else if (0 == strcmp(argv[10], "BBU"))
{
	cout << "HLOG OUTPUT:" << endl;
	hNetMan.m_hLog.output();

	cout << "SONO ALLA FINE DEL MAIN. Premi invio per stampare i risultati nel file" << endl;
	//cin.get();
	//cin.get();
	cin.get();
	cout << "----------- STAMPO RISULTATI NEL FILE Results.txt -----------" << endl;
	
	cout << "\n_______________________________________________________________________________";
	cout << "\nINPUT DATA:" << endl;
	cout << "Network topology: " << argv[2] << endl;
	cout << "Number of mobile nodes: " << hNetMan.m_hWDMNet.numMobileNodes << endl;
	cout << "Number of fixed nodes: " << hNetMan.m_hWDMNet.numFixedNodes << endl;
	cout << "Number of fixed-mobile nodes: " << hNetMan.m_hWDMNet.numFixMobNodes << endl;
	cout << "Number of candidate hotel nodes: " << hNetMan.m_hWDMNet.countCandidateHotels() << endl;
	cout << "Avg arrival rate: " << ar << endl;
	cout << "Avg holding time: " << HT << endl;
	cout << "Traffic (arr rate * holding time) [Erlang]: " << ar*HT << endl;
	cout << "Andamento traffico variabile: ";
	for (int i = 0; i < 24; i++)
	{
		cout << hSimulator.tassiarrivo[i] << " ";
	}
	cout << endl;
	cout << "Amount of traffic per connection [OC1]: " << BWDGRANULARITY << endl;
	//cout << "Max path length in hops: " << atoi(argv[7]) << endl;
	cout << "Num of wavelenghts per fiber: " << hNetMan.m_hWDMNet.numberOfChannels << endl;
	cout << "Channel capacity [OC1]: " << hNetMan.m_hWDMNet.getChannelCapacity() << endl;
	//cout << "Max num BBU in a single hotel node: " << MAXNUMBBU << endl;
	//cout << "Smallest bandwidth for MOBILE connections: " << OC6 << " OC1" << endl;
	//cout << "Biggest bandwidth for MOBILE connections: " << OC15 << " OC1" << endl;
	//cout << "Smallest bandwidth for FIXED connections: " << OC192 << " OC1" << endl;
	//cout << "Biggest bandwidth for FIXED connections: " << OC384 << " OC1" << endl;
	//cout << "Required bandwidth for CPRI connections: " << OCcpriMC << " OC1" << endl;
	//cout << "Propagation latency: " << PROPAGATIONLATENCY << endl;
	//cout << "Electronic switch latency: " << ELSWITCHLATENCY << endl;
	cout << "Fronthaul latency budget: " << LATENCYBUDGET << endl;
	cout << "Transitory phase duration: " << N_TRANS_CONNECTIONS << " connections" << endl;
	cout << "Running phase duration: " << N_RUN_CONNECTIONS << " connections" << endl;
	cout << "Max tot num of connections to be simulated: " << atoi(argv[4]) << endl;
	cout << "Max tot simulation duration: " << SIM_DURATION << " seconds" << endl;
	cout << "Confidence percentage: " << hNetMan.conf << endl;
	cout << "\nRESULTS:" << endl;
	cout << "\nSimulation Time: " << (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime)
		<< " + transitory time: " << (hNetMan.m_hLog.transitoryTime);
	cout << " - # connections: " << hNetMan.m_hLog.NumOfConnections << endl;
	cout << "Policy BBU placement: ";
	if (BBUPOLICY == 0)
		cout << "placeBBUHigh";
	else if (BBUPOLICY == 1)
		cout << "placeBBUClose";
	else if (BBUPOLICY == 2)
		cout << "placeBBU_Metric";
	cout << "\nErrore percentuale: " << hNetMan.p_block->confpercerr(hNetMan.conf) << endl;
	//cout << "Number of grooming 'possibilities': " << hNetMan.m_hLog.groomingPossibilities;
	
	cout << "Hotels Power Consumption: " << hNetMan.m_hLog.powerConsumption << " (PEAK) - "
		<< hNetMan.m_hLog.powerConsumption / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime) << " (AVERAGE)" << endl;
	cout << "AVERAGE latency = " << hNetMan.m_hLog.avgLatency / hNetMan.m_hLog.countConnForLatency << endl;
	cout << "\nNumber of active (hotel) nodes: " << hNetMan.m_hLog.peakNumActiveNodes << " (PEAK) - "
		<< hNetMan.m_hLog.avgActiveNodes / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime) << " (AVERAGE)" << endl;
	cout << "Number of active DUs: " << hNetMan.m_hLog.peakNumActiveBBUs << " (PEAK) - "
		<< hNetMan.m_hLog.avgActiveBBUs / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime) << " (AVERAGE)" << endl;
	cout << "Number of active CUs: " << hNetMan.m_hLog.peakNumActiveCUs << " (PEAK) - "
		<< hNetMan.m_hLog.avgActiveCUs / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime) << " (AVERAGE)" << endl;
	cout << "Number of active small cells: " << hNetMan.m_hLog.peakActiveSC << "  (PEAK) - "
		<< hNetMan.m_hLog.avgActiveSC / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime) << " (AVERAGE)" << endl;
	cout << "Number of active lightpaths: " << hNetMan.m_hLog.peakNumLightpaths << " (PEAK) - "
		<< hNetMan.m_hLog.avgActiveLightpaths / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime) << " (AVERAGE)" << endl;
	cout <<  "Network cost: " << hNetMan.m_hLog.peakNetCost << " (PEAK) - " << (hNetMan.m_hLog.networkCost / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime))
		<< " (AVERAGE) [costo rete * time]" << endl;
	cout << "Hotels power consumption: " << hNetMan.m_hLog.peakNetCost << " (PEAK) - " << (hNetMan.m_hLog.networkCost / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime))
		<< " (AVERAGE) [costo rete * time]" << endl;
	cout << "Network cost per OC1 (<--> cost per connection OR cost per bit): costo rete / provisioned backhaul bwd = "
		<< (hNetMan.m_hLog.networkCost / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime)) << " / "
		<< (hNetMan.m_hLog.m_nProvisionedMobileBackBW + hNetMan.m_hLog.m_nProvisionedFixMobBackBW + hNetMan.m_hLog.m_nProvisionedFixedBackBW) << " = "
		<< ((hNetMan.m_hLog.networkCost / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime)) /
		((float)hNetMan.m_hLog.m_nProvisionedMobileBackBW + (float)hNetMan.m_hLog.m_nProvisionedFixMobBackBW + (float)hNetMan.m_hLog.m_nProvisionedFixedBackBW)) << endl;
	
	cout <<  "\nNumber of provisioned connections (FH+MH+BH): "<< hNetMan.m_hLog.m_nProvisionedCon;
	cout << "\nNumber of blocked connections (FH+MH+BH): " << hNetMan.m_hLog.m_nBlockedCon << endl;
	cout << "Number of blocked fronthaul connections (mob + fixmob): " << hNetMan.m_hLog.m_nBlockedMobileFrontConn << " + "
		<< hNetMan.m_hLog.m_nBlockedFixMobFrontConn << " = " << (hNetMan.m_hLog.m_nBlockedMobileFrontConn + hNetMan.m_hLog.m_nBlockedFixMobFrontConn) << endl;
	cout << "Number of blocked midhaul connections: " << hNetMan.m_hLog.m_nBlockedFixMidConn << " = " << (hNetMan.m_hLog.m_nBlockedFixMidConn) << endl;
	cout << "Number of blocked backhaul connections (mob + fixmob + fixed): " << hNetMan.m_hLog.m_nBlockedMobileBackConn << " + "
		<< hNetMan.m_hLog.m_nBlockedFixMobBackConn << " + " <<  hNetMan.m_hLog.m_nBlockedFixedBackConn << " = " 
		<< (hNetMan.m_hLog.m_nBlockedMobileBackConn + hNetMan.m_hLog.m_nBlockedFixMobBackConn + hNetMan.m_hLog.m_nBlockedFixedBackConn) << endl;
	cout << "\nBWD of provisioned connections (FH+BH): " << hNetMan.m_hLog.m_nProvisionedBW;
	cout << "\nBWD of blocked connections (FH+MH+BH): " << hNetMan.m_hLog.m_nBlockedBW;
	cout << "\nBWD of blocked fronthaul connections (mob + fixmob): " << hNetMan.m_hLog.m_nBlockedMobileFrontBW << " + "
 		<< hNetMan.m_hLog.m_nBlockedFixMobFrontBW << " = " << (hNetMan.m_hLog.m_nBlockedMobileFrontBW + hNetMan.m_hLog.m_nBlockedFixMobFrontBW);
	cout << "\nBWD of blocked backhaul connections (mob + fixmob + fix): " << hNetMan.m_hLog.m_nBlockedMobileBackBW<< " + "
		<< hNetMan.m_hLog.m_nBlockedFixMobBackBW << " + " << hNetMan.m_hLog.m_nBlockedFixedBackBW << " = "
		<< (hNetMan.m_hLog.m_nBlockedFixedBackBW + hNetMan.m_hLog.m_nBlockedFixMobBackBW + hNetMan.m_hLog.m_nBlockedMobileBackBW);
	cout << "\nBWD of provisioned fronthaul connections (mob + fixmob): " << hNetMan.m_hLog.m_nProvisionedMobileFrontBW << " + "
		<< hNetMan.m_hLog.m_nProvisionedFixMobFrontBW << " = " << (hNetMan.m_hLog.m_nProvisionedFixMobFrontBW + hNetMan.m_hLog.m_nProvisionedMobileFrontBW);
	cout << "\nBWD of provisioned midhaul connections: " << hNetMan.m_hLog.m_nProvisionedFixedMidBW << " = " << (hNetMan.m_hLog.m_nProvisionedFixedMidBW);
	cout << "\nBWD of provisioned backhaul connections (mob + fixmob + fix): " << hNetMan.m_hLog.m_nProvisionedMobileBackBW << " + "
		<< hNetMan.m_hLog.m_nProvisionedFixMobBackBW << " + " << hNetMan.m_hLog.m_nProvisionedFixedBackBW << " = "
		<< (hNetMan.m_hLog.m_nProvisionedMobileBackBW + hNetMan.m_hLog.m_nProvisionedFixMobBackBW + hNetMan.m_hLog.m_nProvisionedFixedBackBW);
	cout << "\nBlocked BH BWD for blocked bh connections: " << hNetMan.m_hLog.m_nBlockedBHForBHBlock;
	cout << "\nBlocked FH BWD for blocked fh connections: " << hNetMan.m_hLog.m_nBlockedFHForFHBlock << endl;
	
	cout << "Blocking probability - BACKHAUL (Overall; BWD) = Blocked backhaul bwd / total backhaul bwd = "
		<< (hNetMan.m_hLog.m_nBlockedMobileBackBW + hNetMan.m_hLog.m_nBlockedFixedBackBW + hNetMan.m_hLog.m_nBlockedFixMobBackBW) << " / "
		<< (hNetMan.m_hLog.m_nProvisionedMobileBackBW + hNetMan.m_hLog.m_nProvisionedFixMobBackBW + hNetMan.m_hLog.m_nProvisionedFixedBackBW +
			hNetMan.m_hLog.m_nBlockedMobileBackBW + hNetMan.m_hLog.m_nBlockedFixedBackBW + hNetMan.m_hLog.m_nBlockedFixMobBackBW - (6* hNetMan.m_hLog.m_nNumConnectionsChangingBBU)) << " = "
		<< (((float)((float)hNetMan.m_hLog.m_nBlockedMobileBackBW + (float)hNetMan.m_hLog.m_nBlockedFixedBackBW + (float)hNetMan.m_hLog.m_nBlockedFixMobBackBW)) /
		(((float)((float)hNetMan.m_hLog.m_nBlockedMobileBackBW + (float)hNetMan.m_hLog.m_nBlockedFixedBackBW + (float)hNetMan.m_hLog.m_nBlockedFixMobBackBW)) +
			((float)((float)hNetMan.m_hLog.m_nProvisionedMobileBackBW + (float)hNetMan.m_hLog.m_nProvisionedFixMobBackBW +
			(float)hNetMan.m_hLog.m_nProvisionedFixedBackBW)) - (float)(6* hNetMan.m_hLog.m_nNumConnectionsChangingBBU))) << endl;
	if (hNetMan.m_hLog.m_nBlockedCon > 0)
	{
		cout << "\nBlocking probability because of latency: "
			<< (hNetMan.m_hLog.m_nBlockedConDueToLatency / hNetMan.m_hLog.m_nBlockedCon) << endl;
		cout << "\nBlocking probability because of unreachability: "
			<< (hNetMan.m_hLog.m_nBlockedConDueToUnreach / hNetMan.m_hLog.m_nBlockedCon) << endl;
	}
	else
	{
		cout << "\nBlocking probability because of latency: "
			<< hNetMan.m_hLog.m_nBlockedConDueToLatency << " / 0" << endl;
		cout << "\nBlocking probability because of unreachability: "
			<< hNetMan.m_hLog.m_nBlockedConDueToUnreach << " / 0" << endl;
	}

	cout << "\nAverage latency: ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.AvgLatency_hour[oo] << " ";
	}cout << endl;
	cout << "\nBlocking probability (connections; overall): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.Pblock_hour[oo] << " ";
	}cout << endl;
	cout << "\nBlocking probability (BWD; overall): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.PblockBW_hour[oo] << " ";
	}cout << endl;
	cout << "\nBlocking probability fronthaul (connections; overall): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.PblockFront_hour[oo] << " ";
	}cout << endl;
	cout << "\nBlocking probability fronthaul (BWD; overall): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.PblockFrontBW_hour[oo] << " ";
	}cout << endl;
	cout << "\nBlocking probability backhaul (connections; overall): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.PblockBack_hour[oo] << " ";
	}cout << endl;
	cout << "\nBlocking probability backhaul (BWD; overall): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.PblockBackBW_hour[oo] << " ";
	}cout << endl;

	/* -B: COMMENTED BECAUSE WE SHOULD USE runLog IF WE WANT TO HAVE PERIODICAL STATS
	cout << "\n\nNetwork cost (PERIODIC; min by min): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.NetCost_hour[oo] << " ";
	}cout << endl;
	*/
	cout << "\nAVERAGE network cost (weighted on time; overall): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.Avg_NetCost_hour[oo] << " ";
	}cout << endl;

	cout << "\nActivity time of each node (simulation time  = " << (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime) << "): " << endl;
	list<AbstractNode*>::const_iterator itr;
	OXCNode*pNode;
	for (itr = hNetMan.m_hWDMNet.m_hNodeList.begin(); itr != hNetMan.m_hWDMNet.m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		if (pNode->m_dActivityTime > 0)
			cout << "Node #" << pNode->getId() << " - activity time: " << pNode->m_dActivityTime << endl;
		if (pNode->m_nCountBBUsChanged > 0) {
			cout << "Node #" << pNode->getId() << " - number of BBUs changed: " << pNode->m_nCountBBUsChanged <<endl;
			hNetMan.m_hLog.totalBBUsChanged += pNode->m_nCountBBUsChanged;
			cout << "Total number of BBUs changed updated to " << hNetMan.m_hLog.totalBBUsChanged << endl;
		}
	}

	/*cout << "\nHotels node of each node:" << endl;
	//list<AbstractNode*>::const_iterator itr;
	//OXCNode*pNode;
	for (itr = hNetMan.m_hWDMNet.m_hNodeList.begin(); itr != hNetMan.m_hWDMNet.m_hNodeList.end(); itr++)
	{
		pNode = (OXCNode*)(*itr);
		if (pNode->m_vLogHotels.size() > 0)
		{
			cout << "Node #" << pNode->getId() << " - hotels logged: ";
			for (int i = 0; i < pNode->m_vLogHotels.size(); i++)
			{
				cout << pNode->m_vLogHotels[i] << " ";
			}
			cout << endl;
		}
		else
		{
			assert(false);
		}
	}*/

	cout << "\nBACKHAUL (provisioned) carried load MONICA[BH BWD * time] = " << hNetMan.m_hLog.m_dSumCarriedLoadMonica / (hNetMan.m_hLog.m_hSimTimeSpan-hNetMan.m_hLog.transitoryTime)  << endl;

	cout << "Total number of lightpaths activated during the simulation: " << hNetMan.m_hLog.m_nLightpath << endl;
	cout << "Sum lightpath holding time(?) [SUM(lightpaths lifetime)]: " << hNetMan.m_hLog.m_dSumLightpathHoldingTime << endl;
	cout << "Sum lightpath load(?) [used lp cap / tot lp cap ; (weighted on time)]: " << hNetMan.m_hLog.m_dSumLightpathLoad << endl;	
	cout << "Sum link load(?) [lifetime * hops * capacity]: " << hNetMan.m_hLog.m_dSumLinkLoad << endl;
	cout << "Total used OC1 time(?) [SUM(busyTime) = SUM(used lp cap * time)]: " << hNetMan.m_hLog.m_dTotalUsedOC1Time << endl;

	cout << "\nCarico(???): ";
	for (int oo = 0; oo < 24; oo++)
	{
		cout << hNetMan.m_hLog.carico[oo] << " ";
	}cout << endl;

	cout << "\nReal avg arrival rate PER SECOND, PER NETWORK [#connections simulated / simulation time]: " << hNetMan.m_hLog.NumOfConnections/ hNetMan.m_hLog.m_hSimTimeSpan << endl;
	cout << "Real avg arrival rate PER SECOND, PER NODE: " << hNetMan.m_hLog.NumOfConnections / hNetMan.m_hLog.m_hSimTimeSpan / hNetMan.m_hWDMNet.numberOfNodes << endl;
	cout << "Average amount of traffic generated per second, per network [OC1]: " << (hNetMan.m_hLog.NumOfConnections / hNetMan.m_hLog.m_hSimTimeSpan) * BWDGRANULARITY << endl;
	cout << "Average amount of traffic generated per second, per node [OC1]: " << ((hNetMan.m_hLog.NumOfConnections - hNetMan.m_hLog.m_nNumConnectionsChangingBBU)/ hNetMan.m_hLog.m_hSimTimeSpan / hNetMan.m_hWDMNet.numberOfNodes)*BWDGRANULARITY << endl;
	cout << "Average number of changes of BBU per node: " << (hNetMan.m_hLog.totalBBUsChanged / hNetMan.m_hWDMNet.numberOfNodes) << endl;
	cout << "Number of connections which changed BBUs: " << hNetMan.m_hLog.m_nNumConnectionsChangingBBU << endl;
	/*
		std::map<UINT, vector<double> >::iterator itr;
	cout << "\n\nFiber load per hour (PERIODICAL):";
	int oo = 0;
	vector<double> vect;
	for (itr = hNetMan.m_hLog.UniFiberLoad_hour.begin(); itr != hNetMan.m_hLog.UniFiberLoad_hour.end(); oo++, itr++) //(int oo = 0; oo < 24; oo++)
	{
		cout << "\nORA #" << oo << endl;
		//-B: il vector uniFiberLoad era stato riempito a partire dall'ultimo link
		for (int i = (hNetMan.getNumberOfUniFibers() - 1), fiberId = 1; i >= 0; i--, fiberId++)
		{
			vect = itr->second;
			cout << "(#" << fiberId << ")" << vect[i] << "  " ;
			//cout << vect[i] << " ";
		}
	}
	*/

	//-B: print some results in a file
	//std::ofstream out("Results", ios_base::app);
	
	FILE *out = fopen("Results.txt", "w");
	if (out == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	//ofstream out;
	//out.open("Results.txt", ofstream::out);
	//if (out.is_open())

		cout << "FILE APERTO" << endl;
		const char *text = "\n_______________________________________________________________________________";
		fprintf(out, "%s", text);
		text = "\nINPUT DATA:\n";
		fprintf(out, "%s", text);
		if (MIDHAUL)
			text = "SI";
		else
			text = "NO";
		fprintf(out, "Midhaul: %s\n", text);
		fprintf(out, "\nLink occupancy thr [free ch]: %d\n", LINKOCCUPANCY_PERC);
		text = "\nNetwork topology: ";
		fprintf(out, "%s%s\n", text, argv[2]);
		text = "Number of mobile nodes: ";
		fprintf(out, "%s%d\n", text, hNetMan.m_hWDMNet.numMobileNodes);
		text = "Number of fixed nodes: ";
		fprintf(out, "%s%d\n", text, hNetMan.m_hWDMNet.numFixedNodes);
		text = "Number of fixed-mobile nodes: ";
		fprintf(out, "%s%d\n", text, hNetMan.m_hWDMNet.numFixMobNodes);
		text = "Number of candidate hotel nodes: ";
		fprintf(out, "%s%d\n", text, hNetMan.m_hWDMNet.countCandidateHotels());
		
		cout << "STOP 1" << endl;
		text = "Avg arrival rate: ";
		fprintf(out, "%s%f\n", text, ar);
		text = "Avg holding time: ";
		fprintf(out, "%s%f\n", text, HT);
		fprintf(out, "Traffic (arr rate * holding time) [Erlang]: %f\n", ar*HT);
		text = "Andamento traffico variabile: ";
		fprintf(out, "%s", text);
		for (int i = 0; i < 24; i++)
		{
			fprintf(out, "%f ", hSimulator.tassiarrivo[i]);
			//out << hSimulator.tassiarrivo[i] << " ";
		}
		fprintf(out, "Amount of traffic per connection [OC1]: %d\n", BWDGRANULARITY);
		//text = "Max path length in hops: ";
		//fprintf(out, "\n%s%f\n", text, atoi(argv[7]));
		text = "Num of wavelenghts per fiber: ";
		fprintf(out, "\n%s%d\n", text, hNetMan.m_hWDMNet.numberOfChannels);
		text = "Channel capacity [OC1]: ";
		fprintf(out, "%s%d\n", text, hNetMan.m_hWDMNet.getChannelCapacity());
		//text = "Max num BBU in a single hotel node: ";
		//fprintf(out, "%s%d\n", text, MAXNUMBBU);
		
		text = "Fronthaul latency budget: ";
		fprintf(out, "%s%f\n", text, LATENCYBUDGET);
		text = "Transitory phase duration: ";
		fprintf(out, "%s%d\n", text, N_TRANS_CONNECTIONS);
		text = "Running phase duration: ";
		fprintf(out, "%s%d\n", text, N_RUN_CONNECTIONS);
		text = "Max tot num of connections to be simulated: ";
		fprintf(out, "%s%d\n", text, atoi(argv[4]));
		const char *text1 = "Max tot simulation duration: ";
		const char *text2 = "seconds";
		//fprintf(out, "%s%f %s\n", text1, SIM_DURATION, text2); //-B: SE PROVO A STAMPARE QUESTA MI DA ERRORE! BOOOOO!!!
		text = "Confidence percentage: ";
		fprintf(out, "%s%f\n", text, hNetMan.conf);
		
		text = "\nRESULTS:\nSimulation Time: ";
		text1 = " + transitory time: ";
		text2 = " - # connections: ";
		fprintf(out, "%s%f%s%f%s%f", text, (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime),
			text1, hNetMan.m_hLog.transitoryTime, text2, hNetMan.m_hLog.NumOfConnections);
		text = "\nPolicy BBU placement: ";
		if (BBUPOLICY == 0)
			text1 = "placeBBUHigh";
		else if (BBUPOLICY == 1)
			text1 = "placeBBUClose";
		else if (BBUPOLICY == 2)
			text1 = "placeBBU_Metric";
		fprintf(out, "%s%s", text, text1);
		text = "\nErrore percentuale: ";
		//fprintf(out, "%s%f\n", text, hNetMan.p_block->confpercerr(hNetMan.conf));
		//text = "\nNumber of grooming 'possibilities': ";
		//fprintf (out, "%s%f", text, hNetMan.m_hLog.groomingPossibilities);
		text = "\nCARRIED LOAD [bh prov conn*time] = ";
		fprintf (out, "%s%f\n", text, hNetMan.m_hLog.m_dSumCarriedLoad);
		text = "AVERAGE latency = ";
		fprintf(out, "%s%f", text, (hNetMan.m_hLog.avgLatency / hNetMan.m_hLog.countConnForLatency));
		if (BBUPOLICY == 2)
			fprintf(out, "\nNumber of candidate hotel nodes: %d", hSimulator.m_nNumOfActiveHotels);
		text = "\nHotels power consumption: "; //-L
		fprintf(out, "%s%d (PEAK) - %f (AVERAGE)", text, hNetMan.m_hLog.powerConsumption, hNetMan.m_hLog.powerConsumption / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime));
		text = "\nNumber of active (hotel) nodes: ";
		fprintf(out, "%s%d (PEAK) - %f (AVERAGE)", text, hNetMan.m_hLog.peakNumActiveNodes, hNetMan.m_hLog.avgActiveNodes / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime));
		text = "\nNumber of active DUs: ";
		fprintf(out, "%s%d (PEAK) - %f (AVERAGE)", text, hNetMan.m_hLog.peakNumActiveBBUs, hNetMan.m_hLog.avgActiveBBUs / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime));
		text = "\nNumber of active CUs: ";
		fprintf(out, "%s%d (PEAK) - %f (AVERAGE)", text, hNetMan.m_hLog.peakNumActiveCUs, hNetMan.m_hLog.avgActiveCUs / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime));
		fprintf(out, "\nNumber of active small cells: %d (PEAK) - %f (AVERAGE)", hNetMan.m_hLog.peakActiveSC,
			(hNetMan.m_hLog.avgActiveSC / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime)));
		text = "\nNumber of active lightpaths: ";
		fprintf(out, "%s%d (PEAK) - %f (AVERAGE)", text, hNetMan.m_hLog.peakNumLightpaths, hNetMan.m_hLog.avgActiveLightpaths / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime));
		text = "\nNetwork cost: ";
		text1 = " [costo rete * time]";
		fprintf(out, "%s%d (PEAK) - %f (AVERAGE)%s\n", text, hNetMan.m_hLog.peakNetCost, hNetMan.m_hLog.networkCost
			/ (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime), text1);
		cout << "STOP 3" << endl;

		text = "Network cost per OC1 (<--> cost per connection OR cost per bit): costo rete / provisioned backhaul bwd = ";
		fprintf(out, "%s%f / %d = %f\n", text, (hNetMan.m_hLog.networkCost / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime)),
			(hNetMan.m_hLog.m_nProvisionedFixedBackBW + hNetMan.m_hLog.m_nProvisionedFixMobBackBW + hNetMan.m_hLog.m_nProvisionedMobileBackBW),
			((hNetMan.m_hLog.networkCost / (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime)) /
			((float)hNetMan.m_hLog.m_nProvisionedMobileBackBW + (float)hNetMan.m_hLog.m_nProvisionedFixMobBackBW + (float)hNetMan.m_hLog.m_nProvisionedFixedBackBW)));

		cout << "STOP 4" << endl;
		text = "\nNumber of provisioned connections: ";
		fprintf(out, "%s%d", text, hNetMan.m_hLog.m_nProvisionedCon);
		text = "\nNumber of blocked connections: ";
		fprintf(out, "%s%d", text, hNetMan.m_hLog.m_nBlockedCon);
		text = "\nNumber of blocked fronthaul connections (mob + fixmob): ";
		fprintf (out, "%s%d + %d = %d", text, hNetMan.m_hLog.m_nBlockedMobileFrontConn, hNetMan.m_hLog.m_nBlockedFixMobFrontConn,
			(hNetMan.m_hLog.m_nBlockedMobileFrontConn + hNetMan.m_hLog.m_nBlockedFixMobFrontConn));
		text = "\nNumber of blocked backhaul connections (mob + fixmob + fixed): ";
		fprintf(out, "%s%d + %d + %d = %d\n", text, hNetMan.m_hLog.m_nBlockedMobileBackConn, hNetMan.m_hLog.m_nBlockedFixMobBackConn, hNetMan.m_hLog.m_nBlockedFixedBackConn,
			(hNetMan.m_hLog.m_nBlockedMobileBackConn + hNetMan.m_hLog.m_nBlockedFixMobBackConn + hNetMan.m_hLog.m_nBlockedFixedBackConn));

		text = "\nBWD of provisioned connections (FH + MH + BH): ";
		fprintf(out, "%s%d", text, hNetMan.m_hLog.m_nProvisionedBW);
		text = "\nBWD of blocked connections (FH + MH + BH): ";
		fprintf(out, "%s%d", text, hNetMan.m_hLog.m_nBlockedBW);
		text = "\nBWD of blocked fronthaul connections (mob + fixmob): ";
		fprintf (out, "%s%d + %d = %d", text, hNetMan.m_hLog.m_nBlockedMobileFrontBW, hNetMan.m_hLog.m_nBlockedFixMobFrontBW,
			(hNetMan.m_hLog.m_nBlockedMobileFrontBW + hNetMan.m_hLog.m_nBlockedFixMobFrontBW));
		text = "\nBWD of blocked backhaul connections (mob + fixmob + fix): ";
		fprintf (out, "%s%d + %d + %d = %d", text, hNetMan.m_hLog.m_nBlockedMobileBackBW, hNetMan.m_hLog.m_nBlockedFixMobBackBW,
			hNetMan.m_hLog.m_nBlockedFixedBackBW, (hNetMan.m_hLog.m_nBlockedFixedBackBW + hNetMan.m_hLog.m_nBlockedFixMobBackBW + hNetMan.m_hLog.m_nBlockedMobileBackBW));
		text = "\nBWD of provisioned fronthaul connections (mob + fixmob): ";
		fprintf (out, "%s%d + %d = %d", text, hNetMan.m_hLog.m_nProvisionedMobileFrontBW, hNetMan.m_hLog.m_nProvisionedFixMobFrontBW,
			(hNetMan.m_hLog.m_nProvisionedFixMobFrontBW + hNetMan.m_hLog.m_nProvisionedMobileFrontBW));
		text = "\nBWD of provisioned backhaul connections (mob + fixmob + fix): ";
		fprintf(out, "%s%d + %d + %d = %d", text, hNetMan.m_hLog.m_nProvisionedMobileBackBW, hNetMan.m_hLog.m_nProvisionedFixMobBackBW,
			hNetMan.m_hLog.m_nProvisionedFixedBackBW, (hNetMan.m_hLog.m_nProvisionedMobileBackBW + hNetMan.m_hLog.m_nProvisionedFixMobBackBW + 
				hNetMan.m_hLog.m_nProvisionedFixedBackBW));
		text = "\nBlocked BH BWD for blocked bh connections: ";
		fprintf(out, "%s%d", text, hNetMan.m_hLog.m_nBlockedBHForBHBlock);
		text = "\nBlocked FH BWD for blocked fh connections: ";
		fprintf(out, "%s%d\n", text, hNetMan.m_hLog.m_nBlockedFHForFHBlock);

		text = "\nBlocking probability - BACKHAUL (Overall; BWD) = Blocked backhaul bwd / total backhaul bwd = ";
		fprintf(out, "%s%d / %d = %f\n", text, (hNetMan.m_hLog.m_nBlockedMobileBackBW + hNetMan.m_hLog.m_nBlockedFixedBackBW + hNetMan.m_hLog.m_nBlockedFixMobBackBW),
			(hNetMan.m_hLog.m_nProvisionedMobileBackBW + hNetMan.m_hLog.m_nProvisionedFixMobBackBW + hNetMan.m_hLog.m_nProvisionedFixedBackBW +
				hNetMan.m_hLog.m_nBlockedMobileBackBW + hNetMan.m_hLog.m_nBlockedFixedBackBW + hNetMan.m_hLog.m_nBlockedFixMobBackBW),
				(((float)((float)hNetMan.m_hLog.m_nBlockedMobileBackBW + (float)hNetMan.m_hLog.m_nBlockedFixedBackBW + (float)hNetMan.m_hLog.m_nBlockedFixMobBackBW)) /
			(((float)((float)hNetMan.m_hLog.m_nBlockedMobileBackBW + (float)hNetMan.m_hLog.m_nBlockedFixedBackBW + (float)hNetMan.m_hLog.m_nBlockedFixMobBackBW)) +
					((float)((float)hNetMan.m_hLog.m_nProvisionedMobileBackBW + (float)hNetMan.m_hLog.m_nProvisionedFixMobBackBW +
				(float)hNetMan.m_hLog.m_nProvisionedFixedBackBW)))));

		if (hNetMan.m_hLog.m_nBlockedCon > 0)
		{
			text = "\nBlocking probability because of latency: ";
			fprintf(out, "%s%f", text, ((double)((double)hNetMan.m_hLog.m_nBlockedConDueToLatency / (double)hNetMan.m_hLog.m_nBlockedCon)));
			text = "\nBlocking probability because of unreachability: ";
			fprintf(out, "%s%f\n", text, ((double)((double)hNetMan.m_hLog.m_nBlockedConDueToUnreach / (double)hNetMan.m_hLog.m_nBlockedCon)));
		}
		else
		{
			text = "\nBlocking probability because of latency: ";
			fprintf(out, "%s%d / %d", text, hNetMan.m_hLog.m_nBlockedConDueToLatency, hNetMan.m_hLog.m_nBlockedCon);
			text = "\nBlocking probability because of fity: ";
			fprintf(out, "%s%d / %d\n", text, hNetMan.m_hLog.m_nBlockedConDueToUnreach, hNetMan.m_hLog.m_nBlockedCon);
		}

		text = "\nAVERAGE LATENCY:\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf (out, "%f ", hNetMan.m_hLog.AvgLatency_hour[oo]);
		}
		//text = "BLOCKING PROBABILITY (Overall; BWD): "; 
		//fprintf(out, "%s%f / %f = %f\n", text, (double)hNetMan.m_hLog.m_nBlockedBW, ((double)hNetMan.m_hLog.m_nBlockedBW + (double)hNetMan.m_hLog.m_nProvisionedBW),
			//((double)hNetMan.m_hLog.m_nBlockedBW / ((double)hNetMan.m_hLog.m_nBlockedBW + (double)hNetMan.m_hLog.m_nProvisionedBW)));
		text = "\nBlocking probability (connections; overall):\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf(out, "%f ", hNetMan.m_hLog.Pblock_hour[oo]);
		}
		text = "\nBLOCKING PROBABILITY (BWD; overall):\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf(out, "%f ", hNetMan.m_hLog.PblockBW_hour[oo]);
		}
		text = "\n\nBlocking probability fronthaul (connections; overall):\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf(out, "%f ", hNetMan.m_hLog.PblockFront_hour[oo]);
		}
		text = "\nBlocking probability fronthaul (BWD; overall):\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf(out, "%f ", hNetMan.m_hLog.PblockFrontBW_hour[oo]);
		}
		text = "\n\nBlocking probability backhaul (connections; overall):\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf(out, "%f ", hNetMan.m_hLog.PblockBack_hour[oo]);
		}
		text = "\nBLOCKING PROBABILITY BACKHAUL (BWD; overall):\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf(out, "%f ", hNetMan.m_hLog.PblockBackBW_hour[oo]);
		}
		
		text = "\nAVG NETWORK COST (weighted on time; overall):\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf(out, "%f ", hNetMan.m_hLog.Avg_NetCost_hour[oo]);
		}
		
		text = "\nActivity time of each node (simulation time  = ";
		fprintf(out, "%s%f):\n", text, (hNetMan.m_hLog.m_hSimTimeSpan - hNetMan.m_hLog.transitoryTime));
		//list<AbstractNode*>::const_iterator itr;
		//OXCNode*pNode;
		for (itr = hNetMan.m_hWDMNet.m_hNodeList.begin(); itr != hNetMan.m_hWDMNet.m_hNodeList.end(); itr++)
		{
			pNode = (OXCNode*)(*itr);
			if (pNode->m_dActivityTime > 0)
				fprintf(out, "Node #%d - activity time: %f\n", pNode->getId(), pNode->m_dActivityTime);
		}

		text = "\nHotels node of each node";
		fprintf(out, "%s:\n", text);
		//list<AbstractNode*>::const_iterator itr;
		//OXCNode*pNode;
		for (itr = hNetMan.m_hWDMNet.m_hNodeList.begin(); itr != hNetMan.m_hWDMNet.m_hNodeList.end(); itr++)
		{
			pNode = (OXCNode*)(*itr);
			if (pNode->m_vLogHotels.size() > 0)
			{
				fprintf(out, "Node #%d - hotels logged: ", pNode->getId());
				for (int i = 0; i < pNode->m_vLogHotels.size(); i++)
				{
					fprintf(out, "%d ", pNode->m_vLogHotels[i]);
				}
				fprintf(out, "\n");
			}
			else
			{
				assert(false);
			}
		}

		fprintf(out, "\nBACKHAUL (provisioned) carried load MONICA[BH BWD * time] = %f (coincides with: bh provisioned conn * holding time)\n", hNetMan.m_hLog.m_dSumCarriedLoad);

		fprintf(out, "Total number of lightpaths activated during the simulation: %d\n", hNetMan.m_hLog.m_nLightpath);

		fprintf(out, "Sum lightpath holding time(?) [SUM(lightpaths lifetime)]: %f\n", hNetMan.m_hLog.m_dSumLightpathHoldingTime);

		fprintf(out, "Sum lightpath load(?) [used lp cap / tot lp cap ; (weighted on time)]: %f\n", hNetMan.m_hLog.m_dSumLightpathLoad);

		fprintf(out, "Sum link load(?) [lifetime * hops * capacity]: %f\n", hNetMan.m_hLog.m_dSumLinkLoad);

		fprintf(out, "Total used OC1 time(?) [SUM(busyTime) = SUM(used lp cap * time)]: %f\n", hNetMan.m_hLog.m_dTotalUsedOC1Time);

		text = "\nCarico(???):\n";
		fprintf(out, "%s", text);
		for (int oo = 0; oo < 24; oo++)
		{
			fprintf(out, "%f ", hNetMan.m_hLog.carico[oo]);
		}

		fprintf(out, "\nReal avg arrival rate PER SECOND, PER NETWORK [#connections simulated / simulation time]: %f\n", hNetMan.m_hLog.NumOfConnections / hNetMan.m_hLog.m_hSimTimeSpan);
		fprintf(out, "Real avg arrival rate PER SECOND, PER NODE: %f\n", hNetMan.m_hLog.NumOfConnections / hNetMan.m_hLog.m_hSimTimeSpan / hNetMan.m_hWDMNet.numberOfNodes);
		fprintf(out, "Average amount of traffic generated per second, per network [OC1]: %f\n", (hNetMan.m_hLog.NumOfConnections / hNetMan.m_hLog.m_hSimTimeSpan) * BWDGRANULARITY);
		fprintf(out, "Average amount of traffic generated per second, per node [OC1]: %f\n", (hNetMan.m_hLog.NumOfConnections / hNetMan.m_hLog.m_hSimTimeSpan / hNetMan.m_hWDMNet.numberOfNodes) * BWDGRANULARITY);

		//std::map<UINT, vector<double> >::iterator itr;
		/*
				oo = 0;	//oo is defined far above
		text = "\n\nFiber load per hour:";
		fprintf(out, "%s", text);
		for (itr = hNetMan.m_hLog.UniFiberLoad_hour.begin(); itr != hNetMan.m_hLog.UniFiberLoad_hour.end(); oo++, itr++) //(int oo = 0; oo < 24; oo++)
		{
			text1 = "\n";
			fprintf(out, "%sORA %d\t", text1, oo);
			//-B: il vector uniFiberLoad era stato riempito a partire dall'ultimo link
			for (int i = (hNetMan.getNumberOfUniFibers() - 1), fiberId = 1; i >= 0; i--, fiberId++)
			{
				vect = itr->second; //vect is defined far above
				fprintf(out, "(#%d)%f ", fiberId, vect[i]);
			}
		}
		*/


	fclose(out);
	//out.close();	

}
cout << endl << "END!";
cin.get();
cin.get();
cin.get();
cin.get();
cin.get();
cin.get();

//_CrtDumpMemoryLeaks();
 return 0;
}



