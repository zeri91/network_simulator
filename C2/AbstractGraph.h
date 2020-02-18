
#ifndef ABSTRACTGRAPH_H
#define ABSTRACTGRAPH_H
#define WAVE 16
#include <list>
#include <vector>
#include <valarray>
#include <map>
#include <bitset>
#include <set>
#include <stdlib.h>		// for srand and rand

namespace NS_OCH {
	class AbstractNode;
	class Vertex;
	class Lightpath;
	class Circuit;
	class UniFiber;
	class Graph;
	class SimplexLink;
	class NetMan;
	class LightpathDB;
	class OXCNode;
	class Connection;

	struct wlStatusList{
	
	bitset<WAVE>  wlLabel;    //wl "rimanenti"
	vector<bool> nodeVisited; //per evitare i loop
	LINK_COST cost;
	AbstractLink* prev;
	int prevpos;

	wlStatusList(int nodes):cost(UNREACHABLE),prev(NULL),prevpos(-1)
	{
		nodeVisited=vector<bool>(nodes);
	}
	wlStatusList(bitset<WAVE> bs,int nodes):wlLabel(bs),cost(0),prev(NULL),prevpos(-1)
	{
		nodeVisited=vector<bool>(nodes);
	}

	wlStatusList(bitset<WAVE> bs,LINK_COST costo, AbstractLink* pr,int pp,int nodes):wlLabel(bs),
	cost(costo),prev(pr),prevpos(pp)	{
	nodeVisited=vector<bool>(nodes);
	}

/*	~wlStatusList(){

		
	}*/

	unsigned long int getLongLabel(){
		return (wlLabel.to_ulong());
	}



	};
///////////////////////////////////////////////////////////////////////////////
//
template<class Key, class T> class MappedLinkList;
class AbstractLink;
class AbstractNode: public OchObject {
public:
	AbstractNode();
	AbstractNode(UINT);
	AbstractNode(UINT, int);
	~AbstractNode();
	AbstractNode(const AbstractNode&);
	const AbstractNode& operator=(const AbstractNode&);
	virtual void dump(ostream&) const;
	UINT getId() const;
	LINK_COST getCost(); //-B
	AbstractLink* lookUpIncomingLink(UINT) const;
	AbstractLink* lookUpOutgoingLink(UINT) const;
	UINT inDegree() const;
	UINT outDegree() const;

	virtual void addIncomingLink(AbstractLink*);
	virtual void addOutgoingLink(AbstractLink*);
	bool valid()const;
	void validate();
	void invalidate();
	bool updateOutLinks(set<int>&);
	bool updateOutLinksBCK(set<int>&);
	bool updateOutLinksPartial(set<int>&);

public:
	// immutable attributes
	UINT	m_nNodeId;		// unique
	MappedLinkList<UINT, AbstractLink*> m_hILinkList; 
	MappedLinkList<UINT, AbstractLink*> m_hOLinkList; 

	// mutable attributes for algorithms
	AbstractLink*		m_pPrevLink;		// better to use previous link than
//	AbstractNode*		m_pPrevHop;			// previous node since there might be
											// multiple links between the same 
											// node pair.
	Lightpath*			m_pPrevLightpath;	//-B: for shortest lightpath computation

	LINK_COST			m_hCost;
	UINT				m_nHops;
	SimulationTime		m_dLatency;			//-B: needed in DijkstraLatency method

	// For MPAC_Optimize & SPAC_Optimize
	UINT				m_nBackupVHops;
	LINK_COST			*m_pBackupCost;

	list<AbstractLink*> excludedLinks; //fabio 15 dic: lo usero nella mia versione di Yen

	vector<wlStatusList*> statusList; //info sulle label d'instradamento
	valarray<LINK_COST> costi;
	valarray<LINK_COST> costiParziali; //FABIO 15 marzo:uso per yen wp
	bool				costiParzialiFlag; //FABIO 15 marzo: indica se ho assegnato costi parziali al nodo

	vector<AbstractLink*> prev;
	vector<AbstractLink*> prevParziali;
	//set<int> updates;//indici da aggiornare

	vector<bool>	updates; //indici da aggiornare
	bool			forceUpdate; //se true, accetta qualsiasi update del nodo

	set<Lightpath*> LPinNode;		//FABIO 18 genn: lista dei Lightpath passanti per il NODO


protected:
	bool			m_bValid;
};

///////////////////////////////////////////////////////////////////////////////
//
class AbstractLink: public OchObject {
	AbstractLink(const AbstractLink&);	
	// cann't be implemented cause don't know m_pSrc & m_pDst
	const AbstractLink& operator=(const AbstractLink&);
public:
	typedef enum {
		LT_Abstract = 0,
		LT_Simplex,
		LT_UniFiber,
		LT_Lightpath
	} LinkType;


public:
	AbstractLink();
	AbstractLink(int, AbstractNode*, AbstractNode*, LINK_COST, float);
	~AbstractLink();
	virtual void dump(ostream&) const;

	virtual LinkType getLinkType() const;
	int getId() const;
	AbstractNode* getSrc() const;
	AbstractNode* getDst() const;
	LINK_COST getCost() const;
    LINK_COST getSavedCost() const;
    LINK_COST getOriginalCost(bool) const;
	float getLength() const;
	void modifyCost(LINK_COST);
	void modifyCostCI(LINK_COST,bool);
	bool costModifed() const;
	void restoreCost();
	void invalidate();
	void validate();
	bool valid() const;
	int getUsedStatus(); 
    void restoreStatus();
	bool getValidity();


public:
	
	int				m_nLinkId;	// Unique Id
	AbstractNode	*m_pSrc;
	AbstractNode	*m_pDst;
	LINK_COST		m_hCost;
	float			m_nLength;	// in km
    int             m_used;
	float			m_latency;	//in microseconds
			
	// for MPAC_Optimize & SPAC_Optimize
	UINT			m_nBackupVHops;
	LINK_COST		*m_pBackupCost;

	// For PAL_SPP & SPAC_SPP
	// in PAL_SPP: 
	//		m_nBChannels = max{m_pCSet[i]}
	// in SPAC_SPP: 
	//		m_nBChannels = (max{m_pCSet[i]} + OCLightpath - 1)/OCLightpath
	UINT			m_nBChannels;	// # of backup channels
	UINT			*m_pCSet;		//reference to an array whose size is equal to the num of link/node
									//(depending on if it is LinkDijoint or NodeDisjoint) -> CSet = CONFLICT SET
	UINT			m_nCSetSize;

	valarray<LINK_COST>  wlOccupation;    //valori che indicano le wl libere
	valarray<LINK_COST>  wlOccupationBCK; //3 genn:per il backup..
	set<Lightpath*>		 LPinFiber	;		//FABIO 3 genn: lista dei Lightpath 

	double			m_dLinkLoad; //-B: to measure the link load of UniFiber (but could be useful also for other AbstractLink)

protected:
	bool			m_bValid;
	LINK_COST		m_hSavedCost;
	bool			m_bCostSaved;
};
 
///////////////////////////////////////////////////////////////////////////////
//
class AbsPath;
class AbstractPath;	// legacy
class AbstractNode;

class AbstractGraph: public OchObject {
public:
	
	UINT			numberOfChannels;
	UINT			numberOfLinks;
	UINT			numberOfNodes;
	typedef enum {
		LCF_ByHop = 0,
		LCF_ByOriginalLinkCost
	} LinkCostFunction;
	class PAbstractNodeComp: public less<AbstractNode*>
	{
	public:
		bool operator()(AbstractNode*& pLeft, 
						AbstractNode*& pRight) const {
			return (pLeft->m_hCost > pRight->m_hCost);
		}
	};
	class PAbsNodeComp4MPAC: public less<AbstractNode*>
	{
	public:
		bool operator()(AbstractNode*& pLeft, 
						AbstractNode*& pRight) const {
			UINT h;
			LINK_COST hLHS, hRHS;
			
			if ((UNREACHABLE == pLeft->m_hCost) 
				|| (NULL == pLeft->m_pBackupCost)) {
				hLHS = UNREACHABLE;
			} else {
				hLHS = pLeft->m_hCost;
				if (pLeft->m_pBackupCost) {
					for (h=0; h<pLeft->m_nBackupVHops; h++)
						hLHS += pLeft->m_pBackupCost[h];
				}
			}

			if ((UNREACHABLE == pRight->m_hCost)
				|| (NULL == pRight->m_pBackupCost)) {
				hRHS = UNREACHABLE;
			} else {
				hRHS = pRight->m_hCost;
				if (pRight->m_pBackupCost) {
					for (UINT h=0; h<pRight->m_nBackupVHops; h++)
						hRHS += pRight->m_pBackupCost[h];
				}
			}
			return (hLHS > hRHS);
		}
	};
	class PAbsPathComp: public less<AbsPath*> {
	public:
		bool operator()(AbsPath*& pLeft, AbsPath*& pRight) const {
			return (pLeft->getCost() > pRight->getCost());
		}
	};
public:
	AbstractGraph();
	AbstractGraph(const AbstractGraph&);
	~AbstractGraph();
	const AbstractGraph& operator=(const AbstractGraph&);
	virtual void dump(ostream&) const;

	// for PAL2_SP
	void allocateConflictSet(UINT);

	// algorithms
	void Yen(list<AbsPath*>&, AbstractNode* pSrc, AbstractNode* pDst, 
			UINT nNumberOfPaths, LinkCostFunction hLCF = LCF_ByHop);
	void YenGreen(list<AbsPath*>&, AbstractNode* pSrc, AbstractNode* pDst, 
			UINT nNumberOfPaths, LinkCostFunction hLCF = LCF_ByHop);
	void Yen(list<AbsPath*>&, UINT nSrc, UINT nDst, 
			UINT nNumberOfPaths, LinkCostFunction hLCF = LCF_ByHop);
	void Floyd(UINT nNumberOfPaths, LinkCostFunction hLCF = LCF_ByHop);

	void DijkstraHelperYen(AbstractNode* pSrc, AbstractNode* pDst, LinkCostFunction hLCF);

	LINK_COST BellmanFord(list<AbstractLink*>&, AbstractNode*, AbstractNode*,
				LinkCostFunction hLCF = LCF_ByHop);
	LINK_COST BellmanFord(AbstractPath&, UINT, UINT, 
				LinkCostFunction hLCF = LCF_ByHop);
	LINK_COST Suurballe(AbstractPath&, AbstractPath&, UINT, UINT, 
				LinkCostFunction hLCF = LCF_ByHop);
	LINK_COST Suurballe(AbstractPath&, AbstractPath&, AbstractNode*, 
				AbstractNode*, LinkCostFunction hLCF = LCF_ByHop);
	LINK_COST Dijkstra(AbstractPath&, UINT, UINT, 
				LinkCostFunction hLCF = LCF_ByHop);
	LINK_COST Dijkstra(list<AbstractLink*>&, AbstractNode*, AbstractNode*,
				LinkCostFunction hLCF = LCF_ByHop);
	LINK_COST DijkstraLatency(list<AbstractLink*>& hMinCostPath, AbstractNode * pSrc, AbstractNode * pDst, LinkCostFunction hLCF);
	LINK_COST recordMinCostPath(list<AbstractLink*>&, AbstractNode*);
	LINK_COST Dijkstra_BBU(BandwidthGranularity bw, Connection * pCon, list<AbstractLink*>& hMinCostPath,
		AbstractNode * pSrc, AbstractNode * pDst, LinkCostFunction hLCF); //-B


	LINK_COST Dijkstra(AbstractPath & hMinCostPath, AbstractNode * pSrc, AbstractNode * pDst, LinkCostFunction hLCF);
	LINK_COST recordMinCostPath(AbstractPath * hPath, AbstractNode * pDst);
	LINK_COST DijkstraLightpath(OXCNode * pSrc, OXCNode * pDst, Circuit & pCircuit, Graph&);
	void DijkstraHelperLP(OXCNode * pSrc, OXCNode * pDst, Graph&);
	LINK_COST recordMinCostPathLP(OXCNode * pDst, Circuit & pCircuit);
	void genAuxOutLightpathsList(OXCNode * pNode);
	void genAuxLightpathsList(BandwidthGranularity & bwd, LightpathDB&);
	void setSimplexLinkCostForGrooming(MappedLinkList<UINT, AbstractLink*>);
	void DijkstraHelper(AbstractNode*, AbstractNode*, 
				LinkCostFunction hLCF = LCF_ByHop);
	void DijkstraHelperLatency(AbstractNode * pSrc, AbstractNode * pDst, LinkCostFunction hLCF);
	double getLinkLatency(float linkLength);
	void DijkstraHelper_BBU(BandwidthGranularity bw, Connection * pCon, AbstractNode * pSrc, AbstractNode * pDst, LinkCostFunction hLCF);
	void invalidateSimplexLinkDueToFreeStatus(MappedLinkList<UINT, AbstractLink*> pOutLinkList);
	void invalidateOutSimplexLinkDueToCap(MappedLinkList<UINT, AbstractLink*> pOutLinkList, UINT nBW);
	//FABIO 4 dic

	LINK_COST myAlg(AbsPath & path, AbstractNode * pSrc, AbstractNode * pDst, int nchannels);
	//fabio 4 genn
	LINK_COST myAlgPartial(AbsPath& ,AbstractNode* ,AbstractNode* ,int);//fabio start
	
	LINK_COST myAlgBCK(AbsPath& ,AbstractNode* ,AbstractNode* ,int);

	void YenWP(list<AbsPath*>& ,AbstractNode*, AbstractNode*,UINT, LinkCostFunction );

	void YenHelperWP(list<AbsPath*>&,AbstractNode*,AbstractNode*,UINT,LinkCostFunction);

	LINK_COST SPPBw(AbsPath* ,AbstractNode* ,AbstractNode* ,int,bool);
	LINK_COST SPPCh(AbsPath* ,AbstractNode* ,AbstractNode* ,int,bool);

	void WPinvalidateRootPaths(AbstractNode*,AbstractNode*);
	// helper functions
	AbstractNode* lookUpNodeById(UINT) const;
	AbstractLink* lookUpLinkById(UINT) const;
	AbstractLink* lookUpLink(UINT, UINT) const;
	UINT getNumberOfNodes() const;
	UINT getNumberOfLinks() const;
	virtual bool addNode(AbstractNode *pNode);
	AbstractLink* addLink(int, UINT, UINT, LINK_COST, float);
	bool addLink(AbstractLink *pLink);
	void removeLink(AbstractLink*);
	void deleteContent();
	void reset4ShortestPathComputation();
	int getNextLinkId();
	void validateAllLinks();
	void validateAllNodes();
	void restoreLinkCost();

	void resetLinks();

public:
	// immutable attributes
	MappedLinkList<UINT, AbstractLink*> m_hLinkList;
	MappedLinkList<UINT, AbstractNode*> m_hNodeList;
	vector<int> channelsToDelete;
	list<Lightpath*>	auxLightpathsList;

protected:
	int		m_nNextLinkId;	// next eligible link id

protected:
	// algorithms: inline functions
	void YenHelper(list<AbsPath*>&, AbstractNode* pSrc, AbstractNode* pDst, 
			UINT nNumberOfPaths, LinkCostFunction hLCF);
	void YenHelperGreen(list<AbsPath*>&, AbstractNode* pSrc, AbstractNode* pDst, 
			UINT nNumberOfPaths, LinkCostFunction hLCF);
	void FloydHelper(UINT nNumberOfPaths, LinkCostFunction hLCF);
	void genCostMatrix(LINK_COST ***, UINT, LinkCostFunction hLCF);

	LINK_COST SuurballeHelper(AbstractPath&, AbstractPath&, AbstractNode*, 
				AbstractNode*, LinkCostFunction hLCF = LCF_ByHop);
	void SuurballeRestorer(list<AbstractLink*>&);
	void BellmanFordHelper(AbstractNode*, LinkCostFunction hLCF = LCF_ByHop);
	void DijkstraHelperGreen(AbstractNode*,AbstractNode*, LinkCostFunction hLCF = LCF_ByHop);
	//FABIO 4 dic
	void DijkstraHelper(AbstractNode*, LinkCostFunction hLCF = LCF_ByHop);
	void invalidateMinCostPath(AbstractNode*);
	void reverseMinCostPath(list<AbstractLink*>&, AbstractNode*);
	void groupTwoPaths(AbstractPath&, AbstractPath&);

	LINK_COST recordMinCostPath(AbstractPath&, UINT);
	LINK_COST recordMinCostPath(AbstractPath&, AbstractNode*);
	int randomChSelection(valarray<double>);
};

};
#endif
