#ifndef CONSTDEF_H
#define CONSTDEF_H

#define SIM_DURATION 86400			//-B: simulation duration in seconds (86400 sec = 24 h) (1440 sec = 24 min)
#define N_RUN_CONNECTIONS 10000		//-B: each 10000 connections the network simulator has to compute some stats
#define N_TRANS_CONNECTIONS 5000	//-B: num of connections in transitory phase

#define END_OFFICE_HOURS 5			//-B: "phase number" (could be time (hour) or running phase, it depends on RUNNING_STATS value)
									//	in which office hours end and residential hours start
									//-B: for Ahmed
#define LT_LINKS 960
#define CHANNEL_CAPACITY 1944		//-B: it sets OCLightpath - 10G = OC192; 25G = OC486; 40G = OC768; 100G = OC1944
									// to get the real channel capacity, multiply it by OC1 = 51.48 Mbps
#define FH_BWD_FX OC48				//-L: BW required for a FH connection at split 7
#define BWDGRANULARITY OC29			//-B: bandwidth granularity -> amount of bandwidth requested by each mobile connection
									//-> needed for BBUReadTopoHelper to set channel capacity and to set backhaul bwd of macro 
#define BH_BWD OC6
#define FIXED_TRAFFIC OC192			//-B: bandwidth requested by fixed requests
#define MAXTRAFFIC_SMALLCELL OC96	//-B: max amount of overall traffic generated by a single SMALL cell
#define MAXTRAFFIC_MACROCELL OC288	//-B: max amount of overall traffic generated by a single MACRO cell

#define WPFLAG false				//-B: wavelength continuity (NOT USED)
#define ONLY_ACTIVE_GROOMING false	//-B: if true, grooming can be performed only in active hotel nodes
									// 'active' = hotel nodes currently hosting at least 1 BBU
#define SHORTEST_PATH 0				//-B: algorithm to be used to compute route (NOT USED anymore)
#define DISTANCE_SC_MC 2			//-B: define the distance between a macro cell and a small cell
#define	PROPAGATIONLATENCY 0.000005 //-B: propagation latency per km
#define ELSWITCHLATENCY 0.00002		//-B: electronic switch latency 0.00002
#define LATENCYBUDGET 0.0003		//-B: max fronthaul latency
#define LATENCY_MH 0.001			//-L: max midhaul latency
#define LATENCY_BH 0.04				//-L: max backhaul latency
#define MAXNUMBBU 2000				//-B: max num of active BBU in the same hotel node (to not limit: > num of nodes in the network)
#define MAXTRAFFIC_FORPOOL OCmax	//-B: amount of traffic that a BBU pooling node can process
#define LINKOCCUPANCY_PERC 1		//-B: percentage of link occupancy allowed
#define SMALLCELLS_PER_MC 10		//-B: # of small cells for each macro cells

//-L
#define SMART_PLACEMENT	1			// if 0 it uses BBUPOLICY and CUPOLICY to choose the placement if 1 it will use the smart algorithm

//-L: network status
#define CRITICAL 0					// The blocking probability in the network is really high, I need to save as much BW as possible
#define HIGH 1						// Few bandwidth available in the network
#define MEDIUM 2					
#define LOW 3					
#define EMPTY 4						// All the links have a good amount of free capacity

//-L
#define CENTRALIZE 0
#define DISTRIBUTE 1

#define MIDHAUL true				//-B: if true, we consider a midhaul traffic generation proportional to backhaul traffic generation
#define MIDHAUL_FACTOR 4.751		//-B: value to be multiplied with backhaul bwd to get midhaul bwd

#define ONE_CONN_PER_NODE false		//-B: if true, we consider a on-off model of traffic in each cell
#define BBUPOLICY 0					//-B: 0 --> placeBBUHigh; 1 --> placeBBUClose; 2 --> placeBBU_Metric; 3--> placeBBUHigh: if last link full, add another BBU
#define CUPOLICY 0					//-L: 0: placeCUHigh; 1: placeCUClose; 2: placeCUSmart
#define BBU_CHANGE_INTERVAL 1.2		//-B:  time interval between changes
#define MAXVALUE_LATENCY 1000		//-B: random high value 

#define BBUSTACKING true			//-B: if true, we consider a 1:1 association between RRH and BBU
#define INTRA_BBUPOOLING false		//-B: if true, we consider a 1:N association between RRH and BBU (==> general purpose resources)
									//	NOT IMPLEMENTED!!!
#define INTER_BBUPOOLING false		//-B: if true, we consider a 1:N association between RRH and BBU (==> general purpose resources -> CLOUD)
									//-B: IT HAS SENSE ONLY IN CASE OF MIDHAUL true (WITH FRONTHAUL THERE WILL NEVER BE INTER_BBUPOOLING)
									//-B: implemented only in BBU_newConnection (NOT in BBU_newConnection_Bernoulli)

#define RUNNING_STATS true			//-B: if TRUE, stats are saved in arrays every N_RUN_CONNECTIONS
									//	if FALSE, stats are saved every x time (usually every hour)

namespace NS_OCH {
#include <limits.h>

typedef enum {
	PC_NoProtection = 0,
	PC_DedicatedPath,
	PC_SharedPath,
	PC_SharedSRG,
	PC_DedicatedLink,
	PC_SharedLink,
	PC_SharedSegment
} ProtectionClass;

typedef enum {
	OCLightpath = CHANNEL_CAPACITY,
	OCcpriMC = 582,		// [MC = MACRO CELL]
	OCcpriSC = 242,		// [SC = SMALL CELL]
	OC0	   = 0,			// 0 Mb/s (for instance, for fixed request that has no cpri traffic)
	OC1    = 1,			// 51.84 Mb/s
	OC2    = 2,			// 102.96 Mb/s
	OC3    = 3,			// 155.52 Mb/s
	OC5    = 5,			// 257.4 Mb/s
	OC6	   = 6,			// 311.04 Mb/s
	OC9    = 9,			// 466.56 Mb/s
	OC12   = 12,		// 622.08  Mb/s
	OC13   = 13,		// 669.24 Mb/s
	OC14   = 14,		// 720.72 Mb/s
	OC15   = 15,		// 777.6 Mb/s
	OC20   = 20,		// 1 Gb/s (1029.6 Mb/s)
	OC29   = 29,
	OC36   = 36,		// 1.8 Gb/s
	OC40   = 40,		// 2 Gb/s (2059.2 Mb/s)
	OC42   = 42,		// 2 Gb/s (2162.16 Mb/s)
	OC48   = 48,		// 2.5 Gb/s (2488.32 Mb/s)
	OC96   = 96,		// 5 Gb/s
	OC120  = 120,		// 6 Gb/s (6177.6 Mb/s)
	OC192  = 192,		// 10 Gb/s (9953.28 Mb/s)	//-> first steo fixed
	OC288  = 288,		// 15 Gb/s
	OC384  = 384,		// 20 Gb/s (19906.56 Mb/s)	//-> last step fixed
	OC486  = 486,		// 25 Gb/s (25019.28 Mb/s)
	OC582  = 582,		// 30 Gb/s (30012.84 Mb/s)
	OC768  = 768,		// 40 Gb/s (39813.12 Mb/s)
	OC1944 = 1944,		// 100 Gb/s
	OCmax  = 10000,		// huge value to set MAXTRAFFIC_FORPOOL (in order to not limit hotel node's processing capacity)
} BandwidthGranularity;


const UINT NumberOfBWGranularity = 12;		// OCLightpath is for convenience not counted //-B:not needed in my case
const LINK_COST UNREACHABLE = INT_MAX;		// 2147483647
const LINK_COST LARGE_COST = 1000000;
const LINK_COST SMALL_COST = 0.000001;
const LINK_CAPACITY INFINITE_CAP = INT_MAX;	// 2147483647

const UINT RUpower = 134;
const UINT DUpower = 65.7;
const UINT CUpower = 7;
const UINT FXpower = 18.2;
const UINT F1power = 10;
const UINT BHpower = 1;
const UINT CSIpower = 2100;
const UINT CSIidlePower = 315;

const int MAX_DC_NUMBER = 6;      // massimo numero di DataCenter nella rete
const double EDFAspan=80;         // [km] ogni quanto serve un amplificatore
const double EDFApower=8;         // [Watt] potenza media consumata da un EDFA
const double ShortReachCost=16.25;  // [Watt] Costo delle Short Reach interfaces (10 Gbit/s) in configurazione Opaca
const double TransponderCost=18.25;  // [Watt] Costo del Trasponder WDM (10 Gbit/s) in configurazione Opaca
const double TransponderCost2=34.5;  // [Watt] Costo del Trasponder WDM (10 Gbit/s) in configurazione IPbasic
const double IPCost=145;			// [Watt] Costo del processing elettronico (10 Gbit/s) in configurazione IPbasic
const double DXCcost=17.75;			// [Watt] Costo del DXC  in configurazione IPoSDH
const double OpticalSwitchCost=1.5;   // [Watt] Costo dello Switch ottico per lambda (10 Gbit/s) in configurazione Opaca
const double ComputingCost=100;   // [Watt/s] costo necessario ad elaborare una connessione nel Data Center
const double gCO2perKWh=228;      // grammi di CO2 per kilowattora
}

#endif
