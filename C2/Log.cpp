#pragma warning(disable: 4786)
#include <assert.h>
#include <iostream>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "AbstractPath.h"
#include "Connection.h"
#include "Log.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

Log::Log() : m_hSimTimeSpan(0),
m_nProvisionedCon(0), m_nBlockedCon(0), m_nProvisionedMobileFrontConn(0), m_nProvisionedFixMobFrontConn(0),
m_nProvisionedFixedBackConn(0), m_nProvisionedMobileBackConn(0), m_nProvisionedFixMobBackConn(0),
m_nBlockedConDueToUnreach(0), m_nBlockedConDueToLatency(0), m_nProvisionedBW(0), m_nBlockedBW(0), m_nProvisionedMobileFrontBW(0),
m_nProvisionedFixMobFrontBW(0), m_nProvisionedFixedBackBW(0), m_nProvisionedMobileBackBW(0), m_nProvisionedFixMobBackBW(0),
m_nBlockedMobileFrontBW(0), m_nBlockedFixMobFrontBW(0), m_nBlockedFixedBackBW(0), m_nBlockedMobileBackBW(0), m_nBlockedFixMobBackBW(0),
m_nBlockedBWDueToUnreach(0), m_nBlockedMobileFrontConn(0), m_nBlockedFixMobFrontConn(0), m_nBlockedFixedBackConn(0),
m_nBlockedMobileBackConn(0), m_nBlockedFixMobBackConn(0), m_nBlockedBHForBHBlock(0), m_nBlockedFHForFHBlock(0),
m_nLightpath(0), m_dSumLightpathLoad(0.0), m_dSumLinkLoad(0.0),
m_nPHopDistance(0), m_nBHopDistance(0), avgLatency(0), countConnForLatency(0),
m_dSumTxLoad(0.0), m_dSumLightpathHoldingTime(0.0),
m_nSumPhysicalPHops(0), m_nSumPhysicalBHops(0), avgActiveLightpaths(0),
m_dTotalUsedOC1Time(0), m_dSumCarriedLoadMonica(0), m_dSumCarriedLoad(0), m_dSumLightpath(0), avgActiveBBUs(0),
m_nBlockedDueToTx(0), m_nBlockedBWDueToTx(0), avgActiveNodes(0), peakActiveSC(0), avgActiveSC(0),
m_nSegments(0), m_dPrimaryRc(0), m_nBlockedConDueToPath(0), UnAvLPMedia(0),deltaMedio(0), N(0),
groomingPossibilities(0), networkCost(0),
peakNumLightpaths(0), peakNumActiveNodes(0), peakNetCost(0), peakNumActiveBBUs(0),
NumOfConnections(0), transitoryTime(0), totalBBUsChanged(0), m_nNumConnectionsChangingBBU(0
)
{
	memset(m_pProvisionedConPerBW, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pBlockedConPerBW, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pLightpathHopsForConP, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pLightpathHopsForConB, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pHoldingTimePerBW, 0, sizeof(SimulationTime)*NumberOfBWGranularity);
	memset(m_pPhysicalHopsForConP, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pPhysicalHopsForConB, 0, sizeof(UINT)*NumberOfBWGranularity);
}

Log::~Log()
{
}

 void Log::resetLog()
{
	m_hSimTimeSpan = 0;
	m_nProvisionedCon = 0;
	m_nBlockedCon = 0;
	m_nBlockedConDueToUnreach = 0;
	m_nProvisionedBW = 0;
	m_nBlockedBW = 0;
	m_nBlockedBWDueToUnreach = 0;
	m_nLightpath = 0;
	m_dSumLightpathLoad = 0.0;
	m_dSumLinkLoad = 0.0;

	m_nBlockedBHForBHBlock = 0;
	m_nBlockedFHForFHBlock = 0;
	
	//-B
	m_nProvisionedFixedBackConn = 0;
	m_nProvisionedFixMobBackConn = 0;
	m_nProvisionedFixMobFrontConn = 0;
	m_nProvisionedMobileBackConn = 0;
	m_nProvisionedMobileFrontConn = 0;
	//-B:
	m_nBlockedConDueToLatency = 0;
	m_nBlockedFixedBackConn = 0;
	m_nBlockedFixMobBackConn = 0;
	m_nBlockedFixMobFrontConn = 0;
	m_nBlockedMobileBackConn = 0;
	m_nBlockedMobileFrontConn = 0;
	//-B:
	m_nProvisionedMobileFrontBW = 0;
	m_nProvisionedFixMobFrontBW = 0;
	m_nProvisionedFixedBackBW = 0;
	m_nProvisionedFixMobBackBW = 0;
	m_nProvisionedMobileBackBW = 0;
	//-B:
	m_nBlockedMobileFrontBW = 0;
	m_nBlockedFixMobFrontBW = 0;
	m_nBlockedFixedBackBW = 0;
	m_nBlockedFixMobBackBW = 0;
	m_nBlockedMobileBackBW = 0;

	m_nPHopDistance = 0;
	m_nBHopDistance = 0;
	m_dSumTxLoad = 0.0;
	m_dSumLightpathHoldingTime = 0.0;
	m_nSumPhysicalPHops = 0;
	m_nSumPhysicalBHops = 0;
	m_dTotalUsedOC1Time = 0;
	m_dSumCarriedLoadMonica = 0;
	m_dSumCarriedLoad = 0;
	m_dSumLightpath = 0;
	m_nBlockedDueToTx = 0;
	m_nBlockedBWDueToTx = 0;
	m_nSegments = 0;
	m_dPrimaryRc = 0;
	m_nBlockedConDueToPath = 0;

	//-B:
	peakNumLightpaths = 0;
	avgActiveLightpaths = 0;
	peakNumActiveNodes = 0;
	avgActiveNodes = 0;
	peakNumActiveBBUs = 0;
	avgActiveBBUs = 0;
	peakActiveSC = 0;
	avgActiveSC = 0;
	peakNetCost = 0;
	networkCost = 0;
	avgLatency = 0;
	countConnForLatency = 0;
	NumOfConnections = 0;
	groomingPossibilities = 0;
	totalBBUsChanged = 0;
	m_nNumConnectionsChangingBBU = 0;

	memset(m_pProvisionedConPerBW, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pBlockedConPerBW, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pLightpathHopsForConP, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pLightpathHopsForConB, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pHoldingTimePerBW, 0, sizeof(SimulationTime)*NumberOfBWGranularity);
	memset(m_pPhysicalHopsForConP, 0, sizeof(UINT)*NumberOfBWGranularity);
	memset(m_pPhysicalHopsForConB, 0, sizeof(UINT)*NumberOfBWGranularity);
}

void Log::output(ostream& out) const
{
	cout << "-> HLOG OUTPUT #2" << endl;

	// connection info
	out<< "Provisioned Connections: " << m_nProvisionedCon << '\t';
	out<< "\nBlocked connections: " << m_nBlockedCon << '\t';
	out << endl;

	// segment info
	out<<"Num of segments: " << m_nSegments << '\t';
	out<<"\nPrimary Hop Distance: " << m_nPHopDistance << '\t';
	out<< "\nBackup Hop distance" << m_nBHopDistance << '\t';
	out << endl;
	
	// for computing the resource-efficiency ratio
	out.precision(4);
	out<< "- ROUTE-COMPUTATION-EFFECTIVENESS-RATIO: " << m_dSumCarriedLoad << '\t';
	out.precision(4);
	out<< "\n- SUM LINK LOAD: " << m_dSumLinkLoad<<'\t';
	out.precision(4);
	out<<"\nPrimary Rc(?): " << m_dPrimaryRc << '\t';
	out.precision(4);
	out<< "\nLink load - Primary Rc: " << (m_dSumLinkLoad - m_dPrimaryRc) << '\t';
	out << endl;

	// for computing link utilization
	out<< "- SIMULATION TIME SPAN: " << m_hSimTimeSpan <<'\t';
	out<< "\nBlocked Connection due to path: " << m_nBlockedConDueToPath << '\t';
	out << endl;

	//for computing Resource Overbuild  ---------------t
    out.precision(4);
    out<< "?: " << (m_dSumLinkLoad - m_dPrimaryRc)/m_dPrimaryRc<<'\t';
	out << endl;

	//for computing UnAvMedia
    out.precision(4);
    out<< "UnAvLPMedia: " << (double) UnAvLPMedia/N;
	out << endl;
    
    //out.precision(4);
	//out<<" (1-Delta)="<<(double)1-((double)deltaMedio/N);

	//if(m_nProvisionedCon+m_nBlockedCon>=10000) out<<"\a";
}


void Log::output() const
{
	cout << "-> xLog output" << endl;

	//-B: Blocked connections
	double conn_block = this->m_nBlockedCon; // # connessioni bloccate (fino a questo istante)
	double mobileFrontConn_block = this->m_nBlockedMobileFrontConn;
	double fixMobFrontConn_block = this->m_nBlockedFixMobFrontConn;
	double fixedBackConn_block = this->m_nBlockedFixedBackConn;
	double mobileBackConn_block = this->m_nBlockedMobileBackConn;
	double fixMobBackConn_block = this->m_nBlockedFixMobBackConn;
	double unreachConn_block = this->m_nBlockedConDueToUnreach;
	double latencyConn_block = this->m_nBlockedConDueToLatency;
	//-B: Blocked bandwidth
	double bwd_block = this->m_nBlockedBW;
	double mobileFrontBwd_block = this->m_nBlockedMobileFrontBW;
	double fixMobFrontBwd_block = this->m_nBlockedFixMobFrontBW;
	double fixedBackBwd_block = this->m_nBlockedFixedBackBW;
	double mobileBackBwd_block = this->m_nBlockedMobileBackBW;
	double fixMobBackBwd_block = this->m_nBlockedFixMobBackBW;
	//-B: Provisioned connections
	double conn_prov = this->m_nProvisionedCon;	// # connessioni instaurate (fino a questo istante) //-B: fronthaul+backhaul should have been counted as a single connection
	double mobileFrontConn_prov = this->m_nProvisionedMobileFrontConn;
	double fixMobFrontConn_prov = this->m_nProvisionedFixMobFrontConn;
	double fixedBackConn_prov = this->m_nProvisionedFixedBackConn;
	double mobileBackConn_prov = this->m_nProvisionedMobileBackConn;
	double fixMobBackConn_prov = this->m_nProvisionedFixMobBackConn;
	//-B: Provisioned bandwidth
	double bwd_prov = this->m_nProvisionedBW;
	double mobileFrontBwd_prov = this->m_nProvisionedMobileFrontBW;
	double fixMobFrontBwd_prov = this->m_nProvisionedFixMobFrontBW;
	double fixedBackBwd_prov = this->m_nProvisionedFixedBackBW;
	double mobileBackBwd_prov = this->m_nProvisionedMobileBackBW;
	double fixMobBackBwd_prov = this->m_nProvisionedFixMobBackBW;
	//-B: get some values for connections
	double tot_conn = conn_block + conn_prov; // # connessioni totali //-B: it counts a provisioned/blocked fronthaul+backhaul connection as a single one
	double frontConn_prov = mobileFrontConn_prov + fixMobFrontConn_prov;
	double backConn_prov = fixedBackConn_prov + mobileBackConn_prov + fixMobBackConn_prov;
	double frontConn_block = mobileFrontConn_block + fixMobFrontConn_block;
	double backConn_block = fixedBackConn_block + mobileBackConn_block + fixMobBackConn_block;
	//-B: get some values for bandwiths
	double tot_bwd = bwd_block + bwd_prov; // # banda totale
	double frontBwd_prov = mobileFrontBwd_prov + fixMobFrontBwd_prov;
	double backBwd_prov = fixedBackBwd_prov + mobileBackBwd_prov + fixMobBackBwd_prov;
	double frontBwd_block = mobileFrontBwd_block + fixMobFrontBwd_block;
	double backBwd_block = fixedBackBwd_block + mobileBackBwd_block + fixMobBackBwd_block;
	//-B: blocking probabilities
	double Pblock = conn_block / tot_conn;
	double Pblock_front = frontConn_block / (frontConn_block + frontConn_prov);
	double Pblock_back = backConn_block / (backConn_block + backConn_prov);
	double Pblock_unreach = unreachConn_block / conn_block;
	double Pblock_latency = latencyConn_block / conn_block;
	assert((unreachConn_block + latencyConn_block) == conn_block);
	double Pblock_Bwd = bwd_block / tot_bwd;
	double Pblock_frontBwd = frontBwd_block / (frontBwd_block + frontBwd_prov);
	double Pblock_backBwd = backBwd_block / (backBwd_block + backBwd_prov);

	cout << "++++++++++++++++++++++++++++++++++ STATS TIME +++++++++++++++++++++++++++++++++++++++++" << endl;
	cout << "- Connessioni provisioned = " << conn_prov << " ; Connessioni bloccate = " << conn_block << endl;
	cout << "- PROBABILITA' DI BLOCCO (in terms of connections) = #connessioni bloccate / #connessioni totali = "
		<< conn_block << "/" << tot_conn << " = "; cout.precision(10); cout << Pblock; cout << endl;
	cout << "- PROBABILITA' DI BLOCCO PER FRONTHAUL CONNECTIONS = #connessioni fronthaul bloccate / #connessioni fronthaul totali = "
		<< frontConn_block << "/" << (frontConn_block + frontConn_prov) << " = "; cout.precision(10); cout << Pblock_front << endl;
	cout << "- PROBABILITA' DI BLOCCO PER BACKHAUL CONNECTIONS = #connessioni backhaul bloccate / #connessioni backhaul totali = "
		<< backConn_block << "/" << (backConn_block + backConn_prov) << " = "; cout.precision(10); cout << Pblock_back << endl;
	cout << "- Probabilita' di connessioni bloccate a causa dell'unreachability = #connessioni bloccate per unreach / #connessioni bloccate = "
		<< unreachConn_block << "/" << conn_block << " = "; cout.precision(10); cout << Pblock_unreach << endl;
	cout << "- Probabilita' di connessioni bloccate a causa della latenza = #connessioni bloccate per latency / #connessioni bloccate = "
		<< latencyConn_block << "/" << conn_block << " = "; cout.precision(10); cout << Pblock_latency << endl;
	cout << "++++++++++++++++++++++++++++++++++++++ BWD ++++++++++++++++++++++++++++++++++++++++++++" << endl;
	cout << "- PROBABILITA' DI BLOCCO (in terms of bandwidth) = " << bwd_block << "/" << tot_bwd << " = " << Pblock_Bwd << endl;
	cout << "- PROBABILITA' DI BLOCCO PER FRONTHAUL BANDWIDTH = " << frontBwd_block << "/"
		<< (frontBwd_block + frontBwd_prov) << " = " << Pblock_frontBwd << endl;
	cout << "- PROBABILITA' DI BLOCCO PER BACKHAUL BANDWIDTH = " << backBwd_block << "/"
		<< (backBwd_block + backBwd_prov) << " = " << Pblock_backBwd << endl;
	cout << "+++++++++++++++++++++++++++++++ LOAD & OTHER STATS ++++++++++++++++++++++++++++++++++++" << endl;
	//cout << "- NUMBER OF GROOMING 'POSSIBILITIES': " << this->groomingPossibilities << endl;
	cout << "- NETWORK COST PESATO (num nodi attivi + num lightpath attivi): " 
		<< (this->networkCost / (this->m_hSimTimeSpan - this->transitoryTime)) << " [costo della rete * time span]" << endl;
	cout << "- SUM # LIGHTPATH (should be = 0, because it has not been used): " << this->m_dSumLightpath << endl;
	cout << "- SUM CARRIED LOAD (calcolato ad ogni iterazione; route-computation-effectiveness-ratio: ???): "
		<< this->m_dSumCarriedLoad << " [#connessioni bh nel db * time span]" << endl;
	cout << "- SUM LIGHTPATH HOLDING TIME (somma lifetime di tutti i lightpath attivati, from start till now): "
		<< this->m_dSumLightpathHoldingTime << " [sec]" << endl;
	cout << "- SUM LIGHTPATH LOAD (carico della rete in terms of capacity, from start till now): "
		<< this->m_dSumLightpathLoad << " [(used capacity OC1 * sec)/(tot capacity OC1 * sec)]" << endl;
	cout << "- SUM LINK LOAD (carico della rete in terms of hop <==> link): " << this->m_dSumLinkLoad 
		<< " [sec * hops * tot capacity OC1]" << endl;
	cout << "- NUM TOT HOPS (from start till now): " << this->m_nPHopDistance << " [hops]" << endl;
	cout << "- NUM TOT ACTIVATED LIGHTPATHS (from start till now): " << this->m_nLightpath << " [lightpaths]" << endl;
	cout << "- TOTAL USED OC1 IN TIME (somma dei busy time di tutti i lightpath attivati, from start till now): "
		<< this->m_dTotalUsedOC1Time << "  [OC1 * sec]" << endl;
	cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl << endl;
}