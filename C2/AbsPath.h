#ifndef ABSPATH_H
#define ABSPATH_H

#include <list>

namespace NS_OCH {

class AbstractLink;

class AbsPath: public OchObject {
public:
	AbsPath();
	AbsPath(int);
	explicit AbsPath(const list<AbstractLink*>&);
	~AbsPath();
	AbsPath(const AbsPath&);
	const AbsPath& operator=(const AbsPath&);
	bool operator==(const AbsPath&)const;

	virtual void dump(ostream&) const;

	void invalidate();
	LINK_COST calculateCost();
	LINK_COST getCost() const;

public:
	list<AbstractLink*>		m_hLinkList;
    bool                    overlap;//-t
	int						wlAssigned; //lamda assegnata, come per lightpath;
protected:
	LINK_COST				m_hCost;
};
	
};	// namespace
#endif
