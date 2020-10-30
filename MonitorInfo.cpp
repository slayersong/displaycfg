#include "MonitorInfo.h"



NvAPI_Status DisplayCfg::GetDisplayID()
{
	NvAPI_Status ret = NvAPI_GPU_GetConnectedDisplayIds(m_hPhysicalGpu[0], m_pDisplayIds, &m_nDisplayIds, 0);
	if (ret != NVAPI_OK && m_nDisplayIds)
	{
		cout << "Cannot get connected DisplayIDs" << endl;
		return ret;
	}

	m_pDisplayIds = (NV_GPU_DISPLAYIDS*)malloc(m_nDisplayIds * sizeof(NV_GPU_DISPLAYIDS));
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

eDisplayState DisplayCfg::CheckStatus()
{
	int totalDisplay = 0;
	for(int i = 0; i < m_pathCount; i++)
	{
		totalDisplay += m_pathInfo[i].targetInfoCount;
	}
	if (m_port_mapinfo.size() == 3)
	{
		// All connected

	}
	return DP2Ext_DVICopy;
	//The right state should be (1,2 extend), 0 for clone mode;
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

			NV_EDID edid;
			edid.version = NV_EDID_VER;
			ret = NvAPI_GPU_GetEDID(m_hPhysicalGpu[0], outputID, &edid);

			connectInfo.version = NV_GPU_CONNECTOR_INFO_VER1;
			ret = NvAPI_GPU_GetConnectorInfo(m_hPhysicalGpu[0], outputID, &connectInfo);
			if (ret != NVAPI_OK)
			{
				cout << "NvAPI_GPU_GetConnectorInfo Failed, status is " << ret << endl;
				return ret;
			}
			
			MonitorInfo info;
			info.iPathIdx = i;
			info.iPathSubIdx = j;
			info.nDisplayID = displayID;
			info.pos = m_pathInfo[i].sourceModeInfo[j].position;
			info.connect_data = connectInfo.connector[0]; // TODO: totally 4 connectors?
			info.bPrimary = (info.iPathSubIdx == 0 && m_pathInfo[i].sourceModeInfo->bGDIPrimary);
			memset(info.edid, 0,edid.sizeofEDID);
			memcpy(info.edid, edid.EDID_Data, NV_EDID_DATA_SIZE);
			FILE* fp = fopen("test.edid", "w+");
			if (fp)
			{
				fwrite(info.edid,sizeof(char), NV_EDID_DATA_SIZE,fp);
			}
			fclose(fp);


			FILE* fp2 = fopen("test.edid", "rb");
			char buf[256];
			memset(buf, 0, NV_EDID_DATA_SIZE);
			int len = fread(buf, sizeof(char), NV_EDID_DATA_SIZE, fp2);

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
	for (NvU32 i = 0; i < m_pathCount; i++)
	{
		m_pathInfo[i].version = NV_DISPLAYCONFIG_PATH_INFO_VER;
	}
	
	ret = NvAPI_DISP_GetDisplayConfig(&m_pathCount, m_pathInfo);
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
		}
		else
		{

#ifdef NV_DISPLAYCONFIG_PATH_INFO_VER3
			m_pathInfo[i].sourceModeInfo = (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)malloc(m_pathInfo[i].sourceModeInfoCount * sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
#endif
		}
		if (m_pathInfo[i].sourceModeInfo == NULL)
		{
			return NVAPI_OUT_OF_MEMORY;
		}
		memset(m_pathInfo[i].sourceModeInfo, 0, sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));

		// Allocate the target array
		m_pathInfo[i].targetInfo = (NV_DISPLAYCONFIG_PATH_TARGET_INFO*)malloc(m_pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
		if (m_pathInfo[i].targetInfo == NULL)
		{
			return NVAPI_OUT_OF_MEMORY;
		}
		// Allocate the target details
		memset(m_pathInfo[i].targetInfo, 0, m_pathInfo[i].targetInfoCount * sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
		for (NvU32 j = 0; j < m_pathInfo[i].targetInfoCount; j++)
		{
			m_pathInfo[i].targetInfo[j].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)malloc(sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
			memset(m_pathInfo[i].targetInfo[j].details, 0, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
			m_pathInfo[i].targetInfo[j].details->version = NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER;
		}
	}

	ret = NvAPI_DISP_GetDisplayConfig(&m_pathCount, m_pathInfo);
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