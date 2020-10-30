#pragma once
#include "include\nvapi.h"
#include <iostream>
#include <vector>
#include<map>

const int MAX_OUTPUT_NUM = 4; 
using namespace std;
struct MonitorInfo
{
	//localIndex and NV_GPU_CONNECTOR_TYPE
	NV_GPU_CONNECTOR_DATA connect_data;
	//int localIndex;
	NV_POSITION pos;
	bool bPrimary;
	NvU32 nDisplayID;
	char edid[256];
	NvU16 iPathIdx;
	NvU16 iPathSubIdx; // Default is 0, if > 0, it means pathinfo.targetcount > 1
};

enum eDisplayState
{
	DP2Ext_DVICopy = 0,
	Dp2ExtNeedSwap_DVICopy,
	AllExt,
	DP1DvI,			//This means only 2monitors, Dp 1 and 0 DVI
	DP2DVI,			//This means only 2monitors, Dp 1 and 0 DVI
	DP1DP2,			
	Only1Dis		// Only One display connected, wiil do nothing
};
 //DVI  ----> Port 0
 //DP next to DVI ----> port 1
 //The last DP  ----> port 2
const int K2200_portIndex[3] = { 2, 1, 0 };

class DisplayCfg
{
public:
	DisplayCfg() {}
	NvAPI_Status Init();
	NvAPI_Status GetDisplayID();
	NvAPI_Status FetchPathInfo();
	int GetPrimaryInfoIndex();
	//checkDisplayPos();

	bool SwapPrimary(int srcLoc, int destLoc);
	bool CloneExtendDisplay(int srcLoc, int destLoc);
	void ShowCurrentDisplayConfig();
	NvAPI_Status GetPortIndex();
	NvAPI_Status GetAllPortEDID();
	eDisplayState CheckStatus();
private:
	vector<MonitorInfo> m_disInfo;
	map<int, MonitorInfo> m_port_mapinfo;

	NvU32 m_pathCount = 0;
	NV_DISPLAYCONFIG_PATH_INFO m_pathInfo[MAX_OUTPUT_NUM];
	NvU32 m_nDisplayIds = 0;
	NvU32 m_physicalGpuCount = 0;
	//NV_GPU_DISPLAYIDS m_pDisplayIds[MAX_OUTPUT_NUM];
	NV_GPU_DISPLAYIDS* m_pDisplayIds = NULL;
	NvPhysicalGpuHandle m_hPhysicalGpu[NVAPI_MAX_PHYSICAL_GPUS];

};