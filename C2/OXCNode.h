#ifndef OXCNODE_H
#define OXCNODE_H

#include <list>
#include "TypeDef.h"	// Added by ClassView

namespace NS_OCH {


	class AbstractNode;
	class Graph;
	class WDMNetwork;
	class Lightpath;

	class OXCNode : public AbstractNode {
	public:
		typedef enum {
			FullWavelengthConversion = 0,	// default
			PartialWavelengthConversion,
			WavelengthContinuous
		} WConversionCapability;

		typedef enum {
			FullGrooming = 0,		// default
			PartialGrooming,
			NoGrooming
		} GroomingCapability;

		typedef enum
		{
			Office = 0,
			Residential = 1
		} UrbanArea;

	public:
		OXCNode(UINT, WConversionCapability, GroomingCapability, UINT, UINT, int, int, int);
		OXCNode(UINT nNodeId, WConversionCapability eConversion, GroomingCapability eGrooming, UINT nTx, UINT nRx, int Nchan, int nDCFlag, int nTimeZone, int cellFlag, UrbanArea nArea);
		virtual ~OXCNode();
		OXCNode(const OXCNode&);
		const OXCNode& operator=(const OXCNode&);

		virtual void	dump(ostream& out) const;

		virtual void	addIncomingLink(AbstractLink*);
		virtual void	addOutgoingLink(AbstractLink*);
		void			setWDMNetwork(WDMNetwork*);

		void			scaleTxRxWRTNodalDeg(double);
		UINT			countOutChannels() const;
		UINT			countInChannels() const;

		void			genStateGraphHelper(Graph&, int&, int&) const;
		void			genStateAuxGraphHelper(Graph&, int&, int&, UINT&);

		bool isMacroCell();

		bool isSmallCell();

		void resetCost();

		void updateHotelCostMetricForP2(UINT, UINT);

		void updateHotelCostMetricForP0(UINT);

		void			genStateGraphFullConversionEverywhereHelper(Graph&, int&, int&) const;
		bool			getBBUHotel();

	protected:
		// immutable attributes
	//	UINT					m_nName;
	//	list<UniFiber*>			m_hOFiberList;	// outgoing-fiber list
	//	list<UniFiber*>			m_hIFiberList;	// incoming-fiber list
		WConversionCapability	m_eConversion;
		GroomingCapability		m_eGrooming;
		UINT					m_nTx;			// # of transmitters
		UINT					m_nRx;			// # of receivers

		WDMNetwork* m_pWDMNetwork;	// the WDM network this node is in
		int						m_nMaxW;		// the maximum of incoming/outgoing
												// w's, used for state graph
		UINT					m_DCFlag; //= 1 -> mobile; = 2 -> fixed; = 3 -> fixed-mobile
		UINT 					m_TimeZone;
		UINT					m_cellFlag; //= 0 -> MACRO cell; = 1 -> SMALL cell
		bool					bbuHotel; //= true, it means that it is a candidate bbu hotel node

		UrbanArea				m_nArea;

	public:
		UINT					getNumberOfRx() const;
		UINT					getNumberOfTx() const;
		UINT					getDCFlag() const;
		UINT					getTimeZone() const;
		WConversionCapability	getConversion();
		GroomingCapability		getGrooming();
		UrbanArea				getArea();
		SimulationTime			m_dActivityTime;
		vector<UINT>			m_vLogHotels;
		double					m_uNumOfBBUSupported; //-B: see computeNumOfActiveHotels to understand what it means

		//-L
		int findCU(int key);
		int findDU(int key);
		bool removeCU(int key);
		bool removeDU(int key);

		double getTrafficProcessed();

		UINT getNumberOfSources();

		UINT getNumberOfBusySources();

		void resetActivityTime();

		//-L
		UINT getNumberOfDUs();
		UINT getNumberOfCUs();

		// mutable attributes
		UINT					m_nFreeTx;
		UINT					m_nFreeRx;

		//-B: in case of BBUSTACKING it is the number of active BBUs in a hotel node
		//	in case of BBUPOOLING_evolved it is the number of connection served by a pooling node
		//	(the problem is that most of prints, related to the first meaning, don't have sense for INTER_BBUPOOLING)!!!!!!!!!!!!
		int						m_nBBUs; // 0 for all nodes, both nodes that are not hotels and for hotel nodes
										 // num of BBU actives in this hotel node (obviously, in case it is an hotel node)
		int						m_nCUs; // -L: 0 for all nodes, numof CU actives in this hotel node
		int						m_nDUs; //-L

		map<int, int>				DUs_placed; //-L
		map<int, int>				CUs_placed; //-L
		map<int, int>::iterator		it; //-L

		LINK_COST				m_nBBUReachCost;			//cost-distance from a node to THIS candiate hotel
		LINK_COST				m_nNetStage;				//cost-distance of a node from core CO
		int						m_nReachabilityDegree;		//-B: num of mobile nodes that can reach THIS OXC (hotel) node
		list<AbstractLink*>		pPath;						//-B: needed while computing the best Hotel node -> path between a node and the core CO
		UINT					m_nBBUNodeIdsAssigned;		//initialized = 0 for all nodes		// hotel node id selected to contain the BBU of a certain node
		UINT					m_nCUNodeIdAssigned;		//-L: initialized = 0 for all nodes // hotel node id that cointain the CU of a certain node
		UINT					m_nBBUNodeIdsAssignedLast;  //initialized = 0 for all nodes. it cointains the last BBU associated to the node when it associates more than one BBU
		UINT					m_nProximityDegree;			//-B: similar principle to reachability degree --> OBSOLETE
															//	(#times a hotel is seen as the closest one for each node of the network)
		UINT					m_nBetweennessCentrality;	//-B: num of times a shortest path from any node to the core CO pass through THIS node
		double					m_dCostMetric;				//-B: see updateHotelCostMetricForP2() to see how it is computed
		float					m_fDistanceFromCoreCO;		//length in km of the shortest path from THIS hotel to the core CO
		double					m_dTrafficGen;				//amount of traffic having this node as source
		double					m_dTrafficProcessed;		//amount of traffic processed by this BBU pool node (==> valid only for BBU hotel nodes)
		double					m_dMaxTraffic;				//max amount of traffic generated by a macro cell node
		vector<OXCNode*>		m_vSmallCellCluster;		//cluster of small cells belonging to a macro cell node (NOT USED ANYMORE --> now smallcells are "hidden" sub-cells of a macrocell)
		SimulationTime			m_dExtraLatency;			//-B: small cell extra latency due to "hidden" path between SC and MC (then used in DijkstraLatencyHelper)
		vector<OXCNode*>		m_vReachableHotels;			//-B: list of all reachable hotel nodes from this node

		bool					m_bBBUAlreadyChanged;
		SimulationTime          m_hChangeBBUTime;
		UINT					m_nPreviousBBU;
		UINT					m_nCountBBUsChanged;

		//-B: NOT IMPLEMENTED YET
		OXCNode* m_pDefaultOXCNode;			//-B: it is the destination node off the default path [FOR FUTURE USE]
		SimulationTime			m_dDefaultLatency;			//-B: it is the lancy due to the propagation along the default path [FOR FUTURE USE]
		list<AbstractLink*>		m_lDefaultPath;				//-B: before the simulation, precompute default path only for those nodes that are in the tree part of the network
															//	(for those nodes having only one outgoing link) [FOR FUTURE USE]

		// for PAL2_SP
		WDMNetwork* m_pStateWDM;				// temp state
		bool					m_bNewLP;					// prev hop is new/existing lp
		list<Lightpath*>		m_hAuxOutgoingLpList; //-B
	};

	class POXCNodeComp : public less<OXCNode*>
	{
	public:
		bool operator()(OXCNode*& pLeft, OXCNode*& pRight) const
		{
			return (pLeft->m_nBBUReachCost > pRight->m_nBBUReachCost);
		}
	};

	class POXCNodeStageComp : public less<OXCNode*>
	{
	public:
		bool operator()(OXCNode*& pLeft, OXCNode*& pRight) const
		{
			return (pLeft->m_nNetStage > pRight->m_nNetStage);
		}
	};

	class POXCNodeReachComp : public less<OXCNode*>
	{
	public:
		bool operator()(OXCNode*& pLeft, OXCNode*& pRight) const
		{
			return (pLeft->m_nReachabilityDegree < pRight->m_nReachabilityDegree); //-B: sostituzione rispetto agli altri: < instead of >
																					//	because we want to have a decreasing ordered list
		}
	};

	class POXCNodeDistanceComp : public less<OXCNode*>
	{
	public:
		bool operator()(OXCNode*& pLeft, OXCNode*& pRight) const
		{
			return (pLeft->m_fDistanceFromCoreCO > pRight->m_fDistanceFromCoreCO);
		}
	};

	class POXCNodeMetricComp : public less<OXCNode*>
	{
	public:
		bool operator()(OXCNode*& pLeft, OXCNode*& pRight) const
		{
			return (pLeft->m_dCostMetric > pRight->m_dCostMetric);
		}
	};

};

#endif