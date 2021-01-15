#pragma once
#include "include\nvapi.h"
#include <iostream>
#include <vector>
#include<map>
#include <iterator>
const int MAX_OUTPUT_NUM = 4; 
using namespace std;
/*****************
	We store the portindex correspoding infomation in MonitorInfo
	bPrimary: if is primary display
	Pos: position for this display

	iPathIdx: the index of m_pathinfo
	iPathSubIdx: Default is 0, if > 0, it means pathinfo.targetcount > 1
******************/
struct MonitorInfo
{
	//localIndex and NV_GPU_CONNECTOR_TYPE
	NV_GPU_CONNECTOR_DATA connect_data;
	//int localIndex;
	NV_POSITION pos;
	NV_EDID edid;
	bool bPrimary;
	NvU32 nDisplayID;
	NvU16 iPathIdx;  // Index for PathInfo
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

 //DVI  ----> Port 0 Copy DP1, Pos 1920 0
 //DP next to DVI ----> port 1  Pos, 1920 0
 //The last DP  ----> port 2 Primary,   Pos 0,0
//const int K2200_portIndex[3] = { 2, 1, 0 };
const int K2200_portIndex[3] = { 0, 1, 2 };

class DisplayCfg
{
public:
	DisplayCfg() {}
	~DisplayCfg();
	NvAPI_Status Init();
	NvAPI_Status GetDisplayID();
	NvAPI_Status FetchPathInfo();
	int GetPrimaryInfoIndex();
	//checkDisplayPos();

	bool SwapPrimary(int srcLoc, int destLoc);
	bool CloneExtendDisplay(int srcLoc, int destLoc);
	void ShowCurrentDisplayConfig();
	NvAPI_Status GetPortIndex();
	NvAPI_Status ForceEdidByPortIndex(int iPortIndex, const NV_EDID srcEdid);
	NvAPI_Status GetAllDisplayIDs();
	eDisplayState CheckStatus();

	void ForceEdid();
	NvAPI_Status Run(const int* portIndex,int portNum );
private:
	vector<MonitorInfo> m_disInfo;
	/*m_port_mapinfo
	Key:Port Index
	Value: MonitorInfo
	*/
	map<int, MonitorInfo> m_port_mapinfo;

	NvU32 m_pathCount = 0;
	NV_DISPLAYCONFIG_PATH_INFO m_pathInfo[MAX_OUTPUT_NUM];
	// Seems that we need consruct the info
	NV_DISPLAYCONFIG_PATH_INFO m_ToSet_pathInfo[2];

	NvU32 m_nDisplayIds = 0;
	NvU32 m_physicalGpuCount = 0;
	NV_GPU_DISPLAYIDS m_pDisplayIds[8];
	NvU32 m_maxDisplayCount;
	//NV_GPU_DISPLAYIDS* m_pDisplayIds = NULL;
	NvPhysicalGpuHandle m_hPhysicalGpu[NVAPI_MAX_PHYSICAL_GPUS];

	//Src index in PathInfo;
	NvU16 iCloneSrcIndex;
	NvU16 iCondeDstIndex;
};