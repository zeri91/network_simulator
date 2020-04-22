#pragma warning (disable: 4786)
#pragma warning(disable: 4996)
#include <fstream>
#include <assert.h>

#include "OchInc.h"
#include "OchObject.h"
#include "MappedLinkList.h"
#include "AbsPath.h"
#include "AbstractGraph.h"
#include "OXCNode.h"
#include "UniFiber.h"
#include "WDMNetwork.h"
#include "TopoReader.h"
#include "math.h"
#include "ConstDef.h"
#include "OchMemDebug.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



using namespace NS_OCH;

TopoReader::TopoReader()
{
}

TopoReader::~TopoReader()
{
}

bool TopoReader::readTopo(WDMNetwork &hNetwork, const char *pTopoFile)
{
	if (NULL == pTopoFile)
		return false;

	bool bSuccess = true;
	// NB: need to check the existence of the input file
	// ios::nocreate is not declared in <fstream>. 
	// Use 0x20, which is declared in <fstream.h>
	// But 0x20 does not work under solaris
	//ifstream fin(pTopoFile, ios::in | 0x20);
	ifstream fin(pTopoFile, ios::in);
	if (fin.fail()) {
		TRACE1("- Error: can't open topo file %s.\n", pTopoFile);
		return false;
	}

	bSuccess = readTopoHelper(hNetwork, fin);
	
	fin.close();
	return bSuccess;
}

bool TopoReader::readTopoHelper(WDMNetwork &hNetwork, ifstream &fin)
//Salvataggio di tutti i parametri dei nodi e della fibre presi dal file di topologia passato in input
{
	const int MAX_LINE_LENGTH = 1024;
	const char *pWavelengthCount = "NumberOfWavelengths = ";
	const char *pChannelCapacity = "ChannelCapacity = ";
	const char *pTxCount = "NumberOfTx = ";
	const char *pRxCount = "NumberOfRx = ";
	const char *pDCCount = "NumberOfDataCenter = ";
	const char *pTxScale = "TxScale = ";
	const char *pOXCNodesCount = "NumberOfOXCNodes = ";
	const char *pOXCNodesHead = "OXCNodes";
	const char *pOXCNodeFormat = "%u, %u, %u, %u, %u, %u, %u";
	const char *pUniFiberCount = "NumberOfUniFibers = ";
	const char *pUniFiberHead = "UniFibers";
	const char cStartDelimiter = '<';
	const char cEndDelimiter = '>';
	const char *pUniFiberFormat = "%u, %u, %u, %u, %lf, %f";
#define _CRT_SECURE_NO_DEPRECATE //
	UINT nWavelengths, nChannelCapacity, nTx, nRx;
	int nDC;
	int i = 0, j = 0, jj = 0; //contatore per DCvett (vettore con NodeID dei Data Center)
	double dTxScale;
	UINT nOXCNodes = 0;
	UINT nUniFibers = 0;
	char pBuf[MAX_LINE_LENGTH + 1];

	// read # of wavelength, if every fiber has the same # of w	avelengths
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pWavelengthCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfWavelengths = %d", &nWavelengths);
	assert(fin);
	hNetwork.numberOfChannels = nWavelengths;
	
	// read capacity of a channel (wavelength)
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pChannelCapacity)) {
		NULL;
	}
	sscanf_s(pBuf, "ChannelCapacity = %d", &nChannelCapacity);
	assert(fin);
	hNetwork.setChannelCapacity(nChannelCapacity*BWDGRANULARITY); //-L: ???

	// read # of transmitters, if every node has the same # of Tx
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pTxCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfTx = %d", &nTx);
	assert(fin && (nTx >= 0));

	// read # of receivers, if every node has the same # of Rx
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pRxCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfRx = %d", &nRx);
	assert(fin && (nRx >= 0));

    // read Number of Data Center
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pDCCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfDataCenter = %d", &nDC);
	hNetwork.nDC = nDC;
	assert(fin && (nDC > 0));

	int *DCvettALL=new int[nDC];
	int *TZvett=new int[nDC];
	int *DCvettW=new int[nDC];
	int *DCvettS=new int[nDC];
	int *DCvettH=new int[nDC];
	//int *DCCount=new int[nDC];
	
	// read TxScale, if every node has # of Tx/Rx propotinal to its nodal deg
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pTxScale)) {
		NULL;
	}
	sscanf_s(pBuf, "TxScale = %lf", &dTxScale);
	assert(fin && (dTxScale >= 0) && (dTxScale <= 1));

	// read # of OXCs
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pOXCNodesCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfOXCNodes = %d", &nOXCNodes);
	assert(fin && (nOXCNodes > 0));
	hNetwork.numberOfNodes = nOXCNodes;

	// read # of unidirectional fibers
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pUniFiberCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfUniFibers = %d", &nUniFibers);
	assert(fin && (nUniFibers > 0));
	hNetwork.numberOfLinks=nUniFibers;

	// read the set of OXC nodes
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pOXCNodesHead)) {
		NULL;
	}
	assert(fin);
	
	UINT nCount = 0;
	int nNodeId, nConversion, nGrooming, nTxAtNode, nRxAtNode, nDCFlag, nTimeZone;
	
	//acquisizione dati OXCNodes
	while ((nCount < nOXCNodes) && fin.getline(pBuf, MAX_LINE_LENGTH, cStartDelimiter))
	{
		fin.getline(pBuf, MAX_LINE_LENGTH, cEndDelimiter);
		sscanf(pBuf, pOXCNodeFormat, &nNodeId, &nConversion, &nGrooming, &nTxAtNode, &nRxAtNode, &nDCFlag, &nTimeZone);
		if (nTx > 0)
			nTxAtNode = nTx;
		if (nRx > 0)
			nRxAtNode = nRx;
		hNetwork.addNode(new OXCNode(nNodeId,
			(OXCNode::WConversionCapability)nConversion,
			(OXCNode::GroomingCapability)nGrooming, nTxAtNode, nRxAtNode, nWavelengths, nDCFlag, nTimeZone));
		nCount++;
	// salvataggio del Dummy Node (flag -1 nel file di topologia)	
          if (nDCFlag <0)
		  {
			  if (jj>0){
				cout<<"Error - more than one DummyNode";
				cin.get();
			  }
	    	int	DummyNode=nNodeId;
			hNetwork.DummyNode = DummyNode;
			hNetwork.DummyNodeMid = 38; //-L
			jj++;

         }

	// riempimento vettore con ID dei Data Center
        if (nDCFlag >0) {
			DCvettALL[j]=nNodeId;
			hNetwork.DCvettALL[j]=DCvettALL[j];
			TZvett[j]=nTimeZone;
			hNetwork.TZvett[j]=TZvett[j];
			j++;
         }

		if (nDCFlag ==1) {
			DCvettW[i]=nNodeId;
			DCvettS[i]=99;           // 99 sta per "DC non presente"
			DCvettH[i]=99;
			hNetwork.DCvettW[i]=DCvettW[i];
			hNetwork.DCvettS[i]=DCvettS[i];
		    hNetwork.DCvettH[i]=DCvettH[i];
		  i++;  
		  //cout<<"Data Center [Wind Powered] in node "<<nNodeId<<"\n";
		}

        if (nDCFlag ==2) {
			DCvettW[i]=99;
			DCvettS[i]=nNodeId;
			DCvettH[i]=99;
			hNetwork.DCvettW[i]=DCvettW[i];
			hNetwork.DCvettS[i]=DCvettS[i];
			hNetwork.DCvettH[i]=DCvettH[i];
			i++;
		//cout<<"Data Center [Solar Powered] in node "<<nNodeId<<"\n";
		}
	
		if (nDCFlag ==3) {
			DCvettW[i]=99;
			DCvettS[i]=99;
			DCvettH[i]=nNodeId;
			hNetwork.DCvettW[i]=DCvettW[i];
			hNetwork.DCvettS[i]=DCvettS[i];
			hNetwork.DCvettH[i]=DCvettH[i];
			i++;
	       //cout<<"Data Center [Hydro Powered] in node "<<nNodeId<<"\n";
		}
	}
	//cin.get();


	// read the set of unidirectional fibers
	while (fin.getline(pBuf, MAX_LINE_LENGTH) 
		&& !strstr(pBuf, pUniFiberHead)) {
		NULL;
	}
	assert(fin);

	nCount = 0;
	int nFiberId, nSrc, nDst, nW;
	float nLength;
	//double hCost; //-B: commented
	double hCostOld;

	//acquisizione dati UniFibers
	while ((nCount < nUniFibers) 
		&& fin.getline(pBuf, MAX_LINE_LENGTH, cStartDelimiter)) {
		fin.getline(pBuf, MAX_LINE_LENGTH, cEndDelimiter);
		sscanf(pBuf, pUniFiberFormat, &nFiberId, &nSrc, &nDst, &nW, &hCostOld, &nLength);
		//-B: comment this kind of binary cost assignment so that it will save the cost given in the input topology file
		/*if (nSrc == hNetwork.DummyNode || nDst == hNetwork.DummyNode){    // se è Dummy Node
			hCost=0;           // tutti i link tra Dummy Node e i DC
		}
		else {
			     hCost=1;  //CASO A no r-a/no-c
				//hCost=9-nWavelengths;  //CASO C no r-a/c
				//hCost=ceil(nLength/EDFAspan)*EDFApower; //Calcolo potenza necessaria per EDFA
				//cout<<"Costo Link "<<nFiberId<<": "<<hCost;
				//cin.get();
		}*/
		// for convenience in input: each fiber has the same # of wavelengths
		if (nWavelengths > 0)	
			nW = nWavelengths;	
		hNetwork.addUniFiber(nFiberId, nSrc, nDst, nW, (LINK_COST)hCostOld, nLength);
		nCount++;
	}

	hNetwork.scaleTxRxWRTNodalDeg(dTxScale);
	
	return true;
}

bool TopoReader::BBUReadTopo(WDMNetwork &hNetwork, const char *pTopoFile)
{
	if (NULL == pTopoFile)
		return false;

	bool bSuccess = true;
	// NB: need to check the existence of the input file
	// ios::nocreate is not declared in <fstream>. 
	// Use 0x20, which is declared in <fstream.h>
	// But 0x20 does not work under solaris
	//ifstream fin(pTopoFile, ios::in | 0x20);
	ifstream fin(pTopoFile, ios::in);
	if (fin.fail()) {
		TRACE1("- Error: can't open topo file %s.\n", pTopoFile);
		return false;
	}

	bSuccess = BBUReadTopoHelper(hNetwork, fin);

	fin.close();
	return bSuccess;
}

bool TopoReader::BBUReadTopoHelper(WDMNetwork &hNetwork, ifstream &fin)
//Salvataggio di tutti i parametri dei nodi e della fibre presi dal file di topologia passato in input
{
#ifdef DEBUGB
	cout << "\t-> BBUReadTopoHelper" << endl;
#endif // DEBUGB
	const int MAX_LINE_LENGTH = 1024;
	const char *pWavelengthCount = "NumberOfWavelengths = ";
	const char *pChannelCapacity = "ChannelCapacity = ";
	const char *pTreesCount = "Num of trees = ";
	const char *pRingsCount = "Num of rings = ";
	const char *pTxCount = "NumberOfTx = ";
	const char *pRxCount = "NumberOfRx = ";
	const char *pDCCount = "NumberOfDataCenter = ";
	const char *pTxScale = "TxScale = ";
	const char *pOXCNodesCount = "NumberOfOXCNodes = ";
	const char *pOXCNodesHead = "OXCNodes";
	const char *pOXCNodeFormat = "%u, %u, %u, %u, %u, %u, %u, %u, %u";
	const char *pUniFiberCount = "NumberOfUniFibers = ";
	const char *pUniFiberHead = "UniFibers";
	const char cStartDelimiter = '<';
	const char cEndDelimiter = '>';
	const char *pUniFiberFormat = "%u, %u, %u, %u, %lf, %f";
#define _CRT_SECURE_NO_DEPRECATE //
	UINT nWavelengths, nChannelCapacity, nTx, nRx, nTrees, nRings;
	int nDC;
	int a = 0, b = 0, c = 0, j = 0, jj = 0; //counter for nodes' vector
	double dTxScale;
	UINT nOXCNodes = 0;
	UINT nUniFibers = 0;
	char pBuf[MAX_LINE_LENGTH + 1];
	// read # of wavelength, if every fiber has the same # of w	avelengths
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pWavelengthCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfWavelengths = %d", &nWavelengths);
	assert(fin);
	hNetwork.numberOfChannels = nWavelengths;

	// read capacity of a channel (wavelength)
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pChannelCapacity)) {
		NULL;
	}
	sscanf_s(pBuf, "ChannelCapacity = %d", &nChannelCapacity);
	assert(fin);
	if (OCLightpath < 0)
		hNetwork.setChannelCapacity(nChannelCapacity*BWDGRANULARITY);
	else
		hNetwork.setChannelCapacity(OCLightpath);

	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pTreesCount)) {
		NULL;
	}
	sscanf_s(pBuf, "Num of trees = %d", &nTrees);
	assert(fin);
	hNetwork.m_nNumOfTrees = nTrees;

	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pRingsCount)) {
		NULL;
	}
	sscanf_s(pBuf, "Num of rings = %d", &nRings);
	assert(fin);
	hNetwork.m_nNumOfRings = nRings;

	// read # of transmitters, if every node has the same # of Tx
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pTxCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfTx = %d", &nTx);
	assert(fin && (nTx >= 0));

	// read # of receivers, if every node has the same # of Rx
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pRxCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfRx = %d", &nRx);
	assert(fin && (nRx >= 0));

	// read Number of Data Center
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pDCCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfDataCenter = %d", &nDC);
	hNetwork.nDC = nDC;
	//-B: commented
	//assert(fin && (nDC > 0));

	/* -B: commented since not used
	int *DCvettALL = new int[nDC];
	int *TZvett = new int[nDC];
	int *DCvettW = new int[nDC];
	int *DCvettS = new int[nDC];
	int *DCvettH = new int[nDC];
	//int *DCCount=new int[nDC];
	*/

	// read TxScale, if every node has # of Tx/Rx propotinal to its nodal deg
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pTxScale)) {
		NULL;
	}
	sscanf_s(pBuf, "TxScale = %lf", &dTxScale);
	assert(fin && (dTxScale >= 0) && (dTxScale <= 1));

	// read # of OXCs
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pOXCNodesCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfOXCNodes = %d", &nOXCNodes);
	assert(fin && (nOXCNodes > 0));
	hNetwork.numberOfNodes = nOXCNodes;

	//-B: gli array sono stati dichiarati in WDMNetork ma non ancora inizializzati
	hNetwork.setVectNodes(nOXCNodes);
	
	// read # of unidirectional fibers
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pUniFiberCount)) {
		NULL;
	}
	sscanf_s(pBuf, "NumberOfUniFibers = %d", &nUniFibers);
	assert(fin && (nUniFibers > 0));
	hNetwork.numberOfLinks = nUniFibers;

	// read the set of OXC nodes
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pOXCNodesHead)) {
		NULL;
	}
	assert(fin);

	UINT nCount = 0;
	int nNodeId, nConversion, nGrooming, nTxAtNode, nRxAtNode, nFlag, bbuFlag, cellFlag, nArea;

	//acquisizione dati OXCNodes
	while ((nCount < nOXCNodes) && fin.getline(pBuf, MAX_LINE_LENGTH, cStartDelimiter))
	{
		fin.getline(pBuf, MAX_LINE_LENGTH, cEndDelimiter);
		sscanf(pBuf, pOXCNodeFormat, &nNodeId, &nConversion, &nGrooming, &nTxAtNode, &nRxAtNode, &nFlag, &bbuFlag, &cellFlag, &nArea);
		if (nTx > 0)
			nTxAtNode = nTx;
		if (nRx > 0)
			nRxAtNode = nRx;
		// salvataggio del Core CO (flag -1 nel file di topologia)	
		if (nFlag < 0) {
			if (jj > 0) {
				cout << "Error - more than one Core CO";
				cin.get();
			}
			int	DummyNode = nNodeId;
			hNetwork.DummyNode = DummyNode;
			hNetwork.DummyNodeMid = 38; //-L
			bbuFlag = 1; //-B: the core CO is always a BBU_Hotel
			jj++;
		}
		hNetwork.addNode(new OXCNode(nNodeId,
				(OXCNode::WConversionCapability)nConversion,
				(OXCNode::GroomingCapability)nGrooming, nTxAtNode, nRxAtNode, nWavelengths, nFlag, bbuFlag, cellFlag, (OXCNode::UrbanArea)nArea));
		nCount++;
		
		/* -B: commented, since not used
		// riempimento vettore con ID dei Data Center
		if (nDCFlag >0) {
			DCvettALL[j] = nNodeId;
			hNetwork.DCvettALL[j] = DCvettALL[j];
			TZvett[j] = nTimeZone;
			hNetwork.TZvett[j] = TZvett[j];
			j++;
		}
		*/

		if (nFlag == 1) { //-B: MOBILE node
			hNetwork.MobileNodes[a] = nNodeId;
			a++;
			/*
			DCvettW[i] = nNodeId;
			DCvettS[i] = 99;           // 99 sta per "DC non presente"
			DCvettH[i] = 99;
			hNetwork.DCvettW[i] = DCvettW[i];
			hNetwork.DCvettS[i] = DCvettS[i];
			hNetwork.DCvettH[i] = DCvettH[i];
			i++;
			//cout<<"Data Center [Wind Powered] in node "<<nNodeId<<"\n";
			*/
		}

		if (nFlag == 2) { //-B: FIXED node
			hNetwork.FixedNodes[b] = nNodeId;
			b++;
			/*
			DCvettW[i] = 99;
			DCvettS[i] = nNodeId;
			DCvettH[i] = 99;
			hNetwork.DCvettW[i] = DCvettW[i];
			hNetwork.DCvettS[i] = DCvettS[i];
			hNetwork.DCvettH[i] = DCvettH[i];
			i++;
			//cout<<"Data Center [Solar Powered] in node "<<nNodeId<<"\n";
			*/
		}

		if (nFlag == 3) { //-B: FIXED-MOBILE node
			hNetwork.FixMobNodes[c] = nNodeId;
			c++;
			/*
			DCvettW[i] = 99;
			DCvettS[i] = 99;
			DCvettH[i] = nNodeId;
			hNetwork.DCvettW[i] = DCvettW[i];
			hNetwork.DCvettS[i] = DCvettS[i];
			hNetwork.DCvettH[i] = DCvettH[i];
			i++;
			//cout<<"Data Center [Hydro Powered] in node "<<nNodeId<<"\n";
			*/
		}
	}

	hNetwork.genAreasOfNodes();

	//-B: print mobile nodes' vector
	cout << "\tMobile nodes IDs:";
	int i;
	for (i = 0; hNetwork.MobileNodes[i] <= nOXCNodes && hNetwork.MobileNodes[i] > 0; i++)
	{
		cout << " " << hNetwork.MobileNodes[i];
	}
	hNetwork.numMobileNodes = i;
	cout << "\t# of mobile nodes: " << hNetwork.numMobileNodes << endl;
	//-B: print fixed nodes' vector
	cout << "\tFixed nodes IDs:";
	for (i = 0; hNetwork.FixedNodes[i] <= nOXCNodes; i++)
	{
		cout << " " << hNetwork.FixedNodes[i];
	}
	hNetwork.numFixedNodes = i;
	cout << "\t# of fixed nodes: " << hNetwork.numFixedNodes << endl;
	//-B: print fixed-mobile nodes' vector
	cout << "\tFixed-mobile nodes IDs:";
	for (i = 0; hNetwork.FixMobNodes[i] <= nOXCNodes; i++)
	{
		cout << " " << hNetwork.FixMobNodes[i];
	}
	hNetwork.numFixMobNodes = i;
	cout << "\t# of fixed-mobile nodes: " << hNetwork.numFixMobNodes << endl;


	// read the set of unidirectional fibers
	while (fin.getline(pBuf, MAX_LINE_LENGTH)
		&& !strstr(pBuf, pUniFiberHead)) {
		NULL;
	}
	assert(fin);

	nCount = 0;
	int nFiberId, nSrc, nDst, nW;
	float nLength;
	//double hCost; //-B: commented
	double hCostOld;

	//acquisizione dati UniFibers
	while ((nCount < nUniFibers)
		&& fin.getline(pBuf, MAX_LINE_LENGTH, cStartDelimiter)) {
		fin.getline(pBuf, MAX_LINE_LENGTH, cEndDelimiter);
		sscanf(pBuf, pUniFiberFormat, &nFiberId, &nSrc, &nDst, &nW, &hCostOld, &nLength);
		//-B: comment this kind of binary cost assignment so that it will save the cost given in the input topology file
		/*if (nSrc == hNetwork.DummyNode || nDst == hNetwork.DummyNode){    // se è Dummy Node
		hCost=0;           // tutti i link tra Dummy Node e i DC
		}
		else {
		hCost=1;  //CASO A no r-a/no-c
		//hCost=9-nWavelengths;  //CASO C no r-a/c
		//hCost=ceil(nLength/EDFAspan)*EDFApower; //Calcolo potenza necessaria per EDFA
		//cout<<"Costo Link "<<nFiberId<<": "<<hCost;
		//cin.get();
		}*/
		// for convenience in input: each fiber has the same # of wavelengths
		if (nWavelengths > 0)
			nW = nWavelengths;
		hNetwork.addUniFiber(nFiberId, nSrc, nDst, nW, (LINK_COST)hCostOld, nLength);
		nCount++;
	}

	hNetwork.scaleTxRxWRTNodalDeg(dTxScale);
	return true;
}

