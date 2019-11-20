#ifndef CHANNEL_H
#define CHANNEL_H

namespace NS_OCH {
class UniFiber;
class Lightpath;
class SimplexLink;
class Graph;

class Channel: public OchObject {
public:
	Channel();
	Channel(UniFiber*, UINT, UINT, bool bFree = true, UpDownStatus eUpDown= UP);
	Channel(const Channel&);
	~Channel();

	const Channel& operator=(const Channel&);
	virtual void dump(ostream &out) const;
	void consume(Lightpath*);
	void release();
    //void incrSGCh(UINT*);//-t
	//void decrSGCh(UINT*);//-t
	void  restoreSavedLightpath();

	void muxConsume(Lightpath*, UINT); //-B

	void deleteId(int wavel); //-B

public:
	// immutable attributes
	UniFiber		*m_pUniFiber;	// the unifiber it belongs to
	UINT			m_nW;			// which wavelength
	UINT			m_nCapacity;

	// mutable attributes
	UpDownStatus	m_eUpDownStatus;
	bool			m_bFree;
	Lightpath		*m_pLightpath;	// the LP that uses this channel
	int				canaliProtetti; //7 gennaio: numero di canali working protetti da questo canale
	
	vector<bool>	linkProtected; //vettore di dimensioni pari al numero di link			
											//gli elementi true sono i link protetti da questo canale

	vector<bool>	nodesProtected;//vettore di dimensioni pari al numero di NODI			
											//gli elementi true sono i NODI protetti da questo canale
	// list of subchannels

//-t
    bool            m_backup;
   
	//set di lightpath SharingGroup del Canale
    list<UINT>      m_bSharingGroupCh;//-t
    //Lightpath		*m_pLightpathCh;
    //map<UINT, Lightpath*> m_bSharingGroupCh;
    
	bool			isLPsaved; //FABIO 6 ott: true se  m_pLightpathSAVED è valido
	Lightpath		*m_pLightpathSAVED; // FABIO 6 Ott:

	vector<int> LightpathsIDs; //-B: all lightpath's ids using the channel

};

};

#endif
