#ifndef LIGHTPATH_H
#define LIGHTPATH_H

namespace NS_OCH {

class OXCNode;
class Circuit;
class AbstractLink;
class AbstractPath;
class UniFiber;
class WDMNetwork;
class NetMan;
class Log;


class Lightpath: public AbstractLink {
	Lightpath(const Lightpath&);

public:
	typedef enum {
		LPT_PAL_Unprotected = 0,	// protection at lightpath level
		LPT_PAL_Dedicated,	
		LPT_PAL_Shared,
		LPT_PAC_Unprotected,		// protection at connection level
		LPT_PAC_DedicatedPath,
		LPT_PAC_SharedPath,
		LPT_SPAC_SharedPath,		// segragate PAC
		LPT_SEG_NO_HOP,				// segmented protection
		LPT_SEG_B_HOP,
		LPT_SEG_PB_HOP
	} LPProtectionType;

	Lightpath();
	Lightpath(UINT, LPProtectionType hType = LPT_PAC_Unprotected, UINT nCap = OCLightpath);
	~Lightpath();
	const Lightpath& operator=(const Lightpath&);
	
	// PAC
	void increaseConflictSet(const Circuit&);
	void decreaseConflictSet(const Circuit&);
	void checkSanity() const;
	
	// PAL_SPP
	void attachBackup(list<AbstractLink*>&);
	void detachBackup();

	virtual void dump(ostream&) const;
	virtual LinkType getLinkType() const;

	void MPAC_SetUp(NetMan*, Circuit*);
	void MPAC_Release(bool&, NetMan*, bool bLog);

	void muxUpdateChannelUsage(list<UniFiber*>&, NetMan*);

	void BBU_WPsetUpLp(NetMan*, Circuit*);

	void updatePhysicalPath(UniFiber*);

	void Unprotected_setUpLightpath(NetMan*, Circuit*);

	void updateSimplexLinkLpCapacity(Lightpath*pLightpath, NetMan * pNetMan, UINT BWToBeAllocated);

	LINK_CAPACITY findLeastCapacityLinkInLightpath(NetMan*);

	void releaseLp(bool&, NetMan*, Circuit*, bool);

	void deleteCircuit(Circuit * pCircuit);

	void restoreUsedStatus(list<UniFiber*>& hPRoute, NetMan * pNetMan);

	void Unprotected_setUpOCLightpath(NetMan * pNetMan, Circuit * pCircuit);

	bool isPresentInLightpathDB(LightpathDB & hLightpathDB);

	virtual void setUp(NetMan*, Circuit*);
	virtual void release(bool&, NetMan*, Circuit*, bool bLog=true); 
	virtual void WPsetUp(NetMan*, Circuit*);//fabio 6 gennaio
	virtual void WPrelease(bool&, NetMan*, Circuit*, bool bLog=true);
	virtual void releaseOnServCopy(list<AbstractLink*>&);
	void muxTearDownOneRoute(list<UniFiber*>& hRoute);
	void consume(NetMan*, Circuit*);
	UINT getFreeCapacity() const;
	UINT getPhysicalHops() const;

	UINT countConnectionsDoingGrooming(NetMan*);

protected:
	void attachToWDM(list<UniFiber*>&, const AbstractPath&, WDMNetwork*);
	void updateChannelUsage(list<UniFiber*>&);
	//void updateChannelUsageTemp(list<UniFiber*>&);
	void tearDownOneRoute(list<UniFiber*>&);
	float calculateLpLength();
	//void tDORTemp(list<UniFiber*>&);
	void muxTearDownOneRoute(list<UniFiber*>&, NetMan*pNetMan);

public:
	bool allocateConflictSet(UINT nSize);
	OXCNode* getDstOXC()const;
	OXCNode* getSrcOXC()const;
	bool traverseUniFiber(const UniFiber*) const;
	virtual void logPeriodical(Log&, SimulationTime);
	virtual void logFinal(Log&);
//	inherited from AbstractLink
//	UINT				m_nLinkId;	// Unique Id
//	AbstractNode		*m_pSrc;	// src OXC
//	AbstractNode		*m_pDst;	// dst OXC
//	LINK_COST			m_hCost;
//	UINT				m_nLength;	// in km
	LPProtectionType	m_eProtectionType;
	LINK_CAPACITY		m_nCapacity;
	LINK_CAPACITY		m_nFreeCapacity;
	LINK_CAPACITY		m_nBackupCapacity;	// capacity used by backup
	list<Circuit*>		m_hCircuitList;

	// These two paths only have node ids
	AbstractPath		m_hPPhysicalPath;
	// valid only when m_eProtectionType is LPT_PAL_Dedicated or LPT_PAL_Shared
	// primary physical path is stored in m_hLinkList
	AbstractPath		m_hBPhysicalPath;

	// These two paths have pointers to UniFibers
	list<UniFiber*>		m_hPRoute;
	list<UniFiber*>		m_hBRoute;

	bool			    m_groomingAlreadydone; //initialized to false; once a grooming is done on the lightpath
											   //it will be set to true(and I will not consider the grooming latency another time)

	MappedLinkList<UINT,Connection*>	m_hConnectionsUsingLightpath;

	UINT				*m_pCSet;			// conflict set
	UINT				m_nCSetSize;
	UINT				m_nBWToBeAllocated;	// BW to be allocated in consume
	UINT				m_nBWToBeReleased;	// BW to be released in release

	int					wlAssigned	;		//FABIO: lambda assegnata durante instradamento,caso wp
	int					wlAssignedBCK	;		//FABIO: ..idem ma per il backup
	vector<int>			wlsAssigned;	//-B: case vwp: different wavelenghts along the path 
	vector<int>			primaryLinkID; //FABIO: identita dei link attraversati dal primario
	vector<int>			primaryNodesID;
	int					backupLength; //FABIO:serve per il calcolo del resource overbuild per la protezione dedicata
	int					primaryLength;
	// for PAL_SPP						  
	list<AbstractLink*>	m_hBRouteInSG;	// backup route in the state graph

    double				UnAvLP;//-t
    double				UnAvLPM;//-t
    double				deltaLP;//-t

protected:
	bool				m_bStateUpdated;	// T: network state has been updated w/ 
								//	consideration the rc of this lightpath

	// for log
	// The life time of this lightpath & a Tx/Rx
	SimulationTime		m_hLifeTime;	
	// The busy time in terms of oc1
	SimulationTime		m_hBusyTime;
};

};
#endif
