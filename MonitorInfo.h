#pragma once
#include "include\nvapi.h"
#include <iostream>
#include <vector>

const int MAX_OUTPUT_NUM = 4; 
using namespace std;
struct MonitorInfo
{
	NV_GPU_CONNECTOR_DATA connect_data;
	int localIndex;
	NV_POSITION pos;
	bool bPrimary;
	NvS32 nDisplayID;

	NvU16 iPathIdx;
	NvU16 iPathSubIdx; // Default is 0, if > 0, it means pathinfo.targetcount > 1
};

vector<MonitorInfo> disInfo(0);

class DisplayCfg
{
public:
	DisplayCfg() {}
	NvAPI_Status Init();
	int GetPrimaryInfoIndex();
	bool SwapPrimary(int srcLoc, int destLoc);
	bool CloneExtendDisplay(int srcLoc, int destLoc);
	void ShowCurrentDisplayConfig();
private:
	NvU32 pathCount = 0;
	//NV_GPU_DISPLAYIDS  pDisplayIds[MAX_OUTPUT_NUM];
	NV_DISPLAYCONFIG_PATH_INFO m_pathInfo[MAX_OUTPUT_NUM];
	NvU32 m_nDisplayIds = 0;
	NvU32 m_physicalGpuCount = 0;
	NV_GPU_DISPLAYIDS* m_pDisplayIds = NULL;
	NvPhysicalGpuHandle m_hPhysicalGpu[NVAPI_MAX_PHYSICAL_GPUS] = {0};

	NvAPI_Status GetPathInfo();

};