#ifndef CONNECTION_H
#define CONNECTION_H
#include <list>


namespace NS_OCH {
class Circuit;
class Log;
class AbstractLink;
// class NetMan;

class Connection: public OchObject {
	Connection();
	Connection(const Connection&); 
	const Connection& operator=(const Connection&);

public:
	typedef enum {
		REQUEST = 1,	// connection request, not set up yet
		SETUP = 2,		// being set up
		DROPPED = 4,	// rejected
		TORNDOWN = 8	// done
	} ConnectionState;

	typedef enum {
		FIXED_BACKHAUL = 0,
		MOBILE_FRONTHAUL = 1,
		FIXEDMOBILE_FRONTHAUL = 2,
		MOBILE_BACKHAUL = 3,
		FIXEDMOBILE_BACKHAUL = 4,
		FIXED_MIDHAUL = 5  // -L
	} ConnectionType;

	Connection(UINT nSeqNo, UINT nSrc, UINT nDst, SimulationTime, SimulationTime, 
		BandwidthGranularity eBW, ProtectionClass ePC = PC_NoProtection);
	Connection(UINT nSeqNo, UINT nSrc, UINT nDst, SimulationTime tArrivalTime, SimulationTime tHoldingTime,
		BandwidthGranularity eBW, BandwidthGranularity CPRIbw, ProtectionClass ePC, ConnectionType connType);
	virtual ~Connection();

	virtual const char* toString() const; //stampa le info riguardanti la connessione nella stringa che restituisce(pStr)
	virtual void dump(ostream& out) const; //stampa le info riguardanti la connessione tramite il metodo toString() e aggiunge anche lo status della connessione considerata
	void log(Log&); //a seconda della stato della connessione, aumenta il num di connessioni provisioned o bloccate
	//	void tearDown(NetMan*, bool);

//public:
	// common attributes
	UINT					m_nSequenceNo;		// sequence number
	UINT					m_nSrc;				// src node
	UINT					m_nDst;				// dst node
	BandwidthGranularity    m_eBandwidth;		// bandwidth granularity (-B: bandwidth requested)
	BandwidthGranularity	m_eCPRIBandwidth;	// -B: CPRI bandwidth requested by mobile or fixed-mobile nodes
	ProtectionClass			m_eProtectionClass;	// type of protection
	ConnectionState			m_eStatus;			// status

	UINT					m_nHopCount;	// max. hop count
	UINT					m_nBackhaulSaved; // ID of the backhaul associated

	Circuit					*m_pPCircuit;	// primary if unprotected
	Circuit					*m_pBCircuit;	// backup if protected (but not PAL)
	list<Circuit*>			m_pCircuits;	//-B: needed for connections requiring a capacity > OCLightpath

	// attributes for given traffic

	// attributes for dynamic traffic
	SimulationTime			m_dArrivalTime;
	SimulationTime			m_dHoldingTime;
	SimulationTime			m_dRoutingTime;	//-B: delay due to routing
	//SimulationTime			m_dGroomingTime; //delay due to grooming
	bool					m_bBlockedDueToUnreach;
	bool					m_bBlockedDueToLatency; //-B
	bool					m_bBlockedDueToTx;
	bool					m_bTrafficToBeUpdatedForArrival; //if set to false, I dont' have to update the traffic per node (at the departure)
	bool					m_bTrafficToBeUpdatedForDep;
	ConnectionType			m_eConnType;
	UINT					grooming; //-B: count all the times that grooming is performed (not sure if it is performed correctly) -> NOT USED

	// attributes for later use
	bool					m_bCanBifurcate;	
	bool					m_bNodeDiversity;

	static UINT				g_pBWMap[NumberOfBWGranularity];

	list<AbstractLink*>		m_pPathToUse; //used for connections changing bbu: contains the path to be used to route the connection
	LINK_COST				m_hCostToUse; //used for connections changing bbu: contains the cost of the path
	float					m_dLatency;
};

};

#endif
