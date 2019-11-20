#ifndef ABSTRACTPATH_H
#define ABSTRACTPATH_H

#include <list>

namespace NS_OCH {

// NB: assume single-link graph: at most one link between a s-d pair
class AbstractPath: public OchObject {
public:
	AbstractPath();
	AbstractPath(const AbstractPath&);
	AbstractPath(const list<UINT>&);
	~AbstractPath();
	const AbstractPath& operator=(const AbstractPath&);
	const AbstractPath& operator=(const list<UINT>&);
	virtual void dump(ostream&) const;

	UINT getSrc() const;
	UINT getDst() const;
	UINT getPhysicalHops() const;
	void deleteContent();
	bool containAbstractLink(UINT, UINT) const;
	bool appendAbstractLink(UINT, UINT);

	void setSrc(UINT);
	void setDst(UINT);
public:
	UINT			m_nSrc;
	UINT			m_nDst;
	list<UINT>		m_hLinkList;
};

};	// namespace
#endif
