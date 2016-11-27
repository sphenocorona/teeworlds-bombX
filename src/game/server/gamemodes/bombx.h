/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_BOMBX_H
#define GAME_SERVER_GAMEMODES_BOMBX_H
#include <game/server/gamecontroller.h>
#include <game/server/entity.h>
#include <vector>

// you can subclass GAMECONTROLLER_CTF, GAMECONTROLLER_TDM etc if you want
// todo a modification with their base as well.
class CGameControllerBOMBX : public IGameController
{
public:
	CGameControllerBOMBX(class CGameContext *pGameServer);
	virtual void Tick();
	// add more virtual functions here if you wish

	// event
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual void OnCharacterSpawn(CCharacter *pChr);
	virtual void DoWarmup(int Seconds);
	virtual void DoWincheck();
	virtual void PostReset();

	int m_ActivePlayers;
	int m_LivePlayers;
	std::vector<int> m_LiveIDs;
	std::vector<int> m_NotBombs; // Used to make bomb selection easy.
	void EnumerateLivePlayers();
	void ChooseBomb();

	void MakeWarningLasers(class CPlayer *pBomb);
	char m_BombSkin[64];

};
#endif
