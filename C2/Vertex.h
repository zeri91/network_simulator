#ifndef VERTEX_H
#define VERTEX_H

#include <list>

namespace NS_OCH {

class OXCNode;
class SimplexLink;
class AbstractNode;
class Graph;

class Vertex: public AbstractNode {
	Vertex(const Vertex&);
	const Vertex& operator=(const Vertex&);
public:
	typedef enum {
		VT_Original = 0,
		VT_Access_In,
		VT_Access_Out,
		VT_Lightpath_In,
		VT_Lightpath_Out,
		VT_Channel_In,
		VT_Channel_Out
	} VertexType;

	// used for mapping
	class VertexKey: public OchObject{
		friend bool operator<(const VertexKey& lhs, const VertexKey& rhs);
		friend bool operator==(const VertexKey& lhs, const VertexKey& rhs);
		VertexKey();
	public:
		explicit VertexKey(UINT, VertexType eVType, int nChannel);
		explicit VertexKey(const Vertex& hVertex);
		~VertexKey();

		UINT		m_nKeyId;
		VertexType	m_eVType;
		int			m_nChannel;
	};
public:
	Vertex(UINT);
	Vertex(OXCNode*, UINT, VertexType, int nChannel = -1);
	~Vertex();

	virtual void dump(ostream&) const;

//	void addIncomingSimplexLink(SimplexLink*);
//	void addOutgoingSimplexLink(SimplexLink*);

public:
//	UINT				m_nName;		// unique in a network
//	list<SimplexLink*>	m_hILinkList;	// unidirectional link
//	list<SimplexLink*>	m_hOLinkList;	// unidirectional link

	// For WDM
	OXCNode		*m_pOXCNode;	// the src OXC of this image vertex
	VertexType	m_eVType;
	int			m_nChannel;		// valid only when m_eVType is VT_Channel_*
								// -1 otherwise
};

};

#endif
