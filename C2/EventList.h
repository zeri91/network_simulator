#ifndef EVENTLIST_H
#define EVENTLIST_H

#include <list>
#include<deque>


namespace NS_OCH {

class Event;
class NetMan;

class EventList: public OchObject
{
    //  class Pred = less<typename list<*Event>::value_type>    ;


       	const EventList* operator=(const EventList&);
		int size;
		int numArr;
		int numDep;
		
public:
	EventList(const EventList&);
	EventList();
	~EventList();
    NetMan* m_pNetMan;
	virtual void dump(ostream&) const;
	void dump2(ostream& ) ;//FABIO
	void insertEvent(Event*);
	Event* nextEvent();
	Event* nextEvent(SimulationTime, NetMan*); //FABIO
	static SimulationTime nextPoissonArrival(double);
	SimulationTime nextBernoulliArrival(double dArrivalRate);
	static SimulationTime expHoldingTime(double);
	static double expDistribution(double);
	//const EventList* operator=(const EventList&);// -M
        void ExtractDepEvents(list<Event*>&);
		
		//FABIO
		void refreshOldList(int);
		void updateOldList_Unprotected(SimulationTime now, SimulationTime interval, NetMan * man);
		void DeleteEvent( Event*);
		void updateOldList(SimulationTime,SimulationTime,NetMan*);
/*		Event* EventList::ExtractDepEvent(list<Event*>& ,int );

		list<Event*> ArrivalList; //Lista con gli arrivi compresi tra il tempo attuale
								// e l'intervallo x l'update dell' info di rete

		list<Event*> DepartureList; //Partenze associate agli arrivi della lista ArrivalList
		list<Event*> DepartureListB; // 15 ott: partenze comprese tra t0 e t0-a; da qui risalgo alle connessioni
			*/						//da re-inserire temporaneamente
	
//void EventList::removeArrivalConnections(NetMan* );

void pushOldEvent(Event*);
int getSize(); //-B
UINT getNumArr(); //-B
UINT getNumDep(); //-B
void increaseArr(); //-B
void increaseDep(); //-B
bool getwpFlag(); //-B

void updateFronthaulEventList(Event * pEvent);

void delayFronthaulEvent(Connection*);

void addBernoulliArrAfterDep(OXCNode * pNode, double);


deque<Event*> oldEventList;
bool wpFlag;
private:


	class EventListItem
	{
	public:
		EventListItem(Event* pEvt, EventListItem *pPrev, EventListItem *pNext)
			: m_pEvent(pEvt), m_pPrev(pPrev), m_pNext(pNext) {}
		~EventListItem() {}
		
		Event *m_pEvent;
		EventListItem *m_pPrev, *m_pNext;
	};

	EventListItem *m_pHead;
	EventListItem *m_pTail;
	
};

};
#endif
