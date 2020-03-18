UINT NetMan::placeBBUHigh(UINT src, vector<OXCNode*>&BBUsList)
{
#ifdef DEBUG
	cout << "-> placeBBUHigh" << endl;
#endif // DEBUGB

	LINK_COST bestCost = UNREACHABLE;
	//LINK_COST cost;
	UINT bestBBU = 0;
	//list<AbstractLink*>path;
	OXCNode*pOXCdst;
	OXCNode*pOXCsrc;
	UINT id;
	LINK_COST pathCost = UNREACHABLE;

	//-B: each time the cycle run calls BBU_newConnection method precomputedPath is cleared (from previous cycle calculation)

	//lookup source vertex
	Vertex *pSrc = m_hGraph.lookUpVertex(src, Vertex::VT_Access_Out, -1);	//-B: ATTENTION!!! VT_Access_Out
	list <AbstractLink*> savedPath;
	bool pathAlreadyFound = false;
	UINT bestBBUFound = 0;

	for (int j = 0; j < BBUsList.size(); j++)
	{
		//-B: we must declare it here (and not before the for cycle)!!!
		//list<AbstractLink*> pPath; --> not NEEDED for this policy

		//-B: reset boolean var because we are considering another node
		m_bHotelNotFoundBecauseOfLatency = false;


		id = BBUsList[j]->getId();

		pOXCdst = (OXCNode*)m_hWDMNet.lookUpNodeById(id);
		pOXCsrc = (OXCNode*)m_hWDMNet.lookUpNodeById(src);
		Vertex* pDst = m_hGraph.lookUpVertex(id, Vertex::VT_Access_In, -1);	//-B: ATTENTION!!! VT_Access_In
		Vertex* pSrc = m_hGraph.lookUpVertex(src, Vertex::VT_Access_Out, -1);

		//-B: STEP 2 - *********** CALCULATE PATH AND RELATED COST ***********
		//-B: calcolo il costo dello shortest path che va dalla source data in input alla funzione
		//	fino al nodo destinazione che cambia ad ogni ciclo

		list<AbsPath*> hPathList;
		m_hGraph.Yen(hPathList, pSrc, pOXCsrc, pDst, 10, this, AbstractGraph::LinkCostFunction::LCF_ByOriginalLinkCost);
		pOXCdst->updateHotelCostMetricForP0(this->m_hWDMNet.getNumberOfNodes());
		pOXCdst->m_dCostMetric += pOXCdst->m_nBBUReachCost;
#ifdef DEBUG
		cout << "List of paths found: " << hPathList.size() << endl;
#endif

		if (hPathList.size() == 0) {
			m_bHotelNotFoundBecauseOfLatency = true;
			groomingConnections.clear();
		}
		else if(hPathList.size() == 1){

				list<AbsPath*>::const_iterator itrPath;

				itrPath = hPathList.begin();
				bool pathAlreadyFound = false;


				list <AbstractLink*> pathComputedByYen = (*itrPath)->m_hLinkList;

				printPath(pathComputedByYen);

				//-B: STEP 2 - *********** CALCULATE PATH AND RELATED COST ***********
				//-B: calcolo il costo dello shortest path che va dalla source data in input alla funzione
				//	fino al nodo destinazione che cambia ad ogni ciclo
				pathCost = (*itrPath)->calculateCost();
				pOXCdst->m_nBBUReachCost = pathCost;

				pOXCdst->updateHotelCostMetricForP0(this->m_hWDMNet.getNumberOfNodes());
				pOXCdst->m_dCostMetric += pOXCdst->m_nBBUReachCost;

				//-B: *********** UPDATE COST METRIC ************ (cost metric is reset in resetPreProcessing method, inside BBU_newConnection)

				///////////////////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////////////    POLICY 0    ///////////////////////////////////////
				////////////////// (CHOOSE CORE CO OR THE "HIGHEST" BBU HOTEL NODE) //////////////////// ("highest" = closest to the core co)
				///////////////////////////////////////////////////////////////////////////////////////////////////////
				if (pathCost < UNREACHABLE)
				{


					if ((pOXCdst->m_dCostMetric < bestCost)) {

						bestCost = pOXCdst->m_dCostMetric;
#ifdef DEBUG

							cout << "New best cost BBU (satisfying constraints) is " << pOXCdst->getId() << endl;
#endif
							pathAlreadyFound = true;
							pOXCdst->pPath = pathComputedByYen;

							//precomputedPath = pOXCdst->pPath; //overwritten at the end of findBestBBUHotel --> SO DON'T DO IT HERE!

							//Reset grooming time related to the old best bbu
							//otherwise, I will update the grooming time to connections not on the chosen path
							list <Connection*>::const_iterator itrG;
							Connection* pConG;
							list<Connection*> connectinGroomingReset;

							for (itrG = groomingConnections.begin(); itrG != groomingConnections.end(); itrG++) {

								pConG = (Connection*)(*itrG);
#ifdef DEBUGC
								cout << "Connection from " << pConG->m_nSrc << " to "
									<< pConG->m_nDst << " has grooming time > 0" << endl;
#endif
								Connection* connFound = checkLinks(pConG, savedPath, pOXCdst->pPath);
								if (connFound != NULL) {

									connectinGroomingReset.push_back(connFound);

								}

							}

							list < Connection*>::const_iterator itrCheckLinks;
							Connection* connectionToDelete;

							for (itrCheckLinks = connectinGroomingReset.begin(); itrCheckLinks != connectinGroomingReset.end(); itrCheckLinks++) {
								connectionToDelete = (Connection*)(*itrCheckLinks);
								groomingConnections.erase(connectionToDelete->m_nSequenceNo);

							}


							savedPath = pOXCdst->pPath;

							bestBBU = pOXCdst->getId();
							if (bestBBU == 46) {
								return bestBBU;
							}

						} else {
							groomingConnections.clear();
						}

					} //end IF unreachable

#ifdef DEBUG
					cout << "\tCosto " << src;
					list<AbstractLink*>::const_iterator itr = pathComputedByYen.begin();
					SimplexLink*sLink;
					while (itr != pathComputedByYen.end())
					{
						sLink = (SimplexLink*)(*itr);
						if (sLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Channel)
							cout << "->" << sLink->m_pUniFiber->getDst()->getId();
						if (sLink->getSimplexLinkType() == SimplexLink::SimplexLinkType::LT_Lightpath)
							cout << "->" << sLink->m_pLightpath->getDst()->getId();
						itr++;
					}
					cout << " = " << pOXCdst->m_nBBUReachCost << endl;
#endif // DEBUGB

			if (!pathAlreadyFound) {
				m_bHotelNotFoundBecauseOfLatency = true;
			}
		}
		else {
			cout << "Why do i arrive here ?" << endl;
			cin.get();
		}

	} //end FOR bbu hotel nodes

	return bestBBU;
}
