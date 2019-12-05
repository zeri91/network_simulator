#ifndef SIMULATOR_H
#define SIMULATOR_H

//#include "WDMNetwork.h"
#include "EventList.h"
namespace NS_OCH {
class EventList;
class NetMan;
class Connection;

class Simulator : public OchObject {
	friend class WDMNetwork;
public:
	Simulator();
	~Simulator();
	virtual void dump(ostream&) const;

	virtual void run(SimulationTime);
	void showLog(ostream&) const;
	Connection * BBU_newConnection(Event * pEvent, int runningPhase);
	Connection * BBU_newConnection_Bernoulli(Event * pEvent, int runningPhase);
	Connection * BBU_newConnection_Bernoulli_NewBBU(Connection* pConTobeSwitched, int newBBU, Event* pEvent);
	UINT genMobileBwdMCell();
	UINT genMobileBwdSCell();
	UINT genFixedBwd();
	void saveStats(int&, Log&);
	UINT genSource(vector<OXCNode*>& nodesList);
	BandwidthGranularity genMidhaulBwd(BandwidthGranularity bwd);
	void updateCorrespondingDepEvent(Connection*);
	void genNextPoissonEvent(OXCNode*, SimulationTime);
	void computeNumOfActiveHotels();
	bool initialize(UINT, double, double, const char*, const char*, int);
	bool setNetworkManager(NetMan*);
	Connection* newConnection(SimulationTime);
	//	EventList*  getEventList();
	bool isBBUNode(UINT); //-B
	void updateBBUforallConnections(UINT newBBU, UINT m_nSrc, Event* pEvent);

public:
	typedef enum {
		BD_Zipf = 0,	// 1: 3: 12: 48: 192 = 192: 64:  16:  4: 1
		BD_SBC,			// 1: 3: 12: 48: 192 = 10K: 2K: 200: 20: 2
		BD_Sprint,		// 1: 3: 12: 48: 192 = 300: 20:   6:  4: 1
		BD_Uniform,
		BD_Debug48,		// 1: 3: 12: 48: 192 = 0: 0: 0: 1: 0
		BD_Debug192,	// 1: 3: 12: 48: 192 = 0: 0: 0: 0: 1
	} BandwidthDistribution;
	double			TotalCost;

private:
	UINT						m_nRunConnections;	// Add by ANDREA
	UINT						m_nTransitoryCon;	// Add by ANDREA
	UINT						m_nTotalConnections;
	BandwidthDistribution		m_eBWDistribution;	//-B: utilizzato solo in una parte (per di più commentata) del metodo newConnection
	ProtectionClass  			m_ePClass;

	double			m_dArrivalRate;
	double			m_dArrivalRate_var;
	double			m_dMeanHoldingTime;

	EventList		m_hEventList;
	EventList		m_hFrontEventList;

	UINT			m_nNumberOfOXCNodes;
	UINT			m_nNumberOfNodePairs;	// no self-loop

	UINT			m_nHopCount; //-B: max num of hops (max path length)
public:
	NetMan			*m_pNetMan; //fabio:reso pubblico
	//---------- Andamento traffico variabile, profilo di traffico preso da INTERNET project
	double			tassiarrivo[24] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }; //-B
					//{ 0.4,0.3,0.2,0.2,0.3,0.2,0.15,0.2,0.3,0.4,1,0.9,0.8,0.8,0.8,0.9,1,0.95,0.9,0.75,0.6,0.95,1.25,1.1 };
	UINT			m_nNumOfActiveHotels;
	//UINT			midhaul_id[200000] = { 0 }; // -L: too big -> buffer overflow 
	UINT			backhaul_id[200000] = { 0 };
};

};
#endif
