#include "MonitorInfo.h"

NvAPI_Status DisplayCfg::Init()
{
	NvAPI_Status ret = NVAPI_OK;

	ret = NvAPI_Initialize();
	if (ret != NVAPI_OK)
	{
		cout << "NvAPI_Initialize() Failed, status is:" << ret << endl;
		return ret;
	}

	ret = NvAPI_EnumPhysicalGPUs(m_hPhysicalGpu, &m_physicalGpuCount);
	if (ret != NVAPI_OK)
	{
		cout << "Cannot enumerate GPUs in the system..." <<endl;
		return ret;
	}

	//Currently only one GPU is needed,ingore multi-gpu
	m_pDisplayIds[0].version = NV_GPU_DISPLAYIDS_VER;
	ret = NvAPI_GPU_GetConnectedDisplayIds(m_hPhysicalGpu[0], m_pDisplayIds, &m_nDisplayIds, 0);
	if (ret != NVAPI_OK && m_nDisplayIds)
	{
		cout << "Cannot get connected DisplayIDs" << endl;
		return ret;
	}

	ret = GetPathInfo();
	if (ret != NVAPI_OK)
	{
		cout << "GetPathInfo Failed" << endl;
		return ret;
	}
	//Get localIndex info
	NvDisplayHandle disp = NULL;
	NV_DISPLAY_PORT_INFO port_info;
	port_info.version = NV_DISPLAY_PORT_INFO_VER2;
	NvU32 outputID;
	NV_GPU_CONNECTOR_INFO connectInfo;

	for (int i = 0; i < pathCount; i++)
	{
		for (int j = 0; j < m_pathInfo[i].targetInfoCount; j ++)
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
				cout << "NvAPI_GetAssociatedDisplayOutputId Failed" << endl;
				return ret;
			}

			ret = NvAPI_GetDisplayPortInfo(disp, outputID, &port_info);
			if (ret != NVAPI_OK)
			{
				cout << "NvAPI_GetDisplayPortInfo Failed" << endl;
				return ret;
			}

			ret = NvAPI_GPU_GetConnectorInfo(m_hPhysicalGpu[0], outputID, &connectInfo);
			if (ret != NVAPI_OK)
			{
				cout << "NvAPI_GPU_GetConnectorInfo Failed" << endl;
				return ret;
			}

			MonitorInfo info;
			info.iPathIdx = i;
			info.iPathSubIdx = j;
			info.nDisplayID = displayID;
			info.connect_data = connectInfo.connector[0]; // TODO: totally 4 connectors?
			info.bPrimary = info.iPathSubIdx == 0 && m_pathInfo[i].sourceModeInfo->bGDIPrimary;
		}

	}

	return ret;
}


NvAPI_Status DisplayCfg::GetPathInfo()
{
	/*
	Call NvAPI_DISP_GetDisplayConfig 3 times, 
	1st get pathcount number,
	2nd  set version and malloc
	3rd get all info
	*/
	//First call to get pathCount num
	NvAPI_Status ret = NvAPI_DISP_GetDisplayConfig(&pathCount, NULL);
	if (ret != NVAPI_OK)    
		return ret;

	for (NvU32 i = 0; i < pathCount; i++)
	{
		m_pathInfo[i].version = NV_DISPLAYCONFIG_PATH_INFO_VER;
	}
	memset(m_pathInfo, 0, pathCount * sizeof(NV_DISPLAYCONFIG_PATH_INFO));
	ret = NvAPI_DISP_GetDisplayConfig(&pathCount, m_pathInfo);
	if (ret != NVAPI_OK)
		return ret;

	for (NvU32 i = 0; i < pathCount; i++)
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

	ret = NvAPI_DISP_GetDisplayConfig(&pathCount, m_pathInfo);
	if (ret != NVAPI_OK)
		return ret;

	return NVAPI_OK;
}

int DisplayCfg::GetPrimaryInfoIndex() { return 1; }
bool DisplayCfg::SwapPrimary(int srcLoc, int destLoc) { return true; }
bool DisplayCfg::CloneExtendDisplay(int srcLoc, int destLoc) { return true; }


void DisplayCfg::ShowCurrentDisplayConfig() 
{
	if (pathCount == 1)
	{
		if (m_pathInfo[0].targetInfoCount == 1) // if pathCount = 1 and targetInfoCount =1 it is Single Mode
			cout << "Single MODE";
		else if (m_pathInfo[0].targetInfoCount > 1) // if pathCount >= 1 and targetInfoCount >1 it is Clone Mode
			cout << "Monitors in Clone MODE" <<endl;
	}
	else
	{
		for (NvU32 PathIndex = 0; PathIndex < pathCount; PathIndex++)
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