/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/* BombX by Sphenocorona */
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <time.h>
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <string>
#include <vector>

#include <game/server/entities/character.h>
#include <game/server/entities/laser.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "bombx.h"

CGameControllerBOMBX::CGameControllerBOMBX(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "BombX";
	g_Config.m_SvTournamentMode = 0;

	srand(time(0));

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}
void CGameControllerBOMBX::DoWarmup(int Seconds)
{
	IGameController::DoWarmup(Seconds);
	g_Config.m_SvSpectatorSlots = 0;
	GameServer()->ResetBIDs();
	for(int j = 0; j < MAX_CLIENTS; j++) {
		if(GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() == TEAM_SPECTATORS &&
				GameServer()->m_apPlayers[j]->m_PreferredTeam != TEAM_SPECTATORS){
			GameServer()->m_apPlayers[j]->SetTeam(0, 0);
		}
	}
}

void CGameControllerBOMBX::Tick()
{
	// Testing something out
//	GameServer()->GameType();


	// this is the main part of the gamemode, this function is run every tick

	IGameController::Tick();

	printf("BIDs: %o\n", (int) GameServer()->GetBIDs());

	// Allow players to join during the warmup period.
	if(m_Warmup){
		g_Config.m_SvSpectatorSlots = 0;
		GameServer()->ResetBIDs();
	}
	else // However, players cannot join during the actual game.
	{
		EnumerateLivePlayers();
		// prevent new players from joining mid-game
		g_Config.m_SvSpectatorSlots = min(MAX_CLIENTS-m_LivePlayers, MAX_CLIENTS-1);

		// if the bomb exists and has been selected, make their fuse burn
		//TODO: update to handle multiple bombs
		if (!((GameServer()->GetBIDs() == 0) || ((m_LivePlayers - m_NotBombs.size()) < ((m_LivePlayers * 100.f) / g_Config.m_SvBombRatio)))){
			GameServer()->BurnDown();
//			GameServer()->HammerBackTick();

			for (int i = 0; i < MAX_CLIENTS; ++i) {
				if (GameServer()->m_apPlayers[i]) {
					if ((GameServer()->GetFuse(i) <= 0) && GameServer()->IsBomb(i)) {
						GameServer()->m_apPlayers[i]->GetCharacter()->Die(i, WEAPON_GRENADE);
					}
					// attempt to fix BID ghosting that leads to server crashes.
					if (GameServer()->IsBomb(i) && (GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)) {
						GameServer()->RemoveBID(i);
					}
				}

			}
			// old single-bomb version of the section
//			if(GameServer()->GetFuse() > 0) {
//				GameServer()->Fuse();
//				GameServer()->HammerBackTick();
//			} else {
//				if(GameServer()->m_apPlayers[GameServer()->GetBIDs()] && GameServer()->m_apPlayers[GameServer()->GetBIDs()]->GetTeam() != TEAM_SPECTATORS) {
//					GameServer()->m_apPlayers[GameServer()->GetBIDs()]->GetCharacter()->Die(GameServer()->GetBIDs(), WEAPON_GRENADE);
//				}
//			}
		} else { // If it's the start of a round, then make players bombs. Has not been edited yet to work for multiple, though.
			ChooseBomb();
		}

		// Make the bomb player look special.
		//TODO: update to handle multiple bombs
		if (GameServer()->GetBIDs() > 0){

			for (int i = 0; i < MAX_CLIENTS; ++i) {
				if ((GameServer()->GetFuse(i) > 0) && GameServer()->IsBomb(i)) {
					str_copy(GameServer()->m_apPlayers[i]->m_TeeInfos.m_SkinName, "bomb",
							sizeof(GameServer()->m_apPlayers[i]->m_TeeInfos.m_SkinName));

					GameServer()->m_apPlayers[i]->m_TeeInfos.m_ColorBody = 16776960;
					GameServer()->m_apPlayers[i]->m_TeeInfos.m_UseCustomColor = 1;
				}
			}
		}

		//TODO: update to handle multiple bombs
		for(int j = 0; j < MAX_CLIENTS; j++) {
			if(GameServer()->m_apPlayers[j] && !GameServer()->IsBomb(j)) {

				// If the player looks like a bomb, fix that...
				if (strncmp("bomb", GameServer()->m_apPlayers[j]->m_TeeInfos.m_SkinName, 4) == 0)
				{
					str_copy(GameServer()->m_apPlayers[j]->m_TeeInfos.m_SkinName,
							GameServer()->m_apPlayers[j]->m_OriginalSkinName, sizeof(GameServer()->m_apPlayers[j]->m_TeeInfos.m_SkinName));
				}
				if (GameServer()->m_apPlayers[j]->m_StunTick > 0) {
					GameServer()->m_apPlayers[j]->m_TeeInfos.m_ColorBody = 8978178;
					GameServer()->m_apPlayers[j]->m_TeeInfos.m_UseCustomColor = 1;
				} else {
					GameServer()->m_apPlayers[j]->m_TeeInfos.m_ColorBody = 16119285;
					GameServer()->m_apPlayers[j]->m_TeeInfos.m_UseCustomColor = 1;
				}
			}
		}
	}
}

void CGameControllerBOMBX::EnumerateLivePlayers()
{
	m_LivePlayers = 0;
	m_ActivePlayers = 0;
	m_LiveIDs.resize(0);
	m_NotBombs.resize(0);
//	std::fill_n(m_LiveIDs, 16, -1); // fill array with -1's instead of 0's since 0 is a valid player ID
	for(int j = 0; j < MAX_CLIENTS; j++)
	{
		if(GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() != TEAM_SPECTATORS)
		{
			m_LiveIDs.push_back(j);
			m_LivePlayers++;
			m_ActivePlayers++;
			if (!GameServer()->IsBomb(j))
				m_NotBombs.push_back(j);
		}
		else if (GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->m_PreferredTeam != TEAM_SPECTATORS)
		{
			m_ActivePlayers++;
		}
	}
}

//TODO: update to handle multiple bombs
int CGameControllerBOMBX::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);
	EnumerateLivePlayers();
	if(!m_Warmup){
		pVictim->GetPlayer()->SetTeamDirect(TEAM_SPECTATORS);
		if (GameServer()->GetBIDs() > 0){
			if (GameServer()->IsBomb(pVictim->GetPlayer()->GetCID())) {
//				GameServer()->SetBID(-1); // Set the current bomb to undefined, so that the bomb will be reset.
				GameServer()->RemoveBID(pVictim->GetPlayer()->GetCID());
				if(GameServer()->m_apPlayers[pVictim->GetPlayer()->GetCID()]) {
					// Bomb EXPLODE!!!!
					pVictim->MakeDeathGrenade(pKiller);
					pVictim->GetPlayer()->m_Score++; // Neutralize score loss due to blowing up
				}
			}
			else if (Weapon == WEAPON_WORLD || pVictim->GetPlayer()->GetCID() == pKiller->GetCID())
				pVictim->GetPlayer()->m_Score++; // Neutralize score loss due to fall/suicide
		}
	}
	else
	{
		if (pKiller != pVictim->GetPlayer())
			pKiller->m_Score--; // Neutralize score gains during warmups
		if (Weapon == WEAPON_WORLD || pVictim->GetPlayer()->GetCID() == pKiller->GetCID())
					pVictim->GetPlayer()->m_Score++; // Neutralize score loss due to fall/suicide
	}
//	MakeWarningLasers(pKiller);
	printf("BIDs after death: %o\n", (int) GameServer()->GetBIDs());
	return 0;
}

void CGameControllerBOMBX::OnCharacterSpawn(CCharacter *pChr)
{
	// give start equipment
	pChr->IncreaseHealth(10);
	pChr->IncreaseArmor(10);

	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->SetWeapon(WEAPON_HAMMER);
	pChr->GiveWeapon(WEAPON_RIFLE, -1);
}

void CGameControllerBOMBX::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup)
	{
		EnumerateLivePlayers();
		if (m_LivePlayers <= 1 ||
				(g_Config.m_SvTimelimit > 0 && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit*Server()->TickSpeed()*60)) {
			if (m_LivePlayers > 0 && GameServer()->m_apPlayers[m_LiveIDs[0]] &&
					GameServer()->m_apPlayers[m_LiveIDs[0]] == GameServer()->m_apPlayers[m_LiveIDs[0]] && m_ActivePlayers > 1)
			{
				GameServer()->m_apPlayers[m_LiveIDs[0]]->m_Score++; // Reward last surviving player.
			}
			DoWarmup(g_Config.m_SvWarmup);
		}
	}
}

void CGameControllerBOMBX::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
		}
	}
}

void CGameControllerBOMBX::ChooseBomb() {
	int BombChoice = (rand() % m_NotBombs.size());
	GameServer()->SetBID(BombChoice);
	GameServer()->SetFuse(g_Config.m_SvBombFuse*Server()->TickSpeed(), BombChoice);
}

void CGameControllerBOMBX::MakeWarningLasers(class CPlayer *pBomb)
{
	for(int i = 0; i < GameServer()->Collision()->GetWidth(); i++)
	{
		for(int j = 0; j < GameServer()->Collision()->GetHeight(); j++)
		{
			if(GameServer()->Collision()->GetCollisionAt(i*32,j*32)/8 >= m_LivePlayers - 1)
			{
				pBomb->GetCharacter()->MakeLaserDot(i,j);
			}
		}
	}
}
