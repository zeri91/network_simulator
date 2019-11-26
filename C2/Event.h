#ifndef EVENT_H
#define EVENT_H

#include <list>
namespace NS_OCH {

class Connection;
class Lightpath;
class Event: public OchObject
{
	
	Event(const Event&);
	const Event* operator=(const Event&);
public:

	typedef enum {
		EVT_ARRIVAL = 1,
		EVT_DEPARTURE = 2
	} SIM_EVENTS;

	Event();
	Event(SimulationTime, SIM_EVENTS, Connection*);
	Event(SimulationTime, SIM_EVENTS, Connection*, Event*);
	Event(SimulationTime, SIM_EVENTS, Connection*, Event*, Event*);
	Event(SimulationTime, SIM_EVENTS, Connection*, OXCNode*);
	~Event();
	virtual void dump(ostream&) const;

public:
	SimulationTime		m_hTime;
	SIM_EVENTS			m_hEvent;
	Connection			*m_pConnection;
	OXCNode				*m_pSource;
	list<Lightpath*>	savedLP; //Fabio 16 ott: percorso salvato per 
							// evitare che venga cancellato dopo la partenza
							//18 ott:..al momento risulta non usato..
	
	SimulationTime		arrTimeAs;	//18 ott:Associo alle partenze il tempo di arrivo della
								// connessione, in modo da aggiornare correttamente DepartureListB

	bool				isDepValid;			//18 ott: flag che indica se la partenza presente in departurelistB
								// deve essere reinstradata (dipende dalla posizione temporale

	Event				*midhaulEvent;      // -L  if it is a backhaul event, it will point to the corresponding midhaul event
	Event				*fronthaulEvent;	//-B a backhaul event points to its corresponding fronthaul event
	
	bool				backhaulBlocked;	//-L: ????????
};

};

#endif
