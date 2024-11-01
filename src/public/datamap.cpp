#include "datamap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

datamap_t *g_pPredDatamapsHead = NULL;
datamap_t *g_pMapDatamapsHead = NULL;

map_datamap_t::map_datamap_t(const char *name)
{
	dataDesc = NULL;
	dataNumFields = 0;

	dataClassName = name;
	baseMap = NULL;

	m_pNext = g_pMapDatamapsHead;
	g_pMapDatamapsHead = this;
}

map_datamap_t::map_datamap_t(const char *name, datamap_t *base)
{
	dataDesc = NULL;
	dataNumFields = 0;

	dataClassName = name;
	baseMap = base;

	m_pNext = g_pMapDatamapsHead;
	g_pMapDatamapsHead = this;
}

pred_datamap_t::pred_datamap_t(const char *name)
{
	dataDesc = NULL;
	dataNumFields = 0;

	dataClassName = name;
	baseMap = NULL;

	m_pNext = g_pPredDatamapsHead;
	g_pPredDatamapsHead = this;
}

pred_datamap_t::pred_datamap_t(const char *name, datamap_t *base)
{
	dataDesc = NULL;
	dataNumFields = 0;

	dataClassName = name;
	baseMap = base;

	m_pNext = g_pPredDatamapsHead;
	g_pPredDatamapsHead = this;
}
