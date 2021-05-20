#pragma once
#include "nvapi.h"
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


#define DVICONNECT 0x00000001 //Port 0
#define DP1CONNECT 0x00000002 //Port 1
#define DP2CONNECT 0x00000004 //Port 2

 //Port 0 DVI  ---->  Copy DP2, Pos 0 0
 //port 1(the middle one) DP next to DVI ---->  Pos, 1920 0
 //port 2(the outter one) The last DP  ----> Primary,   Pos 0,0 bitmask DP2CONNECT

const int K2200_portIndex[3] = { 2, 1, 0 }; // Primary, extend, clone Primary
const int K2200_bitmask[3] = { DVICONNECT , DP1CONNECT, DP2CONNECT };

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
    bool bNeedConstruct(const int* portIndex, int ConnectStatus);
	bool SwapPrimary(int srcLoc, int destLoc);
	bool CloneExtendDisplay(int srcLoc, int destLoc);
	void ShowCurrentDisplayConfig();
	NvAPI_Status GetPortIndex();
	NvAPI_Status ForceEdidByPortIndex(int iPortIndex, const NV_EDID srcEdid);
	NvAPI_Status GetAllDisplayIDs();
	int CheckStatus();

	void ForceEdid();
	NvAPI_Status Run(const int* portIndex,int ConnectStatus);

	NvAPI_Status Construct_primary(const int portIndex[3], int ConnectStatus);
private:
	int m_connectStatus = 0;
	int connectPort = 0;
	
	vector<MonitorInfo> m_disInfo;
	/*m_port_mapinfo
	Key:Port Index
	Value: MonitorInfo
	*/
	map<int, MonitorInfo> m_port_mapinfo;

	NvU32 m_pathCount = 0;
	NV_DISPLAYCONFIG_PATH_INFO m_pathInfo[MAX_OUTPUT_NUM];
	// Seems that we need consruct the info
	NV_DISPLAYCONFIG_PATH_INFO m_ToSet_pathInfo[MAX_OUTPUT_NUM];

	NV_GPU_DISPLAYIDS m_DisplayIDs[3];
	NvU32 m_nDisplayIds = 0;
	NvU32 m_physicalGpuCount = 0;
	NV_GPU_DISPLAYIDS m_pDisplayIds[8];
	NvU32 m_maxDisplayCount;
	//NV_GPU_DISPLAYIDS* m_pDisplayIds = NULL;
	NvPhysicalGpuHandle m_hPhysicalGpu[NVAPI_MAX_PHYSICAL_GPUS];

	//Src index in PathInfo;
	NvU16 iCloneSrcIndex;
	NvU16 iCondeDstIndex;

	bool bNeedSwapPrimary = false;
	bool bNeedClone = false;
};
