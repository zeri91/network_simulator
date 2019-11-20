#pragma warning(disable:4786)
#include <assert.h>
#include <vector>
#include <set>
#include <list> 
#include <map>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "UniFiber.h"
#include "Channel.h"


#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace NS_OCH;

Channel::Channel(): m_pUniFiber(NULL), m_eUpDownStatus(DOWN),
	m_pLightpath(NULL),m_backup(false),isLPsaved(false),canaliProtetti(0)//-t
{
	m_bSharingGroupCh.empty();//-t
	//LightpathsIDs = vector<int>(OCLightpath / BWDGRANULARITY);
}

Channel::Channel(UniFiber *pUniFiber, UINT nW, UINT nCap, 
				 bool bFree, UpDownStatus eUpDown): m_pUniFiber(pUniFiber),
		m_nW(nW), m_nCapacity(nCap), m_bFree(bFree), m_eUpDownStatus(eUpDown),
		m_pLightpath(NULL),m_backup(false),isLPsaved(false),canaliProtetti(0)//-t
{
	assert(pUniFiber);
    m_bSharingGroupCh.empty();//-t
	linkProtected=vector<bool>(86,false);//LINK VARIABILI
	nodesProtected=vector<bool>(24,false);//NODI VARIABILI
	//LightpathsIDs = vector<int>(OCLightpath / BWDGRANULARITY);
}

Channel::Channel(const Channel& rhs)
{
	*this = rhs;
}

Channel::~Channel()
{
}

void Channel::dump(ostream &out) const
{
	assert(false);
}

const Channel& Channel::operator=(const Channel& rhs)
{
	if (&rhs == this)
		return (*this);
	m_pUniFiber = rhs.m_pUniFiber;
	m_nW = rhs.m_nW;
	m_nCapacity = rhs.m_nCapacity;
	m_eUpDownStatus = rhs.m_eUpDownStatus;
	m_bFree = rhs.m_bFree;
	m_pLightpath = rhs.m_pLightpath;
	LightpathsIDs = rhs.LightpathsIDs;
	// todo: other stuff
	
	return (*this);
}

void Channel::consume(Lightpath* pLightpath)
{
	//assert(m_bFree); Fabio 7 genn :tolto per il caso wp 
	if (pLightpath == NULL)
		canaliProtetti++;

	//assert(pLightpath);	// could be NULL for PAL_SPP
	m_pLightpath = pLightpath;
	m_bFree = false;
}

void Channel::release()
{

	if (m_pLightpath == NULL)
	{
		canaliProtetti--;
		if(canaliProtetti==0)
		{
			m_pLightpath = NULL;
			m_bFree = true;
			m_backup = false; //-t
		}
		return;
	}
	m_pLightpath = NULL;
	m_bFree = true;
    m_backup = false; //-t
	//assert(canaliProtetti==0);
}

//Fabio 6 ott: se c'è un ligthpath salvato, lo ripristina
void Channel:: restoreSavedLightpath ()
{
	if ( isLPsaved ) // In questo caso invece è un flag per dire se c'è
	{				 //il percorso salvato
		m_pLightpath=m_pLightpathSAVED;
		m_pLightpathSAVED=NULL;
		isLPsaved=false;
		m_bFree = false;
		m_backup=false;//

	}
}

//-B: originally taken from consume
void Channel::muxConsume(Lightpath*pLightpath, UINT LpId)
{
#ifdef DEBUGC
	cout << "-> muxConsume" << endl;
#endif // DEBUGB

	UINT i;
	for (i = 0; i < LightpathsIDs.size(); i++)
	{
		//-B: if LpId is already present
		if (LightpathsIDs[i] == LpId)
			return;
	}
	LightpathsIDs.push_back(LpId); //channel's vector with all lightpaths' ids using it
	m_pLightpath = pLightpath; //channel's pointer to lightpath using it

#ifdef DEBUGC
	cout << "\tCanale " << this->m_nW << " punta al Lp " << LpId << endl;
	cout << "\tLIGHTPATHIDs: ";
	for (vector<int>::const_iterator itr = LightpathsIDs.begin(); itr != LightpathsIDs.end(); itr++)
	{
		cout << "\t" << *itr;
	}
	cout << endl;
#endif // DEBUGB
}

void Channel::deleteId(int lpId)
{
#ifdef DEBUGB
	cout << "-> deleteId" << endl;
#endif // DEBUGB

	bool eliminato = false;
	UINT i;
	for (i = 0; i < LightpathsIDs.size(); i++)
	{
		if (LightpathsIDs[i] == lpId)
		{
#ifdef DEBUGB
			cout << "\tSto eliminando l'ID: " << LightpathsIDs[i] << endl;
#endif // DEBUGB
			LightpathsIDs.erase(LightpathsIDs.begin()+i);
			eliminato = true;
			break;
		}
	}

#ifdef DEBUGB
	if (eliminato)
	{
		cout << "\tLightpathsIDs: ";
		if (LightpathsIDs.size() == 0)
			cout << "empty";
		else
		{
			UINT j;
			for (j = 0; j < LightpathsIDs.size(); j++)
			{
				cout << " " << LightpathsIDs[j];
			}
		}
		cout << endl;
	}
#endif // DEBUGB
}