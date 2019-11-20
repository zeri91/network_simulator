#pragma warning(disable: 4786)
#include <assert.h>
#include "OchInc.h"

#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "SimplexLink.h"
#include "OXCNode.h"
#include "Vertex.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

bool NS_OCH::operator<(const Vertex::VertexKey& lhs, 
					   const Vertex::VertexKey& rhs)
{
	if (lhs.m_nKeyId < rhs.m_nKeyId)
		return true;
	else if (lhs.m_nKeyId > rhs.m_nKeyId)
		return false;
	
	if (lhs.m_eVType < rhs.m_eVType)
		return true;
	else if (lhs.m_eVType > rhs.m_eVType)
		return false;

	// can be equal cause < is called by find as well
//	assert(lhs.m_nChannel != rhs.m_nChannel);	 
	return (lhs.m_nChannel < rhs.m_nChannel);
}

bool NS_OCH::operator==(const Vertex::VertexKey& lhs, 
						const Vertex::VertexKey& rhs)
{
	bool bEqual = (lhs.m_nKeyId == rhs.m_nKeyId)
				&& (lhs.m_eVType == rhs.m_eVType)
				&& (lhs.m_nChannel == rhs.m_nChannel);
	return bEqual;
}

Vertex::VertexKey::VertexKey(UINT nId, VertexType eVType, int nChannel):
	m_nKeyId(nId), m_eVType(eVType), m_nChannel(nChannel)
{
}

Vertex::VertexKey::VertexKey(const Vertex& hVertex): 
	m_nKeyId(hVertex.m_pOXCNode->m_nNodeId), m_eVType(hVertex.m_eVType), 
	m_nChannel(hVertex.m_nChannel) 
{
}

///////////////////////////////////////////////////////////////////////////////
//
Vertex::VertexKey::~VertexKey()
{
}

Vertex::Vertex(UINT nId): AbstractNode(nId)
{
}

Vertex::Vertex(OXCNode* pNode, UINT nId, VertexType eType, int nChannel): 
	AbstractNode(nId), m_pOXCNode(pNode), m_eVType(eType), m_nChannel(nChannel)
{
}

Vertex::~Vertex()
{
}

void Vertex::dump(ostream& out) const
{
#ifdef DEBUGB
	cout << "VERTEX DUMP" << endl;
	cin.get();
#endif // DEBUGB
	out << "VertexId = ";
	out.width(4);
	out<<m_nNodeId<<" OXC = ";
	out.width(3);
	out<<m_pOXCNode->m_nNodeId<<" W = ";
	out.width(3);
	out<<m_nChannel<<" ";

	if (UNREACHABLE != m_hCost)
		out<<"C = "<<m_hCost<<" ";
	else
		out<<"C = INF ";

	if (NULL == m_pPrevLink)
		out<<"PrevHop = NULL ";
	else
		out<<"PrevHop = "<<m_pPrevLink->m_pSrc->getId()<<" ";

	switch (m_eVType) {
	case VT_Original:
		break;
	case VT_Access_In:
		out<<"A_In";
		break;
	case VT_Access_Out:
		out<<"A_Out";
		break;
	case VT_Lightpath_In:
		out<<"Lp_In";
		break;
	case VT_Lightpath_Out:
		out<<"Lp_Out";
		break;
	case VT_Channel_In:
		out<<"ChIn"<<m_nChannel;
		break;
	case VT_Channel_Out:
		out<<"ChOut "<<m_nChannel;
		break;
	default:
		DEFAULT_SWITCH;
	}
	out<<": "<<endl;
	list<AbstractLink*>::const_iterator iter;
	list<SimplexLink*>::const_iterator iterSL;

	for (iter = m_hOLinkList.begin(); iter != m_hOLinkList.end(); iter++) {
		out<<'\t';
		//-B: ABSTRACTLINK DUMP (very long)
		(*iter)->dump(out);
//		out<<endl;
	}
//	for (iter=m_hILinkList.begin(); iter!=m_hILinkList.end(); iter++) {
//		out<<'\t';
//		(*iter)->dump(out);
//		out<<endl;
//	}
#ifdef _OCHDEBUG9
	if (m_pBackupCost) {
		out<<"\tCS=";
		out<<'{';
		int i;
		for (i=0; i<m_nBackupVHops; i++) {
			out.width(3);
			out<<m_pBackupCost[i];
			if (i<(m_nBackupVHops - 1))
				out<<", ";
		}
		out<<'}'<<endl;
	}
#endif
}

// void Vertex::addIncomingSimplexLink(SimplexLink* pILink)
// {
//	assert(pILink);
//	assert(pILink->m_pDst->m_nName == m_nName);
//	m_hILinkList.push_back(pILink);
// }

// void Vertex::addOutgoingSimplexLink(SimplexLink* pOLink)
// {
//	assert(pOLink);
//	assert(pOLink->m_pSrc->m_nName == m_nName);
//	m_hOLinkList.push_back(pOLink);
// }
