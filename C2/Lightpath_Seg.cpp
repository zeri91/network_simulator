#pragma warning(disable: 4786)
#pragma warning(disable: 4715)
#pragma warning(disable: 4018)
#include <assert.h>
#include <list>
#include <math.h>
//#define max(a,b) (((a)>(b))?(a):(b)) 
//#define min(a,b) (((a)<(b))?(a):(b)) 
#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "AbstractPath.h"
#include "OXCNode.h"
#include "Channel.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "ConnectionDB.h"
#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "Log.h"
#include "LightpathDB.h"
#include "NetMan.h"
#include "Circuit.h"
#include "Lightpath.h"
#include "Lightpath_Seg.h"

#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

Lightpath_Seg::Lightpath_Seg(UINT nId, LPProtectionType hType, UINT nCap):
Lightpath(nId, hType, nCap)
{
}

Lightpath_Seg::~Lightpath_Seg()
{
	list<AbsPath*>::const_iterator itrSeg;
	for (itrSeg=m_hBackupSegs.begin(); itrSeg!=m_hBackupSegs.end(); itrSeg++) {
		delete (*itrSeg);
	}
}

void Lightpath_Seg::dump(ostream& out) const
{
#ifdef DEBUGB
	out << "-> LIGHTPATH_SEG DUMP";
	char*protectionType;
	if (m_eProtectionType == 0)
	{
		protectionType = "LPT_PAL_Unprotected";
		out << "\tLp protection Type: " << protectionType << endl;
	}
	else if (m_eProtectionType == 3)
	{
		protectionType = "LPT_PAL_Shared";
		out << "\tLp protection Type: " << protectionType << endl;
	}
	else
	{
		out << "\tLp protection Type: " << m_eProtectionType << endl;
	}	
#endif // DEBUGB
	
	out<<"LP "<<m_nLinkId<<": ";
	if (m_pSrc && m_pDst)
		out<<m_pSrc->getId()<<"->"<<m_pDst->getId();
	if (UNREACHABLE != this-> m_hCost)
		out<<"\tCOST = "<<m_hCost<<" ";
	else
		out<<"\tCOST = INF";	// NO OUTPUT afterwards if out<<UNREACHABLE
	out << "\tLENGTH = " << m_nLength << "\tCAP = " << m_nCapacity << "\tFREECAP = " << m_nFreeCapacity;
	out << endl << "\tPRIMARY = [";
	list<UniFiber*>::const_iterator itr;
	for (itr = m_hPRoute.begin(); itr != m_hPRoute.end(); itr++) {
		out.width(3);
		out<<(*itr)->getSrc()->getId(); //-B: STRANO: in Lightpath.dump questa linea di codice è: out >> m_pSrc->getId();
										//	MOD: tolgo il +1 perchè credo sia sbagliato
	}
	if (m_pDst) {
		out.width(3);
		out << m_pDst->getId(); //-B: STRANO: in Lightpath.dump questa linea di codice è: out >> m_pDst->getId();
								//	MOD: tolgo il +1 perchè credo sia sbagliato
	}
	out<<"]";
	
	out << " WL = " << this->wlAssigned + 1; //-B: +1 since wavelenght's range is 0-39 instead of 1-40

	out<<endl<<"\tBACKUP = [ ";
	list<AbsPath*>::const_iterator itrSeg;
	list<AbstractLink*>::const_iterator itrB;
	bool bFirst = true;
	for (itrSeg = m_hBackupSegs.begin(); itrSeg != m_hBackupSegs.end(); itrSeg++) {
		if (!bFirst) {
			out<<", ";
		} else {
			bFirst = false;
		}
		for (itrB = (*itrSeg)->m_hLinkList.begin(); itrB != (*itrSeg)->m_hLinkList.end(); itrB++) {
			out.width(3);
			out<<(*itrB)->getSrc()->getId()+1;
		}
		out.width(3);
		out<<(*itrSeg)->m_hLinkList.back()->getDst()->getId()+1;
	}
	out<<"]";

	//	out<<" WL="<<this->wlAssignedBCK;
}

//-B: setup a NEW Lightpath (VWP case)
//	too long method (but in case PTunprotected or PT_BBU it returns very quickly)
void Lightpath_Seg::setUp(NetMan* pNetMan, Circuit* pCircuit)
{
#ifdef DEBUGB
	cout << "LIGHTPAHT_SEG SETUP" << endl;
#endif // DEBUGB
	if (0 == m_nLinkId)
		m_nLinkId = LightpathDB::getNextId(); //-B: m_nLinkId = 2
	if (NULL == m_pSrc)
		m_pSrc = m_hPRoute.front()->getSrc();
	if (NULL == m_pDst)
		m_pDst = m_hPRoute.back()->getDst();
		
	//-B: since it is a NEW lightpath, the free capacity is still equal to the total one
	assert(m_nFreeCapacity >= pCircuit->m_eBW);
	m_nFreeCapacity = m_nFreeCapacity - pCircuit->m_eBW;

	// Update wavelength usage along the primary
	/*if(pCircuit->m_eState==3)//TempTornDown
		updateChannelUsageTemp(m_hPRoute);
		else*/
	updateChannelUsage(m_hPRoute);

	if (pNetMan->m_eProvisionType == NetMan::PT_UNPROTECTED
		|| pNetMan->m_eProvisionType == NetMan::PT_PAL_DPP || pNetMan->m_eProvisionType == NetMan::PT_BBU)
	{
		if(pCircuit->m_eState == Circuit::CT_Setup)
			pNetMan->appendLightpath(this, pCircuit->m_eBW);
		return; //-B: evito il resto del codice di questo metodo che tratta il caso in cui ci sia da fare protection
	}

//////////----------- case UNPROTECTED or BBU: STOP!!!!!!! ----------------

#ifdef _OCHDEBUGA
	{
//		dump(cout);
//		cout<<endl;
	}
#endif

if(pNetMan->isLinkDisjointActive){
	    // Reserve wavelength along backup segments
    list<AbsPath*>::const_iterator itr;
    list<UniFiber*>::const_iterator itrP;
    list<AbstractLink*>::const_iterator itrB;
    for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {
        UINT nSegEndNodeId = (*itr)->m_hLinkList.back()->getDst()->getId();
        for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
            itrB++) {
	        UniFiber *pBFiber = (UniFiber*)(*itrB);
                assert(pBFiber && pBFiber->m_pCSet);
                UINT nNewBChannels = pBFiber->m_nBChannels;
                for (itrP=m_hPRoute.begin(); itrP!=m_hPRoute.end(); itrP++) {
                    UINT nPLinkId = (*itrP)->getId();
                    pBFiber->m_pCSet[nPLinkId]++;
                    if (pBFiber->m_pCSet[nPLinkId] > nNewBChannels)
                        nNewBChannels++;
                    assert(nNewBChannels >= pBFiber->m_pCSet[nPLinkId]);
                }
                if (nNewBChannels > pBFiber->m_nBChannels) {
                    pBFiber->m_nBChannels++;
                    pBFiber->consumeChannel(NULL,1, -1);
                    assert(pBFiber->m_nBChannels == nNewBChannels);
                }
            } // for itrB
            //itrPIndex = itrP;
        } // for itr  
	} //end LINK-DISJOINT

	else{ //if NODE-DISJOINT
     //-t
    int bb=0; int bc=0; int ccp=0; int cpc=0; double m_bb=0; int cpc_=0;
	int shgp=0;
    double AvWork=0.9999;
    double delta=1; double deltatemp=0; int ns=1; double dp=0; double product=0;
    double m=m_hPRoute.size();// cout<<m_hPRoute.size()<<" ";
   double AvLink=pow(AvWork,(1));//pow(AvWork,(1/m)); //cout<<AvLink<<" ";
    //double AvLinkB=pow(AvWork,(1/m));
    double UnAvLPtemp=1;double AvLPtempOver=0;
    map<UINT, Lightpath*> m_bSharingGroupLP;

	// Reserve wavelength along backup segments
	list<AbsPath*>::const_iterator itr;
    list<AbstractLink*>::const_iterator itrB;
	list<AbsPath*>::const_iterator itr_;
	list<AbstractLink*>::const_iterator itra_,itrb_; 
    list<AbstractLink*>::const_iterator itrc_; 
	double ssz=0; int sd=0; int over=0;
	list<UniFiber*>::const_iterator itrP;
	
    list<UniFiber*>::const_iterator itrcc = m_hPRoute.begin();
    list<UniFiber*>::const_iterator itrPP = m_hPRoute.begin();

	for (itr_=m_hBackupSegs.begin(); itr_!=m_hBackupSegs.end(); itr_++){
		sd++;
		ssz += (*itr_)->m_hLinkList.size();
	    itra_=(*itr_)->m_hLinkList.begin();// cout<<(*itra_)->getSrc()->getId()<<"-"<<(*itra_)->getDst()->getId()<<endl;
	    for (itrb_=(*itr_)->m_hLinkList.begin();itrb_!=(*itr_)->m_hLinkList.end();itrb_++) {}
        itrb_--;//cout<<(*itrb_)->getSrc()->getId()<<"-"<<(*itrb_)->getDst()->getId()<<endl;
		if(itr_==m_hBackupSegs.begin()) itrc_=itrb_;
		if((*itrc_)->getDst()->getId()!=(*itra_)->getSrc()->getId()) 
		  {if(sd>1) {over++;//cout<<" overlap:"<<over<<endl;//cout<<"segm:"<<sd<<" itrc_:"<<(*itrc_)->getSrc()->getId()<<"-"<<(*itrc_)->getDst()->getId()<<endl;
		   (*itr_)->overlap=true;}
		 }
        itrc_=itrb_;
	}//tutto questo for stabilisce se c'è overlap...non me ne frega nulla...


   double AvLinkB=pow(AvWork,(1));//pow(AvWork,(1/ssz));

	if (m_hPRoute.size() > 1) {
		// case 1: primary traverse multiple (>1) hops		
        list<UniFiber*>::const_iterator itrPIndex = m_hPRoute.begin();//cout<<(*itrcc)->getSrc()->getId()<<")";
		itrPIndex++;	// only consider intermediate node failures
		for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {//scorro i vari segmenti di backup
			UINT nSegEndNodeId = (*itr)->m_hLinkList.back()->getDst()->getId();
            UINT nSegStartNodeId = (*itr)->m_hLinkList.front()->getSrc()->getId();//-t
     		for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end(); itrB++) //scorro i link di ognuno
			{   ccp++;//-t
				UniFiber *pBFiber = (UniFiber*)(*itrB);//downcast dell iteratore ad UniFiber
				assert(pBFiber && pBFiber->m_pCSet);
            	//pBFiber->incrSG(this);//-t
				UINT nNewBChannels = pBFiber->m_nBChannels;//prelevo v*_temp

				for (itrP=itrPIndex; ((itrP!=m_hPRoute.end()) 
					&& ((*itrP)->getSrc()->getId()!=nSegEndNodeId)); itrP++)
				{//scorro i link del working
					UINT nPNodeId = (*itrP)->getSrc()->getId();//ne prendo il nodo src
                    //if((*itrP)->getDst()->getId()==nSegEndNodeId) cout<<"b:"<<nPNodeId<<"-"<<nSegEndNodeId;////
                    if (((*(--itrP))->getSrc()->getId()==nSegStartNodeId)&&(itrB==(*itr)->m_hLinkList.begin()))
                       {//cout<<"a";
                        UniFiber (*itrPP) = (UniFiber*)(*itrP);
                        //cout<<(*itrPP).getSrc()->getId()<<"a";
                        }
                    ++itrP;
					pBFiber->m_pCSet[nPNodeId]++;//incremento i campi del conflict set relativi a tal nodo
					if (pBFiber->m_pCSet[nPNodeId] > nNewBChannels)//se supero v*_temp  lo incremento 
						nNewBChannels++;
					assert(nNewBChannels >= pBFiber->m_pCSet[nPNodeId]);
				} // fine for itrP

				if (nNewBChannels > pBFiber->m_nBChannels) {//se era aumentato v*_temp
					pBFiber->m_nBChannels++;//aumento v*_definitivo del link
				/*	if(pCircuit->m_eState==Circuit::CT_TemporarySetup)
						pBFiber->consumeChannelSUTemp(NULL,1, -1);
						else*/
					pBFiber->consumeChannel(NULL,1, -1);//AGGIUNTO BACKUP QUI
     				assert(pBFiber->m_nBChannels == nNewBChannels);
				}
                   map<UINT, Lightpath*>::reverse_iterator itrSG;
                   map<UINT, Lightpath*>::const_iterator itrSG1;
				   int b=1; //cout<<"m_bSharingGroup:"<<pBFiber->m_bSharingGroup.size()<<" ";
//shgp=max(shgp,(int)pBFiber->m_bSharingGroup.size());//max numero nello sharing group
				   for (itrSG=pBFiber->m_bSharingGroup.rbegin();itrSG!=pBFiber->m_bSharingGroup.rend();itrSG++){
                       itrSG1=m_bSharingGroupLP.find((*itrSG).first);
                       if(itrSG1==m_bSharingGroupLP.end()){//cout<<(*itrSG).second->UnAvLP<<endl;
                                   m_bSharingGroupLP.insert(pair<UINT, Lightpath*>((*itrSG).first, (*itrSG).second));
  //cout<<"lungh perc in share:"<<(*itrSG).second->m_hPRoute.size();
  delta *=pow(AvWork,(double) (*itrSG).second->m_hPRoute.size());
					              // delta *=((double) 1-((*itrSG).second->UnAvLP));
                       }
				   b++;    // cout.precision(8); cout<<"delta "<<delta<<" "<<endl; //cout.precision(8);cout<<(1 - (*itrSG).second->UnAvLP);//<<"*";
				   } deltatemp +=((double) delta); ns++;  //cout.precision(8); cout<<"deltatemp "<<deltatemp<<" ";// cout<<endl; 
                   //dp=(double) pBFiber->m_nBChannels/ pBFiber->m_bSharingGroup.size();//-t
                   //delta=delta*dp;//-t
			} // for itrB  
			
			//Fabio 20 ott: disabilito aggiornamento dell' availability per le connessioni
			//temporanee rimesse su
			if(pCircuit->m_eState==Circuit::CT_Ready)//solo in questo caso aggiorno i conti
			{
				while(((*itrPP)->getDst()->getId())!=nSegEndNodeId)
				{  itrPP++;
				   bb++;} 
				bb++;
				bb=bb-bc;//#link di working protetto per segmento
				bc=bb;
                ccp=ccp-cpc;//#link backup per segmento
				cpc_=cpc;//#link backup per segmento(segmento precedente)
				cpc=ccp;//cout<<bb<<"*"<<ccp<<" ";
                m_bb=(double) m-bb;
//delta=pow((pow(AvLink,(m))),shgp);//pow(AvWork,(1/m));//max numero nello sharing group
				if((over>=1)&(over<=5)){
				  if(itr!=m_hBackupSegs.begin()){
					AvLPtempOver +=
					(((1-(pow(AvLink,(double)bb)))*(pow(AvLink,(double)m_bb))*(delta*pow(AvLinkB,(double)ccp)))-
					((delta*pow(AvLinkB,(double)cpc_))*(delta*pow(AvLinkB,(double)ccp))*(pow(AvLink,(double)(m-1)))*((double)1-AvLink))); 
					//cout<<"AvLPtempOver:"<<AvLPtempOver<<endl;
				  } else {
					AvLPtempOver +=
					((1-(pow(AvLink,(double)bb)))*(pow(AvLink,(double)m_bb))*(pow(AvLinkB,(double)ccp)));
				    //cout<<"AvLPtempOver:"<<AvLPtempOver<<endl;
				  }
                } else{  UnAvLPtemp*=(1-(pow(AvLink,(double)bb)))*(1-(delta*(pow(AvLinkB,(double)ccp)))); }
			}//if-fabio
			itrPIndex = itrP;
		} // for itr=m_hBackupSegs
	} else {
		// case 2: primary traverses only one hop
		for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {
			UINT nSegEndNodeId = (*itr)->m_hLinkList.back()->getDst()->getId();
            UINT nSegStartNodeId = (*itr)->m_hLinkList.front()->getSrc()->getId();//-t
		  //  cout<<"|"<<(*itr)->m_hLinkList.front()->getSrc()->getId()<<"--"<<(*itr)->m_hLinkList.back()->getDst()->getId()<<"| ";//-t			
			 for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();itrB++)
			 {  ccp++;//-t
				UniFiber *pBFiber = (UniFiber*)(*itrB);
				assert(pBFiber && pBFiber->m_pCSet);
	            //pBFiber->incrSG(this);//-t
				UINT nNewBChannels = pBFiber->m_nBChannels;
				UINT nSrc = m_hPRoute.front()->getSrc()->getId();
				pBFiber->m_pCSet[nSrc]++;
				if (pBFiber->m_pCSet[nSrc] > nNewBChannels)
					nNewBChannels++;
				if (nNewBChannels > pBFiber->m_nBChannels) {
					pBFiber->m_nBChannels++;
				/*	if(pCircuit->m_eState==Circuit::CT_TemporarySetup)
						pBFiber->consumeChannelSUTemp(NULL,1, -1);
						else*/
					pBFiber->consumeChannel(NULL,1, -1);//AGGIUNTO PROTECTION QUI
           		    assert(pBFiber->m_nBChannels == nNewBChannels);
				}
                 map<UINT, Lightpath*>::reverse_iterator itrSG;
                 map<UINT, Lightpath*>::const_iterator itrSG1;
				 int b=1; //cout<<"m_bSharingGroup:"<<pBFiber->m_bSharingGroup.size()<<" ";
//shgp=max(shgp,(int)pBFiber->m_bSharingGroup.size());//max numero nello sharing group
				 //Fabio 20 ott: disabilito aggiornamento dell' availability per le connessioni
			//temporanee rimesse su
				 if(pCircuit->m_eState==Circuit::CT_Ready){//solo in questo caso aggiorno i conti
			  
				 for (itrSG=pBFiber->m_bSharingGroup.rbegin();itrSG!=pBFiber->m_bSharingGroup.rend();itrSG++){
                       itrSG1=m_bSharingGroupLP.find((*itrSG).first);
                       if(itrSG1==m_bSharingGroupLP.end()){//se sto inserendo un nuovo percorso in Share ne uso l'availability
                                   m_bSharingGroupLP.insert(pair<UINT, Lightpath*>((*itrSG).first, (*itrSG).second));
  //cout<<"lungh perc in share:"<<(*itrSG).second->m_hPRoute.size();
  delta *=pow(AvWork,(double) (*itrSG).second->m_hPRoute.size());
					              // delta *=((double) 1-((*itrSG).second->UnAvLP));
                       }  
                   b++;    // cout.precision(8); cout<<"delta "<<delta<<" "<<endl;;//cout.precision(8);cout<<(1 - (*itrSG).second->UnAvLP);//<<"*";
				   } deltatemp +=((double) delta); ns++; //cout.precision(8); cout<<"deltatemp "<<deltatemp<<" ";// cout<<endl; 
                   //dp=(double) pBFiber->m_nBChannels/ pBFiber->m_bSharingGroup.size();//-t
                   //delta=delta*dp;//-t
 			}
			    bb++;  //cout<<bb<<"*"<<ccp<<" ";
//delta=pow((pow(AvLink,(m))),shgp);//pow(AvWork,(1/m)) //max numero nello sharing group
                UnAvLPtemp*=(1-(pow(AvLink,(double)bb)))*(1-(delta*(pow(AvLinkB,(double)ccp))));//-t
	}
	} // if	
	ns--;
	deltaLP=((double) deltatemp/ns);//cout.precision(32);cout<<" deltaLP: "<<deltaLP<<endl;
	}
	
	//Fabio 20 ott: disabilito aggiornamento dell' availability per le connessioni
			//temporanee rimesse su
			if(pCircuit->m_eState==Circuit::CT_Ready)//solo in questo caso aggiorno i conti
			{
	if((over>=1)&(over<=5)){
	AvLPtempOver+=(pow(AvLink,(m)));
    UnAvLP=1-AvLPtempOver;
}else{UnAvLP=UnAvLPtemp;}
    if(pNetMan->m_UnAv)
	cout<</*"UnAvLP"<<*/this->getId()<<'\t'<<UnAvLP<<endl;//-t
			}//IF-fabio
	if (m_hPRoute.size() > 1) {
		// case 1: primary traverse multiple (>1) hops		
       for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {//scorro i vari segmenti di backup
			for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();//scorro i link di ognuno
			itrB++) {
				UniFiber *pBFiber = (UniFiber*)(*itrB);//considero la fibra associata
				pBFiber->incrSG(this);//-t
			} // for itrB          
		} // for itr=m_hBackupSegs
	} else {
		// case 2: primary traverses only one hop
		for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {
			 for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
			itrB++) {
				UniFiber *pBFiber = (UniFiber*)(*itrB);
			    pBFiber->incrSG(this);//-t
			}
	}//
	} // if
} //end NODE DISJOINT
	// Add to lightpath db
	if(pCircuit->m_eState==Circuit::CT_Setup)
	pNetMan->appendLightpath(this, pCircuit->m_eBW);
 
}




void Lightpath_Seg::release(bool& bToDelete, NetMan *pNetMan, 
							Circuit *pCircuit, bool bLog)
{

	//-B: if PT_UNPROTECTED or PT_BBU skip it
	if (pNetMan->m_eProvisionType != NetMan::PT_UNPROTECTED && pNetMan->m_eProvisionType != NetMan::PT_PAL_DPP && pNetMan->m_eProvisionType != NetMan::PT_BBU)
	{
		if(pNetMan->isLinkDisjointActive)
		{
			// update conflict set
			list<AbsPath*>::const_iterator itr;
			list<UniFiber*>::const_iterator itrP;
			list<AbstractLink*>::const_iterator itrB;          
			list<UniFiber*>::const_iterator itrPIndex = m_hPRoute.begin();
			for (itr = m_hBackupSegs.begin(); itr != m_hBackupSegs.end(); itr++)
			{
				 UINT nSegEndNodeId = (*itr)->m_hLinkList.back()->getDst()->getId();
				 for (itrB = (*itr)->m_hLinkList.begin(); itrB != (*itr)->m_hLinkList.end(); itrB++)
				 {
						UniFiber *pBFiber = (UniFiber*)(*itrB);
						assert(pBFiber && pBFiber->m_pCSet);
						for (itrP=m_hPRoute.begin(); itrP!=m_hPRoute.end(); itrP++) {
							UINT nPLinkId = (*itrP)->getId(); 
							if (pBFiber->m_pCSet[nPLinkId] > 0)
								pBFiber->m_pCSet[nPLinkId]--;
						}
						UINT nLinkId;
						UINT nNewBChannels = 0; 
						for (nLinkId=0; nLinkId<pBFiber->m_nCSetSize; nLinkId++) {
							if (pBFiber->m_pCSet[nLinkId] > nNewBChannels) {
								nNewBChannels = pBFiber->m_pCSet[nLinkId];
							}
						}
						if (nNewBChannels < pBFiber->m_nBChannels) {
							pBFiber->m_nBChannels--;
							pBFiber->releaseChannel(NULL);
							assert(pBFiber->m_nBChannels == nNewBChannels);
						}
				 } // for itrB
				 //itrPIndex = itrP;
			} // for itr
		} 
		else
		{
			// update conflict set
			list<AbsPath*>::const_iterator itr;
			list<UniFiber*>::const_iterator itrP;
			list<AbstractLink*>::const_iterator itrB;
			if (m_hPRoute.size() > 1) {
				// case 1: primary traverses multiple (>1) hops
				list<UniFiber*>::const_iterator itrPIndex = m_hPRoute.begin();
				itrPIndex++;	// only consider intermediate node failures
				for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {
					UINT nSegEndNodeId = (*itr)->m_hLinkList.back()->getDst()->getId();
					for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
					itrB++) {
						UniFiber *pBFiber = (UniFiber*)(*itrB);
						assert(pBFiber && pBFiber->m_pCSet);
						//-t
						pBFiber->decrSG(this);
						for (itrP=itrPIndex; ((itrP!=m_hPRoute.end()) 
							&& ((*itrP)->getSrc()->getId()!=nSegEndNodeId)); itrP++) {
							UINT nPNodeId = (*itrP)->getSrc()->getId();
							assert(pBFiber->m_pCSet[nPNodeId] > 0);
							pBFiber->m_pCSet[nPNodeId]--;
						}
						UINT nNodeId;
						UINT nNewBChannels = 0; 
						for (nNodeId=0; nNodeId<pBFiber->m_nCSetSize; nNodeId++) {//scorro tutti i nodi del conflict set del link di backup per trovare il max
							if (pBFiber->m_pCSet[nNodeId] > nNewBChannels) {
								nNewBChannels = pBFiber->m_pCSet[nNodeId];
							}
						}
						if (nNewBChannels < pBFiber->m_nBChannels) {//se il max trovato e' minore dei canali allocati libero un canale
							pBFiber->m_nBChannels--;
							pBFiber->releaseChannel(NULL);
							//pBFiber->releaseChannel_b(this,1);//-t
							assert(pBFiber->m_nBChannels == nNewBChannels);
						}//else{pBFiber->releaseChannel_b(this,0);}//-t
					} // for itrB
					itrPIndex = itrP;
				} // for itr
			} else {
				// case 2: primary traverses only one hop
				for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {
					for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
					itrB++) {
						UniFiber *pBFiber = (UniFiber*)(*itrB);
						assert(pBFiber && pBFiber->m_pCSet);
						//-t
						pBFiber->decrSG(this);
						UINT nSrc = m_hPRoute.front()->getSrc()->getId();
						assert(pBFiber->m_pCSet[nSrc] > 0);
						pBFiber->m_pCSet[nSrc]--;
						UINT nNodeId;
						UINT nNewBChannels = 0; 
						for (nNodeId=0; nNodeId<pBFiber->m_nCSetSize; nNodeId++) {//scorro tutti i nodi del conflict set del link di backup per trovare il max
							if (pBFiber->m_pCSet[nNodeId] > nNewBChannels) {
								nNewBChannels = pBFiber->m_pCSet[nNodeId];
							}
						}
						if (nNewBChannels < pBFiber->m_nBChannels) {//se il max trovato e' minore dei canali allocati libero un canale
							pBFiber->m_nBChannels--;
							pBFiber->releaseChannel(NULL);
							//pBFiber->releaseChannel_b(this,1);//-t
							assert(pBFiber->m_nBChannels == nNewBChannels);
						}//else{pBFiber->releaseChannel_b(this,0);}//-t
					}
				}
			}
		} //END NODE DISJOINT

	}//if iniziale
	m_nFreeCapacity = m_nCapacity;
	if (bLog)	
		logFinal(pNetMan->m_hLog);
	pNetMan->removeLightpath(this);
	// This MUST be the last thing to do.
/*	if (pCircuit->m_eState==3)//fabio 7 nov: rimosso
		tDORTemp(m_hPRoute);
	else*/
	tearDownOneRoute(m_hPRoute);


	bToDelete = true;
}

//-B: setup a NEW Lightpath (VWP case)
void Lightpath_Seg::WPsetUp(NetMan* pNetMan, Circuit* pCircuit)
{
	LINK_COST eps = pNetMan->m_hWDMNet.m_dEpsilon;
	if (0 == m_nLinkId)
		m_nLinkId = LightpathDB::getNextId();
	if (NULL == m_pSrc)
		m_pSrc = m_hPRoute.front()->getSrc();
	
	if (NULL == m_pDst)
		m_pDst = m_hPRoute.back()->getDst();
	//-B: sottraggo dalla capacità disponibile invece che da OCLightpath.
	//	Tanto essendo un nuovo Lightpath, la cap disponibile corrisponde a quella totale
	m_nFreeCapacity = m_nFreeCapacity - pCircuit->m_eBW; //-L: ??? to change
	//m_nFreeCapacity = OCLightpath - pCircuit->m_eBW;

	// Update wavelength usage along the primary

/*	if(pCircuit->m_eState==3)//TempTornDown
		updateChannelUsageTemp(m_hPRoute);
		else*/
	list<AbsPath*>::const_iterator itr;
    list<AbstractLink*>::const_iterator itrB;
	list<UniFiber*>::const_iterator itrC;
	list<UniFiber*>::const_iterator startPoint;
	updateChannelUsage(m_hPRoute);


	if (pNetMan->m_eProvisionType==NetMan::PT_wpUNPROTECTED || pNetMan->m_eProvisionType == NetMan::PT_BBU)
	{
		if (pCircuit->m_eState == Circuit::CT_Setup)
			pNetMan->appendLightpath(this, pCircuit->m_eBW);
		return; //-B: evito il resto del codice di questo metodo che tratta il caso in cui ci sia da fare protection
	}

//////////----------- case UNPROTECTED or BBU: STOP!!!!!!! ----------------

	if (pNetMan->m_eProvisionType==NetMan::PT_wpPAL_DPP)
	{
		for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) 
		{//scorro i vari segmenti di backup
     		for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end(); itrB++) //scorro i link di ognuno
			{   
				UniFiber *pBFiber = (UniFiber*)(*itrB); //downcast dell iteratore ad UniFiber
				pBFiber->consumeChannel(this, 0, this->wlAssignedBCK); //AGGIUNTO BACKUP QUI
				pBFiber->wlOccupation[this->wlAssignedBCK] = UNREACHABLE;
				pBFiber->m_nBChannels++; //AGGIUNGO SEMPRE..tanto non potro mai condividere..
			}
		}
		// Add to lightpath db
		if(pCircuit->m_eState==Circuit::CT_Setup)
		pNetMan->appendLightpath(this, pCircuit->m_eBW);
		return; //-B: evito il resto del codice di questo metodo che tratta il caso in cui ci sia da fare protection
	}
	//////////----------- case UNPROTECTED or BBU: STOP!!!!!!! ----------------

if(pNetMan->isLinkDisjointActive)
{
	for(itrC=m_hPRoute.begin();itrC!=m_hPRoute.end();itrC++)
{
	
	primaryLinkID.push_back((*itrC)->getId());//lista dei nodi attraversati dal primario(escluso il destinazione)
	(*itrC)->LPinFiber.insert(this);
	}

	for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {//scorro i vari segmenti di backup
	
     		for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end(); itrB++) //scorro i link di ognuno
			{   
				UniFiber *pBFiber = (UniFiber*)(*itrB);//downcast dell iteratore ad UniFiber
				
				pBFiber->consumeChannel(NULL,1,this->wlAssignedBCK );//AGGIUNTO BACKUP QUI
				pBFiber->wlOccupation[this->wlAssignedBCK]=UNREACHABLE;
				pBFiber->wlOccupationBCK[this->wlAssignedBCK]=eps;//fabio 10 genn:eps al posto di inf

				if (pNetMan->m_eProvisionType!=NetMan::PT_SPPBw && pNetMan->m_eProvisionType!=NetMan::PT_SPPCh) 
				if(pBFiber->m_pChannel[this->wlAssignedBCK].canaliProtetti==1) //SE è STATO CREATO DA ZERO UN NUOVO CANALE DI PROTEZIONE
				pBFiber->m_nBChannels++;

///////////////////
			 UINT nNewBChannels = pBFiber->m_nBChannels;
				for(itrC=m_hPRoute.begin();itrC!=m_hPRoute.end();itrC++)
				{  
			

 
                    UINT nPLinkId = (*itrC)->getId();
                    pBFiber->m_pCSet[nPLinkId]++;
                    if (pBFiber->m_pCSet[nPLinkId] > nNewBChannels)
                        nNewBChannels++;
                    assert(nNewBChannels >= pBFiber->m_pCSet[nPLinkId]);
                }
                if (nNewBChannels > pBFiber->m_nBChannels) {
                    pBFiber->m_nBChannels++;
                  
                    assert(pBFiber->m_nBChannels == nNewBChannels);
                }
///////////////////
		
				for(int i=0;i<primaryLinkID.size();i++)
				{

				pBFiber->m_pChannel[this->wlAssignedBCK].linkProtected[primaryLinkID[i]]=true;
				}
				}
		}
}
else //NODE DISJOINT..
{
if(m_hPRoute.size()>1)
		{
startPoint=m_hPRoute.begin();
startPoint++;

		}
else  startPoint=m_hPRoute.begin();

for(itrC=startPoint;itrC!=m_hPRoute.end();itrC++)
{
	
	primaryNodesID.push_back((*itrC)->getSrc()->getId());//lista dei nodi attraversati dal primario(escluso il destinazione)
	(*itrC)->getSrc()->LPinNode.insert(this);
}
		for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {//scorro i vari segmenti di backup
	
     		for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end(); itrB++) //scorro i link di ognuno
			{   
				UniFiber *pBFiber = (UniFiber*)(*itrB);//downcast dell iteratore ad UniFiber
				
				pBFiber->consumeChannel(NULL,1,this->wlAssignedBCK );//AGGIUNTO BACKUP QUI
				pBFiber->wlOccupation[this->wlAssignedBCK]=UNREACHABLE;
				pBFiber->wlOccupationBCK[this->wlAssignedBCK]=eps;//fabio 10 genn:eps al posto di inf
			
				
				///////////////////
				 UINT nNewBChannels = pBFiber->m_nBChannels;
				for(itrC=startPoint;itrC!=m_hPRoute.end();itrC++)
				{  
			

 
                    UINT nPNodeId = (*itrC)->getSrc()->getId();
                    pBFiber->m_pCSet[nPNodeId]++;
                    if (pBFiber->m_pCSet[nPNodeId] > nNewBChannels)
                        nNewBChannels++;
                    assert(nNewBChannels >= pBFiber->m_pCSet[nPNodeId]);
                }
                if (nNewBChannels > pBFiber->m_nBChannels) {
                    pBFiber->m_nBChannels++;
                   // pBFiber->consumeChannel(NULL,1, -1);
                    assert(pBFiber->m_nBChannels == nNewBChannels);
                }
///////////////////
				for(int i=0;i<primaryNodesID.size();i++)
				{

				pBFiber->m_pChannel[this->wlAssignedBCK].nodesProtected[primaryNodesID[i]]=true;
				}
				}
		}
		}//End NODE DISJOINT
	// Add to lightpath db
	if(pCircuit->m_eState==Circuit::CT_Setup)
	pNetMan->appendLightpath(this, pCircuit->m_eBW);
 
}


void Lightpath_Seg::WPrelease(bool& bToDelete, NetMan *pNetMan, 
							Circuit *pCircuit, bool bLog)
{
#ifdef DEBUGB
	cout << "-> WPrelease Lightpath_Seg" << endl;
#endif // DEBUGB
	//-B: if PT_BBU, it doesn't enter into this cycle
	if (pNetMan->m_eProvisionType !=NetMan::PT_wpUNPROTECTED && pNetMan->m_eProvisionType != NetMan::PT_wpPAL_DPP && pNetMan->m_eProvisionType != NetMan::PT_BBU)
	{
		list<UniFiber*>::const_iterator startPoint;
		list<UniFiber*>::const_iterator itrC;
		list<AbsPath*>::const_iterator itr;
		list<AbstractLink*>::const_iterator itrB;
		if(pNetMan->isLinkDisjointActive)
		{
			for(itrC = m_hPRoute.begin(); itrC != m_hPRoute.end(); itrC++)
			{
				int count = (*itrC)->LPinFiber.erase(this);
				if (count == 0)
					cout<< "assertion failed!" <<endl; //-B: commented by me, since it is showed but I don't care
			}
			for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++)
			{
				for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end(); itrB++) 
				{
					UniFiber *pBFiber = (UniFiber*)(*itrB);
					for(itrC=m_hPRoute.begin();itrC!=m_hPRoute.end();itrC++)
					{	
////////////////////////////
                    UINT nPLinkId = (*itrC)->getId(); 
					assert(pBFiber->m_pCSet[nPLinkId] > 0);
                  
                        pBFiber->m_pCSet[nPLinkId]--;
                
                UINT nLinkId;
                UINT nNewBChannels = 0; 
                for (nLinkId=0; nLinkId<pBFiber->m_nCSetSize; nLinkId++)
				{
                    if (pBFiber->m_pCSet[nLinkId] > nNewBChannels)
					{
                        nNewBChannels = pBFiber->m_pCSet[nLinkId];
                    }
                }
                if (nNewBChannels < pBFiber->m_nBChannels) {
                    pBFiber->m_nBChannels--;
                  //  pBFiber->releaseChannel(NULL);
                    assert(pBFiber->m_nBChannels == nNewBChannels);
                }
////////////////////////////
				
				}
					pBFiber->m_pChannel[this->wlAssignedBCK].release();
					pBFiber->wlOccupation[this->wlAssignedBCK]=UNREACHABLE;
					if(pBFiber->m_pChannel[this->wlAssignedBCK].canaliProtetti==0)
						{
						pBFiber->wlOccupation[this->wlAssignedBCK]=1;//FABIO 13 febb:cambiamento!!!
					pBFiber->wlOccupationBCK[this->wlAssignedBCK]=1;//se non sto piu proteggendo canali,rimuovo il backup
					if (pNetMan->m_eProvisionType!=NetMan::PT_SPPBw && pNetMan->m_eProvisionType!=NetMan::PT_SPPCh)
					pBFiber->m_nBChannels--;
				if(pBFiber->m_nBChannels<0) cout<<"FAIL";
					}
				for(int i=0;i<primaryLinkID.size();i++)
				pBFiber->m_pChannel[this->wlAssignedBCK].linkProtected[primaryLinkID[i]]=false;
                    //pBFiber->releaseChannel_b(this,1);//-t
				
				}//else{pBFiber->releaseChannel_b(this,0);}//-t
			}
}//END LINK DISJOINT
else{  //NODE DISJOINT
if(m_hPRoute.size()>1)
		{
startPoint=m_hPRoute.begin();
startPoint++;

		}
else  startPoint=m_hPRoute.begin();
	

	
	for(itrC=startPoint;itrC!=m_hPRoute.end();itrC++)
{
//	primaryLinkID.push_back((*itrC)->m_nLinkId);
	int count=(*itrC)->getSrc()->LPinNode.erase(this);
	assert(count>0);
}
	LINK_COST eps=pNetMan->m_hWDMNet.m_dEpsilon;

		for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) 		{
			for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
			itrB++) 
			{
				UniFiber *pBFiber = (UniFiber*)(*itrB);
			
				for(itrC=startPoint;itrC!=m_hPRoute.end();itrC++)
				{
////////////////////////////
   
                    UINT nPNodeId = (*itrC)->getSrc()->getId(); 
					assert(pBFiber->m_pCSet[nPNodeId] > 0);
                    if (pBFiber->m_pCSet[nPNodeId] > 0)
                        pBFiber->m_pCSet[nPNodeId]--;
                
                UINT nLinkId;
                UINT nNewBChannels = 0; 
                for (nLinkId=0; nLinkId<pBFiber->m_nCSetSize; nLinkId++) 
				{
                    if (pBFiber->m_pCSet[nPNodeId] > nNewBChannels)
					{
                        nNewBChannels = pBFiber->m_pCSet[nPNodeId];
                    }
                }
                if (nNewBChannels < pBFiber->m_nBChannels)
				{
                    pBFiber->m_nBChannels--;
                  //  pBFiber->releaseChannel(NULL);
                    assert(pBFiber->m_nBChannels == nNewBChannels);
                }
////////////////////////////
}
				assert(pBFiber->m_pCSet[(*itrC)->getSrc()->getId()]>=0);
					pBFiber->m_pChannel[this->wlAssignedBCK].release();
					pBFiber->wlOccupation[this->wlAssignedBCK]=UNREACHABLE;
					if(pBFiber->m_pChannel[this->wlAssignedBCK].canaliProtetti==0)
						{
						pBFiber->wlOccupation[this->wlAssignedBCK]=1;//FABIO 13 febb:cambiamento!!!
					pBFiber->wlOccupationBCK[this->wlAssignedBCK]=1;//se non sto piu proteggendo canali,rimuovo il backup
				}
				for(int i=0;i<primaryNodesID.size();i++)
				pBFiber->m_pChannel[this->wlAssignedBCK].nodesProtected[primaryNodesID[i]]=false;
                    //pBFiber->releaseChannel_b(this,1);//-t
				
				}//else{pBFiber->releaseChannel_b(this,0);}//-t
			}
		}//END NODE DISJOINT
	}//if iniziale
		//-B: if PT_BBU, it doesn't enter into this cycle
		if(pNetMan->m_eProvisionType == NetMan::PT_wpPAL_DPP)
		{
			list<AbsPath*>::const_iterator itr;
			list<AbstractLink*>::const_iterator itrB;
	for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) 		{
			for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
			itrB++) 
			{
				UniFiber *pBFiber = (UniFiber*)(*itrB);
			
					pBFiber->m_pChannel[this->wlAssignedBCK].release();
					pBFiber->wlOccupation[this->wlAssignedBCK]=1;
					pBFiber->m_nBChannels--;
						}
			}
		}
		m_nFreeCapacity = m_nCapacity;
		if (bLog)	
			logFinal(pNetMan->m_hLog); //FABIO TOLTO 7 genn
		pNetMan->removeLightpath(this);
		
		// This MUST be the last thing to do.
		/*	if (pCircuit->m_eState==3)//fabio 7 nov: rimosso
				tDORTemp(m_hPRoute);
			else*/
		tearDownOneRoute(m_hPRoute);


	bToDelete = true;
}

UniFiber* Lightpath_Seg::LookUpByLinkId(int LinkId2, list<AbstractLink*>& hLinkToBeD3)
{
		 list<AbstractLink*>::const_iterator itrUF;
                 for (itrUF=hLinkToBeD3.begin(); itrUF!=hLinkToBeD3.end(); itrUF++) {
		      UniFiber *pUF = (UniFiber*)(*itrUF);
                      assert(pUF);		
                      if  ((pUF-> m_nLinkId)== LinkId2)
		     { return pUF;
		     break;}
               	}
                	
                // cerr <<  pUF-> m_hCost << endl; 
}

void Lightpath_Seg::releaseOnServCopy(list<AbstractLink*>& hLinkToBeD2)
{
	// update conflict set
 
	list<AbsPath*>::const_iterator itr;
	list<UniFiber*>::const_iterator itrP;
	list<AbstractLink*>::const_iterator itrB;
	if (m_hPRoute.size() > 1) {
		// case 1: primary traverses multiple (>1) hops
		list<UniFiber*>::const_iterator itrPIndex = m_hPRoute.begin();
		// Cerca di trasformarlo in funzione 
		/* list<AbstractLink*>::const_iterator itrUF;
                for (itrUF=hLinkToBeD.begin(); itrUF!=hLinkToBeD.end(); itrUF++) {
		 pUF = (UniFiber*)(*itrUF);
                 assert(pUniFiber);		
                 if  ((pUF-> m_hLinkId)== NodeServId)) 
		 break;
                // cerr <<  pUF-> m_hCost << endl; 
		}*/
		itrPIndex++;	// only consider intermediate node failures
		for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {
			UINT nSegEndNodeId = (*itr)->m_hLinkList.back()->getDst()->getId();
			for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
			itrB++) {
				UniFiber *pBFiber = (UniFiber*)(*itrB);
                                int LinkServId= pBFiber->getId();
                                //cerr << "originale" << pBFiber->getId()<< "-"<<  pBFiber->getSrc()->getId() << "-"<<   pBFiber->getDst()->getId() <<endl; 

                                UniFiber *pBFiberInLTBD   =  LookUpByLinkId(LinkServId, hLinkToBeD2);
				// cerr << "copia" << pBFiberInLTBD->getId()<< "-"<< pBFiberInLTBD->getSrc()->getId() << "-"<<   pBFiberInLTBD->getDst()->getId() << endl; 

				assert(pBFiberInLTBD && pBFiberInLTBD->m_pCSet);
				for (itrP=itrPIndex; ((itrP!=m_hPRoute.end()) 
					&& ((*itrP)->getSrc()->getId()!=nSegEndNodeId)); itrP++) {
					UINT nPNodeId = (*itrP)->getSrc()->getId();

                                        //giusto per non bloccarmi ma andrebbe controllato il perche
                                        //cerr << "Original CS of nPNodeId" <<  pBFiber->m_pCSet[nPNodeId]<< endl;  
                                        //cerr << "Copy CS of nPNodeId" <<  pBFiberInLTBD->m_pCSet[nPNodeId]<< endl;  
                     			assert(pBFiberInLTBD->m_pCSet[nPNodeId] > 0);
                                        if(pBFiberInLTBD->m_pCSet[nPNodeId] > 0)
					//pBFiber->m_pCSet[nPNodeId]--;

					//cerr << "originale" << pBFiber->m_pCSet[nPNodeId]<< endl; 
                                	//cerr << "copia" << pBFiberInLTDB->m_pCSet[nPNodeId]<< endl; 
                                	pBFiberInLTBD->m_pCSet[nPNodeId]--;
				}
				UINT nNodeId;
				UINT nNewBChannels = 0; 
				for (nNodeId=0; nNodeId<pBFiber->m_nCSetSize; nNodeId++) {
					if (pBFiberInLTBD->m_pCSet[nNodeId] > nNewBChannels) {
						nNewBChannels = pBFiberInLTBD->m_pCSet[nNodeId];
					}
				}
				if (nNewBChannels < pBFiberInLTBD->m_nBChannels) {
					pBFiberInLTBD->m_nBChannels--;
					pBFiberInLTBD->releaseChannel(this);
					assert(pBFiberInLTBD->m_nBChannels == nNewBChannels);
				}
			} // for itrB
			itrPIndex = itrP; //????
		} // for itr
	} else {
		// case 2: primary traverses only one hop
		for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {
			for (itrB=(*itr)->m_hLinkList.begin(); itrB!=(*itr)->m_hLinkList.end();
			itrB++) {
				UniFiber *pBFiber = (UniFiber*)(*itrB);
                                int LinkServId= pBFiber->getId();
                                UniFiber *pBFiberInLTBD=LookUpByLinkId(LinkServId, hLinkToBeD2);
				assert(pBFiberInLTBD && pBFiberInLTBD->m_pCSet);
				UINT nSrc = m_hPRoute.front()->getSrc()->getId();
				assert(pBFiberInLTBD->m_pCSet[nSrc] > 0);
				pBFiberInLTBD->m_pCSet[nSrc]--;

				UINT nNodeId;
				UINT nNewBChannels = 0; 
				for (nNodeId=0; nNodeId<pBFiberInLTBD->m_nCSetSize; nNodeId++) {
					if (pBFiberInLTBD->m_pCSet[nNodeId] > nNewBChannels) {
						nNewBChannels = pBFiberInLTBD->m_pCSet[nNodeId];
					}
				}
				if (nNewBChannels < pBFiberInLTBD->m_nBChannels) {
					pBFiberInLTBD->m_nBChannels--;
					pBFiberInLTBD->releaseChannel(NULL);
					assert(pBFiberInLTBD->m_nBChannels == nNewBChannels);
				}
			}
		}
	}    
	/*m_nFreeCapacity = m_nCapacity;
	if (bLog)	
		logFinal(pNetMan->m_hLog);
		pNetMan->removeLightpath(this);*/

	//tearDownOneRoute(m_hPRoute);--> Subsitute with another version of TearDownOneRoute 
        //  which affects directly -hLinkToBeD2-
        list<UniFiber*>::iterator itrPR;
        for (itrPR=m_hPRoute.begin(); itrPR!=m_hPRoute.end(); itrPR++)
             {  UniFiber *pBFiber = (UniFiber*)(*itrPR);
                int LinkServId= pBFiber->getId();
                UniFiber *pBFiberInLTBD=LookUpByLinkId(LinkServId, hLinkToBeD2);
		//cerr << "lightpath passato" << getId() << endl;
                pBFiberInLTBD->releaseChannel(this);   // release a channel this o NULL???
	}
         //bToDelete = true;
}

void Lightpath_Seg::logPeriodical(Log& hLog, SimulationTime hTimeSpan)
{//cout<<"a";
	m_hLifeTime += hTimeSpan;
	m_hBusyTime += hTimeSpan;
}

void Lightpath_Seg::logFinal(Log& hLog)
{
    hLog.UnAvLPMedia +=UnAvLP;//-t
	hLog.N +=1;//-t
    hLog.deltaMedio +=deltaLP; //cout.precision(32);cout<<" deltaMedio:"<<hLog.deltaMedio<<endl;//-t

	UINT nPHops = m_hPRoute.size()-1;  //modificato per non contare l'hop tra DC e dummy node
	if(backupLength!=-2)//CASO VWP DEDICATO...
	{
		hLog.m_dSumLinkLoad += (double)backupLength;
		hLog.m_dSumLinkLoad += (double)primaryLength;
		hLog.m_dPrimaryRc +=(double)primaryLength;
	}
	else
	{	
		hLog.m_nPHopDistance += nPHops;
		hLog.m_nSegments += m_hBackupSegs.size();
		hLog.m_dSumLinkLoad += (double)(m_hLifeTime * (double)nPHops);
		hLog.m_dPrimaryRc += (double)(m_hLifeTime * (double)nPHops);
	}
	UINT nBHops = 0;
	AbstractNode *pSegSrc, *pSegDst;
	list<AbsPath*>::const_iterator itr;
	for (itr=m_hBackupSegs.begin(); itr!=m_hBackupSegs.end(); itr++) {
		nBHops += (*itr)->m_hLinkList.size();

		// segments can overlap on primary
		pSegSrc = (*itr)->m_hLinkList.front()->m_pSrc;
		pSegDst = (*itr)->m_hLinkList.back()->m_pDst;
		list<UniFiber*>::const_iterator itrP = m_hPRoute.begin();
		while ((itrP != m_hPRoute.end()) && ((*itrP)->m_pSrc != pSegSrc)) {
			itrP++;
		}
//////
/*if (itrP == m_hPRoute.end()){
list<UniFiber*>::const_iterator itruddd ;
cout<<"m_hPRoute-----------------------------------------------"<<endl;
for (itruddd=m_hPRoute.begin(); itruddd!=m_hPRoute.end(); itruddd++){
(*itruddd)->dump(cout);
//cout<<endl;
}
cout<<endl;}*/
//////
//		assert(itrP != m_hPRoute.end());//se non verificata nodo src PPath diverso da nodo src primo Segmento
		UINT nPSegHops = 1;
		while ((itrP != m_hPRoute.end()) && ((*itrP)->m_pDst != pSegDst)) {
			itrP++;
			nPSegHops++;
		}
		hLog.m_nPHopDistance += nPSegHops;
	}
	hLog.m_nBHopDistance += nBHops;
}
