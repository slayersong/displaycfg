#include "Displaycfg.h"


NvAPI_Status DisplayCfg::GetAllDisplayIDs()
{
	NvAPI_Status ret = NvAPI_GPU_GetAllDisplayIds(m_hPhysicalGpu[0], NULL, &m_maxDisplayCount);
	for (int i = 0; i < m_maxDisplayCount; i++)
	{
		m_pDisplayIds[i].version = NV_GPU_DISPLAYIDS_VER;
	}
	
	ret = NvAPI_GPU_GetAllDisplayIds(m_hPhysicalGpu[0], m_pDisplayIds, &m_maxDisplayCount);
	if (ret != NVAPI_OK && m_nDisplayIds)
	{
		cout << "Cannot get All DisplayIDs" << endl;
		return ret;
	}

//	NvAPI_Status ret = NVAPI_OK;
	NvDisplayHandle disp = NULL;
	NV_DISPLAY_PORT_INFO port_info;
	port_info.version = NV_DISPLAY_PORT_INFO_VER2;
	NvU32 outputID;
	NV_GPU_CONNECTOR_INFO connectInfo;
	NV_GPU_CONNECTOR_INFO infs[8];
	for (int i = 0; i < m_maxDisplayCount; i++)
	{
		NvU32 displayID = m_pDisplayIds[i].displayId;
		ret = NvAPI_DISP_GetDisplayHandleFromDisplayId(displayID, &disp);
		if (ret != NVAPI_OK)
		{
			cout << "GetDisplayHandleFromDisplayId Failed, status is " << ret << endl;
			return ret;
		}

		ret = NvAPI_GetAssociatedDisplayOutputId(disp, &outputID);
		if (ret != NVAPI_OK)
		{
			cout << "NvAPI_GetAssociatedDisplayOutputId Failed, status is " << ret << endl;
			return ret;
		}

		ret = NvAPI_GetDisplayPortInfo(disp, outputID, &port_info);
		if (ret != NVAPI_OK)
		{
			cout << "NvAPI_GetDisplayPortInfo Failed, status is " << ret << endl;
			return ret;
		}

		connectInfo.version = NV_GPU_CONNECTOR_INFO_VER1;
		ret = NvAPI_GPU_GetConnectorInfo(m_hPhysicalGpu[0], outputID, &connectInfo);
		if (ret != NVAPI_OK)
		{
			cout << "NvAPI_GPU_GetConnectorInfo Failed, status is " << ret << endl;
			return ret;
		}

		infs[i] = connectInfo;
	}
}

NvAPI_Status DisplayCfg::ForceEdidByPortIndex(int iPortIndex, const NV_EDID srcEdid)
{
	if (m_port_mapinfo.size() > 0)
	{
		map<int, MonitorInfo>::iterator it = m_port_mapinfo.find(iPortIndex);
		if (it == m_port_mapinfo.end())
		{
			cout << "Can't find the port " << it->first << "'s edid " << endl;
			return NVAPI_ERROR;
		}
				
		NvAPI_Status ret = NvAPI_GPU_SetEDID(NULL, it->second.nDisplayID, &it->second.edid);
		if (ret != NVAPI_OK)
		{
			cout << "Cant set the port " << iPortIndex << "status is " << ret << endl;
			return ret;
		}	
	}

	return NVAPI_OK;
}

NvAPI_Status DisplayCfg::Construct_primary(const int portIndex[3], int ConnectStatus)
{
	NvU32 uDisplayIDSrc, uDisplayIDDest;

    if (bNeedConstruct(portIndex,ConnectStatus) == false)
    {
        cout << "No need to construct" << endl;
        return NVAPI_OK;
    }	
	//memset(m_ToSet_pathInfo, 0, 2 * sizeof(NV_DISPLAYCONFIG_PATH_INFO));

	/*****************************************************
	****	1st Allocate resource *****************
	1) version
	2) sourceModeInfo
	3) targetInfoCount and targetInfo
	4) details
 	******************************************************/
	/*	for (NvU32 i = 0; i < 2; i++)
	{
		m_ToSet_pathInfo[i].version = NV_DISPLAYCONFIG_PATH_INFO_VER;
		m_ToSet_pathInfo[i].sourceId = i;
		m_ToSet_pathInfo[i].reserved_sourceId = i;

		m_ToSet_pathInfo[i].sourceModeInfo = 
			(NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)malloc(m_ToSet_pathInfo[i].sourceModeInfoCount * sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));

		if (m_ToSet_pathInfo[i].sourceModeInfo == NULL)
		{
			return NVAPI_OUT_OF_MEMORY;
		}
		memset(m_ToSet_pathInfo[i].sourceModeInfo, 0, sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
		m_ToSet_pathInfo[i].sourceModeInfoCount = 1;

		m_ToSet_pathInfo[i].targetInfoCount = i + 1;

		m_ToSet_pathInfo[i].targetInfo = 
			(NV_DISPLAYCONFIG_PATH_TARGET_INFO*)malloc(m_ToSet_pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
		
		if (m_ToSet_pathInfo[i].targetInfo == NULL )//|| m_ToSet_pathInfo[1].targetInfo == NULL)
		{
			return NVAPI_OUT_OF_MEMORY;
		}

		memset(m_ToSet_pathInfo[i].targetInfo, 0, m_ToSet_pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));

		for (NvU32 j = 0; j < m_ToSet_pathInfo[i].targetInfoCount; j++)
		{
			m_ToSet_pathInfo[i].targetInfo[j].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
			memset(m_ToSet_pathInfo[i].targetInfo[j].details, 0, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
			m_ToSet_pathInfo[i].targetInfo[j].details->version = NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER;
		}
	}*/

	/*****************************************************
	****	2nd Step Fill sourceModeInfo *****************
	******************************************************/
	PathIdx iDestPrimaryIndex = { -1,-1 };
	//int iCurPrimaryIndex = -1;
	PathIdx ICloneSrc, ICloned = { -1,-1 };
	PathIdx ISingleSrc = { -1,-1 };
    map<int, MonitorInfo>::iterator it ;
	
    if(bNeedSwapPrimary)
    {
        it= m_port_mapinfo.find(portIndex[0]);
        if (it != m_port_mapinfo.end())
        {
            iDestPrimaryIndex = it->second.pathIdx;
        }
        else
        {
            it = m_port_mapinfo.find(portIndex[2]);
            if (it != m_port_mapinfo.end())
                iDestPrimaryIndex = it->second.pathIdx;
        }

    }

    it = m_port_mapinfo.find(portIndex[1]);
    if (it != m_port_mapinfo.end())
    {
        ISingleSrc = it->second.pathIdx; // it->second.iPathIdx;
    }

	if (bNeedClone)
	{
        it= m_port_mapinfo.find(portIndex[0]);
        if (it != m_port_mapinfo.end())
        {
            ICloneSrc = it->second.pathIdx;
            iDestPrimaryIndex = ICloneSrc;
        }

		it = m_port_mapinfo.find(portIndex[2]);
        if (it != m_port_mapinfo.end())
		{
            ICloned = m_port_mapinfo[portIndex[2]].pathIdx;
		}

        //NvU32 uDisplayIDSrc, uDisplayIDDest;
        uDisplayIDSrc = m_pathInfo[ICloneSrc.iPathIdx].targetInfo[ICloneSrc.iPathSubIdx].displayId;
	}

	if (bNeedClone)
	{
        m_ToSet_pathInfo[0].sourceModeInfoCount = 1;//2;
	}
	else
		m_ToSet_pathInfo[0].sourceModeInfoCount = 1;
	delete m_ToSet_pathInfo[0].sourceModeInfo;

	m_ToSet_pathInfo[0].sourceModeInfo =
		(NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V2*)malloc(m_ToSet_pathInfo[0].sourceModeInfoCount * sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V2));
	memset(m_ToSet_pathInfo[0].sourceModeInfo, 0, m_ToSet_pathInfo[0].sourceModeInfoCount * sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V2));

    m_ToSet_pathInfo[0].sourceModeInfo[0] = m_pathInfo[iDestPrimaryIndex.iPathIdx].sourceModeInfo[0]; //iDestPrimaryIndex.iPathSubIdx];
	m_ToSet_pathInfo[0].sourceModeInfo[0].bGDIPrimary = true;
    m_ToSet_pathInfo[0].sourceModeInfo[0].position = { 0,0 };
/*
	if (bNeedClone)
	{
        m_ToSet_pathInfo[0].sourceModeInfo[1] = m_pathInfo[ICloned.iPathIdx].sourceModeInfo[0];//ICloned.iPathSubIdx];
		m_ToSet_pathInfo[0].sourceModeInfo->position = { 0,0 };
	}
*/
    if (ISingleSrc.iPathIdx != -1)
    {
        m_ToSet_pathInfo[1].sourceModeInfo = m_pathInfo[ISingleSrc.iPathIdx].sourceModeInfo;
        m_ToSet_pathInfo[1].sourceModeInfo->bGDIPrimary = false;
		m_ToSet_pathInfo[1].sourceModeInfoCount = 1;
		m_ToSet_pathInfo[1].targetInfoCount = 1;
		m_ToSet_pathInfo[1].sourceModeInfo->position = 
			NV_POSITION{ NvS32(m_pathInfo[ISingleSrc.iPathIdx].sourceModeInfo->resolution.width) ,0 };//{ 1920,0 }; 
    }

	
	//Construct clone path m_ToSet_pathInfo[1]
	if (bNeedClone)
	{
		m_ToSet_pathInfo[0].targetInfoCount = 2;
	}
	else
		m_ToSet_pathInfo[0].targetInfoCount = 1;


	delete m_ToSet_pathInfo[0].targetInfo;

	m_ToSet_pathInfo[0].targetInfo =
		(NV_DISPLAYCONFIG_PATH_TARGET_INFO*)malloc(m_ToSet_pathInfo[0].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
	memset(m_ToSet_pathInfo[0].targetInfo, 0, m_ToSet_pathInfo[0].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));

	for (size_t i = 0; i < m_ToSet_pathInfo[0].targetInfoCount; i++)
	{
		m_ToSet_pathInfo[0].targetInfo[i].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
		memset(m_ToSet_pathInfo[0].targetInfo[i].details, 0, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
	}


	// Fill Targetinfo details and displayID
	m_ToSet_pathInfo[0].targetInfo[0].displayId = m_pathInfo[iDestPrimaryIndex.iPathIdx].targetInfo[iDestPrimaryIndex.iPathSubIdx].displayId;
	if (m_pathInfo[iDestPrimaryIndex.iPathIdx].targetInfo[0].details)
	{
		*m_ToSet_pathInfo[0].targetInfo[0].details = *m_pathInfo[iDestPrimaryIndex.iPathIdx].targetInfo[iDestPrimaryIndex.iPathSubIdx].details;
	}


	/*m_ToSet_pathInfo[1].targetInfo[0].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
	m_ToSet_pathInfo[1].targetInfo[1].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
	memset(m_ToSet_pathInfo[1].targetInfo[0].details, 0, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
	memset(m_ToSet_pathInfo[1].targetInfo[1].details, 0, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));*/

    if(ISingleSrc.iPathIdx != -1)
    {
        *m_ToSet_pathInfo[1].targetInfo[0].details = *m_pathInfo[ISingleSrc.iPathIdx].targetInfo[ISingleSrc.iPathSubIdx].details;
        m_ToSet_pathInfo[1].targetInfo[0].displayId = m_pathInfo[ISingleSrc.iPathIdx].targetInfo[ISingleSrc.iPathSubIdx].displayId;
    }

	if (bNeedClone)
	{
		*m_ToSet_pathInfo[0].targetInfo[1].details = *m_pathInfo[ICloned.iPathIdx].targetInfo[ICloned.iPathSubIdx].details;
		m_ToSet_pathInfo[0].targetInfo[1].displayId = m_pathInfo[ICloned.iPathIdx].targetInfo[ICloned.iPathSubIdx].displayId;
	}
    NvAPI_Status ret;
    if(ISingleSrc.iPathIdx != -1)
        ret = NvAPI_DISP_SetDisplayConfig(2, m_ToSet_pathInfo, 0);
    else
        ret = NvAPI_DISP_SetDisplayConfig(1, m_ToSet_pathInfo, 0);

	return ret;
}

bool DisplayCfg::bNeedConstruct(const int* portIndex, int ConnectStatus)
{
	//TODO: we only check the individual portIndex[0], portIndex[1], portIndex[2]
	// instead of the following process....
    map<int, MonitorInfo>::iterator it;
    if (ConnectStatus == DP2CONNECT || ConnectStatus == DP1CONNECT || ConnectStatus == DVICONNECT)
    {
        //Only one monitor connected Do nothing
        return false;
    }
    else if (ConnectStatus == (DP2CONNECT | DP1CONNECT) )
    {
        //Swap the primary if needed
        it = m_port_mapinfo.find(portIndex[0]);
        if(it != m_port_mapinfo.end() && !it->second.bPrimary || it->second.pos.x !=0 )
            bNeedSwapPrimary = true;
    }
    else if(ConnectStatus == (DP1CONNECT | DVICONNECT))
    {
        it = m_port_mapinfo.find(portIndex[2]);
        if(it != m_port_mapinfo.end() && !it->second.bPrimary || it->second.pos.x != 0)
            bNeedSwapPrimary = true;
    }
    else if (ConnectStatus == (DP2CONNECT | DVICONNECT))
    {
		if (m_port_mapinfo.size() == 2)
		{
			// So many hards code...
			if (m_port_mapinfo.find(portIndex[0]) != m_port_mapinfo.end()
				&& m_port_mapinfo.find(portIndex[2])!= m_port_mapinfo.end())
			{
				if (m_port_mapinfo[portIndex[0]].bPrimary && 
					m_port_mapinfo[portIndex[2]].bPrimary)
				{
					bNeedClone = false;
				}
				else
					bNeedClone = true;

				if (m_port_mapinfo[portIndex[0]].pos.x != 0 || m_port_mapinfo[portIndex[2]].pos.x != 0)
					bNeedSwapPrimary = true;
			}
		} 
    }
	else if (ConnectStatus == (DP2CONNECT | DVICONNECT | DP1CONNECT))
	{
		if (m_port_mapinfo.find(portIndex[0]) != m_port_mapinfo.end()
			&& m_port_mapinfo.find(portIndex[2]) != m_port_mapinfo.end())
		{
			if (m_port_mapinfo[portIndex[0]].bPrimary &&
				m_port_mapinfo[portIndex[2]].bPrimary)
				bNeedClone = false;
			else
				bNeedClone = true;

            if (m_port_mapinfo[portIndex[0]].pos.x != 0 || m_port_mapinfo[portIndex[2]].pos.x != 0
                    || m_port_mapinfo[portIndex[2]].pos.x!= 1920)
            {
                // For 3 monitor we must do clone, for easily op
                bNeedClone = true;
                bNeedSwapPrimary = true;
            }
		}
	}

     if (m_pathCount == 3)
    {
        bNeedClone = true;
        //ignore the portindx[2], construct as portindex[0]
        //it = m_port_mapinfo.find(portIndex[0]);
        //if(it != m_port_mapinfo.end() && !it->second.bPrimary)
        bNeedSwapPrimary = true;
    }

    return (bNeedSwapPrimary || bNeedClone);
/*
	if (m_port_mapinfo.size() > 1)
	{
		// portIndex[0] should be the primary display
		map<int, MonitorInfo>::iterator it = m_port_mapinfo.find(portIndex[0]);
		if (it == m_port_mapinfo.end())
		{
			cout << "Port " << portIndex[0] << " disconncected" << endl;
			return false;
		}
		else
		{
			if (it->second.bPrimary == true)
			{
			}
			else
			{
				bNeedSwapPrimary = true;
			}
		}

		if (m_pathCount == 3)
		{
			bNeedClone = true;
		}
	}
    return (bNeedSwapPrimary || bNeedClone);*/
}

NvAPI_Status DisplayCfg::Run(const int* portIndex, int ConnectStatus)
{
	/*
	bool bNeedSwapPrimary = false;
	bool bNeedSwapPrimaryPos = false;
	bool bNeedClone = false;
	//Index in PathInfo
	int iCurPrimaryIndex = -1;
	int iDestPrimaryIndex = -1;
	int iClondeIndx;
	NvAPI_Status ret;

	if (ConnectStatus == DP2CONNECT || ConnectStatus == DP1CONNECT || ConnectStatus == DVICONNECT)
	{
		//Only one monitor connected Do nothing
		return NVAPI_OK;
	}
	else if (ConnectStatus == (DP2CONNECT | DP1CONNECT) || ConnectStatus == (DP2CONNECT | DVICONNECT))
	{
		//Swap the primary if needed
	}

	else if (ConnectStatus == (DP1CONNECT | DVICONNECT))
	{
		// we should avoid this thing happened, 
		// the user should connected DP2(Port2) by default and DVI, not DP1 with DVI
		// Clone or just swap the primary? 
		return NVAPI_ERROR;
		//bNeedClone = true;
	}
	else
	{
		
	}

	if (m_port_mapinfo.size() > 1)
	{
		// portIndex[0] should be the primary display
		map<int, MonitorInfo>::iterator it = m_port_mapinfo.find(portIndex[0]);
		if (it == m_port_mapinfo.end())
		{
			// This is only run the first time, if we have forced the EDID,
			// We dno't need to check if the connetor is connected

			// For the case ConnectStatus == (DP2CONNECT | DVICONNECT), we should avoid such action, customer should connect DP1 and DVI 
			// By default for the first time
			cout << "Port " << portIndex[0] << " disconncected" << endl;
			//return NVAPI_ERROR;
		}
		else
		{ 
			if (it->second.bPrimary == true )
			{
			}
			else
			{
				bNeedSwapPrimary = true;
				iDestPrimaryIndex = it->second.iPathIdx;
				for (it = m_port_mapinfo.begin(); it!=m_port_mapinfo.end(); it++)
				{
					if (it->second.bPrimary == true)// && it->second.iPathIdx != iDestPrimaryIndex)
						iCurPrimaryIndex = it->second.iPathIdx;
				}
			}
		}

		if (m_pathCount == 3)
		{
			bNeedClone = true;
		}

		//Find the DP1 Port pathinfo index;

		it = m_port_mapinfo.find(portIndex[1]);
		if (it == m_port_mapinfo.end())
		{
			cout << "Clone Port " << it->first << " disconncected" << endl;
			return NVAPI_ERROR;
		}
		
		if (bNeedSwapPrimary && iCurPrimaryIndex != -1 && iDestPrimaryIndex != -1)
		{
			m_pathInfo[iCurPrimaryIndex].sourceModeInfo->bGDIPrimary = false;
			m_pathInfo[iDestPrimaryIndex].sourceModeInfo->bGDIPrimary = true;

			//iDestPrimaryIndex portIndex[0] 
			m_pathInfo[iCurPrimaryIndex].sourceModeInfo->position = NV_POSITION{ 0,0 };
			m_pathInfo[iDestPrimaryIndex].sourceModeInfo->position = NV_POSITION{ 1920,0 };
		}
		if (bNeedClone) // m_port_mapinfo.haskey(portIndex[2]) && m_port_mapinfo.haskey[portIndex[1]]
		{
			// for the clone case, only 3 monitor connected, we should do the clone process
			NvU32 DisplayID = 0;
			NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO *details = NULL;
			details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));

			// use the little one of m_pathinfo index 
			iCloneSrcIndex = std::fmin(m_port_mapinfo[portIndex[1]].iPathIdx,
				m_port_mapinfo[portIndex[2]].iPathIdx);
			// Store [2]DVI displayId to clone use
			int Indx = m_port_mapinfo[portIndex[2]].iPathIdx;

			DisplayID = m_pathInfo[Indx].targetInfo[0].displayId;
			if (m_pathInfo[Indx].targetInfo[0].details)
			{
				*details = *m_pathInfo[Indx].targetInfo[0].details;
			}

			// Consctruct the clone info
			m_pathInfo[iCloneSrcIndex].targetInfoCount = 2;
			NV_DISPLAYCONFIG_PATH_TARGET_INFO* primary = (NV_DISPLAYCONFIG_PATH_TARGET_INFO*)malloc(m_pathInfo[iCloneSrcIndex].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
			memset(primary, 0, m_pathInfo[iCloneSrcIndex].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
			primary->displayId = m_pathInfo[iCloneSrcIndex].targetInfo[0].displayId;
			primary->details = m_pathInfo[iCloneSrcIndex].targetInfo[0].details;
			primary++;
			primary->displayId = DisplayID;
			primary->details = details;
			primary--;
			delete m_pathInfo[iCloneSrcIndex].targetInfo;
			m_pathInfo[iCloneSrcIndex].targetInfo = (NV_DISPLAYCONFIG_PATH_TARGET_INFO*)malloc(2 * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
			m_pathInfo[iCloneSrcIndex].targetInfo = primary;

			m_pathInfo[iCloneSrcIndex].sourceModeInfo->position = NV_POSITION{ 1920,0 };

			ret = NvAPI_DISP_SetDisplayConfig(2, m_pathInfo, 0);
		}
	}
		
	*/
	return NVAPI_OK;
}

NvAPI_Status DisplayCfg::GetDisplayID()
{
	NvU32 nIDs;
	NvAPI_Status ret;
	m_DisplayIDs[0].version = NV_GPU_DISPLAYIDS_VER1;
	m_DisplayIDs[1].version = NV_GPU_DISPLAYIDS_VER1;
	m_DisplayIDs[2].version = NV_GPU_DISPLAYIDS_VER1;

	//for (int i = 0; i < 8; i++)
	//{
	//	m_pDisplayIds[i].version = NV_GPU_DISPLAYIDS_VER1;
	//}

	//ret = NvAPI_GPU_GetAllDisplayIds(
	//	m_hPhysicalGpu[0], m_pDisplayIds, &nIDs);

	ret = NvAPI_GPU_GetConnectedDisplayIds(m_hPhysicalGpu[0], NULL, &m_nDisplayIds, 0);
	if (ret != NVAPI_OK && m_nDisplayIds)
	{
		cout << "Cannot get connected DisplayIDs" << endl;
		return ret;
	}

	//m_pDisplayIds = (NV_GPU_DISPLAYIDS*)malloc(m_nDisplayIds * sizeof(NV_GPU_DISPLAYIDS));
	if (m_pDisplayIds)
	{
		memset(m_pDisplayIds, 0, m_nDisplayIds * sizeof(NV_GPU_DISPLAYIDS));
		m_pDisplayIds[0].version = NV_GPU_DISPLAYIDS_VER;
		ret = NvAPI_GPU_GetConnectedDisplayIds(m_hPhysicalGpu[0], m_pDisplayIds, &m_nDisplayIds, 0);
		if (ret != NVAPI_OK && m_nDisplayIds)
		{
			cout << "Cannot get connected DisplayIDs" << endl;
			return ret;
		}
	}
	
	return ret;
}

NvAPI_Status DisplayCfg::Init()
{
	NvAPI_Status ret = NVAPI_OK;

	ret = NvAPI_Initialize();
	if (ret != NVAPI_OK)
	{
		cout << "NvAPI_Initialize() Failed, status is:" << ret << endl;
		return ret;
	}

	for (NvU32 PhysicalGpuIndex = 0; PhysicalGpuIndex < NVAPI_MAX_PHYSICAL_GPUS; PhysicalGpuIndex++)
	{
		m_hPhysicalGpu[PhysicalGpuIndex] = 0;
	}

	ret = NvAPI_EnumPhysicalGPUs(m_hPhysicalGpu, &m_physicalGpuCount);
	if (ret != NVAPI_OK)
	{
		cout << "Cannot enumerate GPUs in the system..." <<endl;
		return ret;
	}
	//ret = GetAllDisplayIDs();
	ret = GetDisplayID();
	if (ret != NVAPI_OK)
	{
		cout << "GetDisplayID Failed" << endl;
		return ret;
	}

	ret = FetchPathInfo();
	if (ret != NVAPI_OK)
	{
		cout << "FetchPathInfo Failed" << endl;
		return ret;
	}
	//Get localIndex info
	ret = GetPortIndex();
	if (ret != NVAPI_OK)
	{
		cout << "GetPortIndex Failed" << endl;
		return ret;
	}

	return ret;
}

void DisplayCfg::ForceEdid()
{
	//cout << "ForceEdid to handle hot plug" << endl;
	
	NvAPI_Status ret;
	if (m_port_mapinfo.find(0) != m_port_mapinfo.end()) // can also check status | K2200_bitmask[it->first]
	{
		ret = ForceEdidByPortIndex(0, m_port_mapinfo[0].edid);
	}
	if (m_port_mapinfo.find(1) != m_port_mapinfo.end())
	{
		ret = ForceEdidByPortIndex(1, m_port_mapinfo[1].edid);
	}

	if (m_port_mapinfo.find(2) != m_port_mapinfo.end())
	{
		ret = ForceEdidByPortIndex(2, m_port_mapinfo[2].edid);
	}
	
	if (ret == NVAPI_OK)
	{
		cout << "Successful Force EDID " << endl;
	}
}

DisplayCfg::~DisplayCfg()
{
	// Destruct all the malloc resource 
	//for()
	//	for()
}

int DisplayCfg::CheckStatus()
{
	int ret = 0;

	map<int, MonitorInfo>::iterator it;
	for (it = m_port_mapinfo.begin(); it != m_port_mapinfo.end(); it++)
	{
		ret |= K2200_bitmask[it->first];
	}

	return ret;
}

NvAPI_Status DisplayCfg::GetPortIndex()
{
	NvAPI_Status ret = NVAPI_OK;
	NvDisplayHandle disp = NULL;
	NV_DISPLAY_PORT_INFO port_info;

	port_info.version = NV_DISPLAY_PORT_INFO_VER2;
	NvU32 outputID;
	NV_GPU_CONNECTOR_INFO connectInfo;

	m_disInfo.empty();
	m_port_mapinfo.empty();

	for (int i = 0; i < m_pathCount; i++)
	{
		for (int j = 0; j < m_pathInfo[i].targetInfoCount; j++)
		{
			NvU32 displayID = m_pathInfo[i].targetInfo[j].displayId;

			ret = NvAPI_SYS_GetGpuAndOutputIdFromDisplayId(displayID, &m_hPhysicalGpu[0], &
				outputID);

			connectInfo.version = NV_GPU_CONNECTOR_INFO_VER1;
			ret = NvAPI_GPU_GetConnectorInfo(m_hPhysicalGpu[0], outputID, &connectInfo);
			if (ret != NVAPI_OK)
			{
				cout << "NvAPI_GPU_GetConnectorInfo Failed, status is " << ret << endl;
				return ret;
			}
			
			//get edid
			NV_EDID edid;
			edid.version = NV_EDID_VER;
			ret = NvAPI_GPU_GetEDID(m_hPhysicalGpu[0], outputID, &edid);
			if (ret != NVAPI_OK)
			{
				cout << "NvAPI_GPU_GetEDID Failed, status is " << ret 
					 <<", Port Index is"<< connectInfo.connector->locationIndex << endl;
				return ret;
			}

			MonitorInfo info;
			//info.iPathIdx = i;
			//info.iPathSubIdx = j;
			info.pathIdx.iPathIdx = i;
			info.pathIdx.iPathSubIdx = j;

            int sourcecount = m_pathInfo[i].sourceModeInfoCount;
			info.nDisplayID = displayID;
            info.pos = m_pathInfo[i].sourceModeInfo[sourcecount-1].position;
			info.connect_data = connectInfo.connector[0]; // TODO: totally 4 connectors?
            info.bPrimary = /*(info.iPathSubIdx == 0 &&*/ m_pathInfo[i].sourceModeInfo[sourcecount-1].bGDIPrimary;

			memset(info.edid.EDID_Data, 0,edid.sizeofEDID);

			info.edid.version = NV_EDID_VER;
			info.edid.edidId = edid.edidId;
			info.edid.sizeofEDID = edid.sizeofEDID;
			info.edid.offset = edid.offset;
			memcpy(info.edid.EDID_Data, edid.EDID_Data, NV_EDID_DATA_SIZE);
/*
			FILE* fp = fopen("test.edid", "w+");
			//File test, just copy is OK
			if (fp)
			{
				fwrite(info.edid,sizeof(char), NV_EDID_DATA_SIZE,fp);
			}
			fclose(fp);


			FILE* fp2 = fopen("test.edid", "rb");
			char buf[256];
			memset(buf, 0, NV_EDID_DATA_SIZE);
			int len = fread(buf, sizeof(char), NV_EDID_DATA_SIZE, fp2);
			if (strcmp(buf,info.edid) != 0)
			{
				cout << "load edid failed" << endl;
			}*/
			m_disInfo.push_back(info);

			m_port_mapinfo.insert(pair<int, MonitorInfo>(info.connect_data.locationIndex, info));
		}
	}

	return ret;
}


NvAPI_Status DisplayCfg::FetchPathInfo()
{
	/*
	Call NvAPI_DISP_GetDisplayConfig 3 times, 
	1st get pathcount number,
	2nd  set version and malloc
	3rd get all info
	*/
	//First call to get pathCount num
	NvAPI_Status ret = NvAPI_DISP_GetDisplayConfig(&m_pathCount, NULL);
	if (ret != NVAPI_OK)    
		return ret;

	memset(m_pathInfo, 0, m_pathCount * sizeof(NV_DISPLAYCONFIG_PATH_INFO));
	memset(m_ToSet_pathInfo, 0, m_pathCount * sizeof(NV_DISPLAYCONFIG_PATH_INFO));

	for (NvU32 i = 0; i < m_pathCount; i++)
	{
		m_pathInfo[i].version = NV_DISPLAYCONFIG_PATH_INFO_VER;
		m_ToSet_pathInfo[i].version = NV_DISPLAYCONFIG_PATH_INFO_VER;
	}
	
	ret = NvAPI_DISP_GetDisplayConfig(&m_pathCount, m_pathInfo);
	if (ret != NVAPI_OK)
		return ret;

	ret = NvAPI_DISP_GetDisplayConfig(&m_pathCount, m_ToSet_pathInfo);
	if (ret != NVAPI_OK)
		return ret;

	for (NvU32 i = 0; i < m_pathCount; i++)
	{
		// Allocate the source mode info
		if (m_pathInfo[i].version == NV_DISPLAYCONFIG_PATH_INFO_VER1 
			|| m_pathInfo[i].version == NV_DISPLAYCONFIG_PATH_INFO_VER2)
		{
			//m_pathInfo[i].sourceModeInfo = new NV_DISPLAYCONFIG_SOURCE_MODE_INFO;
			m_pathInfo[i].sourceModeInfo = (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
			m_ToSet_pathInfo[i].sourceModeInfo = (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
		}
		else
		{
#ifdef NV_DISPLAYCONFIG_PATH_INFO_VER3
			m_pathInfo[i].sourceModeInfo = (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)malloc(m_pathInfo[i].sourceModeInfoCount * sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
			m_ToSet_pathInfo[i].sourceModeInfo = (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)malloc(m_ToSet_pathInfo[i].sourceModeInfoCount * sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
#endif
		}
		if (m_pathInfo[i].sourceModeInfo == NULL || m_ToSet_pathInfo[i].sourceModeInfo == NULL)
		{
			return NVAPI_OUT_OF_MEMORY;
		}

		memset(m_pathInfo[i].sourceModeInfo, 0, sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
		memset(m_ToSet_pathInfo[i].sourceModeInfo, 0, sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));

		// Allocate the target array
		m_pathInfo[i].targetInfo = (NV_DISPLAYCONFIG_PATH_TARGET_INFO*)malloc(m_pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
		m_ToSet_pathInfo[i].targetInfo = (NV_DISPLAYCONFIG_PATH_TARGET_INFO*)malloc(m_ToSet_pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));

		if (m_pathInfo[i].targetInfo == NULL || m_ToSet_pathInfo[i].targetInfo == NULL)
		{
			return NVAPI_OUT_OF_MEMORY;
		}
		// Allocate the target details
		memset(m_pathInfo[i].targetInfo, 0, m_pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
		memset(m_ToSet_pathInfo[i].targetInfo, 0, m_ToSet_pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));

		for (NvU32 j = 0; j < m_pathInfo[i].targetInfoCount; j++)
		{
			m_pathInfo[i].targetInfo[j].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
			memset(m_pathInfo[i].targetInfo[j].details, 0, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
			m_pathInfo[i].targetInfo[j].details->version = NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER;
		}

		for (NvU32 j = 0; j < m_ToSet_pathInfo[i].targetInfoCount; j++)
		{
			m_ToSet_pathInfo[i].targetInfo[j].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
			memset(m_ToSet_pathInfo[i].targetInfo[j].details, 0, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
			m_ToSet_pathInfo[i].targetInfo[j].details->version = NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER;
		}
	}

	
	ret = NvAPI_DISP_GetDisplayConfig(&m_pathCount, m_pathInfo);
	if (ret != NVAPI_OK)
		return ret;

	ret = NvAPI_DISP_GetDisplayConfig(&m_pathCount, m_ToSet_pathInfo);
	if (ret != NVAPI_OK)
		return ret;

	return NVAPI_OK;
}

int DisplayCfg::GetPrimaryInfoIndex() { return 1; }
bool DisplayCfg::SwapPrimary(int srcLoc, int destLoc) { return true; }
bool DisplayCfg::CloneExtendDisplay(int srcLoc, int destLoc) { return true; }


void DisplayCfg::ShowCurrentDisplayConfig() 
{
	if (m_pathCount == 1)
	{
		if (m_pathInfo[0].targetInfoCount == 1) // if pathCount = 1 and targetInfoCount =1 it is Single Mode
			cout << "Single MODE";
		else if (m_pathInfo[0].targetInfoCount > 1) // if pathCount >= 1 and targetInfoCount >1 it is Clone Mode
			cout << "Monitors in Clone MODE" <<endl;
	}
	else
	{
		for (NvU32 PathIndex = 0; PathIndex < m_pathCount; PathIndex++)
		{
			if (m_pathInfo[PathIndex].targetInfoCount == 1)
			{

				printf("Monitor with Display Id 0x%x is in Extended MODE\n", m_pathInfo[PathIndex].targetInfo->displayId);
				// if pathCount > 1 and targetInfoCount =1 it is Extended Mode
			}
			else if (m_pathInfo[PathIndex].targetInfoCount > 1)
			{
				for (NvU32 TargetIndex = 0; TargetIndex < m_pathInfo[PathIndex].targetInfoCount; TargetIndex++)
				{
					cout << "Monitors with Display Id" 
						<< m_pathInfo[PathIndex].targetInfo[TargetIndex].displayId <<"are in Clone MODE" << endl;
				}
			}
		}
	}
}
