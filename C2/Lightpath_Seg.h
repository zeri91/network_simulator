#ifndef LIGHTPATH_SEG_H
#define LIGHTPATH_SEG_H

//#include<list>

namespace NS_OCH {

class Circuit;
class AbsPath;
class NetMan;
class Log;

class Lightpath_Seg: public Lightpath {
	Lightpath_Seg(const Lightpath_Seg&);
	const Lightpath_Seg& operator=(const Lightpath_Seg&);

public:
	Lightpath_Seg(UINT, LPProtectionType hType = LPT_SEG_NO_HOP, UINT nCap = OCLightpath);
	~Lightpath_Seg();

	virtual void dump(ostream&) const;

	virtual void setUp(NetMan*, Circuit*);
	virtual void WPsetUp(NetMan*, Circuit*);//fabio 6 gennaio
	virtual void WPrelease(bool&, NetMan*, Circuit*, bool bLog=true);
	virtual void release(bool&, NetMan*, Circuit*, bool bLog=true);
    virtual void releaseOnServCopy(list<AbstractLink*>&);
    UniFiber*	 LookUpByLinkId(int, list<AbstractLink*>&);
	virtual void logPeriodical(Log&, SimulationTime);
	virtual void logFinal(Log&);

public:
	list<AbsPath*>	m_hBackupSegs;
	
	//double            UnAvLP; //-t
    //double            UnAvLPM; //-t
};

};
#endif
