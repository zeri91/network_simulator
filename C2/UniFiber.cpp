#pragma warning(disable: 4786)
#include <assert.h>
#include "OchInc.h"
#include <vector>
#include <set>
#include <list> 

//#include "math.h"

#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "AbstractPath.h"
#include "OXCNode.h"
#include "Channel.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "Lightpath.h"
#include "LightpathDB.h"
#include "ConnectionDB.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "Log.h"
#include "NetMan.h"
#include "UniFiber.h"


#define cons_release

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

UniFiber::UniFiber(): AbstractLink(), m_pChannel(NULL)
{
}

UniFiber::UniFiber(UINT nId, OXCNode *pSrc, OXCNode *pDst, 
				   LINK_COST hCost, float nLength, UINT nNoOfChannels):
	AbstractLink(nId, pSrc, pDst, hCost, nLength), 
	m_nW(nNoOfChannels), m_pChannel(NULL)
{
	wlOccupation = valarray<LINK_COST>(1, nNoOfChannels); //CANALI VARIABILI
								//val,#elmts
	wlOccupationBCK = valarray<LINK_COST>(1, nNoOfChannels);
}

UniFiber::~UniFiber()
{
	if (m_pChannel)
		delete []m_pChannel;
}

void UniFiber::dump(ostream &out) const
{
	out<<"Fiber "<< m_nLinkId <<": ";
	if (m_nLinkId < 10)
		out << " ";
	out << m_pSrc->m_nNodeId << "->" << m_pDst->m_nNodeId;
	if (m_pSrc->m_nNodeId < 10)
		out << " ";
	if (m_pDst->m_nNodeId < 10)
		out << " ";
	out << " | num of channels = " << m_nW;
	if (m_bValid)
		out<<" | VALID";
	else
		out<<" | INVALID";
	
///-t
    out<<" | cost = ";
    out.precision(5);
    //out.setf(ios::fixed, ios::floatfield); //http://www.cplusplus.com/reference/ios/ios_base/precision/?kw=precision
	out << m_hCost << " | Length (km) = ";
	out.width(8); out << m_nLength;
	/*
	out << "\t| status: ";
	if (m_eUpDownStatus == 0 || m_eUpDownStatus == 1)
		out << m_eUpDownStatus;
	else
		out << "/  ";
	*/
	//out<<m_hCost<<" ";// <<endl;

	////////-B: COMMENTATO QUELLO CHE NON MI INTERESSAVA
 	/*
  	out<<"\tChannels=[";
  	if (m_pChannel) {
  		int w;
  		for (w=0; w<m_nW; w++) {
  			if (m_pChannel[w].m_bFree)
				out<<"- ";
  			else 
				out<<"B ";
  		}
  	} else {
  		int w;
  		for (w=0; w<m_nW; w++)
  			out<<'0';
  	}
  	out<<"] ";
	*/

///-t
	//-B: stampa conflict set non mi interessa, la commento
		/*if (m_pCSet) {
		out<<" CSet={";
		UINT i;
		for (i=0; i<m_nCSetSize; i++) {
			out<<m_pCSet[i];
			if ((i+1)<m_nCSetSize)
				out<<',';
		}
		out<<'}';
		}*/
	
	/*
	out << " v*=" << m_nBChannels << " ";
    
	out << " BCh:";
	int  c, b = 0;
	for (c = 0; c < m_nW; c++) {
		if (m_pChannel[c].m_backup)
			b++;
	}
	out << b << " ";
	*/
	/*static double B_=0;
    static double bbb=0;
	bbb++;
    B_+=m_nBChannels;
	//out<<" B_ "<<B_<<" ";
	double B_m=B_ / bbb;
	if(m_nLinkId==0){out<<"B_m="<< B_m;}//-t
    */
	/*
	out << " SG = " << m_bSharingGroup.size() << " ";
	
	map<UINT, Lightpath*>::const_iterator itrSG;
	out<<" SG:";
	for (itrSG = m_bSharingGroup.begin(); itrSG != m_bSharingGroup.end(); itrSG++) {
	//cout<<"LP"<<(*itrSG).first<<" ";
    out<<(*itrSG).second->getId()<<",";
	}
	*/

	//out<<" ChB:";
	/*for (int w=0; w<m_nW; w++) {
		if(m_pChannel[w].m_backup)
		{out<<w<<"(";
         list<UINT>::const_iterator itrSGCh;
         for (itrSGCh=m_pChannel[w].m_bSharingGroupCh.begin();itrSGCh!=m_pChannel[w].m_bSharingGroupCh.end(); itrSGCh++) {
		 //out<<m_pChannel[w].m_bSharingGroupCh.front();
		 out<<(*itrSGCh)<<",";}
         out<<")";
		}out<<" ";
		}
*/

	//-B: try to print wlOccupation. It should be an array of 40 elements but it just print 8 elements, don't know why
	out << "\tWlOccupation: [";
	for (int i = 0; i < this->m_nW; i++)
	{
		if (wlOccupation[i + 1] != NULL)
		{
			if (wlOccupation[i] == UNREACHABLE)
				out << "UNREACHABLE, ";
			else
			{
				out.precision(5);
				out << wlOccupation[i] << ", ";
			}
		}
		else
		{
			if (wlOccupation[i] == UNREACHABLE)
				out << "UNREACHABLE, ";
			else
			{
				out.precision(5);
				out << wlOccupation[i] << ", ";
			}
		}
	}
	out << "]" << endl;
}

AbstractLink::LinkType UniFiber::getLinkType() const
{
	return LT_UniFiber;
}

UINT UniFiber::numOfChannels() const
{
	return m_nW;
}


UINT UniFiber::countFreeChannels() const
{
	assert(m_pChannel);
	UINT nFreeChannels = 0;
	int  w;
	for (w = 0; w < m_nW; w++) {
		if (m_pChannel[w].m_bFree)
			nFreeChannels++;
	}
	return nFreeChannels;
}


void UniFiber::consumeChannelSUTemp(Lightpath* pLightpath,int b, int nChannel)//FABIO 6 nov:inverto i nomi!!!
{
	assert(m_pChannel);
	if (-1 == nChannel) {//se serve per il working(chiamata da updateChannelUsage())
		//oppure per un backup non condivisibile in un canale gia' usato
		//allora  nChannel rappresenta il numero del nuovo canale da usare
		int w = -1;
//	while ((++w < m_nW) && (!m_pChannel[w].m_bFree))
//		NULL											..salvo l'originale...
		while ((++w < m_nW) )//trova il primo canale libero
		{
			if(m_pChannel[w].m_bFree)
			{

				if(pLightpath==NULL)//FABIO 25 ott:se devo allocare un backup,
				{					// controllo che non ci siano canali salvati sottostanti
					if(m_pChannel[w].isLPsaved==true)
						continue; //analizza il prossimo canale x backup
					else break; //posso allocare backup su questo canale
				}
				else break;//alloco working su canale libero
			}//b free
			else continue;//se canale è occupato
		}
		assert(w < m_nW);
//-t
		if (b==1){//-t
			m_pChannel[w].m_backup=true;
           // UINT num=(pLightpath->getId());
            //UINT* pnum = &num;
            //m_pChannel[w].incrSGCh(pnum);
        }else{
	        m_pChannel[w].m_backup=false;
		}


        m_pChannel[w].consume(pLightpath);

/*#ifdef cons_release
{		///-t
        //cout<<endl;///-t
		cout<<"cons_W:Fibra "<<this->getId()<<":"<<this->getSrc()->getId()<<"->"<<this->getDst()->getId()<<" ";
        cout<<"Canale usato "<<w<<" ";
		cout<<"Lightpath "<<m_pChannel[w].m_pLightpath->getId()<<" ";
		//(*pLightpath).dump(cout);
        cout<<endl;///-t
}
#endif*/
	

	} else if ((nChannel >= 0) && (nChannel < m_nW)) {//non si usa mai
		
		m_pChannel[nChannel].consume(pLightpath);//allora 
/*#ifdef cons_release
{     ///-t
        //cout<<endl;///-t
		cout<<"cons_B:Fibra "<<this->getId()<<":"<<this->getSrc()->getId()<<"->"<<this->getDst()->getId()<<" ";
        cout<<"Canale usato "<<nChannel<<" ";
		cout<<"Lightpath "<<m_pChannel[nChannel].m_pLightpath->getId()<<" ";
		//(*pLightpath).dump(cout);
        cout<<endl;///-t
}
#endif*/

	} else {
		assert(false);
	}
}

//-B: modified by me
void UniFiber::consumeChannel(Lightpath* pLightpath, int b, int nChannel) //FABIO 6 nov: inverto i nomi!!!
{
	assert(m_pChannel);

#ifdef DEBUGB
	cout << "\t-> consumeChannel" << endl;
	cout << "\tCCCCCSulla fibra " << this->getId() << " dovrei occupare il canale: " << nChannel;
#endif // DEBUGB

	int w; //-B: assigned channel
	w = nChannel;
	
	//-B: case VWP
	if (-1 == nChannel)
	{ //-B: case VWP
		//se serve per il working (chiamata da updateChannelUsage())
		//oppure per un backup non condivisibile in un canale gia' usato
		//allora  nChannel rappresenta il numero del nuovo canale da usare
		while ((++w < m_nW) && (!m_pChannel[w].m_bFree))
			NULL;
		//-B: esce da questo ciclo quando trova il primo canale libero
		//	w indica quindi l'id di tale canale libero
		assert(w < m_nW);
	}
	//-B: case WP
	else if ((nChannel >= 0) && (nChannel < m_nW))
	{
		pLightpath->wlAssigned = w; //-B: assign wavel to the lightpath only if it is constant (WP) along it
									//	else, the wavel won't be constant along all the lightpath
	}
	else
	{
		assert(false);
	}

	//-B: if b == 1 then backup, if b == 0 then only primary
	if (b == 1)
	{
		m_pChannel[w].m_backup = true;
	}
	else
	{
		m_pChannel[w].m_backup = false;
	}
	
	//-B: 'consume' method set to false the boolean var indicating the channel "freedom" (while the Channel builder set it as true)
	m_pChannel[w].consume(pLightpath);

}

//-B: it seems it cares about both wp release and no wp release
void UniFiber::releaseChannel(Lightpath* pLightpath)
{
	//cout << "lightpath passato" << pLightpah->getId() << endl
	assert(m_pChannel);
	int w = -1;
	if (pLightpath == NULL || pLightpath->wlAssigned == -1)
	{//Fabio
		if (pLightpath != NULL)
		{//Fabio 19 ott: cambio strategia per il rilascio dei canali NULL(di backup)
			while (++w < m_nW)
			{
				//cerca il canale non libero corrispondente al pLightpath
				if ((!m_pChannel[w].m_bFree) && (m_pChannel[w].m_pLightpath == pLightpath))
					break;
			}
		}
		else
		{//Se è NULL libero i canali di backup a partire dal fondo..
			w = m_nW;
			while (--w > -1) {
				//cerca il canale non libero corrispondente al pLightpath
				if ((!m_pChannel[w].m_bFree) && (m_pChannel[w].m_pLightpath == pLightpath))
					break;
			}
		}
#ifdef DEBUGB
		cout << "\tCHI SUCCIRIU??? " << w << endl;
		cin.get();
#endif // DEBUGB
		assert(w < m_nW);
		assert(w >= 0);

		//-B: set m_bFree = false and m_pLightpath reference = NULL
		m_pChannel[w].release(); //lo libera
	}
	else
	{
		m_pChannel[pLightpath->wlAssigned].release();

#ifdef DEBUGB
		cout << "Libero banda nel canale: " << pLightpath->wlAssigned << endl;
#endif // DEBUGB

	}
}

//fabio 9 ott: per il rilascio temporaneo dei canali
//7 nov: rimosso
/*
void UniFiber::releaseChannelTemp(Lightpath* pLightpath)
{
 
	assert(m_pChannel);
	int w = -1;
	while (++w < m_nW) {
		if ((!m_pChannel[w].m_bFree) 
		 && (m_pChannel[w].m_pLightpath == pLightpath))//cerca il canale non libero corrispondente al pLightpath
		 	break;
	}
	assert(w < m_nW);
	m_pChannel[w].isLPsaved=true;
	m_pChannel[w].release();//lo libera

}*/


//-t
void UniFiber::incrSG(Lightpath* pLightpath)
{   //cout<<"incrSG: ";//pLightpath->dump(cout);cout<<endl;
    //cout<<"UnAvLP:"<<pLightpath->UnAvLP<<endl;
	m_bSharingGroup.insert(pair<UINT, Lightpath*>(pLightpath->getId(), pLightpath));
}

void UniFiber::decrSG(Lightpath* pLightpath)
{   //cout<<m_bSharingGroup.find(pLightpath->getId())->first<<"_UnAvLP:"<<pLightpath->UnAvLP<<" ";
    m_bSharingGroup.erase(m_bSharingGroup.find(pLightpath->getId()));
}

//-B: it seems it cares about both wp release and no wp release
//	originally taken from releaseChannel
void UniFiber::muxReleaseChannel(Lightpath* pLightpath)
{
#ifdef DEBUG
	cout << "-> muxReleaseChannel" << endl;
	cout << "\t---> FIBER " << this->getId() << endl;
#endif // DEBUGB

	assert(m_pChannel);
	int w;
	//-B: scan all the channels
	for (w = 0; w < this->m_nW; w++)
	{
		if (m_pChannel[w].m_pLightpath)
		{
			//-B: if the channel w points to the selected lightpath
			if (m_pChannel[w].m_pLightpath == pLightpath)
			{
				m_pChannel[w].release();
				wlOccupation[w] = 1;
				m_pChannel[w].deleteId(pLightpath->getId());
				break;
			}
		}
	}

	/*
	for (w = 0; pLightpath->wlsAssigned.size(); w++)
	{
		assert(pLightpath->wlsAssigned[w] > -1 && pLightpath->wlsAssigned[w] < this->m_nW);
		ch = pLightpath->wlsAssigned[w];
		
		//-B: set m_bFree to true and m_pLightpath's reference to NULL
		m_pChannel[ch].release();
		wlOccupation[ch] = 1;
		m_pChannel[ch].deleteId(pLightpath->getId());
	}*/
}

//-B: it seems it cares about both wp release and no wp release
//	originally taken from releaseChannel
void UniFiber::muxReleaseChannel(Lightpath* pLightpath, NetMan*pNetMan)
{
#ifdef DEBUG
	cout << "-> muxReleaseChannel" << endl;
	cout << "\t---> FIBER " << this->getId() << " (";
	cout.width(3); cout << this->getSrc()->getId();
	cout << "->"; cout.width(3); cout << this->getDst()->getId();
	cout << ")" << endl;

	if (this->getSrc()->getId()==46) {
		
		cout << "Why i used this ??" << endl;
		cin.get();
		cin.get();
	
	}

#endif // DEBUGB

	assert(m_pChannel);
	UINT w = pLightpath->wlAssigned;
	if (m_pChannel[w].m_pLightpath)
	{
		//-B: if the channel w points to the selected lightpath
		assert(m_pChannel[w].m_pLightpath == pLightpath);

#ifdef DEBUGC
		cout << "\t---> CANALE " << w << endl;
#endif // DEBUGB

		if (pNetMan->checkFreedomSimplexLink(this->getSrc()->getId(), this->getDst()->getId(), w))
		{
#ifdef DEBUGC
			cout << "\tElimino il puntatore al lightpath " << pLightpath->getId() << " dal canale " << w << endl;
#endif // DEBUGB
			m_pChannel[w].release();
			wlOccupation[w] = 1;
			m_pChannel[w].deleteId(pLightpath->getId());
		}
		else
		{
#ifdef DEBUGC
			cout << "\tCanale che ha altre connessioni. NON elimino il puntatore del canale al lightpath, ne' imposto free status = true, ne' wlOccupation[ch] = 1" << endl;
#endif // DEBUGB
		}
	}
}


//-B: originally taken from consumeChannel
void UniFiber::muxConsumeChannel(Lightpath* pLightpath, int w, NetMan* pNetMan)
{
	assert(m_pChannel);
	
#ifdef DEBUGC
	cout << "-> muxConsumeChannel" << endl;
#endif // DEBUGB

	//assert(m_pChannel[w].m_bFree);

	//-B: if b == 1 then backup, if b == 0 then only primary
	m_pChannel[w].m_backup = false;

	//-B: --------------- UPDATE LIGHTPATHS IDs' LIST USING CHANNEL w ------------------
	m_pChannel[w].muxConsume(pLightpath, pLightpath->getId());

	//-B: -------------- UPDATE SIMPLEX LINK CAPACITY ----------------
	pNetMan->updateLinkCapacity(pLightpath, w, this);

#ifdef DEBUGC
	pNetMan->printChannelReference(this);
#endif // DEBUGB

}