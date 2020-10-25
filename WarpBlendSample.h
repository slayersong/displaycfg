#ifndef NV_TEST_BACKEND_H
#define NV_TEST_BACKEND_H

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "include\nvapi.h"
using namespace std;

const int MAX_LEN = 256;
const int COMPONENTS_PER_VERTEX = 6;
class CNvImage;

typedef enum
{
	NONE = 0,    	// no feature
	QUERY,			// just query GPU info
	INTENSITY,		// intensity
	WARPING 		// warping
} FeatureMode;

typedef enum
{
	COMMAND_LINE = 0,	// command line input
	MENU				// menu
} InterfaceMode;

typedef struct
{
	NvU32  displayId;
	NvPhysicalGpuHandle gpuHandle;		// this can be queried, but just keep it for efficiency
	NvSBox desktopRect;
	NvSBox scanoutRect;
} DISP_VIEW_PORT;

typedef struct
{
	NvU32                       gpuCount;
	NvPhysicalGpuHandle         nvGPUHandles[NVAPI_MAX_PHYSICAL_GPUS];
	NvU32                       gridCount;
	NV_MOSAIC_GRID_TOPO*        gridTopo;
	std::vector<DISP_VIEW_PORT> displayViewport;
} GPU_INFO;

// per pixel intensity:
// enable  feature: nvPerPixelIntensity.exe -d [displayId] -f [filename]. If no filename is specified, the app will use an automatically generated texture
// disable feature: nvPerPixelIntensity.exe
typedef struct
{
	int           enable;
	int           temporary; // time in seconds until the feature is disabled again (0 == never).
	NvU32         displayId;
	char          filename[MAX_LEN];
	NvU32         width;
	NvU32         height;
	int           bSticky;
	float*        pBlendingTexture;

	void clear()
	{
		enable = 0;
		displayId = 0;
		memset(filename, 0, MAX_LEN);	// **.ppm
		width = 0;
		height = 0;
		pBlendingTexture = NULL;
		bSticky = false;
	}
} INTENSITY_SETTINGS;

// warping
// enable  warping: nvWarping.exe -enable -d [displayId] -f [configure.txt]. If no filename is specified, the feature will not be enabled
// disable warping: nvWarping.exe
typedef struct
{
	int           enable;
	int           temporary; // time in seconds until the feature is disabled again (0 == never).
	NvU32         displayId;
	char          filename[MAX_LEN];			// **.txt
	NvSBox        scanoutRect;
	float*        pVertices;
	NV_GPU_WARPING_VERTICE_FORMAT vertexFormat;
	int           numVertices;
	int           maxNumvertices;
	int           bSticky;

	void clear()
	{
		enable = 0;
		displayId = 0;
		memset(filename, 0, MAX_LEN);			// **.txt
		pVertices = NULL;
		numVertices = 0;
		maxNumvertices = 0;
		bSticky = 0;
	}
} WARPING_SETTINGS;

typedef struct
{
	int           enable;
	int           temporary; // time in seconds until the feature is disabled again (0 == never).
	NvU32         displayId;
	char          filename[MAX_LEN];
	NvU32         width;
	NvU32         height;
	int           bSticky;
	float*        pBlendingTexture;
	NvSBox        scanoutRect;
	float*        pVertices;
	NV_GPU_WARPING_VERTICE_FORMAT vertexFormat;
	int           numVertices;
	int           maxNumvertices;

	void clear()
	{
		enable = 0;
		displayId = 0;
		memset(filename, 0, MAX_LEN);
		width = 0;
		height = 0;
		pBlendingTexture = NULL;
		pVertices = NULL;
		numVertices = 0;
		maxNumvertices = 0;
		bSticky = false;
	}
} SETTINGS;
#endif
