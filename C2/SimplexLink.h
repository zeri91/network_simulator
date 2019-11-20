#ifndef SIMPLEXLINK_H
#define SIMPLEXLINK_H

namespace NS_OCH {

class Vertex;
class UniFiber;
class Lightpath;
class Channel;
class UniFiber;
class Graph;

class SimplexLink: public AbstractLink {
	SimplexLink(const SimplexLink&);
	const SimplexLink& operator=(const SimplexLink&);
public:
	typedef enum {
		LT_Channel = 0,		// a wavelength on a fiber
		LT_UniFiber,		// a fiber, for link bundling
		LT_Lightpath,		// 2
		LT_Channel_Bypass,	// 3
		LT_Grooming,		// 4
		LT_Mux,				// 5
		LT_Demux,			// 6
		LT_Tx,				// 7
		LT_Rx,				// 8
		LT_Converter		// 9
	} SimplexLinkType;
	class SimplexLinkKey: public OchObject{
		friend bool operator<(const SimplexLinkKey& lhs, const SimplexLinkKey& rhs);
		SimplexLinkKey();
	public:
		explicit SimplexLinkKey(int, SimplexLinkType eLType, int nChannel);
		explicit SimplexLinkKey(const SimplexLink& hSimplexLink);
		~SimplexLinkKey();

		int				m_nKeyId;	// -1 for NULL fiber case
		SimplexLinkType	m_eLType;
		int				m_nChannel;
	};
public:
	SimplexLink(int, Vertex*, Vertex*, LINK_COST, float, 
		UniFiber*, SimplexLinkType, int, LINK_CAPACITY hCap = INFINITE_CAP);
	~SimplexLink();

	static void extractNonAuxLinks(list<SimplexLink*>&,
				const list<AbstractLink*>&);
	bool auxVirtualLink() const;
	
	virtual void dumpSL(ostream& out) const;
	virtual AbstractLink::LinkType getLinkType() const;
	SimplexLinkType getSimplexLinkType() const;

	void releaseBandwidth(UINT);
	void consumeBandwidth(UINT);
	void consumeBandwidthHelper(int);

	void allocateConflictSet(UINT);

public:
	//inherited from AbstractLink
//	UINT			m_nLinkId;
//	Vertex			*m_pSrc;
//	Vertex			*m_pDst;
//	LINK_COST		m_hCost;
//	UINT			m_nLength;
	LINK_CAPACITY	m_hFreeCap;

	// For WDM
	SimplexLinkType	m_eSimplexLinkType;
	int				m_nChannel;
	UniFiber		*m_pUniFiber;
	Lightpath		*m_pLightpath;

	// for SPAC_SPP only
	// m_nBackupCap = max{m_pCSet[i]}
	LINK_CAPACITY	m_nBackupCap;
	LINK_CAPACITY	m_nBWToBeAllocated;
};
class PSimplexLinkComp : public less<SimplexLink*>
{
public:
	bool operator()(SimplexLink*& pLeft,
		SimplexLink*& pRight) const {
		return (pLeft->m_hFreeCap > pRight->m_hFreeCap);
	}
};

};

#endif
