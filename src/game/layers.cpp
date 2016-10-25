/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "layers.h"
#include "gamecore.h"
#include <stdio.h>

CLayers::CLayers()
{
	m_GroupsNum = 0;
	m_GroupsStart = 0;
	m_LayersNum = 0;
	m_LayersStart = 0;
	m_pGameGroup = 0;
	m_pGameLayer = 0;
	m_pMap = 0;
	m_PowLayerExists = 0;
}

//TODO: Fix some parts of the variable layer handling.
void CLayers::Init(class IKernel *pKernel)
{
	int PowLayerName[3];

	StrToInts(PowLayerName, sizeof(PowLayerName)/sizeof(int), "var pow");
	printf("String as ints 1: %d ", PowLayerName[0]);
	printf("String as ints 2: %d ", PowLayerName[1]);
	printf("String as ints 3: %d ", PowLayerName[2]);

	m_pMap = pKernel->RequestInterface<IMap>();
	m_pMap->GetType(MAPITEMTYPE_GROUP, &m_GroupsStart, &m_GroupsNum);
	m_pMap->GetType(MAPITEMTYPE_LAYER, &m_LayersStart, &m_LayersNum);

	for(int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer+l);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);

				printf("NameVar1: %d ",pTilemap->m_aName[0]);
				printf("NameVar2: %d ",pTilemap->m_aName[1]);
				printf("NameVar3: %d ",pTilemap->m_aName[2]);

				if(pTilemap->m_Flags&TILESLAYERFLAG_GAME)
				{
					m_pGameLayer = pTilemap;
					m_pGameGroup = pGroup;

					// make sure the game group has standard settings
					m_pGameGroup->m_OffsetX = 0;
					m_pGameGroup->m_OffsetY = 0;
					m_pGameGroup->m_ParallaxX = 100;
					m_pGameGroup->m_ParallaxY = 100;

					if(m_pGameGroup->m_Version >= 2)
					{
						m_pGameGroup->m_UseClipping = 0;
						m_pGameGroup->m_ClipX = 0;
						m_pGameGroup->m_ClipY = 0;
						m_pGameGroup->m_ClipW = 0;
						m_pGameGroup->m_ClipH = 0;
					}

//					break;
				}
				else if (pTilemap->m_aName[0] == PowLayerName[0]) // if beginning of layer name is "var "
				{
					printf("First part of var pow finder succeeded!");
					switch (pTilemap->m_aName[1]) // only look at first three characters
					{
					case -252708992: // case "pow" (value found using StrToInts. The format is confusing.)
						m_pPowerLayer = pTilemap;
						m_PowLayerExists = true;
						break;
					default:
						break;
					}
				}
			}
		}
	}
}

CMapItemGroup *CLayers::GetGroup(int Index) const
{
	return static_cast<CMapItemGroup *>(m_pMap->GetItem(m_GroupsStart+Index, 0, 0));
}

CMapItemLayer *CLayers::GetLayer(int Index) const
{
	return static_cast<CMapItemLayer *>(m_pMap->GetItem(m_LayersStart+Index, 0, 0));
}


