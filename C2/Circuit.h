#ifndef CIRCUIT_H
#define CIRCUIT_H

#include "TypeDef.h"	// Added by ClassView
namespace NS_OCH {

class Lightpath;
class Log;

class Circuit: public OchObject {
	Circuit(const Circuit&);
	const Circuit& operator=(const Circuit&);
public:
	typedef enum {
		CT_Unprotected = 0,	// unprotected
		CT_PAL_DPP,
		CT_PAL_SPP,
		CT_PAC_DPP_Primary,	// dedicated-path protected primary
		CT_PAC_DPP_Backup,
		CT_PAC_SPP_Primary,	// shared-path protected primary
		CT_PAC_SPP_Backup,
		CT_SPAC_SPP,
		CT_SEG				// seg. protection
	} CircuitType;

	Circuit(BandwidthGranularity, CircuitType, Circuit*);
	~Circuit();
	virtual void dump(ostream&) const;

	// for PAL2
	void PAL2_SetUp(NetMan*);
	void PAL2_TearDown(NetMan*);

	void BBU_setUpCir(NetMan * pNetMan);
	void BBU_WPsetUpCir(NetMan * pNetMan);
	void Unprotected_setUpCircuit(NetMan * pNetMan);
	void tearDownCircuit(NetMan* pNetMan, Connection* connectionDeparting, bool bLog = true);

	void Unprotected_setUpCircuitOCLightpath(NetMan * pNetMan);

	void WPtearDownCircuit(NetMan * pNetMan, bool bLog);

	void computeLatency(); //-B

	void assignBandwidthToAllocate(); //set all lightpaths'BWToBeAllocated = circuit.m_eBW

	bool checkGrooming();
	
	// for MPAC
	void MPAC_SetUp(NetMan*);
	void MPAC_TearDown(NetMan*, bool bLog);

	// for SPAC
	void SPAC_AttachBackup(NetMan*, list<AbstractLink*>&);
	void SPAC_DetachBackup(NetMan*);

	void setUp(NetMan*);
	void WPsetUp(NetMan*);//fabio 6 genn
	void WPtearDown(NetMan*, bool bLog=true);
	void tearDown(NetMan*, bool bLog=true);
	// methods for constructing a circuit
	void addVirtualLink(Lightpath*);

	void addFrontVirtualLink(Lightpath * pLink);

	// methods for accessing the circuit
	AbstractNode* getSrc() const;
	AbstractNode* getDst() const;

	UINT getPhysicalHops() const;
	void deleteTempLightpaths();
	UINT getLightpathHops() const;

public:
	float					latency; //-B: routing time
	list<Lightpath*>		m_hRoute;
	list<Lightpath*>		m_hRouteA;
	list<Lightpath*>		m_hRouteB;
	BandwidthGranularity	m_eBW;
	CircuitType				m_eCircuitType;
	Circuit					*m_pCircuit;	// points to the backup (P) if 
											// m_eCircuitType is CT_Primary (B)
											// NULL if CT_Unprotected
protected:
//	void increaseConflictSet();
//	void decreaseConflictSet();

	list<AbstractLink*>		m_hBRouteInSG;	// backup route in the state graph

public:
	typedef enum {
		CT_Ready = 0,
		CT_Setup,
		CT_Torndown,
		CT_SetupB,//Fabio 29 Sett :Nuovo flag
		CT_TorndownB
	} CircuitState;
	CircuitState			m_eState;
};


};



#endif
