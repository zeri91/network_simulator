#ifndef GRAPH_H
#define GRAPH_H
#include <list>
#include <map>
#include "TypeDef.h"	// Added by ClassView
#include "SimplexLink.h"

namespace NS_OCH {

class Vertex;
class SimplexLink;
class UniFiber;
class Channel;
class Circuit;
class NetMan;


class Graph: public AbstractGraph {

public:
	Graph();
	~Graph();
	Graph(const Graph&);
	const Graph& operator=(const Graph&);
	virtual void dump(ostream&) const;

	// MPAC_Optimize
	void MPAC_Opt_ComputePrimaryGivenBackup(list<AbstractLink*>&,
			LINK_COST&, LINK_COST&, const Circuit&, Vertex*, Vertex*);
	void SPAC_Opt_ComputePrimaryGivenBackup(list<AbstractLink*>&,
			LINK_COST&, LINK_COST&, const list<AbstractLink*>&, 
			Vertex*, Vertex*);

	// K-shortest loopless paths
	void Yen(list<AbsPath*>&, AbstractNode* pSrc, OXCNode* pOXCsrc, AbstractNode* pDst, 
			UINT nNumberOfPaths, NetMan* m_pNetman, LinkCostFunction hLCF = LCF_ByHop);
	void Yen(list<AbsPath*>&, UINT nSrc, UINT nDst, 
			UINT nNumberOfPaths, NetMan* m_pNetman, LinkCostFunction hLCF = LCF_ByHop);

	SimplexLink* lookUpSimplexLink(UINT, Vertex::VertexType, int,
					UINT, Vertex::VertexType, int) const;
	Vertex* lookUpVertex(UINT, Vertex::VertexType, int) const;
	void invalidateSimplexLinkDueToSRG(const UniFiber*);
	void invalidateUniFiberSimplexLink(const UniFiber*);
	void invalidateSimplexLinkDueToCap(UINT);
	void validateSimplexLinkDueToCap(BandwidthGranularity residualBWD);
	void invalidateSimplexLinkLightpath();
	void invalidateSimplexLinkDueToPath(list<AbstractLink*> pPath);
	void removeSimplexLinkLightpathNotUsed();
	void preferWavelPath();
	void removeSimplexLinkLightpath(Lightpath * pLightpath);
	void removeSimplexLinkLightpath();
	void releaseSimplexLinkLightpathBwd(UINT nBWToRelease, Lightpath*pLightpath);
	bool isSimplexLinkRemoved(Lightpath * pLightpath);

	void invalidateGrooming();

	void validateGroomingHotelNode();

	void invalidateSimplexLinkDueToWContinuity(UINT src,UINT nBW); //-B
	UINT WPrandomChannelSelection(UINT src, UINT DST, UINT nBW); //-B

	void modifyBackHaulLinkCost(const UniFiber*, LINK_COST);

	virtual bool addNode(AbstractNode *pNode);
	SimplexLink* addSimplexLink(UINT nLinkId, Vertex *pSrc, Vertex *pDst,
			LINK_COST hCost, float nLength,
			UniFiber* pFiber, SimplexLink::SimplexLinkType eType,
			int nChannel, LINK_CAPACITY hCap = INFINITE_CAP);
	SimplexLink* addSimplexLink(UINT nLinkId, 
			UINT nSrc, Vertex::VertexType, int,
			UINT nDst, Vertex::VertexType, int, 
			LINK_COST hCost, float nLength,
			UniFiber* pFiber, SimplexLink::SimplexLinkType eType,
			int nChannel, LINK_CAPACITY hCap = INFINITE_CAP);
	void deleteContent();

	void PAL_SPP_invalidateSimplexLinkDueToTxRx();
	void allocateConflictSet(UINT);
public:
	void computeCutSet(map<UINT, OXCNode*>&) const;
	void releaseLightpathBandwidth(const Lightpath*);

	typedef pair<SimplexLink::SimplexLinkKey, SimplexLink*> SimplexLinkMapPair;
	typedef map<SimplexLink::SimplexLinkKey, SimplexLink*> SimplexLinkMap;
	typedef pair<Vertex::VertexKey, Vertex*> VertexMapPair;
	typedef map<Vertex::VertexKey, Vertex*> VertexMap;

	VertexMap			m_hVertexMap;
	SimplexLinkMap		m_hSimplexLinkMap;

protected:
	void MPAC_Opt_ComputePrimaryGivenBackup_Aux(const Circuit&, 
				Vertex*, Vertex*);
	void SPAC_Opt_ComputePrimaryGivenBackup_Aux(const list<AbstractLink*>&, 
				Vertex*, Vertex*);
	void YenHelper(list<AbsPath*>&, AbstractNode* pSrc, OXCNode* pOXCsrc, AbstractNode* pDst,
			UINT nNumberOfPaths, NetMan* m_pNetman, LinkCostFunction hLCF);
	void invalidateVNode(list<AbstractNode*>&, Vertex*, int);
	void invalidateVLink(list<AbstractLink*>&, SimplexLink*);
	list<AbstractLink*> removeRXLink(list<AbstractLink*>, list<AbstractLink*>);
	list<AbstractLink*> removeTXLink(list<AbstractLink*>, Vertex*);
	list<AbstractLink*> checkWavelengthsOnFibers(list<AbstractLink*>, list<AbstractLink*>);
};

};

#endif
