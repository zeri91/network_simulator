UINT NetMan::placeBBUClose(UINT src, vector<OXCNode*>&BBUsList)
{
#ifdef DEBUGB
	cout << "-> placeBBUClose" << endl;
#endif // DEBUGB

	LINK_COST minCost = UNREACHABLE;
	//LINK_COST cost;
	UINT bestBBU = 0;
	//list<AbstractLink*>path;
	OXCNode*pOXCdst;
	int id;
	LINK_COST pathCost;

	//-B: each time the cycle run calls BBU_newConnection method precomputedPath is cleared (from previous cycle calculation)

	//lookup source vertex
	Vertex *pSrc = m_hGraph.lookUpVertex(src, Vertex::VT_Access_Out, -1);	//-B: ATTENTION!!! VT_Access_Out

	for (int j = 0; j < BBUsList.size(); j++)
	{
		//-B: reset boolean var because we are considering another node
		m_bHotelNotFoundBecauseOfLatency = false;

		id = BBUsList[j]->getId();
		pOXCdst = (OXCNode*)m_hWDMNet.lookUpNodeById(id);
		Vertex *pDst = m_hGraph.lookUpVertex(id, Vertex::VT_Access_In, -1);	//-B: ATTENTION!!! VT_Access_In


		//-B: STEP 2 - *********** CALCULATE PATH AND RELATED COST ***********
		//-B: calcolo il costo dello shortest path che va dalla source data in input alla funzione
		//	fino al nodo destinazione che cambia ad ogni ciclo
		pathCost = m_hGraph.DijkstraLatency(pOXCdst->pPath, pSrc, pDst, AbstractGraph::LinkCostFunction::LCF_ByOriginalLinkCost);
		pOXCdst->m_nBBUReachCost = pathCost;

		///////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////      POLICY 1    //////////////////////////////////////
		///////////////////////// (CHOOSE THE NEAREST BBU HOTEL NODE) //////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////
		if (pathCost < UNREACHABLE)
		{
			Connection::ConnectionType connType = Connection::MOBILE_FRONTHAUL;

			float lat = computeLatencyP3(pOXCdst->pPath, pSrc, (UINT)connType);
			if (lat <= LATENCYBUDGET) //-B: -> should be unnecessary using DijkstraLatency -> IT IS ABSOLUTELY NECESSARY! (read at the end of DijkstraHelperLatency)
			{

				//if current selected BBU is better than previous best BBU
				if (pathCost < minCost)
				{
					minCost = pathCost;
					//precomputedPath = pOXCdst->pPath; //then overwritten at the end of findBestBBUHotel --> SO DON'T DO IT!
					bestBBU = id;
				}
			} //end IF latency
			else
			{
#ifdef DEBUGB
				cout << "\tHotel node " << id << " scartato per superamento di latenza max -> " << lat << endl;
				//cin.get();
#endif // DEBUGB
				m_bHotelNotFoundBecauseOfLatency = true;
			}
		} //end IF cost < UNREACHABLE
		//////////////////////////////////////////////////////////////////////////////////////////////////////7

#ifdef DEBUGB
		cout << "\tCosto " << src;
		list<AbstractLink*>::const_iterator itr = pOXCdst->pPath.begin();
		SimplexLink*sLink;
		while (itr != pOXCdst->pPath.end())
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

	} //end FOR BBU hotel nodes

	return bestBBU;
}
