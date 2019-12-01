#ifndef WDMNETWORK_H
#define WDMNETWORK_H

#include <map>
#include <list>
#include <vector>
#include "TypeDef.h"	// Added by ClassView

namespace NS_OCH {

class OXCNode;
class UniFiber;
class Graph;
class Connection;
class Circuit;
class NetMan;
class Event;
class ConnectionDB;
class Log;

class WDMNetwork: public AbstractGraph {
public:
	WDMNetwork();								//-B: set m_nChannelCapacity e m_dTxScale equal to 0. Set the full wavel conversion everywhere
	WDMNetwork(const WDMNetwork&);				//-B: reference of 'this' object points to WDMNetwork object passed as parameter
	WDMNetwork& operator=(const WDMNetwork&);	//-B: operator function to create a new WDMNetwork object equal to the WDMNetwork object passed as reference parameter
	virtual ~WDMNetwork();						//-B: nothing to do

	virtual void dump(ostream&)const;			//-B: print the network status (WDMNetwork object status)
												//(useful for debugging purpose; to be tried)
	UINT countRx() const;						//-B: count the total num of rx in the whole network
	UINT countTx() const;						//-B: count the total num of tx in the whole network
	UINT countOC1Links() const;					//-B: it seems to count all the channels of all the link in the network
												//	(la cosa strana è che la capcity credo sia 192, quindi si parlerebbe di OC192, non OC1)
	UINT countChannels() const;					//-B: return the num of channels of the first object (link) of the m_hLinkList list
												//	(I guess buecause the num of channles is the same for every link)
	LINK_COST hCost;
	// for SEG_SP with hop-count constraint on (p + b) path
	LINK_COST SEG_SP_PB_HOP_ComputeRoute(Circuit&, NetMan*, 
		OXCNode*, OXCNode*, Connection *pCon);

	// for SEG_SP with hop-count constraint on backup path 
	LINK_COST SEG_SP_B_HOP_ComputeRoute(Circuit&, NetMan*, 
		OXCNode*, OXCNode*, Connection *pCon);

	// for SEG_SP with graph tranformation
	LINK_COST SEG_SP_NO_HOP_ComputeRoute(Circuit&, NetMan*, 
		OXCNode*, OXCNode*);

    //for link protection
	LINK_COST SEG_SP_L_NO_HOP_ComputeRoute(Circuit&, NetMan*, //-t
		OXCNode*, OXCNode*, Connection *pCon);
	
	//for SEG_AGBSP
    LINK_COST SEG_SP_AGBSP_ComputeRoute(Circuit&, NetMan*, //-t
		OXCNode*, OXCNode*);

	
	// for SEG_SPP
	LINK_COST SEG_SPP_ComputeRoute(Circuit&, NetMan*, 
		OXCNode*, OXCNode*);
	LINK_COST wpSEG_SPP_ComputeRoute(Circuit&, NetMan*, 
		OXCNode*, OXCNode*);
	LINK_COST SEG_SPP_B_HOP_ComputeRoute(Circuit&, NetMan*, 
		OXCNode*, OXCNode*, Connection*);
	LINK_COST SEG_SPP_PB_HOP_ComputeRoute(Circuit&, NetMan*, 
		OXCNode*, OXCNode*, Connection*);

	//UNPROTECTED 9 dic
	LINK_COST UNPROTECTED_ComputeRoute(Circuit&, NetMan*, OXCNode*, OXCNode*);
	LINK_COST UNPROTECTED_ComputeRouteGreen(Circuit&, NetMan*, OXCNode*, OXCNode*);
	
	LINK_COST wpUNPROTECTED_ComputeRoute(Circuit& ,NetMan *,  OXCNode* ,  OXCNode* );
	//DEDICATA 9 dic
	LINK_COST PAL_DPP_ComputeRoute(Circuit&, NetMan*, OXCNode*, OXCNode*);

	LINK_COST wpPAL_DPP_ComputeRoute(Circuit& ,NetMan* ,OXCNode* ,OXCNode* );

	//-B: MIE FUNZIONI DI PROVISIONING
	LINK_COST BBU_ComputeRoute(Circuit*, NetMan*, OXCNode*, OXCNode*, int);
	LINK_COST wpBBU_ComputeRoute(Circuit*hCircuit, NetMan * pNetMan, OXCNode * pSrc, OXCNode * pDst);
	
	bool checkPathExist(NetMan*, Lightpath*);
	bool isMobileNode(UINT nodeId); //-B
	bool isFixMobNode(UINT nodeId); //-B
	bool isFixedNode(UINT nodeId); //-B
	void setVectNodes(UINT numOfNodes);

	void genReachabilityGraphOCLightpath(AbstractGraph & hGraph, NetMan*) const;

	LINK_COST BBU_UNPROTECTED_ComputeRoute(list<AbstractLink*> hPPath, OXCNode * pSrc, OXCNode * pDst, UINT & channel);

	void BBU_WDMNetdump(ostream & out) const;

	void invalidateWlOccupationLinks(Circuit*pCircuit, Graph&pGraph); //-B

	void resetBBUsReachabilityCost(); //-B: reset BBUsReachCost = UNREACHABLE for all nodes

	void updateBBUsUseAfterBlock(Connection*pCon, ConnectionDB&);
	void updatePoolsUse(Connection * pCon, ConnectionDB & connDB);
	void updateBBUsUseAfterDeparture(Connection*pCon, ConnectionDB&);

	void removeActiveBBUs(OXCNode * pOXCBBUNode);

	void printHotelNodes();
	void printBBUs();

	UINT countBBUs();

	void logBBUInHotel();

	int countConnections(OXCNode * pOXCSrc, OXCNode * pOXCDst, ConnectionDB & connDB);

	void fillHotelsList();

	void sortHotelsList(Graph & m_hGraph);

	void sortHotelsList_Evolved(Graph & m_hGraph);

	void logPeriodical(Log&, SimulationTime);

	UINT countCandidateHotels();

	float computeLatency(list<AbstractLink*>& path);

	float computeLength(list<AbstractLink*>& path);

	UINT countActiveNodes();

	UINT countActivePools(ConnectionDB & connDB);

	void restoreNodeCost();

	void computeShortestPaths(Graph & m_hGraph);

	void setNodesStage(Graph&m_hGraph);

	void setReachabilityDegree(Graph & m_hGraph);

	void resetPreProcessing();

	void setProximityDegree(Graph & m_hGraph);

	void setBetweennessCentrality(Graph & m_hGraph);

	void genAreasOfNodes();

	void extractSameStageNodes(vector<OXCNode*>&auxHotelsList, list<OXCNode*>&sameStageNodes, LINK_COST nStage);

	void extractSameReachDegreeNodes(list<OXCNode*>& sameStageNodes, list<OXCNode*>& sameReachDegreeNodes, int reachDegree);

	void sortHotelsByDistance(vector<OXCNode*>&);

	void updateTrafficPerNode(Event*pEvent);

	void extractPotentialSourcesMC_Nconn(bool, vector<UINT>&, BandwidthGranularity, double);

	void extractPotentialSourcesMC_1conn(bool, vector<UINT>&, MappedLinkList<UINT, Connection*>&);

	void computeTrafficProcessed_BBUStacking(OXCNode * pNode);

	void computeTrafficProcessed_BBUPooling(OXCNode * pNode, MappedLinkList<UINT, Connection*>& connectionList);

	void extractPotentialSourcesSC_1conn(vector<UINT>& potentialSources, MappedLinkList<UINT, Connection*>& connectionList, UINT);

	void extractPotentialSourcesSC_Nconn(vector<UINT>& potentialSources, double, UINT macroCellID);

	bool isAlreadySource(UINT nodeID, MappedLinkList<UINT, Connection*>& connectionList);

	UINT countActiveSmallCells();

	void logHotelsActivation(SimulationTime hTimeSpan);

	void resetHotelStats();

	void computeNumOfBBUSupported(UINT);

	void computeNumOfBBUSupported_CONSERVATIVE(UINT offeredTraffic_MC);

	void sortHotelsListByMetric();
	
	//LINK_COST myAlg(AbsPath&,OXCNode*,OXCNode*,int&);
	void ExtractListUniFiber(list<AbstractLink*>&); //-B: build as many UniFiber object as the num of link in m_hLinkList and put them in the hLinkToBeDecr list passed as parameter
	void SEG_SPP_UpdateBackupCostWP(const list<AbstractLink*>&,set<Lightpath*>&,bool);
	void SEG_SPP_RestoreBackupCostWP(set<Lightpath*>& result); //fabio 4 genn
	void DEDICATED_UpdateBackupCostWP(const list<AbstractLink*>& ,set<Lightpath*>& ,bool);

	//BARCELLONA
	LINK_COST SPPBw_ComputeRoute(Circuit& ,NetMan* ,OXCNode* ,OXCNode* );
	LINK_COST SPPCh_ComputeRoute(Circuit& ,NetMan* ,OXCNode* ,OXCNode* );

	// for PAL2_SP
	LINK_COST PAL2_SP_ComputeRoute(Circuit&, NetMan*, OXCNode*, OXCNode*, 
			BandwidthGranularity);
	void PAL2_SP_ComputeRoute_ReleaseMem(OXCNode*);
	void PAL2_SP_ComputeRoute_RelaxOneHop(NetMan*, OXCNode*, OXCNode*, 
			BandwidthGranularity);
	LINK_COST PAL2_SP_ComputeRoute_RelaxOneHop_Aux(list<AbstractLink*>&, 
			list<AbstractLink*>&, OXCNode*, OXCNode*, 
			UINT, LINK_COST, BandwidthGranularity);
	void PAL2_SP_UpdateLinkCost_Backup(const list<AbstractLink*>&);
	WDMNetwork* PAL2_SP_ComputeRoute_RelaxOneHop_NewState(Lightpath&, 
			OXCNode*, OXCNode*,
			const list<AbstractLink*>&, const list<AbstractLink*>&);

	// wavelength-continuous case
	void setWavelengthContinuous(bool bWContinuous = true);
	void BBU_GenAuxGraph(Graph&, Circuit&, Graph&, UINT&); //-B
	void genStateGraph(Graph&) const;											//-B: see WDMNetwork.cpp to see comments on how it works
	void genStateGraphFullConversionEverywhere(Graph&) const;
	void genReachabilityGraphThruWL(AbstractGraph&) const;						//-B: Reachability graph: adjacency based on available wavelength links 
																				//	-> Clear the AbstractGraph object passed as parameter and fill it 
																				//	with those links which have at least one free channel
	void scaleTxRxWRTNodalDeg(double);											//-B: for each node of the network, it sets the num of tx/rx (and the num of free tx/rx)
																				//	equal to the num of outgoing/ingoing channels * dTxScale (0 < (double)dTxScale <= 1)
	virtual bool addNode(AbstractNode*);										//-B: add node passed as parameter to the m_hNodeList of AbstractGraph object
																				//	and creates a new OXCNode object that refers to the WDMNetwork is in
	UniFiber* addUniFiber(UINT, UINT, UINT, UINT, LINK_COST, float);				//-B: add a new UniFiber object (derived from AbstractLink object) to the AbstractGraph object.
																				//	Each UniFiber object has a Channel array, whose size is the num of wavel/channels
	UniFiber* addUniFiberOnServCopy(UINT, UINT, UINT, UINT, LINK_COST,UINT);	//-B: as addUniFiber, but it doesn't add the UniFiber object to the AbstractGraph.
																				//This way it returns a reference to a new Unifiber object
	void setChannelCapacity(UINT);												//-B: simply set the related var to the value passed as parameter

	UINT getChannelCapacity();
	
	double EstimateGreenPower(int,int,int); //-é*°çò@#@@@@@@@@@@@@@@@@@@@@@
	double SolarPower(int,int);
	double WindPower(int,int);
	double BrownPower(int,int);
										 //-é*°çò@#@@@@@@@@@@@@@@@@@@@@@
	// for debugging purpose
	UINT countFreeChannels() const;				//-B: for each link of the network, it counts the num of free channels over all the links
	void dumpcontrol(); //FABIO 5 nov			//-B: it checks, for each link of the network, if the real num of backup channels
												//	is equal to the value stored in the var m_nBChannels (attribute of the lin itself)

public:
	double	m_dEpsilon;					// use by the cost function in computing backup
	int nDC;							// numero di Data Center nella rete	
	int BestGreenNode;					// nodo con la massima disponibilità di energia
	int DummyNode;						// -L: nodeID of the backhaul destination
	int DummyNodeMid;					// -L: nodeID of the midhaul destination
	int DCused;			                // nodeID del DataCenter che sta elaborando la connessione in quel momento
	int DCvettALL[MAX_DC_NUMBER];		// vettore con tutti i DataCenter
	int TZvett[MAX_DC_NUMBER];			// vettore con tutti TimeZone dei rispettivi DataCenter
	int DCvettW[MAX_DC_NUMBER];			// vettore con i nodeId dei DataCenter WIND	
	int DCvettS[MAX_DC_NUMBER];			// vettore con i nodeId dei DataCenter SOLAR
	int DCvettH[MAX_DC_NUMBER];			// vettore con i nodeId dei DataCenter HYDRO
	int DCCount[MAX_DC_NUMBER];			// vettore contatore delle connessioni che passano per un DC
	int nconn[MAX_DC_NUMBER];				 
	double gp[MAX_DC_NUMBER];			//vettore con energia rinnovabile disponibile in ogni DataCenter
	double g_en_residual[MAX_DC_NUMBER];
	double DCprocessing[MAX_DC_NUMBER];
	double DCprocessing_old[MAX_DC_NUMBER];
	double EDFACost;					//consumo di potenza degli amplificatori ottici
	double NodeProcessingCost;			//consumo di potenza dovuto al processing dei nodi

	//-B: added by me
	UINT				*MobileNodes;
	UINT				*FixedNodes;
	UINT				*FixMobNodes;
	UINT				numMobileNodes;
	UINT				numFixedNodes;
	UINT				numFixMobNodes;
	vector<OXCNode*>		OfficeNodes;
	vector<OXCNode*>		ResidentialNodes;
	vector<OXCNode*>	BBUs; //candidate hotel nodes that are ACTIVE BBU hotel nodes (simple mobile nodes that host their own BBU don't belong to this list)
	vector<OXCNode*>	hotelsList; //all BBU candidate hotel nodes, sorted by several parameters
	vector<UINT>		groomingNodesIds; //a NetMan method (getGroomNodesList) build a list of all nodes ids in which grooming is performed
	float				consolidationFactor;
	UINT				m_nNumOfTrees;
	UINT				m_nNumOfRings;
	double				m_nAvgNodesCrossed; //-B: used in setBetweennessCentrality (not used anymore)

protected:
	void SEG_SP_NO_HOP_UpdateLinkCost_Backup(const list<AbstractLink*>&);
    void SEG_SP_NO_HOP_UpdateLinkCost_BackupInvalidate(const list<AbstractLink*>&);
    void SEG_SP_NO_HOP_UpdateLinkCost_Backup_ServCI(const list<AbstractLink*>&, const list<AbstractLink*>&, const SimulationTime, bool);
    
	void SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup(const list<AbstractLink*>&,const list<AbstractLink*>&);
    void SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_BackupInvalidate(const list<AbstractLink*>&,const list<AbstractLink*>&);
    void SEG_SEG_SP_AGBSP_aux_UpdateLinkCost_Backup_ServCI(const list<AbstractLink*>&,const list<AbstractLink*>&,const list<AbstractLink*>&, const SimulationTime, bool);
    LINK_COST SEG_SP_AGBSP_aux(list<AbsPath*>&,list<AbsPath*>&, const AbsPath&,NetMan*); //-t

	void SEG_SP_NO_HOP_Transform_Graph(list<AbstractLink*>&, const AbsPath&);
	void SEG_SP_NO_HOP_Parse_Backup_Segs(list<AbsPath*>&, const AbsPath&, 
			const list<AbstractLink*>&);
	LINK_COST SEG_SP_NO_HOP_Calculate_BSegs_Cost(const AbsPath&, 
			const list<AbsPath*>&);

	void SEG_SPP_UpdateLinkCost_Backup(const list<AbstractLink*>&,bool);	
        void SEG_SPP_UpdateLinkCost_BackupInvalidate(const list<AbstractLink*>&);
   	void SEG_SPP_UpdateLinkCost_BackupCI(const list<AbstractLink*>&, const SimulationTime, bool); //mah.. vediamo.. prob bisogna passare molti piu parametr
	void SEG_SPP_UpdateLinkCost_Backup_ServCI(const list<AbstractLink*>&, const list<AbstractLink*>&, const SimulationTime, bool);

	LINK_COST SEG_SP_B_HOP_ComputeBackup(const AbsPath&, list<AbsPath*>&,
			NetMan*, OXCNode*, OXCNode*, Connection*);
	LINK_COST SEG_SP_B_HOP_ComputeBSeg(list<AbstractLink*>&, 
			const list<AbstractNode*>&, const list<AbstractLink*>&, 
			Connection*);
	void SEG_SP_B_HOP_UpdateLinkCost_Backup(const list<AbstractLink*>&);
    void SEG_SP_B_HOP_UpdateLinkCost_BackupInvalidate(const list<AbstractLink*>&);
    void SEG_SP_B_HOP_UpdateLinkCost_Backup_ServCI(const list<AbstractLink*>&, const list<AbstractLink*>&, const SimulationTime, bool);

	LINK_COST SEG_SP_PB_HOP_ComputeBackup(const AbsPath&, list<AbsPath*>&,
			OXCNode*, OXCNode*, Connection*);
	LINK_COST SEG_SP_PB_HOP_ComputeBSeg(list<AbstractLink*>&, 
			const list<AbstractNode*>&, const list<AbstractLink*>&, 
			const list<AbstractLink*>&, Connection*);

	void SEG_SP_HOP_Reserve_BSeg(const list<AbstractLink*>&, 
			const list<AbstractLink*>&);
	void SEG_SP_HOP_Release_BSegs(const list<AbstractLink*>&, 
			const list<AbsPath*>&);

protected:
	UINT			m_nChannelCapacity;
	bool			m_bFullWConversionEverywhere;
	bool			m_bWContinuous;
	// if 0.0 < m_dTxScale <= 1.0, then the # of Tx(Rx) at node v is
	// m_dTxScale * v's out degree (in degree)
	double			m_dTxScale;	
	
public:
	vector<int> dbggap; //FABIO 17 ott:raccoglie id connessioni fallite
	vector<int> truecon; //24 ott:associato a dbggap: inserisco la vera id associata
	bool ConIdReq; //24 ott:indica se devo inserire l'id della connessione fallita in dbggap
};
};

#endif
