#ifndef LOG_H
#define LOG_H

namespace NS_OCH {
class Connection;

class Log: public OchObject {
public:
	Log();
	~Log();
   	void resetLog();
	void output(ostream&) const;
	void output() const;
public:
	SimulationTime	m_hSimTimeSpan;

	// # of provisioned connections
	UINT	m_nProvisionedCon;	
	UINT	m_nBlockedCon;
	//-B:
	UINT	m_nProvisionedMobileFrontConn;
	UINT	m_nProvisionedFixMobFrontConn;
	UINT	m_nProvisionedFixedBackConn;
	UINT	m_nProvisionedMobileBackConn;
	UINT	m_nProvisionedFixMobBackConn;
	// segment performance gain, connections blocked by path but not segment
	UINT	m_nBlockedConDueToPath;	
	UINT	m_nBlockedConDueToUnreach;
	UINT	m_nBlockedDueToTx;
	//-B:
	UINT	m_nBlockedConDueToLatency;
	UINT	m_nBlockedMobileFrontConn;
	UINT	m_nBlockedFixMobFrontConn;
	UINT	m_nBlockedFixedBackConn;
	UINT	m_nBlockedMobileBackConn;
	UINT	m_nBlockedFixMobBackConn;
	// amount of BW (in terms of OC1) provisioned/blocked
	UINT	m_nProvisionedBW;	//both fronthaul and backhaul
	UINT	m_nBlockedBW;
	UINT	m_nBlockedBHForBHBlock;
	UINT	m_nBlockedFHForFHBlock;
	UINT	m_nBlockedBWDueToUnreach;
	UINT	m_nBlockedBWDueToTx;
	//-B:
	UINT	m_nProvisionedMobileFrontBW;
	UINT	m_nProvisionedFixMobFrontBW;
	UINT	m_nProvisionedFixedBackBW;
	UINT	m_nProvisionedFixMobBackBW;
	UINT	m_nProvisionedMobileBackBW;
	//-B:
	UINT	m_nBlockedMobileFrontBW;
	UINT	m_nBlockedFixMobFrontBW;
	UINT	m_nBlockedFixedBackBW;
	UINT	m_nBlockedFixMobBackBW;
	UINT	m_nBlockedMobileBackBW;

	// b/w distribution of provisioned/blocked connections
	UINT	m_pProvisionedConPerBW[NumberOfBWGranularity];
	UINT	m_pBlockedConPerBW[NumberOfBWGranularity];
	UINT	m_pLightpathHopsForConP[NumberOfBWGranularity];
	UINT	m_pLightpathHopsForConB[NumberOfBWGranularity];	// valid in PAC
	SimulationTime	m_pHoldingTimePerBW[NumberOfBWGranularity];
	// Avg lightpath load = m_dSumLightpathLoad / m_nLightpath
	UINT	m_nLightpath;	// # of lightpaths provisioned
	double	m_dSumLightpathHoldingTime;
	double	m_dSumLightpathLoad;
	double	m_dSumLinkLoad;	// in terms of OC1
	UINT	m_nPHopDistance; // sum of primary lightpath hop distances
	UINT	m_nBHopDistance;

	double	m_dSumTxLoad;
	double	m_dSumCarriedLoadMonica;
	double	m_dSumCarriedLoad;	// route-computation-effectiveness ratio
								// = m_dSumCarriedLoad / m_dSumLinkLoad
	double	m_dSumLightpath;	// m_dSumLightpath / total simulation time:
								// avg # of lightpath at any instance

	// valid in PAC
	UINT	m_nSumPhysicalPHops;
	UINT	m_nSumPhysicalBHops;
	UINT	m_pPhysicalHopsForConP[NumberOfBWGranularity];
	UINT	m_pPhysicalHopsForConB[NumberOfBWGranularity];	

	UINT	m_nSegments;
	double	m_dPrimaryRc;
//	double	m_dBackupRc = m_dSumLinkLoad - m_dPrimaryRc

	// for debug only
	double	m_dTotalUsedOC1Time;

    double  UnAvLPMedia;//-t
	double  deltaMedio;//-t
	int     N;//-t

	
	//-B:
	double carico[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double Pblock_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double NetCost_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double Avg_NetCost_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double AvgLatency_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double PblockFront_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double PblockBack_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double PblockBW_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double PblockFrontBW_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	double PblockBackBW_hour[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	map<UINT, vector<double> > UniFiberLoad_hour;
	vector<double> hotelsActivity;
	UINT peakNumLightpaths;
	double avgActiveLightpaths;
	UINT peakNumActiveNodes;
	double avgActiveNodes;
	UINT peakNumActiveBBUs;
	double avgActiveBBUs;
	UINT peakActiveSC;
	double avgActiveSC;
	UINT peakNetCost;
	float networkCost; //-B: weighted in time (more interesting than simple mean)
	float avgLatency; //-B: avgLatency += routing_time --> at the end: avgLatency = avgLatency / countConnForLatency
	UINT countConnForLatency;
	double NumOfConnections;
	double groomingPossibilities;
	SimulationTime transitoryTime;
	double totalBBUsChanged;
	double m_nNumConnectionsChangingBBU;
};

};
#endif
