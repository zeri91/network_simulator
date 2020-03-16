#pragma warning(disable: 4018)
#include <iostream>
#include <assert.h>
#include <math.h>
#include <vector>
#include <map>
#include <random>
#include "OchInc.h"
#include "OchObject.h"
#include "AbsPath.h"//FABIO
#include "AbstractPath.h"//FABIO
#include "MappedLinkList.h"//FABIO
#include "AbstractGraph.h"//FABIO
#include "Event.h"
#include "EventList.h"
#include "Lightpath.h"//FABIO
#include "Lightpath_Seg.h"//FABIO
#include "Connection.h"//FABIO

#include "Vertex.h"
#include "SimplexLink.h"
#include "Graph.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "ConnectionDB.h"
#include "LightpathDB.h"
#include "Log.h"
#include "OXCNode.h"
#include "NetMan.h"
#include "Circuit.h"//FABIO
#include <functional>//FABIO
#include <algorithm> //FABIO
#include <limits>
#include "OchMemDebug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


using namespace NS_OCH;


/*
class EventTimeLess: public less<Event*>
	{
	int value;
	public:
		explicit EventTimeLess(const int val): value(val){}

		bool operator()(const Event*& ev) const {
			return (ev->m_hTime < value);
		}
	};*/
EventList::EventList(): m_pHead(NULL), m_pTail(NULL),size(0), oldEventList(deque<Event*>(0)), numArr(0), numDep(0)
{
}

//FABIO:Definito costruttore x copia
EventList::EventList(const EventList& e)
{
/*
m_pHead= new EventListItem(NULL,NULL,NULL);
m_pHead->m_pEvent= e.m_pHead->m_pEvent;

m_pTail= new EventListItem(NULL,NULL,NULL);
m_pTail= e.m_pTail;
*/
}

EventList::~EventList()
{
	EventListItem *pCurrent = m_pHead;
	EventListItem *pNext;
	while (pCurrent) {
		pNext = pCurrent->m_pNext;
		delete pCurrent->m_pEvent;
		delete pCurrent;
		pCurrent = pNext;
	}
	m_pHead = NULL;
	m_pTail = NULL;
	
}

void EventList::insertEvent(Event* pEvent)
{
	size++;
	// Empty list case
	if (NULL == m_pHead) {
		m_pHead = new EventListItem(pEvent, NULL, NULL);
		m_pTail = m_pHead;
		return;
	}
	// Insert to the head of the list
	if (m_pHead->m_pEvent->m_hTime > pEvent->m_hTime) {
		EventListItem *pItem = new EventListItem(pEvent, NULL, m_pHead);
		m_pHead->m_pPrev = pItem;
		m_pHead = pItem;
		return;
	}

	// Insert to the tail of the list
	if (m_pTail->m_pEvent->m_hTime <= pEvent->m_hTime) {
		EventListItem *pItem = new EventListItem(pEvent, m_pTail, NULL);
		m_pTail->m_pNext = pItem;
		m_pTail = pItem;
		return;
	}

	// Insert to the middle of the list, before pCurrent
	EventListItem *pCurrent = m_pHead;
	while (pCurrent->m_pEvent->m_hTime <= pEvent->m_hTime) {
		pCurrent = pCurrent->m_pNext;
	}
	assert(pCurrent);
	EventListItem *pItem = new EventListItem(pEvent, pCurrent->m_pPrev, pCurrent);
	pCurrent->m_pPrev->m_pNext = pItem;
	pCurrent->m_pPrev = pItem;
}

Event* EventList::nextEvent()
{
#ifdef DEBUG
	cout << "\n-> NextEvent" << endl;
#endif // DEBUGB

	if (NULL == m_pHead)
		return NULL;

	//-B: faccio puntare m_pHead all'evento successivo
	//	e faccio puntare pCurrent, e quindi pEvent, all'evento che sto per andare a considerare
	//	durante iterazione del ciclo while che sto per iniziare
	EventListItem *pCurrent = m_pHead;
	Event *pEvent = pCurrent->m_pEvent;
	m_pHead = m_pHead->m_pNext;
	delete pCurrent;
	size--;

#ifdef DEBUG
	cout << "\tL'evento selezionato da nextEvent e': ";
	if (pEvent->m_hEvent == 1)
	{
		cout << "ARRIVAL" << endl;
	}
	else
	{
		cout << "DEPARTURE" << endl;
	}
#endif // DEBUGB

	return pEvent;
}

void EventList::ExtractDepEvents(list<Event*>& hDepEvents)
{//FABIO:da riscrivere per la old Event List
        EventListItem *pCurrent = m_pHead;
        while (pCurrent) {
	       Event *DepEvent = pCurrent->m_pEvent;
	       if (DepEvent->m_hEvent == Event::EVT_DEPARTURE)  
	  
		 hDepEvents.push_back(pCurrent->m_pEvent);
	        
	       pCurrent = pCurrent->m_pNext;
        }
}

SimulationTime EventList::nextPoissonArrival(double dArrivalRate)
{
	return expDistribution(dArrivalRate);
}

SimulationTime EventList::nextBernoulliArrival(double dArrivalRate)
{
	return expDistribution(dArrivalRate);
}

SimulationTime EventList::expHoldingTime(double dHoldingTime)
{
	return expDistribution(dHoldingTime);
}

inline double EventList::expDistribution(double dAlpha)
{
	assert(dAlpha != 0);
	return ((-1.0/dAlpha) * log(1.0 - rand()/ (RAND_MAX + 1.0)));
}


void EventList::dump(ostream& out) const
{
	cout << "###################" << endl;
	if (NULL == m_pHead) {
		out<<"NULL EventList"<<endl;
		cout << endl << "###################" << endl << endl;
		return;
	}
	EventListItem *pCurrent = m_pHead;
	while (pCurrent) {
		pCurrent->m_pEvent->dump(out);
		pCurrent = pCurrent->m_pNext;
	}
	cout << endl << "###################" << endl << endl;
	cin.get();
	return;
}

//TRACCIA DEI SEQ # DELLE LISTE

void EventList::dump2(ostream& out) {



cout<< " m-h LIST SEQ NUMBER" << "\n";
		

	EventListItem *pCurrent = m_pHead;
        while (pCurrent) {
			if(pCurrent->m_pEvent->m_pConnection==NULL)
			{
				out<<"AnA ";//arrivo non assegnato
				pCurrent = pCurrent->m_pNext;
				continue;
			}
	       int seq = pCurrent->m_pEvent->m_pConnection->m_nSequenceNo;
	       
	        
		   if(pCurrent->m_pEvent->m_hEvent == Event::EVT_ARRIVAL)
			 cout<< seq << "Ar ";
			
		   else if(pCurrent->m_pEvent->m_hEvent == Event::EVT_DEPARTURE)
		   cout<< seq << "Dep ";

else
out<< "N/A";

pCurrent = pCurrent->m_pNext;
		
		}
out<< "\n\n";


cout<< "OLD LIST SEQ NUMBER" << "\n";
		deque<Event*>::iterator itr3;

	for(itr3=oldEventList.begin();itr3 !=oldEventList.end();itr3++)
	{
		int seq =(*itr3)->m_pConnection->m_nSequenceNo;


   if((*itr3)->m_hEvent ==Event::EVT_ARRIVAL)
			 cout<< seq << "Ar ";
			
		   else if((*itr3)->m_hEvent ==Event::EVT_DEPARTURE)
		   cout<< seq << "Dep ";
	}

cout<< "\n======================================\n\n";

}
	//FABIO:Seguono nuove funzioni da me create

//rimuove un evento dalla lista m_hEvent
void EventList::DeleteEvent(Event* rEvent)
{
	
		
		EventListItem *pCurrent = m_pHead;
		
        while (pCurrent) 
		{
	       Event *ArrEvent = pCurrent->m_pEvent;//ArrEvent..nome fuorviante in questo caso
	       if (ArrEvent == rEvent)  
		   {
			   
				 if ( pCurrent==m_pTail && pCurrent==m_pHead )
					{

						m_pHead=NULL;
						m_pTail=NULL;
						size--;
						assert(size==0);
						break;
					}
			   		else if ( pCurrent==m_pHead)
					{
						m_pHead=pCurrent->m_pNext;
						pCurrent->m_pNext->m_pPrev=NULL;
						size--;
						break;
					}

					else if ( pCurrent==m_pTail )
					{

						m_pTail=pCurrent->m_pPrev;
						m_pTail->m_pNext=NULL;
						size--;
						break;
					}
				
					else if( (pCurrent->m_pPrev != NULL) && (pCurrent->m_pNext != NULL))//SE Prev e Next NON sono NULL
					{
						pCurrent->m_pPrev->m_pNext=pCurrent->m_pNext;
						pCurrent->m_pNext->m_pPrev=pCurrent->m_pPrev;
						size--;
						break;
					}

		   }//if
			 
	       pCurrent = pCurrent->m_pNext;
		}

	delete pCurrent;

}

//estrae UN particolare evento di partenza corrispondente ad un dato seq# della connessione
/*Event* EventList::ExtractDepEvent(list<Event*>& backup,int seqN)
{
 EventListItem *pCurrent = m_pHead;
        while (pCurrent) {
	       Event *DepEvent = pCurrent->m_pEvent;
	       if (DepEvent->m_hEvent == Event::EVT_DEPARTURE && DepEvent->m_pConnection->m_nSequenceNo== seqN)  
		   {
		 backup.push_back(pCurrent->m_pEvent);
		 return  pCurrent->m_pEvent;
		   }
	       pCurrent = pCurrent->m_pNext;
}
		return NULL;
}

*/
//ridefinizione di nextEvent gia esistente..l'argomento indica l'intervallo "a"
//

Event* EventList::nextEvent(SimulationTime interval, NetMan* man)
{
#ifdef DEBUGB
	cout << "-> NextEvent" << endl;
#endif // DEBUGB
	if (NULL == m_pHead)
		return NULL;
	EventListItem *pCurrent = m_pHead;
	Event *pEvent = pCurrent->m_pEvent;
	m_pHead = m_pHead->m_pNext;
	size--;

	//se è un arrivo,inserisco in ArrivalList
	/*if(pEvent->m_hEvent ==Event::EVT_ARRIVAL)
	{
		ArrivalList.push_back(pEvent);
		//..e inserisco la rispettiva partenza in DepartureList
		Event* dEvent=ExtractDepEvent(DepartureList,pEvent->m_pConnection->m_nSequenceNo);
		DeleteEvent(dEvent);
	}*/
	
	if (man->m_eProvisionType == NetMan::PT_BBU)
		updateOldList_Unprotected(pEvent->m_hTime, interval, man);
	else
		updateOldList(pEvent->m_hTime, interval, man);
	
	delete pCurrent;

#ifdef DEBUG
	cout << "\tL'evento selezionato da nextEvent e': ";
	if (pEvent->m_hEvent == 1)
	{
		cout << "ARRIVAL" << endl;
	}
	else
	{
		cout << "DEPARTURE" << endl;
	}
#endif // DEBUGB

	return pEvent;
}


//Cancello da lista degli arrivi gli eventi troppo vecchi
void EventList::updateOldList(SimulationTime now, SimulationTime interval, NetMan* man)
{
#ifdef DEBUGB
	cout << "-> updateOldList" << endl;
#endif // DEBUGB
	//HP:old list è in ordine temporale crescente
	if(oldEventList.empty()) 
		return;
	int i;

	double ni = now-interval;
	if(ni < 0)
		return;

	int indexLimit = -1;
	for(i = 0; i < oldEventList.size(); i++)
	{
	// cout<<"Size:"<<oldEventList.size()<<endl;
	// NOTA : i indica l'elemento SUCCESSIVO a quello contenuto nell' intervallo iniziale
		if((oldEventList[i]->m_hTime) > ni)//cambio in >=//cambio in >
		{
			double t = oldEventList[i]->m_hTime;
			indexLimit=i;
		//	cout<<i<<endl;
			break;
		}
		else
			indexLimit=i+1;
	}
	if (indexLimit<=0)
		return;

	if (indexLimit>1)
		refreshOldList(indexLimit);

	deque<Event*>::iterator itrEV;
	
	i = 0;	//verificare <=

	while(i < indexLimit)
	{
	
		itrEV =	oldEventList.begin();

		int size = oldEventList.size();
		if((*itrEV)->isDepValid == false)
		{
			if ((*itrEV)->m_hEvent == Event::EVT_DEPARTURE)
			{//PULIZIA DELLE VARIABILI
				list<Lightpath*>::iterator LPIter;
				Lightpath_Seg* LPS;
				for (LPIter = (*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.begin(); LPIter != (*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.end(); LPIter++)
				{
					 LPS=(Lightpath_Seg*)(*LPIter);
					//	delete LPS->m_hBackupSegs;
					/*
					list<AbsPath*>::iterator APIter;
					for(APIter=LPS->m_hBackupSegs.begin();APIter!=LPS->m_hBackupSegs.end();APIter++)
					{
					delete (*APIter);
					}*/

					delete (*LPIter);
				}
				(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.clear();

 //===========================================

				for(LPIter=(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.begin();LPIter!=(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.end();LPIter++)
				{
					 LPS=(Lightpath_Seg*)(*LPIter);
					//	delete LPS->m_hBackupSegs
					/*
					list<AbsPath*>::iterator APIter;
					for(APIter=LPS->m_hBackupSegs.begin();APIter!=LPS->m_hBackupSegs.end();APIter++)
					{
					delete (*APIter);
					}*/

					delete (*LPIter);
				}
				(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.clear();

////////////////////
							delete (*itrEV)->m_pConnection;//elimina in automatico anche il circuit
							delete (*itrEV);
			}//FINE PULIZIA VARIABILI
#ifdef DEBUGB
			cout << "\tBEFORE OLDEVENTLIST.POP_FRONT" << endl;
#endif // DEBUGB
			oldEventList.pop_front();//SE ARRIVO-PARTENZA NON VALIDI, MI LIMITO A SCARTARLI
									///SENZA FARE SET-UP / TEAR-DOWN
			i++;
			continue;
		}
		if((*itrEV)->m_hEvent==Event::EVT_ARRIVAL)
			{
#ifdef DEBUGB
				cout << "\tARRIVOOO" << endl;
#endif // DEBUGB
			//DA FARE: SET UP SU RETE B
			//....................
			/////////
		/*		m_pNetMan->m_hWDMNet.dump(cout);
				cout<<"====================================";
				m_pNetMan->m_hWDMNetPast.dump(cout);*/
			/////////////////
			Event* DBGev=(*itrEV);
			(*itrEV)->m_pConnection->m_pPCircuit->m_eState = Circuit::CT_SetupB;
			list<Lightpath*>::iterator itrLP, itrLP2;
			itrLP = (*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.begin();
			itrLP2 = (*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.begin(); //HP:lista di un unico elemento...
			(*itrLP2)->m_nLinkId = (*itrLP)->m_nLinkId;
#ifdef DEBUGB
			cout << "\tBEFORE CIRCUIT SETUP" << endl;
#endif // DEBUGB
			if(wpFlag)
				(*itrEV)->m_pConnection->m_pPCircuit->WPsetUp(man);
			else
				(*itrEV)->m_pConnection->m_pPCircuit->setUp(man);
			////////////////////////////		
	/*		m_pNetMan->m_hWDMNet.dump(cout);
			cout<<"====================================";
			m_pNetMan->m_hWDMNetPast.dump(cout);*/
			////////////////////////////////////
			delete (*itrEV);//FABIO 23 ott: mi evita i memory leaks..?
		
				
			} //FINE EVENTO ARRIVO
			else
			{ //SE EVENTO PARTENZA
#ifdef DEBUGB
				cout << "\tPARTENZAAA" << endl;
#endif // DEBUGB
				//......................................
					/*		m_pNetMan->m_hWDMNet.dump(cout);
				cout<<"====================================";
				m_pNetMan->m_hWDMNetPast.dump(cout);*/

				(*itrEV)->m_pConnection->m_pPCircuit->m_eState=Circuit::CT_TorndownB;
				if(wpFlag)
					(*itrEV)->m_pConnection->m_pPCircuit->WPtearDown(man,false);
				else
					(*itrEV)->m_pConnection->m_pPCircuit->tearDown(man,false);

				/*			m_pNetMan->m_hWDMNet.dump(cout);
				cout<<"====================================";
				m_pNetMan->m_hWDMNetPast.dump(cout);*/
///////////////////////
				list<Lightpath*>::iterator LPIter;
				Lightpath_Seg* LPS;
				for(LPIter=(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.begin();LPIter!=(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.end();LPIter++)
				{
					 LPS=(Lightpath_Seg*)(*LPIter);
					//	delete LPS->m_hBackupSegs

					/*list<AbsPath*>::iterator APIter;
					for(APIter=LPS->m_hBackupSegs.begin();APIter!=LPS->m_hBackupSegs.end();APIter++)
					{
					delete (*APIter); //IL DISTRUTTORE DI LP SEG LO FA GIA IN AUTOMATICO!!!!

					}*/

					 delete (*LPIter);
				}
				(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.clear();

 //===========================================

	

				for(LPIter=(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.begin();LPIter!=(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.end();LPIter++)
				{
					LPS=(Lightpath_Seg*)(*LPIter);
					//	delete LPS->m_hBackupSegs

					/*list<AbsPath*>::iterator APIter;
					for(APIter=LPS->m_hBackupSegs.begin();APIter!=LPS->m_hBackupSegs.end();APIter++)
					{
					delete (*APIter);
					}*/

					delete (*LPIter);
				}
				(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.clear();
				Event* DBGev=oldEventList.front();
////////////////////
				delete (*itrEV)->m_pConnection;//elimina in automatico anche il circuit

				delete (*itrEV);//FABIO 23 ott: mi evita i memory leaks..?
			} // FINE EVENTO PARTENZA						
#ifdef DEBUGB
		cout << "\tBEFORE POP_FRONT FINALE" << endl;
#endif // DEBUGB	
			oldEventList.pop_front();
			i++;					
		}//while
	
		/*
		//FABIO 16 ott: inserisco nella lista di eventi le partenze "fittizie"
		//				( rimozione in simulator::run() )
		//		da fare: dovro sistemare il tempo di simulazione in modo che queste partenze non siano viste
		//				come passate...
		for(itr3=DepartureListB.begin();itr3 !=DepartureListB.end();itr3++){
			if((*itr3)->isDepValid==true)//18 ott
			insertEvent(*itr3);
		}*/

}

// Rimuove temporaneamente le connessioni presenti in arrivalList
void EventList::pushOldEvent(Event* pEvent)
{
	
oldEventList.push_back(pEvent);

}

int EventList::getSize()
{
	return size;
}

UINT NS_OCH::EventList::getNumArr()
{
	return numArr;
}

UINT NS_OCH::EventList::getNumDep()
{
	return numDep;
}

void NS_OCH::EventList::increaseArr()
{
	numArr++;
}

void NS_OCH::EventList::increaseDep()
{
	numDep++;
}

bool NS_OCH::EventList::getwpFlag()
{
	return wpFlag;
}

// marco le coppie partenze-arrivi che non vanno instradate
void EventList::refreshOldList(int limitIndex){
	
	//chiave=seq# ,valore=index
	map<int,int> valori;//map con coppia seq#arrivi,indice corrispondente

	int maxArr=-1;
	int minArr=-1;
	bool first=true;
	int i;
	assert(limitIndex>0);
	for(i=0;i<limitIndex;i++) //PRIMA ERA <=
	{
		Event* pEvent = oldEventList[i];
		if (pEvent->m_hEvent == Event::EVT_ARRIVAL) //HP:gli arrivi sono in ordine crescente..
		{
			if (first)
			{
				minArr = maxArr = pEvent->m_pConnection->m_nSequenceNo;
				first = false;
			}
			else
			{
				maxArr = pEvent->m_pConnection->m_nSequenceNo;
			}
			valori[pEvent->m_pConnection->m_nSequenceNo] = i;
		}

	}//FINE FASE 1: ho i valori  dei seq # degli arrivi

//
	for(i=0;i<limitIndex;i++) //PRIMA ERA <=
	{
		Event* pEvent = oldEventList[i];
		if(pEvent->m_hEvent==Event::EVT_DEPARTURE)

		{
			if((pEvent->m_pConnection->m_nSequenceNo >= minArr) &&
				(pEvent->m_pConnection->m_nSequenceNo <= maxArr))
			{
				pEvent->isDepValid=false;//invalido partenza
				map<int,int>::iterator itr;
				itr=valori.lower_bound(pEvent->m_pConnection->m_nSequenceNo);
				int pr=	itr->second;
				oldEventList[itr->second]->isDepValid=false;
			
			}
		
		}

	}
}

//-B: originally taken from updateOldList (I don't know what it does exactly)
//Cancello da lista degli arrivi gli eventi troppo vecchi
void EventList::updateOldList_Unprotected(SimulationTime now, SimulationTime interval, NetMan* man)
{
#ifdef DEBUGB
	cout << "\t-> updateOldList_Unprotected" << endl;
#endif // DEBUGB

	//HP:old list è in ordine temporale crescente
	if (oldEventList.empty())
		return;
	int i;

	double ni = now - interval;
	if (ni < 0)
		return;

	int indexLimit = -1;
	for (i = 0; i < oldEventList.size(); i++)
	{
		// cout<<"Size:"<<oldEventList.size()<<endl;
		// NOTA : i indica l'elemento SUCCESSIVO a quello contenuto nell' intervallo iniziale
		if ((oldEventList[i]->m_hTime) > ni) //cambio in >=//cambio in >
		{
			double t = oldEventList[i]->m_hTime;
			indexLimit = i;
			//cout<<i<<endl;
			break;
		}
		else
			indexLimit = i + 1;
	}
	if (indexLimit <= 0)
		return;

	if (indexLimit>1)
		refreshOldList(indexLimit);

	deque<Event*>::iterator itrEV;

	i = 0;	//verificare <=

	while (i < indexLimit)
	{

		itrEV = oldEventList.begin();
		int size = oldEventList.size();
		if ((*itrEV)->isDepValid == false)
		{
			if ((*itrEV)->m_hEvent == Event::EVT_DEPARTURE)
			{ //PULIZIA DELLE VARIABILI
				
				(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.clear();

				//===========================================

				(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.clear();

				////////////////////
				delete (*itrEV)->m_pConnection; //elimina in automatico anche il circuit
				delete (*itrEV);
			} //FINE PULIZIA VARIABILI

			oldEventList.pop_front(); //SE ARRIVO-PARTENZA NON VALIDI, MI LIMITO A SCARTARLI
									  //SENZA FARE SET-UP / TEAR-DOWN
			i++;
			continue;
		}

		///////////////////////////////////////////////-B: try to comment this part
		/*
		if ((*itrEV)->m_hEvent == Event::EVT_ARRIVAL)
		{
#ifdef DEBUGB
			cout << "\tARRIVOOO" << endl;
#endif // DEBUGB
			//Event* DBGev = (*itrEV);
			//(*itrEV)->m_pConnection->m_pPCircuit->m_eState = Circuit::CT_SetupB;
			//list<Lightpath*>::iterator itrLP;
			//itrLP = (*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.begin();
#ifdef DEBUGB
			cout << "\tBEFORE CIRCUIT SETUP. WPFLAG = " << wpFlag << endl;
#endif // DEBUGB
			if (wpFlag)
				(*itrEV)->m_pConnection->m_pPCircuit->WPsetUp(man);
			else
				(*itrEV)->m_pConnection->m_pPCircuit->Unprotected_setUpCircuit(man);

			delete (*itrEV); //FABIO 23 ott: mi evita i memory leaks..?

		} //FINE EVENTO ARRIVO
		else
		{ //SE EVENTO PARTENZA
#ifdef DEBUGB
			cout << "\tPARTENZAAA" << endl;
#endif // DEBUGB

			//(*itrEV)->m_pConnection->m_pPCircuit->m_eState = Circuit::CT_TorndownB;
#ifdef DEBUGB
			cout << "\tBEFORE CIRCUIT SETUP. WPFLAG = " << wpFlag << endl;
#endif // DEBUGB
			if (wpFlag)
				(*itrEV)->m_pConnection->m_pPCircuit->WPtearDown(man, false);
			else
				(*itrEV)->m_pConnection->m_pPCircuit->tearDownCir(man, false);
			
			(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteB.clear();

			//===========================================

			(*itrEV)->m_pConnection->m_pPCircuit->m_hRouteA.clear();
			////////////////////
			delete (*itrEV)->m_pConnection; //elimina in automatico anche il circuit

			delete (*itrEV); //FABIO 23 ott: mi evita i memory leaks..?
		} // FINE EVENTO PARTENZA						
		*/
	
		oldEventList.pop_front();
		i++;
	}
}

// -L: cancella il primo elemento della lista
void NS_OCH::EventList::updateFronthaulEventList(Event*pEvent)
{
#ifdef DEBUGC
	cout << "->updateFronthaulEventList" << endl;
#endif // DEBUGB

	//-B: pEvent is a backhaul event, with a valid pointer to its fronthaul event
	EventListItem*pCurrent = m_pHead;
	m_pHead = m_pHead->m_pNext;

#ifdef DEBUGC
	cout << "\tSto per considerare la connessione con arrival time " << pEvent->m_hTime;
	cout << ", legata alla fronthaul connection arrivata a " << pCurrent->m_pEvent->m_hTime << endl;
#endif // DEBUGB

	assert(pEvent->m_hTime == pCurrent->m_pEvent->m_hTime);
	assert(pCurrent->m_pEvent->m_pConnection->m_nSequenceNo == pEvent->fronthaulEvent->m_pConnection->m_nSequenceNo);
	
	delete pCurrent;
	size--;

	return;
}

//-B: we have to delay the corresponding FRONTHAUL departure event of an already existing connection
void EventList::delayFronthaulEvent(Connection*pCon)
{
#ifdef DEBUGB
	cout << "-> delayFronthaulEvent" << endl;
#endif // DEBUGB

	//-B: scan event list
	EventListItem*pCurrent = m_pHead->m_pNext;
	while (pCurrent) //!= NULL
	{
		//-B: if the event is a departure event
		if (pCurrent->m_pEvent->m_hEvent == Event::EVT_DEPARTURE)
		{
			//if it corresponds to a fronthaul connection
			if (pCurrent->m_pEvent->m_pConnection->m_eConnType == Connection::MOBILE_FRONTHAUL
				|| pCurrent->m_pEvent->m_pConnection->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
			{
				//if the fronthaul connection has the same source and dst nodes
				if (pCurrent->m_pEvent->m_pConnection->m_nSrc == pCon->m_nSrc && pCurrent->m_pEvent->m_pConnection->m_nDst == pCon->m_nDst)
				{
					//if its fronthaul bandwidth is > 0 (otherwise it could find another connection, not the first one, with the same src and dst, but with CPRIbwd = 0)
					if (pCurrent->m_pEvent->m_pConnection->m_eCPRIBandwidth > OC0)
					{
#ifdef DEBUGB
						cout << "\tTrovato evento della connessione " << pCurrent->m_pEvent->m_pConnection->m_nSrc << "->"
							<< pCurrent->m_pEvent->m_pConnection->m_nDst << " corrispondente alla connessione attuale " << pCon->m_nSrc << "->" << pCon->m_nDst << endl;
						cout << "\tLa connessione precedente ha FH bwd = " << pCurrent->m_pEvent->m_pConnection->m_eCPRIBandwidth << " - "
							<< "la connessione attuale ha FH bwd = " << pCon->m_eCPRIBandwidth << endl;
						cout << "\tL'evento aveva come tempo di departure " << pCurrent->m_pEvent->m_hTime << endl;
						cout << "\tIl tempo di departure della connessione attuale e' " << pCon->m_dArrivalTime + pCon->m_dHoldingTime << endl;
#endif // DEBUGB						
						//-B: change departure time of the previous event only if the current connection will end after the event's connection
						if (pCurrent->m_pEvent->m_hTime < pCon->m_dArrivalTime + pCon->m_dHoldingTime)
						{
							pCurrent->m_pEvent->m_hTime = pCon->m_dArrivalTime + pCon->m_dHoldingTime; //no routing time is needed
						}
						
#ifdef DEBUGB
						cout << "\t-> ora l'evento ha come tempo di departure " << pCurrent->m_pEvent->m_hTime << endl;

						cout << "\tSto per eliminare il departure event. Event list size: " << this->getSize() << endl;
#endif // DEBUGB


						Event*nEv_DepFront = new Event(pCurrent->m_pEvent->m_hTime,	Event::EVT_DEPARTURE, pCurrent->m_pEvent->m_pConnection);
						this->DeleteEvent(pCurrent->m_pEvent);
#ifdef DEBUGB
						cout << "\tHo eliminato il departure event. Event list size: " << this->getSize() << endl;
#endif // DEBUGB

						this->insertEvent(nEv_DepFront);
#ifdef DEBUGB
						cout << "\tHo reinserito l'evento nella event list. Size: " << this->getSize() << endl;
						//cin.get();
#endif // DEBUGB

						//-B: FUNDAMENTAL!!! Exit from while cycle
						break;
					}
				}
			}
		}
		pCurrent = pCurrent->m_pNext;
	} //end WHILE

	return;
}

void EventList::addBernoulliArrAfterDep(OXCNode*pNode, double m_dArrivalRate)
{
#ifdef DEBUGB
	cout << "-> addBernoulliArrAfterDep" << endl;
#endif // DEBUGB

	//-B: scan event list
	EventListItem*pCurrent = m_pHead->m_pNext;

	while (pCurrent) //!= NULL
	{
		//first check if it is a departure (because if it was an arrival, pCurrent->m_pEvent->m_pConnection would be NULL)
		if (pCurrent->m_pEvent->m_hEvent == Event::EVT_DEPARTURE)
		{
			//if the fronthaul connection of this backhaul connection has the same source
			if (pCurrent->m_pEvent->m_pConnection->m_nSrc == pNode->getId())
			{
				//if it corresponds to a backhaul connection
				if (pCurrent->m_pEvent->m_pConnection->m_eConnType == Connection::MOBILE_FRONTHAUL
					|| pCurrent->m_pEvent->m_pConnection->m_eConnType == Connection::FIXEDMOBILE_FRONTHAUL)
				{
					cout << "Event of type departure with seq number " << pCurrent->m_pEvent->m_pConnection->m_nSequenceNo
						<< endl;

					assert(pNode->getNumberOfSources() - pNode->getNumberOfBusySources() == 0);
					//-B: when the departure event will be executed, pNode->getNumberOfSources() - pNode->getNumberOfBusySources() will be == 1
					double arrRate = m_dArrivalRate * 1;
					double dNextArrival = this->nextBernoulliArrival(arrRate);
					this->insertEvent(new Event(pCurrent->m_pEvent->m_hTime + dNextArrival, Event::EVT_ARRIVAL, NULL, pNode));
#ifdef DEBUGB
					cout << "ATTENTION PLEASE! Aggiunto arrival per il nodo #" << pNode->getId() 
						<< " al tempo " << pCurrent->m_pEvent->m_hTime + dNextArrival<< endl;
					//cin.get();
#endif // DEBUGB
					break;
				}
			}
		}
		pCurrent = pCurrent->m_pNext;
	} //end WHILE
}