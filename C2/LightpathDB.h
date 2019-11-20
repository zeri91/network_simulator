#ifndef LIGHTPATHDB_H
#define LIGHTPATHDB_H

#include <list>
#include <map>

namespace NS_OCH {

class OchObject;
class Lightpath;
class Log;

class LightpathDB: public OchObject {
	LightpathDB(const LightpathDB&);
	const LightpathDB& operator=(const LightpathDB&);
public:
	LightpathDB();
	~LightpathDB();
	virtual void dump(ostream&) const;

	void appendLightpath(Lightpath*);
	void removeLightpath(Lightpath*);

	static UINT getNextId();
	static UINT peekNextId();
public:
	void logFinal(Log&);
	void logPeriodical(Log&, SimulationTime);
	typedef pair<UINT, list<Lightpath*>::iterator> LightpathMapPair;
	typedef map<UINT, list<Lightpath*>::iterator> LightpathMap;
	list<Lightpath*>	m_hLightpathList;
	LightpathMap		m_hLightpathMap;

protected:
	static UINT			g_nNextLightpathId;
};

};
#endif
