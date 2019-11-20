#ifndef UNIFIBER_H
#define UNIFIBER_H

namespace NS_OCH {

class OXCNode;
class Channel;
class Lightpath;
class NetMan;
class SimplexLink;
class UniFiber;
class Graph;

class UniFiber: public AbstractLink{
	UniFiber(const UniFiber&);
	const UniFiber& operator=(const UniFiber&);
public:
	UniFiber();
	UniFiber(UINT, OXCNode*, OXCNode*, LINK_COST, float, UINT);
	virtual ~UniFiber();

	virtual void dump(ostream& out) const;
	virtual AbstractLink::LinkType getLinkType() const;
	UINT countFreeChannels() const;
	UINT numOfChannels() const;
	void consumeChannel(Lightpath*, int , int nChannel=-1);
	void consumeChannelSUTemp(Lightpath* ,int, int nChannel=-1);
	void releaseChannel(Lightpath*);

//	void releaseChannelTemp(Lightpath*);fabio 7 nov:rimossa
    //void consumeChannel_b(Lightpath*, int , int nChannel=-1);
    //void releaseChannel_b(Lightpath*, int);

    void incrSG(Lightpath*);//-t
	void decrSG(Lightpath*);
	void muxReleaseChannel(Lightpath * pLightpath);
	//-t
    //void incrSGChannel(Lightpath*);//-t
	//void decrSGChannel(Lightpath*);//-t

	void muxConsumeChannel(Lightpath*, int, NetMan*); //-B
	void muxReleaseChannel(Lightpath * pLightpath, NetMan*pNetMan); //-B:


public:
	// immutable attributes
//	UINT			m_nFiberId;	// unique in the same network
//	OXCNode			*m_pSrc;
//	OXCNode			*m_pDst;
//	LINK_COST		m_hCost;	// administrative cost
//	UINT			m_nLength;	// length in kilometer
	UpDownStatus	m_eUpDownStatus;
	int				m_nW;		// # of channels on this fiber
	// N.B: may need to use later on
	// list<int>	m_hSRGList;
	Channel			*m_pChannel;	// allocated in WDMNetwork::addUniFiber -> ARRAY!!!!!!!!!!!!!!!!!!!
		//FABIO:IN REALTA E' UN ARRAY DI DIMENSIONE UGUALE AL NUMERO DI WAVEL!!!!!	// deleted in destructor
		//non molto chiara la dichiarazione

	// mutable attributes
	// case 1: src node is full-range wavelength convertible

	// case 2: src node is wavelength continuous

	// case 3: src node is partial wavelength convertible
	// NB: not implemented yet


    //set di lightpath SharingGroup(e)
    map<UINT, Lightpath*> m_bSharingGroup;//-t

};

};

#endif
